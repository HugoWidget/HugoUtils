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
 * HugoUtilsC - C language interface for HugoUtils.
 * Provides C-callable wrappers for the C++ HugoUtils library.
 *
 * Usage notes:
 * - String output functions use caller-provided buffers.
 *   Return value: required length excluding null terminator.
 *   If buffer is NULL or too small, required length is returned and nothing is written.
 *   If the underlying operation fails (e.g. not found), returns 0.
 * - Opaque handles must be destroyed by the corresponding Destroy function.
 * - Result codes mirror FreezeOperationResult: 0=Success, 1=Failed, 2=InvalidParam,
 *   3=DriverError, 4=NetworkError, 5=InitFailed, 6=NotInitialized, 7=NotSupported.
 */
#ifndef HUGOUTILS_C_H
#define HUGOUTILS_C_H

#include <stdint.h>
#include <stddef.h>

 /* For DWORD type on Windows */
#ifdef _WIN32
#include <windows.h>
#endif

// build as static library: define HUGOUTILS_NO_EXPORTS to disable DLL export/import
#if defined(HUGOUTILS_NO_EXPORTS)
#define HUGO_C_API
#else
// build as DLL: define HUGOUTILS_EXPORTS when building the DLL, otherwise import
#if defined(_WIN32)
#if defined(HUGOUTILS_EXPORTS)
#define HUGO_C_API __declspec(dllexport)
#else
#define HUGO_C_API __declspec(dllimport)
#endif
#else
#define HUGO_C_API
#endif // HUGOUTILS_EXPORTS

#endif // HUGOUTILS_NO_EXPORT

#ifdef __cplusplus
extern "C" {
#endif

	/* ======================================================================== */
	/* Common types                                                             */
	/* ======================================================================== */

	/* Result codes (mirror of FreezeOperationResult) */
	typedef enum HugoResult {
		HUGO_OK = 0,
		HUGO_FAILED = 1,
		HUGO_INVALID_PARAM = 2,
		HUGO_DRIVER_ERROR = 3,
		HUGO_NETWORK_ERROR = 4,
		HUGO_INIT_FAILED = 5,
		HUGO_NOT_INIT = 6,
		HUGO_NOT_SUPPORTED = 7
	} HugoResult;

	/* Drive freeze state (mirror of DriveFreezeState) */
	typedef enum HugoDriveFreezeState {
		HUGO_DRIVE_UNFROZEN = 0,
		HUGO_DRIVE_FROZEN = 1,
		HUGO_DRIVE_PENDING_FREEZE = 2,
		HUGO_DRIVE_PENDING_UNFREEZE = 3,
		HUGO_DRIVE_UNKNOWN = 4
	} HugoDriveFreezeState;

	/* Crack mode (mirror of CrackMode) */
	typedef enum HugoCrackMode {
		HUGO_CRACK_V1 = 0,
		HUGO_CRACK_V2 = 1,
		HUGO_CRACK_V3 = 2
	} HugoCrackMode;

	/* Password type (mirror of PasswordType) */
	typedef enum HugoPasswordType {
		HUGO_PASSWORD_ADMIN = 0,
		HUGO_PASSWORD_LOCK = 1
	} HugoPasswordType;

	/* Disk info for freeze state queries */
	typedef struct HugoDiskInfo {
		HugoDriveFreezeState state;
		uint64_t bytesFree;
		uint64_t bytesTotal;
	} HugoDiskInfo;

	/* Download result (mirror of DownloadResult) */
	typedef struct HugoDownloadResult {
		int      success;
		int      statusCode;
		char     errorMsg[512];
		uint64_t downloaded;
		uint64_t total;
		char     filename[260];
	} HugoDownloadResult;

	/* Progress callback for download: received, total */
	typedef void (*HugoDownloadProgressCb)(int64_t received, int64_t total);

	/* Crack result for batch cracking */
	typedef struct HugoCrackResult {
		int  success;
		int  mode;
		int  type;
		char ciphertext[256];
		char plaintext[32];
		char error_message[128];
	} HugoCrackResult;

	/* Opaque handle forward declarations */
	typedef struct HugoSharedFlag         HugoSharedFlag;
	typedef struct HugoMount              HugoMount;
	typedef struct HugoFreezeApi          HugoFreezeApi;
	typedef struct HugoFreezeDriver       HugoFreezeDriver;
	typedef struct HugoFreezeDriverPrivate HugoFreezeDriverPrivate;
	typedef struct HugoDownloader         HugoDownloader;

	/* ======================================================================== */
	/* 1. HugoUtils - Drive utilities                                           */
	/* ======================================================================== */

	HUGO_C_API int  Hugo_GetDrivesInUse(char* buf, int bufSize);
	HUGO_C_API int  Hugo_GetLogicalDrives(char* buf, int bufSize);

	/* ======================================================================== */
	/* 2. HArt - ASCII art text                                                 */
	/* ======================================================================== */

	HUGO_C_API void Hugo_PrintArtText(int idx);
	HUGO_C_API int  Hugo_GetArtTextLineCount(int idx);
	HUGO_C_API int  Hugo_GetArtTextLine(int idx, int line, wchar_t* buf, int bufSize);

	/* ======================================================================== */
	/* 3. GPL3 - License display                                               */
	/* ======================================================================== */

	HUGO_C_API void Hugo_ShowWarranty(void);
	HUGO_C_API void Hugo_ShowLicense(const wchar_t* licensePath);

	/* ======================================================================== */
	/* 4. HInfo - Seewo info queries                                           */
	/* ======================================================================== */

	HUGO_C_API int  Hugo_GetHugoVersion(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetHugoFolder(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetHugoProtectDriverFolder(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetHugoProtectDriverPath(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetMachineId(char* buf, int bufSize);
	HUGO_C_API int  Hugo_GetSeewoCoreIniPath(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetLockConfigIniPath(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetLockConfigIniPath2(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetSeewoSchoolFilePath(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_GetHugoUpdateFolderCount(void);
	HUGO_C_API int  Hugo_GetHugoUpdateFolder(int index, wchar_t* buf, int bufSize);

	/* ======================================================================== */
	/* 5. HLock - SharedFlag                                                   */
	/* ======================================================================== */

	HUGO_C_API HugoSharedFlag* Hugo_SharedFlag_Create(const wchar_t* name);
	HUGO_C_API void Hugo_SharedFlag_Destroy(HugoSharedFlag* flag);
	HUGO_C_API void Hugo_SharedFlag_Set(HugoSharedFlag* flag, int val);
	HUGO_C_API int  Hugo_SharedFlag_Get(HugoSharedFlag* flag);
	HUGO_C_API int  Hugo_SharedFlag_Valid(HugoSharedFlag* flag);

	/* ======================================================================== */
	/* 6. HMount - Virtual disk mount management                               */
	/* ======================================================================== */

	HUGO_C_API HugoMount* Hugo_Mount_Create(void);
	HUGO_C_API void       Hugo_Mount_Destroy(HugoMount* m);
	HUGO_C_API void       Hugo_Mount_PrintAllInfo(HugoMount* m);
	HUGO_C_API int        Hugo_Mount_Mount(HugoMount* m, int diskId, int partId, char driveLetter);
	HUGO_C_API int        Hugo_Mount_UnmountById(HugoMount* m, int diskId, int partId);
	HUGO_C_API int        Hugo_Mount_UnmountByLetter(HugoMount* m, char driveLetter);
	HUGO_C_API int        Hugo_Mount_FindMountedDrive(HugoMount* m, int diskId, int partId, char* buf, int bufSize);

	/* ======================================================================== */
	/* 7. HFreezeInterface - Volume mask helper                                */
	/* ======================================================================== */

	HUGO_C_API uint32_t Hugo_CalculateVolumeMask(const wchar_t* driveLetters);

	/* ======================================================================== */
	/* 8. HFreezeApi - Seewo Freeze HTTP API                                   */
	/* ======================================================================== */

	HUGO_C_API HugoFreezeApi* Hugo_FreezeApi_Create(void);
	HUGO_C_API void          Hugo_FreezeApi_Destroy(HugoFreezeApi* h);
	HUGO_C_API HugoResult    Hugo_FreezeApi_Init(HugoFreezeApi* h);
	HUGO_C_API void          Hugo_FreezeApi_Cleanup(HugoFreezeApi* h);
	HUGO_C_API int           Hugo_FreezeApi_IsInitialized(HugoFreezeApi* h);
	HUGO_C_API void          Hugo_FreezeApi_SetConfig(HugoFreezeApi* h, const wchar_t* ip, uint16_t port);
	HUGO_C_API int           Hugo_FreezeApi_GetConfig(HugoFreezeApi* h, wchar_t* ipBuf, int ipBufSize, uint16_t* outPort);
	HUGO_C_API HugoResult    Hugo_FreezeApi_GetFreezeState(HugoFreezeApi* h, wchar_t* msgBuf, int msgBufSize);
	HUGO_C_API HugoResult    Hugo_FreezeApi_TryProtect(HugoFreezeApi* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize);
	HUGO_C_API HugoResult    Hugo_FreezeApi_SetFreezeState(HugoFreezeApi* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize);

	/* ======================================================================== */
	/* 9. HFreezeDriver - Freeze driver management                             */
	/* ======================================================================== */

	HUGO_C_API HugoFreezeDriver* Hugo_FreezeDriver_Create(void);
	HUGO_C_API void              Hugo_FreezeDriver_Destroy(HugoFreezeDriver* h);
	HUGO_C_API HugoResult        Hugo_FreezeDriver_Init(HugoFreezeDriver* h);
	HUGO_C_API void              Hugo_FreezeDriver_Cleanup(HugoFreezeDriver* h);
	HUGO_C_API int               Hugo_FreezeDriver_IsInitialized(HugoFreezeDriver* h);
	HUGO_C_API HugoResult        Hugo_FreezeDriver_GetFreezeState(HugoFreezeDriver* h, int* diskCount, wchar_t* msgBuf, int msgBufSize);
	HUGO_C_API int               Hugo_FreezeDriver_GetDiskEntry(HugoFreezeDriver* h, int index, wchar_t* outLetter, HugoDiskInfo* outInfo);
	HUGO_C_API HugoResult        Hugo_FreezeDriver_TryProtect(HugoFreezeDriver* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize);
	HUGO_C_API HugoResult        Hugo_FreezeDriver_SetFreezeState(HugoFreezeDriver* h, const wchar_t* driveLetters, wchar_t* msgBuf, int msgBufSize);

	/* ======================================================================== */
	/* 10. HFreezeFile_p - Volume info config file I/O                         */
	/* ======================================================================== */

	HUGO_C_API void Hugo_FreezeFile_SetConfigPath(const wchar_t* path);
	HUGO_C_API int  Hugo_FreezeFile_GetConfigPath(wchar_t* buf, int bufSize);
	HUGO_C_API int  Hugo_FreezeFile_ReadConfig(uint8_t* outConfig, int configSize);
	HUGO_C_API int  Hugo_FreezeFile_ReadConfigFrom(const wchar_t* path, uint8_t* outConfig, int configSize);
	HUGO_C_API int  Hugo_FreezeFile_WriteConfig(const uint8_t* config, int configSize);
	HUGO_C_API int  Hugo_FreezeFile_WriteConfigTo(const uint8_t* config, int configSize, const wchar_t* path);
	HUGO_C_API int  Hugo_FreezeFile_BuildFreezeConfig(const uint8_t* original, uint32_t targetVolMask, int enableFreeze, uint8_t* out);

	/* ======================================================================== */
	/* 11. HFreezeDriver_p - Direct driver IOCTL communication                 */
	/* ======================================================================== */

	HUGO_C_API HugoFreezeDriverPrivate* Hugo_FreezeDrvPriv_Create(void);
	HUGO_C_API void                     Hugo_FreezeDrvPriv_Destroy(HugoFreezeDriverPrivate* h);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_Init(HugoFreezeDriverPrivate* h);
	HUGO_C_API void                     Hugo_FreezeDrvPriv_Cleanup(HugoFreezeDriverPrivate* h);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_IsInitialized(HugoFreezeDriverPrivate* h);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryBootConfig(HugoFreezeDriverPrivate* h, uint8_t* outConfig, int configSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_WriteBootConfig(HugoFreezeDriverPrivate* h, const uint8_t* config, int configSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryBootSystem(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryKeyResult(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryProtectionState(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryPassThrough(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryOldDriverQuality(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryDiskFull(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryBsodInfo(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_QueryRedirectData(HugoFreezeDriverPrivate* h, void* outBuf, int bufSize);
	HUGO_C_API void                     Hugo_FreezeDrvPriv_TriggerBSOD(HugoFreezeDriverPrivate* h);
	HUGO_C_API void                     Hugo_FreezeDrvPriv_FlushWppLogs(HugoFreezeDriverPrivate* h);
	HUGO_C_API DWORD                    Hugo_FreezeDrvPriv_GetLastErrorCode(HugoFreezeDriverPrivate* h);
	HUGO_C_API int                      Hugo_FreezeDrvPriv_GetLastErrorMsg(HugoFreezeDriverPrivate* h, wchar_t* buf, int bufSize);

	/* ======================================================================== */
	/* 12. HPassword - Password cracking                                        */
	/* ======================================================================== */

	HUGO_C_API int Hugo_CrackPassword(int mode, int type,
		const char* ciphertext,
		const char* deviceId,
		const char* machineId,
		char* plainOut, int plainBufSize);

	HUGO_C_API int Hugo_CrackAllPasswords(HugoCrackResult* outResults, int maxResults);

	/* ======================================================================== */
	/* 13. HInstaller - HTTP downloader                                        */
	/* ======================================================================== */

	HUGO_C_API HugoDownloader* Hugo_Downloader_Create(int maxRedirects);
	HUGO_C_API void            Hugo_Downloader_Destroy(HugoDownloader* h);
	HUGO_C_API void            Hugo_Downloader_SetTimeout(HugoDownloader* h, int seconds);
	HUGO_C_API void            Hugo_Downloader_SetUserAgent(HugoDownloader* h, const char* ua);
	HUGO_C_API int             Hugo_Downloader_Download(HugoDownloader* h,
		const char* url,
		const char* outDir,
		const char* customName,
		HugoDownloadProgressCb progress,
		int resume,
		HugoDownloadResult* outResult);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUGOUTILS_C_H */
