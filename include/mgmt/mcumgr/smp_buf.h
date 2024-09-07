/****************************************************************************
 * apps/mgmt/mcumgr/smp_buf.h
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

#ifndef __INCLUDE_MGMT_MCUMGR_SMP_BUF_H
#define __INCLUDE_MGMT_MCUMGR_SMP_BUF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* SMP buffer */

struct smp_buf_s
{
  uint8_t *data;
  size_t len;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int smp_buf_init(size_t count, size_t size, size_t usize);

void smp_buf_reset(FAR struct smp_buf_s *buf);

int smp_buf_tailroom(FAR struct smp_buf_s *nb);

FAR void *smp_buf_pull(FAR struct smp_buf_s *buf, size_t len);

FAR struct smp_buf_s *smp_buf_alloc(FAR struct smp_buf_pool *pool,
                                  unsigned int timeout);

FAR struct smp_buf_s *smp_buf_get(FAR struct k_fifo *fifo,
                                unsigned int timeout, FAR const char *func,
                                int line);

void smp_buf_unref(FAR struct smp_buf_s *buf);

FAR void *smp_buf_user_data(FAR const struct smp_buf_s *buf);

void smp_buf_put(FAR struct k_fifo *fifo, FAR struct smp_buf_s *buf);

FAR void *smp_buf_add_mem(FAR struct smp_buf_s *buf, FAR const void *mem,
                          size_t len);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_MGMT_MCUMGR_SMP_BUF_H */
