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

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_serial_init
 ****************************************************************************/

int foc_serial_init(FAR struct foc_serial_s *s, FAR const char *devpath)
{
  int            ret = OK;
  struct termios tio;

  /* Open serial device */

  s->fd = open(devpath, O_RDWR);
  if (s->fd < 0)
    {
      FOCLIBERR("ERROR Failed to open %s %d\n", devpath, errno);
      goto errout;
    }

  /* Configure serial device */

  /* Fill the termios struct with the current values. */

  ret = tcgetattr(s->fd, &tio);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: getting attributes: %d\n", errno);
      goto errout;
    }

  /* Configure a baud rate */

  ret = cfsetspeed(&tio, FOC_SERIAL_BAUD);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: setting baud rate: %d\n", errno);
      goto errout;
    }

  /* Make "raw" mode */

  cfmakeraw(&tio);

  /* Change the attributes now */

  ret = tcsetattr(s->fd, TCSANOW, &tio);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: setting attributes: %d\n", errno);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_serial_deinit
 ****************************************************************************/

int foc_serial_deinit(FAR struct foc_serial_s *s)
{
  if (s->fd)
    {
      close(s->fd);
    }

  return OK;
}


/****************************************************************************
 * Name: foc_serial_checksum
 ****************************************************************************/

uint8_t foc_serial_checksum(FAR const uint8_t *data, int framelen)
{
  uint8_t checksum = 0x00;
  int     i        = 0;

  for (i = 0; i < (framelen - 1); i+=1)
    {
      checksum ^= data[i];
    }

  /* Return the complement of the resulting checksum */

  checksum ^= 0xff;

  return checksum;
}

/****************************************************************************
 * Name: foc_serial_blocking
 ****************************************************************************/

int foc_serial_blocking(FAR struct foc_serial_s *s, bool blocking)
{
  int flags = 0;

  flags = fcntl(s->fd, F_GETFL, 0);

  if (blocking == true)
    {
      flags |= O_NONBLOCK;
    }
  else
    {
      flags &= ~O_NONBLOCK;
    }

  return fcntl(s->fd, F_SETFL, flags);
}

/****************************************************************************
 * Name: foc_serial_read
 ****************************************************************************/

int foc_serial_read(FAR struct foc_serial_s *s)
{
  int     ret      = OK;
  uint8_t checksum = 0;

  /* Read data */

  ret = read(s->fd, s->buffer, READ_BUFFER_SIZE);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: fd read failed %d\n", errno);
      ret = -errno;
      goto errout;
    }

  /* No data */

  if (ret == 0)
    {
      goto errout;
    }

  /* Validate the checksum */

  checksum = foc_serial_checksum(s->buffer, (ret-1));
  if (checksum != s->buffer[ret - 1])
    {
      FOCLIBERR("ERROR: drop frame\n");
      ret = -EINVAL;
      goto errout;
    }

errout:
  return ret;
}
