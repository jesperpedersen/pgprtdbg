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

/* pgprtdbg */
#include <pgprtdbg.h>
#include <logging.h>
#include <memory.h>
#include <message.h>
#include <network.h>
#include <pipeline.h>
#include <worker.h>
#include <utils.h>

#define ZF_LOG_TAG "worker"
#include <zf_log.h>

/* system */
#include <ev.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

volatile int running = 1;
volatile int exit_code = WORKER_FAILURE;

static void sigquit_cb(struct ev_loop *loop, ev_signal *w, int revents);

void
pgprtdbg_worker(int client_fd, void* shmem)
{
   struct ev_loop *loop = NULL;
   struct signal_info signal_watcher;
   struct worker_io client_io;
   struct worker_io server_io;
   struct configuration* config;
   pid_t pid;
   int server_fd = -1;
   bool connected = false;

   pgprtdbg_start_logging(shmem);
   pgprtdbg_memory_init(shmem);

   config = (struct configuration*)shmem;
   pid = getpid();

   memset(&client_io, 0, sizeof(struct worker_io));
   memset(&server_io, 0, sizeof(struct worker_io));


   for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++)
   {
      if (config->pids[i] == 0)
      {
         config->pids[i] = pid;
         break;
      }
   }

   /* Connect */
   if (!pgprtdbg_connect(shmem, config->server[0].host, config->server[0].port, &server_fd))
   {
      ZF_LOGD("pgprtdbg_worker: PID %d (%d -> %d)", pid, client_fd, server_fd);

      atomic_fetch_add(&config->active_connections, 1);
      connected = true;
      
      ev_io_init((struct ev_io*)&client_io, pipeline_client, client_fd, EV_READ);
      client_io.client_fd = client_fd;
      client_io.server_fd = server_fd;
      client_io.shmem = shmem;
      
      ev_io_init((struct ev_io*)&server_io, pipeline_server, server_fd, EV_READ);
      server_io.client_fd = client_fd;
      server_io.server_fd = server_fd;
      server_io.shmem = shmem;
      
      loop = ev_loop_new(pgprtdbg_libev(config->libev));

      ev_signal_init((struct ev_signal*)&signal_watcher, sigquit_cb, SIGQUIT);
      signal_watcher.shmem = shmem;
      ev_signal_start(loop, (struct ev_signal*)&signal_watcher);

      ev_io_start(loop, (struct ev_io*)&client_io);
      ev_io_start(loop, (struct ev_io*)&server_io);

      while (running)
      {
         ev_loop(loop, 0);
      }
   }

   ZF_LOGD("Disconnect: Client(%d) Server(%d)", client_fd, server_fd);
   pgprtdbg_disconnect(client_fd);
   pgprtdbg_disconnect(server_fd);

   if (loop)
   {
      ev_io_stop(loop, (struct ev_io*)&client_io);
      ev_io_stop(loop, (struct ev_io*)&server_io);

      ev_signal_stop(loop, (struct ev_signal*)&signal_watcher);

      ev_loop_destroy(loop);
   }

   for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++)
   {
      if (config->pids[i] == pid)
      {
         config->pids[i] = 0;
         break;
      }
   }

   if (connected)
   {
      atomic_fetch_sub(&config->active_connections, 1);
   }

   pgprtdbg_memory_destroy();
   pgprtdbg_stop_logging(shmem);

   exit(exit_code);
}

static void
sigquit_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
   ZF_LOGD("pgprtdbg: SIGQUIT for PID %d", getpid());

   exit_code = WORKER_FAILURE;
   running = 0;
   ev_break(loop, EVBREAK_ALL);
}
