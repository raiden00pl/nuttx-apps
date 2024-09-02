/****************************************************************************
 * apps/system/mcumgr/config/stat_mgmt_config.h
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

#ifndef H_IMG_MGMT_CONFIG_
#define H_IMG_MGMT_CONFIG_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of updatable images */

#define IMG_MGMT_UPDATABLE_IMAGE_NUMBER 1

/* Image status list will only contain image attributes that are true/non-zero */

#define IMG_MGMT_FRUGAL_LIST    0

#define IMG_MGMT_UL_CHUNK_SIZE  0
#define IMG_MGMT_VERBOSE_ERR    0
#define IMG_MGMT_LAZY_ERASE     0
#define IMG_MGMT_DUMMY_HDR      0
#define IMG_MGMT_BOOT_CURR_SLOT 0

#endif
