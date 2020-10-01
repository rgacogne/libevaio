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

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "evaio.h"

#define BASE(obj) ((typeof(&obj->base))obj)

typedef struct user_io_op
{
    struct aiocb base; // inheritance
    TAILQ_ENTRY(user_io_op) entries;
    evaio_completion_handler * cb;
    void * user_data;
}
user_io_op;

struct evaio
{
    TAILQ_HEAD(user_io_op_queue, user_io_op) base; // inheritance
    struct event * sigio_event;
};

__nonnull((1, 2)) __inline __attribute__ ((__nothrow__))
static void evaio_add_elt(evaio * aio, user_io_op * elt)
{
    TAILQ_INSERT_TAIL(BASE(aio), elt, entries);
}

__nonnull((1, 2)) __inline __attribute__ ((__nothrow__))
static void evaio_del_elt(evaio * aio, user_io_op * elt)
{
    TAILQ_REMOVE(BASE(aio), elt, entries);
    free(elt);
}

__nonnull((3)) __attribute__ ((__nothrow__))
static void evaio_sigio_handler(int signo   __attribute__((unused)),
                                short flags __attribute__((unused)),
                                void * user_data)
{
    evaio * aio = user_data;

    for (user_io_op * elt = TAILQ_FIRST(BASE(aio)); elt != NULL;)
    {
        errno_t status = aio_error(BASE(elt));

        if (status != EINPROGRESS)
        {
            user_io_op * next = TAILQ_NEXT(elt, entries);
            ssize_t progress = aio_return(BASE(elt));
            elt->cb(aio, status, (size_t) progress, elt->user_data);
            evaio_del_elt(aio, elt), elt = next;
        }
        else
        {
            elt = TAILQ_NEXT(elt, entries);
        }
    }
}

__nonnull((1)) __attribute__ ((__nothrow__)) __attribute_warn_unused_result__
static user_io_op * user_io_op_init(evaio_config const * config)
{
    user_io_op * self = calloc(1, sizeof *self);

    if (self != NULL)
    {
        struct aiocb * acb = BASE(self);

        self->cb = config->cb;
        self->user_data = config->user_data;

        acb->aio_fildes = config->fd;
        acb->aio_buf    = (typeof(acb->aio_buf))config->data;
        acb->aio_nbytes = config->data_size;
        acb->aio_offset = config->offset;

        acb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
        acb->aio_sigevent.sigev_signo  = SIGIO;
    }

    return self;
}

__nonnull((1, 2)) __inline __attribute__ ((__nothrow__)) __attribute_warn_unused_result__
static errno_t evaio_enqueue(evaio_config const * config, int (*io_operation)(struct aiocb *))
{
    errno_t result = 0;

    if(config == NULL || config->aio == NULL || config->data == NULL || config->cb == NULL ||
       config->fd < 0 || config->data_size == 0)
    {
        result = EINVAL;
    }
    else
    {
        user_io_op * elt = user_io_op_init(config);

        if (elt != NULL)
        {
            evaio_add_elt(config->aio, elt);

            result = io_operation(BASE(elt));

            if (result == -1)
            {
                evaio_del_elt(config->aio, elt), elt = NULL;
                result = errno;
            }
        }
        else
        {
            result = ENOMEM;
        }
    }

    return result;
}

errno_t evaio_read(evaio_config const * config)
{
    return evaio_enqueue(config, aio_read);
}

errno_t evaio_write(evaio_config const * config)
{
    return evaio_enqueue(config, aio_write);
}

errno_t evaio_fsync(evaio_config const * config,
                    int const op)
{
    errno_t result = 0;

    if(config == NULL || config->aio == NULL || config->cb == NULL ||
       config->fd < 0 || (op != O_DSYNC && op != O_SYNC))
    {
        result = EINVAL;
    }
    else
    {
        user_io_op * elt = user_io_op_init(config);

        if (elt != NULL)
        {
            evaio_add_elt(config->aio, elt);

            result = aio_fsync(op,
                               BASE(elt));

            if (result == -1)
            {
                evaio_del_elt(config->aio, elt), elt = NULL;
                result = errno;
            }
        }
        else
        {
            result = ENOMEM;
        }
    }

    return result;
}

evaio * evaio_new(struct event_base * const base)
{
    evaio * self = malloc(sizeof *self);

    if (self != NULL)
    {
        TAILQ_INIT(BASE(self));
        self->sigio_event = evsignal_new(base, SIGIO, evaio_sigio_handler, self);

        if (self->sigio_event != NULL)
        {
            if (event_add(self->sigio_event, NULL) != 0)
            {
                event_free(self->sigio_event);
                free(self), self = NULL;
            }
        }
        else
        {
            free(self), self = NULL;
        }
    }

    return self;
}

void evaio_del(evaio * aio)
{
    if (aio != NULL)
    {
        for (user_io_op * elt = TAILQ_FIRST(BASE(aio)), * next = NULL; elt != NULL; elt = next)
        {
            next = TAILQ_NEXT(elt, entries);
            free(elt);
        }

        event_free(aio->sigio_event);
        free(aio);
    }
}
