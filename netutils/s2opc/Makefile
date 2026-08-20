############################################################################
# apps/netutils/s2opc/Makefile
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

S2OPC_VERSION = $(patsubst "%",%,$(CONFIG_S2OPC_VERSION))
S2OPC_TAG = S2OPC_Toolkit_$(S2OPC_VERSION)
S2OPC_ARCHIVE = $(S2OPC_TAG).tar.gz
S2OPC_ARCHIVES = $(wildcard S2OPC_Toolkit_*.tar.gz)
S2OPC_SRCDIR = s2opc
S2OPC_URL ?= https://gitlab.com/systerel/S2OPC/-/archive
S2OPC_COMMON = $(S2OPC_SRCDIR)/src/Common
S2OPC_CS = $(S2OPC_SRCDIR)/src/ClientServer
S2OPC_PS = $(S2OPC_SRCDIR)/src/PubSub
S2OPC_PLATFORM = $(S2OPC_COMMON)/helpers_platform_dep
S2OPC_CSCERT = $(S2OPC_CS)/frontend/common_wrapper/certificate_management
S2OPC_CSALARM = $(S2OPC_CS)/frontend/server_wrapper/alarm_conditions
S2OPC_CS_XML_WRAPPERS = \
	$(S2OPC_CS)/frontend/client_wrapper/libs2opc_client_config_xml.c \
	$(S2OPC_CS)/frontend/server_wrapper/libs2opc_server_config_xml.c

CSRCS += $(wildcard $(S2OPC_COMMON)/configuration/*.c)
CSRCS += $(wildcard $(S2OPC_COMMON)/helpers/*.c)
CSRCS += $(wildcard $(S2OPC_COMMON)/opcua_types/*.c)
CSRCS += $(wildcard $(S2OPC_COMMON)/sks/*.c)
CSRCS += $(wildcard $(S2OPC_COMMON)/crypto/*.c)
CSRCS += $(wildcard $(S2OPC_COMMON)/crypto/lib_dep/mbedtls/*.c)

CSRCS += $(S2OPC_PLATFORM)/sopc_askpass.c
CSRCS += $(S2OPC_PLATFORM)/sopc_time_reference.c
CSRCS += port/nuttx/p_sopc_askpass.c
CSRCS += port/nuttx/p_sopc_atomic.c
CSRCS += port/nuttx/p_sopc_date_time.c
CSRCS += port/nuttx/p_sopc_eth_sockets.c
CSRCS += port/nuttx/p_sopc_filesystem.c
CSRCS += port/nuttx/p_sopc_mem_alloc.c
CSRCS += port/nuttx/p_sopc_random.c
CSRCS += port/nuttx/p_sopc_sockets.c
CSRCS += port/nuttx/p_sopc_threads.c
CSRCS += port/nuttx/p_sopc_time_reference.c
CSRCS += port/nuttx/p_sopc_udp_sockets.c
CSRCS += port/nuttx/sopc_build_info.c

CSRCS += $(wildcard $(S2OPC_CS)/address_space/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/address_space/internal/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/api_toolkit/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/configuration/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/sockets/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/secure_channels/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/services/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/services/b2c/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/services/bgenc/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/frontend/common_wrapper/*.c)
CSRCS += $(wildcard $(S2OPC_CSCERT)/internal/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/frontend/helper_wrapper/*.c)
CSRCS += $(filter-out $(S2OPC_CS_XML_WRAPPERS), \
	$(wildcard $(S2OPC_CS)/frontend/client_wrapper/*.c))
CSRCS += $(wildcard $(S2OPC_CS)/frontend/client_wrapper/internal/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/frontend/client_wrapper/alarm_conditions/*.c)
CSRCS += $(filter-out $(S2OPC_CS_XML_WRAPPERS), \
	$(wildcard $(S2OPC_CS)/frontend/server_wrapper/*.c))
CSRCS += $(wildcard $(S2OPC_CS)/frontend/server_wrapper/internal/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/frontend/server_wrapper/file_transfer/*.c)
CSRCS += $(wildcard $(S2OPC_CSALARM)/*.c)
CSRCS += $(wildcard $(S2OPC_CSALARM)/internal/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/loaders/address_space_loaders/embedded/*.c)

ifeq ($(CONFIG_S2OPC_XML_LOADERS),y)
CSRCS += $(S2OPC_CS_XML_WRAPPERS)
CSRCS += $(wildcard $(S2OPC_CS)/loaders/helpers/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/loaders/address_space_loaders/xml_expat/*.c)
CSRCS += $(wildcard $(S2OPC_CS)/loaders/config_loaders/xml_expat/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/config_loaders/xml_expat/*.c)

$(S2OPC_CS)/loaders/config_loaders/xml_expat/sopc_users_loader.c_CFLAGS += \
	-Wno-format
endif

CSRCS += $(wildcard $(S2OPC_PS)/common/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/dataset/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/network/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/protocol/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/publisher/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/security/*.c)
CSRCS += $(wildcard $(S2OPC_PS)/subscriber/*.c)

# GCC 16 diagnoses upstream integer format strings on targets where uint32_t
# is unsigned long.  Keep this suppression limited to the unmodified upstream
# sources that emit the diagnostics.

$(S2OPC_PS)/publisher/sopc_pub_scheduler.c_CFLAGS += -Wno-format

CFLAGS += ${INCDIR_PREFIX}port/nuttx
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_COMMON)/crypto/lib_dep/mbedtls
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/address_space/internal
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/sockets
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/secure_channels
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/services
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/services/b2c
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/services/bgenc
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/frontend/common_wrapper/internal
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CSCERT)
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CSCERT)/internal
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/frontend/client_wrapper/internal
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/frontend/server_wrapper/internal
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/frontend/server_wrapper/file_transfer
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CSALARM)
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CSALARM)/internal
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/frontend/client_wrapper/alarm_conditions
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/loaders/address_space_loaders/embedded

CFLAGS += -std=gnu99 -D_GNU_SOURCE -DS2OPC_COMMON_STATIC_DEFINE
CFLAGS += -DSOPC_TIME_T_SIZE=8
CFLAGS += -include $(CURDIR)/port/nuttx/sopc_nuttx_config.h
CFLAGS += -Wno-shadow -Wno-cast-qual -Wno-conversion
CFLAGS += -Wno-missing-prototypes -Wno-strict-prototypes
CFLAGS += -Wno-unused-but-set-variable -Wno-unused-function
CFLAGS += -Wno-unused-variable

ifeq ($(CONFIG_S2OPC_XML_LOADERS),y)
CFLAGS += ${INCDIR_PREFIX}$(S2OPC_CS)/loaders/helpers
CFLAGS += -DUANODESET_LOADER_LOG
endif

ifeq ($(CONFIG_S2OPC_MQTT),y)
CFLAGS += -DUSE_MQTT_PAHO=1
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/netutils/paho_mqtt/paho_mqtt/src
else
CFLAGS += -DUSE_MQTT_PAHO=0
endif

ifeq ($(CONFIG_ARCH_64BIT),y)
CFLAGS += -DSOPC_PTR_SIZE=8
else
CFLAGS += -DSOPC_PTR_SIZE=4
endif

ifeq ($(CONFIG_ENDIAN_BIG),y)
CFLAGS += -DSOPC_IS_LITTLE_ENDIAN=0
else
CFLAGS += -DSOPC_IS_LITTLE_ENDIAN=1
endif

S2OPC_PORTDIR = port
S2OPC_PORT_CLEANFILES = $(S2OPC_PORTDIR)$(DELIM)*$(OBJEXT)
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)*.gcda
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)*.gcno
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)*.su
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)nuttx$(DELIM)*$(OBJEXT)
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)nuttx$(DELIM)*.gcda
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)nuttx$(DELIM)*.gcno
S2OPC_PORT_CLEANFILES += $(S2OPC_PORTDIR)$(DELIM)nuttx$(DELIM)*.su

clean::
	$(call DELFILE, $(S2OPC_PORT_CLEANFILES))

ifeq ($(wildcard $(S2OPC_SRCDIR)/.git),)
$(S2OPC_ARCHIVE):
	@echo "Downloading: $(S2OPC_ARCHIVE)"
	$(Q) curl -o $(S2OPC_ARCHIVE) -L \
		$(S2OPC_URL)/$(S2OPC_TAG)/S2OPC-$(S2OPC_TAG).tar.gz

$(S2OPC_SRCDIR): $(S2OPC_ARCHIVE)
	$(Q) tar -xf $(S2OPC_ARCHIVE)
	$(Q) mv S2OPC-$(S2OPC_TAG) $(S2OPC_SRCDIR)

context:: $(S2OPC_SRCDIR)

distclean::
	$(call DELDIR, $(S2OPC_SRCDIR))
	$(call DELFILE, $(S2OPC_ARCHIVES))
endif

MODULE = $(CONFIG_NETUTILS_S2OPC)

include $(APPDIR)/Application.mk
