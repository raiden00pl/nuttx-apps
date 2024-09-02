# ##############################################################################
# apps/fsutils/zcbor/CMakeLists.txt
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

if(CONFIG_FSUTILS_ZCBOR)

  set(ZCBOR_DIR ${CMAKE_CURRENT_LIST_DIR}/zcbor)

  if(NOT EXISTS ${ZCBOR_DIR})
    set(ZCBOR_VERSION 9a0b27b189b751afb7d4f2276b1912c6c7e2e903)
    set(ZCBOR_URL
      https://github.com/NordicSemiconductor/zcbor/archive/${ZCBOR_VERSION}.zip)
    file(DOWNLOAD ${ZCBOR_URL} ${CMAKE_CURRENT_LIST_DIR}/zcbor.zip)
  endif()

  set(CSRCS
    ${ZCBOR_DIR}/src/zcbor_common.c
    ${ZCBOR_DIR}/src/zcbor_decode.c
    ${ZCBOR_DIR}/src/zcbor_encode.c
    ${ZCBOR_DIR}/src/zcbor_print.c
  )

  nuttx_add_library(zcbor STATIC)
  target_sources(zcbor PRIVATE ${CSRCS})
  target_include_directories(zcbor PRIVATE ${ZCBOR_DIR}/include)

endif()
