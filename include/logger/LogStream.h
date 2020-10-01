//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include "NonCopyAble.h"
#include <string>

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template<int SIZE>
class FixedBuffer : NonCopyAble {
public:
    FixedBuffer();

    ~FixedBuffer() = default;

    void append(const char *buf, size_t len);

    const char *data() const;

    int length() const;

    char *current();

    int avail() const;

    void add(size_t len);

    void reset();

    void bzero();

private:
    const char *end() const;

    char _data[SIZE]{};
    char *_cur;
};

class LogStream : NonCopyAble {
public:
    using Buffer = FixedBuffer<kSmallBuffer>;

    LogStream &operator<<(bool v);

    LogStream &operator<<(short);

    LogStream &operator<<(unsigned short);

    LogStream &operator<<(int);

    LogStream &operator<<(unsigned int);

    LogStream &operator<<(long);

    LogStream &operator<<(unsigned long);

    LogStream &operator<<(long long);

    LogStream &operator<<(unsigned long long);

    LogStream &operator<<(float v);

    LogStream &operator<<(double);

    LogStream &operator<<(long double);

    LogStream &operator<<(char v);

    LogStream &operator<<(const char *str);

    LogStream &operator<<(const unsigned char *str);

    LogStream &operator<<(const std::string &v);
    const Buffer &buffer() const ;

private:
    template<typename T>
    void formatInteger(T);

    Buffer _buffer;
    static const int kMaxNumericSize = 32;
};