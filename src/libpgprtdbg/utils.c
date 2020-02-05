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
#include <utils.h>

#define ZF_LOG_TAG "utils"
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

#ifndef EVBACKEND_LINUXAIO
#define EVBACKEND_LINUXAIO 0x00000040U
#endif

#ifndef EVBACKEND_IOURING
#define EVBACKEND_IOURING  0x00000080U
#endif

signed char
pgprtdbg_read_byte(void* data)
{
   return (signed char) *((char*)data);
}

int16_t
pgprtdbg_read_int16(void* data)
{
   unsigned char bytes[] = {*((unsigned char*)data),
                            *((unsigned char*)(data + 1))};

   int16_t res = (int16_t)((bytes[0] << 8)) |
                          ((bytes[1]     ));

   return res;
}

int32_t
pgprtdbg_read_int32(void* data)
{
   unsigned char bytes[] = {*((unsigned char*)data),
                            *((unsigned char*)(data + 1)),
                            *((unsigned char*)(data + 2)),
                            *((unsigned char*)(data + 3))};

   int32_t res = (int32_t)((bytes[0] << 24)) |
                          ((bytes[1] << 16)) |
                          ((bytes[2] <<  8)) |
                          ((bytes[3]      ));

   return res;
}

long
pgprtdbg_read_long(void* data)
{
   unsigned char bytes[] = {*((unsigned char*)data),
                            *((unsigned char*)(data + 1)),
                            *((unsigned char*)(data + 2)),
                            *((unsigned char*)(data + 3)),
                            *((unsigned char*)(data + 4)),
                            *((unsigned char*)(data + 5)),
                            *((unsigned char*)(data + 6)),
                            *((unsigned char*)(data + 7))};

   long res = (long)(((long)bytes[0]) << 56) |
                    (((long)bytes[1]) << 48) |
                    (((long)bytes[2]) << 40) |
                    (((long)bytes[3]) << 32) |
                    (((long)bytes[4]) << 24) |
                    (((long)bytes[5]) << 16) |
                    (((long)bytes[6]) <<  8) |
                    (((long)bytes[7])      );

   return res;
}

char*
pgprtdbg_read_string(void* data)
{
   return (char*)data;
}


void
pgprtdbg_write_byte(void* data, signed char b)
{
   *((char*)(data)) = b;
}

void
pgprtdbg_write_int32(void* data, int32_t i)
{
   char *ptr = (char*)&i;

   *((char*)(data + 3)) = *ptr;
   ptr++;
   *((char*)(data + 2)) = *ptr;
   ptr++;
   *((char*)(data + 1)) = *ptr;
   ptr++;
   *((char*)(data)) = *ptr;
}

void
pgprtdbg_write_long(void* data, long l)
{
   char *ptr = (char*)&l;

   *((char*)(data + 7)) = *ptr;
   ptr++;
   *((char*)(data + 6)) = *ptr;
   ptr++;
   *((char*)(data + 5)) = *ptr;
   ptr++;
   *((char*)(data + 4)) = *ptr;
   ptr++;
   *((char*)(data + 3)) = *ptr;
   ptr++;
   *((char*)(data + 2)) = *ptr;
   ptr++;
   *((char*)(data + 1)) = *ptr;
   ptr++;
   *((char*)(data)) = *ptr;
}

void
pgprtdbg_write_string(void* data, char* s)
{
   memcpy(data, s, strlen(s));
}

void
pgprtdbg_libev_engines()
{
   unsigned int engines = ev_supported_backends();

   if (engines & EVBACKEND_SELECT)
   {
      ZF_LOGD("libev available: select");
   }
   if (engines & EVBACKEND_POLL)
   {
      ZF_LOGD("libev available: poll");
   }
   if (engines & EVBACKEND_EPOLL)
   {
      ZF_LOGD("libev available: epoll");
   }
   if (engines & EVBACKEND_LINUXAIO)
   {
      ZF_LOGD("libev available: linuxaio");
   }
   if (engines & EVBACKEND_IOURING)
   {
      ZF_LOGD("libev available: iouring");
   }
   if (engines & EVBACKEND_KQUEUE)
   {
      ZF_LOGD("libev available: kqueue");
   }
   if (engines & EVBACKEND_DEVPOLL)
   {
      ZF_LOGD("libev available: devpoll");
   }
   if (engines & EVBACKEND_PORT)
   {
      ZF_LOGD("libev available: port");
   }
}

unsigned int
pgprtdbg_libev(char* engine)
{
   unsigned int engines = ev_supported_backends();

   if (engine)
   {
      if (!strcmp("select", engine))
      {
         if (engines & EVBACKEND_SELECT)
         {
            return EVBACKEND_SELECT;
         }
         else
         {
            ZF_LOGW("libev not available: select");
         }
      }
      else if (!strcmp("poll", engine))
      {
         if (engines & EVBACKEND_POLL)
         {
            return EVBACKEND_POLL;
         }
         else
         {
            ZF_LOGW("libev not available: poll");
         }
      }
      else if (!strcmp("epoll", engine))
      {
         if (engines & EVBACKEND_EPOLL)
         {
            return EVBACKEND_EPOLL;
         }
         else
         {
            ZF_LOGW("libev not available: epoll");
         }
      }
      else if (!strcmp("linuxaio", engine))
      {
         if (engines & EVBACKEND_LINUXAIO)
         {
            return EVBACKEND_LINUXAIO;
         }
         else
         {
            ZF_LOGW("libev not available: linuxaio");
         }
      }
      else if (!strcmp("iouring", engine))
      {
         if (engines & EVBACKEND_IOURING)
         {
            return EVBACKEND_IOURING;
         }
         else
         {
            ZF_LOGW("libev not available: iouring");
         }
      }
      else if (!strcmp("devpoll", engine))
      {
         if (engines & EVBACKEND_DEVPOLL)
         {
            return EVBACKEND_DEVPOLL;
         }
         else
         {
            ZF_LOGW("libev not available: devpoll");
         }
      }
      else if (!strcmp("port", engine))
      {
         if (engines & EVBACKEND_PORT)
         {
            return EVBACKEND_PORT;
         }
         else
         {
            ZF_LOGW("libev not available: port");
         }
      }
      else if (!strcmp("auto", engine) || !strcmp("", engine))
      {
         return EVFLAG_AUTO;
      }
      else
      {
         ZF_LOGW("libev unknown option: %s", engine);
      }
   }

   return EVFLAG_AUTO;
}

char*
pgprtdbg_libev_engine(unsigned int val)
{
   switch (val)
   {
      case EVBACKEND_SELECT:
         return "select";
      case EVBACKEND_POLL:
         return "poll";
      case EVBACKEND_EPOLL:
         return "epoll";
      case EVBACKEND_LINUXAIO:
         return "linuxaio";
      case EVBACKEND_IOURING:
         return "iouring";
      case EVBACKEND_KQUEUE:
         return "kqueue";
      case EVBACKEND_DEVPOLL:
         return "devpoll";
      case EVBACKEND_PORT:
         return "port";
   }

   return "Unknown";
}
