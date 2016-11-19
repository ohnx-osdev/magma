// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_thread.h"
#include "gtest/gtest.h"
#include <thread>

class TestPlatformThread {
public:
    static void ThreadFunc(magma::PlatformThreadId* thread_id)
    {
        EXPECT_FALSE(thread_id->IsCurrent());
    }

    static void Test()
    {
        magma::PlatformThreadId thread_id;
        EXPECT_TRUE(thread_id.IsCurrent());

        std::thread thread(ThreadFunc, &thread_id);
        thread.join();
    }
};

TEST(PlatformThread, Test) { TestPlatformThread::Test(); }
