//%2006////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, 2001, 2002 BMC Software; Hewlett-Packard Development
// Company, L.P.; IBM Corp.; The Open Group; Tivoli Systems.
// Copyright (c) 2003 BMC Software; Hewlett-Packard Development Company, L.P.;
// IBM Corp.; EMC Corporation, The Open Group.
// Copyright (c) 2004 BMC Software; Hewlett-Packard Development Company, L.P.;
// IBM Corp.; EMC Corporation; VERITAS Software Corporation; The Open Group.
// Copyright (c) 2005 Hewlett-Packard Development Company, L.P.; IBM Corp.;
// EMC Corporation; VERITAS Software Corporation; The Open Group.
// Copyright (c) 2006 Hewlett-Packard Development Company, L.P.; IBM Corp.;
// EMC Corporation; Symantec Corporation; The Open Group.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN
// ALL COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//==============================================================================
//
//%/////////////////////////////////////////////////////////////////////////////

#ifndef Pegasus_DefaultPropertyTableSCX_h
#define Pegasus_DefaultPropertyTableSCX_h

    {"socketWriteTimeout", PEGASUS_DEFAULT_SOCKETWRITE_TIMEOUT_SECONDS_STRING,
     IS_STATIC, 0, 0, IS_VISIBLE},
    {"idleConnectionTimeout", "0", IS_DYNAMIC, 0, 0, IS_VISIBLE},
    {"httpPort", "5988", IS_STATIC, 0, 0, IS_VISIBLE},
    {"forceProviderProcesses", "true", IS_STATIC, 0, 0, IS_VISIBLE},
    {"enableHttpConnection", "false", IS_STATIC, 0, 0, IS_VISIBLE},
    {"enableHttpsConnection", "true", IS_STATIC, 0, 0, IS_VISIBLE},
#if defined(PEGASUS_PLATFORM_DARWIN_IX86_GNU)
    {"providerManagerDir", "/usr/libexec/microsoft/scx/lib"},
#else
    {"providerManagerDir", "/opt/microsoft/scx/lib"},
#endif
    {"sslCertificateFilePath", "/etc/opt/microsoft/scx/ssl/scx.pem"},
    {"sslKeyFilePath",      "/etc/opt/microsoft/scx/ssl/scx-key.pem"},
    {"httpsPort",           "1270"},
#if defined(PEGASUS_PLATFORM_LINUX_GENERIC_GNU)
# include "DefaultPropertyTableLinuxSCX.h"
#elif PEGASUS_PLATFORM_HPUX_ACC
# include "DefaultPropertyTableHpuxSCX.h"
#else
    {"httpsPort", "1270", IS_STATIC, 0, 0, IS_VISIBLE},
    {"home", "./", IS_STATIC, 0, 0, IS_VISIBLE},
#if defined(PEGASUS_PLATFORM_AIX_RS_IBMCXX) || defined(PEGASUS_PLATFORM_DARWIN_IX86_GNU)
    // AIX and Mac OS (when using launchd) service start/stop don't like daemons that fork
    {"daemon", "false", IS_STATIC, 0, 0, IS_VISIBLE},
#else
    {"daemon", "true", IS_STATIC, 0, 0, IS_VISIBLE},
#endif
    {"slp", "false", IS_STATIC, 0, 0, IS_VISIBLE},
    {"enableAssociationTraversal", "true", IS_STATIC, 0, 0, IS_VISIBLE},
    {"enableIndicationService", "true", IS_STATIC, 0, 0, IS_VISIBLE},
    {"sslClientVerificationMode", "disabled", IS_STATIC, 0, 0, IS_VISIBLE},
# ifdef PEGASUS_ENABLE_AUDIT_LOGGER
    {"enableAuditLog", "false", IS_DYNAMIC, 0, 0, IS_VISIBLE},
# endif
    {"maxProviderProcesses", "0", IS_STATIC, 0, 0, IS_VISIBLE}
#endif

#endif /* Pegasus_DefaultPropertyTableSCX_h */
