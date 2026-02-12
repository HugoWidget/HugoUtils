#pragma once
#ifndef MD5_H
#define MD5_H

#include <stdint.h>
#include <string>
#define MD5_DIGEST_LENGTH 16
//This file will be removed in the future, use hashlib instead
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    unsigned char buffer[64];
} MD5_CTX;

void MD5_Init(MD5_CTX* context);
void MD5_Update(MD5_CTX* context, const unsigned char* input, unsigned int inputLen);
void MD5_Final(unsigned char digest[16], MD5_CTX* context);
std::string MD5_Hash(const std::string input);
#endif