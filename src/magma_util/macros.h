// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MACROS_H_
#define MACROS_H_

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h> // PAGE_SIZE

#define MAGMA_DEBUG 0

#define MAGMA_DRET_ENABLE MAGMA_DEBUG

namespace magma {

#define DASSERT(...)                                                                               \
    if (!(__VA_ARGS__)) {                                                                          \
        printf("%s:%d ASSERT\n", __FILE__, __LINE__);                                              \
        assert(false);                                                                             \
    }

#if MAGMA_DRET_ENABLE

static inline int dret(const char* file, int line, int ret)
{
    printf("%s:%d returning error: %d\n", file, line, ret);
    return ret;
}

#define DRET(ret) (ret == 0 ? 0 : magma::dret(__FILE__, __LINE__, ret))

static inline bool dret_false(const char* file, int line, const char* msg)
{
    printf("%s:%d returning false: %s\n", file, line, msg);
    return false;
}

#define DRETF(ret, msg) (ret == true ? true : magma::dret_false(__FILE__, __LINE__, msg))

static inline void dret_null(const char* file, int line, const char* msg)
{
    printf("%s:%d returning null: %s\n", file, line, msg);
}

#define DRETP(ret, msg) (ret == nullptr ? magma::dret_false(__FILE__, __LINE__, msg), ret : ret)

#else
#define DRET(ret) (ret)
#define DRETF(ret, msg) (ret)
#define DRETP(ret, msg) (ret)
#endif

#define UNIMPLEMENTED(...)                                                                         \
    do {                                                                                           \
        DLOG("UNIMPLEMENTED: " #__VA_ARGS__);                                                      \
        DASSERT(false);                                                                            \
    } while (0)

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                         \
    TypeName(const TypeName&) = delete;                                                            \
    void operator=(const TypeName&) = delete
#endif

static inline bool is_page_aligned(uint64_t val) { return (val & (PAGE_SIZE - 1)) == 0; }

static inline uint32_t upper_32_bits(uint64_t n) { return static_cast<uint32_t>(n >> 32); }

static inline uint32_t lower_32_bits(uint64_t n) { return static_cast<uint32_t>(n); }

static inline bool get_pow2(uint64_t val, uint64_t* pow2_out)
{
    if (val == 0)
        return DRETF(false, "zero is not a power of two");

    uint64_t result = 0;
    while ((val & 1) == 0) {
        val >>= 1;
        result++;
    }
    if (val >> 1)
        return DRETF(false, "not a power of 2");

    *pow2_out = result;
    return true;
}

static inline bool is_broadwell_gt3(uint32_t device_id) { return (device_id & 0xFFF0) == 0x1620; }

static inline uint32_t round_up_pot_32(uint32_t val, uint32_t power_of_two)
{
    assert(power_of_two % 2 == 0);
    return ((val - 1) | (power_of_two - 1)) + 1;
}

static inline uint64_t round_up_64(uint64_t x, uint64_t y) { return ((x + (y - 1)) / y) * y; }

static inline uint8_t inb(uint16_t _port)
{
    uint8_t rv;
    __asm__ __volatile__("inb %1, %0" : "=a"(rv) : "d"(_port));
    return (rv);
}

static inline void outb(uint8_t _data, uint16_t _port)
{
    __asm__ __volatile__("outb %1, %0" : : "d"(_port), "a"(_data));
}

} // namespace magma

#endif // MACROS_H_
