############################################################################
# apps/industry/s2opc/Makefile
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
###########################################################################

# Standard includes

include $(APPDIR)/Make.defs

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

S2OPC_VERSION = $(patsubst "%",%,$(strip $(CONFIG_INDUSTRY_S2OPC_VERSION)))

S2OPC_TARBALL = S2OPC-$(S2OPC_VERSION).tar.gz
S2OPC_UNPACKNAME = S2OPC-$(S2OPC_VERSION)
S2OPC_SRCNAME = S2OPC
UNPACK ?= tar -zxf
S2OPC_SRCDIR = $(WD)/$(S2OPC_SRCNAME)

CSRCS += $(S2OPC_SRCDIR)/src/Common/configuration/sopc_common.c

MODULE = $(CONFIG_INDUSTRY_S2OPC)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(S2OPC_SRCNAME)/.git),)
$(S2OPC_TARBALL):
	@echo "Downloading: $(S2OPC_TARBALL)"
	$(Q) curl -L -O $(CONFIG_INDUSTRY_S2OPC_URL)/$(S2OPC_TARBALL)

$(S2OPC_SRCNAME): $(S2OPC_TARBALL)
	@echo "Unpacking: $(S2OPC_TARBALL) -> $(S2OPC_UNPACKNAME)"
	$(Q) $(UNPACK) $(S2OPC_TARBALL)
	$(Q) mv $(S2OPC_UNPACKNAME) $(S2OPC_SRCNAME)
	$(Q) cat 0001-NuttX-port.patch | patch -s -N -d $(S2OPC_SRCNAME) -p1
	$(Q) echo "Patching $(S2OPC_SRCNAME)"
endif

context:: $(S2OPC_SRCNAME)

distclean::
ifeq ($(wildcard $(S2OPC_SRCNAME)/.git),)
	$(call DELDIR, $(S2OPC_SRCNAME))
	$(call DELFILE, $(S2OPC_TARBALL))
endif

include $(APPDIR)/Application.mk
