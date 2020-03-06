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
#include <message.h>
#include <pipeline.h>
#include <protocol.h>
#include <worker.h>
#include <utils.h>

#define ZF_LOG_TAG "protocol"
#include <zf_log.h>

/* system */
#include <ev.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

static void output_write(char* id, int from, int to, void* shmem, signed char kind, char* text);

static int fe_zero(int client_fd, struct message* msg, char** text);
static int fe_B(struct message* msg, char** text);
static int fe_C(struct message* msg, char** text);
static int fe_D(struct message* msg, char** text);
static int fe_E(struct message* msg, char** text);
static int fe_F(struct message* msg, char** text);
static int fe_H(struct message* msg, char** text);
static int fe_P(struct message* msg, char** text);
static int fe_Q(struct message* msg, char** text);
static int fe_S(struct message* msg, char** text);
static int fe_X(struct message* msg, char** text);
static int fe_c(struct message* msg, char** text);
static int fe_d(struct message* msg, char** text);
static int fe_f(struct message* msg, char** text);
static int fe_p(struct message* msg, char** text);

static int be_one(const struct message* msg, const int offset, char** text);
static int be_two(const struct message* msg, const int offset, char** text);
static int be_three(const struct message* msg, const int offset, char** text);
static int be_A(const struct message* msg, const int offset, char** text);
static int be_C(const struct message* msg, const int offset, char** text);
static int be_D(const struct message* msg, const int offset, char** text);
static int be_E(const struct message* msg, const int offset, char** text);
static int be_G(const struct message* msg, const int offset, char** text);
static int be_H(const struct message* msg, const int offset, char** text);
static int be_I(const struct message* msg, const int offset, char** text);
static int be_K(const struct message* msg, const int offset, char** text);
static int be_N(const struct message* msg, const int offset, char** text);
static int be_R(const struct message* msg, const int offset, char** text);
static int be_S(const struct message* msg, const int offset, char** text);
static int be_T(const struct message* msg, const int offset, char** text);
static int be_V(const struct message* msg, const int offset, char** text);
static int be_W(const struct message* msg, const int offset, char** text);
static int be_Z(const struct message* msg, const int offset, char** text);
static int be_n(const struct message* msg, const int offset, char** text);
static int be_s(const struct message* msg, const int offset, char** text);
static int be_t(const struct message* msg, const int offset, char** text);
static int be_v(const struct message* msg, const int offset, char** text);

void
pgprtdbg_client(int from, int to, void* shmem, struct message* msg)
{
   char* text = NULL;
   signed char kind;
   int offset;

   ZF_LOGV_MEM(msg->data, msg->length, "Client message %p:", (const void *)msg->data);

   offset = 0;

   while (offset < msg->length)
   {
      kind = pgprtdbg_read_byte(msg->data + offset);

      switch (kind)
      {
         case 0:
            offset = fe_zero(from, msg, &text);
            break;
         case 'B':
            offset = fe_B(msg, &text);
            break;
         case 'C':
            offset = fe_C(msg, &text);
            break;
         case 'D':
            offset = fe_D(msg, &text);
            break;
         case 'E':
            offset = fe_E(msg, &text);
            break;
         case 'F':
            offset = fe_F(msg, &text);
            break;
         case 'H':
            offset = fe_H(msg, &text);
            break;
         case 'P':
            offset = fe_P(msg, &text);
            break;
         case 'Q':
            offset = fe_Q(msg, &text);
            break;
         case 'S':
            offset = fe_S(msg, &text);
            break;
         case 'X':
            offset = fe_X(msg, &text);
            break;
         case 'c':
            offset = fe_c(msg, &text);
            break;
         case 'd':
            offset = fe_d(msg, &text);
            break;
         case 'f':
            offset = fe_f(msg, &text);
            break;
         case 'p':
            offset = fe_p(msg, &text);
            break;
         default:
            if ((kind >= 'A' && kind <= 'Z') || (kind >= 'a' && kind <= 'z'))
            {
               if ((msg->kind >= 'A' && msg->kind <= 'Z') || (msg->kind >= 'a' && msg->kind <= 'z'))
               {
                  ZF_LOGI("Unsupported: %c (%c)", kind, msg->kind);
               }
               else
               {
                  ZF_LOGI("Unsupported: %c (%d)", kind, msg->kind);
               }
            }
            else
            {
               ZF_LOGI("Unsupported: %d (%d)", kind, msg->kind);
            }

            offset = msg->length;
            break;
      }

      output_write("C", from, to, shmem, kind, text);
      free(text);
      text = NULL;

      if (offset == -1)
      {
         exit_code = WORKER_CLIENT_FAILURE;
         running = 0;
         return;
      }
   }
}

void
pgprtdbg_server(int from, int to, void* shmem, struct message* msg)
{
   char* text = NULL;
   signed char kind;
   int offset;

   ZF_LOGV_MEM(msg->data, msg->length, "Server message %p:", (const void *)msg->data);

   offset = 0;

   while (offset < msg->length)
   {
      kind = pgprtdbg_read_byte(msg->data + offset);

      switch (kind)
      {
         case '1':
            offset = be_one(msg, offset, &text);
            break;
         case '2':
            offset = be_two(msg, offset, &text);
            break;
         case '3':
            offset = be_three(msg, offset, &text);
            break;
         case 'A':
            offset = be_A(msg, offset, &text);
            break;
         case 'C':
            offset = be_C(msg, offset, &text);
            break;
         case 'D':
            offset = be_D(msg, offset, &text);
            break;
         case 'E':
            offset = be_E(msg, offset, &text);
            break;
         case 'G':
            offset = be_G(msg, offset, &text);
            break;
         case 'H':
            offset = be_H(msg, offset, &text);
            break;
         case 'I':
            offset = be_I(msg, offset, &text);
            break;
         case 'K':
            offset = be_K(msg, offset, &text);
            break;
         case 'N':
            offset = be_N(msg, offset, &text);
            break;
         case 'R':
            offset = be_R(msg, offset, &text);
            break;
         case 'S':
            offset = be_S(msg, offset, &text);
            break;
         case 'T':
            offset = be_T(msg, offset, &text);
            break;
         case 'V':
            offset = be_V(msg, offset, &text);
            break;
         case 'W':
            offset = be_W(msg, offset, &text);
            break;
         case 'Z':
            offset = be_Z(msg, offset, &text);
            break;
         case 'n':
            offset = be_n(msg, offset, &text);
            break;
         case 's':
            offset = be_s(msg, offset, &text);
            break;
         case 't':
            offset = be_t(msg, offset, &text);
            break;
         case 'v':
            offset = be_v(msg, offset, &text);
            break;
         default:
            if ((kind >= 'A' && kind <= 'Z') || (kind >= 'a' && kind <= 'z'))
            {
               if ((msg->kind >= 'A' && msg->kind <= 'Z') || (msg->kind >= 'a' && msg->kind <= 'z'))
               {
                  ZF_LOGI("Unsupported: %c (%c)", kind, msg->kind);
               }
               else
               {
                  ZF_LOGI("Unsupported: %c (%d)", kind, msg->kind);
               }
            }
            else
            {
               ZF_LOGI("Unsupported: %d (%d)", kind, msg->kind);
            }

            offset = msg->length;
            break;
      }

      output_write("S", from, to, shmem, kind, text);
      free(text);
      text = NULL;

      if (offset == -1)
      {
         exit_code = WORKER_SERVER_FAILURE;
         running = 0;
         return;
      }
   }
}

static void
output_write(char* id, int from, int to, void* shmem, signed char kind, char* text)
{
   char line[MISC_LENGTH];
   struct configuration* config;

   memset(&line, 0, sizeof(line));
   config = (struct configuration*)shmem;

   sem_wait(&config->lock);

   if ((kind >= 'A' && kind <= 'Z') || (kind >= 'a' && kind <= 'z'))
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
fe_zero(int client_fd, struct message* msg, char** text)
{
   int start, end;
   int counter;
   signed char c;
   char** array = NULL;
   int32_t length;
   int32_t request;

   if (msg->length < 8)
   {
      return msg->length;
   }

   length = pgprtdbg_read_int32(msg->data);
   request = pgprtdbg_read_int32(msg->data + 4);
   
   ZF_LOGV("FE: 0 Length: %d Request: %d", length, request);

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
         ZF_LOGV("FE: 0/Req Data: %s", array[i]);
      }

      for (int i = 0; i < counter; i++)
      {
         free(array[i]);
      }
      free(array);
   }
   else if (request == 80877103)
   {
      /* SSL: Not supported */
      ZF_LOGD("SSL");
      transport = SSL;
   }
   else if (request == 80877104)
   {
      /* GSS: Not supported */
      ZF_LOGD("GSS");
   }
   else if (request == 131072)
   {
      /* Protocol v2: Not supported */
      ZF_LOGD("protocol v2");
      pgprtdbg_write_connection_refused_old(client_fd);
      pgprtdbg_write_empty(client_fd);
      goto error;
   }
   else
   {
      ZF_LOGE("Unknown request: %d\n", request);
      ZF_LOGE_MEM(msg->data, msg->length, "Message %p:", (const void *)msg->data);
      exit(1);
   }

   return msg->length;

error:

   return -1;
}

/* fe_B */
static int
fe_B(struct message* msg, char** text)
{
   ZF_LOGV("FE: B");

   return msg->length;
}

/* fe_C */
static int
fe_C(struct message* msg, char** text)
{
   ZF_LOGV("FE: C");

   return msg->length;
}

/* fe_D */
static int
fe_D(struct message* msg, char** text)
{
   ZF_LOGV("FE: D");

   return msg->length;
}

/* fe_E */
static int
fe_E(struct message* msg, char** text)
{
   ZF_LOGV("FE: E");

   return msg->length;
}

/* fe_F */
static int
fe_F(struct message* msg, char** text)
{
   ZF_LOGV("FE: F");

   return msg->length;
}

/* fe_H */
static int
fe_H(struct message* msg, char** text)
{
   ZF_LOGV("FE: H");

   return msg->length;
}

/* fe_P */
static int
fe_P(struct message* msg, char** text)
{
   ZF_LOGV("FE: P");

   return msg->length;
}

/* fe_Q */
static int
fe_Q(struct message* msg, char** text)
{
   ZF_LOGV("FE: Q");
   ZF_LOGV("Data: %s", (char*)(msg->data + 5));

   return msg->length;
}

/* fe_S */
static int
fe_S(struct message* msg, char** text)
{
   ZF_LOGV("FE: S");

   return msg->length;
}

/* fe_X */
static int
fe_X(struct message* msg, char** text)
{
   ZF_LOGV("FE: X");

   return msg->length;
}

/* fe_c */
static int
fe_c(struct message* msg, char** text)
{
   ZF_LOGV("FE: c");

   return msg->length;
}

/* fe_d */
static int
fe_d(struct message* msg, char** text)
{
   ZF_LOGV("FE: d");

   return msg->length;
}

/* fe_f */
static int
fe_f(struct message* msg, char** text)
{
   ZF_LOGV("FE: f");

   return msg->length;
}

/* fe_p */
static int
fe_p(struct message* msg, char** text)
{
   int32_t length;

   length = pgprtdbg_read_int32(msg->data + 1);

   ZF_LOGV("FE: p");
   ZF_LOGV_MEM(msg->data + 5, length - 4, "Data: %p", (const void *)msg->data + 5);

   return msg->length;
}

/* be_one */
static int
be_one(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: 1");

   return offset + length + 1;
}

/* be_two */
static int
be_two(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: 2");

   return offset + length + 1;
}

/* be_three */
static int
be_three(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: 3");

   return offset + length + 1;
}

/* be_A */
static int
be_A(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: A");

   return offset + length + 1;
}

/* be_C */
static int
be_C(const struct message* msg, const int offset, char** text)
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
   
   ZF_LOGV("BE: C");
   ZF_LOGV("Data: %s", str);

   return offset + length + 1;
}

/* be_D */
static int
be_D(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;
   int16_t number_of_columns;
   int32_t column_length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   number_of_columns = pgprtdbg_read_int16(msg->data + o);
   o += 2;

   ZF_LOGV("BE: D");
   ZF_LOGV("Number: %d", number_of_columns);
   for (int16_t i = 0; i < number_of_columns; i++)
   {
      column_length = pgprtdbg_read_int32(msg->data + o);
      o += 4;

      char buf[column_length + 1];
      memset(&buf, 0, column_length + 1);
      
      for (int16_t j = 0; j < column_length; j++)
      {
         buf[j] = pgprtdbg_read_byte(msg->data + o);
         o += 1;
      }

      ZF_LOGV("Length: %d", column_length);
      ZF_LOGV("Data  : %s", buf);
   }

   return offset + length + 1;
}

/* be_E */
static int
be_E(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;
   signed char type;
   char* str;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: E");
   while (o < length - 4)
   {
      type = pgprtdbg_read_byte(msg->data + o);
      str = pgprtdbg_read_string(msg->data + o + 1);

      ZF_LOGV("Data: %c %s", type, str);

      o += (strlen(str) + 2);
   }

   return offset + length + 1;
}

/* be_G */
static int
be_G(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: G");

   return offset + length + 1;
}

/* be_H */
static int
be_H(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: H");

   return offset + length + 1;
}

/* be_I */
static int
be_I(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: I");

   return offset + length + 1;
}

/* be_K */
static int
be_K(const struct message* msg, const int offset, char** text)
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

   ZF_LOGV("BE: K");
   ZF_LOGV("Process: %d", process);
   ZF_LOGV("Secret : %d", secret);

   return offset + length + 1;
}

/* be_N */
static int
be_N(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: N");

   return offset + length + 1;
}

/* be_R */
static int
be_R(const struct message* msg, const int offset, char** text)
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

   switch (type)
   {
      case 0:
         ZF_LOGV("BE: R - Success");
         break;
      case 2:
         ZF_LOGV("BE: R - KerberosV5");
         break;
      case 3:
         ZF_LOGV("BE: R - CleartextPassword");
         break;
      case 5:
         ZF_LOGV("BE: R - MD5Password");
         ZF_LOGV("             Salt %02hhx%02hhx%02hhx%02hhx",
                 (signed char)(pgprtdbg_read_byte(msg->data + o) & 0xFF),
                 (signed char)(pgprtdbg_read_byte(msg->data + o + 1) & 0xFF),
                 (signed char)(pgprtdbg_read_byte(msg->data + o + 2) & 0xFF),
                 (signed char)(pgprtdbg_read_byte(msg->data + o + 3) & 0xFF));
         o += 4;
         break;
      case 6:
         ZF_LOGV("BE: R - SCMCredential");
         break;
      case 7:
         ZF_LOGV("BE: R - GSS");
         break;
      case 8:
         ZF_LOGV("BE: R - GSSContinue");
         break;
      case 9:
         ZF_LOGV("BE: R - SSPI");
         break;
      case 10:
         ZF_LOGV("BE: R - SASL");
         while (o < length - 8)
         {
            char* mechanism = pgprtdbg_read_string(msg->data + o);
            ZF_LOGV("             %s", mechanism);
            o += strlen(mechanism) + 1;
         }
         o += 1;
         break;
      case 11:
         ZF_LOGV("BE: R - SASLContinue");
         ZF_LOGV_MEM(msg->data + o, length - 8, "Message %p:", (const void *)msg->data + o);
         o += length - 8;
         break;
      case 12:
         ZF_LOGV("BE: R - SASLFinal");
         ZF_LOGV_MEM(msg->data + o, length - 8, "Message %p:", (const void *)msg->data + o);
         o += length - 8;
         break;
      default:
         break;
   }

   return offset + length + 1;
}

/* be_S */
static int
be_S(const struct message* msg, const int offset, char** text)
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

   ZF_LOGV("BE: S");
   ZF_LOGV("Name : %s", name);
   ZF_LOGV("Value: %s", value);

   return offset + length + 1;
}

/* be_T */
static int
be_T(const struct message* msg, const int offset, char** text)
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

   ZF_LOGV("BE: T");
   ZF_LOGV("Number       : %d", number_of_fields);
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

      ZF_LOGV("Name         : %s", field_name);
      ZF_LOGV("OID          : %d", oid);
      ZF_LOGV("Attribute    : %d", attr);
      ZF_LOGV("Type OID     : %d", type_oid);
      ZF_LOGV("Type length  : %d", type_length);
      ZF_LOGV("Type modifier: %d", type_modifier);
      ZF_LOGV("Format       : %d", format);
   }

   return offset + length + 1;
}

/* be_V */
static int
be_V(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: V");

   return offset + length + 1;
}

/* be_W */
static int
be_W(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: W");

   return offset + length + 1;
}

/* be_Z */
static int
be_Z(const struct message* msg, const int offset, char** text)
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
   
   ZF_LOGV("BE: Z");
   ZF_LOGV("Data: %s", buf);

   return offset + length + 1;
}

/* be_n */
static int
be_n(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: n");

   return offset + length + 1;
}

/* be_s */
static int
be_s(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: s");

   return offset + length + 1;
}

/* be_t */
static int
be_t(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: t");

   return offset + length + 1;
}

/* be_v */
static int
be_v(const struct message* msg, const int offset, char** text)
{
   int o;
   int32_t length;

   o = offset;
   o += 1;
   length = pgprtdbg_read_int32(msg->data + o);
   o += 4;

   ZF_LOGV("BE: v");

   return offset + length + 1;
}
