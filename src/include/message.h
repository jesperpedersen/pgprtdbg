/*
 * Copyright (C) 2022 Red Hat
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

#ifndef PGPRTDBG_MESSAGE_H
#define PGPRTDBG_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pgprtdbg.h>

#include <stdbool.h>
#include <stdlib.h>

#define MESSAGE_STATUS_ZERO  0
#define MESSAGE_STATUS_OK    1
#define MESSAGE_STATUS_ERROR 2

/**
 * Read a message
 * @param socket The socket descriptor
 * @param msg The resulting message
 * @return One of MESSAGE_STATUS_ZERO, MESSAGE_STATUS_OK or MESSAGE_STATUS_ERROR
 */
int
pgprtdbg_read_message(int socket, struct message** msg);

/**
 * Read a message in blocking mode
 * @param socket The socket descriptor
 * @param msg The resulting message
 * @return One of MESSAGE_STATUS_ZERO, MESSAGE_STATUS_OK or MESSAGE_STATUS_ERROR
 */
int
pgprtdbg_read_block_message(int socket, struct message** msg);

/**
 * Write a message
 * @param socket The socket descriptor
 * @param msg The message
 * @return One of MESSAGE_STATUS_ZERO, MESSAGE_STATUS_OK or MESSAGE_STATUS_ERROR
 */
int
pgprtdbg_write_message(int socket, struct message* msg);

/**
 * Create a message
 * @param data A pointer to the data
 * @param length The length of the message
 * @param msg The resulting message
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_create_message(void* data, ssize_t length, struct message** msg);
   
/**
 * Copy a message
 * @param msg The resulting message
 * @return The copy
 */
struct message*
pgprtdbg_copy_message(struct message* msg);

/**
 * Free a message
 * @param msg The resulting message
 */
void
pgprtdbg_free_message(struct message* msg);

/**
 * Free a copy message
 * @param msg The resulting message
 */
void
pgprtdbg_free_copy_message(struct message* msg);

/**
 * Get the request identifier
 * @param msg The message
 * @return The identifier
 */
int32_t
pgprtdbg_get_request(struct message* msg);

/**
 * Write an empty message
 * @param socket The socket descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_write_empty(int socket);

/**
 * Write a connection refused message (protocol 1 or 2)
 * @param socket The socket descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_write_connection_refused_old(int socket);

#ifdef __cplusplus
}
#endif

#endif
