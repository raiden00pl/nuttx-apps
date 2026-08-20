/****************************************************************************
 * apps/netutils/s2opc/port/nuttx/p_sopc_sockets.c
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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "p_sopc_sockets.h"
#include "sopc_mem_alloc.h"
#include "sopc_raw_sockets.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define S2OPC_DSEC_PER_SEC 10

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s2opc_socket_configure
 ****************************************************************************/

static SOPC_ReturnStatus s2opc_socket_configure(int sock, bool nonblocking)
{
  int enabled = 1;
  int flags;

  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enabled,
                 sizeof(enabled)) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  if (!nonblocking)
    {
      return SOPC_STATUS_OK;
    }

  flags = fcntl(sock, F_GETFL);
  if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: s2opc_socket_would_block
 ****************************************************************************/

static bool s2opc_socket_would_block(int error)
{
#if EWOULDBLOCK == EAGAIN
  return error == EAGAIN;
#else
  return error == EAGAIN || error == EWOULDBLOCK;
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: SOPC_Socket_Network_Enable_Keepalive
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Socket_Network_Enable_Keepalive(SOPC_Socket sock,
                                     unsigned int first_probe_delay,
                                     unsigned int interval,
                                     unsigned int counter)
{
  int enabled = 1;
  int idle;
  int keep_interval;
  int keep_count;

  if (sock == SOPC_INVALID_SOCKET ||
      first_probe_delay > UINT16_MAX / S2OPC_DSEC_PER_SEC ||
      interval > UINT16_MAX / S2OPC_DSEC_PER_SEC || counter > UINT8_MAX)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  /* NuttX expresses TCP_KEEPIDLE and TCP_KEEPINTVL in deciseconds. */

  idle = first_probe_delay * S2OPC_DSEC_PER_SEC;
  keep_interval = interval * S2OPC_DSEC_PER_SEC;
  keep_count = counter;

  if (setsockopt(sock->sock, SOL_SOCKET, SO_KEEPALIVE, &enabled,
                 sizeof(enabled)) < 0 ||
      setsockopt(sock->sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle,
                 sizeof(idle)) < 0 ||
      setsockopt(sock->sock, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval,
                 sizeof(keep_interval)) < 0 ||
      setsockopt(sock->sock, IPPROTO_TCP, TCP_KEEPCNT, &keep_count,
                 sizeof(keep_count)) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_Network_Disable_Keepalive
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Socket_Network_Disable_Keepalive(SOPC_Socket sock)
{
  int enabled = 0;

  if (sock == SOPC_INVALID_SOCKET)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  return setsockopt(sock->sock, SOL_SOCKET, SO_KEEPALIVE, &enabled,
                    sizeof(enabled)) == 0 ? SOPC_STATUS_OK : SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Socket_Network_Initialize
 ****************************************************************************/

bool SOPC_Socket_Network_Initialize(void)
{
  return true;
}

/****************************************************************************
 * Name: SOPC_Socket_Network_Clear
 ****************************************************************************/

bool SOPC_Socket_Network_Clear(void)
{
  return true;
}

/****************************************************************************
 * Name: SOPC_Socket_AddrInfo_Get
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Socket_AddrInfo_Get(FAR const char *hostname, FAR const char *port,
                         FAR SOPC_Socket_AddressInfo **addrs)
{
  SOPC_Socket_AddressInfo hints;
  FAR struct addrinfo *result;
  int ret;

  if ((hostname == NULL && port == NULL) || addrs == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *addrs = NULL;
  memset(&hints, 0, sizeof(hints));

  /* S2OPC opens one wildcard listener and assumes an IPv6 listener accepts
   * IPv4 connections.  NuttX does not implement IPV6_V6ONLY or dual-stack
   * TCP listeners, so use the IPv4 wildcard when IPv4 is available.  An
   * explicit IPv6 address continues to resolve through AF_UNSPEC.
   */

#ifdef CONFIG_NET_IPv4
  hints.addrinfo.ai_family = hostname == NULL ? AF_INET : AF_UNSPEC;
#else
  hints.addrinfo.ai_family = AF_UNSPEC;
#endif
  hints.addrinfo.ai_socktype = SOCK_STREAM;
  hints.addrinfo.ai_flags = AI_PASSIVE;

  result = NULL;
  ret = getaddrinfo(hostname, port, &hints.addrinfo, &result);
  if (ret != 0)
    {
      return SOPC_STATUS_NOK;
    }

  *addrs = (FAR SOPC_Socket_AddressInfo *)result;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_AddrInfo_IterNext
 ****************************************************************************/

FAR SOPC_Socket_AddressInfo *
SOPC_Socket_AddrInfo_IterNext(FAR SOPC_Socket_AddressInfo *addr)
{
  if (addr == NULL)
    {
      return NULL;
    }

  return (FAR SOPC_Socket_AddressInfo *)addr->addrinfo.ai_next;
}

/****************************************************************************
 * Name: SOPC_Socket_AddrInfo_IsIPV6
 ****************************************************************************/

uint8_t
SOPC_Socket_AddrInfo_IsIPV6(FAR const SOPC_Socket_AddressInfo *addr)
{
  return addr != NULL && addr->addrinfo.ai_family == AF_INET6;
}

/****************************************************************************
 * Name: SOPC_Socket_AddrInfoDelete
 ****************************************************************************/

void SOPC_Socket_AddrInfoDelete(FAR SOPC_Socket_AddressInfo **addrs)
{
  if (addrs != NULL && *addrs != NULL)
    {
      freeaddrinfo(&(*addrs)->addrinfo);
      *addrs = NULL;
    }
}

/****************************************************************************
 * Name: SOPC_SocketAddress_Delete
 ****************************************************************************/

void SOPC_SocketAddress_Delete(FAR SOPC_Socket_Address **addr)
{
  if (addr != NULL && *addr != NULL)
    {
      SOPC_Free((*addr)->address.ai_addr);
      SOPC_Free(*addr);
      *addr = NULL;
    }
}

/****************************************************************************
 * Name: SOPC_Socket_CopyAddress
 ****************************************************************************/

FAR SOPC_Socket_Address *
SOPC_Socket_CopyAddress(FAR SOPC_Socket_AddressInfo *addr)
{
  FAR SOPC_Socket_Address *copy;

  if (addr == NULL || addr->addrinfo.ai_addr == NULL)
    {
      return NULL;
    }

  copy = SOPC_Calloc(1, sizeof(*copy));
  if (copy == NULL)
    {
      return NULL;
    }

  copy->address.ai_addr = SOPC_Malloc(addr->addrinfo.ai_addrlen);
  if (copy->address.ai_addr == NULL)
    {
      SOPC_Free(copy);
      return NULL;
    }

  memcpy(copy->address.ai_addr, addr->addrinfo.ai_addr,
         addr->addrinfo.ai_addrlen);
  copy->address.ai_addrlen = addr->addrinfo.ai_addrlen;
  copy->address.ai_family = addr->addrinfo.ai_family;
  return copy;
}

/****************************************************************************
 * Name: SOPC_Socket_GetPeerAddress
 ****************************************************************************/

FAR SOPC_Socket_Address *SOPC_Socket_GetPeerAddress(SOPC_Socket sock)
{
  FAR SOPC_Socket_Address *address;
  FAR struct sockaddr_storage *storage;
  socklen_t length;

  if (sock == SOPC_INVALID_SOCKET)
    {
      return NULL;
    }

  address = SOPC_Calloc(1, sizeof(*address));
  storage = SOPC_Calloc(1, sizeof(*storage));
  if (address == NULL || storage == NULL)
    {
      SOPC_Free(storage);
      SOPC_Free(address);
      return NULL;
    }

  length = sizeof(*storage);
  if (getpeername(sock->sock, (FAR struct sockaddr *)storage, &length) < 0)
    {
      SOPC_Free(storage);
      SOPC_Free(address);
      return NULL;
    }

  address->address.ai_family = storage->ss_family;
  address->address.ai_addrlen = length;
  address->address.ai_addr = (FAR struct sockaddr *)storage;
  return address;
}

/****************************************************************************
 * Name: SOPC_SocketAddress_GetNameInfo
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_SocketAddress_GetNameInfo(FAR const SOPC_Socket_Address *addr,
                               FAR char **host, FAR char **service)
{
  FAR char *host_result = NULL;
  FAR char *service_result = NULL;
  socklen_t host_length = 0;
  socklen_t service_length = 0;
  int flags = 0;
  int ret;

  if (addr == NULL || (host == NULL && service == NULL))
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  if (host != NULL)
    {
      *host = NULL;
      host_result = SOPC_Calloc(NI_MAXHOST, sizeof(char));
      if (host_result == NULL)
        {
          return SOPC_STATUS_OUT_OF_MEMORY;
        }

      host_length = NI_MAXHOST;
      flags |= NI_NUMERICHOST;
    }

  if (service != NULL)
    {
      *service = NULL;
      service_result = SOPC_Calloc(NI_MAXSERV, sizeof(char));
      if (service_result == NULL)
        {
          SOPC_Free(host_result);
          return SOPC_STATUS_OUT_OF_MEMORY;
        }

      service_length = NI_MAXSERV;
      flags |= NI_NUMERICSERV;
    }

  ret = getnameinfo(addr->address.ai_addr, addr->address.ai_addrlen,
                    host_result, host_length, service_result,
                    service_length, flags);
  if (ret != 0)
    {
      SOPC_Free(host_result);
      SOPC_Free(service_result);
      return SOPC_STATUS_NOK;
    }

  if (host != NULL)
    {
      *host = host_result;
    }

  if (service != NULL)
    {
      *service = service_result;
    }

  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_Clear
 ****************************************************************************/

void SOPC_Socket_Clear(FAR SOPC_Socket *sock)
{
  if (sock != NULL)
    {
      *sock = SOPC_INVALID_SOCKET;
    }
}

/****************************************************************************
 * Name: SOPC_Socket_CreateNew
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Socket_CreateNew(FAR SOPC_Socket_AddressInfo *addr, bool reuse,
                      bool nonblocking, FAR SOPC_Socket *sock)
{
  FAR SOPC_Socket_Impl *implementation;
  int enabled = 1;

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
  if (implementation->sock < 0 ||
      s2opc_socket_configure(implementation->sock, nonblocking) !=
      SOPC_STATUS_OK ||
      (reuse && setsockopt(implementation->sock, SOL_SOCKET, SO_REUSEADDR,
                           &enabled, sizeof(enabled)) < 0))
    {
      if (implementation->sock >= 0)
        {
          close(implementation->sock);
        }

      SOPC_Free(implementation);
      return SOPC_STATUS_NOK;
    }

  *sock = implementation;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_Listen
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_Listen(SOPC_Socket sock,
                                     FAR SOPC_Socket_AddressInfo *addr)
{
  if (sock == SOPC_INVALID_SOCKET || addr == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  if (bind(sock->sock, addr->addrinfo.ai_addr,
           addr->addrinfo.ai_addrlen) < 0 ||
      listen(sock->sock, SOMAXCONN) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_Accept
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_Accept(SOPC_Socket listening_sock,
                                     bool nonblocking,
                                     FAR SOPC_Socket *accepted_sock)
{
  FAR SOPC_Socket_Impl *implementation;
  struct sockaddr_storage remote;
  socklen_t length;
  int ret;

  if (listening_sock == SOPC_INVALID_SOCKET || accepted_sock == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  *accepted_sock = SOPC_INVALID_SOCKET;
  implementation = SOPC_Calloc(1, sizeof(*implementation));
  if (implementation == NULL)
    {
      return SOPC_STATUS_OUT_OF_MEMORY;
    }

  length = sizeof(remote);
  do
    {
      ret = accept(listening_sock->sock, (FAR struct sockaddr *)&remote,
                   &length);
    }
  while (ret < 0 && errno == EINTR);

  implementation->sock = ret;
  if (ret < 0 || s2opc_socket_configure(ret, nonblocking) != SOPC_STATUS_OK)
    {
      if (ret >= 0)
        {
          close(ret);
        }

      SOPC_Free(implementation);
      return SOPC_STATUS_NOK;
    }

  implementation->family = remote.ss_family;
  *accepted_sock = implementation;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_Connect
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_Connect(SOPC_Socket sock,
                                      FAR SOPC_Socket_AddressInfo *addr)
{
  int ret;
  int error;

  if (sock == SOPC_INVALID_SOCKET || addr == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  do
    {
      ret = connect(sock->sock, addr->addrinfo.ai_addr,
                    addr->addrinfo.ai_addrlen);
    }
  while (ret < 0 && errno == EINTR);

  error = errno;
  if (ret == 0 || error == EISCONN || error == EINPROGRESS ||
      error == EALREADY)
    {
      return SOPC_STATUS_OK;
    }

  return SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Socket_ConnectToLocal
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_ConnectToLocal(SOPC_Socket from,
                                             SOPC_Socket to)
{
  SOPC_Socket_AddressInfo addr;
  struct sockaddr_storage storage;

  if (from == SOPC_INVALID_SOCKET || to == SOPC_INVALID_SOCKET)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  memset(&addr, 0, sizeof(addr));
  memset(&storage, 0, sizeof(storage));
  addr.addrinfo.ai_addr = (FAR struct sockaddr *)&storage;
  addr.addrinfo.ai_addrlen = sizeof(storage);
  if (getsockname(to->sock, addr.addrinfo.ai_addr,
                  &addr.addrinfo.ai_addrlen) < 0)
    {
      return SOPC_STATUS_NOK;
    }

  return SOPC_Socket_Connect(from, &addr);
}

/****************************************************************************
 * Name: SOPC_Socket_CheckAckConnect
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_CheckAckConnect(SOPC_Socket sock)
{
  socklen_t length;
  int error = 0;

  if (sock == SOPC_INVALID_SOCKET)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  length = sizeof(error);
  if (getsockopt(sock->sock, SOL_SOCKET, SO_ERROR, &error, &length) < 0 ||
      error != 0)
    {
      return SOPC_STATUS_NOK;
    }

  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_SocketSet_Create
 ****************************************************************************/

FAR SOPC_SocketSet *SOPC_SocketSet_Create(void)
{
  FAR SOPC_SocketSet *set;

  set = SOPC_Calloc(1, sizeof(*set));
  if (set != NULL)
    {
      SOPC_SocketSet_Clear(set);
    }

  return set;
}

/****************************************************************************
 * Name: SOPC_SocketSet_Delete
 ****************************************************************************/

void SOPC_SocketSet_Delete(FAR SOPC_SocketSet **set)
{
  if (set != NULL)
    {
      SOPC_Free(*set);
      *set = NULL;
    }
}

/****************************************************************************
 * Name: SOPC_SocketSet_Add
 ****************************************************************************/

void SOPC_SocketSet_Add(SOPC_Socket sock, FAR SOPC_SocketSet *set)
{
  if (sock != SOPC_INVALID_SOCKET && set != NULL)
    {
      FD_SET(sock->sock, &set->set);
      if (sock->sock > set->fdmax)
        {
          set->fdmax = sock->sock;
        }
    }
}

/****************************************************************************
 * Name: SOPC_SocketSet_IsPresent
 ****************************************************************************/

bool SOPC_SocketSet_IsPresent(SOPC_Socket sock, FAR SOPC_SocketSet *set)
{
  return sock != SOPC_INVALID_SOCKET && set != NULL &&
         FD_ISSET(sock->sock, &set->set);
}

/****************************************************************************
 * Name: SOPC_SocketSet_Clear
 ****************************************************************************/

void SOPC_SocketSet_Clear(FAR SOPC_SocketSet *set)
{
  if (set != NULL)
    {
      FD_ZERO(&set->set);
      set->fdmax = 0;
    }
}

/****************************************************************************
 * Name: SOPC_Socket_WaitSocketEvents
 ****************************************************************************/

int32_t SOPC_Socket_WaitSocketEvents(FAR SOPC_SocketSet *read_set,
                                     FAR SOPC_SocketSet *write_set,
                                     FAR SOPC_SocketSet *except_set,
                                     uint32_t wait_ms)
{
  FAR struct timeval *timeout_ptr;
  struct timeval timeout;
  int fdmax;
  int ret;

  if (read_set == NULL || write_set == NULL || except_set == NULL)
    {
      return -1;
    }

  fdmax = read_set->fdmax;
  if (write_set->fdmax > fdmax)
    {
      fdmax = write_set->fdmax;
    }

  if (except_set->fdmax > fdmax)
    {
      fdmax = except_set->fdmax;
    }

  if (wait_ms == 0)
    {
      timeout_ptr = NULL;
    }
  else
    {
      timeout.tv_sec = wait_ms / 1000;
      timeout.tv_usec = (wait_ms % 1000) * 1000;
      timeout_ptr = &timeout;
    }

  do
    {
      ret = select(fdmax + 1, &read_set->set, &write_set->set,
                   &except_set->set, timeout_ptr);
    }
  while (ret < 0 && errno == EINTR);

  return ret;
}

/****************************************************************************
 * Name: SOPC_Socket_Write
 ****************************************************************************/

SOPC_ReturnStatus
SOPC_Socket_Write(SOPC_Socket sock, FAR const uint8_t *data,
                  uint32_t count, FAR uint32_t *sent_bytes)
{
  ssize_t ret;
  int error;

  if (sock == SOPC_INVALID_SOCKET || data == NULL || sent_bytes == NULL ||
      count > INT32_MAX)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  do
    {
      ret = send(sock->sock, data, count, MSG_NOSIGNAL);
    }
  while (ret < 0 && errno == EINTR);

  error = errno;
  if (ret >= 0)
    {
      *sent_bytes = ret;
      return SOPC_STATUS_OK;
    }

  *sent_bytes = 0;
  return s2opc_socket_would_block(error) ? SOPC_STATUS_WOULD_BLOCK :
         SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Socket_Read
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_Read(SOPC_Socket sock, FAR uint8_t *data,
                                   uint32_t size, FAR uint32_t *read_count)
{
  ssize_t ret;
  int error;

  if (sock == SOPC_INVALID_SOCKET || data == NULL || size == 0 ||
      read_count == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  do
    {
      ret = recv(sock->sock, data, size, 0);
    }
  while (ret < 0 && errno == EINTR);

  error = errno;
  if (ret > 0)
    {
      *read_count = ret;
      return SOPC_STATUS_OK;
    }

  *read_count = 0;
  if (ret == 0)
    {
      return SOPC_STATUS_CLOSED;
    }

  return s2opc_socket_would_block(error) ? SOPC_STATUS_WOULD_BLOCK :
         SOPC_STATUS_NOK;
}

/****************************************************************************
 * Name: SOPC_Socket_BytesToRead
 ****************************************************************************/

SOPC_ReturnStatus SOPC_Socket_BytesToRead(SOPC_Socket sock,
                                          FAR uint32_t *bytes_to_read)
{
  int available;
  int ret;

  if (sock == SOPC_INVALID_SOCKET || bytes_to_read == NULL)
    {
      return SOPC_STATUS_INVALID_PARAMETERS;
    }

  do
    {
      ret = ioctl(sock->sock, FIONREAD, &available);
    }
  while (ret < 0 && errno == EINTR);

  if (ret < 0 || available < 0)
    {
      return SOPC_STATUS_NOK;
    }

  *bytes_to_read = available;
  return SOPC_STATUS_OK;
}

/****************************************************************************
 * Name: SOPC_Socket_Close
 ****************************************************************************/

void SOPC_Socket_Close(FAR SOPC_Socket *sock)
{
  int ret;

  if (sock == NULL || *sock == SOPC_INVALID_SOCKET)
    {
      return;
    }

  do
    {
      ret = close((*sock)->sock);
    }
  while (ret < 0 && errno == EINTR);

  SOPC_Free(*sock);
  *sock = SOPC_INVALID_SOCKET;
}
