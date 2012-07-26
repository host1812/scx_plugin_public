#--------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
#--------------------------------------------------------------------------------

SCX_ROOT_PATH=../..
SOURCE_PATH=$(SCX_ROOT_PATH)/source
TEST_PATH=$(SCX_ROOT_PATH)/test
PROVSUP_PATH=$(SOURCE_PATH)/code/provsup_lib
TESTPROVIDER_PATH=$(PROVSUP_PATH)/testprovider
PROVIDER_PATH=$(SOURCE_PATH)/code/providers
CPUPROVIDER_PATH=$(PROVIDER_PATH)/cpu_provider
MEMORYPROVIDER_PATH=$(PROVIDER_PATH)/memory_provider
DISKPROVIDER_PATH=$(PROVIDER_PATH)/disk_provider
UTIL_PATH=$(SOURCE_PATH)/code/common_lib/util
PAL_PATH=$(SOURCE_PATH)/code/common_lib/pal
LOG_PATH=$(SOURCE_PATH)/code/common_lib/util/log
PALSYSTEMCOMMON_PATH=$(SOURCE_PATH)/code/common_lib/pal/system/common
PALSYSTEMCPU_PATH=$(SOURCE_PATH)/code/common_lib/pal/system/cpu
PALSYSTEMMEMORY_PATH=$(SOURCE_PATH)/code/common_lib/pal/system/memory
PALSYSTEMDISK_PATH=$(SOURCE_PATH)/code/common_lib/pal/system/disk
SCXCOREINCLUDE_PATH=$(SOURCE_PATH)/code/include/scxcorelib
SCXPROVIDERLIB_INCLUDE_PATH=$(SOURCE_PATH)/code/include/scxproviderlib
SCXSYSTEM_INCLUDE_PATH=$(SOURCE_PATH)/code/include/scxsystemlib
INCLUDE_PATH=$(SOURCE_PATH)/code/include
TESTCOMMONLIB_PATH=$(TEST_PATH)/code/common_lib
TESTPALSYSTEM_PATH=$(TEST_PATH)/code/common_lib/pal/system
TESTPROVIDERSUPPORT_PATH=$(TEST_PATH)/code/provsup_lib
TESTMEMORYPROVIDER_PATH=$(TEST_PATH)/code/providers/memory_provider
MOF_PATH=$(SOURCE_PATH)/code/mof
EXT_INCLUDE_PATH=$(SOURCE_PATH)/ext/include
TEST_EXT_INCLUDE_PATH=$(TEST_PATH)/ext/include
TEST_EXT_LIB_PATH=$(TEST_PATH)/ext/lib
DOXYGEN_OUTPUT_PATH=$(SCX_ROOT_PATH)/build/temporary/doxygen

INCLUDES=-I$(INCLUDE_PATH) -I$(UTIL_PATH) -I$(TESTPROVIDER_PATH) -I$(CPUPROVIDER_PATH) -I$(MEMORYPROVIDER_PATH) -I$(DISKPROVIDER_PATH) 

ifeq ($(OS),Windows_NT)
	PLATFORM_INCLUDES=-I$(EXT_INCLUDE_PATH) -I$(TEST_EXT_INCLUDE_PATH)
	TEST_EXT_LIB_PLATFORM_PATH=$(TEST_EXT_LIB_PATH)/windows/ia32
	CPPUNIT_LIB_PATH=$(TEST_EXT_LIB_PLATFORM_PATH)/cppunit
	CXX_BASE=cl /Od $(INCLUDES) $(PLATFORM_INCLUDES) /D "WIN32" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /FD /EHa /MDd /WX /nologo /c /clr /TP /errorReport:prompt 
	CXX=$(CXX_BASE) /W4
	CXX_TESTRUN=$(CXX_BASE) /W3
	LINK=link.exe /OUT:"testrunner.exe" /INCREMENTAL:no /PROFILE /NOLOGO /MANIFEST /MANIFESTFILE:"testrunner.exe.manifest" /DEBUG /ASSEMBLYDEBUG /PDB:"testrunner.pdb" /MACHINE:X86 /FIXED:No /ERRORREPORT:PROMPT /LIBPATH:"$(CPPUNIT_LIB_PATH)"
	SCX_OS=win
	OBJ_FILE_SUFFIX=obj
	LIB_FILE_SUFFIX=lib
	AR=lib.exe /OUT:"$@" /NOLOGO	
	RM=cmd /c del /f /q
	RMDIR=cmd /c rmdir /s /q
	MKPATH=mkdir 
else
	PLATFORM_INCLUDES=-isystem$(EXT_INCLUDE_PATH) -isystem$(TEST_EXT_INCLUDE_PATH)

	P_OS := $(shell uname -s)
	ifeq ($(P_OS),SunOS)
		TEST_EXT_LIB_PLATFORM_PATH=$(TEST_EXT_LIB_PATH)/solaris/ia32/
		SCX_OS=solaris
		SO=g++ -shared -m32 -L. -o $@
		SO=g++ -shared -Xlinker -R -Xlinker /usr/local/lib -m32 -mimpure-text -L. -o $@
	else
		TEST_EXT_LIB_PLATFORM_PATH=$(TEST_EXT_LIB_PATH)/linux/ia32/
		SCX_OS=linux
		SO=g++ -shared -Xlinker -rpath -Xlinker /usr/lib -m32 -L. -o $@
	endif
	CXXFLAGS=-Wall -pedantic -Winit-self -Wmissing-include-dirs -Wswitch-enum -Wunused-parameter -Wextra -Wundef -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Werror
	CPPUNIT_LIB_PATH=$(TEST_EXT_LIB_PLATFORM_PATH)/cppunit
	CXX=g++ -ggdb $(CXXFLAGS) -c -o $@ $(INCLUDES) $(PLATFORM_INCLUDES)
	CXX_TESTRUN=$(CXX)
	LINK=g++ -L. -o $@
	OBJ_FILE_SUFFIX=o
	LIB_FILE_SUFFIX=a
	AR=ar -rc $@ 
	RM=rm -f
	RMDIR=rm -rf
	MKPATH=mkdir -p
endif

CIMMOF=cimmof
CIMPROVIDER=cimprovider
STRIP=strip

all : iteration1_installer libscxassertabort.$(LIB_FILE_SUFFIX)

providers: libSCXTestProvider.so libSCXCoreProviderModule.so 

iteration1_installer : scx_install_iter1.tgz 

clean: doxygen_clean
	$(RM) *.tgz
	$(RM) *.$(OBJ_FILE_SUFFIX)
	$(RM) *.so
	$(RM) *.$(LIB_FILE_SUFFIX)
	$(RM) *.exe
	$(RM) iostat

register :
	$(CIMMOF) -nroot/cimv2 $(TESTPROVIDER_PATH)/scx_test.mof
	$(CIMMOF) -nroot/PG_InterOp $(TESTPROVIDER_PATH)/scx_test_r.mof
	$(CIMMOF) -nroot/cimv2 $(MOF_PATH)/scx_perf.mof
	$(CIMMOF) -nroot/PG_InterOp $(PROVIDER_PATH)/scx_core_r.mof

unregister:
	$(CIMPROVIDER) -r -m SCXTestModule
	$(CIMPROVIDER) -r -m SCXCoreProviderModule


testenumeration.$(OBJ_FILE_SUFFIX) : $(TESTPROVIDER_PATH)/testenumeration.cpp $(TESTPROVIDER_PATH)/testenumeration.h
	$(CXX) $<

testinstance.$(OBJ_FILE_SUFFIX) : $(TESTPROVIDER_PATH)/testinstance.cpp $(TESTPROVIDER_PATH)/testinstance.h
	$(CXX) $<

entityinstance.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMCOMMON_PATH)/entityinstance.cpp $(SCXSYSTEM_INCLUDE_PATH)/entityinstance.h
	$(CXX) $<

scxkstat.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMCOMMON_PATH)/scxkstat.cpp $(SCXSYSTEM_INCLUDE_PATH)/scxkstat.h
	$(CXX) $<

cpuenumeration.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMCPU_PATH)/cpuenumeration.cpp $(SCXSYSTEM_INCLUDE_PATH)/cpuenumeration.h
	$(CXX) $<

cpuinstance.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMCPU_PATH)/cpuinstance.cpp $(SCXSYSTEM_INCLUDE_PATH)/cpuinstance.h
	$(CXX) $<

cpuprovider.$(OBJ_FILE_SUFFIX) : $(CPUPROVIDER_PATH)/cpuprovider.cpp $(CPUPROVIDER_PATH)/cpuprovider.h $(SCXPROVIDERLIB_INCLUDE_PATH)/cmpibase.h
	$(CXX) $<

memoryinstance.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMMEMORY_PATH)/memoryinstance.cpp $(SCXSYSTEM_INCLUDE_PATH)/memoryinstance.h
	$(CXX) $<

memoryenumeration.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMMEMORY_PATH)/memoryenumeration.cpp $(SCXSYSTEM_INCLUDE_PATH)/memoryenumeration.h
	$(CXX) $<

memoryprovider.$(OBJ_FILE_SUFFIX) : $(MEMORYPROVIDER_PATH)/memoryprovider.cpp $(MEMORYPROVIDER_PATH)/memoryprovider.h $(SCXPROVIDERLIB_INCLUDE_PATH)/cmpibase.h
	$(CXX) $<

diskinstance.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMDISK_PATH)/diskinstance.cpp $(SCXSYSTEM_INCLUDE_PATH)/diskinstance.h
	$(CXX) $<

diskenumeration.$(OBJ_FILE_SUFFIX) : $(PALSYSTEMDISK_PATH)/diskenumeration.cpp $(SCXSYSTEM_INCLUDE_PATH)/diskenumeration.h
	$(CXX) $<

diskprovider.$(OBJ_FILE_SUFFIX) : $(DISKPROVIDER_PATH)/diskprovider.cpp $(DISKPROVIDER_PATH)/diskprovider.h $(SCXPROVIDERLIB_INCLUDE_PATH)/cmpibase.h
	$(CXX) $<

testprovider.$(OBJ_FILE_SUFFIX) : $(TESTPROVIDER_PATH)/testprovider.cpp $(TESTPROVIDER_PATH)/testprovider.h $(SCXPROVIDERLIB_INCLUDE_PATH)/cmpibase.h
	$(CXX) $<

cmpibase.$(OBJ_FILE_SUFFIX) : $(PROVSUP_PATH)/cmpibase.cpp $(SCXPROVIDERLIB_INCLUDE_PATH)/cmpibase.h
	$(CXX) $<

scxinstance.$(OBJ_FILE_SUFFIX) : $(PROVSUP_PATH)/scxinstance.cpp $(SCXPROVIDERLIB_INCLUDE_PATH)/scxinstance.h
	$(CXX) $<

scxproperty.$(OBJ_FILE_SUFFIX) : $(PROVSUP_PATH)/scxproperty.cpp $(SCXPROVIDERLIB_INCLUDE_PATH)/scxproperty.h
	$(CXX) $<

scxinstancecollection.$(OBJ_FILE_SUFFIX) : $(PROVSUP_PATH)/scxinstancecollection.cpp $(SCXPROVIDERLIB_INCLUDE_PATH)/scxinstancecollection.h
	$(CXX) $<

scxprovidercapabilities.$(OBJ_FILE_SUFFIX) : $(PROVSUP_PATH)/scxprovidercapabilities.cpp $(SCXPROVIDERLIB_INCLUDE_PATH)/scxprovidercapabilities.h
	$(CXX) $<

ifneq ($(SCX_OS),win)
libpalsystem.$(LIB_FILE_SUFFIX) : entityinstance.$(OBJ_FILE_SUFFIX) cpuenumeration.$(OBJ_FILE_SUFFIX) cpuinstance.$(OBJ_FILE_SUFFIX) memoryenumeration.$(OBJ_FILE_SUFFIX) memoryinstance.$(OBJ_FILE_SUFFIX) diskenumeration.$(OBJ_FILE_SUFFIX) diskinstance.$(OBJ_FILE_SUFFIX) scxkstat.$(OBJ_FILE_SUFFIX)
	$(AR) $^

libprovidersupport.$(LIB_FILE_SUFFIX) : cmpibase.$(OBJ_FILE_SUFFIX) scxinstance.$(OBJ_FILE_SUFFIX) scxproperty.$(OBJ_FILE_SUFFIX) scxinstancecollection.$(OBJ_FILE_SUFFIX) scxprovidercapabilities.$(OBJ_FILE_SUFFIX)
	$(AR) $^
endif

stringaid.$(OBJ_FILE_SUFFIX) : $(UTIL_PATH)/stringaid.cpp $(SCXCOREINCLUDE_PATH)/stringaid.h
	$(CXX) $<

scxfilepath.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxfilepath.cpp $(SCXCOREINCLUDE_PATH)/scxfilepath.h $(SCXCOREINCLUDE_PATH)/stringaid.h
	$(CXX) $<

scxsleeper.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxsleeper.cpp $(SCXCOREINCLUDE_PATH)/scxsleeper.h
	$(CXX) $<

scxfilestream.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxfilestream.cpp $(SCXCOREINCLUDE_PATH)/scxfilestream.h $(SCXCOREINCLUDE_PATH)/stringaid.h
	$(CXX) $<

scxexception.$(OBJ_FILE_SUFFIX) : $(UTIL_PATH)/scxexception.cpp $(SCXCOREINCLUDE_PATH)/scxexception.h
	$(CXX) $<

scxmath.$(OBJ_FILE_SUFFIX) : $(UTIL_PATH)/scxmath.cpp $(SCXCOREINCLUDE_PATH)/scxmath.h 
	$(CXX) $<

scxthread.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxthread.cpp $(SCXCOREINCLUDE_PATH)/scxthread.h 
	$(CXX) $<

scxthreadlock.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxthreadlock.cpp $(SCXCOREINCLUDE_PATH)/scxthreadlock.h 
	$(CXX) $<

scxthreadlockhandle.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxthreadlockhandle.cpp $(SCXCOREINCLUDE_PATH)/scxthreadlock.h 
	$(CXX) $<

scxthreadlockfactory.$(OBJ_FILE_SUFFIX) : $(PAL_PATH)/os/scxthreadlockfactory.cpp $(SCXCOREINCLUDE_PATH)/scxthreadlock.h 
	$(CXX) $<

scxloghandle.$(OBJ_FILE_SUFFIX) : $(LOG_PATH)/scxloghandle.cpp $(SCXCOREINCLUDE_PATH)/scxlog.h 
	$(CXX) $<

scxloghandlefactory.$(OBJ_FILE_SUFFIX) : $(LOG_PATH)/scxloghandlefactory.cpp $(SCXCOREINCLUDE_PATH)/scxlog.h 
	$(CXX) $<

scxlogitem.$(OBJ_FILE_SUFFIX) : $(LOG_PATH)/scxlogitem.cpp $(LOG_PATH)/scxlogitem.h 
	$(CXX) $<

scxlogbackendsimple.$(OBJ_FILE_SUFFIX) : $(LOG_PATH)/scxlogbackendsimple.cpp $(LOG_PATH)/scxlogbackendsimple.h $(SCXCOREINCLUDE_PATH)/scxthreadlock.h $(SCXCOREINCLUDE_PATH)/scxfilestream.h
	$(CXX) $<

scxassert.$(OBJ_FILE_SUFFIX) : $(UTIL_PATH)/scxassert.cpp $(SCXCOREINCLUDE_PATH)/scxassert.h $(SCXCOREINCLUDE_PATH)/scxunit.h 
	$(CXX) $<

scxassert_abort.$(OBJ_FILE_SUFFIX) : $(UTIL_PATH)/scxassert_abort.cpp $(SCXCOREINCLUDE_PATH)/scxassert.h 
	$(CXX) $<

scxassert_cppunit.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/scxassert_cppunit.cpp $(SCXCOREINCLUDE_PATH)/scxassert.h 
	$(CXX) $<

#--------------------------------------------------------------------------------
# Static Libraries

libscxassertabort.$(LIB_FILE_SUFFIX) : scxassert_abort.$(OBJ_FILE_SUFFIX)
	$(AR) $^

ifeq ($(SCX_OS),win)

libscxcore.$(LIB_FILE_SUFFIX) : stringaid.$(OBJ_FILE_SUFFIX) scxassert.$(OBJ_FILE_SUFFIX) scxfilestream.$(OBJ_FILE_SUFFIX) scxfilepath.$(OBJ_FILE_SUFFIX) scxexception.$(OBJ_FILE_SUFFIX) scxmath.$(OBJ_FILE_SUFFIX) scxthread.$(OBJ_FILE_SUFFIX) scxthreadlock.$(OBJ_FILE_SUFFIX) scxthreadlockhandle.$(OBJ_FILE_SUFFIX) scxthreadlockfactory.$(OBJ_FILE_SUFFIX) scxloghandle.$(OBJ_FILE_SUFFIX) scxloghandlefactory.$(OBJ_FILE_SUFFIX) scxlogitem.$(OBJ_FILE_SUFFIX) scxlogbackendsimple.$(OBJ_FILE_SUFFIX)
	$(AR) $^

else

libscxcore.$(LIB_FILE_SUFFIX) : stringaid.$(OBJ_FILE_SUFFIX) scxassert.$(OBJ_FILE_SUFFIX) scxfilestream.$(OBJ_FILE_SUFFIX) scxfilepath.$(OBJ_FILE_SUFFIX) scxsleeper.$(OBJ_FILE_SUFFIX) scxexception.$(OBJ_FILE_SUFFIX) scxmath.$(OBJ_FILE_SUFFIX) scxthread.$(OBJ_FILE_SUFFIX) scxthreadlock.$(OBJ_FILE_SUFFIX) scxthreadlockhandle.$(OBJ_FILE_SUFFIX) scxthreadlockfactory.$(OBJ_FILE_SUFFIX) scxloghandle.$(OBJ_FILE_SUFFIX) scxloghandlefactory.$(OBJ_FILE_SUFFIX) scxlogitem.$(OBJ_FILE_SUFFIX) scxlogbackendsimple.$(OBJ_FILE_SUFFIX)
	$(AR) $^

endif

libcpuprovider.$(LIB_FILE_SUFFIX) : cpuprovider.$(OBJ_FILE_SUFFIX)
	$(AR) $^

libmemoryprovider.$(LIB_FILE_SUFFIX) : memoryprovider.$(OBJ_FILE_SUFFIX)
	$(AR) $^

libdiskprovider.$(LIB_FILE_SUFFIX) : diskprovider.$(OBJ_FILE_SUFFIX)
	$(AR) $^

#--------------------------------------------------------------------------------
# LINUX 

ifeq ($(SCX_OS),linux)

libSCXCoreProviderModule.so : \
	libcpuprovider.$(LIB_FILE_SUFFIX) \
	libmemoryprovider.$(LIB_FILE_SUFFIX) \
	libdiskprovider.$(LIB_FILE_SUFFIX) \
	libpalsystem.$(LIB_FILE_SUFFIX) \
	libprovidersupport.$(LIB_FILE_SUFFIX) \
	libscxcore.$(LIB_FILE_SUFFIX) \
	libscxassertabort.$(LIB_FILE_SUFFIX)
	$(SO) -u SCX_CPUProvider_Create_InstanceMI -u SCX_CPUProvider_Create_MethodMI -u SCX_MemoryProvider_Create_InstanceMI -u SCX_MemoryProvider_Create_MethodMI -u SCX_DiskProvider_Create_InstanceMI -u SCX_DiskProvider_Create_MethodMI -lcpuprovider -lmemoryprovider -ldiskprovider -lpalsystem -lprovidersupport -lscxassertabort -lscxcore -ldl -lpthread -lcrypt -lrt
	$(STRIP) -K SCX_CPUProvider_Create_InstanceMI -K SCX_CPUProvider_Create_MethodMI -K SCX_MemoryProvider_Create_InstanceMI -K SCX_MemoryProvider_Create_MethodMI  -K SCX_DiskProvider_Create_InstanceMI -K SCX_DiskProvider_Create_MethodMI libSCXCoreProviderModule.so

scx_install_iter1.tgz: libSCXTestProvider.so libSCXCoreProviderModule.so 
	/bin/sh $(SCX_ROOT_PATH)/installer/iter1/linux/build/scx_build.sh $(SCX_ROOT_PATH)

endif

#--------------------------------------------------------------------------------
# SOLARIS

ifeq ($(SCX_OS),solaris)

libSCXCoreProviderModule.so : \
	libcpuprovider.$(LIB_FILE_SUFFIX) \
	libmemoryprovider.$(LIB_FILE_SUFFIX) \
	libdiskprovider.$(LIB_FILE_SUFFIX) \
	libpalsystem.$(LIB_FILE_SUFFIX) \
	libprovidersupport.$(LIB_FILE_SUFFIX) \
	libscxcore.$(LIB_FILE_SUFFIX) \
	libscxassertabort.$(LIB_FILE_SUFFIX)
	$(SO) -u SCX_CPUProvider_Create_InstanceMI -u SCX_CPUProvider_Create_MethodMI -u SCX_MemoryProvider_Create_InstanceMI -u SCX_MemoryProvider_Create_MethodMI  -u SCX_DiskProvider_Create_InstanceMI -u SCX_DiskProvider_Create_MethodMI -lcpuprovider -lmemoryprovider -ldiskprovider -lpalsystem -lprovidersupport -lscxassertabort -lscxcore -ldl -lpthread -lcrypt -lrt -lkstat

scx_install_iter1.tgz: libSCXTestProvider.so libSCXCoreProviderModule.so 
#	/bin/sh $(SCX_ROOT_PATH)/installer/iter1/solaris/build/scx_build.sh $(SCX_ROOT_PATH)

endif

#--------------------------------------------------------------------------------

libSCXTestProvider.so : testprovider.$(OBJ_FILE_SUFFIX) testenumeration.$(OBJ_FILE_SUFFIX) testinstance.$(OBJ_FILE_SUFFIX) entityinstance.$(OBJ_FILE_SUFFIX) libprovidersupport.$(LIB_FILE_SUFFIX) libscxcore.$(LIB_FILE_SUFFIX) libscxassertabort.$(LIB_FILE_SUFFIX)
	$(SO) $^ -lprovidersupport -lscxcore -lscxassertabort -ldl -lpthread -lcrypt -lrt

scxstringaid_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/scxstringaid_test.cpp
	$(CXX_TESTRUN) $<

scxfilepath_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/pal/os/scxfilepath_test.cpp
	$(CXX_TESTRUN) $<

scxsleeper_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/pal/os/scxsleeper_test.cpp
	$(CXX_TESTRUN) $<

cpuenumeration_test.$(OBJ_FILE_SUFFIX) : $(TESTPALSYSTEM_PATH)/cpu/cpuenumeration_test.cpp
	$(CXX_TESTRUN) $<

memoryinstance_test.$(OBJ_FILE_SUFFIX) : $(TESTPALSYSTEM_PATH)/memory/memoryinstance_test.cpp 
	$(CXX_TESTRUN) $<

memoryenumeration_test.$(OBJ_FILE_SUFFIX) : $(TESTPALSYSTEM_PATH)/memory/memoryenumeration_test.cpp
	$(CXX_TESTRUN) $<

memoryprovider_test.$(OBJ_FILE_SUFFIX) : $(TESTMEMORYPROVIDER_PATH)/memoryprovider_test.cpp
	$(CXX_TESTRUN) $<

diskpal_test.$(OBJ_FILE_SUFFIX) : $(TESTPALSYSTEM_PATH)/disk/diskpal_test.cpp
	$(CXX_TESTRUN) $<

scxprovidercapabilities_test.$(OBJ_FILE_SUFFIX) : $(TESTPROVIDERSUPPORT_PATH)/scxprovidercapabilities_test.cpp
	$(CXX_TESTRUN) $<

datasampler_test.$(OBJ_FILE_SUFFIX) : $(TESTPALSYSTEM_PATH)/datasampler_test.cpp $(SCXSYSTEM_INCLUDE_PATH)/datasampler.h
	$(CXX_TESTRUN) $<

scxfilestream_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/pal/os/scxfilestream_test.cpp
	$(CXX_TESTRUN) $<

scxmath_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/scxmath_test.cpp
	$(CXX_TESTRUN) $<

scxthreadlock_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/pal/os/scxthreadlock_test.cpp
	$(CXX_TESTRUN) $<

scxthread_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/pal/os/scxthread_test.cpp
	$(CXX_TESTRUN) $<

scxloghandle_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/log/scxloghandle_test.cpp
	$(CXX_TESTRUN) $<

scxlogbackendsimple_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/log/scxlogbackendsimple_test.cpp
	$(CXX_TESTRUN) $<

scxexception_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/scxexception_test.cpp
	$(CXX_TESTRUN) $<

scxsingleton_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/scxsingleton_test.cpp
	$(CXX_TESTRUN) $<

scxhandle_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/util/scxhandle_test.cpp
	$(CXX_TESTRUN) $<

scxkstat_test.$(OBJ_FILE_SUFFIX) : $(TESTCOMMONLIB_PATH)/pal/system/common/scxkstat_test.cpp
	$(CXX_TESTRUN) $<

testrunner.$(OBJ_FILE_SUFFIX) : $(TEST_PATH)/util/testrunner.cpp
	$(CXX_TESTRUN) $<



TESTOBJS_COMMON = \
	testrunner.$(OBJ_FILE_SUFFIX)   \
	scxexception_test.$(OBJ_FILE_SUFFIX)  \
	scxstringaid_test.$(OBJ_FILE_SUFFIX)  \
	scxfilepath_test.$(OBJ_FILE_SUFFIX) \
	scxfilestream_test.$(OBJ_FILE_SUFFIX) \
	scxmath_test.$(OBJ_FILE_SUFFIX) \
	scxthreadlock_test.$(OBJ_FILE_SUFFIX) \
	scxthread_test.$(OBJ_FILE_SUFFIX) \
	scxloghandle_test.$(OBJ_FILE_SUFFIX) \
	scxlogbackendsimple_test.$(OBJ_FILE_SUFFIX) \
	scxsingleton_test.$(OBJ_FILE_SUFFIX) \
	scxhandle_test.$(OBJ_FILE_SUFFIX) \
	scxassert_cppunit.$(OBJ_FILE_SUFFIX) \
	libscxcore.$(LIB_FILE_SUFFIX) \


TESTOBJS_LINUX =  \
	scxsleeper_test.$(OBJ_FILE_SUFFIX) \
	$(TESTOBJS_COMMON) \
	cpuenumeration_test.$(OBJ_FILE_SUFFIX) \
	memoryinstance_test.$(OBJ_FILE_SUFFIX) \
	memoryenumeration_test.$(OBJ_FILE_SUFFIX) \
	datasampler_test.$(OBJ_FILE_SUFFIX) \
	memoryprovider_test.$(OBJ_FILE_SUFFIX) \
	diskpal_test.$(OBJ_FILE_SUFFIX) \
	scxprovidercapabilities_test.$(OBJ_FILE_SUFFIX) \
	scxkstat_test.$(OBJ_FILE_SUFFIX) \
	libpalsystem.$(LIB_FILE_SUFFIX) \
	cpuprovider.$(OBJ_FILE_SUFFIX)\
	memoryprovider.$(OBJ_FILE_SUFFIX)\
	libprovidersupport.$(LIB_FILE_SUFFIX) \

TESTOBJS_WIN =         \
	$(TESTOBJS_COMMON) \

ifeq ($(SCX_OS),linux)
testrunner.exe : $(TESTOBJS_LINUX)
	$(LINK) $^ $(CPPUNIT_LIB_PATH)/libcppunit.a -ldl -pthread -lrt
endif

ifeq ($(SCX_OS),solaris)
testrunner.exe : $(TESTOBJS_LINUX)
	$(LINK) $^ $(CPPUNIT_LIB_PATH)/libcppunit.a -ldl -lpthread -lrt -lkstat
endif

ifeq ($(SCX_OS),win)
testrunner.exe : $(TESTOBJS_WIN)
	$(LINK) $^ cppunitd.lib
endif

ifeq ($(SCX_OS),linux)
testrun : testrunner.exe iostat
	./testrunner.exe 
else
testrun : testrunner.exe
	./testrunner.exe 
endif

pegasustest : 
	cd ../../test/util/pegasustest; ./pegasus_test.sh; cd ../../../build/temporary

pegasustestcp : 
	cd ../../test/util/pegasustest; ./pegasus_test.sh copy; cd ../../../build/temporary

iostat:
	ln -f -s ../../test/ext/bin/linux/ia32/iostat iostat
	chmod a+x ../../test/ext/bin/linux/ia32/iostat

doxygen_corelib:
# Cannot use relative path which starts with .. on Win, have to use explicit path 
	-$(RMDIR) doxygen
	$(MKPATH) $(DOXYGEN_OUTPUT_PATH)
	( cat doxygen_scxcore.cfg ; echo "STRIP_FROM_PATH=$(SOURCE_PATH)/code" ) | doxygen -

doxygen_clean:
	-$(RMDIR) doxygen
