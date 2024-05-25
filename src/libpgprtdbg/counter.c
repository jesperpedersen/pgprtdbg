/*
 * Copyright (C) 2024 The pgprtdbg community
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
#include <counter.h>

/* system */
#include <sys/mman.h>

size_t event_counters_offset = sizeof(struct configuration);

struct event_counter*
pgprtdbg_counter_get(int client_number)
{
   struct event_counter* event_counters = (struct event_counter*) (shmem + event_counters_offset);
   struct event_counter* counter = &(event_counters[client_number]);
   return counter;
}

void
pgprtdbg_counter_output_statistics(int client_count)
{
   FILE* output_file = stdout;
   struct configuration* config;
   struct event_counter* event_counters = (struct event_counter*) (shmem + event_counters_offset);
   struct event_counter counter;

   config = (struct configuration*) shmem;

   if (strlen(config->statistics_output) > 1)
   {
      output_file = fopen(config->statistics_output, "w");
   }

   fprintf(output_file, "------------------------------\n");
   for (int client = 0; client < client_count; client++)
   {
      counter = event_counters[client];
      fprintf(output_file, "Client:                  %d\n", client + 1);
      fprintf(output_file, "Bytes Sent:              %ld\n", counter.sent_bytes);
      fprintf(output_file, "Messages Sent:           %d\n", counter.sent_messages);
      fprintf(output_file, "Bytes Received:          %ld\n", counter.rcvd_bytes);
      fprintf(output_file, "Messages Received:       %d\n", counter.rcvd_messages);
      if (client + 1 < client_count)
      {
         fprintf(output_file, "------------------------------\n");
      }
   }
}
