//
// Created by Yvan on 2020/9/23.
//
#include "Logging.h"
#include "AsyncLogging.h"
#include <sys/time.h>

static AsyncLogging * asyncLogger;
std::string Logger::_logFileName = "./WebServer.log";
void once_init() {
    asyncLogger = new AsyncLogging(Logger::getLogFileName());
}

void output(const char *msg, int len) {
    static std::once_flag once_flag;
    // 保证在多线程环境下，只调用GetMaxOpenFileSys一次
    // 也就是说保证只调用::getrlimit一次
    std::call_once(once_flag, once_init);
    asyncLogger->append(msg, len);
}

Logger::Impl::Impl(const char *fileName, int line)
        : _stream(),
          _line(line),
          _basename(fileName) {
    formatTime();
}

void Logger::Impl::formatTime() {
    struct timeval tv{};
    time_t time;
    char str_t[26] = {0};
    gettimeofday(&tv, nullptr);
    time = tv.tv_sec;
    struct tm *p_time = localtime(&time);
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    _stream << str_t;
}

Logger::Logger(const char *fileName, int line)
        : _impl(fileName, line) {}

Logger::~Logger() {
    _impl._stream << " -- " << _impl._basename << ':' << _impl._line << '\n';
    const LogStream::Buffer &buf(stream().buffer());
    output(buf.data(), buf.length());
}

LogStream &Logger::stream() { return _impl._stream; }

void Logger::setLogFileName(std::string fileName) { _logFileName = std::move(fileName); }

std::string Logger::getLogFileName()  { return _logFileName; }
