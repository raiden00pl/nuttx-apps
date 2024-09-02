/****************************************************************************
 * apps/system/mcumgr/porting/img_mgmt.c
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

#include "mgmt/mgmt.h"
#include "img_mgmt/img_mgmt_impl.h"
#include "img_mgmt/img_mgmt.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int img_mgmt_impl_upload_inspect(const struct img_mgmt_upload_req *req,
                             struct img_mgmt_upload_action *action,
                             const char **errstr)
{
  return 0;
}

int img_mgmt_impl_erase_slot(void)
{
  return 0;
}

int img_mgmt_impl_write_pending(int slot, bool permanent)
{
  return 0;
}

int img_mgmt_impl_write_confirmed(void)
{
  return 0;
}

int img_mgmt_impl_read(int slot, unsigned int offset, void *dst,
                   unsigned int num_bytes)
{
  return 0;
}

int img_mgmt_impl_write_image_data(unsigned int offset, const void *data,
                                   unsigned int num_bytes, bool last)
{
  return 0;
}

int img_mgmt_impl_erase_image_data(unsigned int off, unsigned int num_bytes)
{
  return 0;
}

int img_mgmt_impl_erase_if_needed(uint32_t off, uint32_t len)
{
  return 0;
}

int img_mgmt_impl_swap_type(int slot)
{
  return 0;
}

int img_mgmt_impl_erased_val(int slot, uint8_t *erased_val)
{
  return 0;
}

void img_mgmt_module_init(void)
{
  return 0;
}
