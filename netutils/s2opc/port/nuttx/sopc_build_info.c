/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/sopc_build_info.c
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

#include "sopc_common_build_info.h"
#include "sopc_pubsub_build_info.h"
#include "sopc_toolkit_build_info.h"
#include "sopc_version.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

const SOPC_Build_Info sopc_common_build_info =
{
  .buildVersion = SOPC_TOOLKIT_VERSION,
  .buildSrcCommit = "S2OPC_Toolkit_" CONFIG_S2OPC_VERSION,
  .buildDockerId = "",
  .buildBuildDate = ""
};

const SOPC_Build_Info sopc_client_server_build_info =
{
  .buildVersion = SOPC_TOOLKIT_VERSION,
  .buildSrcCommit = "S2OPC_Toolkit_" CONFIG_S2OPC_VERSION,
  .buildDockerId = "",
  .buildBuildDate = ""
};

const SOPC_Build_Info sopc_pubsub_build_info =
{
  .buildVersion = SOPC_TOOLKIT_VERSION,
  .buildSrcCommit = "S2OPC_Toolkit_" CONFIG_S2OPC_VERSION,
  .buildDockerId = "",
  .buildBuildDate = ""
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_Common_GetBuildInfo
 ****************************************************************************/

SOPC_Build_Info SOPC_Common_GetBuildInfo(void)
{
  return sopc_common_build_info;
}

/****************************************************************************
 * Name: SOPC_ClientServer_GetBuildInfo
 ****************************************************************************/

SOPC_Build_Info SOPC_ClientServer_GetBuildInfo(void)
{
  return sopc_client_server_build_info;
}

/****************************************************************************
 * Name: SOPC_PubSub_GetBuildInfo
 ****************************************************************************/

SOPC_Build_Info SOPC_PubSub_GetBuildInfo(void)
{
  return sopc_pubsub_build_info;
}
