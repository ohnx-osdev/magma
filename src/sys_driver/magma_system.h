// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _MAGMA_SYSTEM_H_
#define _MAGMA_SYSTEM_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct magma_system_connection {
    uint32_t magic_;
};

// a relocation entry that informs the system driver how to patch GPU virtual addresses
// in an exec resource. The 32 bit word at offset in the buffer will be overwritten with
// the GPU virtual address of the 32 bit word at target_offset in target_buffer.
struct magma_system_relocation_entry {
    uint32_t offset;                 // offset in the batch buffer
    uint32_t target_buffer;          // buffer which we are getting addresses from
    uint32_t target_offset;          // offset in the target buffer
    uint32_t read_domains_bitfield;  // memory domains in which the target is readable
    uint32_t write_domains_bitfield; // memory domains in which the target is writable
};

// a single batch buffer to be scheduled for execution
struct magma_system_batch_buffer {
    uint32_t batch_buffer;
    uint32_t num_relocations;
    struct magma_system_relocation_entry* relocations;
};

// a collection of batch buffers submitted for execution together
struct magma_system_exec_buffer {
    uint32_t num_batch_buffers;
    struct magma_system_exec_buffer* batch_buffers;
};

// Opens a device - triggered by a client action. returns null on failure
struct magma_system_connection* magma_system_open(uint32_t device_handle);
void magma_system_close(struct magma_system_connection* connection);

// Returns the device id.  0 is an invalid device id.
uint32_t magma_system_get_device_id(struct magma_system_connection* connection);

bool magma_system_create_context(struct magma_system_connection* connection, uint32_t* context_id);
bool magma_system_destroy_context(struct magma_system_connection* connection, uint32_t context_id);

bool magma_system_alloc(struct magma_system_connection* connection, uint64_t size,
                        uint64_t* size_out, uint32_t* handle_out);
bool magma_system_free(struct magma_system_connection* connection, uint32_t handle);

bool magma_system_set_tiling_mode(struct magma_system_connection* connection, uint32_t handle,
                                  uint32_t tiling_mode);

bool magma_system_map(struct magma_system_connection* connection, uint32_t handle, void** paddr);
bool magma_system_unmap(struct magma_system_connection* connection, uint32_t handle, void* addr);

bool magma_system_set_domain(struct magma_system_connection* connection, uint32_t handle,
                             uint32_t read_domains, uint32_t write_domain);

bool magma_system_execute_buffer(struct magma_system_connection* connection,
                                 struct magma_system_exec_buffer* execbuffer, uint32_t context_id);

void magma_system_wait_rendering(struct magma_system_connection* connection, uint32_t handle);

#if defined(__cplusplus)
}
#endif

#endif /* _MAGMA_SYSTEM_H_ */
