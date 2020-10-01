//
// Created by Yvan on 2020/9/23.
//
#include "LogStream.h"
#include <cstring>
#include <algorithm>

const char digits[] = "9876543210123456789";
const char *zero = digits + 9;

template<int SIZE>
FixedBuffer<SIZE>::FixedBuffer() : _cur(_data) {}

template<int SIZE>
const char *FixedBuffer<SIZE>::end() const { return _data + sizeof(_data); }

template<int SIZE>
int FixedBuffer<SIZE>::avail() const { return static_cast<int>(end() - _cur); }

template<int SIZE>
void FixedBuffer<SIZE>::append(const char *buf, size_t len) {
    if (avail() > static_cast<int>(len)) {
        memcpy(_cur, buf, len);
        _cur += len;
    }
}

template<int SIZE>
const char *FixedBuffer<SIZE>::data() const {
    return _data;
}

template<int SIZE>
int FixedBuffer<SIZE>::length() const {
    return static_cast<int>(_cur - _data);
}

template<int SIZE>
char *FixedBuffer<SIZE>::current() {
    return _cur;
}

template<int SIZE>
void FixedBuffer<SIZE>::add(size_t len) {
    _cur += len;
}

template<int SIZE>
void FixedBuffer<SIZE>::reset() {
    _cur = _data;
}

template<int SIZE>
void FixedBuffer<SIZE>::bzero() {
    memset(_data, 0, sizeof(_data));
}


template<typename T>
size_t convert(char buf[], T value) {
    T num = value;
    char *p = buf;

    do {
        int lsd = static_cast<int>(num % 10);
        num /= 10;
        *p++ = zero[lsd];
    } while (num != 0);

    if (value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

template
class FixedBuffer<kSmallBuffer>;

template
class FixedBuffer<kLargeBuffer>;

template<typename T>
void LogStream::formatInteger(T v) {
    // buffer容不下kMaxNumericSize个字符的话会被直接丢弃
    if (_buffer.avail() >= kMaxNumericSize) {
        size_t len = convert(_buffer.current(), v);
        _buffer.add(len);
    }
}

LogStream &LogStream::operator<<(bool v) {
    _buffer.append(v ? "1" : "0", 1);
    return *this;
}

LogStream &LogStream::operator<<(short v) {
    *this << static_cast<int>(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream &LogStream::operator<<(int v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long long v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long long v) {
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
}

LogStream &LogStream::operator<<(double v) {
    if (_buffer.avail() >= kMaxNumericSize) {
        int len = snprintf(_buffer.current(), kMaxNumericSize, "%.12g", v);
        _buffer.add(len);
    }
    return *this;
}

LogStream &LogStream::operator<<(long double v) {
    if (_buffer.avail() >= kMaxNumericSize) {
        int len = snprintf(_buffer.current(), kMaxNumericSize, "%.12Lg", v);
        _buffer.add(len);
    }
    return *this;
}

LogStream &LogStream::operator<<(char v) {
    _buffer.append(&v, 1);
    return *this;
}

LogStream &LogStream::operator<<(const char *str) {
    if (str)
        _buffer.append(str, strlen(str));
    else
        _buffer.append("(null)", 6);
    return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str) {
    return operator<<(reinterpret_cast<const char *>(str));
}

LogStream &LogStream::operator<<(const std::string &v) {
    _buffer.append(v.c_str(), v.size());
    return *this;
}

const LogStream::Buffer &LogStream::buffer() const {
    return _buffer;
}
