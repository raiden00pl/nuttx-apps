/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_threads.h
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

#ifndef __APPS_NETUTILS_S2OPC_PORT_NUTTX_P_SOPC_THREADS_H
#define __APPS_NETUTILS_S2OPC_PORT_NUTTX_P_SOPC_THREADS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <pthread.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct SOPC_Mutex_Impl
{
  pthread_mutex_t mutex;
};

struct SOPC_Condition_Impl
{
  pthread_cond_t cond;
};

struct SOPC_Thread_Impl
{
  pthread_t thread;
};

#endif /* __APPS_NETUTILS_S2OPC_PORT_NUTTX_P_SOPC_THREADS_H */
