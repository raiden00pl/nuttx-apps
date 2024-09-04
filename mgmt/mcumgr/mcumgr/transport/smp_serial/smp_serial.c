/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/smp_serial.c
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
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/buf.h>
#include <zephyr/drivers/console/uart_mcumgr.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * Processes a single line (fragment) coming from the mcumgr UART driver.
 */
static void smp_uart_process_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

	/* Decode the fragment and write the result to the global receive
	 * context.
	 */
	nb = mcumgr_serial_process_frag(&smp_uart_rx_ctxt,
					rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	uart_mcumgr_free_rx_buf(rx_buf);

	/* If a complete packet has been received, pass it to SMP for
	 * processing.
	 */
	if (nb != NULL) {
		smp_rx_req(&smp_uart_transport, nb);
	}
}

static void smp_uart_process_rx_queue(struct k_work *work)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	while ((rx_buf = k_fifo_get(&smp_uart_rx_fifo, K_NO_WAIT)) != NULL) {
		smp_uart_process_frag(rx_buf);
	}
}

/**
 * Enqueues a received SMP fragment for later processing.  This function
 * executes in the interrupt context.
 */
static void smp_uart_rx_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	k_fifo_put(&smp_uart_rx_fifo, rx_buf);
	k_work_submit(&smp_uart_work);
}

static uint16_t smp_uart_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_TRANSPORT_UART_MTU;
}

static int smp_uart_tx_pkt(struct net_buf *nb)
{
	int rc;

	rc = uart_mcumgr_send(nb->data, nb->len);
	smp_packet_free(nb);

	return rc;
}

/****************************************************************************
 * Public Function
 ****************************************************************************/

int smp_uart_init(void)
{
	int rc;

	smp_uart_transport.functions.output = smp_uart_tx_pkt;
	smp_uart_transport.functions.get_mtu = smp_uart_get_mtu;

	rc = smp_transport_init(&smp_uart_transport);
	if (rc == 0)
    {
      uart_mcumgr_register(smp_uart_rx_frag);
    }

	return rc;
}
