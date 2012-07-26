/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file
   
   \brief      Defines the openSSL certificate generation class
   
   \date       1-29-2008
   
   Wraps the openSSL certificate generation functions for use with the SCX
   installation tools.
   
*/

#ifndef SCXSSLCERT_H
#define SCXSSLCERT_H

#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxexception.h>
#include <iosfwd>
#include <openssl/x509.h>

/*----------------------------------------------------------------------------*/
/**
   Generic exception for SSL Certificate errors.
    
*/ 
class SCXSSLException : public SCXCoreLib::SCXException {
 public:
    /*----------------------------------------------------------------------------*/
    /**
       Ctor
       \param[in] reason       Cause of the exception.
       \param[in] l            Source code location object
    */
    SCXSSLException(std::wstring reason,
                    const SCXCoreLib::SCXCodeLocation& l) :
        SCXException(l),
        m_Reason(reason)
    { };

    std::wstring What() const;

 protected:
    //! The source code name of the violating pointer
    std::wstring   m_Reason;
};

/*----------------------------------------------------------------------------*/
/**
   openSSL certificate provides a wrapper class for the openSSL certificate
   generation functons.
   
   \author Carl Nicol
   \date 1-31-2008

   SCXSSLCertificate wraps the openSSL API calls used to generate certificates.

   \note Data is stored using wstring, however, the openSSL API calls use C-strings
   so all data is translated to a C-string before it is actually used.

*/
class SCXSSLCertificate
{
 private:
    /// The type of encoding used to produce the key.
    enum KeyType {
        eKeyTypeNone = 0,       ///< No encoding type specified.
        eKeyTypeRSA  = 1,       ///< RSA encoding
        eKeyTypeDSA  = 2,       ///< DSA encoding
        eKeyTypeDH   = 3,       ///< DH encoding
        eKeyTypeEC   = 4,       ///< EC encoding
        eKeyTypeMax  = 5        ///< Above range of encoding enums.
    };

    /// The certificate file format type
    enum FormatType {
        eFormatTypeNone = 0,    ///< No certificate format specified.
        eFormatTypeASN1 = 1,    ///< ASN1 formatted certificate.
        eFormatTypePEM  = 3,    ///< PEM formatted certificate
        eFormatTypeMax  = 4     ///< Above range of format types.
    };

    SCXCoreLib::SCXFilePath m_KeyPath;   ///< Path to key file;
    SCXCoreLib::SCXFilePath m_CertPath;  ///< Path to certificate;
    int m_startDays;                     ///< Days to offset valid start time with;
    int m_endDays;                       ///< Days to offset valid end time with;
    std::wstring m_hostname;             ///< Hostname
    std::wstring m_domainname;           ///< Domainname
    int m_bits;                          ///< Number of bits in key

 public:
    SCXSSLCertificate(SCXCoreLib::SCXFilePath keyPath, SCXCoreLib::SCXFilePath certPath,
                      int startDays, int endDays, const std::wstring & hostname,
                      const std::wstring & domainname, int bits);
    virtual ~SCXSSLCertificate();

    void LoadRndNumber();
    void SaveRndNumber();

    void Generate();

protected:
    size_t LoadRandomFromFile(const char* file, size_t num);

 private:
    /// Do not allow copy. Intentionally left unimplemented.
    SCXSSLCertificate( const SCXSSLCertificate & rhs );
    /// Do not allow copy. Intentionally left unimplemented.
    SCXSSLCertificate & operator=( const SCXSSLCertificate & rhs );

    void DoGenerate();
    void SetX509Properties(X509_REQ *req, EVP_PKEY *pkey);
    virtual size_t LoadRandomFromDevRandom(size_t randomNeeded);
    virtual size_t LoadRandomFromDevUrandom(size_t randomNeeded);
    virtual size_t LoadRandomFromUserFile();
    virtual void DisplaySeedWarning(size_t goodRandomNeeded);
};



#endif /* SCXSSLCERT_H */

/*--------------------------E-N-D---O-F---F-I-L-E----------------------------*/

