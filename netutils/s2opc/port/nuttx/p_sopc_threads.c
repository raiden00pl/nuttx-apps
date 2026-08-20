/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_threads.c
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
#include <sched.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "p_sopc_threads.h"
#include "sopc_mem_alloc.h"
#include "sopc_mutexes.h"
#include "sopc_threads.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define S2OPC_NSEC_PER_SEC  1000000000l
#define S2OPC_NSEC_PER_MSEC 1000000l

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_thread_start
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_thread_start(FAR pthread_t *thread,
                   FAR void *(*entry)(FAR void *), FAR void *arg,
                   FAR const char *name, int priority, int affinity)
{
  struct sched_param param;
  pthread_attr_t attr;
#ifdef CONFIG_SMP
  cpu_set_t cpuset;
#endif
  char taskname[CONFIG_TASK_NAME_SIZE + 1];
  int ret;

  ret = pthread_attr_init(&attr);
  if (ret != 0)
    {
      return SOPC_STATUS_NOK;
    }

  ret = pthread_attr_setstacksize(&attr, CONFIG_S2OPC_THREAD_STACKSIZE);
  if (ret == 0)
    {
      param.sched_priority = priority > 0 ? priority :
                             SCHED_PRIORITY_DEFAULT;
      ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
      if (ret == 0)
        {
          ret = pthread_attr_setschedpolicy(&attr, priority > 0 ?
                                            SCHED_FIFO : SCHED_RR);
        }

      if (ret == 0)
        {
          ret = pthread_attr_setschedparam(&attr, &param);
        }
    }

#ifdef CONFIG_SMP
  if (ret == 0 && affinity >= 0)
    {
      CPU_ZERO(&cpuset);
      CPU_SET((size_t)affinity, &cpuset);
      ret = pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
    }
#endif

  if (ret == 0)
    {
      ret = pthread_create(thread, &attr, entry, arg);
    }

  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      return SOPC_STATUS_NOK;
    }

  if (name != NULL)
    {
      strlcpy(taskname, name, sizeof(taskname));
      pthread_setname_np(*thread, taskname);
    }

  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: s2opc_thread_create
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_thread_create(FAR SOPC_Thread *thread,
                    FAR void *(*entry)(FAR void *), FAR void *arg,
                    FAR const char *name, int priority, int affinity)
{
  FAR SOPC_Thread_Impl *implementation;
  SOPC_ReturnStatus status;

  if (thread == NULL || entry == NULL || priority < 0 ||
      priority > SCHED_PRIORITY_MAX || affinity >= CONFIG_SMP_NCPUS)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *thread = SOPC_INVALID_THREAD;
  implementation = SOPC_Calloc(1, sizeof(*implementation));
  if (implementation == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  status = s2opc_thread_start(&implementation->thread, entry, arg, name,
                              priority, affinity);
  if (status == SOPC_STATUS_OK)
    {
      *thread = implementation;
    }
  else
    {
      SOPC_Free(implementation);
    }

  return status;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_Condition_Init
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Condition_Init(FAR SOPC_Condition *cond)
{
  FAR SOPC_Condition_Impl *implementation;

  if (cond == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *cond = SOPC_INVALID_COND;
  implementation = SOPC_Calloc(1, sizeof(*implementation));
  if (implementation == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  if (pthread_cond_init(&implementation->cond, NULL) != 0)
    {
      SOPC_Free(implementation);
      return SOPC_STATUS_NOK;
    }

  *cond = implementation;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Condition_Clear
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Condition_Clear(FAR SOPC_Condition *cond)
{
  FAR SOPC_Condition_Impl *implementation;

  if (cond == NULL || *cond == SOPC_INVALID_COND)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  implementation = *cond;
  if (pthread_cond_destroy(&implementation->cond) != 0)
    {
      return SOPC_STATUS_NOK;
    }

  SOPC_Free(implementation);
  *cond = SOPC_INVALID_COND;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Condition_SignalAll
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Condition_SignalAll(FAR SOPC_Condition *cond)
{
  FAR SOPC_Condition_Impl *implementation;

  if (cond == NULL || *cond == SOPC_INVALID_COND)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  implementation = *cond;
  return pthread_cond_broadcast(&implementation->cond) == 0 ?
         SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Mutex_Initialization
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Mutex_Initialization(FAR SOPC_Mutex *mutex)
{
  FAR SOPC_Mutex_Impl *implementation;
  pthread_mutexattr_t attr;
  int ret;

  if (mutex == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *mutex = SOPC_INVALID_MUTEX;
  ret = pthread_mutexattr_init(&attr);
  if (ret != 0)
    {
      return SOPC_STATUS_NOK;
    }

  ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  if (ret == 0)
    {
      implementation = SOPC_Calloc(1, sizeof(*implementation));
      if (implementation == NULL)
        {
          ret = ENOMEM;
        }
      else if (pthread_mutex_init(&implementation->mutex, &attr) != 0)
        {
          SOPC_Free(implementation);
          ret = EINVAL;
        }
      else
        {
          *mutex = implementation;
        }
    }

  pthread_mutexattr_destroy(&attr);
  if (ret == ENOMEM)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  return ret == 0 ? SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Mutex_Clear
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Mutex_Clear(FAR SOPC_Mutex *mutex)
{
  FAR SOPC_Mutex_Impl *implementation;

  if (mutex == NULL || *mutex == SOPC_INVALID_MUTEX)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  implementation = *mutex;
  if (pthread_mutex_destroy(&implementation->mutex) != 0)
    {
      return SOPC_STATUS_NOK;
    }

  SOPC_Free(implementation);
  *mutex = SOPC_INVALID_MUTEX;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Mutex_Lock
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Mutex_Lock(FAR SOPC_Mutex *mutex)
{
  FAR SOPC_Mutex_Impl *implementation;

  if (mutex == NULL || *mutex == SOPC_INVALID_MUTEX)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  implementation = *mutex;
  return pthread_mutex_lock(&implementation->mutex) == 0 ?
         SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Mutex_Unlock
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Mutex_Unlock(FAR SOPC_Mutex *mutex)
{
  FAR SOPC_Mutex_Impl *implementation;

  if (mutex == NULL || *mutex == SOPC_INVALID_MUTEX)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  implementation = *mutex;
  return pthread_mutex_unlock(&implementation->mutex) == 0 ?
         SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Mutex_UnlockAndWaitCond
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Mutex_UnlockAndWaitCond(FAR SOPC_Condition *cond,
                             FAR SOPC_Mutex *mutex)
{
  FAR SOPC_Condition_Impl *condition;
  FAR SOPC_Mutex_Impl *implementation;

  if (cond == NULL || *cond == SOPC_INVALID_COND || mutex == NULL ||
      *mutex == SOPC_INVALID_MUTEX)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  condition = *cond;
  implementation = *mutex;
  return pthread_cond_wait(&condition->cond, &implementation->mutex) == 0 ?
         SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Mutex_UnlockAndTimedWaitCond
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Mutex_UnlockAndTimedWaitCond(FAR SOPC_Condition *cond,
                                  FAR SOPC_Mutex *mutex,
                                  uint32_t milliseconds)
{
  FAR SOPC_Condition_Impl *condition;
  FAR SOPC_Mutex_Impl *implementation;
  struct timespec abstime;
  uint64_t nanoseconds;
  int ret;

  if (cond == NULL || *cond == SOPC_INVALID_COND || mutex == NULL ||
      *mutex == SOPC_INVALID_MUTEX || milliseconds == 0)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  if (clock_gettime(CLOCK_REALTIME, &abstime) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  abstime.tv_sec += milliseconds / 1000;
  nanoseconds = abstime.tv_nsec +
                (uint64_t)(milliseconds % 1000) * S2OPC_NSEC_PER_MSEC;
  abstime.tv_sec += nanoseconds / S2OPC_NSEC_PER_SEC;
  abstime.tv_nsec = nanoseconds % S2OPC_NSEC_PER_SEC;

  condition = *cond;
  implementation = *mutex;
  ret = pthread_cond_timedwait(&condition->cond, &implementation->mutex,
                               &abstime);
  if (ret == ETIMEDOUT)
    {
      return SOPC_STATUS_TIMEOUT;
    }

  return ret == 0 ? SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Thread_Create
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Thread_Create(FAR SOPC_Thread *thread,
                                     FAR void *(*entry)(FAR void *),
                                     FAR void *arg, FAR const char *name)
{
  return s2opc_thread_create(thread, entry, arg, name, 0, -1);
}

/****************************************************************************
 * Name: SOPC_Thread_CreatePrioritized
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Thread_CreatePrioritized(FAR SOPC_Thread *thread,
                              FAR void *(*entry)(FAR void *), FAR void *arg,
                              int priority, int affinity,
                              FAR const char *name)
{
  return s2opc_thread_create(thread, entry, arg, name, priority, affinity);
}

/****************************************************************************
 * Name: SOPC_Thread_Join
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Thread_Join(FAR SOPC_Thread *thread)
{
  FAR SOPC_Thread_Impl *implementation;

  if (thread == NULL || *thread == SOPC_INVALID_THREAD)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  implementation = *thread;
  if (pthread_join(implementation->thread, NULL) != 0)
    {
      return SOPC_STATUS_NOK;
    }

  SOPC_Free(implementation);
  *thread = SOPC_INVALID_THREAD;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Sleep
 ****************************************************************************/

void SOPC_Sleep(unsigned int milliseconds)
{
  struct timespec delay;

  delay.tv_sec = milliseconds / 1000;
  delay.tv_nsec = (milliseconds % 1000) * S2OPC_NSEC_PER_MSEC;
  while (nanosleep(&delay, &delay) < 0 && errno == EINTR)
    {
    }
}
