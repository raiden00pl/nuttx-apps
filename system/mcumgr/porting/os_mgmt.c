/****************************************************************************
 * apps/system/mcumgr/porting/os_mgmt.c
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
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "mgmt/mgmt.h"
#include "os_mgmt/os_mgmt_impl.h"
#include "os_mgmt/os_mgmt.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int os_mgmt_impl_task_info(int idx, struct os_mgmt_task_info *out_info)
{
  return 0;
}

int os_mgmt_impl_reset(unsigned int delay_ms)
{
  return 0;
}
