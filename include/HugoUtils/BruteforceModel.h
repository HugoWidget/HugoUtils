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
#include "HugoUtils/HugoUtilsDef.h"
#ifndef HU_DISABLE_PASSWORD
#include <string>
#include <vector>
#include <cstdint>

enum CrackMode { MODE_V1, MODE_V2, MODE_V3 };
enum PasswordType { TYPE_ADMIN, TYPE_LOCK };

struct CrackTask {
    CrackMode mode;
    PasswordType type;
    std::string ciphertext;      
    std::string device_id;       
    std::string machine_id;
};

struct CrackResult {
    CrackTask task;
    bool success;
    std::string plaintext;
    std::string error_message;
};

class InfoAcquirer {
public:
    virtual ~InfoAcquirer() = default;
    virtual std::vector<CrackTask> acquire() = 0;
};

class Decryptor {
public:
    virtual ~Decryptor() = default;
    virtual bool canHandle(const CrackTask& task) const = 0;
    // return plaintext,empty if it fails
    virtual std::string decrypt(const CrackTask& task) = 0;
};

class ResultOutput {
public:
    virtual ~ResultOutput() = default;
    virtual void output(const std::vector<CrackResult>& results) = 0;
};

class CrackExecutor {
public:
    std::vector<CrackResult> execute(const std::vector<CrackTask>& tasks,
        const std::vector<Decryptor*>& decryptors);
};
#endif // !HU_DISABLE_PASSWORD