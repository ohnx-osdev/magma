// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "magma_buffer.h"
#include "magma_device.h"

MagmaBuffer::MagmaBuffer(MagmaDevice* device, const char* name, uint32_t align)
    : LibdrmIntelGen::Buffer(name, align), device_(device)
{
    magic_ = kMagic;
}

MagmaBuffer::~MagmaBuffer()
{
    DLOG("~MagmaBuffer");
    magma_system_free(device_->sys_dev(), this->handle);

    this->handle = 0xdeadbeef;
    this->size = 0;
}

bool MagmaBuffer::Alloc(uint64_t size)
{
    uint32_t handle;
    if (!magma_system_alloc(device_->sys_dev(), size, &size, &handle))
        return false;

    this->handle = static_cast<uint32_t>(handle);
    this->size = size;
    this->virt = nullptr;
    this->offset64 = 0;

    return true;
}

void MagmaBuffer::SetTilingMode(uint32_t tiling_mode)
{
    if (magma_system_set_tiling_mode(device_->sys_dev(), this->handle, tiling_mode))
        tiling_mode_ = tiling_mode;
}

bool MagmaBuffer::Map(bool write)
{
    void* addr;
    if (!magma_system_map(device_->sys_dev(), this->handle, &addr))
        return false;

    this->virt = addr;

    if (!magma_system_set_domain(device_->sys_dev(), this->handle, MAGMA_DOMAIN_CPU,
                                 write ? MAGMA_DOMAIN_CPU : 0))
        return false;

    return true;
}

bool MagmaBuffer::Unmap()
{
    if (!magma_system_unmap(device_->sys_dev(), this->handle, this->virt))
        return false;

    this->virt = nullptr;
    return true;
}

void MagmaBuffer::WaitRendering()
{
    return magma_system_wait_rendering(device_->sys_dev(), this->handle);
}