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

#if defined(PEGASUS_USE_RELEASE_DIRS) && \
    defined(PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS)
# include <Pegasus/Config/ProductDirectoryStructure.h>
#endif

    {"providerManagerDir", "/usr/libexec/microsoft/scx/lib"},
    {"sslCertificateFilePath", "/etc/opt/microsoft/scx/ssl/scx.pem"},
    {"sslKeyFilePath",      "/etc/opt/microsoft/scx/ssl/scx-key.pem"},
    {"httpsPort",           "1270"},
#ifdef PEGASUS_USE_RELEASE_CONFIG_OPTIONS
    {"httpPort",            "5988"},
    {"home",                ""},
    {"install",             "false"},
    {"remove",              "false"},
    {"slp",                 "false"},
    {"httpAuthType",        "Basic"},
    {"enableBinaryRepository", "false"},
#endif
#if defined(PEGASUS_USE_RELEASE_DIRS)
# if defined(PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS)
    {"traceFilePath",       PEGASUS_TRACE_FILE_PATH},
#  if !defined(PEGASUS_USE_SYSLOGS)
    {"logdir",              PEGASUS_LOG_DIR},
#  endif
    {"passwordFilePath",     PEGASUS_CONFIG_DIR"/cimserver.passwd"},
    {"sslTrustStore",        PEGASUS_SSL_SERVER_TRUSTSTORE},
#  ifdef PEGASUS_ENABLE_SSL_CRL_VERIFICATION
    {"crlStore",             PEGASUS_SSL_SERVER_CRL},
#  endif
    {"repositoryDir",        PEGASUS_REPOSITORY_DIR},
/*
    SCX: Removed since we dont want any providers under /usr/
    {"providerDir", PEGASUS_PROVIDER_LIB_DIR ":/usr/" PEGASUS_ARCH_LIB "/cmpi"},
*/
    {"providerDir", PEGASUS_PROVIDER_LIB_DIR },
# else /* PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS */
    {"traceFilePath",       "/var/opt/tog-pegasus/cache/trace/cimserver.trc"},
#  if !defined(PEGASUS_USE_SYSLOGS)
    {"logdir",              "/var/opt/tog-pegasus/log"},
#  endif
    {"passwordFilePath",    "/etc/opt/tog-pegasus/cimserver.passwd"},
    {"sslTrustStore",       "/etc/opt/tog-pegasus/cimserver_trust"},
    {"crlStore",            "/etc/opt/tog-pegasus/crl"},
    {"repositoryDir",       PEGASUS_REPOSITORY_DIR},
#  if defined(PEGASUS_ENABLE_CMPI_PROVIDER_MANAGER)
    {"providerDir",         "/usr/libexec/tog-pegasus/providers/lib:/usr/"
                                PEGASUS_ARCH_LIB "/cmpi"},
#  else
    {"providerDir",         "/usr/libexec/tog-pegasus/providers/lib"},
#  endif
    {"messageDir",          "/usr/libexec/tog-pegasus/share/locale/ICU_Messages"},
# endif /* PEGASUS_OVERRIDE_DEFAULT_RELEASE_DIRS */
#endif /* defined(PEGASUS_USE_RELEASE_DIRS) */
#if !defined(PEGASUS_USE_RELEASE_CONFIG_OPTIONS) && \
    !defined(PEGASUS_USE_RELEASE_DIRS)
    {"bogus", "MyBogusValue"} /* Remove this line if others are added */
#endif
