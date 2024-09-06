/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/mgmt.c
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

#include <string.h>
#include <assert.h>

#include <mgmt/mcumgr/mgmt.h>

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

static struct list_node g_mgmt_group_list =
  LIST_INITIAL_VALUE(g_mgmt_group_list);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function
 ****************************************************************************/

/****************************************************************************
 * Name: mgmt_unregister_group
 *
 * Description:
 *
 ****************************************************************************/

void mgmt_unregister_group(FAR struct mgmt_group_s *group)
{
  /* Not supported */

  ASSERT(0);
}

/****************************************************************************
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

FAR const struct mgmt_handler_s *
mgmt_find_handler(uint16_t group_id, uint16_t command_id)
{
  FAR struct mgmt_group_s *group = NULL;
  FAR struct mgmt_group_s *loop_group;

  /* Find the group with the specified group id, if one exists
   * check the handler for the command id and make sure
   * that is not NULL. If that is not set, look for the group
   * with a command id that is set
   */

  list_for_every_entry(&g_mgmt_group_list, loop_group,
                       struct mgmt_group_s, node)
  {
    if (loop_group->mg_group_id == group_id)
      {
        if (command_id >= loop_group->mg_handlers_count)
          {
            break;
          }

        if (!loop_group->mg_handlers[command_id].mh_read
            && !loop_group->mg_handlers[command_id].mh_write)
          {
            continue;
          }

        group = loop_group;
        break;
      }
  }

  if (group == NULL)
    {
      return NULL;
    }

  return &group->mg_handlers[command_id];
}

/****************************************************************************
 * Name: mgmt_find_group
 *
 * Description:
 *
 ****************************************************************************/

const struct mgmt_group_s *mgmt_find_group(uint16_t group_id)
{
  struct mgmt_group_s *loop_group;

	/* Find the group with the specified group id */

  list_for_every_entry(&g_mgmt_group_list, loop_group,
                       struct mgmt_group_s, node)
    {
      if (loop_group->mg_group_id == group_id)
        {
          return loop_group;
        }
    }

	return NULL;
}

/****************************************************************************
 * Name: mgmt_get_handler
 *
 * Description:
 *
 ****************************************************************************/

const struct mgmt_handler_s *
mgmt_get_handler(const struct mgmt_group_s *group, uint16_t command_id)
{
	if (command_id >= group->mg_handlers_count)
    {
      return NULL;
    }

	if (!group->mg_handlers[command_id].mh_read &&
	    !group->mg_handlers[command_id].mh_write)
    {
      return NULL;
    }

	return &group->mg_handlers[command_id];
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/****************************************************************************
 * Name: mgmt_find_error_translation_function
 *
 * Description:
 *
 ****************************************************************************/

smp_translate_error_fn
mgmt_find_error_translation_function(uint16_t group_id)
{
	FAR struct mgmt_group_s *group = NULL;
  FAR struct mgmt_group_s *loop_group;

	/* Find the group with the specified group ID. */

  list_for_every_entry(&g_mgmt_group_list, loop_group,
                       struct mgmt_group_s, node)
    {
      if (loop_group->mg_group_id == group_id)
        {
          group = loop_group;
          break;
        }
    }

	if (group == NULL)
    {
      return NULL;
    }

	return group->mg_translate_error;
}
#endif

/****************************************************************************
 * Name: mgmt_register_group
 *
 * Description:
 *
 ****************************************************************************/

void mgmt_register_group(FAR struct mgmt_group_s *group)
{
  list_add_tail(&g_mgmt_group_list, &group->node);
}
