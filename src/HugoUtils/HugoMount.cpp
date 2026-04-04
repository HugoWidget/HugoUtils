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
 */
#include "HugoUtils/HugoUtilsDef.h"
#ifndef HU_DISABLE_MOUNT
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <mutex>
#include <iomanip>

#include "HugoUtils/HugoMount.h"
#include "WinUtils/Logger.h"
#include "WinUtils/StrConvert.h"
using namespace std;
using namespace WinUtils;

constexpr const char* DRIVER_SERVICE_NAME = "SWAce";
constexpr const char* VDK_NT_PATH_FMT = "\\??\\VirtualDK%d\\Partition%d";
constexpr DWORD VDK_IOCTL_GET_IMAGE_INFO_SIZE = 0x72050;
constexpr DWORD VDK_IOCTL_GET_IMAGE_INFO_DATA = 0x72054;
constexpr DWORD VDK_IOCTL_GET_STATS = 0x72074;
static Logger logger(L"HugoMount");

// ----------------------------------------------------------------------
// Public member functions
// ----------------------------------------------------------------------

// Retrieves all VirtualDK information: driver status, disk count, disk details,
// backing files and partition tables.
HugoMountInfo HugoMount::GetAllInfo() const {
	logger.DLog(LogLevel::Debug, L"Starting to list all VirtualDK information");

	HugoMountInfo info;

	// Open SCM and query driver service configuration/status
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hSCM) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error,
			format(L"Failed to open SCM, error code: {}", err));
		info.diskCount = GetDiskCount();
		return info;
	}

	SC_HANDLE hService = OpenServiceA(hSCM, DRIVER_SERVICE_NAME,
		SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
	if (hService) {
		DWORD bytesNeeded = 0;
		QueryServiceConfigA(hService, NULL, 0, &bytesNeeded);
		unique_ptr<BYTE[]> pConfigBuf(new BYTE[bytesNeeded]);
		LPQUERY_SERVICE_CONFIGA pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGA>(pConfigBuf.get());

		if (QueryServiceConfigA(hService, pConfig, bytesNeeded, &bytesNeeded)) {
			info.driver.binaryPath = pConfig->lpBinaryPathName;

			string fullPath;
			if (info.driver.binaryPath == "\\SystemRoot") {
				fullPath = "C:\\Windows" + info.driver.binaryPath.substr(11);
			}
			else {
				fullPath = info.driver.binaryPath;
			}
			info.driver.version = *GetDriverFileVersion(fullPath);

			SERVICE_STATUS_PROCESS ssStatus = { 0 };
			if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
				reinterpret_cast<LPBYTE>(&ssStatus), sizeof(ssStatus), &bytesNeeded)) {
				info.driver.isRunning = (ssStatus.dwCurrentState == SERVICE_RUNNING);
			}

			logger.DLog(LogLevel::Info,
				format(L"Driver status: {}", info.driver.isRunning ? L"RUNNING" : L"STOPPED"));
		}
		CloseServiceHandle(hService);
	}
	else {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Warn,
			format(L"Driver {} not found or insufficient permissions, error code: {}",
				ConvertString<wstring>(DRIVER_SERVICE_NAME), err));
	}
	CloseServiceHandle(hSCM);

	// Get disk count and scan each disk
	info.diskCount = GetDiskCount();
	int scanLimit = (info.diskCount > 0) ? info.diskCount : 4;
	for (int i = 0; i < scanLimit; ++i) {
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

		HugoMountInfo::DiskInfo disk;
		disk.index = i;

		// Retrieve disk geometry (capacity)
		DISK_GEOMETRY geometry = { 0 };
		DWORD ret = 0;
		if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY,
			NULL, 0, &geometry, sizeof(geometry), &ret, NULL)) {
			disk.capacitySectors = geometry.Cylinders.QuadPart *
				geometry.TracksPerCylinder * geometry.SectorsPerTrack;
			logger.DLog(LogLevel::Debug,
				format(L"Disk{} capacity: {} sectors", i, disk.capacitySectors));
		}
		else {
			logger.DLog(LogLevel::Warn,
				format(L"Failed to get geometry information for Disk{}", i));
		}

		// Get backing file information via custom IOCTL
		void* block = nullptr;
		if (GetImageInfoBlock(hDevice, &block) == 0 && block != nullptr) {
			VDK_IMAGE_HEADER* header = reinterpret_cast<VDK_IMAGE_HEADER*>(block);
			VDK_FILE_ENTRY* entry = reinterpret_cast<VDK_FILE_ENTRY*>(
				reinterpret_cast<char*>(block) + sizeof(VDK_IMAGE_HEADER));
			const char* strPtr = reinterpret_cast<const char*>(
				reinterpret_cast<char*>(block) + sizeof(VDK_IMAGE_HEADER) +
				(header->FileCount * sizeof(VDK_FILE_ENTRY)));

			for (DWORD k = 0; k < header->FileCount; ++k) {
				HugoMountInfo::BackingFileInfo bf;
				bf.size = entry->Size;
				bf.path = strPtr;
				disk.backingFiles.push_back(bf);

				logger.DLog(LogLevel::Debug,
					format(L"Disk{} backing file{}: {} (size: {})",
						i, k, ConvertString<wstring>(strPtr), entry->Size));

				strPtr += entry->PathLength;
				++entry;
			}

			// Collect partition information from MBR if available
			if (header->TotalSectors > 0) {
				disk.partitions = GetPartitionsInfo(hDevice, header->TotalSectors);
			}

			delete[] block;
			block = nullptr;
		}

		info.disks.push_back(move(disk));
		CloseHandle(hDevice);
	}

	logger.DLog(LogLevel::Debug, L"Finished listing VirtualDK information");
	return info;
}

// Prints all VirtualDK information to the console in a human-readable format.
void HugoMount::PrintAllInfo() const {
	HugoMountInfo info = GetAllInfo();

	// Driver information
	if (!info.driver.binaryPath.empty()) {
		cout << "Driver  : " << info.driver.binaryPath << "\n";
		cout << "Version : " << info.driver.version << "\n";
		cout << "Status  : " << (info.driver.isRunning ? "RUNNING" : "STOPPED") << "\n";
	}
	else {
		cout << "Driver '" << DRIVER_SERVICE_NAME << "' not found or access denied.\n";
	}

	// Disk count and per-disk details
	cout << "Slots   : " << info.diskCount << "\n\n";
	for (const auto& disk : info.disks) {
		cout << "Disk " << disk.index << "\n";
		cout << "Capacity        : " << disk.capacitySectors << " sectors\n";
		cout << "Backing Files   : " << disk.backingFiles.size() << "\n";
		for (size_t k = 0; k < disk.backingFiles.size(); ++k) {
			const auto& bf = disk.backingFiles[k];
			cout << "  [" << bf.size << "] " << bf.path << "\n";
		}
		// Partition information
		PrintPartitionsInfo(disk);
	}
}

// Reads the MBR from the given device and returns a vector of partition info.
vector<HugoMountInfo::PartitionInfo> HugoMount::GetPartitionsInfo(HANDLE hDevice, DWORD totalSectors) const
{
	vector<HugoMountInfo::PartitionInfo> partitions;
	BYTE buffer[512] = { 0 };
	DWORD bytesRead = 0;
	MBR_SECTOR* mbr = reinterpret_cast<MBR_SECTOR*>(buffer);

	LARGE_INTEGER offset = { 0 };
	if (!SetFilePointerEx(hDevice, offset, NULL, FILE_BEGIN)) {
		logger.DLog(LogLevel::Warn, L"SetFilePointerEx failed, unable to read MBR");
		return partitions;
	}
	if (!ReadFile(hDevice, buffer, 512, &bytesRead, NULL) || bytesRead != 512) {
		logger.DLog(LogLevel::Warn, L"Failed to read MBR sector");
		return partitions;
	}
	if (mbr->Signature != 0xAA55) {
		logger.DLog(LogLevel::Warn, L"Invalid MBR signature (not 0xAA55)");
		return partitions;
	}

	for (int i = 0; i < 4; ++i) {
		MBR_PARTITION_ENTRY* p = &mbr->Partitions[i];
		if (p->PartitionType == 0) continue;

		HugoMountInfo::PartitionInfo pi;
		pi.index = i + 1;
		pi.startLBA = p->StartLBA;
		pi.sizeInSectors = p->SizeInSectors;
		pi.partitionType = p->PartitionType;

		switch (p->PartitionType) {
		case 0x07: pi.typeDescription = "IFS(NTFS/HPFS)"; break;
		case 0x0B:
		case 0x0C: pi.typeDescription = "FAT32"; break;
		case 0x05:
		case 0x0F: pi.typeDescription = "EXTENDED"; break;
		default:
			char buf[32];
			sprintf_s(buf, "Type: 0x%02X", (int)p->PartitionType);
			pi.typeDescription = buf;
			break;
		}

		partitions.push_back(pi);
	}

	logger.DLog(LogLevel::Debug,
		format(L"Found {} partitions in MBR for device", partitions.size()));
	return partitions;
}

// Prints partition information for a single disk to the console.
void HugoMount::PrintPartitionsInfo(const HugoMountInfo::DiskInfo& disk) const {
	cout << "      0              0    ";
	PrintFormattedSize(disk.capacitySectors);
	cout << "  <disk>\n";

	// Print each partition
	for (const auto& p : disk.partitions) {
		cout << "      " << p.index << "   " << setw(12) << p.startLBA << "    ";
		PrintFormattedSize(p.sizeInSectors);
		cout << "  " << p.typeDescription << "\n";
	}
	cout << "\n";
}

// Retrieves the file version string of the driver binary.
// Handles "\\SystemRoot" prefix and disables WOW64 redirection temporarily.
expected<string, DWORD> HugoMount::GetDriverFileVersion(const string& path) const {
	char fullPath[MAX_PATH * 2] = { 0 };
	// Resolve \SystemRoot prefix to actual Windows directory
	if (path.compare(0, 11, "\\SystemRoot") == 0) {
		char winDir[MAX_PATH];
		if (!GetWindowsDirectoryA(winDir, MAX_PATH)) {
			return unexpected(GetLastError());
		}
		snprintf(fullPath, sizeof(fullPath), "%s%s", winDir, path.c_str() + 11);
	}
	else {
		snprintf(fullPath, sizeof(fullPath), "%s", path.c_str());
	}

	// Disable WOW64 file system redirection to access the correct sysnative path
	PVOID oldRedir = nullptr;
	HMODULE hKernel = GetModuleHandleA("kernel32.dll");
	if (!hKernel) {
		return unexpected(ERROR_MOD_NOT_FOUND);
	}
	auto pfnDisable = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(hKernel, "Wow64DisableWow64FsRedirection");
	auto pfnRevert = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(hKernel, "Wow64RevertWow64FsRedirection");
	if (pfnDisable) pfnDisable(&oldRedir);

	DWORD dwHandle = 0;
	DWORD dwSize = GetFileVersionInfoSizeA(fullPath, &dwHandle);
	if (dwSize == 0) {
		DWORD err = GetLastError();
		if (pfnRevert) pfnRevert(oldRedir);
		return unexpected(err);
	}

	vector<char> verData(dwSize);
	if (!GetFileVersionInfoA(fullPath, dwHandle, dwSize, verData.data())) {
		DWORD err = GetLastError();
		if (pfnRevert) pfnRevert(oldRedir);
		return unexpected(err);
	}

	string result = "Unknown";
	// Try to get StringFileInfo\FileVersion (localized)
	struct LANGANDCODEPAGE { WORD wLanguage; WORD wCodePage; } *lpTranslate = nullptr;
	UINT cbTranslate = 0;
	if (VerQueryValueA(verData.data(), "\\VarFileInfo\\Translation",
		(LPVOID*)&lpTranslate, &cbTranslate) && cbTranslate >= sizeof(LANGANDCODEPAGE)) {
		char subBlock[64];
		sprintf_s(subBlock, "\\StringFileInfo\\%04x%04x\\FileVersion",
			lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
		char* pVerStr = nullptr;
		UINT uLen = 0;
		if (VerQueryValueA(verData.data(), subBlock, (LPVOID*)&pVerStr, &uLen) && uLen > 0) {
			result = pVerStr;
		}
	}
	// Fallback to fixed file version info
	if (result == "Unknown") {
		VS_FIXEDFILEINFO* pFileInfo = nullptr;
		UINT uLen = 0;
		if (VerQueryValueA(verData.data(), "\\", (LPVOID*)&pFileInfo, &uLen) && pFileInfo) {
			char verBuf[32];
			sprintf_s(verBuf, "%d.%d", HIWORD(pFileInfo->dwFileVersionMS), LOWORD(pFileInfo->dwFileVersionMS));
			result = verBuf;
		}
	}

	if (pfnRevert) pfnRevert(oldRedir);
	return result;
}

// Mounts the specified disk/partition to the given drive letter (or auto-assigns one).
// Returns 0 on success, otherwise an error code.
int HugoMount::Mount(int diskId, int partId, char driveLetter) const {
	logger.DLog(LogLevel::Info,
		format(L"Executing mount command: Disk{} Partition{}, specified drive letter: {}", diskId, partId, driveLetter ? wstring(1, (wchar_t)driveLetter) : L"auto-assign"));

	// Auto-assign drive letter if none provided
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

	// Perform the actual link creation
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

// Unmounts a previously mounted drive. The drive can be specified by disk/partition ID or directly by letter.
// Returns 0 on success, otherwise an error code.
int HugoMount::Unmount(int diskId, int partId, char driveLetter) const {
	logger.DLog(LogLevel::Info,
		format(L"Executing unmount command: Disk{} Partition{}, specified drive letter: {}", diskId, partId, driveLetter ? wstring(1, (wchar_t)driveLetter) : L"auto-search"));

	// If disk/partition IDs are given, find the corresponding drive letter
	if (diskId >= 0 && partId >= 0) {
		if (FindDriveLetter(diskId, partId, driveLetter) != 0) {
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

	// Perform the actual link removal
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

// ----------------------------------------------------------------------
// Private member functions
// ----------------------------------------------------------------------

// Returns a string containing all currently used drive letters (lowercase).
std::string HugoMount::GetDrivesInUse() const
{
	auto IsDriveLetterInUse = [](char driveLetter) ->bool {
		char dosDevice[4] = { static_cast<char>(toupper(driveLetter)), ':', '\0' };
		char buf[MAX_PATH] = {};
		return QueryDosDeviceA(dosDevice, buf, MAX_PATH) != 0;
		};
	string res;
	char c = 'a';
	while (c <= 'z') {
		if (IsDriveLetterInUse(c)) res += c;
		c++;
	}
	return res;
}

// Finds the first available drive letter from C to Z that is not currently in use.
char HugoMount::GetFirstAvailableDrive() const {
	/*
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
	*/
	auto IsDriveLetterInUse = [](char driveLetter) ->bool {
		char dosDevice[4] = { static_cast<char>(toupper(driveLetter)), ':', '\0' };
		char buf[MAX_PATH] = {};
		return QueryDosDeviceA(dosDevice, buf, MAX_PATH) != 0;
		};
	for (char c = 'c'; c <= 'z'; c++) {
		if (!IsDriveLetterInUse(c)) return c;
	}
	return 0; // No free drive letter found
}

// Searches for the drive letter that is currently linked to the specified disk/partition.
// If found, stores it in outLetter and returns 0; otherwise returns an error code.
DWORD HugoMount::FindDriveLetter(int diskId, int partId, char& outLetter) const {
	char targetPath[MAX_PATH] = {};
	sprintf_s(targetPath, VDK_NT_PATH_FMT, diskId, partId);
	logger.DLog(LogLevel::Debug,
		format(L"Searching for drive letter: {}", ConvertString<wstring>(targetPath)));

	char driveStr[4] = "A:";
	char queryBuf[MAX_PATH] = {};
	string drives = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";//GetDrivesInUse(); //GetLogicalDrives();

	for (char letter : drives) {
		driveStr[0] = letter;
		if (QueryDosDeviceA(driveStr, queryBuf, MAX_PATH) > 0) {
			if ((string)queryBuf == (string)targetPath) {
				outLetter = letter;
				logger.DLog(LogLevel::Debug,
					format(L"Found corresponding drive letter: {}", (wchar_t)letter));
				return 0;
			}
		}
	}

	logger.DLog(LogLevel::Warn,
		format(L"No drive letter found for Disk{} Partition{}", diskId, partId));
	return ERROR_FILE_NOT_FOUND;
}

// Queries the VirtualDK driver for the total number of disks currently configured.
int HugoMount::GetDiskCount() const {
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

// Retrieves the image information block from the driver via two IOCTLs.
// Allocates memory and returns it in outBlock; caller must free() it.
DWORD HugoMount::GetImageInfoBlock(HANDLE hDevice, void** outBlock) const {
	if (!outBlock) {
		logger.DLog(LogLevel::Error, L"GetImageInfoBlock: Input parameter outBlock is null");
		return ERROR_INVALID_PARAMETER;
	}

	DWORD bytesReturned = 0;
	DWORD requiredSize = 0;

	// First get the required buffer size
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

	// Allocate memory and read the actual data

	*outBlock = new (std::nothrow) char[requiredSize] {};
	if (!*outBlock) {
		logger.DLog(LogLevel::Error, L"Failed to allocate memory for image info block");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

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

// Core function to create a DOS device link (mount) for the specified disk/partition.
DWORD HugoMount::DoLink(int diskId, int partId, char driveLetter) const {
	char dosDevice[4] = { static_cast<char>(toupper(driveLetter)), ':', 0, 0 };
	char targetPath[MAX_PATH] = { 0 };
	sprintf_s(targetPath, VDK_NT_PATH_FMT, diskId, partId);

	logger.DLog(LogLevel::Debug,
		format(L"Attempting to mount: {} -> {}", ConvertString<wstring>(dosDevice), ConvertString<wstring>(targetPath)));

	// Verify the drive letter is not already taken
	char checkBuf[MAX_PATH] = { 0 };
	if (QueryDosDeviceA(dosDevice, checkBuf, MAX_PATH) != 0) {
		logger.DLog(LogLevel::Warn,
			format(L"Drive letter {} is already in use: {}", driveLetter, ConvertString<wstring>(checkBuf)));
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

// Core function to remove a DOS device link (unmount) for the given drive letter.
DWORD HugoMount::DoUnlink(char driveLetter) const {
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

// Helper function to print a sector count in a formatted way (sectors + MB).
void HugoMount::PrintFormattedSize(long long sectors) const {
	double sizeMB = static_cast<double>(sectors * 512) / (1024 * 1024);
	cout << setw(10) << sectors << " (" << fixed << setprecision(0) << setw(6) << sizeMB << " MB)";
}
#endif // !HU_DISABLE_MOUNT