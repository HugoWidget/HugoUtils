#pragma once
#include <windows.h>
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