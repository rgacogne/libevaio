/*
 * Copyright (c) 2012, Remi Gacogne, Nicolas Sitbon
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef EVENT_AIO_H_
#define EVENT_AIO_H_

#include <sys/cdefs.h>
#include <stddef.h>
#include <sys/types.h>

#include <event2/event.h>

/**
   @mainpage

   @section intro Introduction

   Libevaio is a simple library allowing the use of POSIX AIO with libevent.

   @section usage Standard usage

   Every program that uses libevaio must include thee <evaio.h>
   header and pass the -levaio flag to the linker.

   Note that the program with also be linked with libevent.

   The program may also be linked with librt if the POSIX AIO implementation
   requires it.

   @section setup Library setup

   The evaio_new() function should be called before every other
   function of the libevaio library.

   @section warning Warning

   Note than the underlying implementation of asynchronous may use POSIX threads.
   This is especially the case of glibc under Linux.

   @section api API Reference

   evaio.h

*/

typedef int errno_t;

/**
 * Opaque structure containing libevaio data.
 * This structure is linked to the event_base passed to
 * evaio_new() and should not outlive it.
 *
 * @see evaio_new(), evaio_del()
 */
typedef struct evaio evaio;

/**
 * Callback called when an asynchonous operation is finished.
 *
 * @param aio is the evaio struct passed to the asynchronous request.
 * @param status is set to 0 if the request has been successful,
 *  ECANCELED is the request has been canceled or a positive errno value if
 *  the asynchonous operation failed. In this last case, the value is the same
 *  that would have been set on a failed synchronous operation.
 * @param transfered if status is equal to 0, this contains the number of bytes read or written.
 * @param user_data the user_data parameter passed to the asynchronous request.
 *
 * @see evaio_read() evaio_write() evaio_fsync()
 */
typedef void (evaio_completion_handler)(evaio * aio, errno_t status, size_t transfered, void * user_data);

/**
 * Structure containing parameters for an asynchronous request.
 *
 * @see evaio_new(), evaio_del()
 */
typedef struct
{
    evaio * aio; /**< evaio opaque structure to which the operation will be linked. */
    void const * data; /**< pointer to buffer to read from or to write to. */
    size_t data_size; /**< size of the data buffer. */
    evaio_completion_handler * cb; /**< callback to call after the operation is finished, or in case of error. */
    void * user_data; /**< pointer that will be passed to callback. */
    off_t offset; /**< file offset to read from or write to. */
    int fd; /**< file descriptor to read from or write to. */
}
evaio_config;

__BEGIN_DECLS

/**
 * Asynchronous write request
 *
 * @param config a pointer to an evaio_config struct holding parameters for the request.
 *
 * @return EINVAL on invalid parameters.
 * ENOMEM if an allocation failed, the return value of aio_write() otherwise.
 */
extern errno_t evaio_write(evaio_config const * config)
    __attribute__ ((__nothrow__)) __nonnull((1)) __attribute__((visibility ("default"))) __attribute_warn_unused_result__;

/**
 * Asynchronous read request
 *
 * @param config a pointer to an evaio_config struct holding parameters for the request.
 *   The value pointed to by the data field needs to exist until the associated callback is called.
 *
 * @return EINVAL on invalid parameters.
 * ENOMEM if an allocation failed, the return value of aio_read() otherwise.
 */
extern errno_t evaio_read(evaio_config const * config)
    __attribute__ ((__nothrow__)) __nonnull((1)) __attribute__((visibility ("default"))) __attribute_warn_unused_result__;

/**
 * Asynchronous fsync request
 *
 * @param config a pointer to an evaio_config struct holding parameters for the request.
 *   The value pointed to by the data field needs to exist until the associated callback is called.
 * @param op see the op parameter of aio_fsync().
 *
 * @return EINVAL on invalid parameters.
 * ENOMEM if an allocation failed, the return value of aio_fsync() otherwise.
 */
extern errno_t evaio_fsync(evaio_config const * config,
                           int op)
    __attribute__ ((__nothrow__)) __nonnull((1)) __attribute__((visibility ("default"))) __attribute_warn_unused_result__;

/**
 * Initialize libevaio and associate an evaio structure
 * to an event_base.
 *
 * For now, libevaio uses an underlying signal notification with the SIGIO
 * signal. This means that this signal shoud not be used for any other usage,
 * and that using more than one event_aio in the same program results in an
 * undefined behavior.
 *
 * @param base event_base the asynchronous operations will be dispatched in.
 *
 * @return An allocated evaio structure or NULL if an error occurs.
 *
 * @see evaio_del()
 */
extern evaio * evaio_new(struct event_base * base)
    __attribute__ ((__nothrow__)) __nonnull((1)) __attribute__((visibility ("default")))__attribute_warn_unused_result__;

/**
 * Free an evaio structure and associated pending asynchronous operations.
 *
 * @param aio aio structure to free.
 *
 * @see evaio_new()
 */
extern void evaio_del(evaio * aio)
    __attribute__ ((__nothrow__)) __attribute__((visibility ("default")));

__END_DECLS

#endif /* EVENT_AIO_H_ */
