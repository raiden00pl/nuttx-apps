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

#include <zcbor_common.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct smp_transport;
struct smp_buf;

/****************************************************************************
 * Name: smp_transport_out_fn
 *
 * Description:
 *   SMP transmit callback for transport
 *
 *   The supplied smp_buf is always consumed, regardless of return code.
 *
 * Input Parameters:
 *   nb - The smp_buf to transmit.
 *
 * Return Value:
 *   0 on success,
 *
 *   mcumgr_err_t code on failure.
 *
 ****************************************************************************/

typedef CODE int (*smp_transport_out_fn)(FAR struct smp_buf *nb);

/****************************************************************************
 * Name: smp_transport_get_mtu_fn
 *
 * Description:
 *   SMP MTU query callback for transport
 *
 *   The supplied smp_buf should contain a request received from the peer
 *   whose MTU is being queried.  This function takes a smp_buf parameter
 *   because some transports store connection-specific information in the
 *   smp_buf user header (e.g., the BLE transport stores the peer address).
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
  FAR const struct smp_buf *nb);

/****************************************************************************
 * Name: smp_transport_ud_copy_fn
 *
 * Description:
 *   SMP copy user_data callback
 *
 *   The supplied src smp_buf should contain a user_data that cannot be
 *   copied using regular memcpy function (e.g., the BLE transport smp_buf
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

typedef CODE int (*smp_transport_ud_copy_fn)(FAR struct smp_buf *dst,
                                             FAR const struct smp_buf *src);

/****************************************************************************
 * Name: smp_transport_ud_free_fn
 *
 * Description:
 *   SMP free user_data callback
 *
 *   This function frees smp_buf user data, because some transports store
 *   connection-specific information in the smp_buf user data (e.g., the BLE
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
  FAR struct smp_buf *nb, FAR void *arg);

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/****************************************************************************
 * Name: smp_translate_error_fn
 *
 * Description:
 *   Translates a SMP version 2 error response to a legacy SMP version 1
 *   error code.
 *
 * Input Parameters:
 *   ret - The SMP version 2 group error value.
 *
 * Return Value
 *   #enum mcumgr_err_t Legacy SMP version 1 error code to return to client.
 *
 ****************************************************************************/

typedef int (*smp_translate_error_fn)(uint16_t err);
#endif

/* SMP MCUmgr protocol version, part of the SMP header */

enum smp_mcumgr_version_t
{
  /* Version 1: the original protocol */

  SMP_MCUMGR_VERSION_1 = 0,

  /* Version 2: adds more detailed error reporting capabilities */

  SMP_MCUMGR_VERSION_2,
};

/* CBOR reader state */

struct cbor_nb_reader
{
  FAR struct smp_buf *nb;

  /* CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2 translates to minimal
   * zcbor backup states.
   */

  zcbor_state_t zs[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
};

/* CBOR writer state */

struct cbor_nb_writer
{
  struct smp_buf *nb;
  zcbor_state_t zs[CONFIG_MCUMGR_SMP_CBOR_MAX_ENCODING_LEVELS + 2];

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
  uint16_t error_group;
  uint16_t error_ret;
#endif
};

/* Decodes, encodes, and transmits SMP packets. */

struct smp_streamer
{
  FAR struct smp_transport  *smpt;
  FAR struct cbor_nb_reader *reader;
  FAR struct cbor_nb_writer *writer;

#ifdef CONFIG_MCUMGR_SMP_VERBOSE_ERR_RESPONSE
  FAr const char *rc_rsn;
#endif
};

/* SMP header */

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
  FAR struct smp_buf *current;    /* smp_buf used for reassembly */
  uint16_t            expected;		/* expected bytes to come */
};
#endif

/* SMP transport object for sending SMP responses. */

struct smp_transport
{
	/* Function pointers */

	struct smp_transport_api_t functions;

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
  struct smp_transport_reassembly __reassembly;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: smp_process_request_packet
 *
 * Description:
 *   Processes a single SMP request packet and sends all corresponding responses.
 *
 *   Processes all SMP requests in an incoming packet.  Requests are processed
 *   sequentially from the start of the packet to the end.  Each response is sent
 *   individually in its own packet.  If a request elicits an error response,
 *   processing of the packet is aborted.  This function consumes the supplied
 *   request buffer regardless of the outcome.
 *
 * Input Parameters:
 *  streamer - The streamer providing the required SMP callbacks.
 *  req      - The request packet to process.
 *
 * Return Value:
 *   0 on success, #mcumgr_err_t code on failure.
 *
 ****************************************************************************/

int smp_process_request_packet(struct smp_streamer *streamer, void *req);

/****************************************************************************
 * Name: smp_add_cmd_err
 *
 * Description:
 *   Appends an "err" response
 *
 * This appends an err response to a pending outgoing response which contains a
 * result code for a specific group. Note that error codes are specific to the
 * command group they are emitted from).
 *
 * Input Parameters:
 *   zse   - The zcbor encoder to use.
 *   group - The group which emitted the error.
 *   ret   - The command result code to add.
 *
 * Return Value:
 *   true on success, false on failure (memory error).
 *
 ****************************************************************************/

bool smp_add_cmd_err(zcbor_state_t *zse, uint16_t group, uint16_t ret);

/****************************************************************************
 * Name: smp_packet_alloc
 *
 * Description:
*   Allocates a smp_buf for holding an mcumgr request or response.
*
* Return Value:
*   A newly-allocated buffer smp_buf on success;
*   NULL on failure.
*
****************************************************************************/

struct smp_buf *smp_packet_alloc(void);

/****************************************************************************
 * Name: smp_packet_free
 *
 * Description:
 *   Frees an mcumgr smp_buf
 *
 * Input Parameters:
 *   nb - The smp_buf to free.
 *
 ****************************************************************************/

void smp_packet_free(struct smp_buf *nb);

/****************************************************************************
 * Name: smp_rx_req
 *
 * Description:
 *   Enqueues an incoming SMP request packet for processing.
 *   This function always consumes the supplied smp_buf.
 *
 * Input Parameters:
 *   smtp - The transport to use to send the corresponding response(s).
 *   nb   - The request packet to process.
 *
 ****************************************************************************/

void smp_rx_req(FAR struct smp_transport *smtp, FAR struct smp_buf *nb);

/****************************************************************************
 * Name: smp_alloc_rsp
 *
 * Description:
 *   Allocates a response buffer.
 *   If a source buf is provided, its user data is copied into the new
 *   buffer.
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
 *   smpt - The transport to use.
 *   arg - Argument provided to callback smp_transport_query_valid_check_fn()
 *         function.
 *
 ****************************************************************************/

void smp_rx_remove_invalid(FAR struct smp_transport *smpt, FAR void *arg);

/****************************************************************************
 * Name: smp_rx_clear
 *
 * Description:
 *   Used to clear pending queued requests for an SMP transport.
 *
 * Input Parameters:
 *   smpt - The transport to use.
 *
 ****************************************************************************/

void smp_rx_clear(FAR struct smp_transport *smpt);

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_MGMT_MCUMGR_SMP_H */
