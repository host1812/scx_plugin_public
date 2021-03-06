/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file

   \brief      openSSL configuration tool for SCX install tasks.

   \date       1-30-2008

*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxnameresolver.h>
#include <scxcorelib/scxdefaultlogpolicyfactory.h> // Using the default log policy.

#include "scxsslcert.h"

#include <errno.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using std::wcerr;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;
using SCXCoreLib::SCXFilePath;


static void usage(const char * name, int exitValue);
static int DoGenerate(const wstring & targetPath, int startDays, int endDays,
                      const wstring & hostname, const wstring & domainname,
    int bits);



/*----------------------------------------------------------------------------*/
/**
   main function.

   \param argc size of \a argv[]
   \param argv array of string pointers from the command line.
   \returns 0 on success, otherwise, 1 on error.

   Usage: scxgencert [-d domain]  [-h hostname] [-g targetpath] [-e days] [-s days]
   \n -v             toggle the debug flag.
   \n -?             print the help message and exit.
   \n -g targetpath  target path where certificates should be written
   \n -s days        days to offset valid start date with
   \n -e days        days to offset valid end date with
   \n -d domain      domain name
   \n -h host        hostname
   \n -b bits        number of key bits (defaults to 2048)

   Result Code
   \n -1  an exception has occured
   \n  0  success
   \n >1  an error occured while executing the command.

*/
int main(int argc, char *argv[])
{
    // commandline switches
    const string helpFlag       ("-?");
    const string bitsFlag       ("-b");
    const string domainFlag     ("-d");
    const string enddaysFlag    ("-e");
    const string forceFlag      ("-f");
    const string generateFlag   ("-g");
    const string hostFlag       ("-h");
    const string startdaysFlag  ("-s");
    const string debugFlag      ("-v");

    // Control variables built from command line arguments (defaulted as needed by SCX)
    bool debugMode = false;
    bool doGenerateCert = false;
    wstring targetPath = L"/etc/opt/microsoft/scx/ssl";
    int startDays = -365;
    int endDays = 7300;
#if defined(hpux) && defined(hppa)
    int bits = 512;
#else
    int bits = 2048;
#endif
    SCXCoreLib::NameResolver mi;
    wstring hostname(mi.GetHostname());
    wstring domainname(mi.GetDomainname());

    int i = 1;
    for (; i < argc; ++i)
    {
        if (debugFlag == argv[i])
        {
            debugMode = ! debugMode;
            wcout << L"Setting debugMode=" << (debugMode ? L"true" :L"false") << endl;
        }
        else if (helpFlag == argv[i])
        {
            usage(argv[0], 0);
        }
        else if (forceFlag == argv[i])
        {
            doGenerateCert = true;
        }
        else if(bitsFlag == argv[i])
        {
            if (++i >= argc)
            {
                wcout << L"Enter number of bits." << endl;
                usage(argv[0], 1);
            }
            bits = atoi(argv[i]);
            if (0 == bits || 0 != bits%512)
            {
                wcout << L"Bits must be non-zero dividable by 512." << endl;
                usage(argv[0], 1);
            }
        }
        else if(domainFlag == argv[i])
        {
            if (++i >= argc)
            {
                wcout << L"Enter a domain name." << endl;
                usage(argv[0], 1);
            }
            domainname = SCXCoreLib::StrFromMultibyte(argv[i]);
        }
        else if(hostFlag == argv[i])
        {
            if (++i >= argc)
            {
                wcout << "Enter a hostname." << endl;
                usage(argv[0], 1);
            }
            hostname = SCXCoreLib::StrFromMultibyte(argv[i]);
        }
        else if (generateFlag == argv[i])
        {
            // Ensure the path argument exists.
            if (++i >= argc)
            {
                wcout << "Enter a target path to generate certificates." << endl;
                usage(argv[0], 1);
            }
            targetPath = SCXCoreLib::StrFromMultibyte(argv[i]);
        }
        else if (startdaysFlag == argv[i])
        {
            // Ensure the value argument exists.
            if (++i >= argc)
            {
                wcout << "Enter a value for start days." << endl;
                usage(argv[0], 1);
            }
            startDays = atoi(argv[i]);
        }
        else if (enddaysFlag == argv[i])
        {
            // Ensure the value argument exists.
            if (++i >= argc || atoi(argv[i]) == 0)
            {
                wcout << "Enter a non-zero value for end days." << endl;
                usage(argv[0], 1);
            }
            endDays = atoi(argv[i]);
        }
        else
        {
            break;
        }
    }

    // Fail if all arguments are not used. 
    if (i < argc) {
        wcout << L"Unused arguments:" << endl;
        for (; i < argc; ++i)
        {
            wcout << L"\t" << argv[i] << endl;
        }
        wcout << endl;
        usage(argv[0], 1);
    }

    if(debugMode)
    {
        // Show what we would have used - even if user specified specific host/domain

        wcout << "Generated hostname:   \"" << mi.GetHostname()
              << "\" (" << mi.DumpSourceString(mi.GetHostnameSource()) << ")" << endl;
        wcout << "Generated domainname: \"" << mi.GetDomainname()
              << "\" (" << mi.DumpSourceString(mi.GetDomainnameSource()) << ")" << endl << endl;

        wcout << L"Host Name:     " << hostname << endl;
        wcout << L"Domain Name:   " << domainname << endl;
        wcout << L"Start Days:    " << startDays << endl;
        wcout << L"End Days:      " << endDays << endl;
        wcout << L"Cert Length:   " << bits << endl;
        wcout << L"Target Path:   " << targetPath << endl << endl;
    }

    // We only generate the certificate if "-f" was specified, or if no certificate exists
    // (Note: If no certificate exists, we should still return a success error code!)
    if (!doGenerateCert)
    {
        SCXFilePath keyPath;
        keyPath.SetDirectory(targetPath);
        keyPath.SetFilename(L"scx-key.pem");

        SCXCoreLib::SCXFileInfo keyInfo(keyPath);
        if ( ! keyInfo.Exists() )
        {
            doGenerateCert = true;
        }
        else
        {
            wcerr << L"Certificate not generated - '" << keyPath.Get() << "' exists" << endl;
        }
    }

    int rc = 0;
    if (doGenerateCert)
    {
        // We need to watch the length - SSL has a maxmimum length of 64 bytes

        if (hostname.length() + domainname.length() + 1 > 64)
        {
            domainname.clear();
        }

        size_t keyLen = hostname.length();
        if (domainname.length())
        {
            keyLen += domainname.length() + 1;
        }

        // Output what we'll be using for certificate generation
        wcout << "Generating certificate with hostname=\"" << hostname << "\"";
        if (domainname.length())
        {
            wcout << ", domainname=\"" << domainname << "\"";
        }
        wcout << endl;

        if (keyLen <= 64) {
            rc = DoGenerate(targetPath, startDays, endDays, hostname, domainname, bits);
        }
        else
        {
            wcout << "Hostname too long to generate certificate!  (TotalLen=" << keyLen << ")" << endl;
        }
    }

    if (debugMode)
    {
        wcout << L"return code = " << rc << endl;
    }
    exit(rc);
}

/*----------------------------------------------------------------------------*/
/**
   Output a usage message.
   
   \param name Application name (derived from argv[0]).
   \param exitValue Value to return after writing the usage message.
   \return Does not return.
*/
static void usage(char const * const name, int exitValue)
{
    wcout << L"Usage: " << name << L" [-v] [-s days] [-e days] [-d domain] [-h host] [-g targetpath]" << endl
          << endl
          << L"-v             - toggle debug flag" << endl
          << L"-g targetpath  - generate certificates in targetpath" << endl
          << L"-s days        - days to offset valid start date with (0)" << endl
          << L"-e days        - days to offset valid end date with (3650)" << endl
          << L"-f             - force certificate to be generated even if one exists" << endl
          << L"-d domain      - domain name" << endl
          << L"-h host        - host name" << endl
          << L"-b bits        - number of key bits" << endl
          << L"-?             - this help message" << endl
    ;
    exit(exitValue);
} 

/*----------------------------------------------------------------------------*/
/** 
    Generate Key and Certificate
    \param[in] targetPath Path where the certificates should be written.
    \param[in] startDays Days to offset valid start date.
    \param[in] endDays Days to offset valid end date.
    \param[in] hostname Hostname to put into the certificate.
    \param[in] domainname Domainname to put into the certificate.
    \param[in] bits Number of bits in key.
    \returns Zero on success.
*/
static int DoGenerate(const wstring & targetPath, int startDays, int endDays,
                      const wstring & hostname, const wstring & domainname,
    int bits)
{
    std::wstring c_certFilename(L"scx-host-");  // Remainder must be generated
    const std::wstring c_keyFilename(L"scx-key.pem");

    int rc = 0;
    // Do not allow an exception to slip out
    try
    {
        // The certificate filename must be something like scx-host-<hostname>.pem; generate it
        c_certFilename.append(hostname);
        c_certFilename.append(L".pem");

        SCXFilePath keyPath;
        keyPath.SetDirectory(targetPath);
        keyPath.SetFilename(c_keyFilename);
        SCXFilePath certPath;
        certPath.SetDirectory(targetPath);
        certPath.SetFilename(c_certFilename);
        SCXSSLCertificate cert(keyPath, certPath, startDays, endDays, hostname, domainname, bits);
        cert.Generate();

        /*
        ** We actually have three certificate files in total:
        **
        ** Certificate File: scx-host-<hostname>.pem  (public)
        ** Key File:         scx-key.pem              (private)
        ** Soft link:        scx.pem  (soft link to certificate file, used by openwsman)
        **
        **
        ** Create the soft link to point to the certificate file.
        */

        SCXFilePath fpLinkFile;
        fpLinkFile.SetDirectory(targetPath);
        fpLinkFile.SetFilename(L"scx.pem");

        std::string sLinkFile = SCXCoreLib::StrToMultibyte(fpLinkFile.Get());
        std::string sCertFile = SCXCoreLib::StrToMultibyte(certPath.Get());

        rc = unlink(sLinkFile.c_str());
        if (0 != rc && ENOENT != errno) {
            throw SCXCoreLib::SCXErrnoFileException(L"unlink", fpLinkFile.Get(), errno, SCXSRCLOCATION);
        }

        rc = symlink(sCertFile.c_str(), sLinkFile.c_str());
        if (0 != rc) {
            throw SCXCoreLib::SCXErrnoFileException(L"unlink", fpLinkFile.Get(), errno, SCXSRCLOCATION);
        }

        /*
        ** Finally, make sure the permissions are right:
        ** The public key gets 444, the private key gets 400
        */

        rc = chmod(sCertFile.c_str(), 00444);
        if (0 != rc) {
            throw SCXCoreLib::SCXErrnoFileException(L"chmod", certPath.Get(), errno, SCXSRCLOCATION);
        }

        std::string sKeyFile = SCXCoreLib::StrToMultibyte(keyPath.Get());
        rc = chmod(sKeyFile.c_str(), 00400);
        if (0 != rc) {
            throw SCXCoreLib::SCXErrnoFileException(L"chmod", keyPath.Get(), errno, SCXSRCLOCATION);
        }
    }
    catch(SCXCoreLib::SCXException & e)
    {
        wcout << e.Where() << endl
              << e.What() << endl;
        // use -1 to indicate an exception occured.
        rc = -1;
    }
    return rc;
}
