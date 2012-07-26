#//%LICENSE////////////////////////////////////////////////////////////////
#//
#// Licensed to The Open Group (TOG) under one or more contributor license
#// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
#// this work for additional information regarding copyright ownership.
#// Each contributor licenses this file to you under the OpenPegasus Open
#// Source License; you may not use this file except in compliance with the
#// License.
#//
#// Permission is hereby granted, free of charge, to any person obtaining a
#// copy of this software and associated documentation files (the "Software"),
#// to deal in the Software without restriction, including without limitation
#// the rights to use, copy, modify, merge, publish, distribute, sublicense,
#// and/or sell copies of the Software, and to permit persons to whom the
#// Software is furnished to do so, subject to the following conditions:
#//
#// The above copyright notice and this permission notice shall be included
#// in all copies or substantial portions of the Software.
#//
#// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#//
#//////////////////////////////////////////////////////////////////////////
ROOT = ../../../..

DIR = Pegasus/ProviderManager2/JMPI

include $(ROOT)/mak/config.mak

ifeq ($(OS_TYPE),windows)
   JAVALIBS=$(JAVA_SDK)/jre/lib/
   EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(JAVA_SDK)/include -I$(JAVA_SDK)/include/win32
   EXTRA_LIBRARIES += $(JAVA_SDK)/lib/jvm.lib
else
ifeq ($(PEGASUS_PLATFORM),ZOS_ZSERIES_IBM)
   SYS_INCLUDES += -I${JAVA_SDK}/include
   EXTRA_LIBRARIES += ${JAVA_SDK}/bin/classic/libjvm.x
else
ifndef PEGASUS_JVM
   PEGASUS_JVM=sun
endif
ifeq ($(PEGASUS_JVM),sun)
   JAVALIBS=$(JAVA_SDK)/jre/lib/$(PEGASUS_JAVA_ARCH)
   EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(JAVA_SDK)/include -I$(JAVA_SDK)/include/linux
   EXTRA_LIBRARIES += -L$(JAVALIBS)/native_threads -L$(JAVALIBS)/$(PEGASUS_JAVA_TYPE) -ljvm -lhpi -lcrypt
endif
ifeq ($(PEGASUS_JVM),ibm)
   JAVALIBS=$(JAVA_SDK)/jre/bin
   EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(JAVA_SDK)/include
   EXTRA_LIBRARIES += -L$(JAVALIBS)/classic/ -L$(JAVALIBS)/ -ljvm -lhpi -lcrypt
endif
ifeq ($(PEGASUS_JVM),ibm64)
   JAVALIBS=$(JAVA_SDK)/jre/bin
   EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(JAVA_SDK)/include
   EXTRA_LIBRARIES += -L$(JAVALIBS)/j9vm/ -L$(JAVALIBS)/classic/ -L$(JAVALIBS)/ -ljvm
endif
ifeq ($(PEGASUS_JVM),bea)
   JAVALIBS=$(JAVA_SDK)/jre/lib/$(PEGASUS_JAVA_ARCH)
   EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(JAVA_SDK)/include/ -I$(JAVA_SDK)/include/linux/
   EXTRA_LIBRARIES += -L$(JAVALIBS)/ -L$(JAVALIBS)/jrockit/ -L$(JAVALIBS)/native_threads/ -ljvm -lhpi -lcrypt
endif
ifeq ($(PEGASUS_JVM),gcj)
   JAVALIBS=$(JAVA_SDK)/jre/lib/$(PEGASUS_JAVA_ARCH)
   EXTRA_LIBRARIES += -L$(JAVALIBS)/$(PEGASUS_JAVA_TYPE) -ljvm
endif
endif
endif

LOCAL_DEFINES = -DPEGASUS_JMPIPM_INTERNAL -DPEGASUS_INTERNALONLY

ifeq ($(OS_TYPE),vms)
 EXTRA_LIBRARIES += java\$jvm_shr
 VMS_VECTOR = PegasusCreateProviderManager
endif

LIBRARY = JMPIProviderManager

LIBRARIES = \
	pegprovidermanager \
	pegconfig \
	pegwql \
	pegquerycommon \
	pegprovider \
	pegclient \
	pegcommon

SOURCES = \
        JMPIProviderManagerMain.cpp \
        JMPIProviderManager.cpp \
        JMPILocalProviderManager.cpp \
        JMPIProviderModule.cpp \
        JMPIProvider.cpp \
        JMPIImpl.cpp

include $(ROOT)/mak/dynamic-library.mak

ifeq ($(OS_TYPE),vms)
all:    $(FULL_PROGRAM)
else
all:    $(FULL_LIB)
endif

repository tests poststarttests: