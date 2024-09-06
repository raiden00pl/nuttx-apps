/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/smp.c
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

#include <assert.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#include "smp_reassembly.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smp_packet_alloc
 ****************************************************************************/

/****************************************************************************
 * Name: smp_process_packet
 *
 * Description:
 *   Processes a single SMP packet and sends the corresponding response(s).
 *
 ****************************************************************************/

static int smp_process_packet(FAR struct smp_transport *smpt, FAR struct smp_buf *nb)
{
  struct cbor_nb_reader reader;
  struct cbor_nb_writer writer;
  struct smp_streamer   streamer;
  int                   ret;

  streamer.reader = &reader;
  streamer.writer = &writer;
  streamer.smpt   = smpt;

  ret = smp_process_request_packet(&streamer, nb);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct smp_buf *smp_packet_alloc(void)
{
  return smp_buf_alloc(&pkt_pool, K_NO_WAIT);
}

/****************************************************************************
 * Name: smp_packet_free
 ****************************************************************************/

void smp_packet_free(FAR struct smp_buf *nb)
{
  smp_buf_unref(nb);
}

/****************************************************************************
 * Name: smp_alloc_rsp
 *
 * Description:
 *   Allocates a response buffer.
 *   If a source buf is provided, its user data is copied into the new
 *   buffer.
 *
 * Input Parameters:
 *   req -An optional source buffer to copy user data from.
 *   arg -The streamer providing the callback.
 *
 * Return Value:
 *   Newly-allocated buffer on success
 *
 *   NULL on failure.
 *
 ****************************************************************************/

FAR void *smp_alloc_rsp(FAR const void *req, FAR void *arg)
{
  FAR const struct smp_buf *req_nb;
  FAR struct smp_buf       *rsp_nb;
  FAR struct smp_transport *smpt = arg;

  req_nb = req;

  rsp_nb = smp_packet_alloc();
  if (rsp_nb == NULL)
    {
      return NULL;
    }

  if (smpt->functions.ud_copy)
    {
      smpt->functions.ud_copy(rsp_nb, req_nb);
    }
  else
    {
      memcpy(smp_buf_user_data(rsp_nb), smp_buf_user_data((void *)req_nb),
             req_nb->user_data_size);
    }

  return rsp_nb;
}

/****************************************************************************
 * Name: smp_free_buf
 ****************************************************************************/

void smp_free_buf(FAR void *buf, FAR void *arg)
{
  FAR struct smp_transport *smpt = arg;

  if (!buf)
    {
      return;
    }

  if (smpt->functions.ud_free)
    {
      smpt->functions.ud_free(smp_buf_user_data((struct smp_buf *)buf));
    }

  smp_packet_free(buf);
}

/****************************************************************************
 * Name: smp_transport_init
 ****************************************************************************/

int smp_transport_init(FAR struct smp_transport *smpt)
{
  DEBUGASSERT((smpt->functions.output != NULL));

  if (smpt->functions.output == NULL)
    {
      return -EINVAL;
    }

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
  smp_reassembly_init(smpt);
#endif

  return 0;
}

/****************************************************************************
 * Name: smp_rx_req
 *
 * Description:
 *   Enqueues an incoming SMP request packet for processing.
 *   This function always consumes the supplied smp_buf.
 *
 * Input Parameters:
 *   smpt - The transport to use to send the corresponding response(s).
 *   nb   - The request packet to process.
 *
 ****************************************************************************/

void smp_rx_req(FAR struct smp_transport *smpt, FAR struct smp_buf *nb)
{
  smp_process_packet(smpt, nb);
}

/****************************************************************************
 * Name: smp_rx_remove_invalid
 ****************************************************************************/

void smp_rx_remove_invalid(FAR struct smp_transport *smpt, FAR void *arg)
{
  /* Not implemented yet */

  ASSERT(0);
}

/****************************************************************************
 * Name: smp_rx_clear
 ****************************************************************************/

void smp_rx_clear(FAR struct smp_transport *smpt)
{
  /* Not implemented */
  ASSERT(0);
}

/****************************************************************************
 * Name: smp_init
 ****************************************************************************/

#define CONFIG_MGMT_MCUMGR_SMP_BUF_COUNT 10
#define CONFIG_MGMT_MCUMGR_SMP_BUF_SIZE  255
#define CONFIG_MGMT_MCUMGR_SMP_BUF_USIZE 32
int smp_init(void)
{
  /* Initialize SMP buffers */

  ret = smp_buf_init(CONFIG_MGMT_MCUMGR_SMP_BUF_COUNT,
               CONFIG_MGMT_MCUMGR_SMP_BUF_SIZE,
               CONFIG_MGMT_MCUMGR_SMP_BUF_USIZE);

  return 0;
}
