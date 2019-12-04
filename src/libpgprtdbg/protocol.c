/*
 * Copyright (C) 2019 Red Hat
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
#include <protocol.h>
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

static void output_write(char* id, void* shmem, signed char kind, char* text);

static int fe_zero(struct message* msg, char** text);
static int fe_Q(struct message* msg, char** text);
static int fe_p(struct message* msg, char** text);
static int fe_X(struct message* msg, char** text);

static int be_C(struct message* msg, int offset, char** text);
static int be_D(struct message* msg, int offset, char** text);
static int be_E(struct message* msg, char** text);
static int be_K(struct message* msg, int offset, char** text);
static int be_N(struct message* msg, int offset, char** text);
static int be_R(struct message* msg, int offset, char** text);
static int be_S(struct message* msg, int offset, char** text);
static int be_T(struct message* msg, char** text);
static int be_Z(struct message* msg, int offset, char** text);

/**
 *
 */
void
pgprtdbg_process(char* id, void* shmem, struct message* msg)
{
   char* text = NULL;
   signed char kind;
   int offset;

   ZF_LOGV_MEM(msg->data, msg->length, "Message %p:", (const void *)msg->data);

   offset = 0;

   while (offset < msg->length)
   {
      kind = pgprtdbg_read_byte(msg->data + offset);

      switch (kind)
      {
         case 0:
            offset = fe_zero(msg, &text);
            break;
         case 'Q':
            offset = fe_Q(msg, &text);
            break;
         case 'p':
            offset = fe_p(msg, &text);
            break;
         case 'X':
            offset = fe_X(msg, &text);
            break;
         case 'C':
            offset = be_C(msg, offset, &text);
            break;
         case 'D':
            offset = be_D(msg, offset, &text);
            break;
         case 'E':
            offset = be_E(msg, &text);
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
            offset = be_T(msg, &text);
            break;
         case 'Z':
            offset = be_Z(msg, offset, &text);
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

      output_write(id, shmem, kind, text);
      free(text);
      text = NULL;
   }
}

static void
output_write(char* id, void* shmem, signed char kind, char* text)
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
         snprintf(&line[0], sizeof(line), "%s,%c,%s\n", id, kind, text);
      }
      else
      {
         snprintf(&line[0], sizeof(line), "%s,%c\n", id, kind);
      }
   }
   else
   {
      if (text != NULL)
      {
         snprintf(&line[0], sizeof(line), "%s,%d,%s\n", id, kind, text);
      }
      else
      {
         snprintf(&line[0], sizeof(line), "%s,%d\n", id, kind);
      }
   }

   fputs(line, config->file);
   fflush(config->file);

   sem_post(&config->lock);
}

static int
fe_zero(struct message* msg, char** text)
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
   }
   else if (request == 80877104)
   {
      /* GSS: Not supported */
   }
   else
   {
      printf("Unknown request: %d\n", request);
      exit(1);
   }

   return msg->length;
}

static int
fe_Q(struct message* msg, char** text)
{
   ZF_LOGV("FE: Q");

   return msg->length;
}


static int
fe_p(struct message* msg, char** text)
{
   ZF_LOGV("FE: p");
   ZF_LOGV("Data: %s", pgprtdbg_read_string(msg->data + 5));

   return msg->length;
}


static int
fe_X(struct message* msg, char** text)
{
   ZF_LOGV("FE: X");

   return msg->length;
}

static int
be_C(struct message* msg, int offset, char** text)
{
   char* str = NULL;

   str = pgprtdbg_read_string(msg->data + offset + 5);
   offset += 5;
   
   ZF_LOGV("BE: C");
   ZF_LOGV("Data: %s", str);

   offset += strlen(str) + 1;

   return offset;
}

static int
be_D(struct message* msg, int offset, char** text)
{
   int16_t number_of_columns;
   int32_t column_length;

   number_of_columns = pgprtdbg_read_int16(msg->data + offset + 5);
   offset += 7;

   ZF_LOGV("BE: D");
   ZF_LOGV("Number: %d", number_of_columns);
   for (int16_t i = 0; i < number_of_columns; i++)
   {
      column_length = pgprtdbg_read_int32(msg->data + offset);
      offset += 4;

      char buf[column_length + 1];
      memset(&buf, 0, column_length + 1);
      
      for (int16_t j = 0; j < column_length; j++)
      {
         buf[j] = pgprtdbg_read_byte(msg->data + offset);
         offset += 1;
      }

      ZF_LOGV("Length: %d", column_length);
      ZF_LOGV("Data  : %s", buf);
   }

   return offset;
}

static int
be_E(struct message* msg, char** text)
{
   int32_t length;
   int offset;
   signed char type;
   char* str;

   length = pgprtdbg_read_int32(msg->data + 1);
   offset = 5;

   ZF_LOGV("BE: E");
   while (offset < length - 4)
   {
      type = pgprtdbg_read_byte(msg->data + offset);
      str = pgprtdbg_read_string(msg->data + offset + 1);

      ZF_LOGV("Data: %c %s", type, str);

      offset += (strlen(str) + 2);
   }

   return offset;
}

static int
be_K(struct message* msg, int offset, char** text)
{
   int32_t process;
   int32_t secret;

   offset += 5;

   process = pgprtdbg_read_int32(msg->data + offset);
   offset += 4;

   secret = pgprtdbg_read_int32(msg->data + offset);
   offset += 4;

   ZF_LOGV("BE: K");
   ZF_LOGV("Process: %d", process);
   ZF_LOGV("Secret : %d", secret);

   return offset;
}

static int
be_N(struct message* msg, int offset, char** text)
{
   ZF_LOGV("BE: N");

   return msg->length;
}

static int
be_R(struct message* msg, int offset, char** text)
{
   int32_t length;
   int32_t type;

   length = pgprtdbg_read_int32(msg->data + offset + 1);
   type = pgprtdbg_read_int32(msg->data + offset + 5);
   offset += 9;

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
                 (signed char)(pgprtdbg_read_byte(msg->data + 9) & 0xFF),
                 (signed char)(pgprtdbg_read_byte(msg->data + 10) & 0xFF),
                 (signed char)(pgprtdbg_read_byte(msg->data + 11) & 0xFF),
                 (signed char)(pgprtdbg_read_byte(msg->data + 12) & 0xFF));
         offset += 4;
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
         while (offset < length - 8)
         {
            char* mechanism = pgprtdbg_read_string(msg->data + offset);
            ZF_LOGV("             %s", mechanism);
            offset += strlen(mechanism) + 1;
         }
         break;
      case 11:
         ZF_LOGV("BE: R - SASLContinue");
         ZF_LOGV_MEM(msg->data + offset, length - 8, "Message %p:", (const void *)msg->data + offset);
         offset += length - 8;
         break;
      case 12:
         ZF_LOGV("BE: R - SASLFinal");
         ZF_LOGV_MEM(msg->data + offset, length - 8, "Message %p:", (const void *)msg->data + offset);
         offset += length - 8;
         break;
      default:
         break;
   }

   return offset;
}

static int
be_S(struct message* msg, int offset, char** text)
{
   char* name = NULL;
   char* value = NULL;

   offset += 5;

   name = pgprtdbg_read_string(msg->data + offset);
   offset += strlen(name) + 1;

   value = pgprtdbg_read_string(msg->data + offset);
   offset += strlen(value) + 1;

   ZF_LOGV("BE: S");
   ZF_LOGV("Name : %s", name);
   ZF_LOGV("Value: %s", value);

   return offset;
}

static int
be_T(struct message* msg, char** text)
{
   int16_t number_of_fields;
   char* field_name = NULL;
   int32_t oid;
   int16_t attr;
   int32_t type_oid;
   int16_t type_length;
   int32_t type_modifier;
   int16_t format;
   int offset;

   number_of_fields = pgprtdbg_read_int16(msg->data + 5);
   offset = 7;

   ZF_LOGV("BE: T");
   ZF_LOGV("Number       : %d", number_of_fields);
   for (int16_t i = 0; i < number_of_fields; i++)
   {
      field_name = pgprtdbg_read_string(msg->data + offset);
      offset += strlen(field_name) + 1;

      oid = pgprtdbg_read_int32(msg->data + offset);
      offset += 4;

      attr = pgprtdbg_read_int16(msg->data + offset);
      offset += 2;

      type_oid = pgprtdbg_read_int32(msg->data + offset);
      offset += 4;

      type_length = pgprtdbg_read_int16(msg->data + offset);
      offset += 2;

      type_modifier = pgprtdbg_read_int32(msg->data + offset);
      offset += 4;

      format = pgprtdbg_read_int16(msg->data + offset);
      offset += 2;

      ZF_LOGV("Name         : %s", field_name);
      ZF_LOGV("OID          : %d", oid);
      ZF_LOGV("Attribute    : %d", attr);
      ZF_LOGV("Type OID     : %d", type_oid);
      ZF_LOGV("Type length  : %d", type_length);
      ZF_LOGV("Type modifier: %d", type_modifier);
      ZF_LOGV("Format       : %d", format);
   }

   return offset;
}

static int
be_Z(struct message* msg, int offset, char** text)
{
   char buf[2];

   memset(&buf, 0, 2);

   offset += 5;
   buf[0] = pgprtdbg_read_byte(msg->data + offset);
   offset += 1;
   
   ZF_LOGV("BE: Z");
   ZF_LOGV("Data: %s", buf);

   return offset;
}
