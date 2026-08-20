/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/sopc_nuttx_config.h
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

#ifndef __APPS_NETUTILS_S2OPC_PORT_NUTTX_SOPC_NUTTX_CONFIG_H
#define __APPS_NETUTILS_S2OPC_PORT_NUTTX_SOPC_NUTTX_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* S2OPC uses Boolean constants in preprocessor expressions.  NuttX
 * deliberately defines false and true as typed expressions.  Preserve the
 * standard Boolean type, but provide untyped values to the S2OPC sources.
 */

#undef false
#undef true
#define false 0
#define true 1

#ifndef IPV6_ADD_MEMBERSHIP
#  define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#  define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

#define SOPC_MAX_NB_ELEMENTS_ASYNC_QUEUE_WARNING_ONLY 1
#define SOPC_HAS_FILESYSTEM 1
#define SOPC_LISTENER_LISTEN_ALL_INTERFACES 1

#ifdef CONFIG_S2OPC_NODE_MANAGEMENT
#  define S2OPC_NODE_MANAGEMENT 1
#endif

#ifdef CONFIG_S2OPC_NODE_ADD_OPTIONAL
#  define S2OPC_NODE_ADD_OPTIONAL 1
#endif

#ifdef CONFIG_S2OPC_HISTORY_READ
#  define S2OPC_EXTERNAL_HISTORY_RAW_READ_SERVICE 1
#endif

#ifdef CONFIG_S2OPC_EVENT_MANAGEMENT
#  define S2OPC_EVENT_MANAGEMENT 1
#endif

#ifdef CONFIG_S2OPC_AUDITING
#  define S2OPC_HAS_AUDITING 1
#endif

#endif /* __APPS_NETUTILS_S2OPC_PORT_NUTTX_SOPC_NUTTX_CONFIG_H */
