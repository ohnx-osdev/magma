// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _MAGMA_H_
#define _MAGMA_H_

#include "magma_common_defs.h"
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Opens a device - triggered by a client action. returns null on failure
// |capabilities| must be either MAGMA_SYSTEM_CAPABILITY_RENDERING, MAGMA_SYSTEM_CAPABILITY_DISPLAY,
// or the bitwise or of both
struct magma_connection_t* magma_create_connection(int32_t file_descriptor, uint32_t capabilities);
void magma_release_connection(struct magma_connection_t* connection);

// Returns the first recorded error since the last time this function was called.
// Clears the recorded error.
magma_status_t magma_get_error(struct magma_connection_t* connection);

// Performs a query.
// |id| is one of MAGMA_QUERY_DEVICE_ID, or a vendor-specific id starting from
// MAGMA_QUERY_FIRST_VENDOR_ID.
magma_status_t magma_query(int fd, uint64_t id, uint64_t* value_out);

void magma_create_context(struct magma_connection_t* connection, uint32_t* context_id_out);
void magma_release_context(struct magma_connection_t* connection, uint32_t context_id);

magma_status_t magma_create_buffer(struct magma_connection_t* connection, uint64_t size,
                                   uint64_t* size_out, magma_buffer_t* buffer_out);
void magma_release_buffer(struct magma_connection_t* connection, magma_buffer_t buffer);

uint64_t magma_get_buffer_id(magma_buffer_t buffer);
uint64_t magma_get_buffer_size(magma_buffer_t buffer);

magma_status_t magma_map(struct magma_connection_t* connection, magma_buffer_t buffer,
                         void** addr_out);
magma_status_t magma_unmap(struct magma_connection_t* connection, magma_buffer_t buffer);

magma_status_t magma_create_command_buffer(struct magma_connection_t* connection, uint64_t size,
                                           magma_buffer_t* buffer_out);
void magma_release_command_buffer(struct magma_connection_t* connection,
                                  magma_buffer_t command_buffer);

// Executes a command buffer.
// Note that the buffer referred to by |command_buffer| must contain a valid
// magma_system_command_buffer and all associated data structures
// Transfers ownership of |command_buffer|.
void magma_submit_command_buffer(struct magma_connection_t* connection,
                                 magma_buffer_t command_buffer, uint32_t context_id);

void magma_wait_rendering(struct magma_connection_t* connection, magma_buffer_t buffer);

// makes the buffer returned by |buffer| able to be imported via |buffer_handle_out|
magma_status_t magma_export(struct magma_connection_t* connection, magma_buffer_t buffer,
                            uint32_t* buffer_handle_out);

// makes the buffer returned by |buffer| able to be imported via the fd at |fd_out|
magma_status_t magma_export_fd(struct magma_connection_t* connection, magma_buffer_t buffer,
                               int* fd_out);

// imports the buffer referred to by |buffer_handle| and makes it accessible via |buffer_out|
magma_status_t magma_import(struct magma_connection_t* connection, uint32_t buffer_handle,
                            magma_buffer_t* buffer_out);

// imports the buffer referred to by |fd| and makes it accessible via |buffer_out|
magma_status_t magma_import_fd(struct magma_connection_t* connection, int fd,
                               magma_buffer_t* buffer_out);

// Reads the size of the display in pixels.
magma_status_t magma_display_get_size(int fd, struct magma_display_size* size_out);

// Provides a buffer to be scanned out on the next vblank event.
// |wait_semaphores| will be waited upon prior to scanning out the buffer.
// |signal_semaphores| will be signaled when |buf| is no longer being displayed and is safe to be
// reused.
// |buffer_presented_semaphore| will be signaled when the vblank fires making the buffer visible.
magma_status_t magma_display_page_flip(struct magma_connection_t* connection, magma_buffer_t buffer,
                                       uint32_t wait_semaphore_count,
                                       const magma_semaphore_t* wait_semaphores,
                                       uint32_t signal_semaphore_count,
                                       const magma_semaphore_t* signal_semaphores,
                                       magma_semaphore_t buffer_presented_semaphore);

// Creates a semaphore on the given connection.  If successful |semaphore_out| will be set.
magma_status_t magma_create_semaphore(struct magma_connection_t* connection,
                                      magma_semaphore_t* semaphore_out);

// Destroys |semaphore|.
void magma_release_semaphore(struct magma_connection_t* connection, magma_semaphore_t semaphore);

// Returns the object id for the given semaphore.
uint64_t magma_get_semaphore_id(magma_semaphore_t semaphore);

// Signals |semaphore|.
void magma_signal_semaphore(magma_semaphore_t semaphore);

// Resets |semaphore|.
void magma_reset_semaphore(magma_semaphore_t semaphore);

// Waits for |semaphore| to be signaled.  Returns MAGMA_STATUS_TIMED_OUT if the timeout
// expires first.
magma_status_t magma_wait_semaphore(magma_semaphore_t semaphore, uint64_t timeout);

// Exports |semaphore| to it can be imported into another connection via |semaphore_handle_out|
magma_status_t magma_export_semaphore(struct magma_connection_t* connection,
                                      magma_semaphore_t semaphore, uint32_t* semaphore_handle_out);

// Imports the semaphore referred to by |semaphore_handle| into the given connection and makes it
// accessible via |semaphore_out|
magma_status_t magma_import_semaphore(struct magma_connection_t* connection,
                                      uint32_t semaphore_handle, magma_semaphore_t* semaphore_out);

#if defined(__cplusplus)
}
#endif

#endif /* _MAGMA_H_ */
