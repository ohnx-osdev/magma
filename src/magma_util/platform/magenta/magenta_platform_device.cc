// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/device.h>
#include <ddk/protocol/pci.h>
#include <magenta/process.h>

#include "magenta_platform_device.h"
#include "magenta_platform_interrupt.h"
#include "magma_util/dlog.h"
#include "magma_util/macros.h"
#include "platform_mmio.h"

namespace magma {

static_assert(MX_CACHE_POLICY_CACHED == static_cast<int>(PlatformMmio::CACHE_POLICY_CACHED),
              "enum mismatch");
static_assert(MX_CACHE_POLICY_UNCACHED == static_cast<int>(PlatformMmio::CACHE_POLICY_UNCACHED),
              "enum mismatch");
static_assert(MX_CACHE_POLICY_UNCACHED_DEVICE ==
                  static_cast<int>(PlatformMmio::CACHE_POLICY_UNCACHED_DEVICE),
              "enum mismatch");
static_assert(MX_CACHE_POLICY_WRITE_COMBINING ==
                  static_cast<int>(PlatformMmio::CACHE_POLICY_WRITE_COMBINING),
              "enum mismatch");

class MagentaPlatformMmio : public PlatformMmio {
public:
    MagentaPlatformMmio(void* addr, uint64_t size, mx_handle_t handle)
        : PlatformMmio(addr, size), handle_(handle)
    {
    }

    ~MagentaPlatformMmio()
    {
        DLOG("MagentaPlatformMmio dtor");
        mx_status_t status = mx_vmar_unmap(mx_vmar_root_self(), reinterpret_cast<uintptr_t>(addr()), size());
        if (status != NO_ERROR) {
            DLOG("error unmapping %p (len %zu): %d\n", addr(), size(), status);
        }
        mx_handle_close(handle_);
    }

private:
    mx_handle_t handle_;
};

std::unique_ptr<PlatformMmio>
MagentaPlatformDevice::CpuMapPciMmio(unsigned int pci_bar, PlatformMmio::CachePolicy cache_policy)
{
    DLOG("CpuMapPciMmio bar %d, cache policy %u", pci_bar, cache_policy);

    if (!pci()) {
        return DRETP(nullptr, "couldn't get pci protocol");
    }

    void* addr;
    uint64_t size;
    mx_handle_t handle;
    mx_status_t status = pci()->map_resource(mx_device(), pci_bar, cache_policy, &addr, &size, &handle);
    if (status != NO_ERROR) {
        DLOG("map_resource returned %d\n", status);
        return DRETP(nullptr, "map_resource failed");
    }

    std::unique_ptr<MagentaPlatformMmio> mmio(new MagentaPlatformMmio(addr, size, handle));

    DLOG("map_mmio bar %d cache_policy %d returned: 0x%x", pci_bar, static_cast<int>(cache_policy),
         handle);

    return mmio;
}

bool MagentaPlatformDevice::ReadPciConfig16(uint64_t addr, uint16_t* value)
{
    if (!value || !pci_config() || addr >= cfg_size_) {
        return false;
    }

    *value =
        *reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(pci_config()) + addr);

    return true;
}

std::unique_ptr<PlatformInterrupt> MagentaPlatformDevice::RegisterInterrupt()
{
    if (!pci()) {
        return DRETP(nullptr, "could not get pci protocol");
    }

    uint32_t max_irqs;
    mx_status_t status = pci()->query_irq_mode_caps(mx_device(), MX_PCIE_IRQ_MODE_LEGACY, &max_irqs);
    if (status != NO_ERROR)
        return DRETP(nullptr, "query_irq_mode_caps failed (%d)", status);

    if (max_irqs == 0)
        return DRETP(nullptr, "max_irqs is zero");

    // Mode must be Disabled before we can request Legacy
    status = pci()->set_irq_mode(mx_device(), MX_PCIE_IRQ_MODE_DISABLED, 0);
    if (status != NO_ERROR)
        return DRETP(nullptr, "set_irq_mode(DISABLED) failed (%d)", status);

    status = pci()->set_irq_mode(mx_device(), MX_PCIE_IRQ_MODE_LEGACY, 1);
    if (status != NO_ERROR)
        return DRETP(nullptr, "set_irq_mode(LEGACY) failed (%d)", status);

    mx_handle_t interrupt_handle;
    status = pci()->map_interrupt(mx_device(), 0, &interrupt_handle);
    if (status < 0)
        return DRETP(nullptr, "map_interrupt failed (%d)", status);

    return std::make_unique<MagentaPlatformInterrupt>(interrupt_handle);
}

std::unique_ptr<PlatformDevice> PlatformDevice::Create(void* device_handle)
{
    if (!device_handle)
        return DRETP(nullptr, "device_handle is null, cannot create PlatformDevice");
    return std::unique_ptr<PlatformDevice>(
        new MagentaPlatformDevice(reinterpret_cast<mx_device_t*>(device_handle)));
}

} // namespace
