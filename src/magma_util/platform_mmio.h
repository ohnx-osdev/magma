// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PLATFORM_MMIO_H
#define PLATFORM_MMIO_H

#include "magma_util/dlog.h"
#include "magma_util/macros.h"
#include <memory>
#include <stdint.h>

namespace magma {

// Created from a PlatformDevice.
class PlatformMmio {
public:
    PlatformMmio(void* addr, uint64_t size) : addr_(addr), size_(size) {}

    virtual ~PlatformMmio() {}

    enum CachePolicy {
        CACHE_POLICY_CACHED = 0,
        CACHE_POLICY_UNCACHED = 1,
        CACHE_POLICY_UNCACHED_DEVICE = 2,
        CACHE_POLICY_WRITE_COMBINING = 3,
    };

    void Write32(uint32_t val, uint64_t offset)
    {
        DASSERT(offset < size());
        DASSERT((offset & 0x3) == 0);
        *reinterpret_cast<uint32_t*>(addr(offset)) = val;
    }

    uint32_t Read32(uint64_t offset)
    {
        DASSERT(offset < size());
        DASSERT((offset & 0x3) == 0);
        return *reinterpret_cast<uint32_t*>(addr(offset));
    }

    void Write64(uint64_t val, uint64_t offset)
    {
        DASSERT(offset < size());
        DASSERT((offset & 0x7) == 0);
        *reinterpret_cast<uint64_t*>(addr(offset)) = val;
    }

    uint64_t Read64(uint64_t offset)
    {
        DASSERT(offset < size());
        DASSERT((offset & 0x7) == 0);
        return *reinterpret_cast<uint64_t*>(addr(offset));
    }

    void* addr() { return addr_; }
    uint64_t size() { return size_; }

private:
    void* addr(uint64_t offset)
    {
        DASSERT(offset < size_);
        return reinterpret_cast<uint8_t*>(addr_) + offset;
    }

    void* addr_;
    uint64_t size_;

    DISALLOW_COPY_AND_ASSIGN(PlatformMmio);
};

} // namespace magma

#endif // PLATFORM_MMIO_H
