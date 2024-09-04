/****************************************************************************
 * apps/mgmt/mcumgr/daemon.c
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

#include <nuttx/config.h>

#include <stdio.h>

/****************************************************************************
 * Public Function
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
#warning move to common logic ?
  /* Initialize SMP */

  ret = smp_init();
  if (ret < 0)
    {
      printf("smp_init failed: %d\n", ret);
      goto errout;
    }

  /*  */

  ret = os_mgmt_register_group();
  ret = shell_mgmt_register_group();
  ret = enum_mgmt_register_group();

  /* Initialize handlers */

  ret = mcumgr_handlers_init()



  /*  */

  ret = smp_uart_init();
  /* ret = smp_bt_init(); */
  /* ret = smp_shell_init(); */

errout:
  return 0;
}
