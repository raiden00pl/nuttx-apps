/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_askpass.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to Systerel under one or more contributor license agreements.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.  Systerel licenses this file to you under
 * the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "sopc_askpass.h"
#include "sopc_mem_alloc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define S2OPC_PASSWORD_EXTRA 3

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_AskPass_CustomPromptFromTerminal
 ****************************************************************************/

bool SOPC_AskPass_CustomPromptFromTerminal(FAR const char *prompt,
                                           FAR char **password)
{
  struct termios original;
  struct termios noecho;
  FAR char *buffer;
  FAR char *result;
  size_t buffer_size;
  size_t length;
  int fd;
  int ret;

  if (prompt == NULL || password == NULL)
    {
      return false;
    }

  *password = NULL;
  fd = fileno(stdin);
  if (fd < 0)
    {
      return false;
    }

  buffer_size = SOPC_PASSWORD_MAX_LENGTH + S2OPC_PASSWORD_EXTRA;
  buffer = SOPC_Calloc(buffer_size, sizeof(char));
  if (buffer == NULL)
    {
      return false;
    }

  ret = tcgetattr(fd, &original);
  if (ret == 0)
    {
      noecho = original;
      noecho.c_lflag &= (tcflag_t)~(ECHO | ECHOE | ECHOK | ECHONL);
      ret = tcsetattr(fd, TCSAFLUSH, &noecho);
    }

  result = NULL;
  if (ret == 0)
    {
      fputs(prompt, stdout);
      fflush(stdout);
      result = fgets(buffer, buffer_size, stdin);
      tcsetattr(fd, TCSAFLUSH, &original);
      fputs("\n", stdout);
      fflush(stdout);
    }

  if (result != NULL)
    {
      length = strlen(buffer);
      if (length > 0 && buffer[length - 1] == '\n')
        {
          buffer[length - 1] = '\0';
          *password = buffer;
          return true;
        }
    }

  explicit_bzero(buffer, buffer_size);
  SOPC_Free(buffer);
  return false;
}
