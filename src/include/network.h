/*
 * Copyright (C) 2020 Red Hat
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

#ifndef PGPRTDBG_NETWORK_H
#define PGPRTDBG_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/socket.h>

/**
 * Bind sockets for a host
 * @param hostname The host name
 * @param port The port number
 * @param shmem The shared memory segment
 * @param fds The resulting descriptors
 * @param length The resulting length of descriptors
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_bind(const char* hostname, int port, void* shmem, int** fds, int* length);

/**
 * Connect to a host
 * @param shmem The shared memory segment
 * @param hostname The host name
 * @param port The port number
 * @param fd The resulting descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_connect(void* shmem, const char* hostname, int port, int* fd);

/**
 * Shutdown a descriptor
 * @param fd The descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_shutdown(int fd);

/**
 * Disconnect from a descriptor
 * @param fd The descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_disconnect(int fd);

/**
 * Apply TCP/NODELAY to a descriptor
 * @param fd The descriptor
 * @param shmem The shared memory segment
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_tcp_nodelay(int fd, void* shmem);

/**
 * Set the configured socket buffer size to a descriptor
 * @param fd The descriptor
 * @param shmem The shared memory segment
 * @return 0 upon success, otherwise 1
 */
int
pgprtdbg_socket_buffers(int fd, void* shmem);

#ifdef __cplusplus
}
#endif

#endif
