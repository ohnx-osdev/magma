// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "magma_util/platform/platform_buffer.h"
#include "msd.h"
#include "gtest/gtest.h"

TEST(MsdBuffer, ImportAndDestroy)
{
    auto platform_buf = magma::PlatformBuffer::Create(4096);
    ASSERT_NE(platform_buf, nullptr);

    uint32_t duplicate_handle;
    ASSERT_TRUE(platform_buf->duplicate_handle(&duplicate_handle));

    auto msd_buffer = msd_buffer_import(duplicate_handle);
    ASSERT_NE(msd_buffer, nullptr);

    msd_buffer_destroy(msd_buffer);
}