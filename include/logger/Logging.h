//
// Created by Yvan on 2020/9/23.
//

#pragma once
#include "LogStream.h"
class Logger {
public:
    Logger(const char *fileName, int line);

    ~Logger();

    LogStream &stream();

    static void setLogFileName(std::string fileName);

    static std::string getLogFileName();

private:
    class Impl {
    public:
        Impl(const char *fileName, int line);

        void formatTime();

        LogStream _stream;
        int _line;
        std::string _basename;
    };

    Impl _impl;
    static std::string _logFileName;
};

#define LOG Logger(__FILE__, __LINE__).stream()