/*
 * SPDX-FileCopyrightText: Copyright 2022-2023 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#if !defined(SEMIHOSTING)

#include "uart_stdout.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define UNUSED(x)    (void)(x)

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6100100)
/* Arm compiler re-targeting */

#include <rt_misc.h>
#include <rt_sys.h>


/* Standard IO device handles. */
#define STDIN  0x8001
#define STDOUT 0x8002
#define STDERR 0x8003

#define RETARGET(fun) _sys##fun

#if __ARMCLIB_VERSION >= 6190004
#define TMPNAM_FUNCTION RETARGET(_tmpnam2)
#else
#define TMPNAM_FUNCTION RETARGET(_tmpnam)
#endif

#else
/* GNU compiler re-targeting */

/*
* This type is used by the _ I/O functions to denote an open
* file.
*/
typedef int FILEHANDLE;

/*
* Open a file. May return -1 if the file failed to open.
*/
extern FILEHANDLE _open(const char * /*name*/, int /*openmode*/);

/* Standard IO device handles. */
#define STDIN  0x00
#define STDOUT 0x01
#define STDERR 0x02

#define RETARGET(fun) fun

#define TMPNAM_FUNCTION RETARGET(_tmpnam)

#endif

/* Standard IO device name defines. */
const char __stdin_name[] __attribute__((aligned(4)))  = "STDIN";
const char __stdout_name[] __attribute__((aligned(4))) = "STDOUT";
const char __stderr_name[] __attribute__((aligned(4))) = "STDERR";

void _ttywrch(int ch) {
   (void)fputc(ch, stdout);
}

FILEHANDLE RETARGET(_open)(const char *name, int openmode)
{
   UNUSED(openmode);

   if (strcmp(name, __stdin_name) == 0) {
       return (STDIN);
   }

   if (strcmp(name, __stdout_name) == 0) {
       return (STDOUT);
   }

   if (strcmp(name, __stderr_name) == 0) {
       return (STDERR);
   }

   return -1;
}

int RETARGET(_write)(FILEHANDLE fh, const unsigned char *buf, unsigned int len, int mode)
{
   UNUSED(mode);

   switch (fh) {
   case STDOUT:
   case STDERR: {
       int c;

       while (len-- > 0) {
           c = fputc(*buf++, stdout);
           if (c == EOF) {
               return EOF;
           }
       }

       return 0;
   }
   default:
       return EOF;
   }
}

int RETARGET(_read)(FILEHANDLE fh, unsigned char *buf, unsigned int len, int mode)
{
   UNUSED(mode);

   switch (fh) {
   case STDIN: {
       int c;

       while (len-- > 0) {
           c = fgetc(stdin);
           if (c == EOF) {
               return EOF;
           }

           *buf++ = (unsigned char)c;
       }

       return 0;
   }
   default:
       return EOF;
   }
}

int RETARGET(_istty)(FILEHANDLE fh)
{
   switch (fh) {
   case STDIN:
   case STDOUT:
   case STDERR:
       return 1;
   default:
       return 0;
   }
}

int RETARGET(_close)(FILEHANDLE fh)
{
   if (RETARGET(_istty(fh))) {
       return 0;
   }

   return -1;
}

int RETARGET(_seek)(FILEHANDLE fh, long pos)
{
   UNUSED(fh);
   UNUSED(pos);

   return -1;
}

int RETARGET(_ensure)(FILEHANDLE fh)
{
   UNUSED(fh);

   return -1;
}

long RETARGET(_flen)(FILEHANDLE fh)
{
   if (RETARGET(_istty)(fh)) {
       return 0;
   }

   return -1;
}

int TMPNAM_FUNCTION(char *name, int sig, unsigned int maxlen)
{
   UNUSED(name);
   UNUSED(sig);
   UNUSED(maxlen);

   return 1;
}

char *RETARGET(_command_string)(char *cmd, int len)
{
   UNUSED(len);

   return cmd;
}

void RETARGET(_exit)(int return_code)
{
   UartEndSimulation(return_code);
}

int system(const char *cmd)
{
   UNUSED(cmd);

   return 0;
}

time_t time(time_t *timer)
{
   time_t current;

   current = 0; // To Do !! No RTC implemented

   if (timer != NULL) {
       *timer = current;
   }

   return current;
}

void _clock_init(void) {}

clock_t clock(void)
{
   return (clock_t)-1;
}

int remove(const char *arg) {
   UNUSED(arg);

   return 0;
}

int rename(const char *oldn, const char *newn)
{
   UNUSED(oldn);
   UNUSED(newn);

   return 0;
}

int fputc(int ch, FILE *f)
{
   UNUSED(f);

   return UartPutc(ch);
}

int fgetc(FILE *f)
{
   UNUSED(f);

   return UartPutc(UartGetc());
}

#ifndef ferror

/* arm-none-eabi-gcc with newlib uses a define for ferror */
int ferror(FILE *f)
{
   UNUSED(f);

   return EOF;
}

#endif /* #ifndef ferror */

#endif /* !SEMIHOSTING */

