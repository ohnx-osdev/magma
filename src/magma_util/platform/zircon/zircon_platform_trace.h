// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_PLATFORM_TRACE_H
#define ZIRCON_PLATFORM_TRACE_H

#if MAGMA_ENABLE_TRACING
#include <async/loop.h>
#include <trace-provider/provider.h>
#endif

#include "platform_trace.h"

namespace magma {

#if MAGMA_ENABLE_TRACING
class ZirconPlatformTrace : public PlatformTrace {
public:
    ZirconPlatformTrace();

private:
    async::Loop loop_;
    trace::TraceProvider trace_provider_;
};
#endif

} // namespace magma

#endif // ZIRCON_PLATFORM_TRACE_H
