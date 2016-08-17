// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _MAGMA_BUFFER_H_
#define _MAGMA_BUFFER_H_

#include "magma.h"
#include "magma_system.h"
#include "magma_util/refcounted.h"

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

    MagmaConnection* connection_;

    BufferRefcount* refcount_;
    uint32_t alignment_{};

    uint32_t tiling_mode_ = MAGMA_TILING_MODE_NONE;

    static const uint32_t kMagic = 0x62756666; //"buff"
};

#endif //_MAGMA_BUFFER_H_