// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _MAGMA_SYSTEM_DEVICE_H_
#define _MAGMA_SYSTEM_DEVICE_H_

#include "magma_system_connection.h"
#include "msd.h"
#include "platform_connection.h"
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

using msd_device_unique_ptr_t = std::unique_ptr<msd_device, decltype(&msd_device_destroy)>;

static inline msd_device_unique_ptr_t MsdDeviceUniquePtr(msd_device* msd_dev)
{
    return msd_device_unique_ptr_t(msd_dev, &msd_device_destroy);
}

class MagmaSystemDevice : public MagmaSystemConnection::Owner {
public:
    MagmaSystemDevice(msd_device_unique_ptr_t msd_dev) : msd_dev_(std::move(msd_dev)) {}

    // Opens a connection to the device. On success |connection_handle_out| will contain the
    // connection handle to be passed to the client
    std::unique_ptr<magma::PlatformConnection> Open(msd_client_id client_id, uint32_t capabilities);

    msd_device* msd_dev() { return msd_dev_.get(); }

    // MagmaSystemConnection::Owner
    // Returns the device id. 0 is invalid.
    uint32_t GetDeviceId() override;

    // MagmaSystemDisplay::Owner
    void PageFlip(std::shared_ptr<MagmaSystemBuffer> buf, magma_system_pageflip_callback_t callback,
                  void* data) override;

    // MagmaSystemBufferManager::Owner
    std::shared_ptr<MagmaSystemBuffer> GetBufferForHandle(uint32_t handle) override;
    void ReleaseBuffer(uint64_t id) override;

private:
    msd_device_unique_ptr_t msd_dev_;
    std::unordered_map<uint64_t, std::weak_ptr<MagmaSystemBuffer>> buffer_map_;
    std::mutex buffer_map_mutex_;
};

#endif //_MAGMA_SYSTEM_DEVICE_H_