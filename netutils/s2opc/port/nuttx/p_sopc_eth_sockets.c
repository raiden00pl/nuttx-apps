/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_eth_sockets.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stddef.h>

#include "sopc_eth_sockets.h"

#ifdef CONFIG_S2OPC_ETHERNET

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "p_sopc_sockets.h"
#include "sopc_mem_alloc.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct SOPC_ETH_Socket_ReceiveAddressInfo
{
  struct sockaddr_ll address;
  bool multicast;
  bool filter_destination;
  uint8_t destination[ETH_ALEN];
  bool filter_source;
  uint8_t source[ETH_ALEN];
};

struct SOPC_ETH_Socket_SendAddressInfo
{
  struct sockaddr_ll address;
  uint8_t source[ETH_ALEN];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_eth_hex_value
 ****************************************************************************/

static int s2opc_eth_hex_value(char value)
{
  if (value >= '0' && value <= '9')
    {
      return value - '0';
    }

  if (value >= 'a' && value <= 'f')
    {
      return value - 'a' + 10;
    }

  if (value >= 'A' && value <= 'F')
    {
      return value - 'A' + 10;
    }

  return -1;
}

/****************************************************************************
 * Name: s2opc_eth_would_block
 ****************************************************************************/

static bool s2opc_eth_would_block(int error)
{
#if EWOULDBLOCK == EAGAIN
  return error == EAGAIN;
#else
  return error == EAGAIN || error == EWOULDBLOCK;
#endif
}

/****************************************************************************
 * Name: s2opc_eth_parse_address
 ****************************************************************************/

static bool s2opc_eth_parse_address(FAR uint8_t *address,
                                    FAR const char *value)
{
  int high;
  int low;
  int index;

  if (address == NULL || value == NULL || strlen(value) != 17)
    {
      return false;
    }

  for (index = 0; index < ETH_ALEN; index++)
    {
      high = s2opc_eth_hex_value(value[index * 3]);
      low = s2opc_eth_hex_value(value[index * 3 + 1]);
      if (high < 0 || low < 0 ||
          (index < ETH_ALEN - 1 && value[index * 3 + 2] != '-' &&
           value[index * 3 + 2] != ':'))
        {
          return false;
        }

      address[index] = (uint8_t)((high << 4) | low);
    }

  return true;
}

/****************************************************************************
 * Name: s2opc_eth_get_interface
 ****************************************************************************/

static bool s2opc_eth_get_interface(FAR const char *interface_name,
                                    FAR int *interface_index,
                                    FAR uint8_t *source)
{
  struct ifreq request;
  unsigned int index;
  int socket_fd;
  int ret;

  if (interface_name == NULL || interface_index == NULL)
    {
      return false;
    }

  index = if_nametoindex(interface_name);
  if (index == 0 || index > INT_MAX)
    {
      return false;
    }

  *interface_index = (int)index;
  if (source == NULL)
    {
      return true;
    }

  socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (socket_fd < 0)
    {
      return false;
    }

  memset(&request, 0, sizeof(request));
  strlcpy(request.ifr_name, interface_name, sizeof(request.ifr_name));
  ret = ioctl(socket_fd, SIOCGIFHWADDR,
              (unsigned long)((uintptr_t)&request));
  close(socket_fd);
  if (ret < 0)
    {
      return false;
    }

  memcpy(source, request.ifr_hwaddr.sa_data, ETH_ALEN);
  return true;
}

/****************************************************************************
 * Name: s2opc_eth_set_nonblocking
 ****************************************************************************/

static bool s2opc_eth_set_nonblocking(int socket_fd)
{
  int flags;

  flags = fcntl(socket_fd, F_GETFL, 0);
  if (flags < 0)
    {
      return false;
    }

  return fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

/****************************************************************************
 * Name: s2opc_eth_add_membership
 ****************************************************************************/

static SOPC_ReturnStatus s2opc_eth_add_membership(
    int socket_fd,
    FAR const SOPC_ETH_Socket_ReceiveAddressInfo *receive_address)
{
#if defined(CONFIG_NET_MCASTGROUP) && defined(CONFIG_NET_PKTPROTO_OPTIONS)
  struct packet_mreq request;
  int ret;

  if (!receive_address->filter_destination)
    {
      return SOPC_STATUS_NOT_SUPPORTED;
    }

  memset(&request, 0, sizeof(request));
  request.mr_ifindex = receive_address->address.sll_ifindex;
  request.mr_type = PACKET_MR_MULTICAST;
  request.mr_alen = ETH_ALEN;
  memcpy(request.mr_address, receive_address->destination, ETH_ALEN);

  ret = setsockopt(socket_fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                   &request, sizeof(request));
  return ret == 0 ? SOPC_STATUS_OK : SOPC_STATUS_NOK;
#else
  (void)socket_fd;
  (void)receive_address;
  return SOPC_STATUS_NOT_SUPPORTED;
#endif
}

/****************************************************************************
 * Name: s2opc_eth_create_socket
 ****************************************************************************/

static SOPC_ReturnStatus s2opc_eth_create_socket(
    FAR const struct sockaddr_ll *address, bool nonblocking,
    bool bind_socket, FAR SOPC_Socket *socket_handle)
{
  FAR SOPC_Socket_Impl *socket_impl;

  socket_impl = SOPC_Calloc(1, sizeof(*socket_impl));
  if (socket_impl == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  socket_impl->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  socket_impl->family = AF_PACKET;
  if (socket_impl->sock < 0)
    {
      SOPC_Free(socket_impl);
      return SOPC_STATUS_NOK;
    }

  if ((nonblocking && !s2opc_eth_set_nonblocking(socket_impl->sock)) ||
      (bind_socket &&
       bind(socket_impl->sock, (FAR const struct sockaddr *)address,
            sizeof(*address)) < 0))
    {
      SOPC_Socket_Close(&socket_impl);
      return SOPC_STATUS_NOK;
    }

  *socket_handle = socket_impl;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateSendAddressInfo
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateSendAddressInfo(
    FAR const char *interface_name, FAR const char *destination,
    FAR SOPC_ETH_Socket_SendAddressInfo **send_address)
{
  FAR SOPC_ETH_Socket_SendAddressInfo *address;

  if (interface_name == NULL || destination == NULL ||
      send_address == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *send_address = NULL;
  address = SOPC_Calloc(1, sizeof(*address));
  if (address == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  address->address.sll_family = AF_PACKET;
  address->address.sll_protocol = htons(ETH_P_ALL);
  address->address.sll_halen = ETH_ALEN;

  if (!s2opc_eth_get_interface(interface_name,
                               &address->address.sll_ifindex,
                               address->source) ||
      !s2opc_eth_parse_address(address->address.sll_addr, destination))
    {
      SOPC_Free(address);
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *send_address = address;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateReceiveAddressInfo
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateReceiveAddressInfo(
    FAR const char *interface_name, bool multicast,
    FAR const char *destination, FAR const char *source,
    FAR SOPC_ETH_Socket_ReceiveAddressInfo **receive_address)
{
  FAR SOPC_ETH_Socket_ReceiveAddressInfo *address;

  if (interface_name == NULL || receive_address == NULL)
    {
      return interface_name == NULL ? SOPC_STATUS_NOT_SUPPORTED :
                                      SOPC_STATUS_INVALID_PARAMETERS;
    }

  *receive_address = NULL;
  address = SOPC_Calloc(1, sizeof(*address));
  if (address == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  address->address.sll_family = AF_PACKET;
  address->address.sll_protocol = htons(ETH_P_ALL);
  address->multicast = multicast;
  address->filter_destination = destination != NULL;
  address->filter_source = source != NULL;

  if (!s2opc_eth_get_interface(interface_name,
                               &address->address.sll_ifindex, NULL) ||
      (destination != NULL &&
       !s2opc_eth_parse_address(address->destination, destination)) ||
      (source != NULL &&
       !s2opc_eth_parse_address(address->source, source)) ||
      (multicast && destination != NULL &&
       (address->destination[0] & 1) == 0))
    {
      SOPC_Free(address);
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *receive_address = address;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateToReceive
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateToReceive(
    FAR SOPC_ETH_Socket_ReceiveAddressInfo *receive_address,
    bool nonblocking, FAR SOPC_Socket *socket_handle)
{
  SOPC_ReturnStatus status;

  if (receive_address == NULL || socket_handle == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *socket_handle = SOPC_INVALID_SOCKET;
  status = s2opc_eth_create_socket(&receive_address->address, nonblocking,
                                   true, socket_handle);
  if (status == SOPC_STATUS_OK && receive_address->multicast)
    {
      status = s2opc_eth_add_membership((*socket_handle)->sock,
                                        receive_address);
      if (status != SOPC_STATUS_OK)
        {
          SOPC_ETH_Socket_Close(socket_handle);
        }
    }

  return status;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateToSend
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateToSend(
    FAR SOPC_ETH_Socket_SendAddressInfo *send_address,
    bool nonblocking, FAR SOPC_Socket *socket_handle)
{
  if (send_address == NULL || socket_handle == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *socket_handle = SOPC_INVALID_SOCKET;
  return s2opc_eth_create_socket(&send_address->address, nonblocking,
                                 true, socket_handle);
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_SendTo
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_SendTo(
    SOPC_Socket socket_handle,
    FAR const SOPC_ETH_Socket_SendAddressInfo *send_address,
    uint16_t ethertype, FAR SOPC_Buffer *buffer)
{
  FAR SOPC_Buffer *frame;
  SOPC_ReturnStatus status;
  uint16_t network_ethertype;
  ssize_t sent;
  int error;

  if (socket_handle == SOPC_INVALID_SOCKET || send_address == NULL ||
      buffer == NULL || buffer->position != 0)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  frame = SOPC_Buffer_Create(ETHERNET_HEADER_SIZE + buffer->length);
  if (frame == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  status = SOPC_Buffer_Write(frame, send_address->address.sll_addr,
                             ETH_ALEN);
  if (status == SOPC_STATUS_OK)
    {
      status = SOPC_Buffer_Write(frame, send_address->source, ETH_ALEN);
    }

  if (status == SOPC_STATUS_OK)
    {
      network_ethertype = htons(ethertype);
      status = SOPC_Buffer_Write(frame,
                                 (FAR const uint8_t *)&network_ethertype,
                                 sizeof(network_ethertype));
    }

  if (status == SOPC_STATUS_OK)
    {
      status = SOPC_Buffer_Write(frame, buffer->data, buffer->length);
    }

  if (status == SOPC_STATUS_OK)
    {
      do
        {
          sent = send(socket_handle->sock, frame->data, frame->length, 0);
        }
      while (sent < 0 && errno == EINTR);

      error = errno;
      if (sent < 0)
        {
          status = s2opc_eth_would_block(error) ?
                   SOPC_STATUS_WOULD_BLOCK : SOPC_STATUS_NOK;
        }
      else if ((uint32_t)sent != frame->length)
        {
          status = SOPC_STATUS_WOULD_BLOCK;
        }
    }

  SOPC_Buffer_Delete(frame);
  return status;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_ReceiveFrom
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_ReceiveFrom(
    SOPC_Socket socket_handle,
    FAR const SOPC_ETH_Socket_ReceiveAddressInfo *receive_address,
    bool check_ethertype, uint16_t ethertype, FAR SOPC_Buffer *buffer)
{
  uint16_t network_ethertype;
  ssize_t received;

  if (socket_handle == SOPC_INVALID_SOCKET || receive_address == NULL ||
      buffer == NULL || buffer->current_size < ETHERNET_HEADER_SIZE)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  for (; ; )
    {
      do
        {
          received = recv(socket_handle->sock, buffer->data,
                          buffer->current_size, 0);
        }
      while (received < 0 && errno == EINTR);

      if (received < 0)
        {
          return s2opc_eth_would_block(errno) ?
                 SOPC_STATUS_WOULD_BLOCK : SOPC_STATUS_NOK;
        }

      buffer->length = (uint32_t)received;
      if (buffer->length < ETHERNET_HEADER_SIZE)
        {
          return SOPC_STATUS_NOK;
        }

      if (receive_address->filter_destination &&
          memcmp(receive_address->destination, buffer->data,
                 ETH_ALEN) != 0)
        {
          continue;
        }

      if (receive_address->filter_source &&
          memcmp(receive_address->source, buffer->data + ETH_ALEN,
                 ETH_ALEN) != 0)
        {
          continue;
        }

      memcpy(&network_ethertype, buffer->data + 2 * ETH_ALEN,
             sizeof(network_ethertype));
      if (check_ethertype && ntohs(network_ethertype) != ethertype)
        {
          continue;
        }

      return buffer->length == buffer->current_size ?
             SOPC_STATUS_OUT_OF_MEMORY : SOPC_STATUS_OK;
    }
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_Close
 ****************************************************************************/

void SOPC_ETH_Socket_Close(FAR SOPC_Socket *socket_handle)
{
  SOPC_Socket_Close(socket_handle);
}

#else /* CONFIG_S2OPC_ETHERNET */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateSendAddressInfo
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateSendAddressInfo(
    FAR const char *interface_name, FAR const char *destination,
    FAR SOPC_ETH_Socket_SendAddressInfo **send_address)
{
  (void)interface_name;
  (void)destination;
  (void)send_address;
  return SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateReceiveAddressInfo
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateReceiveAddressInfo(
    FAR const char *interface_name, bool multicast,
    FAR const char *destination, FAR const char *source,
    FAR SOPC_ETH_Socket_ReceiveAddressInfo **receive_address)
{
  (void)interface_name;
  (void)multicast;
  (void)destination;
  (void)source;
  (void)receive_address;
  return SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateToReceive
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateToReceive(
    FAR SOPC_ETH_Socket_ReceiveAddressInfo *receive_address,
    bool nonblocking, FAR SOPC_Socket *socket_handle)
{
  (void)receive_address;
  (void)nonblocking;
  (void)socket_handle;
  return SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_CreateToSend
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_CreateToSend(
    FAR SOPC_ETH_Socket_SendAddressInfo *send_address,
    bool nonblocking, FAR SOPC_Socket *socket_handle)
{
  (void)send_address;
  (void)nonblocking;
  (void)socket_handle;
  return SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_SendTo
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_SendTo(
    SOPC_Socket socket_handle,
    FAR const SOPC_ETH_Socket_SendAddressInfo *send_address,
    uint16_t ethertype, FAR SOPC_Buffer *buffer)
{
  (void)socket_handle;
  (void)send_address;
  (void)ethertype;
  (void)buffer;
  return SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_ReceiveFrom
 ****************************************************************************/

SOPC_ReturnStatus SOPC_ETH_Socket_ReceiveFrom(
    SOPC_Socket socket_handle,
    FAR const SOPC_ETH_Socket_ReceiveAddressInfo *receive_address,
    bool check_ethertype, uint16_t ethertype, FAR SOPC_Buffer *buffer)
{
  (void)socket_handle;
  (void)receive_address;
  (void)check_ethertype;
  (void)ethertype;
  (void)buffer;
  return SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Name: SOPC_ETH_Socket_Close
 ****************************************************************************/

void SOPC_ETH_Socket_Close(FAR SOPC_Socket *socket_handle)
{
  if (socket_handle != NULL)
    {
      *socket_handle = SOPC_INVALID_SOCKET;
    }
}

#endif /* CONFIG_S2OPC_ETHERNET */
