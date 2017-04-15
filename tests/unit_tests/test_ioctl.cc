// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"

#include "magma_util/platform/magenta/magenta_platform_ioctl.h"
#include <magenta/device/display.h>
#include <mxio/io.h>

constexpr char kDisplayPath[] = "/dev/class/display/000";

TEST(MagmaIoctl, DISPLAY_GET_FB)
{
    int fd = open(kDisplayPath, O_RDWR);
    ASSERT_GE(fd, 0);
    ioctl_display_get_fb_t description;
    ssize_t result = ioctl_display_get_fb(fd, &description);
    ASSERT_EQ(result, static_cast<ssize_t>(sizeof(description)));
    EXPECT_NE(MX_HANDLE_INVALID, description.vmo);
    EXPECT_GE(description.info.width, 0u);
    EXPECT_GE(description.info.height, 0u);

    close(fd);
}

TEST(MagmaIoctl, GET_TRACE_MANAGER_CHANNEL)
{
    int fd = open(kDisplayPath, O_RDWR);
    ASSERT_GE(fd, 0);

    mx_handle_t handle = MX_HANDLE_INVALID;
    int ret =
        mxio_ioctl(fd, IOCTL_MAGMA_GET_TRACE_MANAGER_CHANNEL, nullptr, 0, &handle, sizeof(handle));
    close(fd);
    EXPECT_EQ(ret, 4);
    EXPECT_NE(0, handle);
}