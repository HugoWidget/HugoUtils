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

#include "HugoUtils/BruteforceModel.h"

std::vector<CrackResult> CrackExecutor::execute(const std::vector<CrackTask>& tasks,
    const std::vector<Decryptor*>& decryptors) {
    std::vector<CrackResult> results;
    for (const auto& task : tasks) {
        CrackResult result{ task, false, "", "" };
        for (auto* d : decryptors) {
            if (d->canHandle(task)) {
                std::string plain = d->decrypt(task);
                if (!plain.empty()) {
                    result.success = true;
                    result.plaintext = plain;
                    break;
                }
            }
        }
        if (!result.success) {
            result.error_message = "Failed to crack the password.";
        }
        results.push_back(result);
    }
    return results;
}
#endif // !HU_DISABLE_PASSWORD