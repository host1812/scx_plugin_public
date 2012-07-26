/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file

   \brief      Generates SSL certificates for the SCX Installer.

   \date       1-17-2008

   Implementation of class SCXSSLCertificate.

*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/engine.h>
#include <openssl/conf.h>

#include "scxsslcert.h"
#include "resourcehelper.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fcntl.h>

using std::wcout;
using std::wcerr;
using std::endl;
using std::ends;
using std::string;
using std::wstring;

using SCXCoreLib::SCXFile;
using SCXCoreLib::SCXNULLPointerException;
using SCXCoreLib::StrFromMultibyte;
using SCXCoreLib::StrToMultibyte;


/******************************************************************************
 *  
 *  SCXSSLException Implementation 
 *  
 ******************************************************************************/

/*----------------------------------------------------------------------------*/
/* (overload)
   Format which pointer was NULL
   
*/
wstring SCXSSLException::What() const 
{ 
    // Example:
    // Error generating SSL Certificate 'unable to write file'
    return L"Error generating SSL certificate '" + m_Reason + L"'";
 }

/******************************************************************************
 *  
 *  SCXSSLCertificate Implementation 
 *  
 ******************************************************************************/

//------------------------------------------------------------------------------
// Public Methods
//------------------------------------------------------------------------------

/*----------------------------------------------------------------------------*/
/**
   Constructor - Instantiate a SCXSSLCertificate object.

   \param[in] keyPath  Full path to the key file.
   \param[in] certPath Full path to the certificate file.
   \param[in] startDays Days to offset valid start time with.
   \param[in] endDays Days to offset valid end time with.
   \param[in] hostname Hostname to use in the certificates.
   \param[in] domainname Domainname to use in the certificates.
   \param[in] bits Number of bits in key.

   \date   1-17-2008
*/
SCXSSLCertificate::SCXSSLCertificate(SCXCoreLib::SCXFilePath keyPath, SCXCoreLib::SCXFilePath certPath,
                                     int startDays, int endDays, const wstring & hostname,
                                     const wstring & domainname, int bits)
    : m_KeyPath(keyPath),
      m_CertPath(certPath),
      m_startDays(startDays),
      m_endDays(endDays),
      m_hostname(hostname),
      m_domainname(domainname),
      m_bits(bits)
{
}

/*----------------------------------------------------------------------------*/
/**
   Destructor

   \throws SCXSSLException
*/
SCXSSLCertificate::~SCXSSLCertificate()
{
}

/*----------------------------------------------------------------------------*/
/**
   Tries to seed random from a file. Will never block.

   \param file Path to file with radnom bytes.
   \param num Number of bytes to read from file. Zero means complete file.
   \returns Number of bytes actually read from random file.

   If the num parameter is zero the file is read until it blocks.
*/

size_t SCXSSLCertificate::LoadRandomFromFile(const char* file, size_t num)
{
    size_t remain = num;
    size_t result = 0;
    int fd = open(file, O_RDONLY);
    if (-1 == fd)
        goto cleanup;
    if (-1 == fcntl(fd, F_SETFL, O_NONBLOCK))
        goto cleanup;

    char buffer[1024];
    while (remain > 0 || 0 == num) {
        size_t r = read(fd, buffer, (remain>sizeof(buffer) || 0 == num)?sizeof(buffer):remain);
        if (static_cast<size_t>(-1) == r)
            goto cleanup;
        if (0 == r)
            break;
        result += r;
        RAND_seed(buffer, static_cast<int>(r));
        remain -= r;
    }

 cleanup:
    if (-1 != fd)
        close(fd);
    return result;
}
    

/*----------------------------------------------------------------------------*/
/**
   Load the random number seed from a file.

   \throws SCXResourceExhaustedException

*/
void SCXSSLCertificate::LoadRndNumber()
{
    size_t nLoadedBytes = 0;
    const size_t randomNeeded = 1024;
    const size_t goodRandomNeeded = 256;

    nLoadedBytes = LoadRandomFromUserFile();

    // Even if we got a lot of good data from the user rnd file we add extra entropy 
    // from /dev/random if we can to get an even better seed.
    // In the best case scenario this means we have 2048 bytes of random to seed with
    nLoadedBytes += LoadRandomFromDevRandom(randomNeeded);

    // we want to get at least 256 bytes good random data in total from either file or
    // 'random' (ideally both)
    // based on open-ssl man page, "file" is a "good" source of random data
    // in case if file is missing, we take it from "random" device,
    // which is blocking (on some platforms) if there is not enough entropy
    // in that case we log a warning to the user and try to read the rest from urandom device
    // that is not a "good" source. 
    if ( nLoadedBytes < goodRandomNeeded )
    {
        DisplaySeedWarning(goodRandomNeeded);
        nLoadedBytes += LoadRandomFromDevUrandom(randomNeeded);
        // Should not fail, but making sure
        if ( nLoadedBytes < goodRandomNeeded )
        {
            throw SCXCoreLib::SCXResourceExhaustedException(L"random data", L"Failed to get random data - not enough entropy", SCXSRCLOCATION);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
   Save the random number seed to a file.

   \throws SCXSSLException

*/
void SCXSSLCertificate::SaveRndNumber()
{
    char buffer[200];
    const char* file = RAND_file_name(buffer, sizeof buffer);
    if (!RAND_write_file(file))
    {
        ; //TODO: Only log that no random file was found. DO NOT FAIL!
    }
}


/*----------------------------------------------------------------------------*/
/**
   Generate the certificates.
  
   \throws SCXCoreLib::SCXInvalidArgumentException

   \date   1-17-2008

   \note The certificates are generated using the hostname and domain name, which
   is collected automatically.

*/
void SCXSSLCertificate::Generate()
{
        if (0 == m_KeyPath.Get().size())
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"keyPath",
                 L"Key path is not set.", SCXSRCLOCATION);
        }
        if (0 == m_CertPath.Get().size())
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"certPath",
                 L"Certificate path is not set.", SCXSRCLOCATION);
        }
        SCXCoreLib::SCXFileInfo keyInfo(m_KeyPath.GetDirectory());
        SCXCoreLib::SCXFileInfo certInfo(m_CertPath.GetDirectory());
        if( ! keyInfo.PathExists())
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"keyPath", L"Path does not exist.", SCXSRCLOCATION);
        }
        if( ! certInfo.PathExists())
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"certPath", L"Path does not exist.", SCXSRCLOCATION);
        }

        LoadRndNumber();
        DoGenerate();
        SaveRndNumber();
}

//------------------------------------------------------------------------------
// Private Methods
//------------------------------------------------------------------------------

// Resource helper classes.

/** Helper class to construct an ASN1_INTEGER */
struct LoadASN1 {
    X509V3_EXT_METHOD * m_Method;               ///< method function pointer.
    const char *        m_SerialNumber;         ///< serial number

    /** CTOR
        \param m Method function pointer.
        \param s Serial number.
    */
    LoadASN1( X509V3_EXT_METHOD * m, const char * s) : m_Method(m), m_SerialNumber(s) 
    {
    }

    /**
       Create an ASN1_INTEGER struct.
       \returns A pointer to a newly created ASN1_INTEGER struct encoding the serial
       number with the given method.
    */
    ASN1_INTEGER * operator()()
    {
        return s2i_ASN1_INTEGER(m_Method, const_cast<char*>(m_SerialNumber));
    }
};

// Resource helper functions.

/**
   Function to make a macro behave like a static function.

   OpenSSL_add_all_algorithms() is a macro so it could not be passed as a function pointer.
*/
static void SSL_OpenSSL_add_all_algorithms()
{
    // Call to a macro
    OpenSSL_add_all_algorithms();
}

/*----------------------------------------------------------------------------*/
/**
   Generate the SSL Certificates.

   \throws SCXSSLException
   \throws SCXCoreLib::SCXNULLPointerException
*/
void SCXSSLCertificate::DoGenerate()
{
    try
    {
        int newKeyLength = m_bits;

        // Arguments from the command line.
        string outfile(StrToMultibyte(m_CertPath));
        string keyout(StrToMultibyte(m_KeyPath));

        ManagedResource res1(ERR_load_crypto_strings,        ERR_free_strings);
        ManagedResource res2(SSL_OpenSSL_add_all_algorithms, EVP_cleanup);
        ManagedResource res3(ENGINE_load_builtin_engines,    ENGINE_cleanup);

        // Serial number is always set to "1". 
        // This is a self-signed certificate. Serial number is unimportant.
        char one[] = "1";
        ManagedValueResource<ASN1_INTEGER> serial(LoadASN1(NULL, one)(), ASN1_INTEGER_free);
        if (0 == serial.Get())
        {
            throw SCXNULLPointerException(L"Error generating serial number", SCXSRCLOCATION);
        }

        ManagedValueResource<BIO> out(BIO_new(BIO_s_file()), BIO_free_all);
        if (0 == out.Get()) 
        {
            throw SCXSSLException(L"Failed to open out file", SCXSRCLOCATION);
        }
    
        // Allocate an empty private key structure.
        ManagedValueResource<EVP_PKEY> pkey(EVP_PKEY_new(), EVP_PKEY_free);
        if (pkey.Get() == 0) 
        {
            throw SCXNULLPointerException(L"Unable to allocate empty private key structure.",
                                                      SCXSRCLOCATION);
        }
    
        {
            RSA * rsa = RSA_generate_key(newKeyLength, 0x10001, 0, 0);
            if ( ! rsa )
            {
                throw SCXCoreLib::SCXNULLPointerException(L"Error allocating RSA structure.",
                                                      SCXSRCLOCATION);
            }
            if ( ! EVP_PKEY_assign_RSA(pkey.Get(), rsa))
            {
                // Free rsa if the assign was unsuccessful. (If it was successful, then rsa
                // is owned by pkey.)
                RSA_free(rsa);
                throw SCXSSLException(L"Error generating RSA key pair..", SCXSRCLOCATION);
            }
        }

        if (BIO_write_filename(out.Get(),const_cast<char*>(keyout.c_str())) <= 0)
        {
            int e = errno;
            char * p = strerror(e);
            std::wostringstream ss;
            ss << keyout.c_str() << L": ";
            if (0 != p)
            {
                ss << p;
            }
            else
            {
                ss << L"errno=" << e;
            }
            throw SCXSSLException(ss.str(), SCXSRCLOCATION);
        }

        if ( ! PEM_write_bio_PrivateKey(out.Get(),pkey.Get(),NULL,NULL,0,NULL,NULL))
        {
            throw SCXSSLException(L"Error writing private key file", SCXSRCLOCATION);
        }
    
        // Allocate a new X509_REQ structure
        ManagedValueResource<X509_REQ> req(X509_REQ_new(), X509_REQ_free);
        if (0 == req.Get())
        {
            throw SCXNULLPointerException(L"Unable to allocate memory for an X509_REQ struct.",
                                          SCXSRCLOCATION);
        }
    
        // Set the properties in the req structure from the private key.
        SetX509Properties(req.Get(),pkey.Get());
    
        ManagedValueResource<X509> x509ss(X509_new(), X509_free);
        if (0 == x509ss.Get()) 
        {
            throw SCXNULLPointerException(L"Error allocating X509 structure x509ss.",
                                          SCXSRCLOCATION);
        }

        if (!X509_set_serialNumber(x509ss.Get(), serial.Get()))
        {
            throw SCXSSLException(L"Unable to set certificate serial nubmer.", SCXSRCLOCATION);
        }
   
        // Copy the issuer name from the request.
        if (!X509_set_issuer_name(x509ss.Get(), X509_REQ_get_subject_name(req.Get()))) 
        {
            throw SCXSSLException(L"Unable to set issuer name.", SCXSRCLOCATION);
        }

        // Ensure the time is not before the certificate time.
        if (!X509_gmtime_adj(X509_get_notBefore(x509ss.Get()),(long)60*60*24*m_startDays)) 
        {
            throw SCXSSLException(L"Invalid time range.", SCXSRCLOCATION);
        }

        // Ensure the time is not after the certificate time.
        if (!X509_gmtime_adj(X509_get_notAfter(x509ss.Get()), (long)60*60*24*m_endDays)) 
        {
            throw SCXSSLException(L"Invalid time rangs", SCXSRCLOCATION);
        }

        // Copy the subject name from the request.
        if (!X509_set_subject_name(x509ss.Get(), X509_REQ_get_subject_name(req.Get()))) 
        {
            throw SCXSSLException(L"Unable to set subject name.", SCXSRCLOCATION);
        }

        {
            // Get the public key from the request, and set it in our cert.
            ManagedValueResource<EVP_PKEY> tmppkey(X509_REQ_get_pubkey(req.Get()), EVP_PKEY_free);
            if (!tmppkey.Get() || !X509_set_pubkey(x509ss.Get(),tmppkey.Get())) 
            {
                throw SCXSSLException(L"Unable to set the public key in the certificate", SCXSRCLOCATION);
            }
        }


        /* Set up V3 context struct */
        
        X509V3_CTX ext_ctx;
        X509V3_set_ctx(&ext_ctx, x509ss.Get(), x509ss.Get(), NULL, NULL, 0);

        // Sign the certificate
        const EVP_MD * digest = EVP_sha1();
        int i = X509_sign(x509ss.Get(),pkey.Get(),digest);
        if (! i)
        {
            throw SCXSSLException(L"Error signing certificate.", SCXSRCLOCATION);
        }

        // Write the new certificate to a file.
        if ( ! BIO_write_filename(out.Get(),const_cast<char*>(outfile.c_str())))
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Unable to open the cert file for writing.", SCXSRCLOCATION);
        }

        if ( ! PEM_write_bio_X509(out.Get(),x509ss.Get()))
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Error writing the cert file.", SCXSRCLOCATION);
        }

        // Cleanup the rest of the resources that may have been allocated internally.
        OBJ_cleanup();
        CONF_modules_unload(1); 
        CRYPTO_cleanup_all_ex_data(); 
        ERR_remove_state(0);
    } 
    catch (SCXCoreLib::SCXException & e)
    {
        // Blunt force resource release functions.
        OBJ_cleanup();
        CONF_modules_free();
        CRYPTO_cleanup_all_ex_data(); 
        ERR_remove_state(0);
        
        throw;
    }
}

/*----------------------------------------------------------------------------*/
/**
   Set the properties in the X509_REQ object.
   
   \param req pointer to openSSL X509 request information object.
   \param pkey pointer to PEM Private Key object.
   \throws SCXSSLException if unsuccessful.

   The X509_REQ object replaces the need for a seperate config file.

*/
void SCXSSLCertificate::SetX509Properties(X509_REQ *req, EVP_PKEY *pkey)
{
    // Set the version in the request.
    if ( ! X509_REQ_set_version(req,0L))
    {
        throw SCXSSLException(L"Unable to set the version number in the request.", SCXSRCLOCATION);
    }

    // Get the subject from the request.
    X509_NAME *subj = X509_REQ_get_subject_name(req);

    char dcPart[] = "DC";
    wstring tmp = m_domainname;
    wstring::size_type pos;
    while( ! tmp.empty())
    {
        if(wstring::npos != (pos = tmp.find_last_of(L'.')))
        {
            // Add the domain part to the subject.
            if ( ! X509_NAME_add_entry_by_txt(subj, dcPart, MBSTRING_ASC, 
                reinterpret_cast<unsigned char*>(const_cast<char*>(StrToMultibyte(tmp.substr(pos+1)).c_str())), -1, -1, 0))
            {
                throw SCXSSLException(L"Unable to add the domain to the subject.", SCXSRCLOCATION);
            }
            tmp.erase(pos);
        }
        else
        {
            // Add the domain part to the subject. 
            if ( ! X509_NAME_add_entry_by_txt(subj, dcPart, MBSTRING_ASC, 
                reinterpret_cast<unsigned char*>(const_cast<char*>(StrToMultibyte(tmp).c_str())), -1, -1, 0))
            {
                throw SCXSSLException(L"Unable to add the domain to the subject.", SCXSRCLOCATION);
            }
            tmp.erase();
        }
    }

    char cnPart[] = "CN";
    if ( ! X509_NAME_add_entry_by_txt(subj, cnPart, MBSTRING_ASC, 
        reinterpret_cast<unsigned char*>(const_cast<char*>(StrToMultibyte(m_hostname).c_str())), -1, -1, 0))
    {
        throw SCXSSLException(L"Unable to add hostname to the subject.", SCXSRCLOCATION);
    }
    wstring cn(m_hostname);
    // avoid putting the "." if there is no domain.
    if ( ! m_domainname.empty())
    {
        cn = SCXCoreLib::StrAppend(m_hostname,SCXCoreLib::StrAppend(L".",m_domainname));
    }
    if ( ! X509_NAME_add_entry_by_txt(subj, cnPart, MBSTRING_ASC, 
        reinterpret_cast<unsigned char*>(const_cast<char*>(StrToMultibyte(cn).c_str())), -1, -1, 0))
    {
        throw SCXSSLException(L"Unable to add the domain name to the subject.", SCXSRCLOCATION);
    }
    
    if ( ! X509_REQ_set_pubkey(req,pkey))
    {
        throw SCXSSLException(L"Unable to set the public key in the request.", SCXSRCLOCATION);
    }

}

/*----------------------------------------------------------------------------*/
/**
   Load random data from /dev/random
   
   \param randomNeeded Bytes of random data to read
   \returns Number of bytes read
   
*/
size_t SCXSSLCertificate::LoadRandomFromDevRandom(size_t randomNeeded)
{
    return LoadRandomFromFile("/dev/random", randomNeeded);
}

/*----------------------------------------------------------------------------*/
/**
   Load random data from /dev/urandom
   
   \param randomNeeded Bytes of random data to read
   \returns Number of bytes read
   
*/
size_t SCXSSLCertificate::LoadRandomFromDevUrandom(size_t randomNeeded)
{
    return LoadRandomFromFile("/dev/urandom", randomNeeded);
}

/*----------------------------------------------------------------------------*/
/**
   Load random data from user rnd file
   
   \returns Number of bytes read
   
*/
size_t SCXSSLCertificate::LoadRandomFromUserFile()
{
    char buffer[200];
    const char* file = RAND_file_name(buffer, sizeof buffer);
    if ( file != NULL )
    {
        // We load the entire user rnd file since:
        // 1. The default for OpenSSL is to create this file with 1024 bytes of data
        // 2. We will tell OpenSSL to replace this with new data when we are done
        return LoadRandomFromFile(file, 0);
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**
   Display warning to the user that not enough good random data could be read
   
   \param goodRandomNeeded Bytes of random data that was needed
   
*/
void SCXSSLCertificate::DisplaySeedWarning(size_t goodRandomNeeded)
{
    wcout << endl << L"WARNING!" << endl;
    wcout << L"Could not read " << goodRandomNeeded << L" bytes of random data from /dev/random. ";
    wcout << L"Will revert to less secure /dev/urandom." << endl;
    wcout << L"See the security guide for how to regenerate certificates at a later time when more random data might be available." << endl << endl;
}

/*--------------------------E-N-D---O-F---F-I-L-E----------------------------*/
