#pragma once
#include <windows.h>
#include <winioctl.h>
#include <string>
#include <vector>
#include <memory>
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

class VirtualDKManager {
private:
    void PrintSize(long long sectors) const;
    char GetFirstAvailableDrive() const;
    DWORD GetImageInfoBlock(HANDLE hDevice, void** outBlock) const;
    void PrintPartitions(HANDLE hDevice, DWORD totalSectors) const;
    int GetDriverFileVersion(const std::string& rawPath, std::string& outVer) const;
    int GetDiskCount() const;
    DWORD DoLink(int diskId, int partId, char driveLetter) const;
    DWORD DoUnlink(char driveLetter) const;
    DWORD FindDriveLetter(int diskId, int partId, char* outLetter) const;

public:
    VirtualDKManager() = default;
    ~VirtualDKManager() = default;
    VirtualDKManager(const VirtualDKManager&) = delete;
    VirtualDKManager& operator=(const VirtualDKManager&) = delete;
    void ListAll() const;
    int Mount(int diskId, int partId, char driveLetter = 0) const;
    int Unmount(int diskId = -1, int partId = -1, char driveLetter = 0) const;
};