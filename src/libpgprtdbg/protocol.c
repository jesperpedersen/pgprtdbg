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

static void fe_zero(int client_fd, char** text);
static void fe_B(char** text);
static void fe_C(char** text);
static void fe_D(char** text);
static void fe_E(char** text);
static void fe_F(char** text);
static void fe_H(char** text);
static void fe_P(char** text);
static void fe_Q(char** text);
static void fe_S(char** text);
static void fe_X(char** text);
static void fe_c(char** text);
static void fe_d(char** text);
static void fe_f(char** text);
static void fe_p(char** text);

static void be_one(char** text);
static void be_two(char** text);
static void be_three(char** text);
static void be_A(char** text);
static void be_C(char** text);
static void be_D(char** text);
static void be_E(char** text);
static void be_G(char** text);
static void be_H(char** text);
static void be_I(char** text);
static void be_K(char** text);
static void be_N(char** text);
static void be_R(char** text);
static void be_S(char** text);
static void be_T(char** text);
static void be_V(char** text);
static void be_W(char** text);
static void be_Z(char** text);
static void be_c(char** text);
static void be_d(char** text);
static void be_n(char** text);
static void be_s(char** text);
static void be_t(char** text);
static void be_v(char** text);

static signed char kind = 0;
static int32_t length = 0;

static size_t data_size = 0;
static size_t new_data_size = 0;
static void* data = NULL;

void
pgprtdbg_client(int from, int to, struct message* msg)
{
   char* text = NULL;

   pgprtdbg_log_lock();
   pgprtdbg_log_line("--------");

   pgprtdbg_log_line("FE/Message (%d):", msg->length);
   pgprtdbg_log_mem(msg->data, msg->length);

   data = pgprtdbg_data_append(data, data_size, msg->data, msg->length, &new_data_size);
   data_size = new_data_size;

   while (data_size > 0)
   {
      kind = pgprtdbg_read_byte(data);

      if (kind == 0 && data_size >= 8)
      {
         length = pgprtdbg_read_int32(data);
         fe_zero(from, &text);

         data = pgprtdbg_data_remove(data, data_size, length, &new_data_size);
         data_size = new_data_size;
      }
      else if (kind != 0 && data_size >= 5)
      {
         length = pgprtdbg_read_int32(data + 1);

         if (data_size < length + 1)
         {
            goto done;
         }

         switch (kind)
         {
            case 'B':
               fe_B(&text);
               break;
            case 'C':
               fe_C(&text);
               break;
            case 'D':
               fe_D(&text);
               break;
            case 'E':
               fe_E(&text);
               break;
            case 'F':
               fe_F(&text);
               break;
            case 'H':
               fe_H(&text);
               break;
            case 'P':
               fe_P(&text);
               break;
            case 'Q':
               fe_Q(&text);
               break;
            case 'S':
               fe_S(&text);
               break;
            case 'X':
               fe_X(&text);
               break;
            case 'c':
               fe_c(&text);
               break;
            case 'd':
               fe_d(&text);
               break;
            case 'f':
               fe_f(&text);
               break;
            case 'p':
               fe_p(&text);
               break;
            default:
               pgprtdbg_log_line("Unsupported client message: %d", kind);
               break;
         }

         output_write("C", from, to, kind, text);
         free(text);
         text = NULL;

         data = pgprtdbg_data_remove(data, data_size, length + 1, &new_data_size);
         data_size = new_data_size;
      }
      else
      {
         goto done;
      }
   }

done:

   pgprtdbg_log_unlock();
}

void
pgprtdbg_server(int from, int to, struct message* msg)
{
   char* text = NULL;

   pgprtdbg_log_lock();
   pgprtdbg_log_line("--------");

   pgprtdbg_log_line("BE/Message (%d):", msg->length);
   pgprtdbg_log_mem(msg->data, msg->length);

   data = pgprtdbg_data_append(data, data_size, msg->data, msg->length, &new_data_size);
   data_size = new_data_size;

   while (data_size > 0)
   {
      kind = pgprtdbg_read_byte(data);

      if (kind == 'N' && data_size == 1)
      {
         length = 1;
         be_N(&text);

         data = pgprtdbg_data_remove(data, data_size, length, &new_data_size);
         data_size = new_data_size;
      }
      else if (kind != 0 && data_size >= 5)
      {
         length = pgprtdbg_read_int32(data + 1);

         if (data_size < length + 1)
         {
            goto done;
         }

         switch (kind)
         {
            case '1':
               be_one(&text);
               break;
            case '2':
               be_two(&text);
               break;
            case '3':
               be_three(&text);
               break;
            case 'A':
               be_A(&text);
               break;
            case 'C':
               be_C(&text);
               break;
            case 'D':
               be_D(&text);
               break;
            case 'E':
               be_E(&text);
               break;
            case 'G':
               be_G(&text);
               break;
            case 'H':
               be_H(&text);
               break;
            case 'I':
               be_I(&text);
               break;
            case 'K':
               be_K(&text);
               break;
            case 'N':
               be_N(&text);
               break;
            case 'R':
               be_R(&text);
               break;
            case 'S':
               be_S(&text);
               break;
            case 'T':
               be_T(&text);
               break;
            case 'V':
               be_V(&text);
               break;
            case 'W':
               be_W(&text);
               break;
            case 'Z':
               be_Z(&text);
               break;
            case 'c':
               be_c(&text);
               break;
            case 'd':
               be_d(&text);
               break;
            case 'n':
               be_n(&text);
               break;
            case 's':
               be_s(&text);
               break;
            case 't':
               be_t(&text);
               break;
            case 'v':
               be_v(&text);
               break;
            default:
               pgprtdbg_log_line("Unsupported server message: %d", kind);
               break;
         }

         output_write("S", from, to, kind, text);
         free(text);
         text = NULL;

         data = pgprtdbg_data_remove(data, data_size, length + 1, &new_data_size);
         data_size = new_data_size;
      }
      else
      {
         goto done;
      }
   }

done:

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
static void
fe_zero(int client_fd, char** text)
{
   int start, end;
   int counter;
   signed char c;
   char** array = NULL;
   int32_t request;

   request = pgprtdbg_read_int32(data + 4);
   
   pgprtdbg_log_line("FE: 0");
   pgprtdbg_log_line("    Request: %d", request);

   if (request == 196608)
   {
      counter = 0;

      /* We know where the parameters start, and we know that the message is zero terminated */
      for (int i = 8; i < length - 1; i++)
      {
         c = pgprtdbg_read_byte(data + i);
         if (c == 0)
            counter++;
      }

      array = (char**)malloc(sizeof(char*) * counter);

      counter = 0;
      start = 8;
      end = 8;

      for (int i = 8; i < length - 1; i++)
      {
         c = pgprtdbg_read_byte(data + i);
         end++;
         if (c == 0)
         {
            array[counter] = (char*)malloc(end - start);
            memset(array[counter], 0, end - start);
            memcpy(array[counter], data + start, end - start);
               
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
      pgprtdbg_log_line("    PID: %d", pgprtdbg_read_int32(data + 8));
      pgprtdbg_log_line("    Secret: %d", pgprtdbg_read_int32(data + 12));
   }
   else if (request == 80877103)
   {
      /* SSL: Not supported */
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
   }
   else
   {
      exit(1);
   }
}

/* fe_B */
static void
fe_B(char** text)
{
   int o = 0;
   char* destination;
   char* source;
   int16_t codes;
   int16_t values;
   int16_t results;

   o += 1;
   /* length */
   o += 4;

   destination = pgprtdbg_read_string(data + o);
   o += strlen(destination) + 1;

   source = pgprtdbg_read_string(data + o);
   o += strlen(source) + 1;

   codes = pgprtdbg_read_int16(data + o);
   o += 2;

   for (int16_t i = 0; i < codes; i++)
   {
      pgprtdbg_read_int16(data + o);
      o += 2;
   }

   values = pgprtdbg_read_int16(data + o);
   o += 2;

   for (int16_t i = 0; i < values; i++)
   {
      int32_t size;

      size = pgprtdbg_read_int32(data + o);
      o += 4;

      if (size != -1)
      {
         for (int32_t j = 0; j < size; j++)
         {
            pgprtdbg_read_byte(data + o);
            o += 1;
         }
      }
   }

   results = pgprtdbg_read_int16(data + o);
   o += 2;

   for (int16_t i = 0; i < results; i++)
   {
      pgprtdbg_read_int16(data + o);
      o += 2;
   }

   pgprtdbg_log_line("FE: B");
   pgprtdbg_log_line("    Destination: %s", destination);
   pgprtdbg_log_line("    Source: %s", source);
   pgprtdbg_log_line("    Codes: %d", codes);
   pgprtdbg_log_line("    Values: %d", values);
   pgprtdbg_log_line("    Results: %d", results);
}

/* fe_C */
static void
fe_C(char** text)
{
   int o = 0;
   char type;
   char* portal;

   o += 1;
   /* length */
   o += 4;

   type = pgprtdbg_read_byte(data + o);
   o += 1;

   portal = pgprtdbg_read_string(data + o);
   o += strlen(portal) + 1;

   pgprtdbg_log_line("FE: C");
   pgprtdbg_log_line("    Type: %c", type);
   pgprtdbg_log_line("    Portal: %s", portal);
}

/* fe_D */
static void
fe_D(char** text)
{
   int o = 0;
   char type;
   char* name;

   o += 1;
   /* length */
   o += 4;

   type = pgprtdbg_read_byte(data + o);
   o += 1;

   name = pgprtdbg_read_string(data + o);
   o += strlen(name) + 1;

   pgprtdbg_log_line("FE: D");
   pgprtdbg_log_line("    Type: %c", type);
   pgprtdbg_log_line("    Name: %s", name);
}

/* fe_E */
static void
fe_E(char** text)
{
   int o = 0;
   char* portal;
   int32_t rows;

   o += 1;
   /* length */
   o += 4;

   portal = pgprtdbg_read_string(data + o);
   o += strlen(portal) + 1;

   rows = pgprtdbg_read_int32(data + o);
   o += 4;

   pgprtdbg_log_line("FE: E");
   pgprtdbg_log_line("    Portal: %s", portal);
   pgprtdbg_log_line("    MaxRows: %d", rows);
}

/* fe_F */
static void
fe_F(char** text)
{
}

/* fe_H */
static void
fe_H(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("FE: H");
}

/* fe_P */
static void
fe_P(char** text)
{
   int o = 0;
   char* destination;
   char* query;
   int16_t parameters;

   o += 1;
   /* length */
   o += 4;

   destination = pgprtdbg_read_string(data + o);
   o += strlen(destination) + 1;

   query = pgprtdbg_read_string(data + o);
   o += strlen(query) + 1;

   parameters = pgprtdbg_read_int16(data + o);
   o += 2;

   pgprtdbg_log_line("FE: P");
   pgprtdbg_log_line("    Destination: %s", destination);
   pgprtdbg_log_line("    Query: %s", query);
   pgprtdbg_log_line("    Parameters: %d", parameters);

   for (int16_t i = 0; i < parameters; i++)
   {
      int32_t oid;

      oid = pgprtdbg_read_int32(data + o);
      o += 4;

      pgprtdbg_log_line("    OID: %d", oid);
   }
}

/* fe_Q */
static void
fe_Q(char** text)
{
   int o = 0;
   char* query;

   o += 1;
   /* length */
   o += 4;

   query = pgprtdbg_read_string(data + o);
   o += strlen(query) + 1;

   pgprtdbg_log_line("FE: Q");
   pgprtdbg_log_line("    Query: %s", query);
}

/* fe_S */
static void
fe_S(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("FE: S");
}

/* fe_X */
static void
fe_X(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("FE: X");
}

/* fe_c */
static void
fe_c(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("FE: c");
}

/* fe_d */
static void
fe_d(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   for (int32_t j = 0; j < length - 4; j++)
   {
      pgprtdbg_read_byte(data + o);
      o += 1;
   }

   pgprtdbg_log_line("FE: d");
   pgprtdbg_log_line("    Size: %d", length - 4);
}

/* fe_f */
static void
fe_f(char** text)
{
   int o = 0;
   char* failure;

   o += 1;
   /* length */
   o += 4;

   failure = pgprtdbg_read_string(data + o);
   o += strlen(failure) + 1;

   pgprtdbg_log_line("FE: f");
   pgprtdbg_log_line("    Failure: %s", failure);
}

/* fe_p */
static void
fe_p(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("FE: p");
   pgprtdbg_log_mem(data + o, length - 4);
}

/* be_one */
static void
be_one(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: 1");
}

/* be_two */
static void
be_two(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: 2");
}

/* be_three */
static void
be_three(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: 3");
}

/* be_A */
static void
be_A(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: A");
}

/* be_C */
static void
be_C(char** text)
{
   int o = 0;
   char* str = NULL;

   o += 1;
   /* length */
   o += 4;

   str = pgprtdbg_read_string(data + o);
   o += strlen(str) + 1;
   
   pgprtdbg_log_line("BE: C");
   pgprtdbg_log_line("    Tag: %s", str);
}

/* be_D */
static void
be_D(char** text)
{
   int o = 0;
   int16_t number_of_columns;
   int32_t column_length;

   o += 1;
   /* length */
   o += 4;

   number_of_columns = pgprtdbg_read_int16(data + o);
   o += 2;

   pgprtdbg_log_line("BE: D");
   pgprtdbg_log_line("    Columns: %d", number_of_columns);
   for (int16_t i = 1; i <= number_of_columns; i++)
   {
      column_length = pgprtdbg_read_int32(data + o);
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
            buf[j] = pgprtdbg_read_byte(data + o);
            o += 1;
         }
      }
      else
      {
         pgprtdbg_log_line("    Data: NULL");
      }
   }
}

/* be_E */
static void
be_E(char** text)
{
   int o = 0;
   signed char type;
   char* str;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: E");

   while (o < length - 4)
   {
      type = pgprtdbg_read_byte(data + o);
      str = pgprtdbg_read_string(data + o + 1);

      pgprtdbg_log_line("    Code: %c", type);
      pgprtdbg_log_line("    Value: %s", str);

      o += (strlen(str) + 2);
   }
}

/* be_G */
static void
be_G(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: G");
}

/* be_H */
static void
be_H(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: H");
}

/* be_I */
static void
be_I(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: I");
}

/* be_K */
static void
be_K(char** text)
{
   int o = 0;
   int32_t process;
   int32_t secret;

   o += 1;
   /* length */
   o += 4;

   process = pgprtdbg_read_int32(data + o);
   o += 4;

   secret = pgprtdbg_read_int32(data + o);
   o += 4;

   pgprtdbg_log_line("BE: K");
   pgprtdbg_log_line("    Process: %d", process);
   pgprtdbg_log_line("    Secret: %d", secret);
}

/* be_N */
static void
be_N(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: N");
}

/* be_R */
static void
be_R(char** text)
{
   int o = 0;
   int32_t type;

   o += 1;
   /* length */
   o += 4;

   type = pgprtdbg_read_int32(data + o);
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
            char* mechanism = pgprtdbg_read_string(data + o);
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
}

/* be_S */
static void
be_S(char** text)
{
   int o = 0;
   char* name = NULL;
   char* value = NULL;

   o += 1;
   /* length */
   o += 4;

   name = pgprtdbg_read_string(data + o);
   o += strlen(name) + 1;

   value = pgprtdbg_read_string(data + o);
   o += strlen(value) + 1;

   pgprtdbg_log_line("BE: S");
   pgprtdbg_log_line("    Name: %s", name);
   pgprtdbg_log_line("    Value: %s", value);
}

/* be_T */
static void
be_T(char** text)
{
   int o = 0;
   int16_t number_of_fields;
   char* field_name = NULL;
   int32_t oid;
   int16_t attr;
   int32_t type_oid;
   int16_t type_length;
   int32_t type_modifier;
   int16_t format;

   o += 1;
   /* length */
   o += 4;

   number_of_fields = pgprtdbg_read_int16(data + o);
   o += 2;

   pgprtdbg_log_line("BE: T");
   pgprtdbg_log_line("    Number: %d", number_of_fields);
   for (int16_t i = 0; i < number_of_fields; i++)
   {
      field_name = pgprtdbg_read_string(data + o);
      o += strlen(field_name) + 1;

      oid = pgprtdbg_read_int32(data + o);
      o += 4;

      attr = pgprtdbg_read_int16(data + o);
      o += 2;

      type_oid = pgprtdbg_read_int32(data + o);
      o += 4;

      type_length = pgprtdbg_read_int16(data + o);
      o += 2;

      type_modifier = pgprtdbg_read_int32(data + o);
      o += 4;

      format = pgprtdbg_read_int16(data + o);
      o += 2;

      pgprtdbg_log_line("    Name: %s", field_name);
      pgprtdbg_log_line("    OID: %d", oid);
      pgprtdbg_log_line("    Attribute: %d", attr);
      pgprtdbg_log_line("    Type OID: %d", type_oid);
      pgprtdbg_log_line("    Type length: %d", type_length);
      pgprtdbg_log_line("    Type modifier: %d", type_modifier);
      pgprtdbg_log_line("    Format: %d", format);
   }
}

/* be_V */
static void
be_V(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: V");
}

/* be_W */
static void
be_W(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: W");
}

/* be_Z */
static void
be_Z(char** text)
{
   int o = 0;
   char buf[2];

   o += 1;
   /* length */
   o += 4;

   memset(&buf, 0, 2);

   buf[0] = pgprtdbg_read_byte(data + o);
   o += 1;
   
   pgprtdbg_log_line("BE: Z");
   pgprtdbg_log_line("    State: %s", buf);
}

/* be_c */
static void
be_c(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: c");
}

/* be_d */
static void
be_d(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   for (int32_t j = 0; j < length - 4; j++)
   {
      pgprtdbg_read_byte(data + o);
      o += 1;
   }

   pgprtdbg_log_line("BE: d");
   pgprtdbg_log_line("    Size: %d", length - 4);
}

/* be_n */
static void
be_n(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: n");
}

/* be_s */
static void
be_s(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: s");
}

/* be_t */
static void
be_t(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: t");
}

/* be_v */
static void
be_v(char** text)
{
   int o = 0;

   o += 1;
   /* length */
   o += 4;

   pgprtdbg_log_line("BE: v");
}
