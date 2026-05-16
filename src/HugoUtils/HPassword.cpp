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
#ifndef HU_DISABLE_PASSWORD

#include <Windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>
#include <string>
#include <fstream>

#include "hashlib/sha256.h"
#include "hashlib/md5.h"

#include "HugoUtils/HPassword.h"
#include "HugoUtils/HInfo.h"
#include "WinUtils/StrConvert.h"
using namespace std;
using namespace WinUtils;

// -------------------- Helper functions --------------------
static string PreComputeHugoMd5() {
	MD5 md5;
	return md5("hugo");
}

static void HexToBin(const string& hex, uint8_t* bin, int bin_len) {
	for (int i = 0; i < bin_len; i++) {
		sscanf_s(hex.c_str() + 2 * i, "%2hhx", &bin[i]);
	}
}

// -------------------- AutoInfoAcquirer Implementation --------------------
std::vector<CrackTask> AutoInfoAcquirer::acquire() {
    std::vector<CrackTask> tasks;
    std::string device_id, machine_id;

    if (auto machineIdOpt = HInfo::GetMachineId()) {
        machine_id = machineIdOpt.value();
    }

    for (auto adminFilePath : { HInfo::GetSeewoCoreIniPath(), HInfo::GetSeewoSchoolFilePath() }) {
        if (!adminFilePath) continue;
        std::ifstream file(adminFilePath->string());
        if (!file.is_open()) continue;

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (device_id.empty() && line.find("id=") == 0) {
                device_id = line.substr(3);
            }
            static const std::pair<const char*, CrackMode> adminPrefixes[] = {
                {"PASSWORDV1=", MODE_V1},
                {"PASSWORDV2=", MODE_V2},
                {"PASSWORDV3=", MODE_V3}
            };
            for (const auto& [prefix, mode] : adminPrefixes) {
                if (line.find(prefix) == 0) {
                    std::string pwd = line.substr(strlen(prefix));
                    if (!pwd.empty()) {
                        tasks.push_back({ mode, TYPE_ADMIN, pwd, device_id, "", adminFilePath->string() });
                    }
                    break;
                }
            }
        }
    }

    for (auto lockConfigPath : { HInfo::GetLockConfigIniPath(), HInfo::GetLockConfigIniPath2() }) {
        if (!lockConfigPath) continue;
        std::ifstream file(lockConfigPath->string());
        if (!file.is_open()) continue;

        std::string line;
        std::string lock_pwds[3];
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.find("LockPasswardV1=") == 0) {
                lock_pwds[0] = line.substr(15);
            }
            else if (line.find("LockPasswardV2=") == 0) {
                lock_pwds[1] = line.substr(15);
            }
            else if (line.find("LockPasswardV3=") == 0) {
                lock_pwds[2] = line.substr(15);
            }
        }

        static const CrackMode lock_modes[] = { MODE_V1, MODE_V2, MODE_V3 };
        for (int i = 0; i < 3; ++i) {
            if (!lock_pwds[i].empty()) {
                tasks.push_back({ lock_modes[i], TYPE_LOCK, lock_pwds[i], device_id, "", lockConfigPath->string() });
            }
        }
    }

    for (auto& task : tasks) {
        if (task.mode == MODE_V3) {
            task.machine_id = machine_id;
			task.device_id = device_id;
        }
    }

    return tasks;
}

// -------------------- V1Decryptor Implementation --------------------
string V1Decryptor::bruteForce(const string& target_hash) {
	char candidate[7] = { 0 };
	for (int i = 0; i < PASSWORD_MAX; ++i) {
		snprintf(candidate, sizeof(candidate), "%06d", i);
		MD5 md5;
		string md5_hex = md5(candidate);
		string truncated = md5_hex.substr(8, MD5_TRUNCATED_LEN);
		if (truncated == target_hash) {
			return string(candidate);
		}
	}
	return "";
}

bool V1Decryptor::canHandle(const CrackTask& task) const {
	return task.mode == MODE_V1;
}

string V1Decryptor::decrypt(const CrackTask& task) {
	if (task.ciphertext.length() != MD5_TRUNCATED_LEN) {
		return "";
	}
	return bruteForce(task.ciphertext);
}

bool V1Decryptor::matched(const CrackTask&task)
{
    if (task.ciphertext.length() != MD5_TRUNCATED_LEN) {
        return false;
    }
    MD5 md5;
    std::string md5_hex = md5(task.plaintext);
    std::string truncated = md5_hex.substr(8, MD5_TRUNCATED_LEN);
    if (truncated == task.ciphertext) {
        return true;
    }
    return false;
}

// -------------------- V2Decryptor Implementation --------------------
V2Decryptor::V2Decryptor() : hugo_md5(PreComputeHugoMd5()) {}

string V2Decryptor::bruteForce(const string& target_hash) {
	char candidate[7] = { 0 };
	for (int i = 0; i < PASSWORD_MAX; ++i) {
		snprintf(candidate, sizeof(candidate), "%06d", i);
		MD5 md5_pwd;
		string pwd_md5 = md5_pwd(candidate);
		string combined = pwd_md5 + hugo_md5;
		MD5 md5_combined;
		string result_md5 = md5_combined(combined);
		string truncated = result_md5.substr(8, MD5_TRUNCATED_LEN);
		if (truncated == target_hash) {
			return string(candidate);
		}
	}
	return "";
}

bool V2Decryptor::canHandle(const CrackTask& task) const {
	return task.mode == MODE_V2;
}

string V2Decryptor::decrypt(const CrackTask& task) {
	if (task.ciphertext.length() != MD5_TRUNCATED_LEN) {
		return "";
	}
	return bruteForce(task.ciphertext);
}

bool V2Decryptor::matched(const CrackTask&task)
{
    if (task.ciphertext.length() != MD5_TRUNCATED_LEN) {
        return false;
    }
    MD5 md5_pwd;
    std::string pwd_md5 = md5_pwd(task.plaintext);
    std::string combined = pwd_md5 + hugo_md5;
    MD5 md5_combined;
    std::string result_md5 = md5_combined(combined);
    std::string truncated = result_md5.substr(8, MD5_TRUNCATED_LEN);
    if (truncated == task.ciphertext) {
        return true;
    }
    return false;
}

// -------------------- V3Decryptor Implementation --------------------
string V3Decryptor::bruteForce(const string& salt, const string& target_hex) {
	uint8_t target_bin[HASH_BIN_LEN];
	HexToBin(target_hex, target_bin, HASH_BIN_LEN);

	char candidate[7] = { 0 };
	unsigned char hash_bin[HASH_BIN_LEN];

	for (int i = 0; i < PASSWORD_MAX; ++i) {
		snprintf(candidate, sizeof(candidate), "%06d", i);
		string payload = string(candidate) + salt;

		SHA256 sha;
		sha.add(payload.c_str(), payload.length());
		sha.getHash(hash_bin);

		if (memcmp(hash_bin, target_bin, HASH_BIN_LEN) == 0) {
			return string(candidate);
		}
	}
	return "";
}

bool V3Decryptor::canHandle(const CrackTask& task) const {
	return task.mode == MODE_V3;
}

string V3Decryptor::decrypt(const CrackTask& task) {
	if (task.ciphertext.length() != PASS_V3_VALID_LEN) {
		return "";
	}
	if (task.device_id.empty()) {
		return "";
	}

	string partA = task.ciphertext.substr(0, 16);
	string partB = task.ciphertext.substr(32, 64);
	string salt = "@" + partA + "!" + task.device_id + "&" + task.machine_id + "^mf-hu90";

	return bruteForce(salt, partB);
}

bool V3Decryptor::matched(const CrackTask&task)
{
    if (task.ciphertext.length() != PASS_V3_VALID_LEN) {
        return false;
    }
    if (task.device_id.empty()) {
        return false;
    }
    std::string partA = task.ciphertext.substr(0, 16);
    std::string target_hex = task.ciphertext.substr(32, 64);
    std::string salt = "@" + partA + "!" + task.device_id + "&" + task.machine_id + "^mf-hu90";
    std::string payload = std::string(task.plaintext) + salt;
    SHA256 sha;
    unsigned char hash_bin[HASH_BIN_LEN];
    uint8_t target_bin[HASH_BIN_LEN];
    HexToBin(target_hex, target_bin, HASH_BIN_LEN);
    sha.add(payload.c_str(), payload.length());
    sha.getHash(hash_bin);
    if (memcmp(hash_bin, target_bin, HASH_BIN_LEN) == 0) {
        return true;
    }
    return false;
}

#endif // !HU_DISABLE_PASSWORD
