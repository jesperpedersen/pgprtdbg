/*
 * Copyright (C) 2021 Red Hat
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PGPRTDBG_H
#define PGPRTDBG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ev.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#define VERSION "0.2.1"

#define MAX_BUFFER_SIZE      65535
#define DEFAULT_BUFFER_SIZE  65535

#define IDENTIFIER_LENGTH 64
#define MISC_LENGTH 128

#define MAX_NUMBER_OF_CONNECTIONS 1000

#define STATE_FREE   0
#define STATE_IN_USE 1

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

#define MIN(a, b)               \
   ({ __typeof__ (a) _a = (a);  \
      __typeof__ (b) _b = (b);  \
      _a < _b ? _a : _b; })

/**
 * The shared memory segment
 */
extern void* shmem;

/** @struct
 * Defines a server
 */
struct server
{
   char name[MISC_LENGTH]; /**< The name of the server */
   char host[MISC_LENGTH]; /**< The host name of the server */
   int port;               /**< The port of the server */
} __attribute__ ((aligned (64)));

/** @struct
 * Defines the configuration and state of pgprtdbg
 */
struct configuration
{
   char host[MISC_LENGTH]; /**< The host */
   int port;               /**< The port */

   char output[MISC_LENGTH]; /**< The output path */
   FILE* file;               /**< The file */
   sem_t lock;               /**< The file lock */

   bool output_sockets;      /**< Output socket identifiers */

   char unix_socket_dir[MISC_LENGTH]; /**< The directory for the Unix Domain Socket */

   int log_type;               /**< The logging type */
   char log_path[MISC_LENGTH]; /**< The logging path */
   atomic_schar log_lock;      /**< The logging lock */

   char libev[MISC_LENGTH]; /**< Name of libev mode */
   int buffer_size;         /**< Socket buffer size */
   bool keep_alive;         /**< Use keep alive */
   bool nodelay;            /**< Use NODELAY */
   int backlog;             /**< The backlog for listen */

   atomic_ushort active_connections;      /**< The active number of connections */
   pid_t pids[MAX_NUMBER_OF_CONNECTIONS]; /**< The PIDS of the connections */

   struct server server[1]; /**< The server */
} __attribute__ ((aligned (64)));

/** @struct
 * Defines a message
 */
struct message
{
   signed char kind;  /**< The kind of the message */
   ssize_t length;    /**< The length of the message */
   size_t max_length; /**< The maximum size of the message */
   void* data;        /**< The message data */
} __attribute__ ((aligned (64)));

#ifdef __cplusplus
}
#endif

#endif
