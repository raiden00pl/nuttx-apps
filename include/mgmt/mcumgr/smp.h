/****************************************************************************
 * apps/include/mgmt/mcumgr/smp.h
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

#ifndef __INCLUDE_MGMT_MCUMGR_SMP_H
#define __INCLUDE_MGMT_MCUMGR_SMP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct smp_transport;
struct net_buf;

/****************************************************************************
 * Name: smp_transport_out_fn
 *
 * Description:
 *   SMP transmit callback for transport
 *
 *   The supplied net_buf is always consumed, regardless of return code.
 *
 * Input Parameters:
 *   nb - The net_buf to transmit.
 *
 * Return Value:
 *   0 on success,
 *
 *   mcumgr_err_t code on failure.
 *
 ****************************************************************************/

typedef CODE int (*smp_transport_out_fn)(FAR struct net_buf *nb);

/****************************************************************************
 * Name: smp_transport_get_mtu_fn
 *
 * Description:
 *   SMP MTU query callback for transport
 *
 *   The supplied net_buf should contain a request received from the peer
 *   whose MTU is being queried.  This function takes a net_buf parameter
 *   because some transports store connection-specific information in the
 *   net_buf user header (e.g., the BLE transport stores the peer address).
 *
 * Input Parameters:
 *   nb - Contains a request from the relevant peer.
 *
 * Return Value:
 *   The transport's MTU;
 *
 *   0 if transmission is currently not possible.
 *
 ****************************************************************************/

typedef CODE uint16_t (*smp_transport_get_mtu_fn)(
  FAR const struct net_buf *nb);

/****************************************************************************
 * Name: smp_transport_ud_copy_fn
 *
 * Description:
 *   SMP copy user_data callback
 *
 *   The supplied src net_buf should contain a user_data that cannot be
 *   copied using regular memcpy function (e.g., the BLE transport net_buf
 *   user_data stores the connection reference that has to be incremented
 *   when is going to be used by another buffer).
 *
 * Input Parameters:
 *   dst - Source buffer user_data pointer.
 *   src - Destination buffer user_data pointer.
 *
 * Return Value:
 *   0 on success, #mcumgr_err_t code on failure.
 *
 ****************************************************************************/

typedef CODE int (*smp_transport_ud_copy_fn)(FAR struct net_buf *dst,
                                             FAR const struct net_buf *src);

/****************************************************************************
 * Name: smp_transport_ud_free_fn
 *
 * Description:
 *   SMP free user_data callback
 *
 *   This function frees net_buf user data, because some transports store
 *   connection-specific information in the net_buf user data (e.g., the BLE
 *   transport stores the connection reference that has to be decreased).
 *
 * Input Parameters:
 *   ud - Contains a user_data pointer to be freed.
 *
 ****************************************************************************/

typedef CODE void (*smp_transport_ud_free_fn)(FAR void *ud);

/****************************************************************************
 * Name: smp_transport_query_valid_check_fn
 *
 * Description:
 *   Function for checking if queued data is still valid.
 *
 *   This function is used to check if queued SMP data is still valid
 *   e.g. on a remote device disconnecting, this is triggered when
 *   smp_rx_remove_invalid() is called.
 *
 * Input Parameters:
 *   nb  - net buf containing queued request.
 *   arg - Argument provided when calling smp_rx_remove_invalid() function.
 *
 * Return Value:
 *   false if data is no longer valid/should be freed, true otherwise.
 *
 ****************************************************************************/

typedef CODE bool (*smp_transport_query_valid_check_fn)(
  FAR struct net_buf *nb, FAR void *arg);

/* Function pointers of SMP transport functions, if a handler is NULL then
 * it is not supported/implemented.
 */

struct smp_transport_api_t
{
	/* Transport's send function. */

	smp_transport_out_fn output;

	/* Transport's get-MTU function. */

	smp_transport_get_mtu_fn get_mtu;

	/* Transport buffer user_data copy function. */

	smp_transport_ud_copy_fn ud_copy;

	/* Transport buffer user_data free function. */

	smp_transport_ud_free_fn ud_free;

	/* Transport's check function for if a query is valid. */

	smp_transport_query_valid_check_fn query_valid_check;
};

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY

/* Packet reassembly internal data, API access only */

struct smp_transport_reassembly
{
  FAR struct net_buf *current;    /* net_buf used for reassembly */
  uint16_t            expected;		/* expected bytes to come */
};
#endif

/* SMP transport object for sending SMP responses. */

struct smp_transport
{
	/* Must be the first member. */

	struct k_work work;

	/* FIFO containing incoming requests to be processed. */

	struct k_fifo fifo;

	/* Function pointers */

	struct smp_transport_api_t functions;

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
    struct smp_transport_reassembly __reassembly;
#endif
};

/****************************************************************************
 * Name: smp_transport_init
 *
 * Description:
 *   Initializes a SMP transport object.
 *
 * Input Parameters:
 *   smpt - The transport to construct.
 *
 * Return Value:
 *   0 If successful
 *
 *   Negative errno code if failure.
 *
 ****************************************************************************/

int smp_transport_init(FAR struct smp_transport *smpt);

/****************************************************************************
 * Name: smp_rx_remove_invalid
 *
 * Description:
 *   Used to remove queued requests for an SMP transport that are no longer
 *   valid. A smp_transport_query_valid_check_fn() function must be
 *   registered for this to function.
 *   If the smp_transport_query_valid_check_fn() function returns false
 *   during a callback, the queried command will classed as invalid and
 *   dropped.
 *
 * Input Parameters:
 *   zst - The transport to use.
 *   arg - Argument provided to callback smp_transport_query_valid_check_fn()
 *         function.
 *
 ****************************************************************************/

void smp_rx_remove_invalid(FAR struct smp_transport *zst, FAR void *arg);

/****************************************************************************
 * Name: smp_rx_clear
 *
 * Description:
 *   Used to clear pending queued requests for an SMP transport.
 *
 * Input Parameters:
 *   zst - The transport to use.
 *
 ****************************************************************************/

void smp_rx_clear(FAR struct smp_transport *zst);

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_MGMT_MCUMGR_SMP_H */
