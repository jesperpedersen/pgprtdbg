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
#include <network.h>

/* system */
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

static int bind_host(const char* hostname, int port, int** fds, int* length);

/**
 *
 */
int
pgprtdbg_bind(const char* hostname, int port, int** fds, int* length)
{
   struct ifaddrs *ifaddr, *ifa;
   struct sockaddr_in *sa4;
   struct sockaddr_in6 *sa6;
   char addr[50];
   int* star_fds = NULL;
   int star_length = 0;

   if (!strcmp("*", hostname))
   {
      if (getifaddrs(&ifaddr) == -1)
      {
         pgprtdbg_log_lock();
         pgprtdbg_log_line("getifaddrs: %s", strerror(errno));
         pgprtdbg_log_unlock();
         return 1;
      }

      for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
      {
         if (ifa->ifa_addr != NULL &&
             (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6) &&
             (ifa->ifa_flags & IFF_UP))
         {
            int* new_fds = NULL;
            int new_length = 0;

            memset(addr, 0, sizeof(addr));

            if (ifa->ifa_addr->sa_family == AF_INET)
            {
               sa4 = (struct sockaddr_in*) ifa->ifa_addr;
               inet_ntop(AF_INET, &sa4->sin_addr, addr, sizeof(addr));
            }
            else
            {
               sa6 = (struct sockaddr_in6 *) ifa->ifa_addr;
               inet_ntop(AF_INET6, &sa6->sin6_addr, addr, sizeof(addr));
            }

            if (bind_host(addr, port, &new_fds, &new_length))
            {
               free(new_fds);
               continue;
            }

            if (star_fds == NULL)
            {
               star_fds = malloc(new_length * sizeof(int));
               memcpy(star_fds, new_fds, new_length * sizeof(int));
               star_length = new_length;
            }
            else
            {
               star_fds = realloc(star_fds, (star_length + new_length) * sizeof(int));
               memcpy(star_fds + star_length, new_fds, new_length * sizeof(int));
               star_length += new_length;
            }

            free(new_fds);
         }
      }

      *fds = star_fds;
      *length = star_length;

      freeifaddrs(ifaddr);
      return 0;
   }

   return bind_host(hostname, port, fds, length);
}

/**
 *
 */
int
pgprtdbg_connect(const char* hostname, int port, int* fd)
{
   struct addrinfo hints, *servinfo, *p;
   int yes = 1;
   socklen_t optlen = sizeof(int);
   int rv;
   char* sport;
   struct configuration* config;

   config = (struct configuration*)shmem;

   sport = malloc(5);
   memset(sport, 0, 5);
   sprintf(sport, "%d", port);

   /* Connect to server */
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if ((rv = getaddrinfo(hostname, sport, &hints, &servinfo)) != 0) {
      free(sport);
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   free(sport);

   /* Loop through all the results and connect to the first we can */
   for (p = servinfo; p != NULL; p = p->ai_next)
   {
      if ((*fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      {
         pgprtdbg_log_lock();
         pgprtdbg_log_line("pgprtdbg_connect: socket: %s", strerror(errno));
         pgprtdbg_log_unlock();
         return 1;
      }

      if (config->keep_alive)
      {
         if (setsockopt(*fd, SOL_SOCKET, SO_KEEPALIVE, &yes, optlen) == -1)
         {
            pgprtdbg_disconnect(*fd);
            return 1;
         }
      }

      if (config->nodelay)
      {
         if (setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &yes, optlen) == -1)
         {
            pgprtdbg_disconnect(*fd);
            return 1;
         }
      }

      if (setsockopt(*fd, SOL_SOCKET, SO_RCVBUF, &config->buffer_size, optlen) == -1)
      {
         pgprtdbg_disconnect(*fd);
         return 1;
      }

      if (setsockopt(*fd, SOL_SOCKET, SO_SNDBUF, &config->buffer_size, optlen) == -1)
      {
         pgprtdbg_disconnect(*fd);
         return 1;
      }

      if (connect(*fd, p->ai_addr, p->ai_addrlen) == -1)
      {
         pgprtdbg_disconnect(*fd);
         return 1;
      }

      break;
   }

   if (p == NULL)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("pgprtdbg_connect: failed to connect");
      pgprtdbg_log_unlock();
      return 1;
   }

   freeaddrinfo(servinfo);

   return 0;
}

/**
 *
 */
int
pgprtdbg_disconnect(int fd)
{
   if (fd == -1)
      return 1;

   return close(fd);
}

/**
 *
 */
int
pgprtdbg_bind_unix_socket(const char* directory, const char* file, int *fd)
{
   int status;
   char buf[107];
   struct stat st = {0};
   struct sockaddr_un addr;
   struct configuration* config;

   config = (struct configuration*)shmem;

   if ((*fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("pgprtdbg_bind_unix_socket: socket: %s %s", directory, strerror(errno));
      pgprtdbg_log_unlock();
      errno = 0;
      goto error;
   }

   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;

   if (!directory)
   {
      directory = "/tmp/";
   }

   memset(&buf, 0, sizeof(buf));
   snprintf(&buf[0], sizeof(buf), "%s", directory);

   if (stat(&buf[0], &st) == -1)
   {
      status = mkdir(&buf[0], S_IRWXU);
      if (status == -1)
      {
         pgprtdbg_log_lock();
         pgprtdbg_log_line("pgprtdbg_bind_unix_socket: permission defined for %s (%s)", directory, strerror(errno));
         pgprtdbg_log_unlock();
         errno = 0;
         goto error;
      }
   }

   memset(&buf, 0, sizeof(buf));
   snprintf(&buf[0], sizeof(buf), "%s/%s", directory, file);

   strncpy(addr.sun_path, &buf[0], sizeof(addr.sun_path) - 1);
   unlink(&buf[0]);

   if (bind(*fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("pgprtdbg_bind_unix_socket: bind: %s/%s %s", directory, file, strerror(errno));
      pgprtdbg_log_unlock();
      errno = 0;
      goto error;
   }

   if (listen(*fd, config->backlog) == -1)
   {
      pgprtdbg_log_lock();
      pgprtdbg_log_line("pgprtdbg_bind_unix_socket: listen: %s/%s %s", directory, file, strerror(errno));
      pgprtdbg_log_unlock();
      errno = 0;
      goto error;
   }

   return 0;

error:

   return 1;
}

/**
 *
 */
int
pgprtdbg_remove_unix_socket(const char* directory, const char* file)
{
   char buf[MISC_LENGTH];

   memset(&buf, 0, sizeof(buf));
   snprintf(&buf[0], sizeof(buf), "%s/%s", directory, file);

   unlink(&buf[0]);

   return 0;
}

int
pgprtdbg_tcp_nodelay(int fd)
{
   struct configuration* config;
   int yes = 1;
   socklen_t optlen = sizeof(int);

   config = (struct configuration*)shmem;

   if (config->nodelay)
   {
      if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, optlen) == -1)
      {
         return 1;
      }
   }

   return 0;
}

int
pgprtdbg_socket_buffers(int fd)
{
   struct configuration* config;
   socklen_t optlen = sizeof(int);

   config = (struct configuration*)shmem;

   if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &config->buffer_size, optlen) == -1)
   {
      return 1;
   }

   if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &config->buffer_size, optlen) == -1)
   {
      return 1;
   }

   return 0;
}

/**
 *
 */
static int
bind_host(const char* hostname, int port, int** fds, int* length)
{
   int *result = NULL;
   int index, size;
   int sockfd;
   struct addrinfo hints, *servinfo, *addr;
   int yes = 1;
   int rv;
   char* sport;
   struct configuration* config;

   config = (struct configuration*)shmem;

   index = 0;
   size = 0;

   sport = malloc(5);
   memset(sport, 0, 5);
   sprintf(sport, "%d", port);

   /* Find all SOCK_STREAM addresses */
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if ((rv = getaddrinfo(hostname, sport, &hints, &servinfo)) != 0)
   {
      free(sport);
      pgprtdbg_log_lock();
      pgprtdbg_log_line("getaddrinfo: %s:%d (%s)", hostname, port, gai_strerror(rv));
      pgprtdbg_log_unlock();
      return 1;
   }

   free(sport);

   for (addr = servinfo; addr != NULL; addr = addr->ai_next)
   {
      size++;
   }

   result = malloc(size * sizeof(int));
   memset(result, 0, size * sizeof(int));

   /* Loop through all the results and bind to the first we can */
   for (addr = servinfo; addr != NULL; addr = addr->ai_next)
   {
      if ((sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
      {
         pgprtdbg_log_lock();
         pgprtdbg_log_line("server: socket: %s:%d (%s)", hostname, port, strerror(errno));
         pgprtdbg_log_unlock();
         continue;
      }

      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      {
         pgprtdbg_disconnect(sockfd);
         continue;
      }

      if (pgprtdbg_socket_buffers(sockfd))
      {
         pgprtdbg_disconnect(sockfd);
         continue;
      }

      if (pgprtdbg_tcp_nodelay(sockfd))
      {
         pgprtdbg_disconnect(sockfd);
         continue;
      }

      if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1)
      {
         pgprtdbg_disconnect(sockfd);
         pgprtdbg_log_lock();
         pgprtdbg_log_line("server: bind: %s:%d (%s)", hostname, port, strerror(errno));
         pgprtdbg_log_unlock();
         continue;
      }

      if (listen(sockfd, config->backlog) == -1)
      {
         pgprtdbg_disconnect(sockfd);
         pgprtdbg_log_lock();
         pgprtdbg_log_line("server: listen: %s:%d (%s)", hostname, port, strerror(errno));
         pgprtdbg_log_unlock();
         continue;
      }

      *(result + index) = sockfd;
      index++;
   }

   freeaddrinfo(servinfo);

   if (index == 0)
   {
      free(result);
      return 1;
   }

   *fds = result;
   *length = index;

   return 0;
}
