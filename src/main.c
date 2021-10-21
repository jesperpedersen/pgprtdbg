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

/* pgprtdbg */
#include <pgprtdbg.h>
#include <configuration.h>
#include <logging.h>
#include <network.h>
#include <shmem.h>
#include <utils.h>
#include <worker.h>

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
};

struct periodic_info
{
   struct ev_periodic periodic;
};

static volatile int keep_running = 1;
static struct ev_loop* main_loop = NULL;
static struct accept_io io_main[MAX_FDS];
static struct accept_io io_uds;
static int unix_pgsql_socket = -1;
static int* main_fds = NULL;
static int main_fds_length;

static void
start_io(void)
{
   for (int i = 0; i < main_fds_length; i++)
   {
      int sockfd = *(main_fds + i);

      memset(&io_main[i], 0, sizeof(struct accept_io));
      ev_io_init((struct ev_io*)&io_main[i], accept_cb, sockfd, EV_READ);
      io_main[i].socket = sockfd;
      ev_io_start(main_loop, (struct ev_io*)&io_main[i]);
   }
}

static void
shutdown_io(void)
{
   for (int i = 0; i < main_fds_length; i++)
   {
      ev_io_stop(main_loop, (struct ev_io*)&io_main[i]);
      pgprtdbg_disconnect(io_main[i].socket);
   }
}

static void
start_uds(void)
{
   memset(&io_uds, 0, sizeof(struct accept_io));
   ev_io_init((struct ev_io*)&io_uds, accept_cb, unix_pgsql_socket, EV_READ);
   io_uds.socket = unix_pgsql_socket;
   ev_io_start(main_loop, (struct ev_io*)&io_uds);
}

static void
shutdown_uds(void)
{
   char pgsql[MISC_LENGTH];
   struct configuration* config;

   config = (struct configuration*)shmem;

   memset(&pgsql, 0, sizeof(pgsql));
   snprintf(&pgsql[0], sizeof(pgsql), ".s.PGSQL.%d", config->port);

   ev_io_stop(main_loop, (struct ev_io*)&io_uds);
   pgprtdbg_disconnect(unix_pgsql_socket);
   errno = 0;
   pgprtdbg_remove_unix_socket(config->unix_socket_dir, &pgsql[0]);
   errno = 0;
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
   struct ev_signal signal_watcher[6];
   size_t size;
   char pgsql[MISC_LENGTH];
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
   if (pgprtdbg_create_shared_memory(size))
   {
      printf("pgagroal: Error in creating shared memory\n");
      exit(1);
   }
   pgprtdbg_init_configuration();

   if (configuration_path != NULL)
   {
      if (pgprtdbg_read_configuration(configuration_path))
      {
         printf("pgprtdbg: Configuration not found: %s\n", configuration_path);
         exit(1);
      }
   }
   else
   {
      if (pgprtdbg_read_configuration("/etc/pgprtdbg.conf"))
      {
         printf("pgprtdbg: Configuration not found: /etc/pgprtdbg.conf\n");
         exit(1);
      }
   }


   if (pgprtdbg_validate_configuration())
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

   pgprtdbg_start_logging();

   /* Open file */
   config->file = fopen(config->output, "a+");

   memset(&pgsql, 0, sizeof(pgsql));
   snprintf(&pgsql[0], sizeof(pgsql), ".s.PGSQL.%d", config->port);

   /* Bind Unix Domain Socket socket */
   if (strlen(config->unix_socket_dir) > 0)
   {
      if (pgprtdbg_bind_unix_socket(config->unix_socket_dir, &pgsql[0], &unix_pgsql_socket))
      {
         printf("pgprtdbg: Could not bind to %s/%s\n", config->unix_socket_dir, &pgsql[0]);
         exit(1);
      }
   }

   /* Bind main socket */
   if (pgprtdbg_bind(config->host, config->port, &main_fds, &main_fds_length))
   {
      printf("pgprtdbg: Could not bind to %s:%d\n", config->host, config->port);
      exit(1);
   }
   
   if (main_fds_length > MAX_FDS)
   {
      printf("pgprtdbg: Too many descriptors %d\n", main_fds_length);
      exit(1);
   }

   /* libev */
   main_loop = ev_default_loop(pgprtdbg_libev(config->libev));
   if (!main_loop)
   {
      printf("pgprtdbg: No loop implementation (%x) (%x)\n",
             pgprtdbg_libev(config->libev), ev_supported_backends());
      exit(1);
   }

   ev_signal_init(&signal_watcher[0], shutdown_cb, SIGTERM);
   ev_signal_init(&signal_watcher[1], shutdown_cb, SIGHUP);
   ev_signal_init(&signal_watcher[2], shutdown_cb, SIGINT);
   ev_signal_init(&signal_watcher[3], shutdown_cb, SIGTRAP);
   ev_signal_init(&signal_watcher[4], coredump_cb, SIGABRT);
   ev_signal_init(&signal_watcher[5], shutdown_cb, SIGALRM);

   for (int i = 0; i < 6; i++)
   {
      ev_signal_start(main_loop, &signal_watcher[i]);
   }

   if (strlen(config->unix_socket_dir) > 0)
   {
      start_uds();
   }
   start_io();

   pgprtdbg_log_lock();
   pgprtdbg_log_line("--------");
   pgprtdbg_log_line("Startup");
   pgprtdbg_log_line("pgprtdbg: started on %s:%d", config->host, config->port);
   for (int i = 0; i < main_fds_length; i++)
   {
      pgprtdbg_log_line("Socket %d", *(main_fds + i));
   }
   pgprtdbg_libev_engines();
   pgprtdbg_log_line("libev engine: %s", pgprtdbg_libev_engine(ev_backend(main_loop)));
   pgprtdbg_log_line("Configuration size: %lu", size);
   pgprtdbg_log_unlock();

   while (keep_running)
   {
      ev_loop(main_loop, 0);
   }

   pgprtdbg_log_lock();
   pgprtdbg_log_line("pgprtdbg: shutdown");
   pgprtdbg_log_unlock();

   shutdown_io();
   if (strlen(config->unix_socket_dir) > 0)
   {
      shutdown_uds();
   }

   for (int i = 0; i < 6; i++)
   {
      ev_signal_stop(main_loop, &signal_watcher[i]);
   }

   for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++)
   {
      if (config->pids[i] != 0)
      {
         kill(config->pids[i], SIGQUIT);
      }
   }

   ev_loop_destroy(main_loop);

   free(main_fds);

   /* Close file */
   fflush(config->file);
   fclose(config->file);

   pgprtdbg_stop_logging();
   pgprtdbg_destroy_shared_memory(size);

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

   ai = (struct accept_io*)watcher;
   config = (struct configuration*)shmem;

   if (EV_ERROR & revents)
   {
      return;
   }

   if (atomic_load(&config->active_connections) > MAX_NUMBER_OF_CONNECTIONS)
   {
      return;
   }

   client_addr_length = sizeof(client_addr);
   client_fd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_addr_length);
   if (client_fd == -1)
   {
      return;
   }

   if (!fork())
   {
      ev_loop_fork(loop);
      shutdown_io();
      pgprtdbg_disconnect(ai->socket);
      pgprtdbg_worker(client_fd);
   }

   pgprtdbg_disconnect(client_fd);
}


static void
shutdown_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
   ev_break(loop, EVBREAK_ALL);
   keep_running = 0;
}

static void
coredump_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
   abort();
}
