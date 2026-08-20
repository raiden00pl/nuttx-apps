# ##############################################################################
# apps/netutils/s2opc/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_NETUTILS_S2OPC)
  set(S2OPC_DIR ${CMAKE_CURRENT_LIST_DIR}/s2opc)
  set(S2OPC_TAG S2OPC_Toolkit_${CONFIG_S2OPC_VERSION})
  set(S2OPC_URL https://gitlab.com/systerel/S2OPC/-/archive/${S2OPC_TAG})

  if(NOT EXISTS ${S2OPC_DIR})
    FetchContent_Declare(
      s2opc_fetch
      URL ${S2OPC_URL}/S2OPC-${S2OPC_TAG}.tar.gz SOURCE_DIR ${S2OPC_DIR}
          BINARY_DIR ${NUTTX_BINARY_DIR}/apps/netutils/s2opc/s2opc
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 60)
    FetchContent_GetProperties(s2opc_fetch)

    if(NOT s2opc_fetch_POPULATED)
      FetchContent_Populate(s2opc_fetch)
    endif()
  endif()

  set(S2OPC_COMMON ${S2OPC_DIR}/src/Common)
  set(S2OPC_CS ${S2OPC_DIR}/src/ClientServer)
  set(S2OPC_PS ${S2OPC_DIR}/src/PubSub)
  set(S2OPC_PLATFORM ${S2OPC_COMMON}/helpers_platform_dep)

  file(
    GLOB
    COMMON_SRCS
    ${S2OPC_COMMON}/configuration/*.c
    ${S2OPC_COMMON}/helpers/*.c
    ${S2OPC_COMMON}/opcua_types/*.c
    ${S2OPC_COMMON}/sks/*.c
    ${S2OPC_COMMON}/crypto/*.c
    ${S2OPC_COMMON}/crypto/lib_dep/mbedtls/*.c)

  list(
    APPEND
    COMMON_SRCS
    ${S2OPC_PLATFORM}/sopc_askpass.c
    ${S2OPC_PLATFORM}/sopc_time_reference.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_askpass.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_atomic.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_date_time.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_eth_sockets.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_filesystem.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_mem_alloc.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_random.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_sockets.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_threads.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_time_reference.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/p_sopc_udp_sockets.c
    ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/sopc_build_info.c)

  file(
    GLOB
    CLIENTSERVER_SRCS
    ${S2OPC_CS}/address_space/*.c
    ${S2OPC_CS}/address_space/internal/*.c
    ${S2OPC_CS}/api_toolkit/*.c
    ${S2OPC_CS}/configuration/*.c
    ${S2OPC_CS}/sockets/*.c
    ${S2OPC_CS}/secure_channels/*.c
    ${S2OPC_CS}/services/*.c
    ${S2OPC_CS}/services/b2c/*.c
    ${S2OPC_CS}/services/bgenc/*.c
    ${S2OPC_CS}/frontend/common_wrapper/*.c
    ${S2OPC_CS}/frontend/common_wrapper/certificate_management/internal/*.c
    ${S2OPC_CS}/frontend/helper_wrapper/*.c
    ${S2OPC_CS}/frontend/client_wrapper/*.c
    ${S2OPC_CS}/frontend/client_wrapper/internal/*.c
    ${S2OPC_CS}/frontend/client_wrapper/alarm_conditions/*.c
    ${S2OPC_CS}/frontend/server_wrapper/*.c
    ${S2OPC_CS}/frontend/server_wrapper/internal/*.c
    ${S2OPC_CS}/frontend/server_wrapper/file_transfer/*.c
    ${S2OPC_CS}/frontend/server_wrapper/alarm_conditions/*.c
    ${S2OPC_CS}/frontend/server_wrapper/alarm_conditions/internal/*.c
    ${S2OPC_CS}/loaders/address_space_loaders/embedded/*.c)

  list(REMOVE_ITEM CLIENTSERVER_SRCS
       ${S2OPC_CS}/frontend/client_wrapper/libs2opc_client_config_xml.c
       ${S2OPC_CS}/frontend/server_wrapper/libs2opc_server_config_xml.c)

  file(
    GLOB
    PUBSUB_SRCS
    ${S2OPC_PS}/common/*.c
    ${S2OPC_PS}/dataset/*.c
    ${S2OPC_PS}/network/*.c
    ${S2OPC_PS}/protocol/*.c
    ${S2OPC_PS}/publisher/*.c
    ${S2OPC_PS}/security/*.c
    ${S2OPC_PS}/subscriber/*.c)

  if(CONFIG_S2OPC_XML_LOADERS)
    file(
      GLOB
      XML_SRCS
      ${S2OPC_CS}/loaders/helpers/*.c
      ${S2OPC_CS}/loaders/address_space_loaders/xml_expat/*.c
      ${S2OPC_CS}/loaders/config_loaders/xml_expat/*.c
      ${S2OPC_PS}/config_loaders/xml_expat/*.c)
    list(APPEND XML_SRCS
         ${S2OPC_CS}/frontend/client_wrapper/libs2opc_client_config_xml.c
         ${S2OPC_CS}/frontend/server_wrapper/libs2opc_server_config_xml.c)
  endif()

  # GCC 16 diagnoses upstream integer format strings on targets where uint32_t
  # is unsigned long. Keep this suppression limited to the unmodified upstream
  # sources that emit the diagnostics.
  set_source_files_properties(${S2OPC_PS}/publisher/sopc_pub_scheduler.c
                              PROPERTIES COMPILE_FLAGS -Wno-format)
  if(CONFIG_S2OPC_XML_LOADERS)
    set_source_files_properties(
      ${S2OPC_CS}/loaders/config_loaders/xml_expat/sopc_users_loader.c
      PROPERTIES COMPILE_FLAGS -Wno-format)
  endif()

  set(S2OPC_PUBLIC_INCLUDES
      ${CMAKE_CURRENT_LIST_DIR}/port/nuttx
      ${S2OPC_COMMON}/configuration
      ${S2OPC_COMMON}/helpers
      ${S2OPC_COMMON}/helpers_platform_dep
      ${S2OPC_COMMON}/opcua_types
      ${S2OPC_COMMON}/crypto
      ${S2OPC_COMMON}/crypto/lib_itf
      ${S2OPC_COMMON}/crypto/lib_dep
      ${S2OPC_COMMON}/sks
      ${S2OPC_CS}/address_space
      ${S2OPC_CS}/api_toolkit
      ${S2OPC_CS}/configuration
      ${S2OPC_CS}/frontend/common_wrapper
      ${S2OPC_CS}/frontend/common_wrapper/certificate_management
      ${S2OPC_CS}/frontend/helper_wrapper
      ${S2OPC_CS}/frontend/client_wrapper
      ${S2OPC_CS}/frontend/client_wrapper/alarm_conditions
      ${S2OPC_CS}/frontend/server_wrapper
      ${S2OPC_CS}/frontend/server_wrapper/alarm_conditions
      ${S2OPC_CS}/frontend/server_wrapper/file_transfer
      ${S2OPC_CS}/loaders/address_space_loaders/embedded
      ${S2OPC_PS}/common
      ${S2OPC_PS}/dataset
      ${S2OPC_PS}/network
      ${S2OPC_PS}/protocol
      ${S2OPC_PS}/publisher
      ${S2OPC_PS}/security
      ${S2OPC_PS}/subscriber)

  set(S2OPC_PRIVATE_INCLUDES
      ${S2OPC_COMMON}/crypto/lib_dep/mbedtls
      ${S2OPC_CS}/address_space/internal
      ${S2OPC_CS}/sockets
      ${S2OPC_CS}/secure_channels
      ${S2OPC_CS}/services
      ${S2OPC_CS}/services/b2c
      ${S2OPC_CS}/services/bgenc
      ${S2OPC_CS}/frontend/common_wrapper/internal
      ${S2OPC_CS}/frontend/common_wrapper/certificate_management/internal
      ${S2OPC_CS}/frontend/client_wrapper/internal
      ${S2OPC_CS}/frontend/server_wrapper/internal
      ${S2OPC_CS}/frontend/server_wrapper/alarm_conditions/internal)

  if(CONFIG_S2OPC_XML_LOADERS)
    list(
      APPEND
      S2OPC_PUBLIC_INCLUDES
      ${S2OPC_CS}/loaders/address_space_loaders
      ${S2OPC_CS}/loaders/address_space_loaders/xml_expat
      ${S2OPC_CS}/loaders/config_loaders
      ${S2OPC_CS}/loaders/config_loaders/xml_expat
      ${S2OPC_PS}/config_loaders/xml_expat)
    list(APPEND S2OPC_PRIVATE_INCLUDES ${S2OPC_CS}/loaders/helpers)
  endif()

  set(S2OPC_FLAGS
      -std=gnu99
      -D_GNU_SOURCE
      -DS2OPC_COMMON_STATIC_DEFINE
      -DSOPC_TIME_T_SIZE=8
      -include
      ${CMAKE_CURRENT_LIST_DIR}/port/nuttx/sopc_nuttx_config.h
      -Wno-shadow
      -Wno-cast-qual
      -Wno-conversion
      -Wno-missing-prototypes
      -Wno-strict-prototypes
      -Wno-unused-but-set-variable
      -Wno-unused-function
      -Wno-unused-variable)

  if(CONFIG_S2OPC_XML_LOADERS)
    list(APPEND S2OPC_FLAGS -DUANODESET_LOADER_LOG)
  endif()

  if(CONFIG_S2OPC_MQTT)
    list(APPEND S2OPC_FLAGS -DUSE_MQTT_PAHO=1)
    list(APPEND S2OPC_PRIVATE_INCLUDES
         ${CMAKE_CURRENT_LIST_DIR}/../paho_mqtt/paho_mqtt/src)
  else()
    list(APPEND S2OPC_FLAGS -DUSE_MQTT_PAHO=0)
  endif()

  if(CONFIG_ARCH_64BIT)
    list(APPEND S2OPC_FLAGS -DSOPC_PTR_SIZE=8)
  else()
    list(APPEND S2OPC_FLAGS -DSOPC_PTR_SIZE=4)
  endif()

  if(CONFIG_ENDIAN_BIG)
    list(APPEND S2OPC_FLAGS -DSOPC_IS_LITTLE_ENDIAN=0)
  else()
    list(APPEND S2OPC_FLAGS -DSOPC_IS_LITTLE_ENDIAN=1)
  endif()

  nuttx_add_library(s2opc STATIC)
  target_sources(s2opc PRIVATE ${COMMON_SRCS} ${CLIENTSERVER_SRCS}
                               ${PUBSUB_SRCS} ${XML_SRCS})
  target_include_directories(
    s2opc
    PUBLIC ${S2OPC_PUBLIC_INCLUDES}
    PRIVATE ${S2OPC_PRIVATE_INCLUDES})
  target_compile_options(s2opc PRIVATE ${S2OPC_FLAGS})
  nuttx_export_header(TARGET s2opc INCLUDE_DIRECTORIES ${S2OPC_PUBLIC_INCLUDES})
  nuttx_add_dependencies(TARGET s2opc DEPENDS mbedtls)

  if(CONFIG_S2OPC_XML_LOADERS)
    nuttx_add_dependencies(TARGET s2opc DEPENDS expat)
  endif()

  if(CONFIG_S2OPC_MQTT)
    nuttx_add_dependencies(TARGET s2opc DEPENDS mqtt5)
  endif()

endif()
