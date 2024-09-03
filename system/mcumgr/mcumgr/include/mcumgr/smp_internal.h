/****************************************************************************
 * apps/system/mcumgr/mcumgr/include/mcumgr/smp_internal.h
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

#ifndef __SYSTEM_MCUMGR_MCUMGR_INCLUDE_MCUMGR_SMP_INTERNAL_H
#define __SYSTEM_MCUMGR_MCUMGR_INCLUDE_MCUMGR_SMP_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include <mcumgr/transport/smp.h>
#include <zephyr/net/buf.h>

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

struct smp_hdr
{
#ifdef CONFIG_LITTLE_ENDIAN
  uint8_t  nh_op:3;             /* MGMT_OP_[...] */
  uint8_t  nh_version:2;
  uint8_t  _res1:3;
#else
  uint8_t  _res1:3;
  uint8_t  nh_version:2;
  uint8_t  nh_op:3;             /* MGMT_OP_[...] */
#endif
  uint8_t  nh_flags;            /* Reserved for future flags */
  uint16_t nh_len;              /* Length of the payload */
  uint16_t nh_group;            /* MGMT_GROUP_ID_[...] */
  uint8_t  nh_seq;              /* Sequence number */
  uint8_t  nh_id;               /* Message ID within group */
};

struct smp_transport;

/****************************************************************************
 * Public Function Prototyppes
 ****************************************************************************/

/****************************************************************************
 * Name: smp_rx_req
 *
 * Description:
 *   Enqueues an incoming SMP request packet for processing.
 *   This function always consumes the supplied net_buf.
 *
 * Input Parameters:
 *   smtp - The transport to use to send the corresponding response(s).
 *   nb   - The request packet to process.
 *
 ****************************************************************************/

void smp_rx_req(FAR struct smp_transport *smtp, FAR struct net_buf *nb);

/****************************************************************************
 * Name: smp_alloc_rsp
 *
 * Description:
 *   Allocates a response buffer.
 *   If a source buf is provided, its user data is copied into the new
 *buffer.
 *
 * Input Parameters:
 *   arg - The streamer providing the callback.
 *   buf - The buffer to free.
 *
 * Return Value:
 *   Newly-allocated buffer on success, NULL on failure.
 *
 ****************************************************************************/

void *smp_alloc_rsp(FAR const void *req, FAR void *arg);

/****************************************************************************
 * Name: smp_free_buf
 *
 * Description:
 *   Frees an allocated buffer.
 *
 * Input Parameters:
 *   buf - The buffer to free.
 *   arg - The streamer providing the callback.
 *
 ****************************************************************************/

void smp_free_buf(FAR void *buf, FAR void *arg);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_MCUMGR_MCUMGR_INCLUDE_MCUMGR_SMP_INTERNAL_H */
