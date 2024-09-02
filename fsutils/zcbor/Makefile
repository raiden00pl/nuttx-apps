############################################################################
# apps/fsutils/zcbor/Makefile
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

ZCBOR_URL ?= "https://github.com/NordicSemiconductor/zcbor/archive"
ZCBOR_VERSION ?= 9a0b27b189b751afb7d4f2276b1912c6c7e2e903

ZCBOR_UNPACKNAME = zcbor

$(ZCBOR_UNPACKNAME):
	@echo "Downloading: $(ZCBOR_UNPACKNAME)"
	$(Q) curl -O -L $(ZCBOR_URL)/$(ZCBOR_VERSION).zip
	$(Q) mkdir $(ZCBOR_UNPACKNAME)
	$(Q) unzip -o -j $(ZCBOR_VERSION).zip -d $(ZCBOR_UNPACKNAME)
	$(call DELFILE, $(ZCBOR_VERSION).zip)

# Files

CSRCS =
CSRCS += zcbor/src/zcbor_common.c
CSRCS += zcbor/src/zcbor_decode.c
CSRCS += zcbor/src/zcbor_encode.c
CSRCS += zcbor/src/zcbor_print.c

clean::
	$(call DELFILE, $(OBJS))

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(ZCBOR_UNPACKNAME)/.git),)
context:: $(ZCBOR_UNPACKNAME)

distclean::
	$(call DELDIR, $(ZCBOR_UNPACKNAME))
endif

include $(APPDIR)/Application.mk
