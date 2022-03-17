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

/* system */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_LENGTH 32

FILE* log_file = NULL;

/**
 *
 */
int
pgprtdbg_start_logging(void)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   if (config->log_type == PGPRTDBG_LOGGING_TYPE_FILE)
   {
      if (strlen(config->log_path) > 0)
      {
         log_file = fopen(config->log_path, "a");
      }
      else
      {
         log_file = fopen("pgprtdbg.log", "a");
      }

      if (!log_file)
      {
         printf("Failed to open log file %s due to %s\n", strlen(config->log_path) > 0 ? config->log_path : "pgprtdbg.log", strerror(errno));
         errno = 0;
         return 1;
      }
   }

   return 0;
}

/**
 *
 */
int
pgprtdbg_stop_logging(void)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   if (config->log_type == PGPRTDBG_LOGGING_TYPE_FILE)
   {
      return fclose(log_file);
   }

   return 0;
}

void
pgprtdbg_log_lock(void)
{
   time_t start_time;
   signed char isfree;
   struct configuration* config;

   config = (struct configuration*)shmem;

   start_time = time(NULL);

retry:
   isfree = STATE_FREE;

   if (atomic_compare_exchange_strong(&config->log_lock, &isfree, STATE_IN_USE))
   {
      return;
   }
   else
   {
      /* Sleep for 1ms */
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 1000000L;
      nanosleep(&ts, NULL);

      double diff = difftime(time(NULL), start_time);
      if (diff >= 1)
      {
         goto timeout;
      }

      goto retry;
   }

timeout:
   return;
}

void
pgprtdbg_log_unlock(void)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   atomic_store(&config->log_lock, STATE_FREE);
}

void
pgprtdbg_log_line(char* fmt, ...)
{
   va_list vl;
   struct configuration* config;

   config = (struct configuration*)shmem;

   va_start(vl, fmt);

   if (config->log_type == PGPRTDBG_LOGGING_TYPE_CONSOLE)
   {
      vfprintf(stdout, fmt, vl);
      fprintf(stdout, "\n");
      fflush(stdout);
   }
   else if (config->log_type == PGPRTDBG_LOGGING_TYPE_FILE)
   {
      vfprintf(log_file, fmt, vl);
      fprintf(log_file, "\n");
      fflush(log_file);
   }

   va_end(vl);
}

void
pgprtdbg_log_mem(void* data, size_t size)
{
   char buf[256 * 1024];
   int j = 0;
   int k = 0;
   struct configuration* config;

   config = (struct configuration*)shmem;

   memset(&buf, 0, sizeof(buf));

   for (int i = 0; i < size; i++)
   {
      if (k == LINE_LENGTH)
      {
         buf[j] = '\n';
         j++;
         k = 0;
      }
      sprintf(&buf[j], "%02X", (signed char) *((char*)data + i));
      j += 2;
      k++;
   }

   buf[j] = '\n';
   j++;
   k = 0;

   for (int i = 0; i < size; i++)
   {
      signed char c = (signed char) *((char*)data + i);
      if (k == LINE_LENGTH)
      {
         buf[j] = '\n';
         j++;
         k = 0;
      }
      if (c >= 32 && c <= 127)
      {
         buf[j] = c;
      }
      else
      {
         buf[j] = '?';
      }
      j++;
      k++;
   }

   if (config->log_type == PGPRTDBG_LOGGING_TYPE_CONSOLE)
   {
      fprintf(stdout, "%s", buf);
      fprintf(stdout, "\n");
      fflush(stdout);
   }
   else if (config->log_type == PGPRTDBG_LOGGING_TYPE_FILE)
   {
      fprintf(log_file, "%s", buf);
      fprintf(log_file, "\n");
      fflush(log_file);
   }
}
