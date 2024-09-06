/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/os_mgmt.h
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

#ifndef H_OS_MGMT_
#define H_OS_MGMT_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Command IDs for OS management group. */

#define OS_MGMT_ID_ECHO			0
#define OS_MGMT_ID_CONS_ECHO_CTRL	1
#define OS_MGMT_ID_TASKSTAT		2
#define OS_MGMT_ID_MPSTAT		3
#define OS_MGMT_ID_DATETIME_STR		4
#define OS_MGMT_ID_RESET		5
#define OS_MGMT_ID_MCUMGR_PARAMS	6
#define OS_MGMT_ID_INFO			7
#define OS_MGMT_ID_BOOTLOADER_INFO	8

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Command result codes for OS management group */

enum os_mgmt_err_code_t
{
	/* No error, this is implied if there is no ret value in the response */

	OS_MGMT_ERR_OK = 0,

	/* Unknown error occurred. */

	OS_MGMT_ERR_UNKNOWN,

	/* The provided format value is not valid. */

	OS_MGMT_ERR_INVALID_FORMAT,

	/* Query was not recognized. */

	OS_MGMT_ERR_QUERY_YIELDS_NO_ANSWER,

	/* RTC is not set */

	OS_MGMT_ERR_RTC_NOT_SET,

	/* RTC command failed */

	OS_MGMT_ERR_RTC_COMMAND_FAILED,
};

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_ */
