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
#include <message.h>
#include <pipeline.h>
#include <protocol.h>
#include <worker.h>
#include <utils.h>

/* system */
#include <errno.h>
#include <ev.h>
#include <stdlib.h>
#include <unistd.h>

long identifier = 0;

void
pipeline_client(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
   int status = MESSAGE_STATUS_ERROR;
   struct worker_io* wi = NULL;
   struct message* msg = NULL;
   struct configuration* config = NULL;

   wi = (struct worker_io*)watcher;
   config = (struct configuration*)shmem;

   status = pgprtdbg_read_message(wi->client_fd, &msg);
   if (likely(status == MESSAGE_STATUS_OK))
   {
      pgprtdbg_client(wi->client_fd, wi->server_fd, msg);

      if (config->save_traffic)
      {
         identifier++;
         pgprtdbg_save_client_traffic(getpid(), identifier, msg);
      }

      status = pgprtdbg_write_message(wi->server_fd, msg);
      if (unlikely(status != MESSAGE_STATUS_OK))
      {
         goto server_error;
      }

      if (msg->kind == 'X')
      {
         exit_code = WORKER_SUCCESS;
         running = 0;
      }
   }
   else if (status == MESSAGE_STATUS_ZERO)
   {
      goto client_done;
   }
   else
   {
      goto client_error;
   }

   ev_break(loop, EVBREAK_ONE);
   return;

client_done:
   pgprtdbg_log_lock();
   pgprtdbg_log_line("[C] client_done: client_fd %d (%d)", wi->client_fd, status);
   pgprtdbg_log_unlock();

   errno = 0;
   ev_break(loop, EVBREAK_ALL);
   exit_code = WORKER_CLIENT_FAILURE;
   running = 0;
   return;

client_error:
   if (errno != 0)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[C] client_error: client_fd %d - %s (%d)", wi->client_fd, strerror(errno), status);
      pgprtdbg_log_unlock();
   }
   else
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[C] client_error: client_fd %d (%d)", wi->client_fd, status);
      pgprtdbg_log_unlock();
   }

   errno = 0;
   ev_break(loop, EVBREAK_ALL);
   exit_code = WORKER_CLIENT_FAILURE;
   running = 0;
   return;

server_error:
   if (errno != 0)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[C] server_error: server_fd %d - %s (%d)", wi->server_fd, strerror(errno), status);
      pgprtdbg_log_unlock();
   }
   else
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[C] server_error: server_fd %d (%d)", wi->server_fd, status);
      pgprtdbg_log_unlock();
   }

   errno = 0;
   ev_break(loop, EVBREAK_ALL);
   exit_code = WORKER_SERVER_FAILURE;
   running = 0;
   return;
}

void
pipeline_server(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
   int status = MESSAGE_STATUS_ERROR;
   bool fatal = false;
   struct worker_io* wi = NULL;
   struct message* msg = NULL;
   struct configuration* config = NULL;

   wi = (struct worker_io*)watcher;
   config = (struct configuration*)shmem;

   status = pgprtdbg_read_message(wi->server_fd, &msg);
   if (likely(status == MESSAGE_STATUS_OK))
   {
      pgprtdbg_server(wi->server_fd, wi->client_fd, msg);

      if (config->save_traffic)
      {
         pgprtdbg_save_server_traffic(getpid(), identifier, msg);
      }

      status = pgprtdbg_write_message(wi->client_fd, msg);
      if (unlikely(status != MESSAGE_STATUS_OK))
      {
         goto client_error;
      }

      if (unlikely(msg->kind == 'E'))
      {
         fatal = false;

         if (!strncmp(msg->data + 6, "FATAL", 5) || !strncmp(msg->data + 6, "PANIC", 5))
            fatal = true;

         if (!strncmp(msg->data + 20, "0A000", 5))
         {
            fatal = false;
         }

         if (fatal)
         {
            exit_code = WORKER_SERVER_FATAL;
            running = 0;
         }
      }
   }
   else if (status == MESSAGE_STATUS_ZERO)
   {
      goto server_done;
   }
   else
   {
      goto server_error;
   }

   ev_break(loop, EVBREAK_ONE);
   return;

client_error:
   if (errno != 0)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[S] client_error: client_fd %d - %s (%d)", wi->client_fd, strerror(errno), status);
      pgprtdbg_log_unlock();
   }
   else
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[S] client_error: client_fd %d (%d)", wi->client_fd, status);
      pgprtdbg_log_unlock();
   }

   errno = 0;
   ev_break(loop, EVBREAK_ALL);
   exit_code = WORKER_CLIENT_FAILURE;
   running = 0;
   return;

server_done:
   pgprtdbg_log_lock();
   pgprtdbg_log_line("[C] server_done: server_fd %d (%d)", wi->server_fd, status);
   pgprtdbg_log_unlock();

   errno = 0;
   ev_break(loop, EVBREAK_ALL);
   running = 0;
   return;

server_error:
   if (errno != 0)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[S] server_error: server_fd %d - %s (%d)", wi->server_fd, strerror(errno), status);
      pgprtdbg_log_unlock();
   }
   else
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("[S] server_error: server_fd %d (%d)", wi->server_fd, status);
      pgprtdbg_log_unlock();
   }

   errno = 0;
   ev_break(loop, EVBREAK_ALL);
   exit_code = WORKER_SERVER_FAILURE;
   running = 0;
   return;
}
