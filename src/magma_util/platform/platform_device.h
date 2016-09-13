// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_DEVICE_H
#define PLATFORM_DEVICE_H

#include "magma_util/dlog.h"
#include "platform_mmio.h"
#include <memory>

namespace magma {

class PlatformDevice {
public:
    virtual ~PlatformDevice() { DLOG("PlatformDevice dtor"); }

    virtual void* GetDeviceHandle() = 0;

    virtual bool ReadPciConfig16(uint64_t addr, uint16_t* value)
    {
        DLOG("ReadPciConfig16 unimplemented");
        return false;
    }

    virtual std::unique_ptr<PlatformMmio> CpuMapPciMmio(unsigned int pci_bar,
                                                        PlatformMmio::CachePolicy cache_policy)
    {
        DLOG("CpuMapPciMmio unimplemented");
        return nullptr;
    }

    static std::unique_ptr<PlatformDevice> Create(void* device_handle);
};

} // namespace magma

#endif // PLATFORM_DEVICE_H
