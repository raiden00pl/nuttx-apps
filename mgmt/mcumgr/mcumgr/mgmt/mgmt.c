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
#include <zephyr/device.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

static sys_slist_t mgmt_group_list = SYS_SLIST_STATIC_INIT(&mgmt_group_list);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function
 ****************************************************************************/

/****************************************************************************
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

void mgmt_unregister_group(FAR struct mgmt_group *group)
{
  (void)sys_slist_find_and_remove(&mgmt_group_list, &group->node);
}

/****************************************************************************
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

FAR const struct mgmt_handler *mgmt_find_handler(uint16_t group_id,
                                                 uint16_t command_id)
{
  FAR struct mgmt_group *group = NULL;
  FAR sys_snode_t *snp;
  FAR sys_snode_t *sns;

  /* Find the group with the specified group id, if one exists
   * check the handler for the command id and make sure
   * that is not NULL. If that is not set, look for the group
   * with a command id that is set
   */

  SYS_SLIST_FOR_EACH_NODE_SAFE(&mgmt_group_list, snp, sns)
  {
    FAR struct mgmt_group *loop_group
        = CONTAINER_OF(snp, struct mgmt_group, node);
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
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

const struct mgmt_group *mgmt_find_group(uint16_t group_id)
{
	sys_snode_t *snp;
	sys_snode_t *sns;

	/* Find the group with the specified group id */

	SYS_SLIST_FOR_EACH_NODE_SAFE(&mgmt_group_list, snp, sns)
    {
      struct mgmt_group *loop_group =
        CONTAINER_OF(snp, struct mgmt_group, node);
      if (loop_group->mg_group_id == group_id)
        {
          return loop_group;
        }
    }

	return NULL;
}

/****************************************************************************
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

const struct mgmt_handler *
mgmt_get_handler(const struct mgmt_group *group, uint16_t command_id)
{
	if (command_id >= group->mg_handlers_count) {
		return NULL;
	}

	if (!group->mg_handlers[command_id].mh_read &&
	    !group->mg_handlers[command_id].mh_write) {
		return NULL;
	}

	return &group->mg_handlers[command_id];
}

#if defined(CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL)
/****************************************************************************
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

smp_translate_error_fn mgmt_find_error_translation_function(uint16_t group_id)
{
	struct mgmt_group *group = NULL;
	sys_snode_t *snp, *sns;

	/* Find the group with the specified group ID. */
	SYS_SLIST_FOR_EACH_NODE_SAFE(&mgmt_group_list, snp, sns) {
		struct mgmt_group *loop_group =
			CONTAINER_OF(snp, struct mgmt_group, node);
		if (loop_group->mg_group_id == group_id) {
			group = loop_group;
			break;
		}
	}

	if (group == NULL) {
		return NULL;
	}

	return group->mg_translate_error;
}
#endif

/****************************************************************************
 * Name:
 *
 * Description:
 *
 ****************************************************************************/

void mgmt_register_group(FAR struct mgmt_group *group)
{
  sys_slist_append(&mgmt_group_list, &group->node);
}

/****************************************************************************
 * Name: mcumgr_handlers_init
 *
 * Description:
 *   Processes all registered MCUmgr handlers at start up and registers them
 *
 ****************************************************************************/

int mcumgr_handlers_init(void)
{

  STRUCT_SECTION_FOREACH(mcumgr_handler, handler)
    {
      if (handler->init)
        {
          handler->init();
        }
    }

  return 0;
}
