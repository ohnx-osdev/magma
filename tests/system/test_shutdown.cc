// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <thread>

#include "magenta/magenta_platform_ioctl.h"
#include "magma_system.h"
#include "magma_util/macros.h"
#include "gtest/gtest.h"

class TestBase {
public:
    TestBase() { fd_ = open("/dev/class/display/000", O_RDONLY); }

    int fd() { return fd_; }

    ~TestBase() { close(fd_); }

    void GetDeviceId() { EXPECT_NE(magma_system_get_device_id(fd_), 0u); }

private:
    int fd_;
};

class TestConnection : public TestBase {
public:
    TestConnection() { connection_ = magma_system_open(fd(), MAGMA_SYSTEM_CAPABILITY_RENDERING); }

    ~TestConnection()
    {
        if (connection_)
            magma_system_close(connection_);
    }

    int32_t Test()
    {
        DASSERT(connection_);

        uint32_t context_id;
        magma_system_create_context(connection_, &context_id);

        int32_t result = magma_system_get_error(connection_);
        if (result != 0)
            return DRET(result);

        uint64_t size;
        magma_buffer_t batch_buffer, command_buffer;

        result = magma_system_alloc(connection_, PAGE_SIZE, &size, &batch_buffer);
        if (result != 0)
            return DRET(result);

        result = magma_system_alloc(connection_, PAGE_SIZE, &size, &command_buffer);
        if (result != 0)
            return DRET(result);

        EXPECT_TRUE(InitBatchBuffer(batch_buffer, size));
        EXPECT_TRUE(InitCommandBuffer(command_buffer, batch_buffer, size));

        magma_system_submit_command_buffer(connection_, command_buffer, context_id);

        result = magma_system_get_error(connection_);
        if (result != 0)
            return DRET(result);

        magma_system_wait_rendering(connection_, batch_buffer);

        result = magma_system_get_error(connection_);
        if (result != 0)
            return DRET(result);

        magma_system_destroy_context(connection_, context_id);

        result = magma_system_get_error(connection_);
        if (result != 0)
            return DRET(result);

        return 0;
    }

    bool InitBatchBuffer(magma_buffer_t buffer, uint64_t size)
    {
        if ((magma_system_get_device_id(fd()) >> 8) != 0x19)
            return DRETF(false, "not an intel gen9 device");

        void* vaddr;
        if (magma_system_map(connection_, buffer, &vaddr) != 0)
            return DRETF(false, "couldn't map batch buffer");

        memset(vaddr, 0, size);

        // Intel end-of-batch
        *reinterpret_cast<uint32_t*>(vaddr) = 0xA << 23;

        EXPECT_EQ(magma_system_unmap(connection_, buffer), 0);

        return true;
    }

    bool InitCommandBuffer(magma_buffer_t buffer, magma_buffer_t batch_buffer,
                           uint64_t batch_buffer_length)
    {
        void* vaddr;
        if (magma_system_map(connection_, buffer, &vaddr) != 0)
            return DRETF(false, "couldn't map command buffer");

        auto command_buffer = reinterpret_cast<struct magma_system_command_buffer*>(vaddr);
        command_buffer->batch_buffer_resource_index = 0;
        command_buffer->batch_start_offset = 0;
        command_buffer->num_resources = 1;

        auto exec_resource =
            reinterpret_cast<struct magma_system_exec_resource*>(command_buffer + 1);
        exec_resource->buffer_id = magma_system_get_buffer_id(batch_buffer);
        exec_resource->num_relocations = 0;
        exec_resource->offset = 0;
        exec_resource->length = batch_buffer_length;

        EXPECT_EQ(magma_system_unmap(connection_, buffer), 0);

        return true;
    }

private:
    magma_system_connection* connection_;
};

static std::atomic_uint complete_count;
static constexpr uint32_t kMaxCount = 100;
static constexpr uint32_t kRestartCount = kMaxCount / 10;

static void looper_thread_entry()
{
    std::unique_ptr<TestConnection> test(new TestConnection());
    while (complete_count < kMaxCount) {
        int32_t result = test->Test();
        if (result == 0) {
            complete_count++;
        } else {
            EXPECT_EQ(-EINVAL, result);
            test.reset(new TestConnection());
        }
    }
}

TEST(Shutdown, Test)
{
    std::thread looper(looper_thread_entry);
    std::thread looper2(looper_thread_entry);

    TestBase test_base;

    uint32_t count = kRestartCount;
    while (complete_count < kMaxCount) {
        if (complete_count > count) {
            // Should replace this with a request to devmgr to restart the driver
            EXPECT_EQ(mxio_ioctl(test_base.fd(), IOCTL_MAGMA_TEST_RESTART, nullptr, 0, nullptr, 0),
                      0);
            count += kRestartCount;
        }
        std::this_thread::yield();
    }

    looper.join();
    looper2.join();
}
