/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/os_mgmt.c
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

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int os_mgmt_translate_error_code(uint16_t ret);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct mgmt_handler os_mgmt_group_handlers[] =
{
  /* OS_MGMT_ID_ECHO */

#ifdef CONFIG_MCUMGR_GRP_OS_ECHO
	{
		os_mgmt_echo, os_mgmt_echo
	},
#else
  {
    NULL, NULL
  },
#endif

  /* OS_MGMT_ID_CONS_ECHO_CTRL */

  {
    NULL, NULL
  },

  /* OS_MGMT_ID_TASKSTAT */

  {
    NULL, NULL
  },

  /* OS_MGMT_ID_MPSTAT */

  {
    NULL, NULL
  },

  /* OS_MGMT_ID_DATETIME_STR */

  {
    NULL, NULL
  },

  /* OS_MGMT_ID_RESET */

#ifdef CONFIG_MCUMGR_GRP_OS_RESET
	{
		os_mgmt_reset, os_mgmt_reset
	},
#else
  {
    NULL, NULL
  },
#endif

  /* OS_MGMT_ID_MCUMGR_PARAMS */

  {
    NULL, NULL
  },

  /* OS_MGMT_ID_INFO */

  {
    NULL, NULL
  },

  /* OS_MGMT_ID_BOOTLOADER_INFO */

  {
    NULL, NULL
  },
};

static struct mgmt_group_s os_mgmt_group =
{
  .mg_handlers       = os_mgmt_group_handlers,
  .mg_handlers_count = OS_MGMT_GROUP_SZ,
  .mg_group_id       = MGMT_GROUP_ID_OS,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = os_mgmt_translate_error_code,
#endif
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	.mg_group_name = "os mgmt",
#endif
};

/****************************************************************************
 * Private Function
 ****************************************************************************/

#ifdef CONFIG_MCUMGR_GRP_OS_ECHO
/**
 * Command handler: os echo
 */

static int os_mgmt_echo(struct smp_streamer *ctxt)
{
  struct zcbor_string value = { 0 };
  struct zcbor_string key;
  zcbor_state_t *zsd = ctxt->reader->zs;
  zcbor_state_t *zse = ctxt->writer->zs;
  bool ok;

  if (!zcbor_map_start_decode(zsd))
    {
      return MGMT_ERR_EUNKNOWN;
    }

  do
    {
      ok = zcbor_tstr_decode(zsd, &key);

      if (ok)
        {
          if (key.len == 1 && *key.value == 'd')
            {
              ok = zcbor_tstr_decode(zsd, &value);
              break;
            }

          ok = zcbor_any_skip(zsd, NULL);
        }
  } while (ok);

  if (!ok || !zcbor_map_end_decode(zsd))
    {
      return MGMT_ERR_EUNKNOWN;
    }

  ok = zcbor_tstr_put_lit(zse, "r") && zcbor_tstr_encode(zse, &value);

  return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}
#endif

#ifdef CONFIG_MCUMGR_GRP_OS_RESET
static int os_mgmt_reset(struct smp_streamer *ctxt)
{
  /* TODO: */
}
#endif

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
static int os_mgmt_translate_error_code(uint16_t ret)
{
  int rc;

  switch (ret)
    {
    case OS_MGMT_RET_RC_INVALID_FORMAT:
      rc = MGMT_ERR_EINVAL;
      break;

    case OS_MGMT_RET_RC_UNKNOWN:
    default:
      rc = MGMT_ERR_EUNKNOWN;
    }

  return rc;
}
#endif

/****************************************************************************
 * Public Function
 ****************************************************************************/

void os_mgmt_register_group(void)
{
  mgmt_register_group(&os_mgmt_group);
}
