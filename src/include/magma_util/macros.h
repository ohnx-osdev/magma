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

#ifndef MACROS_H_
#define MACROS_H_

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

namespace magma {

#define DASSERT(...)                                                                               \
    if (!(__VA_ARGS__)) {                                                                          \
        printf("%s:%d ASSERT\n", __FILE__, __LINE__);                                              \
        assert(false); \
    }

static inline int dret(const char* file, int line, int ret)
{
    printf("%s:%d returning error: %d", file, line, ret);
    printf("\n");
    return ret;
}

#define DRET(ret) (ret == 0 ? 0 : magma::dret(__FILE__, __LINE__, ret))

#define UNIMPLEMENTED(...)                                                                         \
    do {                                                                                           \
        DLOG("UNIMPLEMENTED: " #__VA_ARGS__);                                                      \
        DASSERT(false);                                                                             \
    } while (0)

static inline uint32_t upper_32_bits(uint64_t n) { return static_cast<uint32_t>(n >> 32); }

static inline uint32_t lower_32_bits(uint64_t n) { return static_cast<uint32_t>(n); }

static inline bool is_broadwell_gt3(uint32_t device_id) { return (device_id & 0xFFF0) == 0x1620; }

static inline uint32_t round_up_pot_32(uint32_t val, uint32_t power_of_two)
{
    assert(power_of_two % 2 == 0);
    return ((val - 1) | (power_of_two - 1)) + 1;
}

static inline uint64_t round_up_64(uint64_t x, uint64_t y) { return ((x + (y - 1)) / y) * y; }

static inline uint8_t inb(uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__("inb %1, %0"
                         : "=a"(rv)
                         : "d"(_port));
    return (rv);
}

static inline void outb(uint8_t _data, uint16_t _port) {
    __asm__ __volatile__("outb %1, %0"
                         :
                         : "d"(_port),
                           "a"(_data));
}

} // namespace magma

#endif // MACROS_H_
