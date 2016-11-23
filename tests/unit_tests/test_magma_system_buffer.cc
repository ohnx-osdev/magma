// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mock/mock_msd.h"
#include "sys_driver/magma_system_connection.h"
#include "sys_driver/magma_system_device.h"
#include "gtest/gtest.h"

class MsdMockBufferManager_Create : public MsdMockBufferManager {
public:
    MsdMockBufferManager_Create() : has_created_buffer_(false), has_destroyed_buffer_(false) {}

    MsdMockBuffer* CreateBuffer(uint32_t handle)
    {
        has_created_buffer_ = true;
        return MsdMockBufferManager::CreateBuffer(handle);
    }

    void DestroyBuffer(MsdMockBuffer* buf)
    {
        has_destroyed_buffer_ = true;
        MsdMockBufferManager::DestroyBuffer(buf);
    }

    bool has_created_buffer() { return has_created_buffer_; };
    bool has_destroyed_buffer() { return has_destroyed_buffer_; };

private:
    bool has_created_buffer_;
    bool has_destroyed_buffer_;
};

TEST(MagmaSystemBuffer, Create)
{
    auto scoped_bufmgr = MsdMockBufferManager::ScopedMockBufferManager(
        std::unique_ptr<MsdMockBufferManager_Create>(new MsdMockBufferManager_Create()));

    auto bufmgr = static_cast<MsdMockBufferManager_Create*>(scoped_bufmgr.get());

    auto msd_drv = msd_driver_create();
    auto msd_dev = msd_driver_create_device(msd_drv, nullptr);
    MagmaSystemDevice dev(MsdDeviceUniquePtr(msd_dev));
    auto msd_connection = msd_device_open(msd_dev, 0);
    ASSERT_NE(msd_connection, nullptr);
    auto connection = std::unique_ptr<MagmaSystemConnection>(new MagmaSystemConnection(
        &dev, MsdConnectionUniquePtr(msd_connection), MAGMA_SYSTEM_CAPABILITY_RENDERING));
    ASSERT_NE(connection, nullptr);

    EXPECT_FALSE(bufmgr->has_created_buffer());
    EXPECT_FALSE(bufmgr->has_destroyed_buffer());

    {
        auto buf = magma::PlatformBuffer::Create(256);

        uint32_t duplicate_handle;
        ASSERT_TRUE(buf->duplicate_handle(&duplicate_handle));

        uint64_t id;
        EXPECT_TRUE(connection->ImportBuffer(duplicate_handle, &id));
        EXPECT_TRUE(bufmgr->has_created_buffer());
        EXPECT_FALSE(bufmgr->has_destroyed_buffer());

        EXPECT_TRUE(connection->ReleaseBuffer(id));
    }
    EXPECT_TRUE(bufmgr->has_created_buffer());
    EXPECT_TRUE(bufmgr->has_destroyed_buffer());

    msd_driver_destroy(msd_drv);
}