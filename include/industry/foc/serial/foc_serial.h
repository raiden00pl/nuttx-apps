/****************************************************************************
 * apps/include/industry/foc/serial/foc_serial.h
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
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_INDUSTRY_FOC_SERIAL_FOC_SERIAL_H
#define __APPS_INCLUDE_INDUSTRY_FOC_SERIAL_FOC_SERIAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#include <nuttx/fs/fs.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_SERIAL_TERMIOS
#  error
#endif

#define FOC_SERIAL_BAUD B2000000

#define READ_BUFFER_SIZE (64)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* */

enum foc_serial_type_e
{
  FOC_SERIAL_TYPE_INVAL    = 0x00, /* Invalid */

  /* Frames from the slave device */

  FOC_SERIAL_TYPE_STATE    = 0x01, /* FOC state */
  FOC_SERIAL_TYPE_STATEBUS = 0x02, /* FOC state + bus state */
  FOC_SERIAL_TYPE_INFO     = 0x03, /* FOC info */

  /* Frames from the master device */

  FOC_SERIAL_TYPE_START    = 0xf1, /* */
  FOC_SERIAL_TYPE_STOP     = 0xf2, /* */
  FOC_SERIAL_TYPE_GETINFO  = 0xf3, /* */
  FOC_SERIAL_TYPE_SETCFG   = 0xf4, /* */
  FOC_SERIAL_TYPE_GETCFG   = 0xf5, /* */
  FOC_SERIAL_TYPE_PARAMS   = 0xf6, /* */
  FOC_SERIAL_TYPE_ACK      = 0xf7, /* */

};


/*  */

begin_packed_struct struct foc_serial_state_s
{
  uint8_t                type;
  struct foc_state_s     state;
  uint8_t                checksum;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_params_s
{
  uint8_t                type;
  struct foc_params_s    params;
  uint8_t                checksum;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_statebus_s
{
  uint8_t                type;
  struct foc_state_s     state;
  b16_t                  vbus;
  b16_t                  ibus;
  uint8_t                checksum;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_simple_s
{
  uint8_t                type;
  uint8_t                checksum;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_ack_s
{
  uint8_t                type;
  uint8_t                ack;
  uint8_t                checksum;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_slave_info_s
{
  uint8_t                phases;
  uint8_t                duty_type;
  uint8_t                curr_type;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_info_s
{
  uint8_t                 type;
  struct foc_info_s       info;
  uint8_t                 checksum;
} end_packed_struct;

/*  */

begin_packed_struct struct foc_serial_cfg_s
{
  uint8_t                type;
  struct foc_cfg_s       foc_cfg;
  uint8_t                bus_pres;
  uint8_t                checksum;
} end_packed_struct;

/*  */

struct foc_serial_s
{
  int     fd;
  uint8_t buffer[READ_BUFFER_SIZE];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_serial_init
 ****************************************************************************/

int foc_serial_init(FAR struct foc_serial_s *s, FAR const char *devpath);

/****************************************************************************
 * Name: foc_serial_deinit
 ****************************************************************************/

int foc_serial_deinit(FAR struct foc_serial_s *ser);

/****************************************************************************
 * Name: foc_serial_checksum
 ****************************************************************************/

uint8_t foc_serial_checksum(FAR const uint8_t *data, int framelen);

/****************************************************************************
 * Name: foc_serial_blocking
 ****************************************************************************/

int foc_serial_blocking(FAR struct foc_serial_s *s, bool blocking);

/****************************************************************************
 * Name: foc_serial_read
 ****************************************************************************/

int foc_serial_read(FAR struct foc_serial_s *s);

#endif /* __APPS_INCLUDE_INDUSTRY_FOC_SERIAL_FOC_SERIAL_H */
