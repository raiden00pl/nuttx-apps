/****************************************************************************
 * apps/industry/foc/foc_serial.c
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

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#include "industry/foc/foc_log.h"
#include "industry/foc/foc_utils.h"
#include "industry/foc/serial/foc_serial.h"
#include "industry/foc/serial/foc_serial_slave.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_serial_slave_init
 ****************************************************************************/

int foc_serial_slave_init(FAR struct foc_serial_slave_s *s,
                          FAR const char *devpath)
{
  /* Reset data */

  memset(s, 0, sizeof(struct foc_serial_slave_s));

  /* Initialize serial */

  return foc_serial_init(&s->ser, devpath);
}

/****************************************************************************
 * Name: foc_serial_slave_deinit
 ****************************************************************************/

int foc_serial_slave_deinit(FAR struct foc_serial_slave_s *s)
{
  return foc_serial_deinit((FAR struct foc_serial_s *)s);
}

/****************************************************************************
 * Name: foc_serial_send_state
 ****************************************************************************/

int foc_serial_send_state(FAR struct foc_serial_slave_s *s,
                          FAR struct foc_state_s *state)
{
  struct foc_serial_state_s frame;

  /* Copy currents */

  memcpy(&frame.state, state, sizeof(struct foc_state_s));

  /* Frame type and checksum */

  frame.type     = FOC_SERIAL_TYPE_STATE;
  frame.checksum = foc_serial_checksum((FAR const uint8_t *)&frame,
                                       sizeof(struct foc_serial_state_s));

  /* Write data */

  return write(s->ser.fd, &frame, sizeof(struct foc_serial_state_s));
}

/****************************************************************************
 * Name: foc_serial_send_statebus
 ****************************************************************************/

int foc_serial_send_statebus(FAR struct foc_serial_slave_s *s,
                             FAR struct foc_state_s *state)
{
  struct foc_serial_statebus_s frame;

  /* Copy currents */

  memcpy(&frame.state, state, sizeof(struct foc_state_s));

  /* Copy bus state */

  frame.vbus = 0;
  frame.ibus = 0;

  /* Frame type and checksum */

  frame.type     = FOC_SERIAL_TYPE_STATEBUS;
  frame.checksum = foc_serial_checksum((FAR const uint8_t *)&frame,
                                       sizeof(struct foc_serial_statebus_s));

  /* Write data */

  return write(s->ser.fd, &frame, sizeof(struct foc_serial_statebus_s));
}

/****************************************************************************
 * Name: foc_serial_send_info
 ****************************************************************************/

int foc_serial_send_info(FAR struct foc_serial_slave_s *s,
                         FAR struct foc_info_s *info)
{
  struct foc_serial_info_s frame;

  /* Copy currents */

  memcpy(&frame, info, sizeof(struct foc_info_s));

  /* Frame type and checksum */

  frame.type     = FOC_SERIAL_TYPE_INFO;
  frame.checksum = foc_serial_checksum((FAR const uint8_t *)&frame,
                                       sizeof(struct foc_serial_info_s));

  /* Write data */

  return write(s->ser.fd, &frame, sizeof(struct foc_serial_info_s));

}

/****************************************************************************
 * Name: foc_serial_read_params
 ****************************************************************************/

static int foc_serial_read_params(FAR struct foc_serial_slave_s *s,
                                  FAR struct foc_params_s *params)
{
  FAR struct foc_serial_params_s *frame = NULL;
  int                             ret   = OK;

  /* Read data from serial */

  ret = foc_serial_read(&s->ser);
  if (ret < 0)
    {
      goto errout;
    }

  /* Only FOC parameter frames are expected here */

  if (s->ser.buffer[0] != FOC_SERIAL_TYPE_PARAMS)
    {
      FOCLIBERR("ERROR: invalid frame\n");
      ret = -EINVAL;
      goto errout;
    }

  /* Get parameters */

  frame = (FAR struct foc_serial_params_s *) s->ser.buffer;
  memcpy(params, &frame->params, sizeof(struct foc_params_s));

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_serial_slave_handle
 ****************************************************************************/

int foc_serial_slave_handle(FAR struct foc_serial_slave_s *s)
{
  int ret = OK;

  /* Non blocking read */

  ret = foc_serial_blocking(&s->ser, false);
  if (ret < 0)
    {
      goto errout;
    }

  /* Read data from serial */

  ret = foc_serial_read(&s->ser);
  if (ret < 0)
    {
      goto errout;
    }

  /* Handle request */

  switch (s->ser.buffer[0])
    {
      case FOC_SERIAL_TYPE_START:
        {
          s->start = true;

          break;
        }

      case FOC_SERIAL_TYPE_STOP:
        {
          s->stop = true;

          break;
        }

      case FOC_SERIAL_TYPE_GETINFO:
        {
          /* TODO */

          break;
        }

      case FOC_SERIAL_TYPE_SETCFG:
        {
          /* TODO */

          break;
        }

      case FOC_SERIAL_TYPE_GETCFG:
        {
          /* TODO */

          break;
        }

      default:
        {
          FOCLIBERR("ERROR: unknown frame type %d\n", s->ser.buffer[0]);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_serial_slave_exchange
 ****************************************************************************/

int foc_serial_slave_exchange(FAR struct foc_serial_slave_s *s,
                              FAR struct foc_state_s *state,
                              FAR struct foc_params_s *params)
{
  int ret = OK;

  /* Blocking read */

  ret = foc_serial_blocking(&s->ser, true);
  if (ret < 0)
    {
      goto errout;
    }

  /* Send the FOC device state */

  ret = foc_serial_send_state(s, state);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: foc_serial_params failed %d\n", ret);
      goto errout;
    }

  /* Wait for the FOC device parameters - blocking */

  ret = foc_serial_read_params(s, params);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: foc_serial_read failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_serial_slave_send_ack
 ****************************************************************************/

int foc_serial_slave_send_ack(FAR struct foc_serial_slave_s *s, bool ack)
{
  struct foc_serial_ack_s frame;

  /* Copy state */

  frame.ack = ack;

  /* Frame type and checksum */

  frame.type     = FOC_SERIAL_TYPE_ACK;
  frame.checksum = foc_serial_checksum((FAR const uint8_t *)&frame,
                                       sizeof(struct foc_serial_ack_s));

  /* Write data */

  return write(s->ser.fd, &frame, sizeof(struct foc_serial_ack_s));
}

/****************************************************************************
 * Name: foc_serial_send_info
 ****************************************************************************/

int foc_serial_slave_send_info(FAR struct foc_serial_slave_s *s,
                               FAR struct foc_info_s *info)
{
  struct foc_serial_info_s frame;

  /* Copy data */

  memcpy(&frame.info, info, sizeof(struct foc_info_s));

  /* Frame type and checksum */

  frame.type     = FOC_SERIAL_TYPE_INFO;
  frame.checksum = foc_serial_checksum((FAR const uint8_t *)&frame,
                                       sizeof(struct foc_serial_info_s));

  /* Write data */

  return write(s->ser.fd, &frame, sizeof(struct foc_serial_info_s));
}
