#include "BruteforceModel.h"

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