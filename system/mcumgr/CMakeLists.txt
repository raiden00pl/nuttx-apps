# ##############################################################################
# apps/system/mcumgr/CMakeLists.txt
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

if(CONFIG_SYSTEM_MCUMGR)
  nuttx_add_library(mcumgr)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/mcumgr)
    FetchContent_Declare(
      mcumgr
      DOWNLOAD_NAME "${CONFIG_SYSTEM_MCUMGR_REF}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "https://github.com/raiden00pl/nuttx-mcumgr/archive/${CONFIG_SYSTEM_MCUMGR_REF}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/nuttx-mcumgr
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/system/mcumgr/nuttx-mcumgr
          CONFIGURE_COMMAND
          ""
          BUILD_COMMAND
          ""
          INSTALL_COMMAND
          ""
          TEST_COMMAND
          ""
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(mcumgr)

    if(NOT mcumgr_POPULATED)
      FetchContent_Populate(mcumgr)
    endif()
  endif()

  # porting sources
  set(SRCS_PORTING
      porting/fs_mgmt.c
      porting/img_mgmt.c
      porting/img_mgmt_log.c
      porting/log_mgmt.c
      porting/os_mgmt.c
      porting/shell_mgmt.c
      porting/stat_mgmt.c)

  # mcumgr sources
  set(SRCS_MCUMGR
      nuttx-mcumgr/cborattr/src/cborattr.c
      nuttx-mcumgr/mgmt/src/mgmt.c
      nuttx-mcumgr/cmd/fs_mgmt/src/fs_mgmt.c
      nuttx-mcumgr/cmd/img_mgmt/src/img_mgmt.c
      nuttx-mcumgr/cmd/img_mgmt/src/img_mgmt_state.c
      nuttx-mcumgr/cmd/img_mgmt/src/img_mgmt_util.c
      nuttx-mcumgr/cmd/log_mgmt/src/log_mgmt.c
      nuttx-mcumgr/cmd/os_mgmt/src/os_mgmt.c
      nuttx-mcumgr/cmd/shell_mgmt/src/shell_mgmt.c
      nuttx-mcumgr/cmd/stat_mgmt/src/stat_mgmt.c)

  target_include_directories(mcumgr PUBLIC config nuttx-mcumgr/include)

  set(SRCS ${SRCS_PORTING} ${SRCS_MCUMGR})

  target_sources(mcumgr PRIVATE ${SRCS})

endif()
