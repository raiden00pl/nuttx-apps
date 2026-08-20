/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_date_time.c
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

#include <limits.h>
#include <time.h>

#include "sopc_date_time.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_Time_GetCurrentTimeUTC
 ****************************************************************************/

SOPC_DateTime SOPC_Time_GetCurrentTimeUTC(void)
{
  struct timespec current;
  int64_t date_time;
  int64_t units_100ns;

  if (clock_gettime(CLOCK_REALTIME, &current) < 0)
    {
      return 0;
    }

  date_time = 0;
  units_100ns = current.tv_nsec / 100;
  if (SOPC_Time_FromUnixTime(current.tv_sec, &date_time) != SOPC_STATUS_OK ||
      INT64_MAX - date_time < units_100ns)
    {
      return INT64_MAX;
    }

  return date_time + units_100ns;
}

/****************************************************************************
 * Name: SOPC_Time_Breakdown_Local
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Time_Breakdown_Local(SOPC_Unix_Time time,
                                            FAR struct tm *result)
{
  return localtime_r(&time, result) == NULL ? SOPC_STATUS_NOK :
         SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Time_Breakdown_UTC
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Time_Breakdown_UTC(SOPC_Unix_Time time,
                                          FAR struct tm *result)
{
  return gmtime_r(&time, result) == NULL ? SOPC_STATUS_NOK :
         SOPC_STATUS_OK;
}
