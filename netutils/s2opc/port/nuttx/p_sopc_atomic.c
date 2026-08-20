/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_atomic.c
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

#include "sopc_atomic.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_Atomic_Int_Get
 ****************************************************************************/

int32_t SOPC_Atomic_Int_Get(FAR int32_t *atomic)
{
  return __atomic_load_n(atomic, __ATOMIC_SEQ_CST);
}

/****************************************************************************
 * Name: SOPC_Atomic_Int_Set
 ****************************************************************************/

void SOPC_Atomic_Int_Set(FAR int32_t *atomic, int32_t value)
{
  __atomic_store_n(atomic, value, __ATOMIC_SEQ_CST);
}

/****************************************************************************
 * Name: SOPC_Atomic_Int_Add
 ****************************************************************************/

int32_t SOPC_Atomic_Int_Add(FAR int32_t *atomic, int32_t value)
{
  return __atomic_fetch_add(atomic, value, __ATOMIC_SEQ_CST);
}

/****************************************************************************
 * Name: SOPC_Atomic_Ptr_Get
 ****************************************************************************/

FAR void *SOPC_Atomic_Ptr_Get(FAR void **atomic)
{
  return __atomic_load_n(atomic, __ATOMIC_SEQ_CST);
}

/****************************************************************************
 * Name: SOPC_Atomic_Ptr_Set
 ****************************************************************************/

void SOPC_Atomic_Ptr_Set(FAR void **atomic, FAR void *value)
{
  __atomic_store_n(atomic, value, __ATOMIC_SEQ_CST);
}
