// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _MSD_H_
#define _MSD_H_

#include "msd_defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Instantiates a driver instance.
struct msd_driver_t* msd_driver_create(void);

// Configures the driver according to |flags|.
void msd_driver_configure(struct msd_driver_t* drv, uint32_t flags);

// Destroys a driver instance.
void msd_driver_destroy(struct msd_driver_t* drv);

// Creates a device at system startup.
struct msd_device_t* msd_driver_create_device(struct msd_driver_t* drv, void* device);

// Destroys a device at system shutdown.
void msd_device_destroy(struct msd_device_t* dev);

// Returns the device id.  0 is an invalid device id.
uint32_t msd_device_get_id(struct msd_device_t* dev);

void msd_device_dump_status(struct msd_device_t* dev);

// Opens a device for the given client. Returns null on failure
struct msd_connection_t* msd_device_open(struct msd_device_t* dev, msd_client_id client_id);

// Provides a buffer to be scanned out on the next vblank event.
// |callback| will be called with |data| as its second argument when |buf| is
// no longer being displayed and is safe to be reused. The first argument to
// |callback| indicates an error with the page flip, where 0 indicates success
void msd_device_page_flip(struct msd_device_t* dev, struct msd_buffer_t* buf,
                          struct magma_system_image_descriptor* image_desc,
                          magma_system_pageflip_callback_t callback, void* data);

// Closes the given connection to the device.
void msd_connection_close(struct msd_connection_t* connection);

// Creates a context for the given connection. returns null on failure.
struct msd_context_t* msd_connection_create_context(struct msd_connection_t* connection);

// Returns 0 on success.
// Blocks until all currently outstanding work on the given buffer completes.
// If more work that references this buffer is queued while waiting, this may return before that
// work is completed.
magma_status_t msd_connection_wait_rendering(struct msd_connection_t* connection,
                                             struct msd_buffer_t* buf);

// Destroys the given context.
void msd_context_destroy(struct msd_context_t* ctx);

// returns 0 on success
// |ctx| is the context in which to execute the command buffer
// |cmd_buf| is the command buffer to be executed
// |exec_resources| are the buffers referenced by the handles in command_buf->exec_resources,
// in the same order
// |wait_semaphores| are the semaphores that must be signalled before starting command buffer
// execution
// |signal_semaphores| are the semaphores to be signalled upon completion of the command buffer
magma_status_t msd_context_execute_command_buffer(struct msd_context_t* ctx,
                                                  struct msd_buffer_t* cmd_buf,
                                                  struct msd_buffer_t** exec_resources,
                                                  struct msd_semaphore_t** wait_semaphores,
                                                  struct msd_semaphore_t** signal_semaphores);

// Creates a buffer that owns the provided handle
// The resulting msd_buffer_t is owned by the caller and must be destroyed
// Returns NULL on failure.
struct msd_buffer_t* msd_buffer_import(uint32_t handle);

// Destroys |buf|
// This releases buf's reference to the underlying platform buffer
void msd_buffer_destroy(struct msd_buffer_t* buf);

// Imports the given handle as a semaphore.
magma_status_t msd_semaphore_import(uint32_t handle, struct msd_semaphore_t** semaphore_out);

// Releases the given semaphore.
void msd_semaphore_release(struct msd_semaphore_t* semaphore);

#if defined(__cplusplus)
}
#endif

#endif // _MSD_H_
