/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/shell_mgmt.c
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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct mgmt_handler_s shell_mgmt_handlers[] =
{
  /* SHELL_MGMT_ID_EXEC */

  { NULL, shell_mgmt_exec },
};

static struct mgmt_group_s shell_mgmt_group =
{
  .mg_handlers       = shell_mgmt_handlers,
  .mg_handlers_count = SHELL_MGMT_HANDLER_CNT,
  .mg_group_id       = MGMT_GROUP_ID_SHELL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/*
 * Command handler: shell exec
 *
 */

static int shell_mgmt_exec(struct smp_streamer *ctxt)
{
}

/****************************************************************************
 * Public Function
 ****************************************************************************/

void shell_mgmt_register_group(void)
{
  mgmt_register_group(&shell_mgmt_group);
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
int shell_mgmt_translate_error_code(uint16_t ret)
{
  int rc;

  switch (ret)
    {
      case SHELL_MGMT_RET_RC_COMMAND_TOO_LONG:
      case SHELL_MGMT_RET_RC_EMPTY_COMMAND:
        rc = MGMT_ERR_EINVAL;
        break;

      default:
        rc = MGMT_ERR_EUNKNOWN;
    }

  return rc;
}
#endif
