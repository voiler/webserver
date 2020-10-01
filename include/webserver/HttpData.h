//
// Created by Yvan on 2020/9/23.
//

#pragma once

#include "Timer.h"
#include <unordered_map>
#include <map>


class Epoll;

enum ProcessState {
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};

enum URIState {
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};

enum HeaderState {
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum AnalysisState {
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR
};

enum ParseState {
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum ConnectionState {
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED
};

enum HttpMethod {
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};

enum HttpVersion {
    HTTP_10 = 1,
    HTTP_11
};


// C++11 新特性enable_shared_from_this
// std::enable_shared_from_this 能让一个对象（假设其名为 t ，且已被一个 std::shared_ptr 对象 pt 管理）安全地生成其他额外的
// std::shared_ptr 实例，它们与 pt 共享对象 t 的所有权。
class HttpData : public std::enable_shared_from_this<HttpData> {
public:
    explicit HttpData(int fd, const std::shared_ptr<Epoll> &holder);

    ~HttpData();


    void reset();

    void timerLink(const std::shared_ptr<TimerNode> &timer);

    void timerDetach();

    void handleClose();

    void handleRequest();

private:
    std::string getMime(const std::string &suffix);

    static std::unordered_map<std::string, std::string> initMIME();

    void handleError(int fd, int err_num, std::string short_msg);

    URIState parseURI();

    HeaderState parseHeaders();

    AnalysisState analysisRequest();

    int _fd;
    std::string _inBuffer;
    std::string _outBuffer;
    bool _error;
    ConnectionState _connectionState;
    std::weak_ptr<Epoll> _holder;
    HttpMethod _method;
    HttpVersion _httpVersion;
    std::string _fileName;
    std::string _path;
    int _nowReadPos;
    ProcessState _state;
    ParseState _hState;
    bool _keepAlive;
    std::map<std::string, std::string> _headers;
    std::weak_ptr<TimerNode> _timer;
    static std::unordered_map<std::string, std::string> _mime;
};

using HttpDataPtr = std::shared_ptr<HttpData>;