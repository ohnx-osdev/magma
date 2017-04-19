// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MAGENTA_PLATFORM_DEVICE_H
#define MAGENTA_PLATFORM_DEVICE_H

#include "platform_device.h"

#include <ddk/device.h>

namespace magma {

class MagentaPlatformDevice : public PlatformDevice {
public:
    MagentaPlatformDevice(mx_device_t* mx_device) : mx_device_(mx_device) {
        void* protocol;
        mx_status_t status = device_op_get_protocol(mx_device_, MX_PROTOCOL_PCI, &protocol);
        if (status != NO_ERROR) {
            return;
        }

        pci_ = reinterpret_cast<pci_protocol_t*>(protocol);

        status = pci_->map_resource(mx_device_, PCI_RESOURCE_CONFIG, MX_CACHE_POLICY_UNCACHED_DEVICE,
                                   (void**)&cfg_, &cfg_size_, &cfg_handle_);
        if (status != NO_ERROR) {
            cfg_ = nullptr;
        }
    }

    void* GetDeviceHandle() override { return mx_device_; }

    bool ReadPciConfig16(uint64_t addr, uint16_t* value) override;

    std::unique_ptr<PlatformMmio> CpuMapPciMmio(unsigned int pci_bar,
                                                PlatformMmio::CachePolicy cache_policy) override;

    std::unique_ptr<PlatformInterrupt> RegisterInterrupt() override;

private:
    mx_device_t* mx_device() const { return mx_device_; }
    pci_protocol_t* pci() const { return pci_; }
    pci_config_t* pci_config() const { return cfg_; }


    mx_device_t* mx_device_ = nullptr;
    pci_protocol_t* pci_ = nullptr;
    pci_config_t* cfg_ = nullptr;
    mx_handle_t cfg_handle_;
    size_t cfg_size_;
};

} // namespace

#endif // MAGENTA_PLATFORM_DEVICE_H
