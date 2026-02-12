#include "HugoMount.h"
#include "Logger.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <mutex>
#include "StrConvert.h"
#include <iomanip>
using namespace std;
using namespace WinUtils;

constexpr const char* DRIVER_SERVICE_NAME = "SWAce";
constexpr const char* VDK_NT_PATH_FMT = "\\??\\VirtualDK%d\\Partition%d";
constexpr DWORD VDK_IOCTL_GET_IMAGE_INFO_SIZE = 0x72050;
constexpr DWORD VDK_IOCTL_GET_IMAGE_INFO_DATA = 0x72054;
constexpr DWORD VDK_IOCTL_GET_STATS = 0x72074;
static Logger logger(L"HugoMount");

// Format size for display
void VirtualDKManager::PrintSize(long long sectors) const {
	double sizeMB = static_cast<double>(sectors * 512) / (1024 * 1024);
	cout << setw(10) << sectors << " (" << fixed << setprecision(0) << setw(6) << sizeMB << " MB)";
}

// Get the first available drive letter
char VirtualDKManager::GetFirstAvailableDrive() const {
	DWORD drives = GetLogicalDrives();
	drives >>= 2;  // Skip A and B drives
	char letter = 'C';

	while (drives & 1) {
		drives >>= 1;
		letter++;
		if (letter > 'Z') {
			logger.DLog(LogLevel::Error, L"No available drive letters (C-Z all occupied)");
			return 0;
		}
	}
	logger.DLog(LogLevel::Debug, format(L"Found first available drive letter: {}", letter));
	return letter;
}

// Get image info block (core logic + logging)
DWORD VirtualDKManager::GetImageInfoBlock(HANDLE hDevice, void** outBlock) const {
	if (!outBlock) {
		logger.DLog(LogLevel::Error, L"GetImageInfoBlock: Input parameter outBlock is null");
		return ERROR_INVALID_PARAMETER;
	}

	DWORD bytesReturned = 0;
	DWORD requiredSize = 0;

	// Get buffer size
	if (!DeviceIoControl(hDevice, VDK_IOCTL_GET_IMAGE_INFO_SIZE,
		NULL, 0, &requiredSize, sizeof(requiredSize), &bytesReturned, NULL)) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error,
			format(L"IOCTL_GET_IMAGE_INFO_SIZE failed, error code: {}", err));
		return err;
	}

	if (requiredSize == 0) {
		logger.DLog(LogLevel::Warn, L"Image info block size is 0");
		return 0;
	}

	// Allocate memory and read data
	*outBlock = malloc(requiredSize);
	if (!*outBlock) {
		logger.DLog(LogLevel::Error, L"Failed to allocate memory for image info block");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	memset(*outBlock, 0, requiredSize);
	if (!DeviceIoControl(hDevice, VDK_IOCTL_GET_IMAGE_INFO_DATA,
		NULL, 0, *outBlock, requiredSize, &bytesReturned, NULL)) {
		DWORD err = GetLastError();
		free(*outBlock);
		*outBlock = nullptr;
		logger.DLog(LogLevel::Error,
			format(L"IOCTL_GET_IMAGE_INFO_DATA failed, error code: {}", err));
		return err;
	}

	logger.DLog(LogLevel::Debug,
		format(L"Successfully read image info block, size: {} bytes", requiredSize));
	return 0;
}

// Print partition information (user output)
void VirtualDKManager::PrintPartitions(HANDLE hDevice, DWORD totalSectors) const {
	BYTE buffer[512] = { 0 };
	DWORD bytesRead = 0;
	MBR_SECTOR* mbr = reinterpret_cast<MBR_SECTOR*>(buffer);

	cout << "      0              0    ";
	PrintSize(totalSectors);
	cout << "  <disk>\n";

	LARGE_INTEGER offset = { 0 };
	if (!SetFilePointerEx(hDevice, offset, NULL, FILE_BEGIN)) {
		logger.DLog(LogLevel::Warn, L"SetFilePointerEx failed, unable to read MBR");
		return;
	}
	if (!ReadFile(hDevice, buffer, 512, &bytesRead, NULL) || bytesRead != 512) {
		logger.DLog(LogLevel::Warn, L"Failed to read MBR sector");
		return;
	}
	if (mbr->Signature != 0xAA55) {
		logger.DLog(LogLevel::Warn, L"Invalid MBR signature (not 0xAA55)");
		return;
	}

	for (int i = 0; i < 4; i++) {
		MBR_PARTITION_ENTRY* p = &mbr->Partitions[i];
		if (p->PartitionType == 0) continue;

		cout << "      " << i + 1 << "   " << setw(12) << p->StartLBA << "    ";
		PrintSize(p->SizeInSectors);

		switch (p->PartitionType) {
		case 0x07: cout << "  IFS(NTFS/HPFS)\n"; break;
		case 0x0B:
		case 0x0C: cout << "  FAT32\n"; break;
		case 0x05:
		case 0x0F: cout << "  EXTENDED\n"; break;
		default:   cout << "  Type: 0x" << hex << setw(2) << setfill('0') << (int)p->PartitionType << dec << "\n"; break;
		}
	}
	cout << "\n";
}

// Get driver file version
int VirtualDKManager::GetDriverFileVersion(const string& rawPath, string& outVer) const {
	char fullPath[MAX_PATH * 2] = { 0 };
	DWORD dwHandle = 0;
	DWORD dwSize = 0;
	void* pData = nullptr;
	int result = -1;

	// Process SystemRoot path
	if (rawPath.compare(0, 11, "\\SystemRoot") == 0) {
		char winDir[MAX_PATH] = { 0 };
		(void)GetWindowsDirectoryA(winDir, MAX_PATH);
		snprintf(fullPath, sizeof(fullPath), "%s%s", winDir, rawPath.c_str() + 11);
	}
	else {
		snprintf(fullPath, sizeof(fullPath), "%s", rawPath.c_str());
	}

	logger.DLog(LogLevel::Debug,
		format(L"Getting driver version, path: {}", AnsiToWideString(fullPath)));

	// Disable WOW64 redirection
	PVOID oldRedirectionValue = NULL;
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (!hKernel32) {
		goto Cleanup;
	}
	{
		LPFN_WOW64DISABLEWOW64FSREDIRECTION pfnDisable =
			reinterpret_cast<LPFN_WOW64DISABLEWOW64FSREDIRECTION>(GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection"));
		LPFN_WOW64REVERTWOW64FSREDIRECTION pfnRevert =
			reinterpret_cast<LPFN_WOW64REVERTWOW64FSREDIRECTION>(GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection"));

		if (pfnDisable) pfnDisable(&oldRedirectionValue);

		// Get version information
		dwSize = GetFileVersionInfoSizeA(fullPath, &dwHandle);
		if (!dwHandle)goto Cleanup;
		if (dwSize > 0) {
			pData = malloc(dwSize);
			if (dwHandle != 0)
				if (pData && GetFileVersionInfoA(fullPath, dwHandle, dwSize, pData)) {
					// Read language and code page
					struct LANGANDCODEPAGE {
						WORD wLanguage;
						WORD wCodePage;
					} *lpTranslate = nullptr;
					UINT cbTranslate = 0;

					if (VerQueryValueA(pData, "\\VarFileInfo\\Translation",
						reinterpret_cast<LPVOID*>(&lpTranslate), &cbTranslate)) {

						char subBlock[64] = { 0 };
						char* pVerStr = nullptr;
						UINT uLen = 0;

						sprintf_s(subBlock, "\\StringFileInfo\\%04x%04x\\FileVersion",
							lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

						if (VerQueryValueA(pData, subBlock, reinterpret_cast<LPVOID*>(&pVerStr), &uLen) && uLen > 0) {
							outVer = pVerStr;
							result = 0;
						}
					}

					// Fallback solution
					if (result == -1) {
						VS_FIXEDFILEINFO* pFileInfo = nullptr;
						UINT uLen = 0;
						if (VerQueryValueA(pData, "\\", reinterpret_cast<LPVOID*>(&pFileInfo), &uLen)) {
							char verBuf[32] = { 0 };
							sprintf_s(verBuf, "%d.%d", HIWORD(pFileInfo->dwFileVersionMS), LOWORD(pFileInfo->dwFileVersionMS));
							outVer = verBuf;
							result = 0;
						}
					}
				}
			free(pData);
		}

		// Restore redirection
		if (pfnRevert) pfnRevert(oldRedirectionValue);
	}
Cleanup:
	if (result == 0) {
		logger.DLog(LogLevel::Debug,
			format(L"Successfully obtained driver version: {}", AnsiToWideString(outVer.c_str())));
	}
	else {
		logger.DLog(LogLevel::Warn, L"Failed to get driver version information");
	}
	return result;
}

// Get disk count
int VirtualDKManager::GetDiskCount() const {
	HANDLE hDevice = CreateFileA("\\\\.\\VirtualDK0\\Partition0",
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);

	if (hDevice == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Warn,
			format(L"Failed to open VirtualDK0, error code: {}, default disk count to 0", err));
		return 0;
	}

	DWORD bytesReturned = 0;
	DWORD stats[4] = { 0 };
	int count = 0;

	if (DeviceIoControl(hDevice, VDK_IOCTL_GET_STATS,
		NULL, 0, stats, sizeof(stats), &bytesReturned, NULL)) {
		count = stats[0] + stats[1] + stats[2];
		logger.DLog(LogLevel::Debug,
			format(L"Successfully obtained disk count: {}", count));
	}
	else {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error,
			format(L"IOCTL_GET_STATS failed, error code: {}", err));
	}

	CloseHandle(hDevice);
	return count;
}

// Create drive letter link (core logic + logging)
DWORD VirtualDKManager::DoLink(int diskId, int partId, char driveLetter) const {
	char dosDevice[4] = { static_cast<char>(toupper(driveLetter)), ':', 0, 0 };
	char targetPath[MAX_PATH] = { 0 };
	sprintf_s(targetPath, VDK_NT_PATH_FMT, diskId, partId);

	logger.DLog(LogLevel::Debug,
		format(L"Attempting to mount: {} -> {}", AnsiToWideString(dosDevice), AnsiToWideString(targetPath)));

	// Check if drive letter is already in use
	char checkBuf[MAX_PATH] = { 0 };
	if (QueryDosDeviceA(dosDevice, checkBuf, MAX_PATH) != 0) {
		logger.DLog(LogLevel::Warn,
			format(L"Drive letter {} is already in use: {}", driveLetter, AnsiToWideString(checkBuf)));
		return ERROR_ALREADY_ASSIGNED;
	}

	if (!DefineDosDeviceA(DDD_RAW_TARGET_PATH, dosDevice, targetPath)) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error,
			format(L"DefineDosDevice failed, error code: {}", err));
		return err;
	}

	logger.DLog(LogLevel::Info,
		format(L"Mount successful: Disk{} Partition{} -> {}:\\", diskId, partId, driveLetter));
	return 0;
}

// Remove drive letter link (core logic + logging)
DWORD VirtualDKManager::DoUnlink(char driveLetter) const {
	char dosDevice[4] = { static_cast<char>(toupper(driveLetter)), ':', 0, 0 };
	logger.DLog(LogLevel::Debug,
		format(L"Attempting to unmount drive letter: {}", driveLetter));

	if (!DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
		dosDevice, NULL)) {
		if (!DefineDosDeviceA(DDD_REMOVE_DEFINITION, dosDevice, NULL)) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error,
				format(L"Failed to unmount drive letter {}, error code: {}", driveLetter, err));
			return err;
		}
	}

	logger.DLog(LogLevel::Info,
		format(L"Unmount successful: {}:\\", driveLetter));
	return 0;
}

// Find drive letter
DWORD VirtualDKManager::FindDriveLetter(int diskId, int partId, char* outLetter) const {
	if (!outLetter) {
		logger.DLog(LogLevel::Error, L"FindDriveLetter: Input parameter outLetter is null");
		return ERROR_INVALID_PARAMETER;
	}

	char targetPath[MAX_PATH] = { 0 };
	sprintf_s(targetPath, VDK_NT_PATH_FMT, diskId, partId);
	logger.DLog(LogLevel::Debug,
		format(L"Searching for drive letter: {}", AnsiToWideString(targetPath)));

	char driveStr[4] = "A:";
	char queryBuf[MAX_PATH] = { 0 };
	DWORD drives = GetLogicalDrives();

	for (char letter = 'A'; letter <= 'Z'; letter++) {
		if (drives & 1) {
			driveStr[0] = letter;
			if (QueryDosDeviceA(driveStr, queryBuf, MAX_PATH) > 0 &&
				(string)queryBuf == targetPath) {
				*outLetter = letter;
				logger.DLog(LogLevel::Debug,
					format(L"Found corresponding drive letter: {}", letter));
				return 0;
			}
		}
		drives >>= 1;
	}

	logger.DLog(LogLevel::Warn,
		format(L"No drive letter found for Disk{} Partition{}", diskId, partId));
	return ERROR_FILE_NOT_FOUND;
}

// List all VirtualDK information (user output + logging)
void VirtualDKManager::ListAll() const {
	logger.DLog(LogLevel::Debug, L"Starting to list all VirtualDK information");

	// Open Service Control Manager
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hSCM) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error,
			format(L"Failed to open SCM, error code: {}", err));
		cout << "SCM Error: " << err << "\n";
		return;
	}

	// Open driver service
	SC_HANDLE hService = OpenServiceA(hSCM, DRIVER_SERVICE_NAME,
		SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);

	if (hService) {
		DWORD bytesNeeded = 0;
		(void)QueryServiceConfigA(hService, NULL, 0, &bytesNeeded);
		unique_ptr<BYTE[]> pConfigBuf(new BYTE[bytesNeeded]);
		LPQUERY_SERVICE_CONFIGA pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGA>(pConfigBuf.get());

		if (QueryServiceConfigA(hService, pConfig, bytesNeeded, &bytesNeeded)) {
			// Get service status
			SERVICE_STATUS_PROCESS ssStatus = { 0 };
			(void)QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
				reinterpret_cast<LPBYTE>(&ssStatus), sizeof(ssStatus), &bytesNeeded);

			// Print driver information
			cout << "Driver  : " << pConfig->lpBinaryPathName << "\n";

			string verStr = "Unknown";
			string fullPath;
			if ((string)pConfig->lpBinaryPathName == "\\SystemRoot") {
				fullPath = "C:\\Windows" + string(pConfig->lpBinaryPathName + 11);
			}
			else {
				fullPath = pConfig->lpBinaryPathName;
			}

			GetDriverFileVersion(fullPath, verStr);
			cout << "Version : " << verStr << "\n";
			cout << "Status  : " <<
				((ssStatus.dwCurrentState == SERVICE_RUNNING) ? "RUNNING" : "STOPPED") << "\n";

			logger.DLog(LogLevel::Info,
				format(L"Driver status: {}", (ssStatus.dwCurrentState == SERVICE_RUNNING) ? L"RUNNING" : L"STOPPED"));
		}

		CloseServiceHandle(hService);
	}
	else {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Warn,
			format(L"Driver {} not found or insufficient permissions, error code: {}", AnsiToWideString(DRIVER_SERVICE_NAME), err));
		cout << "Driver '" << DRIVER_SERVICE_NAME << "' not found or access denied.\n";
	}
	CloseServiceHandle(hSCM);

	// Get disk count
	int diskCount = GetDiskCount();
	cout << "Slots   : " << diskCount << "\n\n";

	// Scan disks
	int scanLimit = (diskCount > 0) ? diskCount : 4;
	for (int i = 0; i < scanLimit; i++) {
		char devicePath[64] = { 0 };
		sprintf_s(devicePath, "\\\\.\\VirtualDK%d\\Partition0", i);

		HANDLE hDevice = CreateFileA(devicePath, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

		if (hDevice == INVALID_HANDLE_VALUE) {
			logger.DLog(LogLevel::Debug,
				format(L"Skipping Disk{}: Failed to open device", i));
			continue;
		}

		cout << "Disk " << i << "\n";

		// Get disk geometry information
		DISK_GEOMETRY geometry = { 0 };
		DWORD ret = 0;
		if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY,
			NULL, 0, &geometry, sizeof(geometry), &ret, NULL)) {
			long long totalSectors = geometry.Cylinders.QuadPart *
				geometry.TracksPerCylinder * geometry.SectorsPerTrack;
			cout << "Capacity        : " << totalSectors << " sectors\n";
			logger.DLog(LogLevel::Debug,
				format(L"Disk{} capacity: {} sectors", i, totalSectors));
		}
		else {
			logger.DLog(LogLevel::Warn,
				format(L"Failed to get geometry information for Disk{}", i));
		}

		// Get image information
		void* block = nullptr;
		if (GetImageInfoBlock(hDevice, &block) == 0)
			if (block != nullptr) {
				VDK_IMAGE_HEADER* header = reinterpret_cast<VDK_IMAGE_HEADER*>(block);
				VDK_FILE_ENTRY* entry = reinterpret_cast<VDK_FILE_ENTRY*>(
					reinterpret_cast<char*>(block) + sizeof(VDK_IMAGE_HEADER));
				const char* strPtr = reinterpret_cast<const char*>(
					reinterpret_cast<char*>(block) + sizeof(VDK_IMAGE_HEADER) +
					(header->FileCount * sizeof(VDK_FILE_ENTRY)));

				cout << "Backing Files   : " << header->FileCount << "\n";
				for (DWORD k = 0; k < header->FileCount; k++) {
					cout << "  [" << entry->Size << "] " << strPtr << "\n";//strPtr:virtual disk file path
					logger.DLog(LogLevel::Debug,
						format(L"Disk{} backing file{}: {} (size: {})", i, k, AnsiToWideString(strPtr), entry->Size));
					strPtr += entry->PathLength;
					entry++;
				}

				if (header->TotalSectors > 0) {
					PrintPartitions(hDevice, header->TotalSectors);
				}

				free(block);
			}

		CloseHandle(hDevice);
		cout << "\n";
	}

	logger.DLog(LogLevel::Debug, L"Finished listing VirtualDK information");
}

// Mount virtual disk (public interface)
int VirtualDKManager::Mount(int diskId, int partId, char driveLetter) const {
	logger.DLog(LogLevel::Info,
		format(L"Executing mount command: Disk{} Partition{}, specified drive letter: {}", diskId, partId, driveLetter ? wstring(1, (wchar_t)driveLetter) : L"auto-assign"));

	// Auto-assign drive letter
	if (driveLetter == 0) {
		driveLetter = GetFirstAvailableDrive();
		if (driveLetter == 0) {
			cout << "Error: No available drive letters.\n";
			return -1;
		}
	}
	else {
		driveLetter = static_cast<char>(toupper(driveLetter));
	}

	// Execute mount
	DWORD ret = DoLink(diskId, partId, driveLetter);
	if (ret == 0) {
		cout << "Mounted Disk " << diskId << " Partition " << partId << " to " << driveLetter << ":\\" << "\n";
		return 0;
	}
	else if (ret == ERROR_ALREADY_ASSIGNED) {
		cout << "Error: Drive " << driveLetter << " is already in use.\n";
	}
	else {
		cout << "Error: Mount failed (Code " << ret << ").\n";
	}

	return static_cast<int>(ret);
}

// Unmount virtual disk (public interface)
int VirtualDKManager::Unmount(int diskId, int partId, char driveLetter) const {
	logger.DLog(LogLevel::Info,
		format(L"Executing unmount command: Disk{} Partition{}, specified drive letter: {}", diskId, partId, driveLetter ? wstring(1, (wchar_t)driveLetter) : L"auto-search"));

	// Find drive letter by disk/partition ID
	if (diskId >= 0 && partId >= 0) {
		if (FindDriveLetter(diskId, partId, &driveLetter) != 0) {
			cout << "Error: No drive letter found for Disk " << diskId << " Partition " << partId << ".\n";
			return -1;
		}
	}
	else if (driveLetter == 0) {
		cout << "Error: Invalid arguments - specify drive letter or disk/partition number.\n";
		return -1;
	}
	else {
		driveLetter = static_cast<char>(toupper(driveLetter));
	}

	// Execute unmount
	DWORD ret = DoUnlink(driveLetter);
	if (ret == 0) {
		cout << "Unmounted " << driveLetter << ":\\" << "\n";
		return 0;
	}
	else {
		cout << "Error: Unmount failed (Code " << ret << ").\n";
		return static_cast<int>(ret);
	}
}