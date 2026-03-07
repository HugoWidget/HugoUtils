#pragma once
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