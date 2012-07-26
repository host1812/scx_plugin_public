##==============================================================================
##
## ssl-config.mak
##
##     This file defines these SSL configuration macros.
##
##         SSL_CFLAGS
##         SSL_LIBS
##         SSL_EXEC_PREFIX
##         SSL_LIBDIR
##
##     Makefiles requiring these should add the following lines before 
##     including the template makefile. 
##
##         EXTRA_LIBRARIES += $(SSL_LIBS)
##         SYS_INCLUDES += $(SSL_CFLAGS)
##
##     If possible, the values of these variables are determined with the
##     pkg-config tool as follows.
##
##         SSL_CFLAGS: pkg-config --cflags openssl
##         SSL_LIBS: pkg-config --libs openssl
##
##==============================================================================

#
# Determine whether we have pkg-config (set HAVE_PKG_CONFIG)
#

ifneq ($(shell pkg-config --variable=prefix openssl 2> /dev/null),"")
  HAVE_PKG_CONFIG=1
endif

#
# Set SSL_CFLAGS
#

# Use OPENSSL_HOME variable.
ifndef SSL_CFLAGS
  ifdef OPENSSL_HOME
    SSL_CFLAGS = -I$(OPENSSL_HOME)/include
  endif
endif

# Use pkg-config.
ifndef SSL_CFLAGS
  ifdef HAVE_PKG_CONFIG
    SSL_CFLAGS = $(shell pkg-config --cflags openssl)
  endif
endif

#
# Set SSL_LIBS
#

# Use Windows settings
ifndef SSL_LIBS
  ifeq ($(OS_TYPE),windows)
    SSL_LIBS = /libpath:$(OPENSSL_HOME)/lib libeay32.lib ssleay32.lib
  endif
endif

# Use VMS settings
ifndef SSL_LIBS
  ifeq ($(OS_TYPE),vms)
    SSL_LIBS = -L$(OPENSSL_LIB) -lssl$$libssl_shr32 -lssl$$libcrypto_shr32
  endif
endif

# Use OPENSSL_HOME
ifndef SSL_LIBS
  ifdef OPENSSL_HOME
    SSL_LIBS = -L$(OPENSSL_HOME)/lib -lssl -lcrypto
  endif
endif

# Use pkg-config
ifndef SSL_LIBS
  ifdef HAVE_PKG_CONFIG
    SSL_LIBS = $(shell pkg-config --libs openssl)
  endif
endif

# Use defaults
ifndef SSL_LIBS
  SSL_LIBS = -lssl -lcrypto
endif

#
# Set SSL_EXEC_PREFIX
#

ifndef SSL_EXEC_PREFIX
  ifdef HAVE_PKG_CONFIG
    SSL_EXEC_PREFIX = $(shell pkg-config --variable=exec_prefix openssl)
  endif
endif

#
# Set SSL_LIBDIR
#

ifndef SSL_LIBDIR
  ifdef HAVE_PKG_CONFIG
    SSL_LIBDIR = $(shell pkg-config --variable=libdir openssl)
  endif
endif
