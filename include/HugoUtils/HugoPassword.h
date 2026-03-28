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

#include <string>
#include <vector>
#include "HugoUtils/BruteforceModel.h"   // Contains definitions for CrackTask, CrackMode, PasswordType, InfoAcquirer, Decryptor, etc.

// Constants
const int PASSWORD_MAX = 1000000;          // Total number of 6-digit passwords
const int PASSWORD_BUF_LEN = 7;             // Password string buffer size
const int MD5_TRUNCATED_LEN = 16;           // MD5 truncation length ([8:24])
const int PASS_V3_VALID_LEN = 96;            // V3 ciphertext length
const int HASH_BIN_LEN = 32;                 // SHA256 binary length

// Acquirer classes
class AutoInfoAcquirer : public InfoAcquirer {
public:
    std::vector<CrackTask> acquire() override;
};


// Decryptor classes
class V1Decryptor : public Decryptor {
private:
    std::string bruteForce(const std::string& target_hash);
public:
    bool canHandle(const CrackTask& task) const override;
    std::string decrypt(const CrackTask& task) override;
};

class V2Decryptor : public Decryptor {
private:
    std::string hugo_md5;
    std::string bruteForce(const std::string& target_hash);
public:
    V2Decryptor();
    bool canHandle(const CrackTask& task) const override;
    std::string decrypt(const CrackTask& task) override;
};

class V3Decryptor : public Decryptor {
private:
    std::string bruteForce(const std::string& salt, const std::string& target_hex);
public:
    bool canHandle(const CrackTask& task) const override;
    std::string decrypt(const CrackTask& task) override;
};