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
 * Every structure corresponds to an IOCTL buffer. All constants (offsets,
 * IOCTL codes, paths, sizes and marker values) live here; no magic numbers are
 * allowed anywhere else in the freeze module.
 * Special thanks to Steve3184 for his reverse engineering of the SWFreeze driver.
 */
#pragma once
#include "WinUtils/WinPch.h"
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <string>

#pragma pack(push, 8)

 // ===========================================================================
 // Global constants - VolumeInfo.config layout (1024 bytes)
 // ===========================================================================
inline constexpr size_t FRZ_CONFIG_SIZE = 1024;      // 1 KB configuration blob
inline constexpr size_t FRZ_CONFIG_MD5_SIZE = 16;    // MD5 digest prefix
inline constexpr size_t FRZ_CONFIG_VALID_LEN = 0x90; // meaningful boot config length
inline constexpr size_t FRZ_CONFIG_INFO_END = 0x98;  // ProtectInfo ends at this blob offset

// Field offsets inside the 1024-byte configuration blob
inline constexpr size_t FRZ_OFF_MD5 = 0x00;            // MD5 digest (covers 0x10..end)
inline constexpr size_t FRZ_OFF_NEXT_MASK = 0x10;      // mask frozen after reboot
inline constexpr size_t FRZ_OFF_FLAG1 = 0x2D;          // write marker #1
inline constexpr size_t FRZ_OFF_STATUS = 0x31;         // status word
inline constexpr size_t FRZ_OFF_FLAG2 = 0x6D;          // write marker #2
inline constexpr size_t FRZ_OFF_VOL_MASK_COPY = 0x8C;  // backup volume mask
inline constexpr size_t FRZ_OFF_DEVICE_ID = 0x55;      // IoT device code (ASCII, NUL ended)
inline constexpr size_t FRZ_OFF_SCHOOL_CODE = 0x68;    // IoT school code (ASCII)
inline constexpr size_t FRZ_SCHOOL_CODE_LEN = 4;

// Values written while building a new configuration
inline constexpr uint8_t  FRZ_FLAG1_WRITE = 0x02;
inline constexpr uint8_t  FRZ_FLAG2_WRITE = 0x01;
inline constexpr uint16_t FRZ_STATUS_FROZEN = 0x0000;
inline constexpr uint16_t FRZ_STATUS_UNFROZEN = 0x03E4;

// ---- Runtime status buffer (IOCTL_QUERY_RUNTIME) ----
inline constexpr size_t FRZ_OFF_RT_PTR1 = 0x04;
inline constexpr size_t FRZ_OFF_RT_LOG = 0x0C;
inline constexpr size_t FRZ_OFF_RT_ACTIVE = 0x114;
inline constexpr size_t FRZ_OFF_RT_STATS = 0x118;

// ---- Driver device path / config file path ----
inline constexpr const wchar_t* FRZ_DRIVER_PATH = L"\\\\.\\SWFreeze";
inline constexpr const wchar_t* FRZ_CONFIG_PATH =
L"C:\\ProgramData\\SeewoFreezeKernelConfig\\VolumeInfo.config";

// ---- NTSTATUS values returned by the driver ----
// The driver rejects every request whose buffer is smaller than the required
// size with STATUS_UNSUCCESSFUL, so it must be treated as a failure.
inline constexpr DWORD FRZ_STATUS_UNSUCCESSFUL = 0xC0000001;

// ---- Required IOCTL buffer sizes ----
inline constexpr size_t FRZ_DRIVER_BUFFER_SIZE = 0x400;         // most IOCTLs
inline constexpr size_t FRZ_DRIVER_KEY_BUFFER_SIZE = 0x800;     // key result
inline constexpr size_t FRZ_DRIVER_REDIRECT_TID_SIZE = 0x828;   // TID array + redirect data

// ---- Fixed-length string fields ----
inline constexpr size_t FRZ_LOG_STRING_LEN = 256;   // driver log strings
inline constexpr size_t FRZ_IMAGE_MAX_PATH = 260;   // MAX_PATH for image paths
inline constexpr size_t FRZ_DRIVE_COUNT = 26;       // A..Z
inline constexpr size_t FRZ_IOT_DEVICE_ID_LEN = 19; // IoT device code incl. NUL
inline constexpr size_t FRZ_IOT_SCHOOL_ID_LEN = 5;  // IoT school code incl. NUL
inline constexpr size_t FRZ_STARTUP_TIME_LEN = 20;  // start time string

// ---- IOCTL control codes ----
inline constexpr DWORD FRZ_IOCTL_QUERY_RUNTIME = 0x80002038;   // boot system
inline constexpr DWORD FRZ_IOCTL_READ_MEM_CONF = 0x80002008;
inline constexpr DWORD FRZ_IOCTL_PREPARE_WRITE = 0x80002064;

inline constexpr DWORD FRZ_IOCTL_QUERY_BASE_VERSION = 0x80002004;
inline constexpr DWORD FRZ_IOCTL_QUERY_PROTECT_VOL_INFO = 0x80002008;
inline constexpr DWORD FRZ_IOCTL_QUERY_KEY_RESULT = 0x80002028;
inline constexpr DWORD FRZ_IOCTL_QUERY_PROTECTION_STATE = 0x8000202C;
inline constexpr DWORD FRZ_IOCTL_SET_NOTIFY_HANDLES = 0x80002034;
inline constexpr DWORD FRZ_IOCTL_QUERY_BOOT_SYSTEM = 0x80002038;
inline constexpr DWORD FRZ_IOCTL_QUERY_PASS_THROUGH = 0x8000203C;
inline constexpr DWORD FRZ_IOCTL_QUERY_OLD_DRIVER_QUALITY = 0x80002040;
inline constexpr DWORD FRZ_IOCTL_QUERY_DISK_FULL = 0x80002044;
inline constexpr DWORD FRZ_IOCTL_QUERY_PROCESS_IMAGE = 0x80002048;
inline constexpr DWORD FRZ_IOCTL_SET_PROCESS_IMAGE = 0x8000204C;
inline constexpr DWORD FRZ_IOCTL_QUERY_DRIVER_IMAGE = 0x80002050;
inline constexpr DWORD FRZ_IOCTL_SET_DRIVER_IMAGE = 0x80002054;
inline constexpr DWORD FRZ_IOCTL_QUERY_BSOD_INFO = 0x80002058;
inline constexpr DWORD FRZ_IOCTL_QUERY_TID_REDIRECT = 0x8000205C;
inline constexpr DWORD FRZ_IOCTL_QUERY_REDIRECT_DATA = 0x80002060;
inline constexpr DWORD FRZ_IOCTL_TRIGGER_BSOD = 0x80002190;
inline constexpr DWORD FRZ_IOCTL_FLUSH_WPP_LOGS = 0x80002194;

// ---- Volume mask helpers ----
inline constexpr uint32_t FRZ_VOLUME_MASK_INVALID = 0xFFFFFFFFu;
inline constexpr int FRZ_MAX_DRIVE_LETTERS = 26;

// ===========================================================================
// 0. Protect Volume Configuration (1024 bytes, pack(1))
// ===========================================================================
#pragma pack(push, 1)
// VolInfo configuration header - the payload that follows the 16-byte MD5
// digest inside the 1024-byte configuration blob. All offsets in the comments
// are relative to the whole blob (the MD5 occupies 0x00..0x0F).
struct ProtectInfo {
    uint32_t readytoProtectVolume;      // 0x10: target volume mask to protect (bitmap, C: = 4)
    uint32_t alreadyProtectVolume;      // 0x14: volume mask currently frozen/protected
    uint8_t  diskNum;                   // 0x18: physical disk index (\PhysicalDriveN)
    int32_t  stopProtect;               // 0x19: emergency stop switch (1 = pass all I/O through)
    int32_t  needUpdate;                // 0x1D: update/pass-through mode (1 = dump/update mode)
    int64_t  storageFileSize;           // 0x21: redirect cache file size (default 1 GB)
    int32_t  bRunSlowly;                // 0x29: degraded mode flag (1 = cache shortage/high latency)
    uint32_t bsodNum;                   // 0x2D: BSOD/abnormal reboots while frozen
    uint32_t bsodMaxUptime;             // 0x31: longest stable uptime while frozen
    int32_t  blueHistoryReport;         // 0x35: BSOD history report switch (1 = on)
    uint32_t lastFreezeState;           // 0x39: freeze engine state before the last shutdown
    uint32_t lastbsodRuntime;           // 0x3D: uptime at the moment of the last BSOD
    uint64_t lastsendbsodtime;          // 0x41: last BSOD telemetry time (FILETIME)
    int32_t  coreDumpZipReport;         // 0x49: kernel minidump report switch
    int32_t  isLastPagefileInFreezeVol; // 0x4D: pagefile lives on a frozen volume
    int32_t  isLastVolumeCorrupt;       // 0x51: volume corruption flag (NTFS dirty on last boot)
    uint8_t  iotDeviceID[FRZ_IOT_DEVICE_ID_LEN]; // 0x55: IoT device code (NUL terminated)
    uint8_t  iotSchoolID[FRZ_IOT_SCHOOL_ID_LEN]; // 0x68: IoT school code (NUL terminated)
    int32_t  bNeedFreeze;               // 0x6D: request freeze on the next I/O or reboot
    int32_t  bNeedUnFreeze;             // 0x71: request unfreeze (drop cache, stop protection)
    uint16_t updateRebootCount;         // 0x75: consecutive reboots during maintenance
    char     startupTime[FRZ_STARTUP_TIME_LEN]; // 0x77: protection cycle start time string
    uint8_t  configVersion;             // 0x8B: configuration version (usually 2)
    uint32_t volMaskCopy;               // 0x8C: volume mask backup (used while upgrading)
    uint32_t updatingTimeSet;           // 0x90: timed pass-through mode enabled (1 = on)
    uint32_t updatingTimeNotAfter;      // 0x94: pass-through deadline (Unix timestamp)
};
// The MD5 digest (0x00..0x0F) is stored by HConfigFile in front of this
// structure, so the struct itself covers the remaining 0x10..0x97 bytes.
static_assert(sizeof(ProtectInfo) == FRZ_CONFIG_INFO_END - FRZ_CONFIG_MD5_SIZE,
    "ProtectInfo size mismatch");
#pragma pack(pop)

// ===========================================================================
// 1. HConfigFile - typed view over the 1024-byte configuration blob.
//    Layout matches the on-disk / driver buffer exactly:
//      [ md5 (16) ][ ProtectInfo ][ placeholder ... ] == 1024 bytes
//    This is a pure data holder: it can only be created from an existing
//    file/driver buffer (never from a ProtectInfo), because the structure may
//    be incomplete.
// ===========================================================================
#pragma pack(push, 1)
struct HConfigFile {
    unsigned char md5[FRZ_CONFIG_MD5_SIZE];
    ProtectInfo   info;
    unsigned char placeholder[FRZ_CONFIG_SIZE - FRZ_CONFIG_MD5_SIZE - sizeof(ProtectInfo)];

    // Copy the whole 1024-byte blob into `data`.
    void toBuffer(unsigned char* data) const noexcept;

    // Deprecated user-facing constructor helper. Prefer the backend APIs.
    [[deprecated("Build HConfigFile through the backend APIs instead")]]
    static HConfigFile fromBuffer(const unsigned char* data) noexcept;

private:
    friend class HFreezeDriverEx;
    friend class HFreezeFileBackend;
    friend class HFreezeDriverBackend;
    friend class HFreezeConfig;
    friend class HFreezeDriver;

    HConfigFile() = default;
    explicit HConfigFile(const unsigned char* data) noexcept;
};
static_assert(sizeof(HConfigFile) == FRZ_CONFIG_SIZE, "HConfigFile size mismatch");
#pragma pack(pop)

// ===========================================================================
// 2. Driver runtime status (IOCTL_QUERY_RUNTIME)
// ===========================================================================
struct DriverRuntimeStatus {
    bool         querySuccess = false;
    uint32_t     activeFlag = 0;
    uint64_t     ptr1 = 0;
    std::wstring logStr;
};

// ===========================================================================
// 3. Driver boot configuration (IOCTL_READ_MEM_CONF)
// ===========================================================================
struct DriverBootConfig {
    bool          querySuccess = false;
    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    size_t        validLen = 0;
};

// ===========================================================================
// 4. Driver event / statistics buffers
//
//    Every structure below maps to exactly one IOCTL buffer. The driver
//    rejects a request whose buffer is smaller than the required size with
//    STATUS_UNSUCCESSFUL, therefore each structure is wrapped in a union with
//    a `reserve` array that forces the size the driver expects.
// ===========================================================================

// ---- 4.1 Driver base version (IOCTL 0x80002004, Out) ----
// The driver answers with a hard-coded byte array (bytes[0] = 2, bytes[1] = 1).
struct FreezeBaseVersion {
    unsigned char bytes[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.2 Boot system & interception statistics (IOCTL 0x80002038, Out) ----
struct FreezeEventBootSystemData {
    uint32_t freezeStartupTime;                        // driver start-up timestamp
    int64_t  freezeDriverState;                        // current state machine flag
    char     strDriverInitState[FRZ_LOG_STRING_LEN];   // driver init log string
    int32_t  readyProtectVolume;                       // "volume ready to protect" flag
    int32_t  bitmapCompState;                          // bitmap comparison state
    int32_t  bootInfoRight;                            // boot information is valid
    int32_t  reinitCallbackTime;                       // re-init callback count / time
    int32_t  prefetchEnable;                           // prefetch interception enabled
    int32_t  originalIrpCount;                         // captured original IRP count
    int32_t  redirectIrpCount;                         // successfully redirected IRP count
    int64_t  redirectAlgoTime;                         // redirect algorithm elapsed time
    int64_t  readBytes;                                // total intercepted read bytes
    int64_t  writeBytes;                               // total intercepted write bytes
    int64_t  logonUIExitTime;                          // logon UI exit timestamp
};
static_assert(sizeof(FreezeEventBootSystemData) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeEventBootSystemData does not fit the driver buffer");

union FreezeEventBootSystem {
    FreezeEventBootSystemData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.3 R3 notification event handles (IOCTL 0x80002034, In) ----
struct FreezeEventNotifyHandlesData {
    void* hEvtDriverLoad;      // driver loaded notification
    void* hEvtProcessCreate;   // process creation notification
    void* hEvtPassThrough;     // disk pass-through read/write notification
    void* hEvtOldDriverQuality;// quality check notification
    void* hEvtDiskFull;        // disk-full notification
};
static_assert(sizeof(FreezeEventNotifyHandlesData) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeEventNotifyHandlesData does not fit the driver buffer");

union FreezeEventNotifyHandles {
    FreezeEventNotifyHandlesData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.4 Low level disk pass-through statistics (IOCTL 0x8000203C, Out) ----
struct FreezeEventPassThroughData {
    LARGE_INTEGER ataWriteDataSumSectors;       // ATA written data sectors
    LARGE_INTEGER ataWritePartTableSumSectors;  // ATA written partition table sectors
    LARGE_INTEGER ataReadDataSumSectors;        // ATA read data sectors
    LARGE_INTEGER ataReadPartTableSumSectors;   // ATA read partition table sectors
    LARGE_INTEGER scsiWriteDataSumSectors;      // SCSI written data sectors
    LARGE_INTEGER scsiWritePartTableSumSectors; // SCSI written partition table sectors
    LARGE_INTEGER scsiReadDataSumSectors;       // SCSI read data sectors
    LARGE_INTEGER scsiReadPartTableSumSectors;  // SCSI read partition table sectors
    int32_t       ideRequestCount;              // total IDE requests
    int32_t       mpioRequestCount;             // total multi-path I/O requests
};
static_assert(sizeof(FreezeEventPassThroughData) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeEventPassThroughData does not fit the driver buffer");

union FreezeEventPassThrough {
    FreezeEventPassThroughData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.5 Old driver quality / failure counters (IOCTL 0x80002040, Out) ----
struct FreezeEventOldDriverQualityData {
    int32_t irpInfoAllocFailed;  // IRP context allocation failures
    int32_t interBufAllocFailed; // internal buffer allocation failures
    int32_t interBufAllocSize;   // requested internal buffer size
    int32_t checkMapTableFailed; // map table lookup failures
    int32_t insertMapTableFailed;// map table insertion failures
    int32_t setBitmapFailed;     // bitmap set failures
    int32_t subIrpFailed;        // sub IRP dispatch failures
    int32_t setRWBitmapFailed;   // read/write bitmap set failures
};
static_assert(sizeof(FreezeEventOldDriverQualityData) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeEventOldDriverQualityData does not fit the driver buffer");

union FreezeEventOldDriverQuality {
    FreezeEventOldDriverQualityData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.6 Free sectors / disk-full statistics (IOCTL 0x80002044, Out) ----
#pragma pack(push, 4)
struct FreezeEventDiskFullData {
    int32_t volumes;                                     // current volume count
    int64_t volFreeSectorCount[FRZ_DRIVE_COUNT];         // free sectors per A..Z letter
};
static_assert(sizeof(FreezeEventDiskFullData) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeEventDiskFullData does not fit the driver buffer");

union FreezeEventDiskFull {
    FreezeEventDiskFullData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};
#pragma pack(pop)

// ---- 4.7 Protected process / driver image (IOCTL 0x80002048/4C/50/54) ----
struct FreezeImageInfoData {
    int32_t       imageId;                              // unique image id
    unsigned char imageFilePath[FRZ_IMAGE_MAX_PATH];     // absolute image path
    uint32_t      majorVersion;                          // image major version
    uint32_t      minorVersion;                          // image minor version
    uint32_t      buildNumber;                           // image build number
    uint32_t      revisionNumber;                        // image revision number
    unsigned char imageCopyRight[FRZ_IMAGE_MAX_PATH];    // signature / copyright text
};
static_assert(sizeof(FreezeImageInfoData) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeImageInfoData does not fit the driver buffer");

union FreezeImageInfo {
    FreezeImageInfoData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.8 IRP redirect queue statistics (IOCTL 0x80002060, Out) ----
#pragma pack(push, 4)
struct FreezeRedirectDataInfo {
    uint32_t timeIndex;             // time slot index
    uint32_t timeBase;              // time base line
    uint32_t maxQueueLen;           // maximum queue length
    uint32_t originalIrpCount;      // captured original IRP count
    uint32_t redirectIrpCount;      // redirected IRP count
    uint64_t readBytes;             // bytes read through redirection
    uint64_t writeBytes;            // bytes written through redirection
    int64_t  maxIrpCompleteTime;    // longest IRP completion time
    int64_t  avgIrpCompleteTime;    // average IRP completion time
};
static_assert(sizeof(FreezeRedirectDataInfo) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeRedirectDataInfo does not fit the driver buffer");
#pragma pack(pop)

union FreezeRedirectData {
    FreezeRedirectDataInfo data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// Linked-list node the driver keeps in kernel memory; only the offsets matter
// for the layout below.
struct RedirectData {
    LIST_ENTRY       listEntry;    // +0x00 doubly linked list node
    FreezeRedirectData redirectData;// +0x08 redirect queue data
};

// ---- 4.9 Frozen thread pool TIDs + redirected queue (IOCTL 0x8000205C, Out) ----
// This IOCTL requires a buffer of exactly FRZ_DRIVER_REDIRECT_TID_SIZE bytes:
// the driver writes the frozen thread pool TID array followed by the redirect
// queue information. The boundary between the two parts is not documented, so
// the blob is kept opaque on the application side.
struct FreezeTidRedirectBuffer {
    unsigned char data[FRZ_DRIVER_REDIRECT_TID_SIZE];
};
static_assert(sizeof(FreezeTidRedirectBuffer) == FRZ_DRIVER_REDIRECT_TID_SIZE,
    "FreezeTidRedirectBuffer size mismatch");

// ---- 4.10 Intercepted BSOD information (IOCTL 0x80002058, Out) ----
struct BsodInfoData {
    unsigned char md5[FRZ_CONFIG_MD5_SIZE]; // crash dump fingerprint
    uint32_t      bsodTime;                 // time the BSOD happened
    uint32_t      startupTimeOccurBsod;     // uptime reached when the BSOD happened
};
static_assert(sizeof(BsodInfoData) <= FRZ_DRIVER_BUFFER_SIZE,
    "BsodInfoData does not fit the driver buffer");

union BsodInfo {
    BsodInfoData data;
    unsigned char reserve[FRZ_DRIVER_BUFFER_SIZE];
};

// ---- 4.11 Core interception initialization results (IOCTL 0x80002028, Out) ----
#pragma pack(push, 1)
struct FreezeKeyResultData {
    // Memory and process callback configuration
    uint8_t  freeze_BugcheckDataMem_AllocaSuccess;  // bugcheck callback memory allocated
    uint32_t freeze_BugcheckDataMem_WriteSize;      // written bugcheck data size
    int32_t  freeze_SetProcessNotify_Status;        // process callback registration result

    // Initialization flow tracking
    uint32_t freeze_InitializerInit_ComIoDevCreate; // communication device creation state
    uint32_t freeze_InitializerInit_AddDeviceCount; // number of attached filter devices
    uint32_t freeze_InitializerInit_Start;          // initialization started

    // Volume configuration / cache file parsing
    uint32_t freeze_InitializerInit_VolConfig_OpenReg;
    uint32_t freeze_InitializerInit_VolConfig_OpenKey;
    uint32_t freeze_InitializerInit_VolConfig_Open;
    uint32_t freeze_InitializerInit_VolConfig_Read;
    uint32_t freeze_InitializerInit_VolConfig_Md5_Wrong;
    uint32_t freeze_InitializerInit_VolConfig_ReadSuccess;
    uint32_t freeze_InitializerInit_VolConfig_ReWrite;
    uint32_t freeze_InitializerInit_VolConfig_Over;

    // System and volume environment checks
    bool     freeze_InitializerInit_bChkdsk;                       // Chkdsk is running
    bool     freeze_InitializerInit_bCheckDump;                    // dump file is being written
    bool     freeze_InitializerInit_bStopProtect;                  // stop-protection flag set
    bool     freeze_InitializerInit_bNeedUpdate;                   // cache/config update needed
    uint32_t freeze_InitializerInit_readyProtectVolume;            // ready protection volume mask
    int32_t  freeze_InitializerInit_StartInitProtectVolResource;   // volume resource init result

    // Disk partition layout parsing (MBR/GPT)
    uint32_t freeze_InitializerInit_VolumeInfo_PartitionStyle_Mbr0_Gpt1; // 0 = MBR, 1 = GPT
    uint32_t freeze_InitializerInit_CalculateEBR;   // EBR calculation state
    uint32_t freeze_InitializerInit_diskEbrNum;     // number of EBRs
    int32_t  freeze_InitializerInit_GetPartitionStyle_Over; // partition parsing finished

    // Resource allocation and hooking
    uint32_t freeze_InitializerInit_ProtectVolResource_gVolumeListisValid;
    uint32_t freeze_InitializerInit_ProtectVolResource_GetVolumeInfo_Start;
    uint32_t freeze_InitializerInit_ProtectVolResource_BitMap_Start;
    uint32_t freeze_InitializerInit_passthroughConfigFile;
    uint32_t freeze_InitializerInit_ProtectVolResource_FailVolume;
    uint32_t freeze_InitializerInit_StartInitProtectVolResource_Over;
    uint32_t freeze_InitializerInit_HookDiskMajorFun_GetObject; // disk object hook result
    uint32_t freeze_InitializerInit_SetLoadImage;               // image callback registration
    uint32_t freeze_InitializerInit_DriveImageLoadCallBack;     // image callback state
    uint32_t freeze_InitializerInit_Over;                       // initialization fully complete

    // Initialization log data
    bool     freeze_InitializerInit_errorRecorded;   // an error was already recorded
    bool     freeze_InitializerInit_bRedirectSuccess;// redirect engine started successfully
    char     freeze_InitializerInit_FileFuncString[FRZ_LOG_STRING_LEN];  // failing file/function
    char     freeze_InitializerInit_LogString[FRZ_LOG_STRING_LEN];       // init log string
    char     freeze_InitializerInit_ErrorFileString[FRZ_LOG_STRING_LEN]; // error log string

    // Runtime behaviour monitoring
    uint8_t  freeze_AddDevice_getDiskNumber[FRZ_DRIVE_COUNT]; // cached disk number per letter
    int32_t  freeze_AddDevice_getDiskNumberStatus;            // disk number query state
    uint32_t freeze_MajorFunction_firstRead;                  // first read trigger state
    uint32_t freeze_MajorFunction_firstWrite;                 // first write trigger state

    // IRP read/write handler dispatcher state
    uint32_t freeze_ReadWriteHandler_Start;
    uint32_t freeze_ReadWriteHandler_Noprotect_Update;
    uint32_t freeze_ReadWriteHandler_UniqueThread;
    uint32_t freeze_ReadWriteHandler_FilterIrpInBootRecord; // boot record interception
    uint32_t freeze_ReadWriteHandler_Vol_Valid;
    uint32_t freeze_ReadWriteHandler_Irp_InProtectVol;      // IRP hit a protected volume
    uint32_t freeze_ReadWriteHandler_KeSetEvent;            // event signalled
    uint32_t freeze_ReadWriteThread_Start;
    uint32_t freeze_ReadWriteThread_ConsumeIprQueue;        // queue consumption state
    uint32_t freeze_ReadWriteThread_DiskIrpHandler;
    uint32_t freeze_ReadWriteThread_FastFsdRequest;         // FastIo handling state
};
static_assert(sizeof(FreezeKeyResultData) <= FRZ_DRIVER_KEY_BUFFER_SIZE,
    "FreezeKeyResultData does not fit the driver buffer");
#pragma pack(pop)

union FreezeKeyResult {
    FreezeKeyResultData data;
    unsigned char reserve[FRZ_DRIVER_KEY_BUFFER_SIZE];
};

// ---- 4.12 Current interception / stop-protect state (IOCTL 0x8000202C, Out) ----
#pragma pack(push, 1)
struct FreezeProtectionState {
    uint64_t freezeDriverState; // driver run state
    uint32_t stopProtect;       // stop-protection flag
    uint64_t reserve;           // reserved
};
#pragma pack(pop)
static_assert(sizeof(FreezeProtectionState) <= FRZ_DRIVER_BUFFER_SIZE,
    "FreezeProtectionState does not fit the driver buffer");

#pragma pack(pop)

// ===========================================================================
// Inline implementations
// ===========================================================================
inline void HConfigFile::toBuffer(unsigned char* data) const noexcept {
    if (data) std::memcpy(data, this, FRZ_CONFIG_SIZE);
}

inline HConfigFile::HConfigFile(const unsigned char* data) noexcept {
    std::memcpy(this, data, FRZ_CONFIG_SIZE);
}

inline HConfigFile HConfigFile::fromBuffer(const unsigned char* data) noexcept {
    return HConfigFile(data);
}

// Format a byte buffer as a hex dump (shared by every freeze layer for debug
// logging).
inline std::wstring FrzHexDump(const unsigned char* data, size_t len) noexcept {
    if (!data) return L"";

    std::wstring content;
    content += L"    Offset | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n";
    content += L"    -------+------------------------------------------------\n";

    for (size_t i = 0; i < len; i += 16) {
        content += std::format(L"    0x{:04X} | ", i);
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < len) {
                content += std::format(L"{:02X} ", data[i + j]);
            }
            else {
                content += L"   ";
            }
        }
        content += L"\n";
    }
    return content;
}
