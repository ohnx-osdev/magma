// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _MAGMA_BUFFER_H_
#define _MAGMA_BUFFER_H_

#include "magma.h"
#include "magma_system.h"
#include "magma_util/refcounted.h"

#include <memory>
#include <vector>

class MagmaConnection;

// Magma is based on intel libdrm.
// LibdrmIntelGen buffers are based on the api exposed magma_buffer.
class MagmaBuffer : public magma_buffer {
public:
    MagmaBuffer(MagmaConnection* connection, const char* name, uint32_t alignment);
    ~MagmaBuffer();

    bool Alloc(uint64_t size);
    bool Map(bool write);
    bool Unmap();
    void WaitRendering();

    MagmaConnection* connection() { return connection_; }

    void SetTilingMode(uint32_t tiling_mode);
    uint32_t tiling_mode() { return tiling_mode_; }

    static MagmaBuffer* cast(magma_buffer* buffer)
    {
        DASSERT(buffer);
        DASSERT(buffer->magic_ == kMagic);
        return static_cast<MagmaBuffer*>(buffer);
    }

    uint64_t alignment() { return alignment_; }

    const char* Name() { return refcount_->name(); }
    void Incref() { return refcount_->Incref(); }
    void Decref() { return refcount_->Decref(); }

    void EmitRelocation(uint32_t offset, MagmaBuffer* target, uint32_t target_offset,
                        uint32_t read_domains_bitfield, uint32_t write_domains_bitfield);
    void ClearRelocations(uint32_t start)
    {
        if (start > relocations_.size())
            return;
        relocations_.erase(relocations_.begin() + start, relocations_.end());
    }
    uint32_t RelocationCount() { return relocations_.size(); }

private:
    class BufferRefcount : public magma::Refcounted {
    public:
        BufferRefcount(const char* name, MagmaBuffer* buffer)
            : magma::Refcounted(name), buffer_(buffer)
        {
        }

        virtual void Delete()
        {
            delete buffer_;
            delete this;
        }

    private:
        MagmaBuffer* buffer_;
    };

    class RelocationEntry {
    public:
        RelocationEntry(uint32_t offset, MagmaBuffer* target, uint32_t target_offset,
                        uint32_t read_domains_bitfield, uint32_t write_domains_bitfield)
            : offset_(offset), target_(target), target_offset_(target_offset),
              read_domains_bitfield_(read_domains_bitfield),
              write_domains_bitfield_(write_domains_bitfield)
        {
        }

        void GetAbiRelocationEntry(magma_system_relocation_entry* abi_reloc_out)
        {
            abi_reloc_out->offset = offset_;
            abi_reloc_out->target_buffer = target_->handle;
            abi_reloc_out->target_offset = target_offset_;
            abi_reloc_out->read_domains_bitfield = read_domains_bitfield_;
            abi_reloc_out->write_domains_bitfield = write_domains_bitfield_;
        }

    private:
        uint32_t offset_;
        MagmaBuffer* target_;
        uint32_t target_offset_;
        uint32_t read_domains_bitfield_;
        uint32_t write_domains_bitfield_;
    };

    std::vector<RelocationEntry> relocations_;

    MagmaConnection* connection_;

    BufferRefcount* refcount_;
    uint32_t alignment_{};

    uint32_t tiling_mode_ = MAGMA_TILING_MODE_NONE;

    static const uint32_t kMagic = 0x62756666; //"buff"
};

#endif //_MAGMA_BUFFER_H_