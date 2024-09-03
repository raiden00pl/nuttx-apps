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

#include <assert.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#include "smp_reassembly.h"

static struct k_work_q smp_work_queue;

static const struct k_work_queue_config smp_work_queue_config
    = { .name = "mcumgr smp" };

NET_BUF_POOL_DEFINE(pkt_pool, CONFIG_MCUMGR_TRANSPORT_NETBUF_COUNT,
                    CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE,
                    CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE, NULL);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smp_packet_alloc
 ****************************************************************************/

FAR struct net_buf *smp_packet_alloc(void)
{
  return net_buf_alloc(&pkt_pool, K_NO_WAIT);
}

/****************************************************************************
 * Name: smp_process_packet
 *
 * Description:
 *   Processes a single SMP packet and sends the corresponding response(s).
 *
 ****************************************************************************/

static int smp_process_packet(FAR struct smp_transport *smpt, FAR struct net_buf *nb)
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
 * Name: smp_handle_regs
 *
 * Description:
 *   Processes all received SNP request packets.
 *
 ****************************************************************************/

static void smp_handle_reqs(FAR struct k_work *work)
{
  FAR struct smp_transport *smpt;
  FAR struct net_buf       *nb;

  smpt = (FAR void *)work;

  while ((nb = net_buf_get(&smpt->fifo, K_NO_WAIT)) != NULL)
    {
      smp_process_packet(smpt, nb);
    }
}

/****************************************************************************
 * Name: smp_init
 ****************************************************************************/

static int smp_init(void)
{
  k_work_queue_init(&smp_work_queue);

  k_work_queue_start(&smp_work_queue, smp_work_queue_stack,
                     K_THREAD_STACK_SIZEOF(smp_work_queue_stack),
                     CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_THREAD_PRIO,
                     &smp_work_queue_config);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smp_packet_free
 ****************************************************************************/

void smp_packet_free(FAR struct net_buf *nb)
{
  net_buf_unref(nb);
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
  FAR const struct net_buf *req_nb;
  FAR struct net_buf       *rsp_nb;
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
      memcpy(net_buf_user_data(rsp_nb), net_buf_user_data((void *)req_nb),
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
      smpt->functions.ud_free(net_buf_user_data((struct net_buf *)buf));
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

  k_work_init(&smpt->work, smp_handle_reqs);
  k_fifo_init(&smpt->fifo);

  return 0;
}

/****************************************************************************
 * Name: smp_rx_remove_invalid
 *
 * Description:
 *   Enqueues an incoming SMP request packet for processing.
 *   This function always consumes the supplied net_buf.
 *
 * Input Parameters:
 *   smpt - The transport to use to send the corresponding response(s).
 *   nb   - The request packet to process.
 *
 ****************************************************************************/

void smp_rx_req(FAR struct smp_transport *smpt, FAR struct net_buf *nb)
{
  net_buf_put(&smpt->fifo, nb);
  k_work_submit_to_queue(&smp_work_queue, &smpt->work);
}

/****************************************************************************
 * Name: smp_rx_remove_invalid
 ****************************************************************************/

void smp_rx_remove_invalid(FAR struct smp_transport *zst, FAR void *arg)
{
  FAR struct net_buf *nb;
  struct k_fifo temp_fifo;

  if (zst->functions.query_valid_check == NULL)
    {
      /* No check check function registered, abort check */

      return;
    }

  /* Cancel current work-queue if ongoing */
  if (k_work_busy_get(&zst->work) & (K_WORK_RUNNING | K_WORK_QUEUED))
    {
      k_work_cancel(&zst->work);
    }

  /* Run callback function and remove all buffers that are no longer needed.
   * Store those that are in a temporary FIFO
   */

  k_fifo_init(&temp_fifo);

  while ((nb = net_buf_get(&zst->fifo, K_NO_WAIT)) != NULL)
    {
      if (!zst->functions.query_valid_check(nb, arg))
        {
          smp_free_buf(nb, zst);
        }
      else
        {
          net_buf_put(&temp_fifo, nb);
        }
    }

  /* Re-insert the remaining queued operations into the original FIFO */

  while ((nb = net_buf_get(&temp_fifo, K_NO_WAIT)) != NULL)
    {
      net_buf_put(&zst->fifo, nb);
    }

  /* If at least one entry remains, queue the workqueue for running */

  if (!k_fifo_is_empty(&zst->fifo))
    {
      k_work_submit_to_queue(&smp_work_queue, &zst->work);
    }
}

/****************************************************************************
 * Name: smp_rx_clear
 ****************************************************************************/

void smp_rx_clear(FAR struct smp_transport *zst)
{
  FAR struct net_buf *nb;

  /* Cancel current work-queue if ongoing */

  if (k_work_busy_get(&zst->work) & (K_WORK_RUNNING | K_WORK_QUEUED))
    {
      k_work_cancel(&zst->work);
    }

  /* Drain the FIFO of all entries without re-adding any */

  while ((nb = net_buf_get(&zst->fifo, K_NO_WAIT)) != NULL)
    {
      smp_free_buf(nb, zst);
    }
}
