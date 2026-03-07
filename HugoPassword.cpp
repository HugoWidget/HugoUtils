#include <Windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iostream>

#include "hashlib/sha256.h"
#include "hashlib/md5.h"
#include "HugoPassword.h"

// -------------------- Helper functions --------------------
static std::string PreComputeHugoMd5() {
    MD5 md5;
    return md5("hugo");
}

static void HexToBin(const std::string& hex, uint8_t* bin, int bin_len) {
    for (int i = 0; i < bin_len; i++) {
        sscanf_s(hex.c_str() + 2 * i, "%2hhx", &bin[i]);
    }
}

// -------------------- AutoInfoAcquirer Implementation --------------------
std::vector<CrackTask> AutoInfoAcquirer::acquire() {
    std::vector<CrackTask> tasks;
    std::string device_id, machine_id;
    char seewocore_ini_path[MAX_PATH] = { 0 };
    char lock_ini_path[MAX_PATH] = { 0 };

    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, seewocore_ini_path))) {
        strcat_s(seewocore_ini_path, MAX_PATH, "\\Seewo\\SeewoCore\\SeewoCore.ini");

        char buf_device[256] = { 0 };
        char buf_admin_v1[100] = { 0 };
        char buf_admin_v2[100] = { 0 };
        char buf_admin_v3[100] = { 0 };
        GetPrivateProfileStringA("device", "id", "", buf_device, sizeof(buf_device), seewocore_ini_path);
        GetPrivateProfileStringA("ADMIN", "PASSWORDV1", "", buf_admin_v1, sizeof(buf_admin_v1), seewocore_ini_path);
        GetPrivateProfileStringA("ADMIN", "PASSWORDV2", "", buf_admin_v2, sizeof(buf_admin_v2), seewocore_ini_path);
        GetPrivateProfileStringA("ADMIN", "PASSWORDV3", "", buf_admin_v3, sizeof(buf_admin_v3), seewocore_ini_path);

        device_id = buf_device;

        if (strlen(buf_admin_v1) > 0)
            tasks.push_back({ MODE_V1, TYPE_ADMIN, buf_admin_v1, device_id, "" });
        if (strlen(buf_admin_v2) > 0)
            tasks.push_back({ MODE_V2, TYPE_ADMIN, buf_admin_v2, device_id, "" });
        if (strlen(buf_admin_v3) > 0)
            tasks.push_back({ MODE_V3, TYPE_ADMIN, buf_admin_v3, device_id, machine_id });
    }

    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, lock_ini_path))) {
        strcat_s(lock_ini_path, MAX_PATH, "\\seewo\\SeewoAbility\\SeewoLockConfig.ini");

        FILE* fp = nullptr;
        errno_t err = fopen_s(&fp, lock_ini_path, "r");
        if (err == 0 && fp) {
            char line[256] = { 0 };
            std::string lock_v1, lock_v2, lock_v3;
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "LockPasswardV1=", 15) == 0) {
                    lock_v1 = line + 15;
                    lock_v1.erase(lock_v1.find_last_not_of("\r\n") + 1);
                }
                else if (strncmp(line, "LockPasswardV2=", 15) == 0) {
                    lock_v2 = line + 15;
                    lock_v2.erase(lock_v2.find_last_not_of("\r\n") + 1);
                }
                else if (strncmp(line, "LockPasswardV3=", 15) == 0) {
                    lock_v3 = line + 15;
                    lock_v3.erase(lock_v3.find_last_not_of("\r\n") + 1);
                }
            }
            fclose(fp);

            if (!lock_v1.empty())
                tasks.push_back({ MODE_V1, TYPE_LOCK, lock_v1, device_id, "" });
            if (!lock_v2.empty())
                tasks.push_back({ MODE_V2, TYPE_LOCK, lock_v2, device_id, "" });
            if (!lock_v3.empty())
                tasks.push_back({ MODE_V3, TYPE_LOCK, lock_v3, device_id, machine_id });
        }
    }

    // Read machine ID (for V3 tasks)
    char buffer[128] = { 0 };
    DWORD buffer_size = sizeof(buffer);
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\SQMClient",
        0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    if (result != ERROR_SUCCESS) {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\SQMClient",
            0, KEY_READ, &hKey);
    }
    if (result == ERROR_SUCCESS) {
        result = RegQueryValueExA(hKey, "MachineId", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(buffer), &buffer_size);
        RegCloseKey(hKey);
        if (result == ERROR_SUCCESS) {
            machine_id = buffer;
        }
    }

    for (auto& task : tasks) {
        if (task.mode == MODE_V3) {
            task.machine_id = machine_id;
        }
    }

    return tasks;
}

// -------------------- V1Decryptor Implementation --------------------
std::string V1Decryptor::bruteForce(const std::string& target_hash) {
    char candidate[7] = { 0 };
    for (int i = 0; i < PASSWORD_MAX; ++i) {
        snprintf(candidate, sizeof(candidate), "%06d", i);
        MD5 md5;
        std::string md5_hex = md5(candidate);
        std::string truncated = md5_hex.substr(8, MD5_TRUNCATED_LEN);
        if (truncated == target_hash) {
            return std::string(candidate);
        }
    }
    return "";
}

bool V1Decryptor::canHandle(const CrackTask& task) const {
    return task.mode == MODE_V1;
}

std::string V1Decryptor::decrypt(const CrackTask& task) {
    if (task.ciphertext.length() != MD5_TRUNCATED_LEN) {
        return "";
    }
    return bruteForce(task.ciphertext);
}

// -------------------- V2Decryptor Implementation --------------------
V2Decryptor::V2Decryptor() : hugo_md5(PreComputeHugoMd5()) {}

std::string V2Decryptor::bruteForce(const std::string& target_hash) {
    char candidate[7] = { 0 };
    for (int i = 0; i < PASSWORD_MAX; ++i) {
        snprintf(candidate, sizeof(candidate), "%06d", i);
        MD5 md5_pwd;
        std::string pwd_md5 = md5_pwd(candidate);
        std::string combined = pwd_md5 + hugo_md5;
        MD5 md5_combined;
        std::string result_md5 = md5_combined(combined);
        std::string truncated = result_md5.substr(8, MD5_TRUNCATED_LEN);
        if (truncated == target_hash) {
            return std::string(candidate);
        }
    }
    return "";
}

bool V2Decryptor::canHandle(const CrackTask& task) const {
    return task.mode == MODE_V2;
}

std::string V2Decryptor::decrypt(const CrackTask& task) {
    if (task.ciphertext.length() != MD5_TRUNCATED_LEN) {
        return "";
    }
    return bruteForce(task.ciphertext);
}

// -------------------- V3Decryptor Implementation --------------------
std::string V3Decryptor::bruteForce(const std::string& salt, const std::string& target_hex) {
    uint8_t target_bin[HASH_BIN_LEN];
    HexToBin(target_hex, target_bin, HASH_BIN_LEN);

    char candidate[7] = { 0 };
    unsigned char hash_bin[HASH_BIN_LEN];

    for (int i = 0; i < PASSWORD_MAX; ++i) {
        snprintf(candidate, sizeof(candidate), "%06d", i);
        std::string payload = std::string(candidate) + salt;

        SHA256 sha;
        sha.add(payload.c_str(), payload.length());
        sha.getHash(hash_bin);

        if (memcmp(hash_bin, target_bin, HASH_BIN_LEN) == 0) {
            return std::string(candidate);
        }
    }
    return "";
}

bool V3Decryptor::canHandle(const CrackTask& task) const {
    return task.mode == MODE_V3;
}

std::string V3Decryptor::decrypt(const CrackTask& task) {
    if (task.ciphertext.length() != PASS_V3_VALID_LEN) {
        return "";
    }
    if (task.device_id.empty()) {
        return "";
    }

    std::string partA = task.ciphertext.substr(0, 16);
    std::string partB = task.ciphertext.substr(32, 64);
    std::string salt = "@" + partA + "!" + task.device_id + "&" + task.machine_id + "^mf-hu90";

    return bruteForce(salt, partB);
}