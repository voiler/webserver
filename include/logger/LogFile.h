//
// Created by Yvan on 2020/9/23.
//

#pragma once
#include <memory>
#include <mutex>
#include "FileUtil.h"
class LogFile : NonCopyAble {
public:
    explicit LogFile(std::string basename, int flushEveryN = 1024);

    ~LogFile() = default;

    void append(const char *logline, int len);
    void flush();

private:
    void append_unlocked(const char *logline, int len);
    const std::string _basename;
    const int _flushEveryN;
    int _count;
    std::mutex _mutex;
    std::unique_ptr<AppendFile> _file;
};