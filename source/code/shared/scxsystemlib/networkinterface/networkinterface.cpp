/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Implementation of network interface PAL

    \date        08-03-03 12:12:02

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include "networkinterface.h"
#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxdumpstring.h>

#if defined(aix)
#include <libperfstat.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

// Header file <strops.h> is on its way out on some systems.  See:
//
//      http://bugzilla.redhat.com/show_bug.cgi?id=444676
//
// for more information.

#if !(defined(PF_DISTRO_REDHAT) && PF_MAJOR >= 6)
#include <stropts.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>

#if defined(sun)
#include <sys/sockio.h>
#endif

using namespace SCXCoreLib;
using namespace std;

namespace {

using namespace SCXSystemLib;
using namespace SCXCoreLib;


#if defined(sun) || defined(linux)
/*----------------------------------------------------------------------------*/
//! Read a network interface name from a stream
//! \param[in]  source  To read from
//! \returns    Network interface name
std::wstring ReadInterfaceName(std::wistream &source) {
    std::wstring text;
    while (SCXStream::IsGood(source) && source.peek() == ' ') {
        source.get();
    }
    while (SCXStream::IsGood(source) && source.peek() != ':') {
        text += source.get();
    }
    source.get();
    return text;
}

#endif

/*----------------------------------------------------------------------------*/
// RAII encapsulation of file descriptor
class FileDescriptor {
public:
    //! Constructor
    //! \param[in]    Descriptor  To be managed
    FileDescriptor(int descriptor) : m_descriptor(descriptor) {
    }

    //! Destructor
    ~FileDescriptor() {
        if (m_descriptor >= 0) {
            close(m_descriptor);
        }
    }

    //! Enable normal use of the descriptor
    operator int() {
        return m_descriptor;
    }

private:
    int m_descriptor;   //!< Native descriptor to be managed
};

/*----------------------------------------------------------------------------*/
//! Convert a socket address to textual format
//! \param[in]  addr To be converted
//! \returns    Textual format
std::wstring ToString(sockaddr &addr) {
    std::wostringstream ipAddress;
    ipAddress << (unsigned char) addr.sa_data[2];
    ipAddress << L"." << (unsigned char) addr.sa_data[3];
    ipAddress << L"." << (unsigned char) addr.sa_data[4];
    ipAddress << L"." << (unsigned char) addr.sa_data[5];
    return ipAddress.str();
}

#if defined(sun)

/*----------------------------------------------------------------------------*/
//! Retrieve the value of an attribute
//! \param[in]    hasAttr     true if the attribute is present
//! \param[in]    attr     value of the attribute
//! \param[out]   attrId   Identifier of attribute
//! \param[out]   knownAttributesMask  Will contain id of attribute if value is present
//! \returns      Value of attribute if present, undefined otherwise
scxulong ValueOf(bool hasAttr, const scxulong attr, NetworkInterfaceInfo::OptionalAttribute attrId, unsigned &knownAttributesMask) {
    scxulong value = 0;
    if (hasAttr) {
        value = attr;
        knownAttributesMask |= attrId;
    }
    return value;
}

/*----------------------------------------------------------------------------*/
//! Retrieve the "best" value of an attribute
//! \param[in]    hasAttr64   true if the attr64 attribute is present
//! \param[in]    attr64        64-bit attribute value
//! \param[in]    hasAttr       true if the attr attribute is present
//! \param[in]    attr            32-bit attribute value
//! \param[out]   attrId   Identifier of attribute
//! \param[out]   knownAttributesMask  Will contain id of attribute if value is present
//! \returns      Best value of attribute if present, undefined otherwise
scxulong BestValueOf(bool hasAttr64, const scxulong attr64, bool hasAttr, const scxulong attr, NetworkInterfaceInfo::OptionalAttribute attrId, unsigned &knownAttributesMask) {
    scxulong value = 0;
    if (hasAttr64) {
        value = attr64;
        knownAttributesMask |= attrId;
    } else if (hasAttr) {
        value = attr;
        knownAttributesMask |= attrId;
    }
    return value;
}

#endif
}

namespace SCXSystemLib {

std::vector<std::wstring> NetworkInterfaceInfo::s_validInterfaces;

    /*----------------------------------------------------------------------------*/
    //! Dump object as string (for logging).
    //! \returns   String representation of object.
    const std::wstring NetworkInterfaceInfo::DumpString() const
    {
        return SCXDumpStringBuilder("NetworkInterfaceInfo")
            .Text("name", m_name)
            .Scalar("knownAttributesMask", m_knownAttributesMask)
            .Text("ipAddress", m_ipAddress)
            .Text("netmask", m_netmask)
            .Text("broadcastAddress", m_broadcastAddress)
            .Scalar("bytesSent", m_bytesSent)
            .Scalar("bytesReceived", m_bytesReceived)
            .Scalar("packetsSent", m_packetsSent)
            .Scalar("packetsReceived", m_packetsReceived)
            .Scalar("errorsSending", m_errorsSending)
            .Scalar("errorsReceiving", m_errorsReceiving)
            .Scalar("collisions", m_collisions)
            .Scalar("up", m_up)
            .Scalar("running", m_running);
    }


#if defined(sun)

/*----------------------------------------------------------------------------*/
//! Find all network interfaces using the KStat API
//! \param[out]    interfaces       To be populated
//! \param[in]     deps             Dependencies to rely on
//! \throws        SCXErrnoException   KStat problems
void NetworkInterfaceInfo::FindAllUsingKStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                             SCXHandle<NetworkInterfaceDependencies> deps) {

    SCXCoreLib::SCXHandle<SCXKstat> kstat = deps->CreateKstat();

    for(kstat_t* cur = kstat->ResetInternalIterator(); cur; cur = kstat->AdvanceInternalIterator())
    {
        if (strcmp(cur->ks_class, "net") == 0 && cur->ks_type == KSTAT_TYPE_NAMED)
        {
            scxulong ipackets;
            scxulong opackets;
            scxulong ipackets64;
            scxulong opackets64;
            scxulong rbytes;
            scxulong obytes;
            scxulong rbytes64;
            scxulong obytes64;
            scxulong ierrors;
            scxulong oerrors;
            scxulong collisions;
            scxulong lbufs;

            bool hasIpackets = kstat->TryGetValue(L"ipackets", ipackets);
            bool hasOpackets = kstat->TryGetValue(L"opackets", opackets);
            bool hasIpackets64 = kstat->TryGetValue(L"ipackets64", ipackets64);
            bool hasOpackets64 = kstat->TryGetValue(L"opackets64", opackets64);
            bool hasRbytes = kstat->TryGetValue(L"rbytes", rbytes);
            bool hasObytes = kstat->TryGetValue(L"obytes", obytes);
            bool hasRbytes64 = kstat->TryGetValue(L"rbytes64", rbytes64);
            bool hasObytes64 = kstat->TryGetValue(L"obytes64", obytes64);
            bool hasIerrors = kstat->TryGetValue(L"ierrors", ierrors);
            bool hasOerrors = kstat->TryGetValue(L"oerrors", oerrors);
            bool hasCollisions = kstat->TryGetValue(L"collisions", collisions);
            bool hasLbufs = kstat->TryGetValue(L"lbufs", lbufs);
            
            if (!hasLbufs && (hasIpackets || hasOpackets || hasIpackets64 || hasOpackets64 || hasRbytes || hasObytes
                    || hasRbytes64 || hasObytes64 || hasIerrors || hasOerrors || hasCollisions))
            {
                NetworkInterfaceInfo instance(deps);
                instance.m_name = StrFromMultibyte(cur->ks_name);
                instance.m_packetsSent = BestValueOf(hasOpackets64, opackets64, hasOpackets, opackets, ePacketsSent, instance.m_knownAttributesMask);
                instance.m_packetsReceived = BestValueOf(hasIpackets64, ipackets64, hasIpackets, ipackets, ePacketsReceived, instance.m_knownAttributesMask);
                instance.m_bytesSent = BestValueOf(hasObytes64, obytes64, hasObytes, obytes, eBytesSent, instance.m_knownAttributesMask);
                instance.m_bytesReceived = BestValueOf(hasRbytes64, rbytes64, hasRbytes, rbytes, eBytesReceived, instance.m_knownAttributesMask);
                instance.m_errorsSending = ValueOf(hasOerrors, oerrors, eErrorsSending, instance.m_knownAttributesMask);
                instance.m_errorsReceiving = ValueOf(hasIerrors, ierrors, eErrorsReceiving, instance.m_knownAttributesMask);
                instance.m_collisions = ValueOf(hasCollisions, collisions, eCollisions, instance.m_knownAttributesMask);
                interfaces.push_back(instance);
            }
        }
    }
}

#endif

#if defined(linux)
/*----------------------------------------------------------------------------*/
//! Find all network interfaces using the KStat API
//! \param[out]    interfaces          To be populated
//! \param[in]     deps                Dependencies to rely on
//! \throws        SCXErrnoException   KStat problems
void NetworkInterfaceInfo::FindAllInFile(std::vector<NetworkInterfaceInfo> &interfaces,
                                         SCXHandle<NetworkInterfaceDependencies> deps) {
    std::vector<std::wstring> lines;
    SCXStream::NLFs foundNlfs;
    SCXFile::ReadAllLines(deps->GetDynamicInfoFile(), lines, foundNlfs);
    for (size_t nr = 2; nr < lines.size(); nr++) {
        std::wistringstream infostream(lines[nr]);
        infostream.exceptions(std::ios::failbit | std::ios::badbit);
        NetworkInterfaceInfo instance(deps);
        instance.m_name = ReadInterfaceName(infostream);
        scxlong skip;
        infostream >> instance.m_bytesReceived;
        instance.m_knownAttributesMask |= eBytesReceived;
        infostream >> instance.m_packetsReceived;
        instance.m_knownAttributesMask |= ePacketsReceived;
        infostream >> instance.m_errorsReceiving;
        instance.m_knownAttributesMask |= eErrorsReceiving;
        infostream >> skip;
        infostream >> skip;
        infostream >> skip;
        infostream >> skip;
        infostream >> skip;
        infostream >> instance.m_bytesSent;
        instance.m_knownAttributesMask |= eBytesSent;
        infostream >> instance.m_packetsSent;
        instance.m_knownAttributesMask |= ePacketsSent;
        infostream >> instance.m_errorsSending;
        instance.m_knownAttributesMask |= eErrorsSending;
        infostream >> skip;
        infostream >> skip;
        infostream >> instance.m_collisions;
        instance.m_knownAttributesMask |= eCollisions;

        interfaces.push_back(instance);

    }

}
#endif

#if defined(aix)
/*----------------------------------------------------------------------------*/
//! Find all network interfaces using the perfstat API
//! \param[out]    interfaces       To be populated
//! \param[in]     deps             Dependencies to rely on
//! \throws        SCXErrnoException   perfstat problems
void NetworkInterfaceInfo::FindAllUsingPerfStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                             SCXHandle<NetworkInterfaceDependencies> deps) {

    perfstat_id_t first;
    int structsAvailable = deps->perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);
    if (structsAvailable < 0) {
        throw SCXErrnoException(L"perfstat_netinterface", errno, SCXSRCLOCATION);
    }
    vector<char> buffer(structsAvailable * sizeof(perfstat_netinterface_t));
    perfstat_netinterface_t *statp = reinterpret_cast<perfstat_netinterface_t *>(&buffer[0]);
    strcpy(first.name, FIRST_NETINTERFACE);
    int structsReturned = deps->perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), structsAvailable);
    if (structsReturned < 0) {
        throw SCXErrnoException(L"perfstat_netinterface", errno, SCXSRCLOCATION);
    }
    for (int nr = 0; nr < structsReturned; nr++) {
        // Currently there is no way to return type of network, our current CIM-model supports ethernet
        if (statp[nr].type == IFT_ETHER) {
            NetworkInterfaceInfo instance(deps);
            instance.m_name = StrFromMultibyte(statp[nr].name);

            instance.m_packetsSent = statp[nr].opackets;
            instance.m_knownAttributesMask |= ePacketsSent;

            instance.m_packetsReceived = statp[nr].ipackets;
            instance.m_knownAttributesMask |= ePacketsReceived;

            instance.m_bytesSent = statp[nr].obytes;
            instance.m_knownAttributesMask |= eBytesSent;

            instance.m_bytesReceived = statp[nr].ibytes;
            instance.m_knownAttributesMask |= eBytesReceived;

            instance.m_errorsSending = statp[nr].oerrors;
            instance.m_knownAttributesMask |= eErrorsSending;

            instance.m_errorsReceiving = statp[nr].ierrors;
            instance.m_knownAttributesMask |= eErrorsReceiving;

            instance.m_collisions = statp[nr].collisions;
            instance.m_knownAttributesMask |= eCollisions;

            interfaces.push_back(instance);
        }
    }
}
#endif

#if defined(hpux)
/*----------------------------------------------------------------------------*/
//! Find all whose name (without index) is baseName
//! \param[in]     baseName            Name except index
//! \param[out]    interfaces          To be populated
//! \param[in]     deps                Dependencies to rely on
void NetworkInterfaceInfo::FindAllNamed(const std::string &baseName,
                         std::vector<NetworkInterfaceInfo> &interfaces,
                         SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps) {
    for (unsigned intfNr = 0; ; intfNr++) {
        FileDescriptor fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ostringstream name;
        name << baseName.c_str() << intfNr;
        strcpy(ifr.ifr_name, name.str().c_str());
        if (deps->ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
            // WI 29507: Network discovery on HPUX fails on newer hardware
            //
            // On HP, we may get an error on the first interface, causing us to
            // not check later instances.  If the interface count is "low", keep
            // looking.
            //
            // Note that we allow up to four interfaces (two trunked interfaces)
            // or three LAN interfaces to return an error.
            if (intfNr <= 2)
            {
                continue;
            }

            break;
        }

        NetworkInterfaceInfo instance(deps);
        instance.m_name = StrFromMultibyte(name.str());
        interfaces.push_back(instance);
    }

}
#endif

#if defined(sun)
/*----------------------------------------------------------------------------*/
//! Constructs a kstat wrapper for the caller
//! \returns    a handle to the new kstat object
SCXCoreLib::SCXHandle<SCXKstat> NetworkInterfaceDependencies::CreateKstat()
{
    return SCXHandle<SCXKstat>(new SCXKstat());
}
#endif


#if defined(linux)
/*----------------------------------------------------------------------------*/
//! Retrieves the name of the file containing dynamic network interface properties
//! \returns    Path to file
SCXCoreLib::SCXFilePath NetworkInterfaceDependencies::GetDynamicInfoFile() const {
    return L"/proc/net/dev";
}

#endif

/*----------------------------------------------------------------------------*/
//! Perform a variety of control functions on devices
//! \param[in]     fildes    Open file descriptor
//! \param[in]     request   Control function selected
//! \param[in]     ifreqptr  Pointer to buffer to be initialized
int NetworkInterfaceDependencies::ioctl(int fildes, int request, void *ifreqptr) {
    return ::ioctl(fildes, request, ifreqptr);
}

#if defined(aix)
/*----------------------------------------------------------------------------*/
//! Find performance statistics for network interface
//! \param[in]   name    Either FIRST_NETINTERFACE, or the first network interface of interest
//! \param[in]   userbuff  Points to a memory area to be filled with structs
//! \param[in]   sizeof_struct Number of bytes of a struct
//! \param[in]   desiredNumber    Number of structs to fetch
//! \returns Number of structs copied to buffer, if name and userbuff are non null
//! If name or userbuff is NULL no structs are copied and the function returns the number of structs
//! that would have been copied.
int  NetworkInterfaceDependencies::perfstat_netinterface(perfstat_id_t *name, perfstat_netinterface_t *userbuff,
                                                         size_t sizeof_struct, int desiredNumber) {
    return ::perfstat_netinterface(name, userbuff, sizeof_struct, desiredNumber);
}
#endif

/*----------------------------------------------------------------------------*/
//! Make the information correspond to the current state of the system
void NetworkInterfaceInfo::Refresh() {
    vector<NetworkInterfaceInfo> latestInterfaces(FindAll(m_deps));
    for (size_t nr = 0; nr < latestInterfaces.size(); nr++) {
        if (latestInterfaces[nr].GetName() == GetName()) {
            *this = latestInterfaces[nr];
            break;
        }
    }

}

/*----------------------------------------------------------------------------*/
//! Find all network interfaces on the machine
//! \param[in]  deps    What this PAL depends on
//! \returns    Information on the network instances
std::vector<NetworkInterfaceInfo> NetworkInterfaceInfo::FindAll(SCXHandle<NetworkInterfaceDependencies> deps) {
    std::vector<NetworkInterfaceInfo> interfaces;
#if defined(linux)
    FindAllInFile(interfaces, deps);
#elif defined(sun)
    FindAllUsingKStat(interfaces, deps);
#elif defined(hpux)
    // The names are standardized according to
    // ifconfig man page. There is a wi5006
    // to collect the names in a more "proper" way
    FindAllNamed("lan", interfaces, deps);
    FindAllNamed("iptu", interfaces, deps);
    FindAllNamed("ip6tu", interfaces, deps);
    FindAllNamed("lo", interfaces, deps);
#elif defined(aix)
    FindAllUsingPerfStat(interfaces, deps);
#else
#error "Platform not supported"
#endif
    FileDescriptor fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    for (size_t nr = 0; nr < interfaces.size(); nr++) {
        NetworkInterfaceInfo &instance = interfaces[nr];
        strcpy(ifr.ifr_name, StrToMultibyte(instance.m_name).c_str());
        if (deps->ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
            instance.m_ipAddress = ToString(ifr.ifr_addr);
            instance.m_knownAttributesMask |= eIPAddress;
        }
        if (deps->ioctl(fd, SIOCGIFNETMASK, &ifr) >= 0) {
            instance.m_netmask = ToString(ifr.ifr_addr);
            instance.m_knownAttributesMask |= eNetmask;
        }
        if (deps->ioctl(fd, SIOCGIFBRDADDR, &ifr) >= 0) {
            instance.m_broadcastAddress = ToString(ifr.ifr_addr);
            instance.m_knownAttributesMask |= eBroadcastAddress;
        }
        if (deps->ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0) {
            instance.m_up = (ifr.ifr_flags & IFF_UP) != 0;
            instance.m_running = (ifr.ifr_flags & IFF_RUNNING) != 0;
            instance.m_knownAttributesMask |= eUp;
            instance.m_knownAttributesMask |= eRunning;
        }
    }

    std::vector<NetworkInterfaceInfo> resultList;
    for (size_t nr = 0; nr < interfaces.size(); nr++) {
        NetworkInterfaceInfo &instance = interfaces[nr];

        // If this interface is "UP" or "RUNNING", add it to our valid list (if needed)
        if ((instance.IsKnownIfUp() && instance.IsUp()) || (instance.IsKnownIfRunning() && instance.IsRunning())) {
            if (!IsOrWasRunningInterface(instance.GetName()))
            {
                s_validInterfaces.push_back(instance.GetName());
            }
        }

        // Only return the interface if it's in our valid list

        if (IsOrWasRunningInterface(instance.GetName()))
        {
            resultList.push_back(instance);
        }
    }

    return resultList;
}

/*----------------------------------------------------------------------------*/
//! Construct an instance out of known information
//! \param[in]  name    Name that identifies an interface
//! \param[in]  knownAttributesMask Bitmask where bits set indicates existing optional attriutes
//! \param[in]  ipAddress   IP-address assigned to the interface
//! \param[in]  netmask     Netmask indicating which network the interface belongs to
//! \param[in]  broadcastAddress    Broadcast address used by the interface
//! \param[in]  bytesSent           Number of bytes sent from the interface
//! \param[in]  bytesReceived       Number of bytes received on the interface
//! \param[in]  packetsSent         Number of packets sent from the interface
//! \param[in]  packetsReceived     Number of packets received on the interface
//! \param[in]  errorsSending       Number of errors that occurred when sending from the interface
//! \param[in]  errorsReceiving     Number of errors that occurred when receiving on the interface
//! \param[in]  collisions          Number of collisions that occured in communication
//! \param[in]  up                  Is the the interface "up"
//! \param[in]  running             Is the interface "running"
//! \param[in]  deps                Dependencies to rely on when information is refreshed
NetworkInterfaceInfo::NetworkInterfaceInfo(const std::wstring &name, unsigned knownAttributesMask,
        const std::wstring &ipAddress, const std::wstring &netmask, const std::wstring &broadcastAddress,
        scxulong bytesSent, scxulong bytesReceived,
        scxulong packetsSent, scxulong packetsReceived,
        scxulong errorsSending, scxulong errorsReceiving,
        scxulong collisions,
        bool up, bool running,
        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
        : m_name(name), m_knownAttributesMask(knownAttributesMask),
          m_ipAddress(ipAddress), m_netmask(netmask), m_broadcastAddress(broadcastAddress),
          m_bytesSent(bytesSent), m_bytesReceived(bytesReceived),
          m_packetsSent(packetsSent), m_packetsReceived(packetsReceived),
          m_errorsSending(errorsSending), m_errorsReceiving(errorsReceiving),
          m_collisions(collisions),
          m_up(up), m_running(running),
          m_deps(deps) {

}


/*----------------------------------------------------------------------------*/
//! Private constructor
//! \param[in]  deps    Dependencies to rely on
NetworkInterfaceInfo::NetworkInterfaceInfo(SCXHandle<NetworkInterfaceDependencies> deps) :
    m_knownAttributesMask(0),
    m_bytesSent(0), m_bytesReceived(0),
    m_packetsSent(0), m_packetsReceived(0),
    m_errorsSending(0), m_errorsReceiving(0),
    m_collisions(0), m_up(false), m_running(false),
    m_deps(deps)
{
}


}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
