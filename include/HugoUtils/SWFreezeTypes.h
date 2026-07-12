/*
 * Copyright 2025-2026 howdy213, JYardX
 *
 * This file is part of HugoUtils.
 *
 * HugoUtils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HugoUtils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with HugoUtils. If not, see <https://www.gnu.org/licenses/>.
 *
 * SWFreeze kernel driver data structures.
 * Every structure corresponds to an IOCTL buffer. No hard-coded offsets are used.
 * Special thanks to Steve3184 for his reverse engineering of the SWFreeze driver.
 */
#pragma once
#include "WinUtils/WinPch.h"
#include <windows.h>
#include <cstdint>
#include <cstring>

#pragma pack(push, 8)

 // ===========================================================================
 // 0. Protect Volume Configuration (1024 bytes, pack(1))
 // ===========================================================================
#pragma pack(push, 1)
struct ProtectInfo {
    uint8_t  md5[16];                    // 0x00 : MD5 checksum (covers 0x10 .. end)
    uint32_t readytoProtectVolume;       // 0x10 : Volume mask to freeze after reboot
    uint32_t alreadyProtectVolume;       // 0x14 : Currently frozen volume mask
    uint8_t  diskNum;                    // 0x18 : Physical disk index
    int32_t  stopProtect;                // 0x19 : Emergency stop (1 = bypass IO)
    int32_t  needUpdate;                 // 0x1D : Pass-through / update mode
    int64_t  storageFileSize;            // 0x21 : Redirect cache file size
    int32_t  bRunSlowly;                 // 0x29 : Degraded performance flag
    uint32_t bsodNum;                    // 0x2D : Number of BSODs during freeze
    uint32_t bsodMaxUptime;              // 0x31 : Longest uptime while frozen (seconds)
    int32_t  blueHistoryReport;          // 0x35 : Enable BSOD history report
    uint32_t lastFreezeState;            // 0x39 : Last freeze state before shutdown
    uint32_t lastbsodRuntime;            // 0x3D : Uptime at last BSOD
    uint64_t lastsendbsodtime;           // 0x41 : FILETIME of last BSOD telemetry upload
    int32_t  coreDumpZipReport;          // 0x49 : Enable minidump packaging
    int32_t  isLastPagefileInFreezeVol;  // 0x4D : Pagefile on frozen volume
    int32_t  isLastVolumeCorrupt;        // 0x51 : Dirty NTFS flag at last boot
    uint8_t  iotDeviceID[19];            // 0x55 : IoT device code (null-terminated ASCII)
    uint8_t  iotSchoolID[5];             // 0x68 : IoT institution code
    int32_t  bNeedFreeze;                // 0x6D : Request freeze
    int32_t  bNeedUnFreeze;              // 0x71 : Request thaw
    uint16_t updateRebootCount;          // 0x75 : Consecutive reboot counter
    char     startupTime[20];            // 0x77 : Protection cycle start time string
    uint8_t  configVersion;              // 0x8B : Configuration version (usually 2)
    uint32_t volMaskCopy;                // 0x8C : Backup volume mask
    uint32_t updatingTimeSet;            // 0x90 : Scheduled pass-through enabled
    uint32_t updatingTimeNotAfter;       // 0x94 : Pass-through deadline (Unix timestamp)

    // Parse from raw buffer (minimum 1024 bytes)
    static ProtectInfo FromBuffer(const uint8_t* buf, size_t len);
    // Write to raw buffer (must be at least 1024 bytes)
    void ToBuffer(uint8_t* buf, size_t len) const;
};
static_assert(sizeof(ProtectInfo) == 0x98, "ProtectInfo size mismatch");
#pragma pack(pop)

// ===========================================================================
// 1. Boot & Event Statistics (1024 bytes, default alignment)
// ===========================================================================
struct FreezeBootSystem {
    uint32_t freezeStartupTime;
    int64_t  freezeDriverState;
    char     strDriverInitState[256];
    int32_t  readyProtectVolume;
    int32_t  bitmapCompState;
    int32_t  bootInfoRight;
    volatile int32_t reinitCallbackTime;
    volatile int32_t prefetchEnable;
    volatile int32_t originalIrpCount;
    volatile int32_t redirectIrpCount;
    volatile int64_t redirectAlgoTime;
    volatile int64_t readBytes;
    volatile int64_t writeBytes;
    volatile int64_t logonUIExitTime;

    static FreezeBootSystem FromBuffer(const uint8_t* buf, size_t len);
};

// ===========================================================================
// 2. Notification Event Handles (1024 bytes)
// ===========================================================================
struct FreezeNotifyHandles {
    void* hEvtDriverLoad;
    void* hEvtProcessCreate;
    void* hEvtPassThrough;
    void* hEvtOldDriverQuality;
    void* hEvtDiskFull;

    void ToBuffer(uint8_t* buf, size_t len) const;
};

// ===========================================================================
// 3. Low-Level Pass-Through Statistics (1024 bytes)
// ===========================================================================
struct FreezePassThrough {
    LARGE_INTEGER ataWriteDataSumSectors;
    LARGE_INTEGER ataWritePartTableSumSectors;
    LARGE_INTEGER ataReadDataSumSectors;
    LARGE_INTEGER ataReadPartTableSumSectors;
    LARGE_INTEGER scsiWriteDataSumSectors;
    LARGE_INTEGER scsiWritePartTableSumSectors;
    LARGE_INTEGER scsiReadDataSumSectors;
    LARGE_INTEGER scsiReadPartTableSumSectors;
    volatile int32_t ideRequestCount;
    volatile int32_t mpioRequestCount;

    static FreezePassThrough FromBuffer(const uint8_t* buf, size_t len);
};

// ===========================================================================
// 4. Driver Quality / Error Statistics (1024 bytes)
// ===========================================================================
struct FreezeOldDriverQuality {
    volatile int32_t irpInfoAllocFailed;
    volatile int32_t interBufAllocFailed;
    volatile int32_t interBufAllocSize;
    volatile int32_t checkMapTableFailed;
    volatile int32_t insertMapTableFailed;
    volatile int32_t setBitmapFailed;
    volatile int32_t subIrpFailed;
    volatile int32_t setRWBitmapFailed;

    static FreezeOldDriverQuality FromBuffer(const uint8_t* buf, size_t len);
};

// ===========================================================================
// 5. Disk Full / Free Sectors (1024 bytes, pack(4))
// ===========================================================================
#pragma pack(push, 4)
struct FreezeDiskFull {
    volatile int32_t volumes;
    volatile int64_t volFreeSectorCount[26];

    static FreezeDiskFull FromBuffer(const uint8_t* buf, size_t len);
};
#pragma pack(pop)

// ===========================================================================
// 6. Protected Image Information (1024 bytes)
// ===========================================================================
struct FreezeImageInfo {
    int32_t  imageId;
    uint8_t  imageFilePath[260];
    uint32_t majorVersion;
    uint32_t minorVersion;
    uint32_t buildNumber;
    uint32_t revisionNumber;
    uint8_t  imageCopyRight[260];

    static FreezeImageInfo FromBuffer(const uint8_t* buf, size_t len);
    void ToBuffer(uint8_t* buf, size_t len) const;
};

// ===========================================================================
// 7. Redirect Queue Statistics (1024 bytes, pack(4))
// ===========================================================================
#pragma pack(push, 4)
struct FreezeRedirectData {
    uint32_t timeIndex;
    uint32_t timeBase;
    uint32_t maxQueueLen;
    uint32_t originalIrpCount;
    uint32_t redirectIrpCount;
    uint64_t readBytes;
    uint64_t writeBytes;
    int64_t  maxIrpCompleteTime;
    int64_t  avgIrpCompleteTime;

    static FreezeRedirectData FromBuffer(const uint8_t* buf, size_t len);
};
#pragma pack(pop)

// ===========================================================================
// 8. BSOD / Crash Information (1024 bytes)
// ===========================================================================
struct FreezeBsodInfo {
    uint8_t  md5[16];
    uint32_t bsodTime;
    uint32_t startupTimeOccurBsod;

    static FreezeBsodInfo FromBuffer(const uint8_t* buf, size_t len);
};

// ===========================================================================
// 9. Core Initialization Result Set (2048 bytes, pack(1))
// ===========================================================================
#pragma pack(push, 1)
struct FreezeKeyResult {
    // Memory & process callback setup
    uint8_t  freeze_BugcheckDataMem_AllocaSuccess;
    uint32_t freeze_BugcheckDataMem_WriteSize;
    int32_t  freeze_SetProcessNotify_Status;

    // Initialization flow tracking
    uint32_t freeze_InitializerInit_ComIoDevCreate;
    uint32_t freeze_InitializerInit_AddDeviceCount;
    uint32_t freeze_InitializerInit_Start;

    // Volume config & cache file parsing
    uint32_t freeze_InitializerInit_VolConfig_OpenReg;
    uint32_t freeze_InitializerInit_VolConfig_OpenKey;
    uint32_t freeze_InitializerInit_VolConfig_Open;
    uint32_t freeze_InitializerInit_VolConfig_Read;
    uint32_t freeze_InitializerInit_VolConfig_Md5_Wrong;
    uint32_t freeze_InitializerInit_VolConfig_ReadSuccess;
    uint32_t freeze_InitializerInit_VolConfig_ReWrite;
    uint32_t freeze_InitializerInit_VolConfig_Over;

    // System & volume environment checks
    bool     freeze_InitializerInit_bChkdsk;
    bool     freeze_InitializerInit_bCheckDump;
    bool     freeze_InitializerInit_bStopProtect;
    bool     freeze_InitializerInit_bNeedUpdate;
    uint32_t freeze_InitializerInit_readyProtectVolume;
    int32_t  freeze_InitializerInit_StartInitProtectVolResource;

    // MBR / GPT partition analysis
    uint32_t freeze_InitializerInit_VolumeInfo_PartitionStyle_Mbr0_Gpt1;
    uint32_t freeze_InitializerInit_CalculateEBR;
    uint32_t freeze_InitializerInit_diskEbrNum;
    int32_t  freeze_InitializerInit_GetPartitionStyle_Over;

    // Resource allocation & hooking
    uint32_t freeze_InitializerInit_ProtectVolResource_gVolumeListisValid;
    uint32_t freeze_InitializerInit_ProtectVolResource_GetVolumeInfo_Start;
    uint32_t freeze_InitializerInit_ProtectVolResource_BitMap_Start;
    uint32_t freeze_InitializerInit_passthroughConfigFile;
    uint32_t freeze_InitializerInit_ProtectVolResource_FailVolume;
    uint32_t freeze_InitializerInit_StartInitProtectVolResource_Over;
    uint32_t freeze_InitializerInit_HookDiskMajorFun_GetObject;
    uint32_t freeze_InitializerInit_SetLoadImage;
    uint32_t freeze_InitializerInit_DriveImageLoadCallBack;
    uint32_t freeze_InitializerInit_Over;

    // Initialization log data
    bool     freeze_InitializerInit_errorRecorded;
    bool     freeze_InitializerInit_bRedirectSuccess;
    char     freeze_InitializerInit_FileFuncString[256];
    char     freeze_InitializerInit_LogString[256];
    char     freeze_InitializerInit_ErrorFileString[256];

    // Runtime behaviour monitoring
    uint8_t  freeze_AddDevice_getDiskNumber[26];
    int32_t  freeze_AddDevice_getDiskNumberStatus;
    uint32_t freeze_MajorFunction_firstRead;
    uint32_t freeze_MajorFunction_firstWrite;

    // IRP read/write handler dispatcher state
    uint32_t freeze_ReadWriteHandler_Start;
    uint32_t freeze_ReadWriteHandler_Noprotect_Update;
    uint32_t freeze_ReadWriteHandler_UniqueThread;
    uint32_t freeze_ReadWriteHandler_FilterIrpInBootRecord;
    uint32_t freeze_ReadWriteHandler_Vol_Valid;
    uint32_t freeze_ReadWriteHandler_Irp_InProtectVol;
    uint32_t freeze_ReadWriteHandler_KeSetEvent;
    uint32_t freeze_ReadWriteThread_Start;
    uint32_t freeze_ReadWriteThread_ConsumeIprQueue;
    uint32_t freeze_ReadWriteThread_DiskIrpHandler;
    uint32_t freeze_ReadWriteThread_FastFsdRequest;

    static FreezeKeyResult FromBuffer(const uint8_t* buf, size_t len);
};
static_assert(sizeof(FreezeKeyResult) <= 0x800, "FreezeKeyResult exceeds 2048 bytes");
#pragma pack(pop)

// ===========================================================================
// 10. Current Protection State (1024 bytes, pack(1))
// ===========================================================================
#pragma pack(push, 1)
struct FreezeProtectionState {
    uint64_t freezeDriverState;
    uint32_t stopProtect;
    uint64_t reserve;

    static FreezeProtectionState FromBuffer(const uint8_t* buf, size_t len);
};
#pragma pack(pop)

#pragma pack(pop)

// ===========================================================================
// Inline implementations of FromBuffer / ToBuffer
// ===========================================================================
inline ProtectInfo ProtectInfo::FromBuffer(const uint8_t* buf, size_t len) {
    ProtectInfo info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}
inline void ProtectInfo::ToBuffer(uint8_t* buf, size_t len) const {
    if (len >= 1024) {
        std::memset(buf, 0, 1024);
        std::memcpy(buf, this, sizeof(*this));
    }
    else if (len >= sizeof(*this)) {
        std::memcpy(buf, this, sizeof(*this));
    }
}

inline FreezeBootSystem FreezeBootSystem::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeBootSystem info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline void FreezeNotifyHandles::ToBuffer(uint8_t* buf, size_t len) const {
    if (len >= sizeof(*this)) std::memcpy(buf, this, sizeof(*this));
}

inline FreezePassThrough FreezePassThrough::FromBuffer(const uint8_t* buf, size_t len) {
    FreezePassThrough info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline FreezeOldDriverQuality FreezeOldDriverQuality::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeOldDriverQuality info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline FreezeDiskFull FreezeDiskFull::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeDiskFull info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline FreezeImageInfo FreezeImageInfo::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeImageInfo info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}
inline void FreezeImageInfo::ToBuffer(uint8_t* buf, size_t len) const {
    if (len >= sizeof(*this)) std::memcpy(buf, this, sizeof(*this));
}

inline FreezeRedirectData FreezeRedirectData::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeRedirectData info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline FreezeBsodInfo FreezeBsodInfo::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeBsodInfo info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline FreezeKeyResult FreezeKeyResult::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeKeyResult info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}

inline FreezeProtectionState FreezeProtectionState::FromBuffer(const uint8_t* buf, size_t len) {
    FreezeProtectionState info{};
    if (len >= sizeof(info)) std::memcpy(&info, buf, sizeof(info));
    return info;
}