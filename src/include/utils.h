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

#ifndef PGPRTDBG_UTILS_H
#define PGPRTDBG_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pgprtdbg.h>

#include <stdlib.h>

/**
 * Read a byte
 * @param data Pointer to the data
 * @return The byte
 */
signed char
pgprtdbg_read_byte(void* data);

/**
 * Read an int16
 * @param data Pointer to the data
 * @return The int16
 */
int16_t
pgprtdbg_read_int16(void* data);

/**
 * Read an int32
 * @param data Pointer to the data
 * @return The int32
 */
int32_t
pgprtdbg_read_int32(void* data);

/**
 * Read a long
 * @param data Pointer to the data
 * @return The long
 */
long
pgprtdbg_read_long(void* data);

/**
 * Read a string
 * @param data Pointer to the data
 * @return The string
 */
char*
pgprtdbg_read_string(void* data);

/**
 * Write a byte
 * @param data Pointer to the data
 * @param b The byte
 */
void
pgprtdbg_write_byte(void* data, signed char b);

/**
 * Write an int32
 * @param data Pointer to the data
 * @param i The int32
 */
void
pgprtdbg_write_int32(void* data, int32_t i);

/**
 * Write a long
 * @param data Pointer to the data
 * @param l The long int
 */
void
pgprtdbg_write_long(void* data, long l);

/**
 * Write a string
 * @param data Pointer to the data
 * @param s The string
 */
void
pgprtdbg_write_string(void* data, char* s);

/**
 * Print the available libev engines
 */
void
pgprtdbg_libev_engines(void);

/**
 * Get the constant for a libev engine
 * @param engine The name of the engine
 * @return The constant
 */
unsigned int
pgprtdbg_libev(char* engine);

/**
 * Get the name for a libev engine
 * @param val The constant
 * @return The name
 */
char*
pgprtdbg_libev_engine(unsigned int val);

/**
 * Save client traffic in a file
 * @param pid The PID
 * @param identifier The number identifier for the message
 * @param msg The message
 * @return The name
 */
int
pgprtdbg_save_client_traffic(pid_t pid, long identifier, struct message* msg);

/**
 * Save server traffic in a file
 * @param pid The PID
 * @param identifier The number identifier for the message
 * @param msg The message
 * @return The name
 */
int
pgprtdbg_save_server_traffic(pid_t pid, long identifier, struct message* msg);

#ifdef __cplusplus
}
#endif

#endif
