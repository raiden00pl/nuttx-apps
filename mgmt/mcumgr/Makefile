############################################################################
# apps/mgmt/mcumgr/Makefile
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

# mcumgr sources

CSRCS += mcumgr/mgmt/mgmt.c
CSRCS += mcumgr/smp/smp.c
CSRCS += mcumgr/smp/smp_err.c
CSRCS += mcumgr/utils/smp_buf.c
CSRCS += mcumgr/utils/zcbor_bulk.c

# mcumgr includes

MCUMGR_ALL_INC = mcumgr

ifeq ($(CONFIG_MGMT_MCUMGR_DAEMON),y)
PROGNAME  = $(MGMT_MCUMGR_DAEMON_PROGNAME)
PRIORITY  = $(MGMT_MCUMGR_DAEMON_PRIORITY)
STACKSIZE = $(MGMT_MCUMGR_DAEMON_STACKSIZE)
MODULE    = $(CONFIG_MGMT_MCUMGR_DAEMON)
MAINSRC   = mcumgr_daemon.c
endif

include $(APPDIR)/Application.mk
