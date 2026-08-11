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
 * HugoUtilsC - C language interface implementation.
 * Wraps the C++ HugoUtils library for C consumers.
 */
#include "HugoUtils/HugoUtilsDef.h"
#include "HugoUtils/HugoUtilsC.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <filesystem>
#include <iterator>
#pragma comment(lib, "Version.lib")


/* ---- Helper: write wide string to caller buffer ---- */
static int WriteWStr(wchar_t* buf, int bufSize, const std::wstring& src) {
    int required = static_cast<int>(src.size());
    if (!buf || bufSize <= 0) return required;
    if (bufSize <= required) return required; /* not enough space */
    wcsncpy_s(buf, bufSize, src.c_str(), _TRUNCATE);
    buf[bufSize - 1] = L'\0';
    return required;
}

static int WriteWStr(wchar_t* buf, int bufSize, const std::wstring_view& src) {
    return WriteWStr(buf, bufSize, std::wstring(src));
}

static int WriteWStr(wchar_t* buf, int bufSize, const wchar_t* src) {
    if (!src) return 0;
    return WriteWStr(buf, bufSize, std::wstring(src));
}

static int WriteWStr(wchar_t* buf, int bufSize, const std::filesystem::path& src) {
    return WriteWStr(buf, bufSize, src.wstring());
}

/* ---- Helper: write narrow string to caller buffer ---- */
static int WriteStr(char* buf, int bufSize, const std::string& src) {
    int required = static_cast<int>(src.size());
    if (!buf || bufSize <= 0) return required;
    if (bufSize <= required) return required;
    strncpy_s(buf, bufSize, src.c_str(), _TRUNCATE);
    buf[bufSize - 1] = '\0';
    return required;
}

/* ======================================================================== */
/* 1. HugoUtils - Drive utilities                                           */
/* ======================================================================== */

#include "HugoUtils/HugoUtils.h"

int Hugo_GetDrivesInUse(char* buf, int bufSize) {
    std::string result = HugoUtils::GetDrivesInUse();
    return WriteStr(buf, bufSize, result);
}

int Hugo_GetLogicalDrives(char* buf, int bufSize) {
    std::string result = HugoUtils::GetLogicalDrives();
    return WriteStr(buf, bufSize, result);
}

/* ======================================================================== */
/* 2. HArt - ASCII art text                                                 */
/* ======================================================================== */

#ifndef HU_DISABLE_ART
#include "HugoUtils/HArt.h"

void Hugo_PrintArtText(int idx) {
    HArt::PrintArtText(idx);
}

int Hugo_GetArtTextLineCount(int idx) {
    auto lines = HArt::GetHugoArtText(idx);
    return static_cast<int>(lines.size());
}

int Hugo_GetArtTextLine(int idx, int line, wchar_t* buf, int bufSize) {
    auto lines = HArt::GetHugoArtText(idx);
    if (line < 0 || line >= static_cast<int>(lines.size())) return 0;
    return WriteWStr(buf, bufSize, lines[line]);
}
#else
void Hugo_PrintArtText(int) {}
int Hugo_GetArtTextLineCount(int) { return 0; }
int Hugo_GetArtTextLine(int, int, wchar_t*, int) { return 0; }
#endif

/* ======================================================================== */
/* 3. GPL3 - License display                                               */
/* ======================================================================== */

#ifndef HU_DISABLE_GPL3
#include "HugoUtils/GPL3.h"

void Hugo_ShowWarranty(void) {
    ShowWarranty();
}

void Hugo_ShowLicense(const wchar_t* licensePath) {
    ShowLicense(licensePath);
}
#else
void Hugo_ShowWarranty(void) {}
void Hugo_ShowLicense(const wchar_t*) {}
#endif

/* ======================================================================== */
/* 4. HInfo - Seewo info queries                                           */
/* ======================================================================== */

#ifndef HU_DISABLE_INFO
#include "HugoUtils/HInfo.h"

int Hugo_GetHugoVersion(wchar_t* buf, int bufSize) {
    auto val = HInfo::getHugoVersion();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetHugoFolder(wchar_t* buf, int bufSize) {
    auto val = HInfo::getHugoFolder();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetHugoProtectDriverFolder(wchar_t* buf, int bufSize) {
    auto val = HInfo::getHugoProtectDriverFolder();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetHugoProtectDriverPath(wchar_t* buf, int bufSize) {
    auto val = HInfo::getHugoProtectDriverPath();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetMachineId(char* buf, int bufSize) {
    auto val = HInfo::GetMachineId();
    if (!val) return 0;
    return WriteStr(buf, bufSize, *val);
}

int Hugo_GetSeewoCoreIniPath(wchar_t* buf, int bufSize) {
    auto val = HInfo::GetSeewoCoreIniPath();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetLockConfigIniPath(wchar_t* buf, int bufSize) {
    auto val = HInfo::GetLockConfigIniPath();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetLockConfigIniPath2(wchar_t* buf, int bufSize) {
    auto val = HInfo::GetLockConfigIniPath2();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetSeewoSchoolFilePath(wchar_t* buf, int bufSize) {
    auto val = HInfo::GetSeewoSchoolFilePath();
    if (!val) return 0;
    return WriteWStr(buf, bufSize, *val);
}

int Hugo_GetHugoUpdateFolderCount(void) {
    auto folders = HInfo::getHugoUpdateFolder();
    return static_cast<int>(folders.size());
}

int Hugo_GetHugoUpdateFolder(int index, wchar_t* buf, int bufSize) {
    auto folders = HInfo::getHugoUpdateFolder();
    if (index < 0 || index >= static_cast<int>(folders.size())) return 0;
    return WriteWStr(buf, bufSize, folders[index]);
}
#else
int Hugo_GetHugoVersion(wchar_t*, int) { return 0; }
int Hugo_GetHugoFolder(wchar_t*, int) { return 0; }
int Hugo_GetHugoProtectDriverFolder(wchar_t*, int) { return 0; }
int Hugo_GetHugoProtectDriverPath(wchar_t*, int) { return 0; }
int Hugo_GetMachineId(char*, int) { return 0; }
int Hugo_GetSeewoCoreIniPath(wchar_t*, int) { return 0; }
int Hugo_GetLockConfigIniPath(wchar_t*, int) { return 0; }
int Hugo_GetLockConfigIniPath2(wchar_t*, int) { return 0; }
int Hugo_GetSeewoSchoolFilePath(wchar_t*, int) { return 0; }
int Hugo_GetHugoUpdateFolderCount(void) { return 0; }
int Hugo_GetHugoUpdateFolder(int, wchar_t*, int) { return 0; }
#endif

/* ======================================================================== */
/* 5. HLock - SharedFlag                                                   */
/* ======================================================================== */

#include "HugoUtils/HLock.h"

struct HugoSharedFlag {
    SharedFlag* impl;
};

HugoSharedFlag* Hugo_SharedFlag_Create(const wchar_t* name) {
    if (!name) return nullptr;
    auto* wrapper = new HugoSharedFlag;
    wrapper->impl = new SharedFlag(name);
    if (!wrapper->impl->Valid()) {
        delete wrapper->impl;
        delete wrapper;
        return nullptr;
    }
    return wrapper;
}

void Hugo_SharedFlag_Destroy(HugoSharedFlag* flag) {
    if (!flag) return;
    delete flag->impl;
    delete flag;
}

void Hugo_SharedFlag_Set(HugoSharedFlag* flag, int val) {
    if (!flag || !flag->impl) return;
    flag->impl->Set(val ? TRUE : FALSE);
}

int Hugo_SharedFlag_Get(HugoSharedFlag* flag) {
    if (!flag || !flag->impl) return 0;
    return flag->impl->Get() ? 1 : 0;
}

int Hugo_SharedFlag_Valid(HugoSharedFlag* flag) {
    if (!flag || !flag->impl) return 0;
    return flag->impl->Valid() ? 1 : 0;
}

/* ======================================================================== */
/* 6. HMount - Virtual disk mount management                               */
/* ======================================================================== */

#ifndef HU_DISABLE_MOUNT
#include "HugoUtils/HMount.h"

struct HugoMount {
    HMount impl;
};

HugoMount* Hugo_Mount_Create(void) {
    return new HugoMount;
}

void Hugo_Mount_Destroy(HugoMount* m) {
    delete m;
}

void Hugo_Mount_PrintAllInfo(HugoMount* m) {
    if (!m) return;
    m->impl.PrintAllInfo();
}

int Hugo_Mount_Mount(HugoMount* m, int diskId, int partId, char driveLetter) {
    if (!m) return -1;
    return m->impl.Mount(diskId, partId, driveLetter);
}

int Hugo_Mount_UnmountById(HugoMount* m, int diskId, int partId) {
    if (!m) return -1;
    return m->impl.Unmount(diskId, partId);
}

int Hugo_Mount_UnmountByLetter(HugoMount* m, char driveLetter) {
    if (!m) return -1;
    return m->impl.Unmount(driveLetter);
}

int Hugo_Mount_FindMountedDrive(HugoMount* m, int diskId, int partId, char* buf, int bufSize) {
    if (!m) return 0;
    auto letters = m->impl.FindMountedDrive(diskId, partId);
    int count = static_cast<int>(letters.size());
    if (!buf || bufSize < count) return count;
    for (int i = 0; i < count; ++i) {
        buf[i] = letters[i];
    }
    return count;
}
#else
struct HugoMount { int dummy; };
HugoMount* Hugo_Mount_Create(void) { return nullptr; }
void Hugo_Mount_Destroy(HugoMount*) {}
void Hugo_Mount_PrintAllInfo(HugoMount*) {}
int Hugo_Mount_Mount(HugoMount*, int, int, char) { return -1; }
int Hugo_Mount_UnmountById(HugoMount*, int, int) { return -1; }
int Hugo_Mount_UnmountByLetter(HugoMount*, char) { return -1; }
int Hugo_Mount_FindMountedDrive(HugoMount*, int, int, char*, int) { return 0; }
#endif

/* ======================================================================== */
/* 7. HFreezeInterface - Volume mask helper                                */
/* ======================================================================== */

#ifndef HU_DISABLE_FREEZE
#include "HugoUtils/HFreezeInterface.h"

uint32_t Hugo_CalculateVolumeMask(const wchar_t* driveLetters) {
    if (!driveLetters) return 0;
    return CalculateVolumeMask(std::wstring(driveLetters));
}
#else
uint32_t Hugo_CalculateVolumeMask(const wchar_t*) { return 0; }
#endif

/* ======================================================================== */
/* 8. HFreezeApi - Seewo Freeze HTTP API                                   */
/* ======================================================================== */

#ifndef HU_DISABLE_FREEZE_API
#include "HugoUtils/HFreezeApi.h"

struct HugoFreezeApi {
    HFreezeApi impl;
};

HugoFreezeApi* Hugo_FreezeApi_Create(void) {
    return new HugoFreezeApi;
}

void Hugo_FreezeApi_Destroy(HugoFreezeApi* h) {
    delete h;
}

HugoResult Hugo_FreezeApi_Init(HugoFreezeApi* h) {
    if (!h) return HUGO_INVALID_PARAM;
    auto res = h->impl.Init();
    return static_cast<HugoResult>(static_cast<int>(res.result));
}

void Hugo_FreezeApi_Cleanup(HugoFreezeApi* h) {
    if (!h) return;
    h->impl.Cleanup();
}

int Hugo_FreezeApi_IsInitialized(HugoFreezeApi* h) {
    if (!h) return 0;
    return h->impl.IsInitialized() ? 1 : 0;
}

void Hugo_FreezeApi_SetConfig(HugoFreezeApi* h, const wchar_t* ip, uint16_t port) {
    if (!h) return;
    h->impl.SetConfig(ip ? std::wstring(ip) : L"", port);
}

int Hugo_FreezeApi_GetConfig(HugoFreezeApi* h, wchar_t* ipBuf, int ipBufSize, uint16_t* outPort) {
    if (!h) return 0;
    std::wstring ip;
    uint16_t port = 0;
    h->impl.GetConfig(ip, port);
    if (outPort) *outPort = port;
    return WriteWStr(ipBuf, ipBufSize, ip);
}

HugoResult Hugo_FreezeApi_GetFreezeState(HugoFreezeApi* h, wchar_t* msgBuf, int msgBufSize) {
    if (!h) return HUGO_INVALID_PARAM;
    auto res = h->impl.GetFreezeState();
    if (msgBuf) WriteWStr(msgBuf, msgBufSize, res.msg);
    return static_cast<HugoResult>(static_cast<int>(res.result));
}

HugoResult Hugo_FreezeApi_TryProtect(HugoFreezeApi* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize) {
    if (!h || !driveLetters) return HUGO_INVALID_PARAM;
    auto res = h->impl.TryProtect(std::wstring(driveLetters));
    if (msgBuf) WriteWStr(msgBuf, msgBufSize, res.msg);
    return static_cast<HugoResult>(static_cast<int>(res.result));
}

HugoResult Hugo_FreezeApi_SetFreezeState(HugoFreezeApi* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize) {
    if (!h || !driveLetters) return HUGO_INVALID_PARAM;
    auto res = h->impl.SetFreezeState(std::wstring(driveLetters));
    if (msgBuf) WriteWStr(msgBuf, msgBufSize, res.msg);
    return static_cast<HugoResult>(static_cast<int>(res.result));
}
#else
struct HugoFreezeApi { int dummy; };
HugoFreezeApi* Hugo_FreezeApi_Create(void) { return nullptr; }
void Hugo_FreezeApi_Destroy(HugoFreezeApi*) {}
HugoResult Hugo_FreezeApi_Init(HugoFreezeApi*) { return HUGO_NOT_SUPPORTED; }
void Hugo_FreezeApi_Cleanup(HugoFreezeApi*) {}
int Hugo_FreezeApi_IsInitialized(HugoFreezeApi*) { return 0; }
void Hugo_FreezeApi_SetConfig(HugoFreezeApi*, const wchar_t*, uint16_t) {}
int Hugo_FreezeApi_GetConfig(HugoFreezeApi*, wchar_t*, int, uint16_t*) { return 0; }
HugoResult Hugo_FreezeApi_GetFreezeState(HugoFreezeApi*, wchar_t*, int) { return HUGO_NOT_SUPPORTED; }
HugoResult Hugo_FreezeApi_TryProtect(HugoFreezeApi*, const wchar_t*, wchar_t*, int) { return HUGO_NOT_SUPPORTED; }
HugoResult Hugo_FreezeApi_SetFreezeState(HugoFreezeApi*, const wchar_t*, wchar_t*, int) { return HUGO_NOT_SUPPORTED; }
#endif

/* ======================================================================== */
/* 9. HFreezeDriver - Freeze driver management                             */
/* ======================================================================== */

#ifndef HU_DISABLE_FREEZE_DRIVER
#include "HugoUtils/HFreezeDriver.h"

struct HugoFreezeDriver {
    HFreezeDriver impl;
    FreezeResult lastState;
};

HugoFreezeDriver* Hugo_FreezeDriver_Create(void) {
    return new HugoFreezeDriver;
}

void Hugo_FreezeDriver_Destroy(HugoFreezeDriver* h) {
    delete h;
}

HugoResult Hugo_FreezeDriver_Init(HugoFreezeDriver* h) {
    if (!h) return HUGO_INVALID_PARAM;
    auto res = h->impl.Init();
    return static_cast<HugoResult>(static_cast<int>(res.result));
}

void Hugo_FreezeDriver_Cleanup(HugoFreezeDriver* h) {
    if (!h) return;
    h->impl.Cleanup();
}

int Hugo_FreezeDriver_IsInitialized(HugoFreezeDriver* h) {
    if (!h) return 0;
    return h->impl.IsInitialized() ? 1 : 0;
}

HugoResult Hugo_FreezeDriver_GetFreezeState(HugoFreezeDriver* h, int* diskCount, wchar_t* msgBuf, int msgBufSize) {
    if (!h) return HUGO_INVALID_PARAM;
    h->lastState = h->impl.GetFreezeState();
    if (diskCount) *diskCount = static_cast<int>(h->lastState.diskInfos.size());
    if (msgBuf) WriteWStr(msgBuf, msgBufSize, h->lastState.msg);
    return static_cast<HugoResult>(static_cast<int>(h->lastState.result));
}

int Hugo_FreezeDriver_GetDiskEntry(HugoFreezeDriver* h, int index, wchar_t* outLetter, HugoDiskInfo* outInfo) {
    if (!h) return 0;
    if (index < 0 || index >= static_cast<int>(h->lastState.diskInfos.size())) return 0;
    auto it = h->lastState.diskInfos.begin();
    std::advance(it, index);
    if (outLetter) *outLetter = it->first;
    if (outInfo) {
        outInfo->state = static_cast<HugoDriveFreezeState>(static_cast<int>(it->second.state));
        outInfo->bytesFree = it->second.bytesFree;
        outInfo->bytesTotal = it->second.bytesTotal;
    }
    return 1;
}

HugoResult Hugo_FreezeDriver_TryProtect(HugoFreezeDriver* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize) {
    if (!h || !driveLetters) return HUGO_INVALID_PARAM;
    auto res = h->impl.TryProtect(std::wstring(driveLetters));
    if (msgBuf) WriteWStr(msgBuf, msgBufSize, res.msg);
    return static_cast<HugoResult>(static_cast<int>(res.result));
}

HugoResult Hugo_FreezeDriver_SetFreezeState(HugoFreezeDriver* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize) {
    if (!h || !driveLetters) return HUGO_INVALID_PARAM;
    auto res = h->impl.SetFreezeState(std::wstring(driveLetters));
    if (msgBuf) WriteWStr(msgBuf, msgBufSize, res.msg);
    return static_cast<HugoResult>(static_cast<int>(res.result));
}
#else
struct HugoFreezeDriver { int dummy; };
HugoFreezeDriver* Hugo_FreezeDriver_Create(void) { return nullptr; }
void Hugo_FreezeDriver_Destroy(HugoFreezeDriver*) {}
HugoResult Hugo_FreezeDriver_Init(HugoFreezeDriver*) { return HUGO_NOT_SUPPORTED; }
void Hugo_FreezeDriver_Cleanup(HugoFreezeDriver*) {}
int Hugo_FreezeDriver_IsInitialized(HugoFreezeDriver*) { return 0; }
HugoResult Hugo_FreezeDriver_GetFreezeState(HugoFreezeDriver*, int*, wchar_t*, int) { return HUGO_NOT_SUPPORTED; }
int Hugo_FreezeDriver_GetDiskEntry(HugoFreezeDriver*, int, wchar_t*, HugoDiskInfo*) { return 0; }
HugoResult Hugo_FreezeDriver_TryProtect(HugoFreezeDriver*, const wchar_t*, wchar_t*, int) { return HUGO_NOT_SUPPORTED; }
HugoResult Hugo_FreezeDriver_SetFreezeState(HugoFreezeDriver*, const wchar_t*, wchar_t*, int) { return HUGO_NOT_SUPPORTED; }
#endif

/* ======================================================================== */
/* 10. HFreezeFile_p - Volume info config file I/O                         */
/* ======================================================================== */

#ifndef HU_DISABLE_FREEZE
#include "HugoUtils/HFreezeFile_p.h"

void Hugo_FreezeFile_SetConfigPath(const wchar_t* path) {
    if (!path) return;
    HFreezeFilePrivate::SetConfigPath(std::wstring(path));
}

int Hugo_FreezeFile_GetConfigPath(wchar_t* buf, int bufSize) {
    return WriteWStr(buf, bufSize, HFreezeFilePrivate::GetConfigPath());
}

int Hugo_FreezeFile_ReadConfig(uint8_t* outConfig, int configSize) {
    if (!outConfig || configSize < static_cast<int>(sizeof(ProtectInfo))) return 0;
    auto cfg = HFreezeFilePrivate::ReadConfig();
    if (!cfg) return 0;
    std::memcpy(outConfig, &(*cfg), sizeof(ProtectInfo));
    return 1;
}

int Hugo_FreezeFile_ReadConfigFrom(const wchar_t* path, uint8_t* outConfig, int configSize) {
    if (!path || !outConfig || configSize < static_cast<int>(sizeof(ProtectInfo))) return 0;
    auto cfg = HFreezeFilePrivate::ReadConfig(std::wstring(path));
    if (!cfg) return 0;
    std::memcpy(outConfig, &(*cfg), sizeof(ProtectInfo));
    return 1;
}

int Hugo_FreezeFile_WriteConfig(const uint8_t* config, int configSize) {
    if (!config || configSize < static_cast<int>(sizeof(ProtectInfo))) return 0;
    ProtectInfo cfg;
    std::memcpy(&cfg, config, sizeof(ProtectInfo));
    return HFreezeFilePrivate::WriteConfig(cfg) ? 1 : 0;
}

int Hugo_FreezeFile_WriteConfigTo(const uint8_t* config, int configSize, const wchar_t* path) {
    if (!config || !path || configSize < static_cast<int>(sizeof(ProtectInfo))) return 0;
    ProtectInfo cfg;
    std::memcpy(&cfg, config, sizeof(ProtectInfo));
    return HFreezeFilePrivate::WriteConfig(cfg, std::wstring(path)) ? 1 : 0;
}

int Hugo_FreezeFile_BuildFreezeConfig(const uint8_t* original, uint32_t targetVolMask, int enableFreeze, uint8_t* out) {
    if (!original || !out) return 0;
    ProtectInfo orig;
    std::memcpy(&orig, original, sizeof(ProtectInfo));
    ProtectInfo result = HFreezeFilePrivate::BuildFreezeConfig(orig, targetVolMask, enableFreeze != 0);
    std::memcpy(out, &result, sizeof(ProtectInfo));
    return 1;
}
#else
void Hugo_FreezeFile_SetConfigPath(const wchar_t*) {}
int Hugo_FreezeFile_GetConfigPath(wchar_t*, int) { return 0; }
int Hugo_FreezeFile_ReadConfig(uint8_t*, int) { return 0; }
int Hugo_FreezeFile_ReadConfigFrom(const wchar_t*, uint8_t*, int) { return 0; }
int Hugo_FreezeFile_WriteConfig(const uint8_t*, int) { return 0; }
int Hugo_FreezeFile_WriteConfigTo(const uint8_t*, int, const wchar_t*) { return 0; }
int Hugo_FreezeFile_BuildFreezeConfig(const uint8_t*, uint32_t, int, uint8_t*) { return 0; }
#endif

/* ======================================================================== */
/* 11. HFreezeDriver_p - Direct driver IOCTL communication                 */
/* ======================================================================== */

#ifndef HU_DISABLE_FREEZE_DRIVER
#include "HugoUtils/HFreezeDriver_p.h"

struct HugoFreezeDriverPrivate {
    HFreezeDriverPrivate impl;
};

HugoFreezeDriverPrivate* Hugo_FreezeDrvPriv_Create(void) {
    return new HugoFreezeDriverPrivate;
}

void Hugo_FreezeDrvPriv_Destroy(HugoFreezeDriverPrivate* h) {
    delete h;
}

int Hugo_FreezeDrvPriv_Init(HugoFreezeDriverPrivate* h) {
    if (!h) return 0;
    return h->impl.Init() ? 1 : 0;
}

void Hugo_FreezeDrvPriv_Cleanup(HugoFreezeDriverPrivate* h) {
    if (!h) return;
    h->impl.Cleanup();
}

int Hugo_FreezeDrvPriv_IsInitialized(HugoFreezeDriverPrivate* h) {
    if (!h) return 0;
    return h->impl.IsInitialized() ? 1 : 0;
}

int Hugo_FreezeDrvPriv_QueryBootConfig(HugoFreezeDriverPrivate* h, uint8_t* outConfig, int configSize) {
    if (!h || !outConfig || configSize < static_cast<int>(sizeof(ProtectInfo))) return 0;
    auto cfg = h->impl.QueryBootConfig();
    if (!cfg) return 0;
    std::memcpy(outConfig, &(*cfg), sizeof(ProtectInfo));
    return 1;
}

int Hugo_FreezeDrvPriv_WriteBootConfig(HugoFreezeDriverPrivate* h, const uint8_t* config, int configSize) {
    if (!h || !config || configSize < static_cast<int>(sizeof(ProtectInfo))) return 0;
    ProtectInfo cfg;
    std::memcpy(&cfg, config, sizeof(ProtectInfo));
    return h->impl.WriteBootConfig(cfg) ? 1 : 0;
}

int Hugo_FreezeDrvPriv_QueryBootSystem(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryBootSystem();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryKeyResult(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryKeyResult();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryProtectionState(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryProtectionState();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryPassThrough(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryPassThrough();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryOldDriverQuality(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryOldDriverQuality();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryDiskFull(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryDiskFull();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryBsodInfo(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryBsodInfo();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

int Hugo_FreezeDrvPriv_QueryRedirectData(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize) {
    if (!h || !outBuf) return 0;
    auto val = h->impl.QueryRedirectData();
    if (!val) return 0;
    if (bufSize < static_cast<int>(sizeof(*val))) return 0;
    std::memcpy(outBuf, &(*val), sizeof(*val));
    return 1;
}

void Hugo_FreezeDrvPriv_TriggerBSOD(HugoFreezeDriverPrivate* h) {
    if (!h) return;
    h->impl.TriggerBSOD();
}

void Hugo_FreezeDrvPriv_FlushWppLogs(HugoFreezeDriverPrivate* h) {
    if (!h) return;
    h->impl.FlushWppLogs();
}

DWORD Hugo_FreezeDrvPriv_GetLastErrorCode(HugoFreezeDriverPrivate* h) {
    if (!h) return 0;
    return h->impl.GetLastErrorCode();
}

int Hugo_FreezeDrvPriv_GetLastErrorMsg(HugoFreezeDriverPrivate* h, wchar_t* buf, int bufSize) {
    if (!h) return 0;
    return WriteWStr(buf, bufSize, h->impl.GetLastErrorMsg());
}
#else
struct HugoFreezeDriverPrivate { int dummy; };
HugoFreezeDriverPrivate* Hugo_FreezeDrvPriv_Create(void) { return nullptr; }
void Hugo_FreezeDrvPriv_Destroy(HugoFreezeDriverPrivate*) {}
int Hugo_FreezeDrvPriv_Init(HugoFreezeDriverPrivate*) { return 0; }
void Hugo_FreezeDrvPriv_Cleanup(HugoFreezeDriverPrivate*) {}
int Hugo_FreezeDrvPriv_IsInitialized(HugoFreezeDriverPrivate*) { return 0; }
int Hugo_FreezeDrvPriv_QueryBootConfig(HugoFreezeDriverPrivate*, uint8_t*, int) { return 0; }
int Hugo_FreezeDrvPriv_WriteBootConfig(HugoFreezeDriverPrivate*, const uint8_t*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryBootSystem(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryKeyResult(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryProtectionState(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryPassThrough(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryOldDriverQuality(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryDiskFull(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryBsodInfo(HugoFreezeDriverPrivate*, void*, int) { return 0; }
int Hugo_FreezeDrvPriv_QueryRedirectData(HugoFreezeDriverPrivate*, void*, int) { return 0; }
void Hugo_FreezeDrvPriv_TriggerBSOD(HugoFreezeDriverPrivate*) {}
void Hugo_FreezeDrvPriv_FlushWppLogs(HugoFreezeDriverPrivate*) {}
DWORD Hugo_FreezeDrvPriv_GetLastErrorCode(HugoFreezeDriverPrivate*) { return 0; }
int Hugo_FreezeDrvPriv_GetLastErrorMsg(HugoFreezeDriverPrivate*, wchar_t*, int) { return 0; }
#endif

/* ======================================================================== */
/* 12. HPassword - Password cracking                                        */
/* ======================================================================== */

#ifndef HU_DISABLE_PASSWORD
#include "HugoUtils/HPassword.h"
#include "HugoUtils/BruteforceModel.h"

int Hugo_CrackPassword(int mode, int type,
                       const char* ciphertext,
                       const char* deviceId,
                       const char* machineId,
                       char* plainOut, int plainBufSize) {
    if (!ciphertext || !plainOut || plainBufSize <= 0) return 0;

    CrackTask task;
    task.mode = static_cast<CrackMode>(mode);
    task.type = static_cast<PasswordType>(type);
    task.ciphertext = ciphertext;
    task.device_id = deviceId ? deviceId : "";
    task.machine_id = machineId ? machineId : "";

    V1Decryptor v1;
    V2Decryptor v2;
    V3Decryptor v3;
    std::vector<Decryptor*> decryptors = { &v1, &v2, &v3 };

    CrackExecutor executor;
    std::vector<CrackResult> results = executor.execute({ task }, decryptors);
    if (results.empty() || !results[0].success) return 0;

    WriteStr(plainOut, plainBufSize, results[0].plaintext);
    return 1;
}

int Hugo_CrackAllPasswords(HugoCrackResult* outResults, int maxResults) {
    AutoInfoAcquirer acquirer;
    auto tasks = acquirer.acquire();

    V1Decryptor v1;
    V2Decryptor v2;
    V3Decryptor v3;
    std::vector<Decryptor*> decryptors = { &v1, &v2, &v3 };

    CrackExecutor executor;
    auto results = executor.execute(tasks, decryptors);

    int count = static_cast<int>(results.size());
    if (count > maxResults) count = maxResults;

    if (outResults) {
        for (int i = 0; i < count; ++i) {
            auto& r = results[i];
            outResults[i].success = r.success ? 1 : 0;
            outResults[i].mode = static_cast<int>(r.task.mode);
            outResults[i].type = static_cast<int>(r.task.type);
            strncpy_s(outResults[i].ciphertext, sizeof(outResults[i].ciphertext), r.task.ciphertext.c_str(), _TRUNCATE);
            strncpy_s(outResults[i].plaintext, sizeof(outResults[i].plaintext), r.plaintext.c_str(), _TRUNCATE);
            strncpy_s(outResults[i].error_message, sizeof(outResults[i].error_message), r.error_message.c_str(), _TRUNCATE);
        }
    }

    return count;
}
#else
int Hugo_CrackPassword(int, int, const char*, const char*, const char*, char*, int) { return 0; }
int Hugo_CrackAllPasswords(HugoCrackResult*, int) { return 0; }
#endif

/* ======================================================================== */
/* 13. HInstaller - HTTP downloader                                        */
/* ======================================================================== */

#ifndef HU_DISABLE_INSTALLER
#include "HugoUtils/HInstaller.h"

struct HugoDownloader {
    HttpDownloader impl;
    explicit HugoDownloader(int maxRedirects) : impl(maxRedirects) {}
};

HugoDownloader* Hugo_Downloader_Create(int maxRedirects) {
    return new HugoDownloader(maxRedirects);
}

void Hugo_Downloader_Destroy(HugoDownloader* h) {
    delete h;
}

void Hugo_Downloader_SetTimeout(HugoDownloader* h, int seconds) {
    if (!h) return;
    h->impl.setTimeout(seconds);
}

void Hugo_Downloader_SetUserAgent(HugoDownloader* h, const char* ua) {
    if (!h || !ua) return;
    h->impl.setUserAgent(std::string(ua));
}

int Hugo_Downloader_Download(HugoDownloader* h,
                             const char* url,
                             const char* outDir,
                             const char* customName,
                             HugoDownloadProgressCb progress,
                             int resume,
                             HugoDownloadResult* outResult) {
    if (!h || !url || !outDir || !outResult) return 0;

    std::function<void(int64_t, int64_t)> progressFn;
    if (progress) {
        progressFn = [progress](int64_t received, int64_t total) {
            progress(received, total);
        };
    }

    DownloadResult res = h->impl.download(
        url, outDir,
        customName ? std::string(customName) : std::string(),
        progressFn,
        resume != 0
    );

    outResult->success = res.success ? 1 : 0;
    outResult->statusCode = res.statusCode;
    strncpy_s(outResult->errorMsg, sizeof(outResult->errorMsg), res.errorMsg.c_str(), _TRUNCATE);
    outResult->downloaded = res.downloaded;
    outResult->total = res.total;
    strncpy_s(outResult->filename, sizeof(outResult->filename), res.filename.c_str(), _TRUNCATE);

    return res.success ? 1 : 0;
}
#else
struct HugoDownloader { int dummy; };
HugoDownloader* Hugo_Downloader_Create(int) { return nullptr; }
void Hugo_Downloader_Destroy(HugoDownloader*) {}
void Hugo_Downloader_SetTimeout(HugoDownloader*, int) {}
void Hugo_Downloader_SetUserAgent(HugoDownloader*, const char*) {}
int Hugo_Downloader_Download(HugoDownloader*, const char*, const char*, const char*, HugoDownloadProgressCb, int, HugoDownloadResult*) { return 0; }
#endif
