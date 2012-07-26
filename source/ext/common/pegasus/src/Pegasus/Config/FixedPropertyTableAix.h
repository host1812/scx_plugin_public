//%LICENSE////////////////////////////////////////////////////////////////
//
// Licensed to The Open Group (TOG) under one or more contributor license
// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
// this work for additional information regarding copyright ownership.
// Each contributor licenses this file to you under the OpenPegasus Open
// Source License; you may not use this file except in compliance with the
// License.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////
//
//%/////////////////////////////////////////////////////////////////////////////

#ifdef PEGASUS_USE_RELEASE_CONFIG_OPTIONS

    {"logLevel",            "INFORMATION"},
    {"httpPort",            "5988"},
    {"httpsPort",           "5989"},
    {"daemon",              "true"},
#ifdef PEGASUS_ENABLE_SLP
    {"slp",                 "false"},
#endif
    {"enableAuthentication", "true"},
    {"enableAssociationTraversal", "true"},
    {"enableIndicationService", "true"},
    {"sslClientVerificationMode", "disabled"},
    {"httpAuthType",        "Basic"},
    {"repositoryIsDefaultInstanceProvider", "false"},
#endif

#ifdef PEGASUS_USE_RELEASE_DIRS
//    {"traceFilePath",    "/opt/freeware/cimom/pegasus/logs/cimserver.trc"},
//    {"logdir",           "/opt/freeware/cimom/pegasus/logs"},
//    {"sslCertificateFilePath", "/opt/freeware/cimom/pegasus/etc/cert.pem"},
//    {"sslKeyFilePath",   "/opt/freeware/cimom/pegasus/etc/file.pem"},
//    {"sslTrustFilePath", "/opt/freeware/cimom/pegasus/etc/client.pem"},
//    {"passwordFilePath", "/opt/freeware/cimom/pegasus/etc/cimserver.passwd"},
    {"messageDir",          "/opt/freeware/cimom/pegasus/msg"},
    {"repositoryDir",       PEGASUS_REPOSITORY_DIR},
    {"providerDir",         "/usr/lib:/usr/pegasus/provider/lib"},
#endif

#if !defined(PEGASUS_USE_RELEASE_CONFIG_OPTIONS) && \
    !defined(PEGASUS_USE_RELEASE_DIRS)
    {"bogus", "MyBogusValue"}      // Remove this line if others are added

#endif