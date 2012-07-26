/*
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
*/

#ifndef Pegasus_FixedPropertyTableHpuxSCX_h
#define Pegasus_FixedPropertyTableHpuxSCX_h

#if defined(PEGASUS_USE_RELEASE_DIRS) && \
    defined(PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS)
# include <Pegasus/Config/ProductDirectoryStructure.h>
#endif

    {"providerManagerDir", "/opt/microsoft/scx/lib"},
    {"sslCertificateFilePath", "/etc/opt/microsoft/scx/ssl/scx.pem"},
    {"sslKeyFilePath",      "/etc/opt/microsoft/scx/ssl/scx-key.pem"},
    {"httpsPort",           "1270"},
#ifdef PEGASUS_USE_RELEASE_CONFIG_OPTIONS
//    {"httpPort",            "5988"},
    {"httpsPort",           "1270"},
    {"home",                ""},
    {"daemon",              "true"},
    {"slp",                 "false"},
    {"enableAssociationTraversal", "true"},
    {"enableIndicationService", "true"},
    {"httpAuthType",        "Basic"},
    {"repositoryIsDefaultInstanceProvider", "false"},
    {"enableBinaryRepository", "false"},
    {"maxProviderProcesses", "0"},
#endif

#ifdef PEGASUS_USE_RELEASE_DIRS
# if defined(PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS)
    {"traceFilePath",       PEGASUS_TRACE_FILE_PATH},
#  if !defined(PEGASUS_USE_SYSLOGS)
    {"logdir",              PEGASUS_LOG_DIR},
#  endif
    {"passwordFilePath",     PEGASUS_CONFIG_DIR"/cimserver.passwd"},
//    {"sslCertificateFilePath", PEGASUS_SSL_CERT_FILE_PATH},
//    {"sslKeyFilePath",       PEGASUS_SSL_KEY_FILE_PATH},
//    {"sslTrustStore",        PEGASUS_SSL_SERVER_TRUSTSTORE},
#  ifdef PEGASUS_ENABLE_SSL_CRL_VERIFICATION
//    {"crlStore",             PEGASUS_SSL_SERVER_CRL},
#  endif
    {"repositoryDir",        PEGASUS_REPOSITORY_DIR},
    {"providerDir",          PEGASUS_PROVIDER_LIB_DIR},

# else /* PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS */
    {"traceFilePath",       "/var/opt/wbem/trace/cimserver.trc"},
#  ifndef PEGASUS_USE_SYSLOGS
    {"logdir",              "/var/opt/wbem/logs"},
#  endif
    {"passwordFilePath",    "/etc/opt/wbem/cimserver.passwd"},
    {"sslCertificateFilePath", "/etc/opt/hp/sslshare/cert.pem"},
    {"sslKeyFilePath",      "/etc/opt/hp/sslshare/file.pem"},
    {"sslTrustStore",       "/etc/opt/hp/sslshare/cimserver_trust"},
    {"sslTrustStoreUserName", ""},
    {"crlStore",            "/etc/opt/hp/sslshare/crl"},
    {"repositoryDir",       PEGASUS_REPOSITORY_DIR},
    {"providerDir",         "/opt/wbem/providers/lib"},
    {"messageDir",         "/opt/wbem/share/locale/ICU_Messages"},
# endif /* PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS */
#endif /* defined(PEGASUS_USE_RELEASE_DIRS) */
#if !defined(PEGASUS_USE_RELEASE_CONFIG_OPTIONS) && \
    !defined(PEGASUS_USE_RELEASE_DIRS)
    {"bogus", "MyBogusValue"}      // Remove this line if others are added
#endif

#endif /* Pegasus_FixedPropertyTableHpuxSCX_h */