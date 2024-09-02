/****************************************************************************
 * apps/system/mcumgr/porting/img_mgmt_log.c
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

#include "img_mgmt/img_mgmt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMG_MGMT_LOG_EV_UPSTART   "upstart"
#define IMG_MGMT_LOG_EV_UPDONE    "updone"
#define IMG_MGMT_LOG_EV_PEND      "pend"
#define IMG_MGMT_LOG_EV_CONF      "conf"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int img_mgmt_impl_log_upload_start(int status)
{
  return 0;
}

int img_mgmt_impl_log_upload_done(int status, const uint8_t *hash)
{
  return 0;
}

int img_mgmt_impl_log_pending(int status, const uint8_t *hash)
{
  return 0;
}

int img_mgmt_impl_log_confirm(int status, const uint8_t *hash)
{
  return 0;
}
