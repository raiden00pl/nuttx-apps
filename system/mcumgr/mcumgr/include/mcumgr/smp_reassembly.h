/****************************************************************************
 * apps/system/mcumgr/mcumgr/include/mcumgr/smp_reassembly.h
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

#ifndef __SYSTEM_MCUMGR_MCUMGR_INCLUDE_MCUMGR_SMP_REASSEMBLY_H
#define __SYSTEM_MCUMGR_MCUMGR_INCLUDE_MCUMGR_SMP_REASSEMBLY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct smp_transport;

/****************************************************************************
 * Public Function Prototyppes
 ****************************************************************************/

/****************************************************************************
 * Name:
 *
 * Description:
 *   Initialize re-assembly context within smp_transport
 *
 * Input Parameters:
 *   smpt - the SMP transport.
 *
 *   Note: for efficiency there is no NULL check on smpt pointer and it is
 *   caller's responsibilityto validate the pointer before passing it to
 *   this function.
 *
 ****************************************************************************/

void smp_reassembly_init(FAR struct smp_transport *smpt);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Collect data to re-assembly buffer
 *
 *   The function adds data to the end of current re-assembly buffer;
 *   it will allocate new buffer if there isn't one allocated.
 *
 *   Note: Currently the function is not able to concatenate buffers so
 *   re-assembled packet needs to fit into one buffer.
 *
 * Input Parameters:
 *   smpt - the SMP transport;
 *   buf  - buffer with data to add;
 *   len  - length of data to add;
 *
 *   Note: For efficiency there are ot NULL checks on smpt and buf pointers
 *   and it is caller's responsibility to make sure these are not NULL.
 *   Also len should not be 0 as there is no point in passing an empty
 *   fragment for re-assembly.
 *
 * Return Value:
 *   number of expected bytes left to complete the packet,
 *
 *   0 means buffer is complete and no more fragments are expected;
 *
 *   -ENOSR if a packet length, read from header, is bigger than
 *   CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE, which means there is no way
 *   to fit it in the configured buffer;
 *
 *   -EOVERFLOW if attempting to add a fragment that would make
 *   complete packet larger than expected;
 *
 *   -ENOMEM if failed to allocate a new buffer for packet assembly;
 *
 *   -ENODATA if the first received fragment was not big enough to
 *   figure out a size of the packet; MTU is set too low;
 *
 ****************************************************************************/

int smp_reassembly_collect(FAR struct smp_transport *smpt,
                           FAR const void *buf, uint16_t len);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Return number of expected bytes to complete the packet
 *
 * Input Parameters:
 *   smpt - the SMP transport;
 *
 *   Note: for efficiency there is no NULL check on smpt pointer and it is
 *   caller's responsibility to validate the pointer before passing it to
 *   this function.
 *
 * Return Value:
 *   number of bytes needed to complete the packet;
 *
 *   -EINVAL if there is no packet in re-assembly;
 *
 ****************************************************************************/

int smp_reassembly_expected(FAR const struct smp_transport *smpt);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Pass packet for further processing
 *
 *   Checks if the packet has enough data to be re-assembled and passes it
 *   for further processing. If successful then the re-assembly context in
 *   smpt will indicate that there is no re-assembly in progress.
 *   The function can be forced to pass a data for processing even if the
 *   packet is not complete, in which case it is users responsibility to
 *   use the user data, passed with the packet, to notify receiving end of
 *   such case.
 *
 * Input Parameters:
 *   smpt  - the SMP transport;
 *   force - process anyway;
 *
 *   Note: for efficiency there is no NULL check on smpt pointer and it is
 *   caller's responsibility to validate the pointer before passing it to
 *   this function.
 *
 * Return Value:
 *   0 on success and not forced;
 *
 *   expected number of bytes if forced to complete buffer with not
 *     enough data;
 *
 *   -EINVAL if there is no re-assembly in progress;
 *
 *   -ENODATA if there is not enough data to consider packet re-assembled,
 *     it has not been passed further.
 *
 ****************************************************************************/

int smp_reassembly_complete(FAR struct smp_transport *smpt, bool force);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Drop packet and release buffer
 *
 * Input Parameters:
 *   smpt - the SMP transport
 *
 *   Note: for efficiency there is no NULL check on smpt pointer and it
 *   is caller's responsibility to validate the pointer before passing it
 *   to this function.
 *
 * Return Value:
 *   -EINVAL if there is no re-assembly in progress.
 *
 ****************************************************************************/

int smp_reassembly_drop(FAR struct smp_transport *smpt);

/****************************************************************************
 * Name:
 *
 * Description:
 *   Get "user data" pointer for current packet re-assembly
 *
 * Input Parameters:
 *   smpt - the SMP transport;
 *
 *   Note: for efficiency there is no NULL check on smpt pointer and it is
 *   caller's responsibility to validate the pointer before passing it to
 *   this function.
 *
 * Return Value:
 *   pointer to "user data" of CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE
 *   size;
 *
 *   NULL if no re-assembly in progress.
 *
 ****************************************************************************/

FAR void *smp_reassembly_get_ud(FAR const struct smp_transport *smpt);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_MCUMGR_MCUMGR_INCLUDE_MCUMGR_SMP_REASSEMBLY_H */
