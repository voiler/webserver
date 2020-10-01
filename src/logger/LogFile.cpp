//
// Created by Yvan on 2020/9/23.
//
#include "LogFile.h"

LogFile::LogFile(std::string basename, int flushEveryN)
        : _basename(basename),
          _flushEveryN(flushEveryN),
          _count(0) {
    _file.reset(new AppendFile(basename));
}

void LogFile::append(const char *logline, int len) {
    std::lock_guard<std::mutex> lockGuard(_mutex);
    append_unlocked(logline, len);
}

void LogFile::flush() {
    std::lock_guard<std::mutex> lockGuard(_mutex);
    _file->flush();
}

void LogFile::append_unlocked(const char *logline, int len) {
    _file->append(logline, len);
    ++_count;
    if (_count >= _flushEveryN) {
        _count = 0;
        _file->flush();
    }
}

