/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_time_reference.c
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

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "sopc_assert.h"
#include "sopc_date_time.h"
#include "sopc_logger.h"
#include "sopc_mem_alloc.h"
#include "sopc_time_reference.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define S2OPC_NSEC_PER_SEC  1000000000l
#define S2OPC_NSEC_PER_MSEC 1000000l
#define S2OPC_NSEC_PER_USEC 1000l
#define S2OPC_USEC_PER_SEC  1000000l

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct SOPC_HighRes_TimeReference
{
  struct timespec time;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_highres_add
 ****************************************************************************/

static void
s2opc_highres_add(FAR SOPC_HighRes_TimeReference *reference,
                  uint64_t duration_us)
{
  reference->time.tv_sec += duration_us / S2OPC_USEC_PER_SEC;
  reference->time.tv_nsec +=
    (duration_us % S2OPC_USEC_PER_SEC) * S2OPC_NSEC_PER_USEC;

  if (reference->time.tv_nsec >= S2OPC_NSEC_PER_SEC)
    {
      reference->time.tv_sec++;
      reference->time.tv_nsec -= S2OPC_NSEC_PER_SEC;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_TimeReference_GetCurrent
 ****************************************************************************/

SOPC_TimeReference SOPC_TimeReference_GetCurrent(void)
{
  struct timespec current;
  uint64_t milliseconds;

  if (clock_gettime(CLOCK_MONOTONIC, &current) < 0 || current.tv_sec < 0 ||
      current.tv_nsec < 0 || current.tv_nsec >= S2OPC_NSEC_PER_SEC)
    {
      return UINT64_MAX;
    }

  if ((uint64_t)current.tv_sec >
      (UINT64_MAX - current.tv_nsec / S2OPC_NSEC_PER_MSEC) / 1000)
    {
      return UINT64_MAX;
    }

  milliseconds = (uint64_t)current.tv_sec * 1000;
  return milliseconds + current.tv_nsec / S2OPC_NSEC_PER_MSEC;
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_Create
 ****************************************************************************/

FAR SOPC_HighRes_TimeReference *SOPC_HighRes_TimeReference_Create(void)
{
  FAR SOPC_HighRes_TimeReference *reference;

  reference = SOPC_Calloc(1, sizeof(*reference));
  if (reference != NULL)
    {
      SOPC_HighRes_TimeReference_GetTime(reference);
    }

  return reference;
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_Delete
 ****************************************************************************/

void SOPC_HighRes_TimeReference_Delete(
  FAR SOPC_HighRes_TimeReference **reference)
{
  if (reference != NULL)
    {
      SOPC_Free(*reference);
      *reference = NULL;
    }
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_Copy
 ****************************************************************************/

void SOPC_HighRes_TimeReference_Copy(
  FAR SOPC_HighRes_TimeReference *destination,
  FAR const SOPC_HighRes_TimeReference *source)
{
  if (destination != NULL && source != NULL)
    {
      *destination = *source;
    }
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_GetTime
 ****************************************************************************/

void SOPC_HighRes_TimeReference_GetTime(
  FAR SOPC_HighRes_TimeReference *reference)
{
  int ret;

  if (reference != NULL)
    {
      ret = clock_gettime(CLOCK_MONOTONIC, &reference->time);
      SOPC_ASSERT(ret == 0);
    }
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_AddSynchedDuration
 ****************************************************************************/

void SOPC_HighRes_TimeReference_AddSynchedDuration(
  FAR SOPC_HighRes_TimeReference *reference, uint64_t duration_us,
  int32_t offset_us)
{
  uint64_t window_offset;
  uint64_t current_remainder;
  uint64_t increment;

  SOPC_ASSERT(reference != NULL);
  increment = duration_us;
  if (offset_us >= 0)
    {
      SOPC_ASSERT(duration_us > 0);
      current_remainder = SOPC_Time_GetCurrentTimeUTC() / 10;
      current_remainder %= duration_us;
      window_offset = (duration_us + current_remainder -
                       (uint32_t)offset_us) % duration_us;
      increment -= window_offset;

      /* A wakeup in the last fifth of a period belongs to the next period.
       * This absorbs small differences between the realtime and monotonic
       * clocks.
       */

      if (increment < duration_us / 5)
        {
          increment += duration_us;
        }
    }

  s2opc_highres_add(reference, increment);
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_IsExpired
 ****************************************************************************/

bool SOPC_HighRes_TimeReference_IsExpired(
  FAR const SOPC_HighRes_TimeReference *reference,
  FAR const SOPC_HighRes_TimeReference *now)
{
  SOPC_HighRes_TimeReference current;

  SOPC_ASSERT(reference != NULL);
  if (now == NULL)
    {
      SOPC_HighRes_TimeReference_GetTime(&current);
      now = &current;
    }

  return reference->time.tv_sec < now->time.tv_sec ||
         (reference->time.tv_sec == now->time.tv_sec &&
          reference->time.tv_nsec <= now->time.tv_nsec);
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_DeltaUs
 ****************************************************************************/

int64_t SOPC_HighRes_TimeReference_DeltaUs(
  FAR const SOPC_HighRes_TimeReference *reference,
  FAR const SOPC_HighRes_TimeReference *time)
{
  SOPC_HighRes_TimeReference current;
  int64_t delta_seconds;
  int64_t delta_nanoseconds;

  SOPC_ASSERT(reference != NULL);
  if (time == NULL)
    {
      SOPC_HighRes_TimeReference_GetTime(&current);
      time = &current;
    }

  delta_seconds = time->time.tv_sec - reference->time.tv_sec;
  delta_nanoseconds = time->time.tv_nsec - reference->time.tv_nsec;
  return delta_seconds * S2OPC_USEC_PER_SEC +
         delta_nanoseconds / S2OPC_NSEC_PER_USEC;
}

/****************************************************************************
 * Name: SOPC_HighRes_TimeReference_SleepUntil
 ****************************************************************************/

void SOPC_HighRes_TimeReference_SleepUntil(
  FAR const SOPC_HighRes_TimeReference *date)
{
  struct timespec current;
  struct timespec delay;
  static bool warned;
  int ret;

  SOPC_ASSERT(date != NULL);
  for (; ; )
    {
      ret = clock_gettime(CLOCK_MONOTONIC, &current);
      if (ret < 0)
        {
          break;
        }

      if (date->time.tv_sec < current.tv_sec ||
          (date->time.tv_sec == current.tv_sec &&
           date->time.tv_nsec <= current.tv_nsec))
        {
          return;
        }

      delay.tv_sec = date->time.tv_sec - current.tv_sec;
      delay.tv_nsec = date->time.tv_nsec - current.tv_nsec;
      if (delay.tv_nsec < 0)
        {
          delay.tv_sec--;
          delay.tv_nsec += S2OPC_NSEC_PER_SEC;
        }

      ret = nanosleep(&delay, NULL);
      if (ret == 0)
        {
          return;
        }

      if (errno != EINTR)
        {
          break;
        }
    }

  if (!warned)
    {
      warned = true;
      SOPC_Logger_TraceError(SOPC_LOG_MODULE_COMMON,
                             "high-resolution sleep failed: %d (%s)",
                             errno, strerror(errno));
    }
}
