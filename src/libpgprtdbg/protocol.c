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
#include <logging.h>
#include <message.h>
#include <pipeline.h>
#include <protocol.h>
#include <worker.h>
#include <utils.h>

/* system */
#include <ev.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

static void output_write(char* id, int from, int to, signed char kind, char* text);

static int fe_zero(struct message* msg, int client_fd, char** text);
static int fe_B(struct message* msg, int offset, char** text);
static int fe_C(struct message* msg, int offset, char** text);
static int fe_D(struct message* msg, int offset, char** text);
static int fe_E(struct message* msg, int offset, char** text);
static int fe_F(struct message* msg, int offset, char** text);
static int fe_H(struct message* msg, int offset, char** text);
static int fe_P(struct message* msg, int offset, char** text);
static int fe_Q(struct message* msg, int offset, char** text);
static int fe_S(struct message* msg, int offset, char** text);
static int fe_X(struct message* msg, int offset, char** text);
static int fe_c(struct message* msg, int offset, char** text);
static int fe_d(struct message* msg, int offset, char** text);
static int fe_f(struct message* msg, int offset, char** text);
static int fe_p(struct message* msg, int offset, char** text);

static int be_one(struct message* msg, int offset, char** text);
static int be_two(struct message* msg, int offset, char** text);
static int be_three(struct message* msg, int offset, char** text);
static int be_A(struct message* msg, int offset, char** text);
static int be_C(struct message* msg, int offset, char** text);
static int be_D(struct message* msg, int offset, int from, int to, char** text, struct message** new_msg, int* new_offset);
static int be_E(struct message* msg, int offset, char** text);
static int be_G(struct message* msg, int offset, char** text);
static int be_H(struct message* msg, int offset, char** text);
static int be_I(struct message* msg, int offset, char** text);
static int be_K(struct message* msg, int offset, char** text);
static int be_N(struct message* msg, int offset, char** text);
static int be_R(struct message* msg, int offset, char** text);
static int be_S(struct message* msg, int offset, char** text);
static int be_T(struct message* msg, int offset, char** text);
static int be_V(struct message* msg, int offset, char** text);
static int be_W(struct message* msg, int offset, char** text);
static int be_Z(struct message* msg, int offset, char** text);
static int be_c(struct message* msg, int offset, char** text);
static int be_d(struct message* msg, int offset, char** text);
static int be_n(struct message* msg, int offset, char** text);
static int be_s(struct message* msg, int offset, char** text);
static int be_t(struct message* msg, int offset, char** text);
static int be_v(struct message* msg, int offset, char** text);

void
pgprtdbg_client(int from, int to, struct message* msg)
{
   char* text = NULL;
   signed char kind;
   int offset;

   pgprtdbg_log_lock();
   pgprtdbg_log_line("--------");
   pgprtdbg_log_line("Message:");
   pgprtdbg_log_mem(msg->data, msg->length);

   offset = 0;

   if (transport == PLAIN)
   {
      while (offset != -1 && offset < msg->length)
      {
         kind = pgprtdbg_read_byte(msg->data + offset);

         switch (kind)
         {
            case 0:
               offset = fe_zero(msg, from, &text);
               break;
            case 'B':
               offset = fe_B(msg, offset, &text);
               break;
            case 'C':
               offset = fe_C(msg, offset, &text);
               break;
            case 'D':
               offset = fe_D(msg, offset, &text);
               break;
            case 'E':
               offset = fe_E(msg, offset, &text);
               break;
            case 'F':
               pgprtdbg_log_line("TODO: %c", kind);
               offset = fe_F(msg, offset, &text);
               break;
            case 'H':
               offset = fe_H(msg, offset, &text);
               break;
            case 'P':
               offset = fe_P(msg, offset, &text);
               break;
            case 'Q':
               offset = fe_Q(msg, offset, &text);
               break;
            case 'S':
               offset = fe_S(msg, offset, &text);
               break;
            case 'X':
               offset = fe_X(msg, offset, &text);
               break;
            case 'c':
               offset = fe_c(msg, offset, &text);
               break;
            case 'd':
               offset = fe_d(msg, offset, &text);
               break;
            case 'f':
               offset = fe_f(msg, offset, &text);
               break;
            case 'p':
               offset = fe_p(msg, offset, &text);
               break;
            default:
               pgprtdbg_log_line("Unsupported client message: %d (%d)", kind, msg->kind);
               offset = msg->length;
               break;
         }

         output_write("C", from, to, kind, text);
         free(text);
         text = NULL;
      }
   }
   else
   {
      while (offset != -1 && offset < msg->length)
      {
         kind = pgprtdbg_read_byte(msg->data + offset);

         switch (kind)
         {
            case 0:
               offset = fe_zero(msg, from, &text);
               break;
            default:
               offset = msg->length;
               break;
         }

         if (transport == PLAIN)
         {
            output_write("C", from, to, kind, text);
         }
         else
         {
            output_write("C", from, to, '?', NULL);
         }

         free(text);
         text = NULL;
      }
   }

   if (offset == -1)
   {
      exit_code = WORKER_CLIENT_FAILURE;
      running = 0;
   }

   pgprtdbg_log_unlock();
}

void
pgprtdbg_server(int from, int to, struct message* msg)
{
   char* text = NULL;
   signed char kind;
   int offset;
   struct message* m;
   struct message* new_msg;
   int new_offset;

   pgprtdbg_log_lock();
   pgprtdbg_log_line("--------");
   pgprtdbg_log_line("Message:");
   pgprtdbg_log_mem(msg->data, msg->length);

   offset = 0;
   m = msg;
   new_msg = NULL;
   new_offset = 0;

   if (transport == PLAIN)
   {
      while (offset != -1 && offset < m->length)
      {
         kind = pgprtdbg_read_byte(m->data + offset);

         switch (kind)
         {
            case '1':
               offset = be_one(m, offset, &text);
               break;
            case '2':
               offset = be_two(m, offset, &text);
               break;
            case '3':
               offset = be_three(m, offset, &text);
               break;
            case 'A':
               offset = be_A(m, offset, &text);
               break;
            case 'C':
               offset = be_C(m, offset, &text);
               break;
            case 'D':
               offset = be_D(m, offset, from, to, &text, &new_msg, &new_offset);
               if (new_msg != NULL)
               {
                  m = new_msg;
                  offset = new_offset;
               }
               break;
            case 'E':
               offset = be_E(m, offset, &text);
               break;
            case 'G':
               offset = be_G(m, offset, &text);
               break;
            case 'H':
               offset = be_H(m, offset, &text);
               break;
            case 'I':
               offset = be_I(m, offset, &text);
               break;
            case 'K':
               offset = be_K(m, offset, &text);
               break;
            case 'N':
               offset = be_N(m, offset, &text);
               break;
            case 'R':
               offset = be_R(m, offset, &text);
               break;
            case 'S':
               offset = be_S(m, offset, &text);
               break;
            case 'T':
               offset = be_T(m, offset, &text);
               break;
            case 'V':
               pgprtdbg_log_line("TODO: %c", kind);
               offset = be_V(m, offset, &text);
               break;
            case 'W':
               pgprtdbg_log_line("TODO: %c", kind);
               offset = be_W(m, offset, &text);
               break;
            case 'Z':
               offset = be_Z(m, offset, &text);
               break;
            case 'c':
               offset = be_c(m, offset, &text);
               break;
            case 'd':
               offset = be_d(m, offset, &text);
               break;
            case 'n':
               offset = be_n(m, offset, &text);
               break;
            case 's':
               offset = be_s(m, offset, &text);
               break;
            case 't':
               pgprtdbg_log_line("TODO: %c", kind);
               offset = be_t(m, offset, &text);
               break;
            case 'v':
               pgprtdbg_log_line("TODO: %c", kind);
               offset = be_v(m, offset, &text);
               break;
            default:
               pgprtdbg_log_line("TODO: %c", kind);
               offset = m->length;
               break;
         }

         output_write("S", from, to, kind, text);
         free(text);
         text = NULL;
      }
   }
   else
   {
      while (offset != -1 && offset < m->length)
      {
         kind = pgprtdbg_read_byte(m->data + offset);

         switch (kind)
         {
            case 'N':
               offset = be_N(m, offset, &text);
               break;
            default:
               offset = m->length;
               break;
         }

         if (transport == PLAIN || kind == 'N')
         {
            output_write("S", from, to, kind, text);
         }
         else
         {
            output_write("S", from, to, '?', NULL);
         }

         free(text);
         text = NULL;
      }
   }

   if (offset == -1)
   {
      exit_code = WORKER_SERVER_FAILURE;
      running = 0;
   }

   pgprtdbg_log_unlock();
}

static void
output_write(char* id, int from, int to, signed char kind, char* text)
{
   char line[MISC_LENGTH];
   struct configuration* config;

   memset(&line, 0, sizeof(line));
   config = (struct configuration*)shmem;

   sem_wait(&config->lock);

   if ((kind >= 'A' && kind <= 'Z') || (kind >= 'a' && kind <= 'z')  || (kind >= '0' && kind <= '9') || kind == '?')
   {
      if (text != NULL)
      {
         if (config->output_sockets)
         {
            snprintf(&line[0], sizeof(line), "%s,%d,%d,%c,%s\n", id, from, to, kind, text);
         }
         else
         {
            snprintf(&line[0], sizeof(line), "%s,%c,%s\n", id, kind, text);
         }
      }
      else
      {
         if (config->output_sockets)
         {
            snprintf(&line[0], sizeof(line), "%s,%d,%d,%c\n", id, from, to, kind);
         }
         else
         {
            snprintf(&line[0], sizeof(line), "%s,%c\n", id, kind);
         }
      }
   }
   else
   {
      if (text != NULL)
      {
         if (config->output_sockets)
         {
            snprintf(&line[0], sizeof(line), "%s,%d,%d,%d,%s\n", id, from, to, kind, text);
         }
         else
         {
            snprintf(&line[0], sizeof(line), "%s,%d,%s\n", id, kind, text);
         }
      }
      else
      {
         if (config->output_sockets)
         {
            snprintf(&line[0], sizeof(line), "%s,%d,%d,%d\n", id, from, to, kind);
         }
         else
         {
            snprintf(&line[0], sizeof(line), "%s,%d\n", id, kind);
         }
      }
   }

   fputs(line, config->file);
   fflush(config->file);

   sem_post(&config->lock);
}

/* fe_zero */
static int
fe_zero(struct message* msg, int client_fd, char** text)
{
   int start, end;
   int counter;
   signed char c;
   char** array = NULL;
   int32_t request;

   if (msg->length < 8)
   {
      return msg->length;
   }

   request = pgprtdbg_read_int32(msg->data + 4);
   
   pgprtdbg_log_line("FE: 0");
   pgprtdbg_log_line("    Request: %d", request);

   if (request == 196608)
   {
      transport = PLAIN;
      counter = 0;

      /* We know where the parameters start, and we know that the message is zero terminated */
      for (int i = 8; i < msg->length - 1; i++)
      {
         c = pgprtdbg_read_byte(msg->data + i);
         if (c == 0)
            counter++;
      }

      array = (char**)malloc(sizeof(char*) * counter);

      counter = 0;
      start = 8;
      end = 8;

      for (int i = 8; i < msg->length - 1; i++)
      {
         c = pgprtdbg_read_byte(msg->data + i);
         end++;
         if (c == 0)
         {
            array[counter] = (char*)malloc(end - start);
            memset(array[counter], 0, end - start);
            memcpy(array[counter], msg->data + start, end - start);
               
            start = end;
            counter++;
         }
      }
         
      for (int i = 0; i < counter; i++)
      {
         pgprtdbg_log_line("    Data: %s", array[i]);
      }

      for (int i = 0; i < counter; i++)
      {
         free(array[i]);
      }
      free(array);
   }
   else if (request == 80877102)
   {
      pgprtdbg_log_line("    PID: %d", pgprtdbg_read_int32(msg->data + 8));
      pgprtdbg_log_line("    Secret: %d", pgprtdbg_read_int32(msg->data + 12));
   }
   else if (request == 80877103)
   {
      /* SSL: Not supported */
      transport = SSL;
   }
   else if (request == 80877104)
   {
      /* GSS: Not supported */
   }
   else if (request == 131072)
   {
      /* Protocol v2: Not supported */
      pgprtdbg_write_connection_refused_old(client_fd);
      pgprtdbg_write_empty(client_fd);
      goto error;
   }
   else
   {
      exit(1);
   }

   return msg->length;

error:

   return -1;
}

/* fe_B */
static int
fe_B(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char* destination;
   char* source;
   int16_t codes;
   int16_t values;
   int16_t results;

   o = offset;

   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   destination = pgprtdbg_read_string(msg->data + o);
   o += strlen(destination) + 1;

   source = pgprtdbg_read_string(msg->data + o);
   o += strlen(source) + 1;

   codes = pgprtdbg_read_int16(msg->data + o);
   o += 2;

   for (int16_t i = 0; i < codes; i++)
   {
      pgprtdbg_read_int16(msg->data + o);
      o += 2;
   }

   values = pgprtdbg_read_int16(msg->data + o);
   o += 2;

   for (int16_t i = 0; i < values; i++)
   {
      int32_t size;

      size = pgprtdbg_read_int32(msg->data + o);
      o += 4;

      if (size != -1)
      {
         for (int32_t j = 0; j < size; j++)
         {
            pgprtdbg_read_byte(msg->data + o);
            o += 1;
         }
      }
   }

   results = pgprtdbg_read_int16(msg->data + o);
   o += 2;

   for (int16_t i = 0; i < results; i++)
   {
      pgprtdbg_read_int16(msg->data + o);
      o += 2;
   }

   pgprtdbg_log_line("FE: B");
   pgprtdbg_log_line("    Destination: %s", destination);
   pgprtdbg_log_line("    Source: %s", source);
   pgprtdbg_log_line("    Codes: %d", codes);
   pgprtdbg_log_line("    Values: %d", values);
   pgprtdbg_log_line("    Results: %d", results);

   return offset + length + 1;
}

/* fe_C */
static int
fe_C(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char type;
   char* portal;

   o = offset;

   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   type = pgprtdbg_read_byte(msg->data + o);
   o += 1;

   portal = pgprtdbg_read_string(msg->data + o);
   o += strlen(portal) + 1;

   pgprtdbg_log_line("FE: C");
   pgprtdbg_log_line("    Type: %c", type);
   pgprtdbg_log_line("    Portal: %s", portal);

   return offset + length + 1;
}

/* fe_D */
static int
fe_D(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char type;
   char* name;

   o = offset;
   o += 1;

   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   type = pgprtdbg_read_byte(msg->data + o);
   o += 1;

   name = pgprtdbg_read_string(msg->data + o);
   o += strlen(name) + 1;

   pgprtdbg_log_line("FE: D");
   pgprtdbg_log_line("    Type: %c", type);
   pgprtdbg_log_line("    Name: %s", name);

   return offset + length + 1;
}

/* fe_E */
static int
fe_E(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char* portal;
   int32_t rows;

   o = offset;

   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   portal = pgprtdbg_read_string(msg->data + o);
   o += strlen(portal) + 1;

   rows = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("FE: E");
   pgprtdbg_log_line("    Portal: %s", portal);
   pgprtdbg_log_line("    MaxRows: %d", rows);

   return offset + length + 1;
}

/* fe_F */
static int
fe_F(struct message* msg, int offset, char** text)
{
   return msg->length;
}

/* fe_H */
static int
fe_H(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("FE: H");

   return offset + length + 1;
}

/* fe_P */
static int
fe_P(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char* destination;
   char* query;
   int16_t parameters;

   o = offset;

   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   destination = pgprtdbg_read_string(msg->data + o);
   o += strlen(destination) + 1;

   query = pgprtdbg_read_string(msg->data + o);
   o += strlen(query) + 1;

   parameters = pgprtdbg_read_int16(msg->data + o);
   o += 2;

   pgprtdbg_log_line("FE: P");
   pgprtdbg_log_line("    Destination: %s", destination);
   pgprtdbg_log_line("    Query: %s", query);
   pgprtdbg_log_line("    Parameters: %d", parameters);

   for (int16_t i = 0; i < parameters; i++)
   {
      int32_t oid;

      oid = pgprtdbg_read_int32(msg->data + o);
      o += 4;

      pgprtdbg_log_line("    OID: %d", oid);
   }

   return offset + length + 1;
}

/* fe_Q */
static int
fe_Q(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char* query;

   o = offset;

   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   query = pgprtdbg_read_string(msg->data + o);
   o += strlen(query) + 1;

   pgprtdbg_log_line("FE: Q");
   pgprtdbg_log_line("    Query: %s", query);

   return offset + length + 1;
}

/* fe_S */
static int
fe_S(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("FE: S");

   return offset + length + 1;
}

/* fe_X */
static int
fe_X(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("FE: X");

   return offset + length + 1;
}

/* fe_c */
static int
fe_c(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("FE: c");

   return offset + length + 1;
}

/* fe_d */
static int
fe_d(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   for (int32_t j = 0; j < length - 4; j++)
   {
      pgprtdbg_read_byte(msg->data + o);
      o += 1;
   }

   pgprtdbg_log_line("FE: d");
   pgprtdbg_log_line("    Size: %d", length - 4);

   return offset + length + 1;
}

/* fe_f */
static int
fe_f(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char* failure;

   o = offset;

   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   failure = pgprtdbg_read_string(msg->data + o);
   o += strlen(failure) + 1;

   pgprtdbg_log_line("FE: f");
   pgprtdbg_log_line("    Failure: %s", failure);

   return offset + length + 1;
}

/* fe_p */
static int
fe_p(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("FE: p");
   pgprtdbg_log_mem(msg->data + o, length - 4);

   return offset + length + 1;
}

/* be_one */
static int
be_one(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: 1");

   return offset + length + 1;
}

/* be_two */
static int
be_two(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: 2");

   return offset + length + 1;
}

/* be_three */
static int
be_three(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: 3");

   return offset + length + 1;
}

/* be_A */
static int
be_A(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: A");

   return offset + length + 1;
}

/* be_C */
static int
be_C(struct message* msg, int offset, char** text)
{
   char* str = NULL;
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   str = pgprtdbg_read_string(msg->data + o);
   o += strlen(str) + 1;
   
   pgprtdbg_log_line("BE: C");
   pgprtdbg_log_line("    Tag: %s", str);

   return offset + length + 1;
}

/* be_D */
static int
be_D(struct message* msg, int offset, int from, int to, char** text, struct message** new_msg, int* new_offset)
{
   int o;
   int32_t l;
   int16_t number_of_columns;
   int32_t column_length;
   struct message* m;

   *text = NULL;
   *new_msg = NULL;
   *new_offset = 0;

   m = (struct message*)msg;

   o = offset;
   o += 1;
   l = pgprtdbg_read_int32(m->data + o);
   o += 4;

   number_of_columns = pgprtdbg_read_int16(m->data + o);
   o += 2;

   pgprtdbg_log_line("BE: D");
   pgprtdbg_log_line("    Columns: %d", number_of_columns);
   for (int16_t i = 1; i <= number_of_columns; i++)
   {
      column_length = pgprtdbg_read_int32(m->data + o);
      o += 4;

      pgprtdbg_log_line("    Column: %d", i);
      pgprtdbg_log_line("    Length: %d", column_length);

      if (column_length != -1)
      {
         char buf[column_length];

         pgprtdbg_log_line("    Data: XXXX");

         memset(&buf, 0, column_length);

         for (int32_t j = 0; j < column_length; j++)
         {
            if (o < m->length)
            {
               buf[j] = pgprtdbg_read_byte(m->data + o);
               o += 1;
            }
            else
            {
               int status;

               status = pgprtdbg_write_message(to, m);
               if (status != MESSAGE_STATUS_OK)
               {
                  return -1;
               }

               status = pgprtdbg_read_message(from, new_msg);
               if (status != MESSAGE_STATUS_OK)
               {
                  return -1;
               }

               m = *new_msg;
               o = 0;
               j--;

            }
         }
      }
      else
      {
         pgprtdbg_log_line("    Data: NULL");
      }
   }

   *new_offset = o;

   return offset + l + 1;
}

/* be_E */
static int
be_E(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   signed char type;
   char* str;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: E");

   while (o < length - 4)
   {
      type = pgprtdbg_read_byte(msg->data + o);
      str = pgprtdbg_read_string(msg->data + o + 1);

      pgprtdbg_log_line("    Code: %c", type);
      pgprtdbg_log_line("    Value: %s", str);

      o += (strlen(str) + 2);
   }

   return offset + length + 1;
}

/* be_G */
static int
be_G(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: G");

   return offset + length + 1;
}

/* be_H */
static int
be_H(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: H");

   return offset + length + 1;
}

/* be_I */
static int
be_I(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: I");

   return offset + length + 1;
}

/* be_K */
static int
be_K(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   int32_t process;
   int32_t secret;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   process = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   secret = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: K");
   pgprtdbg_log_line("    Process: %d", process);
   pgprtdbg_log_line("    Secret: %d", secret);

   return offset + length + 1;
}

/* be_N */
static int
be_N(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: N");

   return offset + length + 1;
}

/* be_R */
static int
be_R(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   int32_t type;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;
   type = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: R");

   switch (type)
   {
      case 0:
         pgprtdbg_log_line("    Success");
         break;
      case 2:
         pgprtdbg_log_line("    KerberosV5");
         break;
      case 3:
         pgprtdbg_log_line("    CleartextPassword");
         break;
      case 5:
         o += 4;
         break;
      case 6:
         pgprtdbg_log_line("    SCMCredential");
         break;
      case 7:
         pgprtdbg_log_line("    GSS");
         break;
      case 8:
         pgprtdbg_log_line("    GSSContinue");
         break;
      case 9:
         pgprtdbg_log_line("    SSPI");
         break;
      case 10:
         pgprtdbg_log_line("    SASL");
         while (o < length - 8)
         {
            char* mechanism = pgprtdbg_read_string(msg->data + o);
            pgprtdbg_log_line("    %s", mechanism);
            o += strlen(mechanism) + 1;
         }
         o += 1;
         break;
      case 11:
         pgprtdbg_log_line("    SASLContinue");
         o += length - 8;
         break;
      case 12:
         pgprtdbg_log_line("    SASLFinal");
         o += length - 8;
         break;
      default:
         break;
   }

   return offset + length + 1;
}

/* be_S */
static int
be_S(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char* name = NULL;
   char* value = NULL;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   name = pgprtdbg_read_string(msg->data + o);
   o += strlen(name) + 1;

   value = pgprtdbg_read_string(msg->data + o);
   o += strlen(value) + 1;

   pgprtdbg_log_line("BE: S");
   pgprtdbg_log_line("    Name: %s", name);
   pgprtdbg_log_line("    Value: %s", value);

   return offset + length + 1;
}

/* be_T */
static int
be_T(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   int16_t number_of_fields;
   char* field_name = NULL;
   int32_t oid;
   int16_t attr;
   int32_t type_oid;
   int16_t type_length;
   int32_t type_modifier;
   int16_t format;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   number_of_fields = pgprtdbg_read_int16(msg->data + o);
   o += 2;

   pgprtdbg_log_line("BE: T");
   pgprtdbg_log_line("    Number: %d", number_of_fields);
   for (int16_t i = 0; i < number_of_fields; i++)
   {
      field_name = pgprtdbg_read_string(msg->data + o);
      o += strlen(field_name) + 1;

      oid = pgprtdbg_read_int32(msg->data + o);
      o += 4;

      attr = pgprtdbg_read_int16(msg->data + o);
      o += 2;

      type_oid = pgprtdbg_read_int32(msg->data + o);
      o += 4;

      type_length = pgprtdbg_read_int16(msg->data + o);
      o += 2;

      type_modifier = pgprtdbg_read_int32(msg->data + o);
      o += 4;

      format = pgprtdbg_read_int16(msg->data + o);
      o += 2;

      pgprtdbg_log_line("    Name: %s", field_name);
      pgprtdbg_log_line("    OID: %d", oid);
      pgprtdbg_log_line("    Attribute: %d", attr);
      pgprtdbg_log_line("    Type OID: %d", type_oid);
      pgprtdbg_log_line("    Type length: %d", type_length);
      pgprtdbg_log_line("    Type modifier: %d", type_modifier);
      pgprtdbg_log_line("    Format: %d", format);
   }

   return offset + length + 1;
}

/* be_V */
static int
be_V(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: V");

   return offset + length + 1;
}

/* be_W */
static int
be_W(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: W");

   return offset + length + 1;
}

/* be_Z */
static int
be_Z(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;
   char buf[2];

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   memset(&buf, 0, 2);

   buf[0] = pgprtdbg_read_byte(msg->data + o);
   o += 1;
   
   pgprtdbg_log_line("BE: Z");
   pgprtdbg_log_line("    State: %s", buf);

   return offset + length + 1;
}

/* be_c */
static int
be_c(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: c");

   return offset + length + 1;
}

/* be_d */
static int
be_d(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   for (int32_t j = 0; j < length - 4; j++)
   {
      pgprtdbg_read_byte(msg->data + o);
      o += 1;
   }

   pgprtdbg_log_line("BE: d");
   pgprtdbg_log_line("    Size: %d", length - 4);

   return offset + length + 1;
}

/* be_n */
static int
be_n(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: n");

   return offset + length + 1;
}

/* be_s */
static int
be_s(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: s");

   return offset + length + 1;
}

/* be_t */
static int
be_t(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: t");

   return offset + length + 1;
}

/* be_v */
static int
be_v(struct message* msg, int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   pgprtdbg_log_line("BE: v");

   return offset + length + 1;
}
