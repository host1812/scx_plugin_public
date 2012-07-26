/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Implements the physical disk instance pal for statistical information.
    
    \date        2008-04-28 15:20:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/statisticalphysicaldiskinstance.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>

#if defined(aix)
#include <sys/systemcfg.h>
#endif

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::StatisticalDiskInstance
*/
    StatisticalPhysicalDiskInstance::StatisticalPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps, bool isTotal /* = false*/) : StatisticalDiskInstance(deps, isTotal)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.statisticalphysicaldiskinstance");
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetReadsPerSecond
*/
    bool StatisticalPhysicalDiskInstance::GetReadsPerSecond(scxulong& value) const
    {
#if defined(hpux)
        value = 0;
        return false;
#else
        return StatisticalDiskInstance::GetReadsPerSecond(value);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetWritesPerSecond
*/
    bool StatisticalPhysicalDiskInstance::GetWritesPerSecond(scxulong& value) const
    {
#if defined(hpux)
        value = 0;
        return false;
#else
        return StatisticalDiskInstance::GetWritesPerSecond(value);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetBytesPerSecond
*/
    bool StatisticalPhysicalDiskInstance::GetBytesPerSecond(scxulong& read, scxulong& write) const
    {
#if defined(hpux)
        read = write = 0;
        return false;
#else
        return StatisticalDiskInstance::GetBytesPerSecond(read, write);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetDiskSize
*/
    bool StatisticalPhysicalDiskInstance::GetDiskSize(scxulong& mbUsed, scxulong& mbFree) const
    {
        mbUsed = mbFree = 0;
        return false;
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetBlockSize
*/
    bool StatisticalPhysicalDiskInstance::GetBlockSize(scxulong& blockSize) const
    {
        blockSize = 0;
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::StatisticalDiskInstance::Sample
    */
    void StatisticalPhysicalDiskInstance::Sample()
    {
#if defined(aix)
        perfstat_id_t id;
        perfstat_disk_t data;

        SCXCoreLib::SCXFilePath dev(m_device);
        memset(&id, 0, sizeof(id));
        strncpy(id.name, SCXCoreLib::StrToMultibyte(dev.GetFilename()).c_str(), sizeof(id.name)-1);
        if (1 == m_deps->perfstat_disk(&id, &data, sizeof(data), 1))
        {
            m_transfers.AddSample(data.xfers);
            m_rBytes.AddSample(data.rblks * data.bsize);
            m_wBytes.AddSample(data.wblks * data.bsize);
            m_tTimes.AddSample(data.time * 1000);

#if PF_MAJOR < 7

/* See /usr/include/sys/iplcb.h to explain the below */
#define XINTFRAC        (static_cast<double>(_system_configuration.Xint)/static_cast<double>(_system_configuration.Xfrac))

#endif

/* hardware ticks per millisecond */
#define HWTICS2MSECS(x)    ((static_cast<double>(x) * XINTFRAC)/1000000.0)

            m_rTimes.AddSample(HWTICS2MSECS(data.rserv));
            m_wTimes.AddSample(HWTICS2MSECS(data.wserv));
            m_qLengths.AddSample(data.qdepth);
        }
        else
        {
            SCX_LOGERROR(m_log, L"perfstat_disk failed");
            return;
        }
#elif defined(hpux)
        SCXCoreLib::SCXHandle<DeviceInstance> di = m_deps->FindDeviceInstance(m_device);

        if ((0 == di) || (DiskDepend::s_cINVALID_INSTANCE == di->m_instance))
        {
            SCX_LOGERROR(m_log, L"Unable to find disk in device map");
            return;
        }
        m_timeStamp.AddSample(time(0));
        
        struct pst_diskinfo diski;
        memset(&diski, 0, sizeof(diski)); 
        if (1 != m_deps->pstat_getdisk(&diski, sizeof(diski), 1, di->m_instance))
        {
            SCX_LOGERROR(m_log, L"pstat_getdisk failed");
            return;
        }
        // Sanity check of cahed instance id
        if (di->m_devID != ((diski.psd_dev.psd_major << 24) | diski.psd_dev.psd_minor))
        {
            SCX_LOGWARNING(m_log, L"Instance changed");
            di->m_instance = FindDiskInfoByID(di->m_devID);
            return;
        }
        m_transfers.AddSample(diski.psd_dkxfer);
        m_tBytes.AddSample(diski.psd_dkwds * 64);
        m_tTimes.AddSample(diski.psd_dkresp.pst_sec * 1000 + diski.psd_dkresp.pst_usec / 1000);
        m_waitTimes.AddSample(diski.psd_dkwait.pst_sec * 1000 + diski.psd_dkwait.pst_usec / 1000);
        m_qLengths.AddSample(diski.psd_dkqlen_curr);
#elif defined(linux)
        std::vector<std::wstring> parts = m_deps->GetProcDiskStats(m_device);
        for (size_t i=0; parts.size() == 0 && i < m_samplerDevices.size(); ++i)
        {
            parts = m_deps->GetProcDiskStats(m_samplerDevices[i]);
        }
        m_timeStamp.AddSample(time(0));
        if (parts.size() > 11)
        {
            try
            {
                m_reads.AddSample(SCXCoreLib::StrToULong(parts[3]));
                m_writes.AddSample(SCXCoreLib::StrToULong(parts[7]));
                m_rBytes.AddSample(SCXCoreLib::StrToULong(parts[5])*m_sectorSize);
                m_wBytes.AddSample(SCXCoreLib::StrToULong(parts[9])*m_sectorSize);
                m_rTimes.AddSample(SCXCoreLib::StrToULong(parts[6]));
                m_wTimes.AddSample(SCXCoreLib::StrToULong(parts[10]));
                m_transfers.AddSample(m_reads[0] + m_writes[0]);
                m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
                m_qLengths.AddSample(SCXCoreLib::StrToULong(parts[11]));
            }
            catch (const SCXCoreLib::SCXNotSupportedException& e)
            {
                SCX_LOGWARNING(m_log, std::wstring(L"Could not parse line from diskstats: ").append(L" - ").append(e.What()));
            }
        }
#elif defined(sun)
        try
        {
            std::wstring name = L"";
            if ( ! m_deps->ReadKstat(m_kstat, name, m_device, true))
            {
                SCX_LOGTRACE(m_log, L"Failed to get kstat path for disk: " + m_device);
                return;
            }
            bool isVopstats = SCXCoreLib::StrIsPrefix(name,L"vopstats_",true);
            
            if (isVopstats)
            {
                SCX_LOGHYSTERICAL(m_log, L"Unable to sample vopstats data");
            }
            else
            {
                m_reads.AddSample(m_kstat->GetValue(L"reads"));
                m_writes.AddSample(m_kstat->GetValue(L"writes"));
                m_transfers.AddSample(m_reads[0] + m_writes[0]); 
                m_rBytes.AddSample(m_kstat->GetValue(L"nread"));
                m_wBytes.AddSample(m_kstat->GetValue(L"nwritten"));
                m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
                m_runTimes.AddSample(m_kstat->GetValue(L"rtime")/1000/1000); //Store ms not ns
                m_waitTimes.AddSample(m_kstat->GetValue(L"wtime")/1000/1000); //Store ms not ns
                m_timeStamp.AddSample(MAX(m_kstat->GetValue(L"rlastupdate"),m_kstat->GetValue(L"wlastupdate"))/1000/1000);
                m_qLengths.AddSample(m_kstat->GetValue(L"wcnt"));
                
                SCX_LOGHYSTERICAL(m_log, L"nR=" + SCXCoreLib::StrFrom(m_reads[0])
                                  + L" nW=" + SCXCoreLib::StrFrom(m_writes[0])
                                  + L" bR=" + SCXCoreLib::StrFrom(m_rBytes[0])
                                  + L" bW=" + SCXCoreLib::StrFrom(m_wBytes[0])
                                  + L" tR=" + SCXCoreLib::StrFrom(m_runTimes[0])
                                  + L" tW=" + SCXCoreLib::StrFrom(m_waitTimes[0])
                                  + L" ts=" + SCXCoreLib::StrFrom(m_timeStamp[0]));
            }
        }
        catch (SCXKstatException& e)
        {
            SCX_LOGERROR(m_log, L"Unable to access kstat: " + e.What() + L" - " + e.Where());
        }
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetLastMetrics
*/
    bool StatisticalPhysicalDiskInstance::GetLastMetrics(scxulong& numR, scxulong& numW, scxulong& bytesR, scxulong& bytesW, scxulong& msR, scxulong& msW) const
    {
#if defined(aix) || defined(hpux)
        if (0 == m_transfers.GetNumberOfSamples())
        {
            return false;
        }
        numR = m_transfers[0];
        numW = 0;
#endif
#if defined(hpux)
        if (0 == m_tBytes.GetNumberOfSamples())
        {
            return false;
        }
        bytesR = m_tBytes[0];
        bytesW = 0;
#else
#if ! defined(aix)
        if (0 == m_reads.GetNumberOfSamples())
        {
            return false;
        }
        numR = m_reads[0];
        if (0 == m_writes.GetNumberOfSamples())
        {
            return false;
        }
        numW = m_writes[0];
#endif
        if (0 == m_rBytes.GetNumberOfSamples())
        {
            return false;
        }
        bytesR = m_rBytes[0];
        if (0 == m_wBytes.GetNumberOfSamples())
        {
            return false;
        }
        bytesW = m_wBytes[0];
#endif

#if defined(hpux)
        if (0 == m_tTimes.GetNumberOfSamples() || 0 == m_waitTimes.GetNumberOfSamples())
        {
            return false;
        }
        msW = m_waitTimes[0];
        msR = m_tTimes[0] - msW;
#elif defined(aix) || defined(linux)
        if (0 == m_rTimes.GetNumberOfSamples())
        {
            return false;
        }
        msR = m_rTimes[0];
        if (0 == m_wTimes.GetNumberOfSamples())
        {
            return false;
        }
        msW = m_wTimes[0];
#elif defined(sun)
        if (0 == m_runTimes.GetNumberOfSamples())
        {
            return false;
        }
        msR = m_runTimes[0];
        if (0 == m_waitTimes.GetNumberOfSamples())
        {
            return false;
        }
        msW = m_waitTimes[0];
#endif
        return true;
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
