/****************************************************************************
 * apps/include/mgmt/mcumgr/mgmt.h
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

#ifndef __INCLUDE_MGMT_MCUMGR_MGMT_H
#define __INCLUDE_MGMT_MCUMGR_MGMT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/list.h>

#include <mgmt/mcumgr/mgmt_defines.h>
#include <mgmt/mcumgr/smp.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_MCUMGR_SMP_VERBOSE_ERR_RESPONSE
#  define MGMT_CTXT_SET_RC_RSN(mc, rsn) ((mc->rc_rsn) = (rsn))
#  define MGMT_CTXT_RC_RSN(mc) ((mc)->rc_rsn)
#else
#  define MGMT_CTXT_SET_RC_RSN(mc, rsn)
#  define MGMT_CTXT_RC_RSN(mc) NULL
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/* Forward declaration */

struct mgmt_group_s;

/****************************************************************************
 * Name: mgmt_alloc_rsp_fn
 *
 * Description:
 *   Allocates a buffer suitable for holding a response.
 *   If a source buf is provided, its user data is copied into the new
 *   buffer.
 *
 * Input Parameters:
 *   src_buf - An optional source buffer to copy user data from.
 *   arg    - Optional streamer argument.
 *
 * Return Value:
 *   Newly-allocated buffer on success NULL on failure.
 *
 ****************************************************************************/

CODE typedef void *(*mgmt_alloc_rsp_fn)(FAR const void *src_buf,
                                        FAR void *arg);

/****************************************************************************
 * Name: mgmt_reset_buf_fn
 *
 * Description:
 *   Resets a buffer to a length of 0.
 *   The buffer's user data remains, but its payload is cleared.
 *
 * Input Parameters:
 *   buf - The buffer to reset.
 *   arg - Optional streamer argument.
 *
 ****************************************************************************/

CODE typedef void (*mgmt_reset_buf_fn)(FAR void *buf, FAR void *arg);

/****************************************************************************
 * Name: mgmt_handler_fn
 *
 * Description:
 *   Processes a request and writes the corresponding response.
 *   A separate handler is required for each supported op-ID pair.
 *
 * Input Parameters:
 *   ctxt - The mcumgr context to use.
 *
 * Return Value:
 * 0 if a response was successfully encoded, #mcumgr_err_t code on failure.
 *
 ****************************************************************************/

CODE typedef int (*mgmt_handler_fn)(FAR struct smp_streamer_s *ctxt);

/****************************************************************************
 * Name: mgmt_groups_cb_t
 *
 * Description:
 *   Group iteration callback
 *
 * Input Parameters:
 *   group     - Group
 *   user_data - User-supplied data
 *
 * @return true to continue with the foreach callback, false to abort
 *
 ****************************************************************************/

CODE typedef bool (*mgmt_groups_cb_t)(FAR const struct mgmt_group_s *group,
                                      FAR void *user_data);

/* Read handler and write handler for a single command ID.
 * Set use_custom_payload to true when using a user defined payload type
 */

struct mgmt_handler_s
{
	mgmt_handler_fn  mh_read;
	mgmt_handler_fn  mh_write;
#ifdef CONFIG_MCUMGR_MGMT_HANDLER_USER_DATA
	FAR void         *user_data;
#endif
};

/* A collection of handlers for an entire command group. */

struct mgmt_group_s
{
	/* Entry list node. */

  struct list_node node;

	/* Array of handlers; one entry per command ID. */

	FAR const struct mgmt_handler_s *mg_handlers;
	uint16_t mg_handlers_count;

	/* The numeric ID of this group. */

	uint16_t mg_group_id;

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	/* A function handler for translating version 2 SMP error codes to
   * version 1 SMP error codes (optional)
	 */

	smp_translate_error_fn mg_translate_error;
#endif

#ifdef CONFIG_MCUMGR_MGMT_CUSTOM_PAYLOAD
	/* Should be true when using user defined payload */

	bool custom_payload;
#endif

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	/* NULL-terminated name of group */

	FAR const char *mg_group_name;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: mgmt_register_group
 *
 * Description:
 *   Registers a full command group.
 *
 * Input Parameters:
 *   group - The group to register.
 *
 ****************************************************************************/

void mgmt_register_group(FAR struct mgmt_group_s *group);

/****************************************************************************
 * Name: mgmt_unregister_group
 *
 * Description:
 *   Unregisters a full command group.
 *
 * Input Parameters:
 *   group - The group to register.
 *
 ****************************************************************************/

void mgmt_unregister_group(FAR struct mgmt_group_s *group);

/****************************************************************************
 * Name: mgmt_groups_foreach
 *
 * Description:
 *   Iterate over groups
 *
 * Input Parameters:
 *   user_cb   - User callback
 *   user_data - User-supplied data
 *
 ****************************************************************************/

void mgmt_groups_foreach(mgmt_groups_cb_t user_cb, FAR void *user_data);

/****************************************************************************
 * Name: mgmt_find_handler
 *
 * Description:
 *   Finds a registered command handler.
 *
 * Input Parameters:
 *   group_id   - The group of the command to find.
 *   command_id - The ID of the command to find.
 *
 * Return Value:
 *   The requested command handler on success;
 *	 NULL on failure.
 *
 ****************************************************************************/

FAR const struct mgmt_handler_s *
mgmt_find_handler(uint16_t group_id, uint16_t command_id);

/****************************************************************************
 * Name: mgmt_find_group
 *
 * Description:
 *   Finds a registered command group.
 *
 * Input Parameters:
 *   group_id - The group id of the command group to find.
 *
 * Return Value:
 *   The requested group on success;
 *	 NULL on failure.
 *
 ****************************************************************************/

FAR const struct mgmt_group_s *mgmt_find_group(uint16_t group_id);

/****************************************************************************
 * Name: mgmt_get_handler
 *
 * Description:
 *   Finds a registered command handler.
 *
 * Input Parameters:
 *   group      - The group of the command to find.
 *   command_id - The ID of the command to find.
 *
 * Return Value:
 *   The requested command handler on success;
 *	 NULL on failure.
 *
 ****************************************************************************/

FAR const struct mgmt_handler_s *
mgmt_get_handler(FAR const struct mgmt_group_s *group, uint16_t command_id);

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/****************************************************************************
 * Name: mgmt_find_error_translation_function
 *
 * Description:
 *   Finds a registered error translation function for converting from SMP
 *	 version 2 error codes to legacy SMP version 1 error codes.
 *
 * Input Parameters:
 *   group_id - The group of the translation function to find.
 *
 * Return Value:
 *   Requested lookup function on success.
 *   NULL on failure.
 *
 ****************************************************************************/

smp_translate_error_fn
mgmt_find_error_translation_function(uint16_t group_id);
#endif  /* CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL */

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_MGMT_MCUMGR_MGMT_H */
