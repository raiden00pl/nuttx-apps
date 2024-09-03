/****************************************************************************
 * apps/system/mcumgr/mcumgr/smp/smp_err.c
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

#ifdef CONFIG_MCUMGR_GRP_FS
#  include <mcumgr/grp/fs_mgmt/fs_mgmt.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_IMG
#  include <mcumgr/grp/img_mgmt/img_mgmt.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_OS
#  include <mcumgr/grp/os_mgmt/os_mgmt.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_SHELL
#  include <mcumgr/grp/shell_mgmt/shell_mgmt.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_STAT
#  include <mcumgr/grp/stat_mgmt/stat_mgmt.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_ZBASIC
#  include <mcumgr/grp/zephyr/zephyr_basic.h>
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smp_translate_error_code
 ****************************************************************************/

int smp_translate_error_code(uint16_t group, uint16_t ret)
{
  switch (group)
    {
#ifdef CONFIG_MCUMGR_GRP_OS
    case MGMT_GROUP_ID_OS:
      return os_mgmt_translate_error_code(ret);
#endif

#ifdef CONFIG_MCUMGR_GRP_IMG
    case MGMT_GROUP_ID_IMAGE:
      return img_mgmt_translate_error_code(ret);
#endif

#ifdef CONFIG_MCUMGR_GRP_STAT
    case MGMT_GROUP_ID_STAT:
      return stat_mgmt_translate_error_code(ret);
#endif

#ifdef CONFIG_MCUMGR_GRP_FS
    case MGMT_GROUP_ID_FS:
      return fs_mgmt_translate_error_code(ret);
#endif

#ifdef CONFIG_MCUMGR_GRP_SHELL
    case MGMT_GROUP_ID_SHELL:
      return shell_mgmt_translate_error_code(ret);
#endif

#ifdef CONFIG_MCUMGR_GRP_ZBASIC
    case ZEPHYR_MGMT_GRP_BASIC:
      return zephyr_basic_group_translate_error_code(ret);
#endif

    default:
      return MGMT_ERR_EUNKNOWN;
    }
}
