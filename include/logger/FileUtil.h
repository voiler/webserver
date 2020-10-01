//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include "NonCopyAble.h"
#include <string>

class AppendFile : NonCopyAble {
public:
    explicit AppendFile(const std::string &filename);

    ~AppendFile();

    void append(const char *logline, const size_t &len);

    void flush();

private:
    size_t write(const char *logline, size_t len);

    FILE *_fp;
    char _buffer[64 * 1024]{};
};