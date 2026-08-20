############################################################################
# apps/examples/s2opc/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_EXAMPLES_S2OPC_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_S2OPC_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_S2OPC_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_S2OPC)

S2OPC_DIR = $(APPDIR)/netutils/s2opc/s2opc
S2OPC_VALIDATION_DIR = $(S2OPC_DIR)/tests/embedded/validation_test

MAINSRC = s2opc_main.c

CSRCS += $(S2OPC_VALIDATION_DIR)/embedded_address_space.c

CFLAGS += -include $(APPDIR)/netutils/s2opc/port/nuttx/sopc_nuttx_config.h
CFLAGS += -Wno-missing-field-initializers

include $(APPDIR)/Application.mk
