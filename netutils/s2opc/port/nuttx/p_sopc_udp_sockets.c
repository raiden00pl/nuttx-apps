/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_udp_sockets.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to Systerel under one or more contributor license agreements.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.  Systerel licenses this file to you under
 * the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the
 * License at
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

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#ifdef CONFIG_NETDEV_IFINDEX
#  include <ifaddrs.h>
#  include <net/if.h>
#endif

#include "p_sopc_sockets.h"
#include "sopc_buffer.h"
#include "sopc_mem_alloc.h"
#include "sopc_udp_sockets.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_udp_would_block
 ****************************************************************************/

static bool s2opc_udp_would_block(int error)
{
  return error == EAGAIN;
}

/****************************************************************************
 * Name: s2opc_udp_resolve
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_udp_resolve(bool ipv6, FAR const char *node, FAR const char *port,
                  FAR SOPC_Socket_AddressInfo **addrs)
{
  SOPC_Socket_AddressInfo hints;
  FAR struct addrinfo *result;

  if ((node == NULL && port == NULL) || addrs == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *addrs = NULL;
  memset(&hints, 0, sizeof(hints));
  hints.addrinfo.ai_family = ipv6 ? AF_INET6 : AF_INET;
  hints.addrinfo.ai_socktype = SOCK_DGRAM;
  hints.addrinfo.ai_protocol = IPPROTO_UDP;
  hints.addrinfo.ai_flags = AI_PASSIVE;

  result = NULL;
  if (getaddrinfo(node, port, &hints.addrinfo, &result) != 0)
    {
      return SOPC_STATUS_NOK;
    }

  *addrs = (FAR SOPC_Socket_AddressInfo *)result;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: s2opc_udp_bind_device
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_udp_bind_device(int sock, FAR const char *interface_name)
{
  if (interface_name == NULL)
    {
      return SOPC_STATUS_OK;
    }

#ifdef CONFIG_NET_BINDTODEVICE
  if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface_name,
                 strlen(interface_name) + 1) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  return SOPC_STATUS_OK;
#else
  return SOPC_STATUS_NOT_SUPPORTED;
#endif
}

/****************************************************************************
 * Name: s2opc_udp_create
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_udp_create(FAR const SOPC_Socket_AddressInfo *addr,
                 FAR const char *interface_name, bool reuse,
                 bool nonblocking, FAR SOPC_Socket *sock)
{
  FAR SOPC_Socket_Impl *implementation;
  int enabled = 1;
  int flags;

  if (addr == NULL || sock == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *sock = SOPC_INVALID_SOCKET;
  implementation = SOPC_Calloc(1, sizeof(*implementation));
  if (implementation == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  implementation->sock = socket(addr->addrinfo.ai_family,
                                addr->addrinfo.ai_socktype,
                                addr->addrinfo.ai_protocol);
  implementation->family = addr->addrinfo.ai_family;
  if (implementation->sock < 0)
    {
      SOPC_Free(implementation);
      return SOPC_STATUS_NOK;
    }

  if ((reuse &&
       setsockopt(implementation->sock, SOL_SOCKET, SO_REUSEADDR, &enabled,
                  sizeof(enabled)) < 0) ||
      s2opc_udp_bind_device(implementation->sock, interface_name) !=
      SOPC_STATUS_OK)
    {
      SOPC_Socket_Close(&implementation);
      return SOPC_STATUS_NOK;
    }

  if (nonblocking)
    {
      flags = fcntl(implementation->sock, F_GETFL);
      if (flags < 0 || fcntl(implementation->sock, F_SETFL,
                             flags | O_NONBLOCK) < 0)
        {
          SOPC_Socket_Close(&implementation);
          return SOPC_STATUS_NOK;
        }
    }

  *sock = implementation;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: s2opc_udp_is_multicast
 ****************************************************************************/

static bool
s2opc_udp_is_multicast(FAR const SOPC_Socket_AddressInfo *address)
{
  FAR const struct sockaddr_in *addr4;
  FAR const struct sockaddr_in6 *addr6;

  if (address->addrinfo.ai_family == AF_INET)
    {
      addr4 = (FAR const struct sockaddr_in *)address->addrinfo.ai_addr;
      return IN_MULTICAST(ntohl(addr4->sin_addr.s_addr));
    }

  if (address->addrinfo.ai_family == AF_INET6)
    {
      addr6 = (FAR const struct sockaddr_in6 *)address->addrinfo.ai_addr;
      return IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr);
    }

  return false;
}

/****************************************************************************
 * Name: s2opc_udp_ipv4_interface
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_udp_ipv4_interface(FAR const char *interface_name,
                         FAR struct in_addr *address)
{
#ifdef CONFIG_NETDEV_IFINDEX
  FAR struct ifaddrs *interfaces;
  FAR struct ifaddrs *current;
  FAR struct sockaddr_in *addr4;
#endif

  address->s_addr = htonl(INADDR_ANY);
  if (interface_name == NULL)
    {
      return SOPC_STATUS_OK;
    }

#ifdef CONFIG_NETDEV_IFINDEX
  interfaces = NULL;
  if (getifaddrs(&interfaces) < 0)
    {
      return SOPC_STATUS_NOT_SUPPORTED;
    }

  for (current = interfaces; current != NULL; current = current->ifa_next)
    {
      if (current->ifa_addr != NULL &&
          current->ifa_addr->sa_family == AF_INET &&
          strcmp(current->ifa_name, interface_name) == 0)
        {
          addr4 = (FAR struct sockaddr_in *)current->ifa_addr;
          *address = addr4->sin_addr;
          freeifaddrs(interfaces);
          return SOPC_STATUS_OK;
        }
    }

  freeifaddrs(interfaces);
  return SOPC_STATUS_NOK;
#else
  return SOPC_STATUS_NOT_SUPPORTED;
#endif
}

/****************************************************************************
 * Name: s2opc_udp_join_multicast
 ****************************************************************************/

static SOPC_ReturnStatus
s2opc_udp_join_multicast(int sock, FAR const char *interface_name,
                         FAR const SOPC_Socket_AddressInfo *multicast)
{
  FAR const struct sockaddr_in *addr4;
  FAR const struct sockaddr_in6 *addr6;
  struct ipv6_mreq request6;
  struct ip_mreq request4;
  SOPC_ReturnStatus status;

  if (multicast->addrinfo.ai_family == AF_INET6)
    {
      addr6 = (FAR const struct sockaddr_in6 *)multicast->addrinfo.ai_addr;
      memset(&request6, 0, sizeof(request6));
      request6.ipv6mr_multiaddr = addr6->sin6_addr;
      if (interface_name != NULL)
        {
#ifdef CONFIG_NETDEV_IFINDEX
          request6.ipv6mr_interface = if_nametoindex(interface_name);
          if (request6.ipv6mr_interface == 0)
            {
              return SOPC_STATUS_NOK;
            }
#else
          return SOPC_STATUS_NOT_SUPPORTED;
#endif
        }

      return setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &request6,
                        sizeof(request6)) == 0 ? SOPC_STATUS_OK :
             SOPC_STATUS_NOT_SUPPORTED;
    }

  if (multicast->addrinfo.ai_family != AF_INET)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  addr4 = (FAR const struct sockaddr_in *)multicast->addrinfo.ai_addr;
  memset(&request4, 0, sizeof(request4));
  request4.imr_multiaddr = addr4->sin_addr;
  status = s2opc_udp_ipv4_interface(interface_name,
                                    &request4.imr_interface);
  if (status != SOPC_STATUS_OK)
    {
      return status;
    }

  return setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &request4,
                    sizeof(request4)) == 0 ? SOPC_STATUS_OK :
         SOPC_STATUS_NOT_SUPPORTED;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_UDP_SocketAddress_Create
 ****************************************************************************/

FAR SOPC_Socket_AddressInfo *
SOPC_UDP_SocketAddress_Create(bool ipv6, FAR const char *node,
                              FAR const char *service)
{
  FAR SOPC_Socket_AddressInfo *address = NULL;

  if (s2opc_udp_resolve(ipv6, node, service, &address) != SOPC_STATUS_OK)
    {
      return NULL;
    }

  return address;
}

/****************************************************************************
 * Name: SOPC_UDP_SocketAddress_Delete
 ****************************************************************************/

void SOPC_UDP_SocketAddress_Delete(FAR SOPC_Socket_AddressInfo **address)
{
  SOPC_Socket_AddrInfoDelete(address);
}

/****************************************************************************
 * Name: SOPC_UDP_Socket_Set_MulticastTTL
 ****************************************************************************/

SOPC_ReturnStatus SOPC_UDP_Socket_Set_MulticastTTL(SOPC_Socket sock,
                                                   uint8_t scope)
{
  int value = scope;
  int level;
  int option;

  if (sock == SOPC_INVALID_SOCKET)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  if (sock->family == AF_INET6)
    {
      level = IPPROTO_IPV6;
      option = IPV6_MULTICAST_HOPS;
    }
  else
    {
      level = IPPROTO_IP;
      option = IP_MULTICAST_TTL;
    }

  return setsockopt(sock->sock, level, option, &value, sizeof(value)) == 0 ?
         SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_UDP_Socket_CreateToReceive
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_UDP_Socket_CreateToReceive(FAR SOPC_Socket_AddressInfo *listen_address,
                                FAR const char *interface_name, bool reuse,
                                bool nonblocking, FAR SOPC_Socket *sock)
{
  SOPC_ReturnStatus status;

  status = s2opc_udp_create(listen_address, interface_name, reuse,
                            nonblocking, sock);
  if (status != SOPC_STATUS_OK)
    {
      return status;
    }

  if (bind((*sock)->sock, listen_address->addrinfo.ai_addr,
           listen_address->addrinfo.ai_addrlen) < 0)
    {
      SOPC_UDP_Socket_Close(sock);
      return SOPC_STATUS_NOK;
    }

  if (s2opc_udp_is_multicast(listen_address))
    {
      status = s2opc_udp_join_multicast((*sock)->sock, interface_name,
                                        listen_address);
      if (status != SOPC_STATUS_OK)
        {
          SOPC_UDP_Socket_Close(sock);
        }
    }

  return status;
}

/****************************************************************************
 * Name: SOPC_UDP_Socket_CreateToSend
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_UDP_Socket_CreateToSend(FAR SOPC_Socket_AddressInfo *destination,
                             FAR const char *interface_name,
                             bool nonblocking, FAR SOPC_Socket *sock)
{
  return s2opc_udp_create(destination, interface_name, false, nonblocking,
                          sock);
}

/****************************************************************************
 * Name: SOPC_UDP_Socket_SendTo
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_UDP_Socket_SendTo(SOPC_Socket sock,
                       FAR const SOPC_Socket_AddressInfo *destination,
                       FAR SOPC_Buffer *buffer)
{
  ssize_t ret;
  int error;

  if (sock == SOPC_INVALID_SOCKET || destination == NULL || buffer == NULL ||
      buffer->position != 0)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  do
    {
      ret = sendto(sock->sock, buffer->data, buffer->length, 0,
                   destination->addrinfo.ai_addr,
                   destination->addrinfo.ai_addrlen);
    }
  while (ret < 0 && errno == EINTR);

  error = errno;
  if (ret >= 0 && (uint32_t)ret == buffer->length)
    {
      return SOPC_STATUS_OK;
    }

  return ret < 0 && s2opc_udp_would_block(error) ?
         SOPC_STATUS_WOULD_BLOCK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_UDP_Socket_ReceiveFrom
 ****************************************************************************/

SOPC_ReturnStatus SOPC_UDP_Socket_ReceiveFrom(SOPC_Socket sock,
                                              FAR SOPC_Buffer *buffer)
{
  struct sockaddr_storage source;
  socklen_t source_length;
  ssize_t ret;
  int error;

  if (sock == SOPC_INVALID_SOCKET || buffer == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  source_length = sizeof(source);
  do
    {
      ret = recvfrom(sock->sock, buffer->data, buffer->current_size, 0,
                     (FAR struct sockaddr *)&source, &source_length);
    }
  while (ret < 0 && errno == EINTR);

  error = errno;
  if (ret < 0)
    {
      return s2opc_udp_would_block(error) ? SOPC_STATUS_WOULD_BLOCK :
             SOPC_STATUS_NOK;
    }

  buffer->length = ret;
  return buffer->length == buffer->current_size ?
         SOPC_STATUS_OUT_OF_MEMORY : SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_UDP_Socket_Close
 ****************************************************************************/

void SOPC_UDP_Socket_Close(FAR SOPC_Socket *sock)
{
  SOPC_Socket_Close(sock);
}
