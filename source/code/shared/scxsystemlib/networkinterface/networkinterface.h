/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Specification of network interface PAL 
    
    \date        08-03-03 12:23:02
    
*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxassert.h>
#include <algorithm>
#include <vector>
#include <string>
 
#if defined(sun)
#include <scxsystemlib/scxkstat.h>
#elif defined(aix)
#include <libperfstat.h>
#endif

namespace SCXSystemLib {
    /*----------------------------------------------------------------------------*/
    //! Encapsulates all external dependencies of the PAL
    class NetworkInterfaceDependencies 
    {
    public:
        //! Constructor
        NetworkInterfaceDependencies() { }
		
#if defined(linux)
        virtual SCXCoreLib::SCXFilePath GetDynamicInfoFile() const;
#elif defined(aix)
        virtual int perfstat_netinterface(perfstat_id_t *name, perfstat_netinterface_t *userbuff,
                                          size_t sizeof_struct, int desired_number);
#elif defined(sun)
        virtual SCXCoreLib::SCXHandle<SCXKstat> CreateKstat();
#endif
        virtual int ioctl(int fildes, int request, void *ifreqptr);
        //! Destructor
        virtual ~NetworkInterfaceDependencies() { }
    protected:
        //! Prevent copying to avoid slicing
        NetworkInterfaceDependencies(const NetworkInterfaceDependencies &);
    };
    
    /*----------------------------------------------------------------------------*/
    //! Information about a network interface
    //! \note IPAddress, netmask and broadcast address are only available if the interface 
    //!       up and running
    class NetworkInterfaceInfo {
    public:
        //! Identifers for attributes whose value might not be known
        enum OptionalAttribute {
            eIPAddress = 1,            //!< Represents the attribute "IP Address"
            eNetmask = 2,              //!< Represents the attribute "netmask"
            eBroadcastAddress = 4,     //!< Represents the attribute "broadcast address"
            eBytesReceived = 8,        //!< Represents the attribute "bytes received"
            eBytesSent = 16,           //!< Represents the attribute "bytes sent"
            ePacketsReceived = 32,     //!< Represents the attribute "packets received"
            ePacketsSent = 64,         //!< Represents the attribute "packets sent"
            eErrorsReceiving = 128,    //!< Represents the attribute "errors receiving"
            eErrorsSending = 256,      //!< Represents the attribute "errors sending"
            eCollisions = 512,         //!< Represents the attribute "collisions"
            eUp = 1024,                //!< Represents the attribute "up"
            eRunning = 2048            //!< Represents the attribute "running"
        };

        static std::vector<NetworkInterfaceInfo> FindAll(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        NetworkInterfaceInfo(const std::wstring &name, unsigned knownAttributesMask, 
                const std::wstring &ipAddress, const std::wstring &netmask, const std::wstring &broadcastAddress,
                scxulong bytesSent, scxulong bytesReceived,
                scxulong packetsSent, scxulong packetsReceived,
                scxulong errorsSending, scxulong errorsReceiving,
                scxulong collisions,
                bool up, bool running,
                SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        void Refresh();
        
        //! Name of interface
        //! \returns Name
        std::wstring GetName() const {return m_name; }
        
        //! Check if the value of an attribute is known
        //! \param[in] attr  Attribbute of interest
        //! \returns true iff the value of the optional attribute is present
        bool IsValueKnown(OptionalAttribute attr) const { return (m_knownAttributesMask & attr) != 0; }
        
        //! Check if the IPAddress is known
        //! \returns true iff the ipaddress is known
        bool IsIPAddressKnown() const {return IsValueKnown(eIPAddress); }
        
        //! IP Address assigned to interface
        //! \returns IP Address
        std::wstring GetIPAddress() const {SCXASSERT(IsValueKnown(eIPAddress)); return m_ipAddress; }
        
        //! Check if the netmask is known
        //! \returns true iff the netmask is known
        bool IsNetmaskKnown() const {return IsValueKnown(eNetmask); }

        //! Netmask assigned to interface
        //! \returns Netmask
        std::wstring GetNetmask() const {SCXASSERT(IsValueKnown(eNetmask)); return m_netmask; }
        
        //! Check if the broadcast address is known
        //! \returns true iff the broadcast address is known
        bool IsBroadcastAddressKnown() const {return IsValueKnown(eBroadcastAddress); }

        //! Broadcast address assigned to interface
        //! \returns Broadcast address
        std::wstring GetBroadcastAddress() const {SCXASSERT(IsValueKnown(eBroadcastAddress)); return m_broadcastAddress; }
        
        //! Check if bytes received is known
        //! \returns true iff bytes received is known
        bool IsBytesReceivedKnown() const {return IsValueKnown(eBytesReceived); }

        //! Number of bytes received from interface
        //! \returns    Number of bytes
        scxulong GetBytesReceived() const {SCXASSERT(IsValueKnown(eBytesReceived)); return m_bytesReceived; }
        
        //! Check if bytes sent is known
        //! \returns true iff bytes sent is known
        bool IsBytesSentKnown() const {return IsValueKnown(eBytesSent); }

        //! Number of bytes sent to interface
        //! \returns    Number of bytes
        scxulong GetBytesSent() const {SCXASSERT(IsValueKnown(eBytesSent)); return m_bytesSent; }
                
        //! Check if packets received is known
        //! \returns true iff packets received is known
        bool IsPacketsReceivedKnown() const {return IsValueKnown(ePacketsReceived); }

        //! Number of packets received from interface
        //! \returns    Number of packets
        scxulong GetPacketsReceived() const {SCXASSERT(IsValueKnown(ePacketsReceived)); return m_packetsReceived; }
        
        //! Check if packets sent is known
        //! \returns true iff packets sent is known
        bool IsPacketsSentKnown() const {return IsValueKnown(ePacketsSent); }

        //! Number of packets sent to interface
        //! \returns    Number of packets
        scxulong GetPacketsSent() const {SCXASSERT(IsValueKnown(ePacketsSent)); return m_packetsSent; }
        
        //! Check if receive errors is known
        //! \returns true iff receive errors is known
        bool IsKnownIfReceiveErrors() const {return IsValueKnown(eErrorsReceiving); }

        //! Number of errors that have occurred when receiving from interface
        //! \returns    Number of errors
        scxulong GetErrorsReceiving() const {SCXASSERT(IsKnownIfReceiveErrors()); return m_errorsReceiving; }        
        
        //! Check if send errors is known
        //! \returns true iff send errors is known
        bool IsKnownIfSendErrors() const {return IsValueKnown(eErrorsSending); }

        //! Number of errors that have occurred when sending to interface
        //! \returns    Number of errors
        scxulong GetErrorsSending() const {SCXASSERT(IsKnownIfSendErrors()); return m_errorsSending; }
        
        //! Check if collisions is known
        //! \returns true iff collisions is known
        bool IsKnownIfCollisions() const {return IsValueKnown(eCollisions); }
        
        //! Number of collisons that have occurred on interface
        //! \returns Number of collisions
        scxulong GetCollisions() const {SCXASSERT(IsKnownIfCollisions()); return m_collisions; }
        
        //! Check if up is known
        //! \returns true iff up is known
        bool IsKnownIfUp() const {return IsValueKnown(eUp); }

        //! Is the interface up
        //! \returns true iff the interface is up
        bool IsUp() const {SCXASSERT(IsValueKnown(eUp)); return m_up; }
        
        //! Check if running is known
        //! \returns true iff running is known
        bool IsKnownIfRunning() const {return IsValueKnown(eRunning); }

        //! Is the interface running, that is, are resources allocated
        //! \returns true iff the interface is running
        bool IsRunning() const {SCXASSERT(IsValueKnown(eRunning)); return m_running; }
        
        const std::wstring DumpString() const;        
       

        //! Clear the list of running interfaces
        //! (This is only used within test code - we need some refactoring to eliminate this)
        static void ClearRunningInterfaceList() { s_validInterfaces.clear(); }

    private: 
#if defined(sun)
        static void FindAllUsingKStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                      SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif
#if defined(aix)
        static void FindAllUsingPerfStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                         SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif
#if defined(linux)
        static void FindAllInFile(std::vector<NetworkInterfaceInfo> &interfaces,
                                  SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps); 
#endif
#if defined(hpux)
        static void FindAllNamed(const std::string &baseName,
                                 std::vector<NetworkInterfaceInfo> &interfaces,
                                 SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif

        NetworkInterfaceInfo(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        //! Test if this interface name is either currently running or was ever
        //! running in the past.  If the interface was never running, it's not
        //! returned.
        //! \returns true iff Interface is or has been running in the past
        static bool IsOrWasRunningInterface(std::wstring name)
        {
            return (s_validInterfaces.end() !=
                    std::find(s_validInterfaces.begin(), s_validInterfaces.end(), name));
        }

        std::wstring m_name;         //!< Name of interface
        unsigned m_knownAttributesMask;   //!< Bitmask holding which attributes that have known values 
        std::wstring m_ipAddress;         //!< IP Address (empty if none is available)
        std::wstring m_netmask;          //!< Netmask (empty if none is available)
        std::wstring m_broadcastAddress;  //!< Broadcast address (empty if none is available)
        scxulong m_bytesSent;        //!< Number of bytes sent to interface
        scxulong m_bytesReceived;    //!< Number of bytes received from interface
        scxulong m_packetsSent;      //!< Number of packets sent to interface
        scxulong m_packetsReceived;  //!< Number of bytes received from interface
        scxulong m_errorsSending;    //!< Number of errors that have occurred when sending to interface
        scxulong m_errorsReceiving;  //!< Number of errors that have occurred when receiving from interface
        scxulong m_collisions;       //!< Number of collisons that have occurred on interface
        bool m_up;                   //!< Is the interface up
        bool m_running;              //!< Is the interface running
        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps; //!< Dependencies to rely on
        static std::vector<std::wstring> s_validInterfaces; //!< List of interfaces that have been running
    };
}


#endif /* NETWORKINTERFACE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
