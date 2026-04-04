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
#pragma once
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_MOUNT

#include <Windows.h>

#include <winioctl.h>
#include <string>
#include <vector>
#include <memory>
#include <expected>
using LPFN_WOW64DISABLEWOW64FSREDIRECTION = BOOL(WINAPI*)(PVOID*);
using LPFN_WOW64REVERTWOW64FSREDIRECTION = BOOL(WINAPI*)(PVOID);

#pragma pack(push, 1)
struct VDK_FILE_ENTRY {
	DWORD Type;
	DWORD Size;
	DWORD PathLength;
	DWORD Unknown;
};

struct VDK_IMAGE_HEADER {
	DWORD Status;
	DWORD TotalSectors;
	DWORD Unk2;
	DWORD Unk3;
	DWORD Unk4;
	DWORD FileCount;
};

struct MBR_PARTITION_ENTRY {
	BYTE BootIndicator;
	BYTE StartHead;
	BYTE StartSector;
	BYTE StartCylinder;
	BYTE PartitionType;
	BYTE EndHead;
	BYTE EndSector;
	BYTE EndCylinder;
	DWORD StartLBA;
	DWORD SizeInSectors;
};

struct MBR_SECTOR {
	BYTE BootstrapCode[446];
	MBR_PARTITION_ENTRY Partitions[4];
	WORD Signature;
};
#pragma pack(pop)
struct HugoMountInfo {
	struct DriverInfo {
		std::string binaryPath;
		std::string version;
		bool isRunning;   // true = RUNNING, false = STOPPED
	};

	struct BackingFileInfo {
		DWORD size;
		std::string path;
	};

	struct PartitionInfo {
		int index;                   //  1-4
		DWORD startLBA;
		DWORD sizeInSectors;
		BYTE partitionType;
		std::string typeDescription; //  "IFS(NTFS/HPFS)"
	};

	struct DiskInfo {
		int index;
		long long capacitySectors;
		std::vector<BackingFileInfo> backingFiles;
		std::vector<PartitionInfo> partitions;
	};

	DriverInfo driver;
	int diskCount;
	std::vector<DiskInfo> disks;
};

class HugoMount {
public:
	HugoMount() = default;
	~HugoMount() = default;
	HugoMountInfo GetAllInfo() const;
	void PrintAllInfo() const;
	std::vector<HugoMountInfo::PartitionInfo> GetPartitionsInfo(
		HANDLE hDevice, DWORD totalSectors) const;
	void PrintPartitionsInfo(const HugoMountInfo::DiskInfo& disk) const;
	std::expected<std::string, DWORD> GetDriverFileVersion(const std::string& path) const;
	int Mount(int diskId, int partId, char driveLetter = 0) const;
	int Unmount(int diskId = -1, int partId = -1, char driveLetter = 0) const;
private:
	std::string GetDrivesInUse()const;
	char GetFirstAvailableDrive() const;
	DWORD FindDriveLetter(int diskId, int partId, char& outLetter) const;
	int GetDiskCount() const;
	DWORD GetImageInfoBlock(HANDLE hDevice, void** outBlock) const;
	DWORD DoLink(int diskId, int partId, char driveLetter) const;
	DWORD DoUnlink(char driveLetter) const;
	void PrintFormattedSize(long long sectors) const;
private:
	HugoMount(const HugoMount&) = delete;
	HugoMount& operator=(const HugoMount&) = delete;
};
#endif // !HU_DISABLE_MOUNT