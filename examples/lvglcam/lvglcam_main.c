/****************************************************************************
 * apps/examples/lvglcam/lvglcam_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/utsname.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/video.h>

#ifdef CONFIG_SENSORS
#  include <nuttx/sensors/ioctl.h>
#  include <nuttx/uorb.h>
#endif

#ifdef CONFIG_NET
#  include <arpa/inet.h>
#  include <net/if.h>
#  include <netinet/in.h>
#  include "netutils/netlib.h"
#endif

#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAM_WIDTH          CONFIG_EXAMPLES_LVGLCAM_WIDTH
#define CAM_HEIGHT         CONFIG_EXAMPLES_LVGLCAM_HEIGHT
#define CAM_BYPP           2                /* RGB565 */
#define CAM_STRIDE         (CAM_WIDTH * CAM_BYPP)
#define CAM_FRAMESIZE      (CAM_STRIDE * CAM_HEIGHT)
#define CAM_NBUFFERS       3
#define CAM_ERRORS_MAX     10               /* Capture errors in a row */
#define CAM_RETRY_US       10000
#define CAM_STOP_MS        10               /* DQBUF cancel period */

#define UI_OVERLAY         CONFIG_EXAMPLES_LVGLCAM_UI_OVERLAY
#define VIEW_OVERLAY       CONFIG_EXAMPLES_LVGLCAM_VIEW_OVERLAY
#define FRAME_OVERLAY      CONFIG_EXAMPLES_LVGLCAM_FRAME_OVERLAY

#if CAM_WIDTH % 8 != 0
#  error "CONFIG_EXAMPLES_LVGLCAM_WIDTH must be a multiple of 8"
#endif

#if UI_OVERLAY == VIEW_OVERLAY || \
    (UI_OVERLAY >= FRAME_OVERLAY && \
     UI_OVERLAY < FRAME_OVERLAY + CAM_NBUFFERS) || \
    (VIEW_OVERLAY >= FRAME_OVERLAY && \
     VIEW_OVERLAY < FRAME_OVERLAY + CAM_NBUFFERS)
#  error "The UI, view and frame overlays must be different"
#endif

/* Snapshots are top-down RGB565 BMP files named IMG_0001.BMP to
 * IMG_9999.BMP.  The header is padded to a sector, so the frame goes to
 * the file system in one aligned write.
 */

#define SNAP_MAX           9999
#define SNAP_TIMEOUT_MS    1000             /* Wait for a frame to save */
#define SNAP_PATHLEN       64
#define BMP_HDRSIZE        512
#define BMP_INFOSIZE       40
#define BMP_BITFIELDS      3

/* Layout: the view card on the left, the live data column on the right */

#define UI_PERIOD_MS       250              /* Live data refresh */
#define UI_BUF_LINES       40               /* LVGL draw buffer */
#define UI_MARGIN          4
#define UI_TOP             28               /* Below the title */
#define UI_TITLE_X         8
#define UI_TITLE_Y         6
#define UI_PAD             6
#define UI_RADIUS          8
#define UI_ROW             4
#define UI_COLUMN_ROW      4
#define UI_FPS_H           32               /* Below the view */
#define UI_SIDE_MIN        160
#define UI_CARD_W          (CAM_WIDTH + 2 * UI_PAD)
#define UI_CARD_H          (CAM_HEIGHT + 2 * UI_PAD + UI_FPS_H)

#define CHART_POINTS       40
#define CHART_H            40
#define CHART_DIVS         3
#define CHART_SCALE        10               /* Points per m/s2 */
#define CHART_RANGE        120

#define ACCEL_INTERVAL_US  100000           /* 10 Hz */

/* Depth card: an 8x8 V4L2 Z16 depth camera, one distance in mm per pixel,
 * 0 for no target.
 */

#define TOF_SIZE           8
#define TOF_ZONES          (TOF_SIZE * TOF_SIZE)
#define TOF_BUFSIZE        (TOF_ZONES * sizeof(uint16_t))
#define TOF_NBUFFERS       2
#define TOF_ALIGN          32               /* Buffer alignment */
#define TOF_CELL           8                /* Heatmap cell pitch */
#define TOF_HUE_FAR        240              /* Blue at the far end */
#define TOF_FAR_MM         CONFIG_EXAMPLES_LVGLCAM_TOF_FAR
#define RAD_TO_DEG         57.29578f

#define COLOR_BG           lv_color_hex(0x0d1117)
#define COLOR_CARD         lv_color_hex(0x161b22)
#define COLOR_ACCENT       lv_color_hex(0x2f81f7)
#define COLOR_TEXT         lv_color_hex(0xe6edf3)
#define COLOR_DIM          lv_color_hex(0x8b949e)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Snapshot handshake between the snapshot and the camera thread */

enum snap_state_e
{
  SNAP_IDLE = 0,                        /* All frames are with V4L2 */
  SNAP_REQUEST,                         /* Keep the next frame */
  SNAP_HELD,                            /* Frame 'hold' is kept */
  SNAP_RELEASE                          /* Give frame 'hold' back */
};

struct lvglcam_s
{
  int vfd;                              /* V4L2 capture device */
  int fbfd;                             /* Framebuffer */
  bool streaming;
  FAR uint8_t *frame[CAM_NBUFFERS];     /* Frames in the frame overlays */
  FAR uint8_t *uimem;                   /* UI overlay */
  size_t uilen;
  fb_coord_t uistride;
  fb_coord_t xres;
  fb_coord_t yres;
  struct fb_area_s viewarea;            /* View overlay window to restore */
  bool viewset;
  FAR lv_display_t *disp;
  FAR uint8_t *uibuf;                   /* LVGL draw buffer */
  lv_area_t view;                       /* Camera view on the screen */

  pthread_mutex_t lock;                 /* Protects the fields below */
  pthread_cond_t cond;                  /* Signals their changes */
  bool running;
  bool camdone;                         /* Camera thread finished */
  uint32_t frames;                      /* Captured frames */
  enum snap_state_e state;
  int hold;                             /* Frame kept for a snapshot */
  bool snapjob;                         /* Snapshot requested by the UI */
  bool snapnew;                         /* Snapshot result to show */
  int snapret;
  char snappath[SNAP_PATHLEN];

  pthread_t camthread;
  pthread_t snapthread;
  bool camstarted;
  bool snapstarted;
  int snaps;                            /* Last snapshot number */

  FAR lv_obj_t *image;                  /* Placeholder for the view */
  FAR lv_obj_t *fps;
  FAR lv_obj_t *sys;
  FAR lv_obj_t *imu;
  FAR lv_obj_t *net;
  FAR lv_obj_t *snap;
  FAR lv_obj_t *chart;
  FAR lv_chart_series_t *ser[3];
  uint32_t lastframes;
  uint32_t lastms;

#ifdef CONFIG_SENSORS
  int accfd;
#endif

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  int tofd;                             /* V4L2 depth device */
  pthread_t tofthread;
  bool tofstarted;
  bool tofdone;                         /* Depth thread finished */
  int toferr;                           /* Its error, 0 if none */
  uint16_t tofmm[TOF_ZONES];            /* Latest depth frame */
  uint32_t tofframes;
  uint32_t tofshown;
  FAR lv_obj_t *tofgrid;
  FAR lv_obj_t *tofcell[TOF_ZONES];
  FAR lv_obj_t *tofinfo;
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct lvglcam_s g_cam;
static volatile sig_atomic_t g_stop;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lvglcam_signal
 ****************************************************************************/

static void lvglcam_signal(int signo)
{
  g_stop = 1;
}

/****************************************************************************
 * Name: cam_running
 ****************************************************************************/

static bool cam_running(FAR struct lvglcam_s *cam)
{
  bool running;

  pthread_mutex_lock(&cam->lock);
  running = cam->running;
  pthread_mutex_unlock(&cam->lock);

  return running;
}

/****************************************************************************
 * Name: cam_deadline
 ****************************************************************************/

static void cam_deadline(FAR struct timespec *ts, int ms)
{
  clock_gettime(CLOCK_MONOTONIC, ts);
  ts->tv_sec  += ms / 1000;
  ts->tv_nsec += (ms % 1000) * 1000000;
  if (ts->tv_nsec >= 1000000000)
    {
      ts->tv_sec++;
      ts->tv_nsec -= 1000000000;
    }
}

/****************************************************************************
 * Name: cam_qbuf
 ****************************************************************************/

static int cam_qbuf(FAR struct lvglcam_s *cam, int index)
{
  struct v4l2_buffer buf;

  memset(&buf, 0, sizeof(buf));
  buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory    = V4L2_MEMORY_USERPTR;
  buf.index     = index;
  buf.m.userptr = (uintptr_t)cam->frame[index];
  buf.length    = CAM_FRAMESIZE;

  if (ioctl(cam->vfd, VIDIOC_QBUF, (uintptr_t)&buf) < 0)
    {
      printf("lvglcam: VIDIOC_QBUF failed: %d\n", errno);
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: fb_open
 *
 * Description:
 *   Open the framebuffer, map the UI overlay and check the view overlay.
 *
 ****************************************************************************/

static int fb_open(FAR struct lvglcam_s *cam)
{
  struct fb_videoinfo_s vinfo;
  struct fb_overlayinfo_s oinfo;
  int ret;

  cam->fbfd = open(CONFIG_EXAMPLES_LVGLCAM_FB_DEVPATH, O_RDWR);
  if (cam->fbfd < 0)
    {
      printf("lvglcam: open fb failed: %d\n", errno);
      return -errno;
    }

  ret = ioctl(cam->fbfd, FBIOGET_VIDEOINFO, (unsigned long)&vinfo);
  if (ret < 0)
    {
      printf("lvglcam: FBIOGET_VIDEOINFO failed: %d\n", errno);
      return -errno;
    }

  if (vinfo.noverlays <= UI_OVERLAY || vinfo.noverlays <= VIEW_OVERLAY ||
      vinfo.noverlays < FRAME_OVERLAY + CAM_NBUFFERS)
    {
      printf("lvglcam: %d fb overlays are not enough\n", vinfo.noverlays);
      return -ENODEV;
    }

  cam->xres = vinfo.xres;
  cam->yres = vinfo.yres;
  if (cam->xres < UI_CARD_W + UI_SIDE_MIN + 3 * UI_MARGIN ||
      cam->yres < UI_CARD_H + UI_TOP + UI_MARGIN)
    {
      printf("lvglcam: %dx%d view does not fit the %dx%d display\n",
             CAM_WIDTH, CAM_HEIGHT, cam->xres, cam->yres);
      return -EINVAL;
    }

  /* LVGL draws the whole display into the UI overlay */

  memset(&oinfo, 0, sizeof(oinfo));
  oinfo.overlay = UI_OVERLAY;
  ret = ioctl(cam->fbfd, FBIOGET_OVERLAYINFO, (unsigned long)&oinfo);
  if (ret < 0 || oinfo.bpp != CAM_BYPP * 8 ||
      oinfo.stride < cam->xres * CAM_BYPP ||
      oinfo.fblen < (size_t)oinfo.stride * cam->yres)
    {
      printf("lvglcam: fb overlay %d unusable for the UI\n", UI_OVERLAY);
      return -ENODEV;
    }

  ret = ioctl(cam->fbfd, FBIO_SELECT_OVERLAY, UI_OVERLAY);
  if (ret < 0)
    {
      printf("lvglcam: FBIO_SELECT_OVERLAY failed: %d\n", errno);
      return -errno;
    }

  cam->uimem = mmap(NULL, oinfo.fblen, PROT_READ | PROT_WRITE, MAP_SHARED,
                    cam->fbfd, 0);
  if (cam->uimem == MAP_FAILED)
    {
      cam->uimem = NULL;
      printf("lvglcam: mmap UI overlay failed: %d\n", errno);
      return -errno;
    }

  cam->uilen    = oinfo.fblen;
  cam->uistride = oinfo.stride;

  /* The camera frames are blitted into the view overlay */

  memset(&oinfo, 0, sizeof(oinfo));
  oinfo.overlay = VIEW_OVERLAY;
  ret = ioctl(cam->fbfd, FBIOGET_OVERLAYINFO, (unsigned long)&oinfo);
  if (ret < 0 || oinfo.bpp != CAM_BYPP * 8 || oinfo.stride < CAM_STRIDE ||
      oinfo.fblen < (size_t)oinfo.stride * CAM_HEIGHT)
    {
      printf("lvglcam: fb overlay %d unusable for the view\n",
             VIEW_OVERLAY);
      return -ENODEV;
    }

  cam->viewarea = oinfo.darea;
  return OK;
}

/****************************************************************************
 * Name: fb_close
 ****************************************************************************/

static void fb_close(FAR struct lvglcam_s *cam)
{
  struct fb_overlayinfo_s oinfo;

  if (cam->viewset)
    {
      memset(&oinfo, 0, sizeof(oinfo));
      oinfo.overlay = VIEW_OVERLAY;
      oinfo.darea   = cam->viewarea;
      ioctl(cam->fbfd, FBIOSET_DESTAREA, (unsigned long)&oinfo);
      cam->viewset = false;
    }

  if (cam->uimem != NULL)
    {
      munmap(cam->uimem, cam->uilen);
      cam->uimem = NULL;
    }

  if (cam->fbfd >= 0)
    {
      close(cam->fbfd);
      cam->fbfd = -1;
    }
}

/****************************************************************************
 * Name: cam_open
 *
 * Description:
 *   Open the V4L2 capture device, set the RGB565 format, queue the frame
 *   overlays as capture buffers and start streaming.
 *
 ****************************************************************************/

static int cam_open(FAR struct lvglcam_s *cam)
{
  struct v4l2_requestbuffers req;
  struct v4l2_format fmt;
  enum v4l2_buf_type type;
  int ret;
  int i;

  cam->vfd = open(CONFIG_EXAMPLES_LVGLCAM_VIDEO_DEVPATH, O_RDONLY);
  if (cam->vfd < 0)
    {
      printf("lvglcam: open video failed: %d\n", errno);
      return -errno;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = CAM_WIDTH;
  fmt.fmt.pix.height      = CAM_HEIGHT;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

  ret = ioctl(cam->vfd, VIDIOC_S_FMT, (uintptr_t)&fmt);
  if (ret < 0)
    {
      printf("lvglcam: VIDIOC_S_FMT failed: %d\n", errno);
      return -errno;
    }

  if (fmt.fmt.pix.width != CAM_WIDTH || fmt.fmt.pix.height != CAM_HEIGHT ||
      fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565)
    {
      printf("lvglcam: camera gives %" PRIu32 "x%" PRIu32 ", not %dx%d "
             "RGB565\n", fmt.fmt.pix.width, fmt.fmt.pix.height, CAM_WIDTH,
             CAM_HEIGHT);
      return -EINVAL;
    }

  memset(&req, 0, sizeof(req));
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count  = CAM_NBUFFERS;
  req.mode   = V4L2_BUF_MODE_FIFO;

  ret = ioctl(cam->vfd, VIDIOC_REQBUFS, (uintptr_t)&req);
  if (ret < 0)
    {
      printf("lvglcam: VIDIOC_REQBUFS failed: %d\n", errno);
      return -errno;
    }

  if (req.count < CAM_NBUFFERS)
    {
      printf("lvglcam: only %" PRIu32 " capture buffers\n", req.count);
      return -ENOMEM;
    }

  /* The frames are captured straight into the frame overlays, the
   * sources of the blits into the view overlay.
   */

  for (i = 0; i < CAM_NBUFFERS; i++)
    {
      struct fb_overlayinfo_s oinfo;

      memset(&oinfo, 0, sizeof(oinfo));
      oinfo.overlay = FRAME_OVERLAY + i;
      ret = ioctl(cam->fbfd, FBIOGET_OVERLAYINFO, (unsigned long)&oinfo);
      if (ret < 0 || oinfo.stride != CAM_STRIDE ||
          oinfo.fblen < CAM_FRAMESIZE)
        {
          printf("lvglcam: fb overlay %d unusable for %dx%d RGB565\n",
                 FRAME_OVERLAY + i, CAM_WIDTH, CAM_HEIGHT);
          return -ENODEV;
        }

      ret = ioctl(cam->fbfd, FBIO_SELECT_OVERLAY, FRAME_OVERLAY + i);
      if (ret < 0)
        {
          printf("lvglcam: FBIO_SELECT_OVERLAY failed: %d\n", errno);
          return -errno;
        }

      cam->frame[i] = mmap(NULL, CAM_FRAMESIZE, PROT_READ | PROT_WRITE,
                           MAP_SHARED, cam->fbfd, 0);
      if (cam->frame[i] == MAP_FAILED)
        {
          cam->frame[i] = NULL;
          printf("lvglcam: mmap fb overlay %d failed: %d\n",
                 FRAME_OVERLAY + i, errno);
          return -errno;
        }

      ret = cam_qbuf(cam, i);
      if (ret < 0)
        {
          return ret;
        }
    }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ret  = ioctl(cam->vfd, VIDIOC_STREAMON, (uintptr_t)&type);
  if (ret < 0)
    {
      printf("lvglcam: VIDIOC_STREAMON failed: %d\n", errno);
      return -errno;
    }

  cam->streaming = true;
  return OK;
}

/****************************************************************************
 * Name: cam_close
 ****************************************************************************/

static void cam_close(FAR struct lvglcam_s *cam)
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int i;

  if (cam->streaming)
    {
      ioctl(cam->vfd, VIDIOC_STREAMOFF, (uintptr_t)&type);
      cam->streaming = false;
    }

  if (cam->vfd >= 0)
    {
      close(cam->vfd);
      cam->vfd = -1;
    }

  for (i = 0; i < CAM_NBUFFERS; i++)
    {
      if (cam->frame[i] != NULL)
        {
          munmap(cam->frame[i], CAM_FRAMESIZE);
          cam->frame[i] = NULL;
        }
    }
}

/****************************************************************************
 * Name: cam_blit
 *
 * Description:
 *   Copy a captured frame into the view overlay (DMA2D).
 *
 ****************************************************************************/

static int cam_blit(FAR struct lvglcam_s *cam, int index)
{
  struct fb_overlayblit_s blit;

  blit.src.overlay  = FRAME_OVERLAY + index;
  blit.src.area.x   = 0;
  blit.src.area.y   = 0;
  blit.src.area.w   = CAM_WIDTH;
  blit.src.area.h   = CAM_HEIGHT;
  blit.dest.overlay = VIEW_OVERLAY;
  blit.dest.area    = blit.src.area;

  if (ioctl(cam->fbfd, FBIOSET_BLIT, (unsigned long)&blit) < 0)
    {
      printf("lvglcam: FBIOSET_BLIT failed: %d\n", errno);
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: cam_thread
 *
 * Description:
 *   Show each captured frame and give it back to V4L2, unless it is kept
 *   for a snapshot.
 *
 ****************************************************************************/

static FAR void *cam_thread(FAR void *arg)
{
  FAR struct lvglcam_s *cam = (FAR struct lvglcam_s *)arg;
  struct v4l2_buffer buf;
  bool running = true;
  int errors = 0;
  int requeue;
  bool keep;

  while (running)
    {
      memset(&buf, 0, sizeof(buf));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;

      if (ioctl(cam->vfd, VIDIOC_DQBUF, (uintptr_t)&buf) < 0 ||
          buf.index >= CAM_NBUFFERS)
        {
          if (!cam_running(cam))
            {
              break;
            }

          if (++errors >= CAM_ERRORS_MAX)
            {
              printf("lvglcam: VIDIOC_DQBUF failed: %d\n", errno);
              break;
            }

          usleep(CAM_RETRY_US);
          continue;
        }

      errors = 0;
      if (cam_blit(cam, buf.index) < 0)
        {
          break;
        }

      pthread_mutex_lock(&cam->lock);
      cam->frames++;
      running = cam->running;

      requeue = -1;
      if (cam->state == SNAP_RELEASE)
        {
          requeue    = cam->hold;
          cam->hold  = -1;
          cam->state = SNAP_IDLE;
          pthread_cond_broadcast(&cam->cond);
        }

      keep = cam->state == SNAP_REQUEST;
      if (keep)
        {
          cam->hold  = buf.index;
          cam->state = SNAP_HELD;
          pthread_cond_broadcast(&cam->cond);
        }

      pthread_mutex_unlock(&cam->lock);

      if ((!keep && cam_qbuf(cam, buf.index) < 0) ||
          (requeue >= 0 && cam_qbuf(cam, requeue) < 0))
        {
          break;
        }
    }

  pthread_mutex_lock(&cam->lock);
  cam->running = false;
  cam->camdone = true;
  pthread_cond_broadcast(&cam->cond);
  pthread_mutex_unlock(&cam->lock);

  return NULL;
}

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
/****************************************************************************
 * Name: tof_thread
 *
 * Description:
 *   Stream the depth camera and keep its latest frame for the UI.
 *
 ****************************************************************************/

static FAR void *tof_thread(FAR void *arg)
{
  FAR struct lvglcam_s *cam = (FAR struct lvglcam_s *)arg;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct v4l2_requestbuffers req;
  struct v4l2_format fmt;
  struct v4l2_buffer buf;
  FAR uint16_t *mem;
  bool streaming = false;
  int ret = OK;
  int fd = -1;
  int i;

  mem = memalign(TOF_ALIGN, TOF_NBUFFERS * TOF_BUFSIZE);
  if (mem == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

  fd = open(CONFIG_EXAMPLES_LVGLCAM_TOF_DEVPATH, O_RDONLY);
  if (fd < 0)
    {
      ret = -errno;
      goto errout;
    }

  pthread_mutex_lock(&cam->lock);
  cam->tofd = fd;
  pthread_mutex_unlock(&cam->lock);

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = TOF_SIZE;
  fmt.fmt.pix.height      = TOF_SIZE;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_Z16;
  if (ioctl(fd, VIDIOC_S_FMT, (uintptr_t)&fmt) < 0)
    {
      ret = -errno;
      goto errout;
    }

  memset(&req, 0, sizeof(req));
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count  = TOF_NBUFFERS;
  if (ioctl(fd, VIDIOC_REQBUFS, (uintptr_t)&req) < 0)
    {
      ret = -errno;
      goto errout;
    }

  for (i = 0; i < TOF_NBUFFERS; i++)
    {
      memset(&buf, 0, sizeof(buf));
      buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory    = V4L2_MEMORY_USERPTR;
      buf.index     = i;
      buf.m.userptr = (uintptr_t)&mem[i * TOF_ZONES];
      buf.length    = TOF_BUFSIZE;
      if (ioctl(fd, VIDIOC_QBUF, (uintptr_t)&buf) < 0)
        {
          ret = -errno;
          goto errout;
        }
    }

  /* The first stream on loads the sensor firmware, it takes a while */

  if (ioctl(fd, VIDIOC_STREAMON, (uintptr_t)&type) < 0)
    {
      ret = -errno;
      goto errout;
    }

  streaming = true;
  while (cam_running(cam))
    {
      memset(&buf, 0, sizeof(buf));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;
      if (ioctl(fd, VIDIOC_DQBUF, (uintptr_t)&buf) < 0)
        {
          if (errno != ECANCELED && errno != EINTR)
            {
              ret = -errno;
            }

          break;
        }

      if ((buf.flags & V4L2_BUF_FLAG_ERROR) == 0 &&
          buf.bytesused >= TOF_BUFSIZE)
        {
          pthread_mutex_lock(&cam->lock);
          memcpy(cam->tofmm, (FAR const void *)buf.m.userptr, TOF_BUFSIZE);
          cam->tofframes++;
          pthread_mutex_unlock(&cam->lock);
        }

      if (ioctl(fd, VIDIOC_QBUF, (uintptr_t)&buf) < 0)
        {
          ret = -errno;
          break;
        }
    }

errout:
  if (streaming)
    {
      ioctl(fd, VIDIOC_STREAMOFF, (uintptr_t)&type);
    }

  pthread_mutex_lock(&cam->lock);
  if (cam->tofd >= 0)
    {
      close(cam->tofd);
      cam->tofd = -1;
    }

  cam->toferr  = ret;
  cam->tofdone = true;
  pthread_cond_broadcast(&cam->cond);
  pthread_mutex_unlock(&cam->lock);

  free(mem);
  return NULL;
}
#endif

/****************************************************************************
 * Name: cam_stop
 *
 * Description:
 *   Stop the camera, depth and snapshot threads.  DQBUF is cancelled until
 *   the capture threads are out of it.
 *
 ****************************************************************************/

static void cam_stop(FAR struct lvglcam_s *cam)
{
  struct timespec ts;

  pthread_mutex_lock(&cam->lock);
  cam->running = false;
  pthread_cond_broadcast(&cam->cond);

  while (cam->camstarted && !cam->camdone)
    {
      pthread_mutex_unlock(&cam->lock);
      ioctl(cam->vfd, VIDIOC_CANCEL_DQBUF, V4L2_BUF_TYPE_VIDEO_CAPTURE);
      pthread_mutex_lock(&cam->lock);

      cam_deadline(&ts, CAM_STOP_MS);
      pthread_cond_timedwait(&cam->cond, &cam->lock, &ts);
    }

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  while (cam->tofstarted && !cam->tofdone)
    {
      if (cam->tofd >= 0)
        {
          ioctl(cam->tofd, VIDIOC_CANCEL_DQBUF,
                V4L2_BUF_TYPE_VIDEO_CAPTURE);
        }

      cam_deadline(&ts, CAM_STOP_MS);
      pthread_cond_timedwait(&cam->cond, &cam->lock, &ts);
    }
#endif

  pthread_mutex_unlock(&cam->lock);

  if (cam->camstarted)
    {
      pthread_join(cam->camthread, NULL);
      cam->camstarted = false;
    }

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  if (cam->tofstarted)
    {
      pthread_join(cam->tofthread, NULL);
      cam->tofstarted = false;
    }
#endif

  if (cam->snapstarted)
    {
      pthread_join(cam->snapthread, NULL);
      cam->snapstarted = false;
    }
}

/****************************************************************************
 * Name: snap_open
 *
 * Description:
 *   Create the next free snapshot file, never overwrite one.
 *
 ****************************************************************************/

static int snap_open(FAR struct lvglcam_s *cam, FAR char *path, size_t size)
{
  int fd;
  int n;

  for (n = cam->snaps + 1; n <= SNAP_MAX; n++)
    {
      snprintf(path, size, "%s/IMG_%04d.BMP",
               CONFIG_EXAMPLES_LVGLCAM_SNAPDIR, n);

      fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
      if (fd >= 0)
        {
          cam->snaps = n;
          return fd;
        }

      if (errno != EEXIST)
        {
          return -errno;
        }
    }

  path[0] = '\0';
  return -ENOSPC;
}

/****************************************************************************
 * Name: snap_hold
 *
 * Description:
 *   Keep the next frame out of V4L2 and return its index.
 *
 ****************************************************************************/

static int snap_hold(FAR struct lvglcam_s *cam)
{
  struct timespec ts;
  int index;
  int ret = 0;

  cam_deadline(&ts, SNAP_TIMEOUT_MS);
  pthread_mutex_lock(&cam->lock);

  /* The frame of the previous snapshot may still be on its way back */

  while (cam->running && cam->state != SNAP_IDLE && ret == 0)
    {
      ret = pthread_cond_timedwait(&cam->cond, &cam->lock, &ts);
    }

  if (cam->running && cam->state == SNAP_IDLE)
    {
      cam->state = SNAP_REQUEST;
      ret = 0;
      while (cam->running && cam->state == SNAP_REQUEST && ret == 0)
        {
          ret = pthread_cond_timedwait(&cam->cond, &cam->lock, &ts);
        }
    }

  if (cam->state == SNAP_HELD)
    {
      index = cam->hold;
    }
  else
    {
      if (cam->state == SNAP_REQUEST)
        {
          cam->state = SNAP_IDLE;
        }

      index = cam->running ? -ETIMEDOUT : -ECANCELED;
    }

  pthread_mutex_unlock(&cam->lock);
  return index;
}

/****************************************************************************
 * Name: snap_release
 ****************************************************************************/

static void snap_release(FAR struct lvglcam_s *cam)
{
  pthread_mutex_lock(&cam->lock);
  cam->state = SNAP_RELEASE;
  pthread_mutex_unlock(&cam->lock);
}

/****************************************************************************
 * Name: snap_write
 ****************************************************************************/

static int snap_write(int fd, FAR const void *buf, size_t len)
{
  FAR const uint8_t *ptr = buf;
  ssize_t nwritten;

  while (len > 0)
    {
      nwritten = write(fd, ptr, len);
      if (nwritten < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      if (nwritten == 0)
        {
          return -EIO;
        }

      ptr += nwritten;
      len -= nwritten;
    }

  return OK;
}

/****************************************************************************
 * Name: snap_save
 *
 * Description:
 *   Save the next frame as a top-down RGB565 BMP.
 *
 ****************************************************************************/

static int snap_save(FAR struct lvglcam_s *cam, FAR char *path,
                     size_t size)
{
  static const uint32_t masks[3] =
  {
    0xf800, 0x07e0, 0x001f
  };

  uint8_t header[BMP_HDRSIZE];
  uint32_t offset = sizeof(header);
  uint32_t filesize = offset + CAM_FRAMESIZE;
  int32_t height = -CAM_HEIGHT;
  uint32_t value;
  int index;
  int ret;
  int fd;

  memset(header, 0, sizeof(header));
  header[0] = 'B';
  header[1] = 'M';
  memcpy(&header[2], &filesize, 4);
  memcpy(&header[10], &offset, 4);
  value = BMP_INFOSIZE;
  memcpy(&header[14], &value, 4);
  value = CAM_WIDTH;
  memcpy(&header[18], &value, 4);
  memcpy(&header[22], &height, 4);
  header[26] = 1;                       /* Planes */
  header[28] = CAM_BYPP * 8;
  header[30] = BMP_BITFIELDS;
  value = CAM_FRAMESIZE;
  memcpy(&header[34], &value, 4);
  memcpy(&header[54], masks, sizeof(masks));

  fd = snap_open(cam, path, size);
  if (fd < 0)
    {
      return fd;
    }

  ret = snap_write(fd, header, sizeof(header));
  if (ret >= 0)
    {
      index = snap_hold(cam);
      if (index < 0)
        {
          ret = index;
        }
      else
        {
          ret = snap_write(fd, cam->frame[index], CAM_FRAMESIZE);
          snap_release(cam);
        }
    }

  if (close(fd) < 0 && ret >= 0)
    {
      ret = -errno;
    }

  if (ret < 0)
    {
      unlink(path);
    }

  return ret;
}

/****************************************************************************
 * Name: snap_thread
 *
 * Description:
 *   Save the snapshots requested by the UI, off the LVGL thread.
 *
 ****************************************************************************/

static FAR void *snap_thread(FAR void *arg)
{
  FAR struct lvglcam_s *cam = (FAR struct lvglcam_s *)arg;
  char path[SNAP_PATHLEN];
  int ret;

  for (; ; )
    {
      pthread_mutex_lock(&cam->lock);
      while (cam->running && !cam->snapjob)
        {
          pthread_cond_wait(&cam->cond, &cam->lock);
        }

      if (!cam->running)
        {
          pthread_mutex_unlock(&cam->lock);
          break;
        }

      pthread_mutex_unlock(&cam->lock);

      ret = snap_save(cam, path, sizeof(path));

      pthread_mutex_lock(&cam->lock);
      strlcpy(cam->snappath, path, sizeof(cam->snappath));
      cam->snapret = ret;
      cam->snapnew = true;
      cam->snapjob = false;
      pthread_mutex_unlock(&cam->lock);
    }

  return NULL;
}

/****************************************************************************
 * Name: snap_cb
 ****************************************************************************/

static void snap_cb(FAR lv_event_t *e)
{
  FAR struct lvglcam_s *cam = lv_event_get_user_data(e);
  bool busy;

  pthread_mutex_lock(&cam->lock);
  busy = cam->snapjob;
  if (!busy)
    {
      cam->snapjob = true;
      pthread_cond_broadcast(&cam->cond);
    }

  pthread_mutex_unlock(&cam->lock);

  if (!busy)
    {
      lv_label_set_text(cam->snap, LV_SYMBOL_SAVE " saving...");
    }
}

/****************************************************************************
 * Name: cpuload
 ****************************************************************************/

static int cpuload(void)
{
  char line[64];
  int load = -1;
  FAR FILE *fp;

  fp = fopen("/proc/cpuload", "r");
  if (fp != NULL)
    {
      if (fgets(line, sizeof(line), fp) != NULL)
        {
          load = (int)(atof(line) + 0.5);
        }

      fclose(fp);
    }

  return load;
}

/****************************************************************************
 * Name: ui_card
 ****************************************************************************/

static FAR lv_obj_t *ui_card(FAR lv_obj_t *parent, FAR const char *title,
                             FAR lv_obj_t **body)
{
  FAR lv_obj_t *card;
  FAR lv_obj_t *label;

  card = lv_obj_create(parent);
  lv_obj_set_width(card, LV_PCT(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_radius(card, UI_RADIUS, 0);
  lv_obj_set_style_pad_all(card, UI_PAD, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(card, UI_ROW, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  label = lv_label_create(card);
  lv_label_set_text(label, title);
  lv_obj_set_style_text_color(label, COLOR_ACCENT, 0);

  if (body != NULL)
    {
      *body = lv_label_create(card);
      lv_label_set_text(*body, "-");
      lv_obj_set_style_text_color(*body, COLOR_TEXT, 0);
    }

  return card;
}

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
/****************************************************************************
 * Name: tof_create
 ****************************************************************************/

static void tof_create(FAR struct lvglcam_s *cam, FAR lv_obj_t *parent)
{
  FAR lv_obj_t *card;
  FAR lv_obj_t *row;
  int i;

  card = ui_card(parent, LV_SYMBOL_EYE_OPEN " ToF 8x8 (V4L2 Z16)", NULL);

  row = lv_obj_create(card);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(row, UI_PAD, 0);

  cam->tofgrid = lv_obj_create(row);
  lv_obj_remove_style_all(cam->tofgrid);
  lv_obj_set_size(cam->tofgrid, TOF_SIZE * TOF_CELL, TOF_SIZE * TOF_CELL);
  lv_obj_remove_flag(cam->tofgrid, LV_OBJ_FLAG_SCROLLABLE);

  for (i = 0; i < TOF_ZONES; i++)
    {
      cam->tofcell[i] = lv_obj_create(cam->tofgrid);
      lv_obj_remove_style_all(cam->tofcell[i]);
      lv_obj_set_size(cam->tofcell[i], TOF_CELL - 1, TOF_CELL - 1);
      lv_obj_set_pos(cam->tofcell[i], (i % TOF_SIZE) * TOF_CELL,
                     (i / TOF_SIZE) * TOF_CELL);
      lv_obj_set_style_bg_opa(cam->tofcell[i], LV_OPA_COVER, 0);
      lv_obj_set_style_bg_color(cam->tofcell[i], COLOR_BG, 0);
    }

  cam->tofinfo = lv_label_create(row);
  lv_obj_set_flex_grow(cam->tofinfo, 1);
  lv_label_set_long_mode(cam->tofinfo, LV_LABEL_LONG_WRAP);
  lv_label_set_text(cam->tofinfo, "starting...");
  lv_obj_set_style_text_color(cam->tofinfo, COLOR_DIM, 0);
}

/****************************************************************************
 * Name: tof_update
 *
 * Description:
 *   Show the latest depth frame: near is red, far is blue, no target is
 *   left dark.
 *
 ****************************************************************************/

static void tof_update(FAR struct lvglcam_s *cam)
{
  uint16_t mm[TOF_ZONES];
  uint32_t frames;
  uint16_t min = UINT16_MAX;
  uint16_t max = 0;
  bool done;
  int err;
  int d;
  int i;

  pthread_mutex_lock(&cam->lock);
  memcpy(mm, cam->tofmm, sizeof(mm));
  frames = cam->tofframes;
  done   = cam->tofdone;
  err    = cam->toferr;
  pthread_mutex_unlock(&cam->lock);

  if (done && err < 0)
    {
      lv_label_set_text_fmt(cam->tofinfo, "%s\n%d",
                            err == -ENOENT ? "files missing" : "error",
                            err);
      return;
    }

  if (frames == cam->tofshown)
    {
      return;
    }

  cam->tofshown = frames;

  /* One invalidation for the grid instead of one per cell */

  lv_display_enable_invalidation(NULL, false);
  for (i = 0; i < TOF_ZONES; i++)
    {
      if (mm[i] == 0)
        {
          lv_obj_set_style_bg_color(cam->tofcell[i], COLOR_BG, 0);
          continue;
        }

      d = mm[i] < TOF_FAR_MM ? mm[i] : TOF_FAR_MM;
      lv_obj_set_style_bg_color(cam->tofcell[i],
                                lv_color_hsv_to_rgb(TOF_HUE_FAR * d /
                                                    TOF_FAR_MM, 100, 100),
                                0);
      min = mm[i] < min ? mm[i] : min;
      max = mm[i] > max ? mm[i] : max;
    }

  lv_display_enable_invalidation(NULL, true);
  lv_obj_invalidate(cam->tofgrid);

  if (max == 0)
    {
      lv_label_set_text(cam->tofinfo, "no target");
    }
  else
    {
      lv_label_set_text_fmt(cam->tofinfo, "%u-%u\nmm", min, max);
    }
}
#endif

/****************************************************************************
 * Name: ui_timer
 *
 * Description:
 *   Refresh the system, sensor, network and snapshot cards.
 *
 ****************************************************************************/

static void ui_timer(FAR lv_timer_t *timer)
{
  FAR struct lvglcam_s *cam = lv_timer_get_user_data(timer);
  struct mallinfo mem = mallinfo();
  struct timespec ts;
  uint32_t now = lv_tick_get();
  uint32_t frames;
  char path[SNAP_PATHLEN];
  bool snapnew;
  int snapret;
  int load;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  pthread_mutex_lock(&cam->lock);
  frames  = cam->frames;
  snapnew = cam->snapnew;
  snapret = cam->snapret;
  strlcpy(path, cam->snappath, sizeof(path));
  cam->snapnew = false;
  pthread_mutex_unlock(&cam->lock);

  if (now != cam->lastms)
    {
      lv_label_set_text_fmt(cam->fps, LV_SYMBOL_VIDEO " %dx%d  %u fps",
                            CAM_WIDTH, CAM_HEIGHT,
                            (unsigned)((frames - cam->lastframes) * 1000 /
                                       (now - cam->lastms)));
    }

  cam->lastframes = frames;
  cam->lastms     = now;

  if (snapnew)
    {
      if (snapret >= 0)
        {
          lv_label_set_text_fmt(cam->snap, LV_SYMBOL_OK " %s", path);
        }
      else if (snapret == -ENOSPC && path[0] == '\0')
        {
          lv_label_set_text(cam->snap,
                            LV_SYMBOL_WARNING " no free IMG_nnnn.BMP");
        }
      else
        {
          lv_label_set_text_fmt(cam->snap,
                                LV_SYMBOL_WARNING " save failed: %d",
                                snapret);
        }
    }

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  tof_update(cam);
#endif

  load = cpuload();
  lv_label_set_text_fmt(cam->sys,
                        "Uptime  %02u:%02u:%02u\n"
                        "CPU     %s%d%%\n"
                        "Free    %u KiB",
                        (unsigned)(ts.tv_sec / 3600),
                        (unsigned)(ts.tv_sec / 60 % 60),
                        (unsigned)(ts.tv_sec % 60),
                        load < 0 ? "n/a " : "", load < 0 ? 0 : load,
                        (unsigned)(mem.fordblks / 1024));

#ifdef CONFIG_SENSORS
  if (cam->accfd >= 0)
    {
      struct sensor_accel acc;

      if (read(cam->accfd, &acc, sizeof(acc)) == sizeof(acc))
        {
          float norm  = sqrtf(acc.x * acc.x + acc.y * acc.y +
                              acc.z * acc.z);
          float roll  = atan2f(acc.y, acc.z) * RAD_TO_DEG;
          float pitch = atan2f(-acc.x, sqrtf(acc.y * acc.y +
                                             acc.z * acc.z)) * RAD_TO_DEG;

          lv_label_set_text_fmt(cam->imu,
                                "Roll   %4d deg\n"
                                "Pitch  %4d deg\n"
                                "|a|    %d.%02d m/s2",
                                (int)roll, (int)pitch, (int)norm,
                                (int)(fmodf(norm, 1.0f) * 100));
          lv_chart_set_next_value(cam->chart, cam->ser[0],
                                  (int32_t)(acc.x * CHART_SCALE));
          lv_chart_set_next_value(cam->chart, cam->ser[1],
                                  (int32_t)(acc.y * CHART_SCALE));
          lv_chart_set_next_value(cam->chart, cam->ser[2],
                                  (int32_t)(acc.z * CHART_SCALE));
        }
    }
#endif

#ifdef CONFIG_NET
  {
    FAR const char *addrstr = "no address";
    uint8_t flags = 0;
#  ifdef CONFIG_NET_IPv4
    struct in_addr addr;

    if (netlib_get_ipv4addr(CONFIG_EXAMPLES_LVGLCAM_NETDEV, &addr) >= 0 &&
        addr.s_addr != INADDR_ANY)
      {
        addrstr = inet_ntoa(addr);
      }
#  endif

    if (netlib_getifstatus(CONFIG_EXAMPLES_LVGLCAM_NETDEV, &flags) < 0)
      {
        flags = 0;
      }

    lv_label_set_text_fmt(cam->net, LV_SYMBOL_WIFI " %s  %s\n%s",
                          CONFIG_EXAMPLES_LVGLCAM_NETDEV,
                          (flags & IFF_RUNNING) ? "link up" : "link down",
                          addrstr);
  }
#endif
}

/****************************************************************************
 * Name: ui_flush
 *
 * Description:
 *   Copy a rendered area into the UI overlay.
 *
 ****************************************************************************/

static void ui_flush(FAR lv_display_t *disp, FAR const lv_area_t *area,
                     FAR uint8_t *pixels)
{
  FAR struct lvglcam_s *cam = lv_display_get_user_data(disp);
  int32_t width = lv_area_get_width(area);
  uint32_t stride = lv_draw_buf_width_to_stride(width,
                                                LV_COLOR_FORMAT_RGB565);
  FAR uint8_t *dest = cam->uimem + area->y1 * cam->uistride +
                      area->x1 * CAM_BYPP;
  int32_t y;

  for (y = area->y1; y <= area->y2; y++)
    {
      memcpy(dest, pixels, width * CAM_BYPP);
      dest   += cam->uistride;
      pixels += stride;
    }

  lv_display_flush_ready(disp);
}

/****************************************************************************
 * Name: ui_open
 *
 * Description:
 *   Create the LVGL display of the UI overlay.
 *
 ****************************************************************************/

static int ui_open(FAR struct lvglcam_s *cam)
{
  uint32_t size = lv_draw_buf_width_to_stride(cam->xres,
                                              LV_COLOR_FORMAT_RGB565) *
                  UI_BUF_LINES;

  cam->uibuf = malloc(size);
  if (cam->uibuf == NULL)
    {
      return -ENOMEM;
    }

  cam->disp = lv_display_create(cam->xres, cam->yres);
  if (cam->disp == NULL)
    {
      return -ENOMEM;
    }

  lv_display_set_user_data(cam->disp, cam);
  lv_display_set_color_format(cam->disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(cam->disp, cam->uibuf, NULL, size,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(cam->disp, ui_flush);
  return OK;
}

/****************************************************************************
 * Name: ui_close
 ****************************************************************************/

static void ui_close(FAR struct lvglcam_s *cam)
{
  if (cam->disp != NULL)
    {
      lv_display_delete(cam->disp);
      cam->disp = NULL;
    }

  free(cam->uibuf);
  cam->uibuf = NULL;
}

/****************************************************************************
 * Name: ui_create
 ****************************************************************************/

static void ui_create(FAR struct lvglcam_s *cam)
{
  FAR lv_obj_t *scr = lv_screen_active();
  FAR lv_obj_t *title;
  FAR lv_obj_t *left;
  FAR lv_obj_t *right;
  FAR lv_obj_t *card;
  FAR lv_obj_t *btn;
  FAR lv_obj_t *label;
  struct utsname name;

  lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
  lv_obj_set_style_text_color(scr, COLOR_TEXT, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  /* Title bar */

  title = lv_label_create(scr);
  if (uname(&name) >= 0)
    {
      lv_label_set_text_fmt(title, "Apache NuttX %s  |  %s  |  LVGL %d.%d",
                            name.release, name.machine, LVGL_VERSION_MAJOR,
                            LVGL_VERSION_MINOR);
    }
  else
    {
      lv_label_set_text_fmt(title, "Apache NuttX  |  LVGL %d.%d",
                            LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR);
    }

  lv_obj_set_style_text_color(title, COLOR_DIM, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, UI_TITLE_X, UI_TITLE_Y);

  /* Camera view card */

  left = lv_obj_create(scr);
  lv_obj_set_size(left, UI_CARD_W, UI_CARD_H);
  lv_obj_align(left, LV_ALIGN_TOP_LEFT, UI_MARGIN, UI_TOP);
  lv_obj_set_style_bg_color(left, COLOR_CARD, 0);
  lv_obj_set_style_border_width(left, 0, 0);
  lv_obj_set_style_radius(left, UI_RADIUS, 0);
  lv_obj_set_style_pad_all(left, UI_PAD, 0);
  lv_obj_remove_flag(left, LV_OBJ_FLAG_SCROLLABLE);

  /* The view overlay covers this placeholder */

  cam->image = lv_obj_create(left);
  lv_obj_set_size(cam->image, CAM_WIDTH, CAM_HEIGHT);
  lv_obj_align(cam->image, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_opa(cam->image, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cam->image, 0, 0);
  lv_obj_set_style_radius(cam->image, 0, 0);
  lv_obj_remove_flag(cam->image, LV_OBJ_FLAG_SCROLLABLE);

  cam->fps = lv_label_create(left);
  lv_obj_set_style_text_color(cam->fps, COLOR_TEXT, 0);
  lv_label_set_text(cam->fps, LV_SYMBOL_VIDEO " starting...");
  lv_obj_align(cam->fps, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  /* Live data column */

  right = lv_obj_create(scr);
  lv_obj_set_size(right, cam->xres - UI_CARD_W - 3 * UI_MARGIN,
                  cam->yres - UI_TOP - UI_MARGIN);
  lv_obj_align(right, LV_ALIGN_TOP_RIGHT, -UI_MARGIN, UI_TOP);
  lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(right, 0, 0);
  lv_obj_set_style_pad_all(right, 0, 0);
  lv_obj_set_style_pad_row(right, UI_COLUMN_ROW, 0);
  lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);

  card = ui_card(right, LV_SYMBOL_SETTINGS " System", &cam->sys);
  cam->net = lv_label_create(card);
  lv_label_set_text(cam->net, "-");
  lv_obj_set_style_text_color(cam->net, COLOR_TEXT, 0);

  card = ui_card(right, LV_SYMBOL_REFRESH " IMU (uORB)", &cam->imu);
  cam->chart = lv_chart_create(card);
  lv_obj_set_size(cam->chart, LV_PCT(100), CHART_H);
  lv_chart_set_point_count(cam->chart, CHART_POINTS);
  lv_chart_set_range(cam->chart, LV_CHART_AXIS_PRIMARY_Y, -CHART_RANGE,
                     CHART_RANGE);
  lv_chart_set_div_line_count(cam->chart, CHART_DIVS, 0);
  lv_obj_set_style_bg_opa(cam->chart, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cam->chart, 0, 0);
  lv_obj_set_style_size(cam->chart, 0, 0, LV_PART_INDICATOR);
  cam->ser[0] = lv_chart_add_series(cam->chart,
                                    lv_palette_main(LV_PALETTE_RED),
                                    LV_CHART_AXIS_PRIMARY_Y);
  cam->ser[1] = lv_chart_add_series(cam->chart,
                                    lv_palette_main(LV_PALETTE_GREEN),
                                    LV_CHART_AXIS_PRIMARY_Y);
  cam->ser[2] = lv_chart_add_series(cam->chart,
                                    lv_palette_main(LV_PALETTE_BLUE),
                                    LV_CHART_AXIS_PRIMARY_Y);

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  tof_create(cam, right);
#endif

  card = ui_card(right, LV_SYMBOL_IMAGE " Snapshot", NULL);
  btn = lv_button_create(card);
  lv_obj_set_width(btn, LV_PCT(100));
  lv_obj_set_style_bg_color(btn, COLOR_ACCENT, 0);
  lv_obj_add_event_cb(btn, snap_cb, LV_EVENT_CLICKED, cam);
  label = lv_label_create(btn);
  lv_label_set_text(label, "Save BMP to " CONFIG_EXAMPLES_LVGLCAM_SNAPDIR);
  lv_obj_center(label);
  cam->snap = lv_label_create(card);
  lv_label_set_text(cam->snap, "");
  lv_obj_set_style_text_color(cam->snap, COLOR_DIM, 0);

  lv_timer_create(ui_timer, UI_PERIOD_MS, cam);
}

/****************************************************************************
 * Name: view_show
 *
 * Description:
 *   Place the view overlay window over the view placeholder.
 *
 ****************************************************************************/

static int view_show(FAR struct lvglcam_s *cam)
{
  struct fb_overlayinfo_s oinfo;

  lv_obj_update_layout(lv_screen_active());
  lv_obj_get_coords(cam->image, &cam->view);

  memset(&oinfo, 0, sizeof(oinfo));
  oinfo.overlay = VIEW_OVERLAY;
  oinfo.darea.x = cam->view.x1;
  oinfo.darea.y = cam->view.y1;
  oinfo.darea.w = CAM_WIDTH;
  oinfo.darea.h = CAM_HEIGHT;

  if (ioctl(cam->fbfd, FBIOSET_DESTAREA, (unsigned long)&oinfo) < 0)
    {
      printf("lvglcam: FBIOSET_DESTAREA failed: %d\n", errno);
      return -errno;
    }

  cam->viewset = true;
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct lvglcam_s *cam = &g_cam;
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  pthread_condattr_t cattr;
  struct sigaction act;
  bool lvinit = false;
  int ret;

  if (lv_is_initialized())
    {
      printf("lvglcam: LVGL already initialized\n");
      return EXIT_FAILURE;
    }

  memset(cam, 0, sizeof(*cam));
  memset(&result, 0, sizeof(result));
  cam->vfd  = -1;
  cam->fbfd = -1;
#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  cam->tofd = -1;
#endif
  cam->hold = -1;
  pthread_mutex_init(&cam->lock, NULL);
  pthread_condattr_init(&cattr);
  pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
  pthread_cond_init(&cam->cond, &cattr);
  pthread_condattr_destroy(&cattr);

  /* Stop with SIGINT or SIGTERM */

  g_stop = 0;
  memset(&act, 0, sizeof(act));
  act.sa_handler = lvglcam_signal;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

#ifdef CONFIG_SENSORS
  cam->accfd = open(CONFIG_EXAMPLES_LVGLCAM_ACCEL_DEVPATH,
                    O_RDONLY | O_NONBLOCK);
  if (cam->accfd < 0)
    {
      printf("lvglcam: no accelerometer: %d\n", errno);
    }
  else if (ioctl(cam->accfd, SNIOC_SET_INTERVAL, ACCEL_INTERVAL_US) < 0)
    {
      printf("lvglcam: SNIOC_SET_INTERVAL failed: %d\n", errno);
    }
#endif

  ret = fb_open(cam);
  if (ret < 0)
    {
      goto errout;
    }

  ret = cam_open(cam);
  if (ret < 0)
    {
      goto errout;
    }

  lv_init();
  lvinit = true;

  /* The display must exist before lv_nuttx_init() binds the touchscreen
   * to the default display.
   */

  ret = ui_open(cam);
  if (ret < 0)
    {
      printf("lvglcam: LVGL display initialization failed: %d\n", ret);
      goto errout;
    }

  lv_nuttx_dsc_init(&info);
  info.fb_path = NULL;
#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_LVGLCAM_INPUT_DEVPATH;
#endif

  lv_nuttx_init(&info, &result);

  ui_create(cam);
  ret = view_show(cam);
  if (ret < 0)
    {
      goto errout;
    }

  cam->running = true;
  ret = pthread_create(&cam->camthread, NULL, cam_thread, cam);
  if (ret != 0)
    {
      printf("lvglcam: pthread_create() failed: %d\n", ret);
      ret = -ret;
      goto errout;
    }

  cam->camstarted = true;
  ret = pthread_create(&cam->snapthread, NULL, snap_thread, cam);
  if (ret != 0)
    {
      printf("lvglcam: pthread_create() failed: %d\n", ret);
      ret = -ret;
      goto errout;
    }

  cam->snapstarted = true;

#ifdef CONFIG_EXAMPLES_LVGLCAM_TOF
  /* The depth card is optional: its errors are shown on the card */

  ret = pthread_create(&cam->tofthread, NULL, tof_thread, cam);
  if (ret != 0)
    {
      printf("lvglcam: pthread_create() failed: %d\n", ret);
      ret = -ret;
      goto errout;
    }

  cam->tofstarted = true;
#endif

  while (!g_stop && cam_running(cam))
    {
      uint32_t idle = lv_timer_handler();

      usleep(LV_CLAMP(1, idle, UI_PERIOD_MS) * 1000);
    }

  /* A stop request ends well, a failed camera thread does not */

  ret = g_stop ? OK : -EIO;

errout:
  cam_stop(cam);

  if (lvinit)
    {
      lv_nuttx_deinit(&result);
      ui_close(cam);
      lv_deinit();
    }

  cam_close(cam);
  fb_close(cam);

#ifdef CONFIG_SENSORS
  if (cam->accfd >= 0)
    {
      close(cam->accfd);
    }
#endif

  pthread_cond_destroy(&cam->cond);
  pthread_mutex_destroy(&cam->lock);

  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
