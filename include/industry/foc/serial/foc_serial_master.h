/****************************************************************************
 * apps/include/industry/foc/serial/foc_serial_master.h
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

#ifndef __APPS_INCLUDE_INDUSTRY_FOC_SERIAL_FOC_SERIAL_MASTER_H
#define __APPS_INCLUDE_INDUSTRY_FOC_SERIAL_FOC_SERIAL_MASTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "industry/foc/serial/foc_serial.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/*  */

struct foc_serial_master_s
{
  struct foc_serial_s ser;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: foc_serial_master_init
 ****************************************************************************/

int foc_serial_master_init(FAR struct foc_serial_master_s *s,
                           FAR const char *devpath);

/****************************************************************************
 * Name: foc_serial_master_deinit
 ****************************************************************************/

int foc_serial_master_deinit(FAR struct foc_serial_master_s *s);

#endif /* __APPS_INCLUDE_INDUSTRY_FOC_SERIAL_FOC_SERIAL_MASTER_H */
