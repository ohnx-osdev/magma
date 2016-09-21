// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "magma_util/platform/platform_device.h"
#include "sys_driver/magma_driver.h"
#include "sys_driver/magma_system_connection.h"
#include "sys_driver/magma_system_context.h"
#include "gtest/gtest.h"

// a class to create and own the command buffer were trying to execute
class CommandBufferHelper {
public:
    static std::unique_ptr<CommandBufferHelper>
    Create(magma::PlatformDevice* platform_device = nullptr)
    {
        auto msd_drv = msd_driver_unique_ptr_t(msd_driver_create(), &msd_driver_destroy);
        if (!msd_drv)
            return DRETP(nullptr, "failed to create msd driver");
        auto msd_dev = msd_driver_create_device(
            msd_drv.get(), platform_device ? platform_device->GetDeviceHandle() : nullptr);
        if (!msd_dev)
            return DRETP(nullptr, "failed to create msd device");
        auto dev = std::unique_ptr<MagmaSystemDevice>(
            new MagmaSystemDevice(msd_device_unique_ptr_t(msd_dev, &msd_device_destroy)));
        uint32_t ctx_id;
        auto connection = dev->Open(0);
        if (!connection)
            return DRETP(nullptr, "failed to connect to msd device");
        connection->CreateContext(&ctx_id);
        auto ctx = connection->LookupContext(ctx_id);
        if (!msd_dev)
            return DRETP(nullptr, "failed to create context");

        return std::unique_ptr<CommandBufferHelper>(new CommandBufferHelper(
            std::move(msd_drv), std::move(dev), std::move(connection), ctx));
    }

    ~CommandBufferHelper()
    {
        for (uint32_t i = 0; i < abi_cmd_buf_->num_resources; i++) {
            delete abi_cmd_buf_->resources[i].relocations;
        }
        delete abi_cmd_buf_->resources;
        delete abi_cmd_buf_;
    }

    magma_system_command_buffer* abi_cmd_buf() { return abi_cmd_buf_; }

    static constexpr uint32_t kNumResources = 3;
    static constexpr uint32_t kBufferSize = 4096;

    std::vector<MagmaSystemBuffer*>& resources() { return resources_; }
    std::vector<msd_buffer*>& msd_resources() { return msd_resources_; }
    msd_context* ctx() { return ctx_->msd_ctx(); }
    MagmaSystemDevice* dev() { return dev_.get(); }

    bool Execute() { return ctx_->ExecuteCommandBuffer(abi_cmd_buf()); }

private:
    CommandBufferHelper(msd_driver_unique_ptr_t msd_drv, std::unique_ptr<MagmaSystemDevice> dev,
                        std::unique_ptr<MagmaSystemConnection> connection, MagmaSystemContext* ctx)
        : msd_drv_(std::move(msd_drv)), dev_(std::move(dev)), connection_(std::move(connection)),
          ctx_(ctx)
    {

        abi_cmd_buf_ = new magma_system_command_buffer();
        abi_cmd_buf_->batch_buffer_resource_index = 0;
        abi_cmd_buf_->num_resources = kNumResources;
        abi_cmd_buf_->resources = new magma_system_exec_resource[kNumResources];

        // batch buffer
        {
            auto batch_buf = &abi_cmd_buf_->resources[0];
            auto buffer = connection_->AllocateBuffer(kBufferSize);
            resources_.push_back(buffer.get());
            batch_buf->buffer_handle = buffer->handle();
            batch_buf->num_relocations = kNumResources - 1;
            batch_buf->relocations = new magma_system_relocation_entry[batch_buf->num_relocations];
            for (uint32_t i = 0; i < batch_buf->num_relocations; i++) {
                auto relocation = &batch_buf->relocations[i];
                relocation->offset =
                    kBufferSize - ((i + 1) * 2 * sizeof(uint32_t)); // every other dword
                relocation->target_resource_index = i;
                relocation->target_offset = kBufferSize / 2; // just relocate right to the middle
                relocation->read_domains_bitfield = MAGMA_DOMAIN_CPU;
                relocation->write_domains_bitfield = MAGMA_DOMAIN_CPU;
            }
        }

        // relocated buffers
        for (uint32_t i = 1; i < kNumResources; i++) {
            auto resource = &abi_cmd_buf_->resources[i];
            auto buffer = connection_->AllocateBuffer(kBufferSize);
            resources_.push_back(buffer.get());
            resource->buffer_handle = buffer->handle();
            resource->num_relocations = 0;
            resource->relocations = nullptr;
        }

        for (auto resource : resources_)
            msd_resources_.push_back(resource->msd_buf());
    }

    msd_driver_unique_ptr_t msd_drv_;
    std::unique_ptr<MagmaSystemDevice> dev_;
    std::unique_ptr<MagmaSystemConnection> connection_;
    MagmaSystemContext* ctx_; // owned by the connection
    std::unique_ptr<CommandBufferHelper> cmd_buf_;

    magma_system_command_buffer* abi_cmd_buf_;
    std::vector<MagmaSystemBuffer*> resources_;
    std::vector<msd_buffer*> msd_resources_;
};
