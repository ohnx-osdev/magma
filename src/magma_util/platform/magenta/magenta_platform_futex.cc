// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "magenta/syscalls.h"
#include "platform_futex.h"

namespace magma {

static_assert(sizeof(mx_futex_t) == sizeof(uint32_t), "futex type incompatible size");

bool PlatformFutex::Wake(uint32_t* value_ptr, int32_t wake_count)
{
    mx_status_t status;
    if ((status = mx_futex_wake(reinterpret_cast<mx_futex_t*>(value_ptr), wake_count)) != MX_OK)
        return DRETF(false, "mx_futex_wake failed: %d", status);
    return true;
}

bool PlatformFutex::Wait(uint32_t* value_ptr, int32_t current_value, uint64_t timeout_ns,
                         WaitResult* result_out)
{
    const mx_time_t deadline = (timeout_ns != MX_TIME_INFINITE) ? mx_deadline_after(timeout_ns) :
            MX_TIME_INFINITE;
    mx_status_t status =
        mx_futex_wait(reinterpret_cast<mx_futex_t*>(value_ptr), current_value, deadline);
    switch (status) {
        case MX_OK:
            *result_out = AWOKE;
            break;
        case MX_ERR_TIMED_OUT:
            *result_out = TIMED_OUT;
            break;
        case MX_ERR_BAD_STATE:
            *result_out = RETRY;
            break;
        default:
            return DRETF(false, "mx_futex_wait returned: %d", status);
    }
    return true;
}

} // namespace
