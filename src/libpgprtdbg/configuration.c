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
#include <utils.h>

/* system */
#include <semaphore.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_LENGTH 256

static void extract_key_value(char* str, char** key, char** value);
static int as_int(char* str);
static bool as_bool(char* str);
static int as_logging_type(char* str);

/**
 *
 */
int
pgprtdbg_init_configuration(void)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   config->output_sockets = false;

   config->buffer_size = DEFAULT_BUFFER_SIZE;
   config->keep_alive = true;
   config->nodelay = false;
   config->backlog = -1;

   config->log_type = PGPRTDBG_LOGGING_TYPE_CONSOLE;
   atomic_init(&config->log_lock, STATE_FREE);

   if (sem_init(&config->lock, 1, 1) == -1)
   {
      return 1;
   }

   config->file = NULL;

   atomic_init(&config->active_connections, 0);

   return 0;
}

/**
 *
 */
int
pgprtdbg_read_configuration(char* filename)
{
   FILE* file;
   char section[LINE_LENGTH];
   char line[LINE_LENGTH];
   char* key = NULL;
   char* value = NULL;
   char* ptr = NULL;
   size_t max;
   struct configuration* config;
   int idx_server = -1;
   struct server srv;

   file = fopen(filename, "r");

   if (!file)
      return 1;
    
   memset(&section, 0, LINE_LENGTH);
   config = (struct configuration*)shmem;

   while (fgets(line, sizeof(line), file))
   {
      if (strcmp(line, ""))
      {
         if (line[0] == '[')
         {
            ptr = strchr(line, ']');
            if (ptr)
            {
               memset(&section, 0, LINE_LENGTH);
               max = ptr - line - 1;
               if (max > MISC_LENGTH - 1)
                  max = MISC_LENGTH - 1;
               memcpy(&section, line + 1, max);
               if (strcmp(section, "pgprtdbg"))
               {
                  idx_server++;

                  if (idx_server == 0)
                  {
                     memcpy(&(config->server[0]), &srv, sizeof(struct server));
                  }
                  else
                  {
                     printf("Maximum number of servers exceeded\n");
                     return 1;
                  }

                  memset(&srv, 0, sizeof(struct server));
                  memcpy(&srv.name, &section, strlen(section));
               }
            }
         }
         else if (line[0] == '#' || line[0] == ';')
         {
            /* Comment, so ignore */
         }
         else
         {
            extract_key_value(line, &key, &value);

            if (key && value)
            {
               bool unknown = false;

               /* printf("|%s|%s|\n", key, value); */

               if (!strcmp(key, "host"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     max = strlen(value);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(config->host, value, max);
                  }
                  else if (strlen(section) > 0)
                  {
                     max = strlen(section);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(&srv.name, section, max);
                     max = strlen(value);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(&srv.host, value, max);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "port"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->port = as_int(value);
                  }
                  else if (strlen(section) > 0)
                  {
                     memcpy(&srv.name, section, strlen(section));
                     srv.port = as_int(value);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "output"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     max = strlen(value);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(config->output, value, max);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "output_sockets"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->output_sockets = as_bool(value);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "unix_socket_dir"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     max = strlen(value);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(config->unix_socket_dir, value, max);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "log_type"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->log_type = as_logging_type(value);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "log_path"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     max = strlen(value);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(config->log_path, value, max);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "libev"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     max = strlen(value);
                     if (max > MISC_LENGTH - 1)
                        max = MISC_LENGTH - 1;
                     memcpy(config->libev, value, max);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "buffer_size"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->buffer_size = as_int(value);

                     if (config->buffer_size > MAX_BUFFER_SIZE)
                     {
                        config->buffer_size = MAX_BUFFER_SIZE;
                     }
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "keep_alive"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->keep_alive = as_bool(value);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "nodelay"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->nodelay = as_bool(value);
                  }
                  else
                  {
                     unknown = true;
                  }
               }
               else if (!strcmp(key, "backlog"))
               {
                  if (!strcmp(section, "pgprtdbg"))
                  {
                     config->backlog = as_int(value);
                  }
                  else
                  {
                     unknown = true;
                  }
               }

               if (unknown)
               {
                  printf("Unknown: Section=%s, Key=%s, Value=%s\n", strlen(section) > 0 ? section : "<unknown>", key, value);
               }

               free(key);
               free(value);
               key = NULL;
               value = NULL;
            }
         }
      }
   }

   if (idx_server != -1 && strlen(srv.name) > 0)
   {
      memcpy(&(config->server[0]), &srv, sizeof(struct server));
   }

   fclose(file);

   return 0;
}

/**
 *
 */
int
pgprtdbg_validate_configuration(void)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   if (strlen(config->host) == 0)
   {
      printf("pgprtdbg: No host defined\n");
      return 1;
   }

   if (config->port == 0)
   {
      printf("pgprtdbg: No port defined\n");
      return 1;
   }

   if (strlen(config->output) == 0)
   {
      printf("pgprtdbg: No output defined\n");
      return 1;
   }

   if (config->backlog <= 0)
   {
      config->backlog = 4;
   }

   if (strlen(config->server[0].name) == 0)
   {
      printf("pgprtdbg: No server defined\n");
      return 1;
   }

   if (strlen(config->server[0].host) == 0)
   {
      printf("pgprtdbg: No host defined for %s\n", config->server[0].name);
      return 1;
   }

   if (config->server[0].port == 0)
   {
      printf("pgprtdbg: No port defined for %s\n", config->server[0].name);
      return 1;
   }

   return 0;
}

static void
extract_key_value(char* str, char** key, char** value)
{
   int c = 0;
   int offset = 0;
   int length = strlen(str);
   char* k;
   char* v;

   while (str[c] != ' ' && str[c] != '=' && c < length)
      c++;

   if (c < length)
   {
      k = malloc(c + 1);
      memset(k, 0, c + 1);
      memcpy(k, str, c);
      *key = k;

      while ((str[c] == ' ' || str[c] == '\t' || str[c] == '=') && c < length)
         c++;

      offset = c;

      while (str[c] != ' ' && str[c] != '\r' && str[c] != '\n' && c < length)
         c++;

      if (c < length)
      {
         v = malloc((c - offset) + 1);
         memset(v, 0, (c - offset) + 1);
         memcpy(v, str + offset, (c - offset));
         *value = v;
      }
   }
}

static int as_int(char* str)
{
   return atoi(str);
}

static bool as_bool(char* str)
{
   if (!strcasecmp(str, "true"))
      return true;

   if (!strcasecmp(str, "on"))
      return true;

   if (!strcasecmp(str, "1"))
      return true;

   if (!strcasecmp(str, "false"))
      return false;

   if (!strcasecmp(str, "off"))
      return false;

   if (!strcasecmp(str, "0"))
      return false;

   return false;
}

static int as_logging_type(char* str)
{
   if (!strcasecmp(str, "console"))
      return PGPRTDBG_LOGGING_TYPE_CONSOLE;

   if (!strcasecmp(str, "file"))
      return PGPRTDBG_LOGGING_TYPE_FILE;

   return 0;
}
