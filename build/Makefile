#--------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
#--------------------------------------------------------------------------------
# Bootstrap Makefile for SCXCore
# 2007-08-23
#--------------------------------------------------------------------------------

# Set up architecture and Build Root Directory
# PF (i.e. PlatForm) is either linux, solaris
PWD:=$(shell pwd)
SCX_BRD=$(subst /build,,$(PWD))
PF_POSIX=true
UNAME=$(shell uname)
ifeq ($(UNAME),Linux)
	PF=Linux
	PF_LINUX=true
endif 
ifeq ($(UNAME),SunOS)
	PF=SunOS
	PF_SOLARIS=true
endif
ifeq ($(UNAME),HP-UX)
	PF=HPUX
	PF_HPUX=true
endif
ifeq ($(UNAME),AIX)
	PF=AIX
	PF_AIX=true
endif
ifeq ($(UNAME),Darwin)
	PF=MacOS
	PF_MACOS=true
endif

BUILD_CONFIG_CACHE=$(SCX_BRD)/build/Makefile.config_cache

all : installerPackage

clean: clean-status doxygen_clean pegasus_clean
	-$(RMDIR) $(INTERMEDIATE_DIR)/include
	-$(RMDIR) $(INTERMEDIATE_DIR)/source
	-$(RMDIR) $(INTERMEDIATE_DIR)/test
	-$(RMDIR) $(SCX_BRD)/installer/intermediate
	-$(RMDIR) $(INTERMEDIATE_DIR)/lib*.a
	-$(RMDIR) $(INTERMEDIATE_DIR)/*.exp
	-$(RMDIR) $(INTERMEDIATE_DIR)/*.map
	-$(RMDIR) $(INTERMEDIATE_DIR)/*.cov
	-$(RMDIR) $(TARGET_DIR)

clean-status:
	$(ECHO) "========================= Performing make clean"

# Include the real stuff
include $(SCX_BRD)/build/Makefile.version

# Include the rest of the stuff we need

include $(SCX_BRD)/build/Makefile.pal
-include $(BUILD_CONFIG_CACHE)
include $(SCX_BRD)/build/Makefile.macros
include $(SCX_BRD)/build/Makefile.pf.$(PF)
include $(SCX_BRD)/build/Makefile.rules

include $(SCX_BRD)/build/Makefile.components
include $(SCX_BRD)/build/Makefile.kits

include $(SCX_BRD)/build/Makefile.stub

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifeq (,$(findstring cache,$(MAKECMDGOALS)))
-include $(DEPFILES) $(DEPFILES_TEST)
endif
endif


#-------------------------------- End of File -----------------------------------


