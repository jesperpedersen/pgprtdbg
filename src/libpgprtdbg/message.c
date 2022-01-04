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

/* pgprtdbg */
#include <pgprtdbg.h>
#include <logging.h>
#include <memory.h>
#include <message.h>
#include <network.h>
#include <worker.h>
#include <utils.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

static int read_message(int socket, bool block, struct message** msg);
static int write_message(int socket, struct message* msg);

int
pgprtdbg_read_message(int socket, struct message** msg)
{
   return read_message(socket, false, msg);
}

int
pgprtdbg_read_block_message(int socket, struct message** msg)
{
   return read_message(socket, true, msg);
}

int
pgprtdbg_write_message(int socket, struct message* msg)
{
   return write_message(socket, msg);
}

int
pgprtdbg_create_message(void* data, ssize_t length, struct message** msg)
{
   struct message* copy = NULL;

   copy = (struct message*)malloc(sizeof(struct message));
   copy->data = malloc(length);

   copy->kind = pgprtdbg_read_byte(data);
   copy->length = length;
   memcpy(copy->data, data, length);
     
   *msg = copy;

   return MESSAGE_STATUS_OK;
}

struct message*
pgprtdbg_copy_message(struct message* msg)
{
   struct message* copy = NULL;

   copy = (struct message*)malloc(sizeof(struct message));
   copy->data = malloc(msg->length);

   copy->kind = msg->kind;
   copy->length = msg->length;
   memcpy(copy->data, msg->data, msg->length);
     
   return copy;
}

void
pgprtdbg_free_message(struct message* msg)
{
   pgprtdbg_memory_free();
}

void
pgprtdbg_free_copy_message(struct message* msg)
{
   if (msg)
   {
      if (msg->data)
      {
         free(msg->data);
         msg->data = NULL;
      }

      free(msg);
      msg = NULL;
   }
}

int32_t
pgprtdbg_get_request(struct message* msg)
{
   return pgprtdbg_read_int32(msg->data + 4);
}

int
pgprtdbg_write_empty(int socket)
{
   char zero[1];
   struct message msg;

   memset(&msg, 0, sizeof(struct message));
   memset(&zero, 0, sizeof(zero));

   msg.kind = 0;
   msg.length = 1;
   msg.data = &zero;

   return pgprtdbg_write_message(socket, &msg);
}

int
pgprtdbg_write_connection_refused_old(int socket)
{
   int size = 20;
   char connection_refused[size];
   struct message msg;

   memset(&msg, 0, sizeof(struct message));
   memset(&connection_refused, 0, sizeof(connection_refused));

   pgprtdbg_write_byte(&connection_refused, 'E');
   pgprtdbg_write_string(&(connection_refused[1]), "connection refused");

   msg.kind = 'E';
   msg.length = size;
   msg.data = &connection_refused;

   return write_message(socket, &msg);
}

static int
read_message(int socket, bool block, struct message** msg)
{
   bool keep_read = false;
   ssize_t numbytes;  
   struct message* m = NULL;

   do
   {
      m = pgprtdbg_memory_message();
      m->data = pgprtdbg_memory_data();

      numbytes = read(socket, m->data, m->max_length);

      if (likely(numbytes > 0))
      {
         m->kind = (signed char)(*((char*)m->data));
         m->length = numbytes;
         *msg = m;

         return MESSAGE_STATUS_OK;
      }
      else if (numbytes == 0)
      {
         pgprtdbg_memory_free();

         if ((errno == EAGAIN || errno == EWOULDBLOCK) && block)
         {
            keep_read = true;
            errno = 0;
         }
         else
         {
            return MESSAGE_STATUS_ZERO;
         }
      }
      else
      {
         pgprtdbg_memory_free();

         if ((errno == EAGAIN || errno == EWOULDBLOCK) && block)
         {
            keep_read = true;
            errno = 0;
         }
         else
         {
            keep_read = false;
         }
      }
   } while (keep_read);

   return MESSAGE_STATUS_ERROR;
}

static int
write_message(int socket, struct message* msg)
{
   bool keep_write = false;
   ssize_t numbytes;
   int offset;
   ssize_t totalbytes;
   ssize_t remaining;

   numbytes = 0;
   offset = 0;
   totalbytes = 0;
   remaining = msg->length;

   do
   {
      numbytes = write(socket, msg->data + offset, remaining);

      if (likely(numbytes == msg->length))
      {
         return MESSAGE_STATUS_OK;
      }
      else if (numbytes != -1)
      {
         offset += numbytes;
         totalbytes += numbytes;
         remaining -= numbytes;

         if (totalbytes == msg->length)
         {
            return MESSAGE_STATUS_OK;
         }

         keep_write = true;
         errno = 0;
      }
      else
      {
         switch (errno)
         {
            case EAGAIN:
               keep_write = true;
               errno = 0;
               break;
            default:
               keep_write = false;
               break;
         }
      }
   } while (keep_write);

   return MESSAGE_STATUS_ERROR;
}
