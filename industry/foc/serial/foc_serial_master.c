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
#include "industry/foc/serial/foc_serial_master.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_serial_master_init
 ****************************************************************************/

int foc_serial_master_init(FAR struct foc_serial_master_s *s,
                           FAR const char *devpath)
{
  /* Reset data */

  memset(s, 0, sizeof(struct foc_serial_master_s));

  /* Initialize serial */

  return foc_serial_init(&s->ser, devpath);
}

/****************************************************************************
 * Name: foc_serial_master_deinit
 ****************************************************************************/

int foc_serial_master_deinit(FAR struct foc_serial_master_s *s)
{
  return foc_serial_deinit((FAR struct foc_serial_s *)s);
}

/****************************************************************************
 * Name: foc_serial_send_params
 ****************************************************************************/

int foc_serial_send_params(FAR struct foc_serial_slave_s *s,
                           FAR struct foc_params_s *params)
{
  struct foc_serial_params_s frame;
  int                        i = 0;

  /* Copy currents */

  memcpy(&frame.params, params, sizeof(struct foc_params_s));

  /* Frame type and checksum */

  frame.type     = FOC_SERIAL_TYPE_PARAMS;
  frame.checksum = foc_serial_checksum((FAR const uint8_t *)&frame,
                                       sizeof(struct foc_serial_params_s));

  /* Write data */

  return write(s->ser.fd, &frame, sizeof(struct foc_serial_params_s));
}

/****************************************************************************
 * Name: foc_serial_mastger_handle
 ****************************************************************************/

int foc_serial_master_handle(FAR struct foc_serial_slave_s *s)
{

}

