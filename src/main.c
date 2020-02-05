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
#include <configuration.h>
#include <logging.h>
#include <network.h>
#include <shmem.h>
#include <utils.h>
#include <worker.h>

#define ZF_LOG_TAG "main"
#include <zf_log.h>

/* system */
#include <errno.h>
#include <ev.h>
#include <getopt.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FDS 64

static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
static void shutdown_cb(struct ev_loop *loop, ev_signal *w, int revents);
static void coredump_cb(struct ev_loop *loop, ev_signal *w, int revents);

struct accept_io
{
   struct ev_io io;
   int socket;
   void* shmem;
};

struct periodic_info
{
   struct ev_periodic periodic;
   void* shmem;
};

static volatile int keep_running = 1;
static struct accept_io io_main[MAX_FDS];
static int length;

static void
shutdown_io(struct ev_loop *loop)
{
   for (int i = 0; i < length; i++)
   {
      ev_io_stop(loop, (struct ev_io*)&io_main[i]);
      pgprtdbg_shutdown(io_main[i].socket);
      pgprtdbg_disconnect(io_main[i].socket);
   }
}

static void
version()
{
   printf("pgprtdbg %s\n", VERSION);
   exit(1);
}

static void
usage()
{
   printf("pgprtdbg %s\n", VERSION);
   printf("  PostgreSQL protocol debugging\n");
   printf("\n");

   printf("Usage:\n");
   printf("  pgprtdbg [ -c CONFIG_FILE ] [ -d ]\n");
   printf("\n");
   printf("Options:\n");
   printf("  -c, --config CONFIG_FILE Set the path to the pgprtdbg.conf file\n");
   printf("  -d, --daemon             Run as a daemon\n");
   printf("  -V, --version            Display version information\n");
   printf("  -?, --help               Display help\n");
   printf("\n");

   exit(1);
}

int
main(int argc, char **argv)
{
   char* configuration_path = NULL;
   bool daemon = false;
   pid_t pid, sid;
   void* shmem = NULL;
   struct ev_loop *loop;
   struct signal_info signal_watcher[6];
   int* fds = NULL;
   size_t size;
   struct configuration* config = NULL;
   int c;

   while (1)
   {
      static struct option long_options[] =
      {
         {"config",  required_argument, 0, 'c'},
         {"daemon", no_argument, 0, 'd'},
         {"version", no_argument, 0, 'V'},
         {"help", no_argument, 0, '?'}
      };
      int option_index = 0;

      c = getopt_long (argc, argv, "dV?c:",
                       long_options, &option_index);

      if (c == -1)
         break;

      switch (c)
      {
         case 'c':
            configuration_path = optarg;
            break;
         case 'd':
            daemon = true;
            break;
         case 'V':
            version();
            break;
         case '?':
            usage();
            break;
         default:
            break;
      }
   }

   if (getuid() == 0)
   {
      printf("pgprtdbg: Using the root account is not allowed\n");
      exit(1);
   }

   size = sizeof(struct configuration);
   shmem = pgprtdbg_create_shared_memory(size);
   pgprtdbg_init_configuration(shmem, size);

   if (configuration_path != NULL)
   {
      if (pgprtdbg_read_configuration(configuration_path, shmem))
      {
         printf("pgprtdbg: Configuration not found: %s\n", configuration_path);
         exit(1);
      }
   }
   else
   {
      if (pgprtdbg_read_configuration("/etc/pgprtdbg.conf", shmem))
      {
         printf("pgprtdbg: Configuration not found: /etc/pgprtdbg.conf\n");
         exit(1);
      }
   }


   if (pgprtdbg_validate_configuration(shmem))
   {
      exit(1);
   }

   config = (struct configuration*)shmem;

   if (daemon)
   {
      if (config->log_type == PGPRTDBG_LOGGING_TYPE_CONSOLE)
      {
         printf("pgprtdbg: Daemon mode can't be used with console logging\n");
         exit(1);
      }

      pid = fork();

      if (pid < 0)
      {
         printf("pgprtdbg: Daemon mode failed\n");
         exit(1);
      }

      if (pid > 0)
      {
         exit(0);
      }

      /* We are a daemon now */
      umask(0);
      sid = setsid();

      if (sid < 0)
      {
         exit(1);
      }
   }

   pgprtdbg_start_logging(shmem);

   /* Open file */
   config->file = fopen(config->output, "a+");

   /* Bind main socket */
   if (pgprtdbg_bind(config->host, config->port, shmem, &fds, &length))
   {
      printf("pgprtdbg: Could not bind to %s:%d\n", config->host, config->port);
      exit(1);
   }
   
   if (length > MAX_FDS)
   {
      printf("pgprtdbg: Too many descriptors %d\n", length);
      exit(1);
   }

   /* libev */
   loop = ev_default_loop(pgprtdbg_libev(config->libev));
   if (!loop)
   {
      printf("pgprtdbg: No loop implementation (%x) (%x)\n",
             pgprtdbg_libev(config->libev), ev_supported_backends());
      exit(1);
   }

   ev_signal_init((struct ev_signal*)&signal_watcher[0], shutdown_cb, SIGTERM);
   ev_signal_init((struct ev_signal*)&signal_watcher[1], shutdown_cb, SIGHUP);
   ev_signal_init((struct ev_signal*)&signal_watcher[2], shutdown_cb, SIGINT);
   ev_signal_init((struct ev_signal*)&signal_watcher[3], shutdown_cb, SIGTRAP);
   ev_signal_init((struct ev_signal*)&signal_watcher[4], coredump_cb, SIGABRT);
   ev_signal_init((struct ev_signal*)&signal_watcher[5], shutdown_cb, SIGALRM);

   for (int i = 0; i < 6; i++)
   {
      signal_watcher[i].shmem = shmem;
      ev_signal_start(loop, (struct ev_signal*)&signal_watcher[i]);
   }

   for (int i = 0; i < length; i++)
   {
      int sockfd = *(fds + i);
      ev_io_init((struct ev_io*)&io_main[i], accept_cb, sockfd, EV_READ);
      io_main[i].socket = sockfd;
      io_main[i].shmem = shmem;
      ev_io_start(loop, (struct ev_io*)&io_main[i]);
   }

   ZF_LOGI("pgprtdbg: started on %s:%d", config->host, config->port);
   for (int i = 0; i < length; i++)
   {
      ZF_LOGD("Socket %d", *(fds + i));
   }
   pgprtdbg_libev_engines();
   ZF_LOGD("libev engine: %s", pgprtdbg_libev_engine(ev_backend(loop)));
   ZF_LOGD("Configuration size: %lu", size);

   while (keep_running)
   {
      ev_loop(loop, 0);
   }

   ZF_LOGI("pgprtdbg: shutdown");

   shutdown_io(loop);

   for (int i = 0; i < 6; i++)
   {
      ev_signal_stop(loop, (struct ev_signal*)&signal_watcher[i]);
   }

   for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++)
   {
      if (config->pids[i] != 0)
      {
         ZF_LOGD("SIGQUIT: %d", config->pids[i]);
         kill(config->pids[i], SIGQUIT);
      }
   }

   ev_loop_destroy(loop);

   free(fds);

   /* Close file */
   fflush(config->file);
   fclose(config->file);

   pgprtdbg_stop_logging(shmem);
   pgprtdbg_destroy_shared_memory(shmem, size);

   return 0;
}

static void
accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
   struct sockaddr_in client_addr;
   socklen_t client_addr_length;
   int client_fd;
   struct accept_io* ai;
   struct configuration* config;

   ZF_LOGV("accept_cb: sockfd ready (%d)", revents);

   ai = (struct accept_io*)watcher;
   config = (struct configuration*)ai->shmem;

   if (EV_ERROR & revents)
   {
      ZF_LOGD("accept_cb: invalid event: %s", strerror(errno));
      return;
   }

   if (atomic_load(&config->active_connections) > MAX_NUMBER_OF_CONNECTIONS)
   {
      ZF_LOGD("accept_cb: Too many connections");
      return;
   }

   client_addr_length = sizeof(client_addr);
   client_fd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_addr_length);
   if (client_fd == -1)
   {
      ZF_LOGD("accept_cb: accept: %s", strerror(errno));
      return;
   }

   if (!fork())
   {
      ev_loop_fork(loop);
      pgprtdbg_disconnect(ai->socket);
      pgprtdbg_worker(client_fd, ai->shmem);
   }

   pgprtdbg_disconnect(client_fd);
}


static void
shutdown_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
   ZF_LOGD("pgprtdbg: shutdown requested");

   ev_break(loop, EVBREAK_ALL);
   keep_running = 0;
}

static void
coredump_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
   ZF_LOGI("pgprtdbg: core dump requested");

   abort();
}
