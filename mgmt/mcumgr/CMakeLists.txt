# ##############################################################################
# apps/mgmt/mcumgr/CMakeLists.txt
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

if(CONFIG_MGMT_MCUMGR)
  nuttx_add_library(mcumgr)

  if(CONFIG_MGMT_MCUMGR_DAEMON)
    nuttx_add_application(
      NAME
      ${MGMT_MCUMGR_DAEMON_PROGNAME}
      SRCS
      hello_main.c
      STACKSIZE
      ${MGMT_MCUMGR_DAEMON_STACKSIZE}
      PRIORITY
      ${MGMT_MCUMGR_DAEMON_PRIORITY}
      DEPENDS
      mcumgr)
  endif()

  set(SRCS
    mcumgr/mgmt/mgmt.c
    mcumgr/smp/smp.c
    mcumgr/smp/smp_err.c
    mcumgr/util/smp_buf.c
    mcumgr/util/zcbor_bulk.c)


  # if(xxx)
  #   list(APPEND CSRCS )
  # endif()


  target_include_directories(mcumgr PRIVATE mcumgr)
  target_sources(mcumgr PRIVATE ${SRCS})
endif()
