# ##############################################################################
# apps/examples/s2opc/CMakeLists.txt
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

if(CONFIG_EXAMPLES_S2OPC)
  set(S2OPC_DIR ${NUTTX_APPS_DIR}/netutils/s2opc/s2opc)

  nuttx_add_application(
    MODULE
    ${CONFIG_EXAMPLES_S2OPC}
    NAME
    ${CONFIG_EXAMPLES_S2OPC_PROGNAME}
    STACKSIZE
    ${CONFIG_EXAMPLES_S2OPC_STACKSIZE}
    PRIORITY
    ${CONFIG_EXAMPLES_S2OPC_PRIORITY}
    SRCS
    s2opc_main.c
    ${S2OPC_DIR}/tests/embedded/validation_test/embedded_address_space.c
    COMPILE_FLAGS
    -include
    ${NUTTX_APPS_DIR}/netutils/s2opc/port/nuttx/sopc_nuttx_config.h
    -Wno-missing-field-initializers
    DEPENDS
    s2opc)
endif()
