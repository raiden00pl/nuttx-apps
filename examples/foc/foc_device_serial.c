/****************************************************************************
 * apps/examples/foc/foc_device.c
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
#include <assert.h>

#include <nuttx/fs/fs.h>

#include <nuttx/motor/foc/foc.h>

#include "foc_debug.h"
#include "foc_device.h"

#include "industry/foc/serial/foc_serial_master.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* TODO: move to kconfig */
#define CONFIG_EXAMPLES_FOC_SERIAL_DEVPATH "/dev/ttyUSB0"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_device_init
 ****************************************************************************/

int foc_device_init(FAR struct foc_device_s *dev, int id)
{
  return focserial_serial_init(dev->s, CONFIG_EXAMPLES_FOC_SERIAL_DEVPATH);
}

/****************************************************************************
 * Name: foc_device_deinit
 ****************************************************************************/

int foc_device_deinit(FAR struct foc_device_s *dev)
{
  return focserial_serial_deinit(dev->s);
}

/****************************************************************************
 * Name: foc_device_start
 ****************************************************************************/

int foc_device_start(FAR struct foc_device_s *dev, bool state)
{
  return OK;
}

/****************************************************************************
 * Name: foc_dev_state_get
 ****************************************************************************/

int foc_dev_state_get(FAR struct foc_device_s *dev)
{
  struct focserial_state_s state;
  int                     ret = OK;
  int                     i   = 0;

  /* Read data */

  ret = read(dev->fd, &state, sizeof(struct focserial_state_s));
  if (ret < 0)
    {
      printf("ERROR: fd read failed %d\n", ret);
      goto errout;
    }

  /* Verify data */

  if (ret != sizeof(struct focserial_state_s))
    {
      ret = -EINVAL;
      goto errout;
    }

  if (state.sof != 0x55)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Get data */

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      dev->state.curr[i] = state.curr[i];
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_dev_params_set
 ****************************************************************************/

int foc_dev_params_set(FAR struct foc_device_s *dev)
{
  struct focserial_params_s params;
  int                      ret = OK;
  int                      i   = 0;

  /* Get data */

  params.sof = 0x55;

  for (i = 0; i < CONFIG_MOTOR_FOC_PHASES; i += 1)
    {
      params.duty[i] = dev->params.duty[i];
    }

  /* Write data */

  ret = write(dev->fd, &params, sizeof(struct focserial_params_s));
  if (ret < 0)
    {
      printf("ERROR: fd write failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_dev_state_handle
 ****************************************************************************/

int foc_dev_state_handle(FAR struct foc_device_s *dev, FAR bool *flag)
{
  return OK;
}
