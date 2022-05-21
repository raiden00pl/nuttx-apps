/****************************************************************************
 * apps/examples/focserial/focserial_main.c
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
#include <sys/boardctl.h>

#include <nuttx/fs/fs.h>

#include <nuttx/motor/foc/foc.h>

#include "focserial_device.h"

#include "industry/foc/serial/foc_serial_slave.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Name: focserial_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct foc_serial_slave_s ser;
  struct foc_device_s       foc;
  int                       ret     = OK;
  bool                      started = false;

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  /* Reset data */

  memset(&ser, 0, sizeof(struct foc_serial_slave_s));
  memset(&foc, 0, sizeof(struct foc_device_s));

  /* Initialize serial comm */

  ret = foc_serial_slave_init(&ser, CONFIG_EXAMPLES_FOCSERIAL_SERIAL_DEVPATH);
  if (ret < 0)
    {
      printf("ERROR: foc_serial_init failed %d\n", ret);
      goto errout;
    }

  /* Initialize FOC device */

  ret = foc_device_init(&foc, 0);
  if (ret < 0)
    {
      printf("ERROR: foc_device_init failed %d\n", ret);
      goto errout;
    }


  /* Main loop */

  while (1)
    {

      /* Handle FOC slave requests */

      ret = foc_serial_slave_handle(&ser);
      if (ret < 0)
        {
          printf("ERROR: foc_serial_slave_handle failed %d\n", ret);
          goto errout;
        }

      /* Start request */

      if (ser.start == true)
        {
          /* Start the FOC device */

          ret = foc_device_start(&foc, true);
          if (ret < 0)
            {
              printf("ERROR: foc_device_start failed %d!\n", ret);
              goto errout;
            }

          /* ACK */

          ret = foc_serial_slave_send_ack(&ser, true);
          if (ret < 0)
            {
              printf("ERROR: foc_serial_slave_send_ack failed %d!\n", ret);
              goto errout;
            }

          ser.start = false;
          started   = true;
        }

      /* Stop request */

      if (ser.stop == true)
        {
          /* Stop the FOC device */

          ret = foc_device_start(&foc, false);
          if (ret < 0)
            {
              printf("ERROR: foc_device_start failed %d!\n", ret);
              goto errout;
            }

          /* ACK */

          ret = foc_serial_slave_send_ack(&ser, true);
          if (ret < 0)
            {
              printf("ERROR: foc_serial_slave_send_ack failed %d!\n", ret);
              goto errout;
            }

          ser.stop = false;
          started  = false;
        }

      /* Info request */

      if (ser.info == true)
        {
          /* Send info */

          ret = foc_serial_slave_send_info(&ser, &foc.info);
          if (ret < 0)
            {
              printf("ERROR: foc_serial_slave_send_info failed %d!\n", ret);
              goto errout;
            }

          ser.info = false;
        }

      /* Config request */

      if (ser.config == true)
        {
          /* TODO */

          /* ACK */

          ret = foc_serial_slave_send_ack(&ser, true);
          if (ret < 0)
            {
              printf("ERROR: foc_serial_slave_send_ack failed %d!\n", ret);
              goto errout;
            }


          ser.config = false;
        }

      /* The FOC device is running */

      if (started == true)
        {
          /* Get FOC device state */

          ret = foc_dev_state_get(&foc);
          if (ret < 0)
            {
              printf("ERROR: foc_devie_state_get failed %d\n", ret);
              goto errout;
            }

          /* Exchange data with the master device */

          ret = foc_serial_slave_exchange(&ser, &foc.state, &foc.params);
          if (ret < 0)
            {
              printf("ERROR: foc_serial_params failed %d\n", ret);
              goto errout;
            }

          /* Set FOC device parameters */

          ret = foc_dev_params_set(&foc);
          if (ret < 0)
            {
              printf("ERROR: foc_devie_state_get failed %d\n", ret);
              goto errout;
            }
        }
    }

errout:

  /* Deinit FOC device */

  ret = foc_device_deinit(&foc);
  if (ret < 0)
    {
      printf("ERROR: foc_device_deinit failed %d\n", ret);
    }

  /* Deinit serial device */

  ret = foc_serial_slave_deinit(&ser);
  if (ret < 0)
    {
      printf("ERROR: foc_serial_deinit failed %d\n", ret);
    }

  return 0;
}
