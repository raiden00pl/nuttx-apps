/****************************************************************************
 * apps/mgmt/mcumgr/mcumgr/smp_buf.h
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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

int smp_buf_init(size_t count, size_t size, size_t usize)
{

}

/****************************************************************************
 * Public Function
 ****************************************************************************/

/****************************************************************************
 * Name: smp_buf_reset
 *
 * Description:
 *   Reset buffer.
 *   Reset buffer data and flags so it can be reused for other purposes.
 *
 * Input Parameters:
 *   buf - Buffer to reset.
 *
 * Return Value:
 *
 ****************************************************************************/

void smp_buf_reset(FAR struct smp_buf *buf)
{

}

/****************************************************************************
 * Name: smp_buf_tailroom
 *
 * Description:
 *   Check buffer tailroom.
 *   Check how much free space there is at the end of the buffer.
 *
 * Input Parameters:
 *   buf - A valid pointer on a buffer
 *
 * Return Value:
 *   Number of bytes available at the end of the buffer.
 *
 ****************************************************************************/

int smp_buf_tailroom(FAR struct smp_buf *nb)
{

}

/****************************************************************************
 * Name: smp_buf_pull
 *
 * Description:
 *   Remove data from the beginning of the buffer.
 *   Removes data from the beginning of the buffer by modifying the data
 *   pointer and buffer length.
 *
 * Input Parameters:
 *   buf - Buffer to update.
 *   len - Number of bytes to remove.
 *
 * Return Value:
 *   New beginning of the buffer data.
 *
 ****************************************************************************/

FAR void *smp_buf_pull(FAR struct smp_buf *buf, size_t len)
{

}

/****************************************************************************
 * Name: smp_buf_alloc
 *
 * Description:
 *  Allocate a new buffer from a pool.
 *
 * Input Parameters:
 *   pool    - Which pool to allocate the buffer from.
 *   timeout - Affects the action taken should the pool be empty.
 *             If K_NO_WAIT, then return immediately. If K_FOREVER, then
 *             wait as long as necessary. Otherwise, wait until the specified
 *             timeout.
 *
 * Return Value:
 *   New buffer or NULL if out of buffers.
 *
 ****************************************************************************/

FAR struct smp_buf *smp_buf_alloc(FAR struct smp_buf_pool *pool,
                                  k_timeout_t timeout)
{

}

/****************************************************************************
 * Name: smp_buf_get
 *
 * Description:
 *   Get a buffer from a FIFO.
 *
 * Input Parameters:
 *   fifo    - Which FIFO to take the buffer from.
 *   timeout - Affects the action taken should the FIFO be empty.
 *             If K_NO_WAIT, then return immediately. If K_FOREVER, then wait as
 *             long as necessary. Otherwise, wait until the specified timeout.
 *
 * Return Value:
 *   New buffer or NULL if the FIFO is empty.
 *
 ****************************************************************************/

FAR struct smp_buf *smp_buf_get(FAR struct k_fifo *fifo,
                                k_timeout_t timeout, FAR const char *func,
                                int line)
{

}

/****************************************************************************
 * Name: smp_buf_unref
 *
 * Description:
 *   Decrements the reference count of a buffer.
 *   The buffer is put back into the pool if the reference count reaches zero.
 *
 * Input Parameters:
 *   buf - A valid pointer on a buffer
 *
 ****************************************************************************/

void smp_buf_unref(FAR struct smp_buf *buf)
{

}

/****************************************************************************
 * Name: smp_buf_user_data
 *
 * Description:
 *   Get a pointer to the user data of a buffer.
 *
 * Input Parameters:
 * buf - A valid pointer on a buffer
 *
 * Return Value:
 *   Pointer to the user data of the buffer.
 *
 ****************************************************************************/

FAR void *smp_buf_user_data(FAR const struct smp_buf *buf)
{
}

/****************************************************************************
 * Name: smp_buf_put
 *
 * Description:
 *  Put a buffer to the end of a FIFO.
 *
 * Input Parameters:
 *   fifo - Which FIFO to put the buffer to.
 *   buf  - Buffer.
 *
 ****************************************************************************/

void smp_buf_put(FAR struct k_fifo *fifo, FAR struct smp_buf *buf)
{

}

/****************************************************************************
 * Name: smp_buf_add_mem
 *
 * Description:
 *   Copies the given number of bytes to the end of the buffer
 *   Increments the data length of the  buffer to account for more data at
 *   the end.
 *
 * Input Parameters:
 *   buf - Buffer to update.
 *   mem - Location of data to be added.
 *   len - Length of data to be added
 *
 * Return Value:
 *   The original tail of the buffer.
 *
 ****************************************************************************/

FAR void *smp_buf_add_mem(FAR struct smp_buf *buf, FAR const void *mem,
                          size_t len)
{

}
