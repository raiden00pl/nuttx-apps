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
#include <string.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <mgmt/mcumgr/mgmt.h>
#include <mgmt/mcumgr/smp.h>
#include <mgmt/mcumgr/smp_buf.h>

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/****************************************************************************
 * Name:
 *
 * Description:
 *   Translate SMP version 2 error code to legacy SMP version 1 MCUmgr
 *   error code.
 *
 * Input Parameters:
 *   group - #mcumgr_group_t group ID
 *   err   - Group-specific error code
 *
 * Return Value:
 *   #mcumgr_err_t error code
 *
 ****************************************************************************/

static int smp_translate_error_code(uint16_t group, uint16_t err)
{
  smp_translate_error_fn translate_error_function = NULL;

  translate_error_function = mgmt_find_error_translation_function(group);

  if (translate_error_function == NULL)
    {
      return MGMT_ERR_EUNKNOWN;
    }

  return translate_error_function(err);
}
#endif

/****************************************************************************
 * Name: cbor_nb_reader_init
 ****************************************************************************/

static void cbor_nb_reader_init(FAR struct cbor_nb_reader_s *cnr,
                                FAR struct smp_buf_s *nb)
{
  cnr->nb = nb;
  zcbor_new_decode_state(cnr->zs, ARRAY_SIZE(cnr->zs), nb->data, nb->len, 1,
                         NULL, 0);
}

/****************************************************************************
 * Name: cbor_nb_writer_init
 ****************************************************************************/

static void cbor_nb_writer_init(FAR struct cbor_nb_writer_s *cnw,
                                FAR struct smp_buf_s *nb)
{
  smp_buf_reset(nb);
  cnw->nb      = nb;
  cnw->nb->len = sizeof(struct smp_hdr_s);
  zcbor_new_encode_state(cnw->zs, ARRAY_SIZE(cnw->zs),
                         nb->data + sizeof(struct smp_hdr_s),
                         smp_buf_tailroom(nb), 0);
}

/****************************************************************************
 * Name: smp_rsp_op
 *
 * Description:
 *   Converts a request opcode to its corresponding response opcode.
 *
 ****************************************************************************/

static uint8_t smp_rsp_op(uint8_t req_op)
{
  if (req_op == MGMT_OP_READ)
    {
      return MGMT_OP_READ_RSP;
    }
  else
    {
      return MGMT_OP_WRITE_RSP;
    }
}

/****************************************************************************
 * Name: smp_make_rsp_hdr
 ****************************************************************************/

static void smp_make_rsp_hdr(FAR const struct smp_hdr_s *req_hdr,
                             FAR struct smp_hdr_s *rsp_hdr, size_t len)
{
  rsp_hdr->nh_len     = sys_cpu_to_be16(len);
  rsp_hdr->nh_flags   = 0;
  rsp_hdr->nh_op      = smp_rsp_op(req_hdr->nh_op);
  rsp_hdr->nh_group   = sys_cpu_to_be16(req_hdr->nh_group);
  rsp_hdr->nh_seq     = req_hdr->nh_seq;
  rsp_hdr->nh_id      = req_hdr->nh_id;
  rsp_hdr->nh_version =
    (req_hdr->nh_version > SMP_MCUMGR_VERSION_2 ? SMP_MCUMGR_VERSION_2
     : req_hdr->nh_version);
}

/****************************************************************************
 * Name: smp_write_hdr
 ****************************************************************************/

static int smp_read_hdr(FAR const struct smp_buf_s *nb,
                        FAR struct smp_hdr_s *dst_hdr)
{
  if (nb->len < sizeof(*dst_hdr))
    {
      return MGMT_ERR_EINVAL;
    }

  memcpy(dst_hdr, nb->data, sizeof(*dst_hdr));
  dst_hdr->nh_len   = sys_be16_to_cpu(dst_hdr->nh_len);
  dst_hdr->nh_group = sys_be16_to_cpu(dst_hdr->nh_group);

  return 0;
}

/****************************************************************************
 * Name: smp_write_hdr
 ****************************************************************************/

static inline int smp_write_hdr(FAR struct smp_streamer_s *streamer,
                                FAR const struct smp_hdr_s *src_hdr)
{
  memcpy(streamer->writer->nb->data, src_hdr, sizeof(*src_hdr));
  return 0;
}

/****************************************************************************
 * Name: smp_build_err_rsp
 ****************************************************************************/

static int smp_build_err_rsp(FAR struct smp_streamer_s *streamer,
                             FAR const struct smp_hdr_s *req_hdr, int status,
                             FAR const char *rc_rsn)
{
  FAR struct cbor_nb_writer_s *nbw = streamer->writer;
  FAR zcbor_state_t           *zsp = nbw->zs;
  struct smp_hdr_s             rsp_hdr;
  bool                         ok;

  UNUSED(rc_rsn);

  ok = zcbor_map_start_encode(zsp, 2) && zcbor_tstr_put_lit(zsp, "rc")
       && zcbor_int32_put(zsp, status);

  ok &= zcbor_map_end_encode(zsp, 2);

  if (!ok)
    {
      return MGMT_ERR_EMSGSIZE;
    }

  smp_make_rsp_hdr(req_hdr, &rsp_hdr,
                   zsp->payload_mut - nbw->nb->data - MGMT_HDR_SIZE);
  nbw->nb->len = zsp->payload_mut - nbw->nb->data;
  smp_write_hdr(streamer, &rsp_hdr);

  return 0;
}

/****************************************************************************
 * Name: smp_handle_single_payload
 *
 * Description:
 *   Processes a single SMP request and generates a response payload (i.e.,
 *   everything after the management header).  On success, the response
 *   payload is written to the supplied cbuf but not transmitted.
 *   On failure, no error response gets written; the caller is expected to
 *   build an error response from the return code.
 *
 * Input Parameters:
 *   cbuf    - A cbuf containing the request and response buffer.
 *   req_hdr - The management header belonging to the incoming request
 *(host-byte order).
 *
 * Return Value:
 *   A MGMT_ERR_[...] error code.
 *
 ****************************************************************************/

static int smp_handle_single_payload(FAR struct smp_streamer_s *cbuf,
                                     FAR const struct smp_hdr_s *req_hdr)
{
  FAR const struct mgmt_group_s   *group;
  FAR const struct mgmt_handler_s *handler;
  mgmt_handler_fn                  handler_fn;
  int                              rc;

  group = mgmt_find_group(req_hdr->nh_group);
  if (group == NULL)
    {
      return MGMT_ERR_ENOTSUP;
    }

  handler = mgmt_get_handler(group, req_hdr->nh_id);
  if (handler == NULL)
    {
      return MGMT_ERR_ENOTSUP;
    }

  switch (req_hdr->nh_op)
    {
      case MGMT_OP_READ:
        handler_fn = handler->mh_read;
        break;

      case MGMT_OP_WRITE:
        handler_fn = handler->mh_write;
        break;

      default:
        return MGMT_ERR_EINVAL;
    }

  if (handler_fn)
    {
      bool ok;

      ok = zcbor_map_start_encode(
          cbuf->writer->zs, CONFIG_MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES);

      MGMT_CTXT_SET_RC_RSN(cbuf, NULL);

      if (!ok)
        {
          return MGMT_ERR_EMSGSIZE;
        }

      rc = handler_fn(cbuf);

      /* End response payload. */

      if (!zcbor_map_end_encode(cbuf->writer->zs,
                                CONFIG_MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES)
          && rc == 0)
        {
          rc = MGMT_ERR_EMSGSIZE;
        }
    }
  else
    {
      rc = MGMT_ERR_ENOTSUP;
    }

  return rc;
}

/****************************************************************************
 * Name: smp_handle_single_req
 *
 * Description:
 *   Processes a single SMP request and generates a complete response (i.e.,
 *   header and payload).  On success, the response is written using the
 *   supplied streamer but not transmitted.  On failure, no error response
 *   gets written;
 *   the caller is expected to build an error response from the return code.
 *
 * Input Parameters:
 *   streamer - The SMP streamer to use for reading the request and writing
 *              the response.
 *   req_hdr  - The management header belonging to the incoming request
 *              (host-byte order).
 *
 * Return Value:
 *   A MGMT_ERR_[...] error code.
 *
 ****************************************************************************/

static int smp_handle_single_req(FAR struct smp_streamer_s *streamer,
                                 FAR const struct smp_hdr_s *req_hdr,
                                 FAR const char **rsn)
{
  FAR struct cbor_nb_writer_s *nbw = streamer->writer;
  FAR zcbor_state_t           *zsp = nbw->zs;
  struct smp_hdr_s             rsp_hdr;
  int                          rc;

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
  nbw->error_group = 0;
  nbw->error_ret   = 0;
#else
  if (req_hdr->nh_version == SMP_MCUMGR_VERSION_1)
    {
      /* Support for the original version is excluded in this build */

      return MGMT_ERR_UNSUPPORTED_TOO_OLD;
    }
#endif

  /* We do not currently support future versions of the protocol */

  if (req_hdr->nh_version > SMP_MCUMGR_VERSION_2)
    {
      return MGMT_ERR_UNSUPPORTED_TOO_NEW;
    }

  /* Process the request and write the response payload. */

  rc = smp_handle_single_payload(streamer, req_hdr);
  if (rc != 0)
    {
      *rsn = MGMT_CTXT_RC_RSN(streamer);
      return rc;
    }

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
  /* If using the legacy protocol, translate the error code to a return code
   */

  if (nbw->error_ret != 0 && req_hdr->nh_version == 0)
    {
      rc   = smp_translate_error_code(nbw->error_group, nbw->error_ret);
      *rsn = MGMT_CTXT_RC_RSN(streamer);
      return rc;
    }
#endif

  smp_make_rsp_hdr(req_hdr, &rsp_hdr,
                   zsp->payload_mut - nbw->nb->data - MGMT_HDR_SIZE);
  nbw->nb->len = zsp->payload_mut - nbw->nb->data;
  smp_write_hdr(streamer, &rsp_hdr);

  return 0;
}

/****************************************************************************
 * Name: smp_on_err
 *
 * Description:
 *   Attempts to transmit an SMP error response.  This function consumes
 *   both supplied buffers.
 *
 * Input Parameters:
 *   streamer - The SMP streamer for building and transmitting the
 *              response.
 *   eq_hdr   - The header of the request which elicited the error.
 *   req      - The buffer holding the request.
 *   rsp      - The buffer holding the response, or NULL if none was
 *              allocated.
 *   status   - The status to indicate in the error response.
 *   rsn      - The text explanation to @status encoded as "rsn" into
 *              CBOR response.
 *
 ****************************************************************************/

static void smp_on_err(FAR struct smp_streamer_s *streamer,
                       FAR const struct smp_hdr_s *req_hdr, FAR void *req,
                       FAR void *rsp, int status, FAR const char *rsn)
{
  int rc;

  /* Prefer the response buffer for holding the error response.  If no
   * response buffer was allocated, use the request buffer instead.
   */

  if (rsp == NULL)
    {
      rsp = req;
      req = NULL;
    }

  /* Clear the partial response from the buffer, if any. */

  cbor_nb_writer_init(streamer->writer, rsp);

  /* Build and transmit the error response. */

  rc = smp_build_err_rsp(streamer, req_hdr, status, rsn);
  if (rc == 0)
    {
      streamer->smpt->functions.output(rsp);
      rsp = NULL;
    }

  /* Free any extra buffers. */

  smp_free_buf(req, streamer->smpt);
  smp_free_buf(rsp, streamer->smpt);
}

/****************************************************************************
 * Public Function
 ****************************************************************************/

/****************************************************************************
 * Name: smp_add_cmd_err
 *
 * Description:
 *   Processes all SMP requests in an incoming packet.
 *   Requests are processed sequentially from the start of the packet to
 *   the end.  Each response is sent individually in its own packet.
 *   If a request elicits an error response, processing of the packet
 *   is aborted.  This function consumes the supplied request buffer
 *   regardless of the outcome. The function will return MGMT_ERR_EOK (0)
 *   when given an empty input stream, and will also release the buffer from
 *   the stream; it does not return MTMT_ERR_ECORRUPT, or any other MGMT
 *   error, because there was no error while processing of the input stream,
 *   it is callers fault that an empty stream has been passed to the
 *   function.
 *
 * Input Parameters:
 *   streamer - The streamer to use for reading, writing, and transmitting.
 *   req      - A buffer containing the request packet.
 *
 * Return Value:
 *   0 on success or when input stream is empty;
 *
 *   MGMT_ERR_ECORRUPT if buffer starts with non SMP data header or there
 *   is not enough bytes to process header, or other MGMT_ERR_[...] code on
 *   failure.
 *
 ****************************************************************************/

int smp_process_request_packet(FAR struct smp_streamer_s *streamer,
                               FAR void *vreq)
{
  FAR struct smp_buf_s *req           = vreq;
  FAR const char       *rsn           = NULL;
  bool                  valid_hdr     = false;
  bool                  handler_found = false;
  struct smp_hdr_s      req_hdr;
  FAR void             *rsp;
  int                   rc            = 0;

  memset(&req_hdr, 0, sizeof(struct smp_hdr_s));

  rsp = NULL;

  while (req->len > 0)
    {
      handler_found = false;
      valid_hdr     = false;

      /* Read the management header and strip it from the request. */

      rc = smp_read_hdr(req, &req_hdr);
      if (rc != 0)
        {
          rc = MGMT_ERR_ECORRUPT;
          break;
        }

      valid_hdr = true;

      /* Skip the smp_hdr_s */

      smp_buf_pull(req, sizeof(struct smp_hdr_s));

      /* Does buffer contain whole message? */

      if (req->len < req_hdr.nh_len)
        {
          rc = MGMT_ERR_ECORRUPT;
          break;
        }

      if (req_hdr.nh_op == MGMT_OP_READ || req_hdr.nh_op == MGMT_OP_WRITE)
        {
          rsp = smp_alloc_rsp(req, streamer->smpt);
          if (rsp == NULL)
            {
              rc = MGMT_ERR_ENOMEM;
              break;
            }

          cbor_nb_reader_init(streamer->reader, req);
          cbor_nb_writer_init(streamer->writer, rsp);

          /* Process the request payload and build the response. */

          rc            = smp_handle_single_req(streamer, &req_hdr, &rsn);
          handler_found = (rc != MGMT_ERR_ENOTSUP);
          if (rc != 0)
            {
              break;
            }

          /* Send the response. */

          rc  = streamer->smpt->functions.output(rsp);
          rsp = NULL;
        }
#ifdef CONFIG_SMP_CLIENT
      else if ((req_hdr.nh_op == MGMT_OP_READ_RSP
                || req_hdr.nh_op == MGMT_OP_WRITE_RSP))
        {
          rc = smp_client_single_response(req, &req_hdr);

          if (rc == MGMT_ERR_EOK)
            {
              handler_found = true;
            }
          else
            {
              /* Server shuold not send error response for response */
              valid_hdr = false;
            }
        }
#endif
      else
        {
          rc = MGMT_ERR_ENOTSUP;
        }

      if (rc != 0)
        {
          break;
        }

      /* Trim processed request to free up space for subsequent responses. */

      smp_buf_pull(req, req_hdr.nh_len);

    }

  if (rc != 0 && valid_hdr)
    {
      smp_on_err(streamer, &req_hdr, req, rsp, rc, rsn);

      return rc;
    }

  smp_free_buf(req, streamer->smpt);
  smp_free_buf(rsp, streamer->smpt);

  return rc;
}

/****************************************************************************
 * Name: smp_add_cmd_err
 ****************************************************************************/

bool smp_add_cmd_err(FAR zcbor_state_t *zse, uint16_t group, uint16_t ret)
{
  FAR struct cbor_nb_writer_s *container;
  bool                         ok = true;

  if (ret != 0)
    {
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
      container = container_of(zse, struct cbor_nb_writer_s, zs[0]);

      container->error_group = group;
      container->error_ret   = ret;
#endif

      ok = zcbor_tstr_put_lit(zse, "err") && zcbor_map_start_encode(zse, 2)
           && zcbor_tstr_put_lit(zse, "group")
           && zcbor_uint32_put(zse, (uint32_t)group)
           && zcbor_tstr_put_lit(zse, "rc")
           && zcbor_uint32_put(zse, (uint32_t)ret)
           && zcbor_map_end_encode(zse, 2);
    }

  return ok;
}
