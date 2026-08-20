/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_random.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
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

#include <sys/random.h>

#include <errno.h>
#include <stdint.h>

#include "sopc_buffer.h"
#include "sopc_random.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_GetRandom
 ****************************************************************************/

SOPC_ReturnStatus SOPC_GetRandom(FAR SOPC_Buffer *buffer, uint32_t length)
{
  uint8_t random_data[32];
  SOPC_ReturnStatus status;
  uint32_t remaining;

  if (buffer == NULL || length == 0)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  remaining = length;
  while (remaining > 0)
    {
      uint32_t chunk = remaining > sizeof(random_data) ?
                       sizeof(random_data) : remaining;
      ssize_t nread = getrandom(random_data, chunk, 0);

      if (nread < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return errno == EAGAIN ? SOPC_STATUS_WOULD_BLOCK : SOPC_STATUS_NOK;
        }

      if (nread == 0)
        {
          return SOPC_STATUS_NOK;
        }

      status = SOPC_Buffer_Write(buffer, random_data, nread);
      if (status != SOPC_STATUS_OK)
        {
          return status;
        }

      remaining -= nread;
    }

  return SOPC_STATUS_OK;
}
