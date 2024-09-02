############################################################################
# apps/system/mcumgr/Makefile
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

MCUMGR_UNPACKDIR = nuttx-mcumgr
MCUMGR_ROOT = $(APPDIR)/system/mcumgr/$(MCUMGR_UNPACKDIR)
CONFIG_MCUMGR_REF := $(patsubst "%",%,$(strip $(CONFIG_SYSTEM_MCUMGR_REF)))
MCUMGR_TAR := $(CONFIG_SYSTEM_MCUMGR_REF).tar.gz
MCUMGR_URL := https://github.com/raiden00pl/nuttx-mcumgr/archive/$(MCUMGR_TAR)

# mcumgr sources

CSRCS =
CSRCS += nuttx-mcumgr/cborattr/src/cborattr.c
CSRCS += nuttx-mcumgr/mgmt/src/mgmt.c
CSRCS += nuttx-mcumgr/cmd/fs_mgmt/src/fs_mgmt.c
CSRCS += nuttx-mcumgr/cmd/img_mgmt/src/img_mgmt.c
CSRCS += nuttx-mcumgr/cmd/img_mgmt/src/img_mgmt_state.c
CSRCS += nuttx-mcumgr/cmd/img_mgmt/src/img_mgmt_util.c
CSRCS += nuttx-mcumgr/cmd/log_mgmt/src/log_mgmt.c
CSRCS += nuttx-mcumgr/cmd/os_mgmt/src/os_mgmt.c
CSRCS += nuttx-mcumgr/cmd/shell_mgmt/src/shell_mgmt.c
CSRCS += nuttx-mcumgr/cmd/stat_mgmt/src/stat_mgmt.c

# mcumgr port

CSRCS += porting/fs_mgmt.c
CSRCS += porting/img_mgmt.c
CSRCS += porting/img_mgmt_log.c
CSRCS += porting/log_mgmt.c
CSRCS += porting/os_mgmt.c
CSRCS += porting/shell_mgmt.c
CSRCS += porting/stat_mgmt.c

# mcumgr includes

MCUMGR_ALL_INC =
MCUMGR_ALL_INC += nuttx-mcumgr/include
MCUMGR_ALL_INC += configs

CFLAGS += $(addprefix ${INCDIR_PREFIX}, $(MCUMGR_ALL_INC))
CXXFLAGS += $(addprefix ${INCDIR_PREFIX}, $(MCUGMR_ALL_INC))

$(MCUMGR_TAR):
	$(Q) curl -L $(MCUMGR_URL) -o $(MCUMGR_TAR)

$(MCUMGR_UNPACKDIR): $(MCUMGR_TAR)
	$(Q) tar zxf $(MCUMGR_TAR)
	$(Q) mv nuttx-mcumgr-$(CONFIG_SYSTEM_MCUMGR_REF) $(MCUMGR_UNPACKDIR)
	$(Q) touch $(MCUMGR_UNPACKDIR)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(MCUMGR_UNPACKDIR)/.git),)
context:: $(MCUMGR_UNPACKDIR)

distclean::
	$(call DELFILE,$(MCUMGR_TAR))
	$(call DELDIR,$(MCUMGR_UNPACKDIR))
endif

include $(APPDIR)/Application.mk
