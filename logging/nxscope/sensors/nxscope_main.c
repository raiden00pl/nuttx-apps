/****************************************************************************
 * apps/logging/nxscope/sensors/nxscope_main.c
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

#include <sys/boardctl.h>
#include <sys/param.h>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>

#include <nuttx/sensors/sensor.h>

#include "logging/nxscope/nxscope.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SENSOR_PATH        "/dev/uorb/"
#define SENSOR_PATH_MAX    62

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

struct nxs_sensor_info_s
{
  FAR const char *name;
  size_t          data_size;    /* Read data size */
  size_t          data_offset;  /* Read data offset without timestamp */
  size_t          dim;          /* Data vector dimenstion */
  int             dtype;        /* Data vector type */
};

struct sensor_object_s
{
  FAR struct nxs_sensor_info_s *info;
  int                           chanid;
};

struct listen_object_s
{
  struct list_node       node;               /* Node of object info list */
  struct sensor_object_s sensor;             /*  */
  int                    fd;                 /*  */
  FAR uint8_t           *data;               /*  */
};

struct nxscope_thr_env_s
{
  struct nxscope_s nxs;
  struct list_node objlist;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct nxs_sensor_info_s g_nxsensor[] =
{
  {"accel", sizeof(struct sensor_accel), 0, 3, NXSCOPE_TYPE_FLOAT},
  {"mag", sizeof(struct sensor_mag), 0, 3, NXSCOPE_TYPE_FLOAT},
  {"gyro", sizeof(struct sensor_gyro), 0, 3, NXSCOPE_TYPE_FLOAT},
  {"light", sizeof(struct sensor_light), 0, 2, NXSCOPE_TYPE_FLOAT},
  {"baro", sizeof(struct sensor_baro), 0, 2, NXSCOPE_TYPE_FLOAT},
  {"prox", sizeof(struct sensor_prox), 0, 1, NXSCOPE_TYPE_FLOAT},
  {"humi", sizeof(struct sensor_humi), 0, 1, NXSCOPE_TYPE_FLOAT},
  {"temp", sizeof(struct sensor_temp), 0, 1, NXSCOPE_TYPE_FLOAT},
  {"rgb", sizeof(struct sensor_rgb), 0, 3, NXSCOPE_TYPE_FLOAT},
  {"hall", sizeof(struct sensor_hall), 0, 1, NXSCOPE_TYPE_INT32},
  {"ir", sizeof(struct sensor_ir), 0, 1, NXSCOPE_TYPE_FLOAT},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_samples_thr
 ****************************************************************************/

static FAR void *nxscope_samples_thr(FAR void *arg)
{
  FAR struct nxscope_thr_env_s *envp   = arg;
  FAR struct listen_object_s   *tmp;
  FAR float                    *data   = NULL;
  int                           ret    = OK;
  size_t                        offset = 0;

  DEBUGASSERT(envp);

  printf("nxscope_samples_thr\n");

  while (1)
    {

      list_for_every_entry(&envp->objlist, tmp, struct listen_object_s, node)
        {
          /* Read data */

          ret = read(tmp->fd, tmp->data, tmp->sensor.info->data_size);
          if (ret < 0)
            {
              printf("ERROR: read failed %d\n", -errno);
            }
          else
            {
              /* Get vector to send */

              offset = tmp->sensor.info->data_offset + sizeof(uint64_t);
              data = (float *)&tmp->data[offset];
              nxscope_put_vfloat(&envp->nxs, tmp->sensor.chanid, data,
                                 tmp->sensor.info->dim);
            }
        }

      usleep(CONFIG_NXSCOPE_SENSORS_FETCH_INTERVAL);
    }

  return NULL;
}

#ifdef CONFIG_NXSCOPE_SENSORS_CDCACM
/****************************************************************************
 * Name: nxscope_cdcacm_init
 ****************************************************************************/

static int nxscope_cdcacm_init(void)
{
  struct boardioc_usbdev_ctrl_s  ctrl;
  FAR void                      *handle;
  int                            ret = OK;

  ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.handle   = &handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("ERROR: BOARDIOC_USBDEV_CONTROL failed %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: listener_add_object
 ****************************************************************************/

static int listener_add_object(FAR struct list_node *objlist,
                               FAR struct sensor_object_s *sensor,
                               int fd)
{
  FAR struct listen_object_s *tmp;

  tmp = malloc(sizeof(struct listen_object_s));
  if (tmp == NULL)
    {
      return -ENOMEM;
    }

  /* Copy data */

  memcpy(&tmp->sensor, sensor, sizeof(*sensor));
  tmp->fd = fd;

  /* Allocate space for data */

  tmp->data = malloc(sensor->info->data_size);

  list_add_tail(objlist, &tmp->node);
  return 0;
}

/****************************************************************************
 * Name: listener_delete_object_list
 ****************************************************************************/

static void listener_delete_object_list(FAR struct list_node *objlist)
{
  FAR struct listen_object_s *tmp;
  FAR struct listen_object_s *next;

  list_for_every_entry_safe(objlist, tmp, next, struct listen_object_s, node)
    {
      free(tmp->data);
      list_delete(&tmp->node);
      free(tmp);
    }

  list_initialize(objlist);
}

/****************************************************************************
 * Name: nxscope_sensor_chinfo
 ****************************************************************************/

static int nxscope_sensor_chinfo(FAR char *path,
                                 FAR struct nxs_sensor_info_s **info)
{
  int i = 0;

  for (i = 0; i < nitems(g_nxsensor); i++)
    {
      if (strstr(path, g_nxsensor[i].name) != NULL)
        {
          *info = &g_nxsensor[i];
          return OK;
        }
    }

  return -EINVAL;
}

/****************************************************************************
 * Name: nxscope_channels_num
 ****************************************************************************/

static int nxscope_channels_num(void)
{
  FAR struct dirent *entry = NULL;
  FAR DIR           *dir   = NULL;
  int                i     = 0;

  dir = opendir(SENSOR_PATH);
  if (!dir)
    {
      return 0;
    }

  while ((entry = readdir(dir)) != NULL)
    {
      if (entry->d_type == DT_CHR)
        {
          i++;
        }
    }

  closedir(dir);

  return i;
}

/****************************************************************************
 * Name: nxscope_channels
 ****************************************************************************/

static int nxscope_channels(FAR struct nxscope_thr_env_s *envp)
{
  union nxscope_chinfo_type_u  u;
  struct sensor_object_s       sensor;
  FAR struct dirent           *entry  = NULL;
  FAR DIR                     *dir    = NULL;
  int                          chanid = 0;
  int                          ret    = OK;
  int                          fd     = 0;
  char path[SENSOR_PATH_MAX];

  /* Initialize objects list */

  list_initialize(&envp->objlist);

  /* Open sensors direcotry */

  dir = opendir(SENSOR_PATH);
  if (!dir)
    {
      return 0;
    }

  /* Get sensors */

  while ((entry = readdir(dir)) != NULL)
    {
      if (entry->d_type != DT_CHR)
        {
          continue;
        }

      /* Get sensor info */

      ret = nxscope_sensor_chinfo(entry->d_name, &sensor.info);
      if (ret != OK)
        {
          printf("ERROR: not supported sensor %s\n", entry->d_name);
        }
      else
        {
          snprintf(path, SENSOR_PATH_MAX, SENSOR_PATH"%s", entry->d_name);
          fd = open(path, O_CLOEXEC | O_RDWR);
          if (fd < 0)
            {
              printf("ERROR: failed to open %s %d\n", entry->d_name, -errno);
            }
          else
            {
              /* Register channel */

              u.s.dtype = NXSCOPE_TYPE_FLOAT;
              u.s._res  = 0;
              u.s.cri   = 0;
#warning TODO: fix channel name, need allocate mem and copy
              nxscope_chan_init(&envp->nxs, chanid,
                                basename(entry->d_name),
                                u.u8, sensor.info->dim, 0);

              /* Add object */

              sensor.chanid = chanid;
              listener_add_object(&envp->objlist, &sensor, fd);

              /* Next channel */

              chanid++;
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int                         ret = OK;
  pthread_t                   thread;
  struct nxscope_thr_env_s    env;
  struct nxscope_cfg_s        nxs_cfg;
  struct nxscope_intf_s       intf;
  struct nxscope_proto_s      proto;
  struct nxscope_ser_cfg_s    nxs_ser_cfg;

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#  ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#  endif
#endif

#ifdef CONFIG_NXSCOPE_SENSORS_CDCACM
  /* Initialize the USB CDCACM device */

  ret = nxscope_cdcacm_init();
  if (ret < 0)
    {
      printf("ERROR: nxscope_cdcacm_init failed %d\n", ret);
      goto errout_noproto;
    }
#endif

  /* Default serial protocol */

  ret = nxscope_proto_ser_init(&proto, NULL);
  if (ret < 0)
    {
      printf("ERROR: nxscope_proto_ser_init failed %d\n", ret);
      goto errout_noproto;
    }

  /* Configuration */

  nxs_ser_cfg.path     = CONFIG_NXSCOPE_SENSORS_SERIAL_PATH;
  nxs_ser_cfg.nonblock = true;
  nxs_ser_cfg.baud     = CONFIG_NXSCOPE_SENSORS_SERIAL_BAUD;

  /* Initialize serial interface */

  ret = nxscope_ser_init(&intf, &nxs_ser_cfg);
  if (ret < 0)
    {
      printf("ERROR: nxscope_ser_init failed %d\n", ret);
      goto errout_nointf;
    }

  /* Initialize nxscope */

  nxs_cfg.intf_cmd      = &intf;
  nxs_cfg.intf_stream   = &intf;
  nxs_cfg.proto_cmd     = &proto;
  nxs_cfg.proto_stream  = &proto;
  nxs_cfg.callbacks     = NULL;
  nxs_cfg.channels      = nxscope_channels_num();
  nxs_cfg.streambuf_len = CONFIG_NXSCOPE_SENSORS_STREAMBUF_LEN;
  nxs_cfg.rxbuf_len     = CONFIG_NXSCOPE_SENSORS_RXBUF_LEN;
  nxs_cfg.rx_padding    = CONFIG_NXSCOPE_SENSORS_RX_PADDING;

  ret = nxscope_init(&env.nxs, &nxs_cfg);
  if (ret < 0)
    {
      printf("ERROR: nxscope_init failed %d\n", ret);
      goto errout_nonxscope;
    }

  /* Create channels */

  ret = nxscope_channels(&env);
  if (ret != OK)
    {
      printf("ERROR: nxscope_channels failed %d\n", ret);
      goto errout;
    }

  /* Create samples thread */

  ret = pthread_create(&thread, NULL, nxscope_samples_thr, &env);
  if (ret != OK)
    {
      printf("ERROR: pthread_create failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_NXSCOPE_SENSORS_FORCE_ENABLE
  /* Enable channels and enable stream */

  nxscope_chan_all_en(&nxs, true);
  nxscope_stream_start(&nxs, true);
#endif

  /* Main loop */

  while (1)
    {
      /* Flush stream data */

      ret = nxscope_stream(&env.nxs);
      if (ret < 0)
        {
          printf("ERROR: nxscope_stream failed %d\n", ret);
        }

      /* Handle recv data */

      ret = nxscope_recv(&env.nxs);
      if (ret < 0)
        {
          printf("ERROR: nxscope_recv failed %d\n", ret);
        }

      usleep(CONFIG_NXSCOPE_SENSORS_MAIN_INTERVAL);
    }

errout:

  /* Delete objects */

  listener_delete_object_list(&env.objlist);

  /* Deinit nxscope */

  nxscope_deinit(&env.nxs);

errout_nonxscope:

  /* Deinit interface */

  nxscope_ser_deinit(&intf);

errout_nointf:

  /* Deinit protocol */

  nxscope_proto_ser_deinit(&proto);

errout_noproto:

  return 0;
}
