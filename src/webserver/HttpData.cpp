//
// Created by Yvan on 2020/9/23.
//
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "HttpData.h"
#include "Logging.h"
#include "Utils.h"
#include "Epoll.h"
const int DEFAULT_KEEP_ALIVE_TIME = 20 * 1000;  // ms
std::unordered_map<std::string, std::string> HttpData::initMIME() {
    std::unordered_map<std::string, std::string> tmp;
    tmp[".html"] = "text/html";
    tmp[".avi"] = "video/x-msvideo";
    tmp[".bmp"] = "image/bmp";
    tmp[".c"] = "text/plain";
    tmp[".doc"] = "application/msword";
    tmp[".gif"] = "image/gif";
    tmp[".gz"] = "application/x-gzip";
    tmp[".htm"] = "text/html";
    tmp[".ico"] = "image/x-icon";
    tmp[".jpg"] = "image/jpeg";
    tmp[".png"] = "image/png";
    tmp[".txt"] = "text/plain";
    tmp[".mp3"] = "audio/mp3";
    tmp["default"] = "text/html";
    return tmp;
}

std::unordered_map<std::string, std::string> HttpData::_mime = initMIME();

std::string HttpData::getMime(const std::string &suffix) {
    if (_mime.find(suffix) == _mime.end())
        return _mime["default"];
    else
        return _mime[suffix];
}

HttpData::HttpData(int connfd, const std::shared_ptr<Epoll>& holder)
        : _fd(connfd),
          _error(false),
          _connectionState(H_CONNECTED),
          _method(METHOD_GET),
          _httpVersion(HTTP_11),
          _nowReadPos(0),
          _state(STATE_PARSE_URI),
          _hState(H_START),
          _keepAlive(false),
          _holder(holder) {

}

HttpData::~HttpData() {
    close(_fd);
}

void HttpData::reset() {
    _inBuffer.clear();
    _outBuffer.clear();
    _fileName.clear();
    _path.clear();
    _nowReadPos = 0;
    _state = STATE_PARSE_URI;
    _hState = H_START;
    _headers.clear();
    _keepAlive = false;
    timerDetach();
}

void HttpData::timerLink(const std::shared_ptr<TimerNode> &timer) {
    _timer = timer;
}

void HttpData::timerDetach() {
    if (_timer.lock()) {
        std::shared_ptr<TimerNode> my_timer(_timer.lock());
        my_timer->clearRequest();
        _timer.reset();
    }
}

void HttpData::handleRequest() {
    do {
        bool zero = false;
        int read_num = readn(_fd, _inBuffer, zero);
        LOG << "Request: " << _inBuffer;
        if (_connectionState == H_DISCONNECTING) {
            _inBuffer.clear();
            break;
        }
        if (read_num < 0) {
            perror("1");
            _error = true;
            handleError(_fd, 400, "Bad Request");
            break;
        } else if (zero) {
            // 有请求出现但是读不到数据，可能是Request
            // Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            // _error = true;
            _connectionState = H_DISCONNECTING;
            if (read_num == 0) {
                // _error = true;
                break;
            }
        }

        if (_state == STATE_PARSE_URI) {
            URIState flag = parseURI();
            if (flag == PARSE_URI_AGAIN)
                break;
            else if (flag == PARSE_URI_ERROR) {
                perror("2");
                LOG << "FD = " << _fd << "," << _inBuffer << "******";
                _inBuffer.clear();
                _error = true;
                handleError(_fd, 400, "Bad Request");
                break;
            } else
                _state = STATE_PARSE_HEADERS;
        }
        if (_state == STATE_PARSE_HEADERS) {
            HeaderState flag = parseHeaders();
            if (flag == PARSE_HEADER_AGAIN)
                break;
            else if (flag == PARSE_HEADER_ERROR) {
                perror("3");
                _error = true;
                handleError(_fd, 400, "Bad Request");
                break;
            }
            if (_method == METHOD_POST) {
                // POST方法准备
                _state = STATE_RECV_BODY;
            } else {
                _state = STATE_ANALYSIS;
            }
        }
        if (_state == STATE_RECV_BODY) {
            int content_length = -1;
            if (_headers.find("Content-length") != _headers.end()) {
                content_length = stoi(_headers["Content-length"]);
            } else {
                _error = true;
                handleError(_fd, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (static_cast<int>(_inBuffer.size()) < content_length) {
                break;
            }
            _state = STATE_ANALYSIS;
        }
        if (_state == STATE_ANALYSIS) {
            AnalysisState flag = analysisRequest();
            if (flag == ANALYSIS_SUCCESS) {
                _state = STATE_FINISH;
                break;
            } else {
                _error = true;
                break;
            }
        }
    } while (false);
    if (!_error) {
        if (!_outBuffer.empty()) {
            //Write
            if (!_error && _connectionState != H_DISCONNECTED) {
                if (writen(_fd, _outBuffer) < 0) {
                    perror("writen");
                    _error = true;
                }
            }
            // _events |= EPOLLOUT;
        }
        // _error may change
        if (!_error && _state == STATE_FINISH) {
            reset();
            if (!_inBuffer.empty()) {
                if (_connectionState != H_DISCONNECTING) handleRequest();
            }
            if ((_keepAlive || !_inBuffer.empty()) && _connectionState ==
                                                      H_CONNECTED) {
                reset();
            }
        }

    }
}


URIState HttpData::parseURI() {
    std::string &str = _inBuffer;
    // 读到完整的请求行再开始解析请求
    size_t pos = str.find('\r', _nowReadPos);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    std::string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else
        str.clear();
    // Method
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");

    if (posGet >= 0) {
        pos = posGet;
        _method = METHOD_GET;
    } else if (posPost >= 0) {
        pos = posPost;
        _method = METHOD_POST;
    } else if (posHead >= 0) {
        pos = posHead;
        _method = METHOD_HEAD;
    } else {
        return PARSE_URI_ERROR;
    }

    // filename
    pos = request_line.find('/', pos);
    if (pos < 0) {
        _fileName = "index.html";
        _httpVersion = HTTP_11;
        return PARSE_URI_SUCCESS;
    } else {
        size_t _pos = request_line.find(' ', pos);
        if (_pos < 0)
            return PARSE_URI_ERROR;
        else {
            if (_pos - pos > 1) {
                _fileName = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = _fileName.find('?');
                if (__pos >= 0) {
                    _fileName = _fileName.substr(0, __pos);
                }
            } else
                _fileName = "index.html";
        }
        pos = _pos;
    }
    // HTTP 版本号
    pos = request_line.find('/', pos);
    if (pos < 0)
        return PARSE_URI_ERROR;
    else {
        if (request_line.size() - pos <= 3)
            return PARSE_URI_ERROR;
        else {
            std::string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                _httpVersion = HTTP_10;
            else if (ver == "1.1")
                _httpVersion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    return PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders() {
    std::string &str = _inBuffer;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i = 0;
    for (; i < str.size() && notFinish; ++i) {
        switch (_hState) {
            case H_START: {
                if (str[i] == '\n' || str[i] == '\r') break;
                _hState = H_KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case H_KEY: {
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
                    _hState = H_COLON;
                } else if (str[i] == '\n' || str[i] == '\r')
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_COLON: {
                if (str[i] == ' ') {
                    _hState = H_SPACES_AFTER_COLON;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_SPACES_AFTER_COLON: {
                _hState = H_VALUE;
                value_start = i;
                break;
            }
            case H_VALUE: {
                if (str[i] == '\r') {
                    _hState = H_CR;
                    value_end = i;
                    if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
                } else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_CR: {
                if (str[i] == '\n') {
                    _hState = H_LF;
                    std::string key(str.begin() + key_start, str.begin() + key_end);
                    std::string value(str.begin() + value_start, str.begin() + value_end);
                    _headers[key] = value;
                    now_read_line_begin = i;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_LF: {
                if (str[i] == '\r') {
                    _hState = H_END_CR;
                } else {
                    key_start = i;
                    _hState = H_KEY;
                }
                break;
            }
            case H_END_CR: {
                if (str[i] == '\n') {
                    _hState = H_END_LF;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_END_LF: {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }
    if (_hState == H_END_LF) {
        str = str.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

AnalysisState HttpData::analysisRequest() {
    if (_method == METHOD_POST) {
        // ------------------------------------------------------
        // My CV stitching handler which requires OpenCV library
        // ------------------------------------------------------
        // string header;
        // header += string("HTTP/1.1 200 OK\r\n");
        // if(_headers.find("Connection") != _headers.end() &&
        // _headers["Connection"] == "Keep-Alive")
        // {
        //     keepAlive_ = true;
        //     header += string("Connection: Keep-Alive\r\n") + "Keep-Alive:
        //     timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        // }
        // int length = stoi(_headers["Content-length"]);
        // vector<char> data(inBuffer_.begin(), inBuffer_.begin() + length);
        // Mat src = imdecode(data, CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_ANYCOLOR);
        // //imwrite("receive.bmp", src);
        // Mat res = stitch(src);
        // vector<uchar> data_encode;
        // imencode(".png", res, data_encode);
        // header += string("Content-length: ") + to_string(data_encode.size()) +
        // "\r\n\r\n";
        // _outBuffer += header + string(data_encode.begin(), data_encode.end());
        // inBuffer_ = inBuffer_.substr(length);
        // return ANALYSIS_SUCCESS;
    } else if (_method == METHOD_GET || _method == METHOD_HEAD) {
        std::string header;
        header += "HTTP/1.1 200 OK\r\n";
        if (_headers.find("Connection") != _headers.end() &&
            (_headers["Connection"] == "Keep-Alive" ||
             _headers["Connection"] == "keep-alive")) {
            _keepAlive = true;
            header += std::string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" +
                      std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        int dot_pos = _fileName.find('.');
        std::string filetype;
        if (dot_pos < 0)
            filetype = getMime("default");
        else
            filetype = getMime(_fileName.substr(dot_pos));

        // echo test
        if (_fileName == "hello") {
            _outBuffer =
                    "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return ANALYSIS_SUCCESS;
        }

        struct stat sbuf{};
        if (stat(_fileName.c_str(), &sbuf) < 0) {
            header.clear();
            handleError(_fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        header += "Content-Type: " + filetype + "\r\n";
        header += "Content-Length: " + std::to_string(sbuf.st_size) + "\r\n";
        header += "Server: Simple Web Server\r\n";
        // 头部结束
        header += "\r\n";
        _outBuffer += header;

        if (_method == METHOD_HEAD) {
            return ANALYSIS_SUCCESS;
        }

        int src_fd = open(_fileName.c_str(), O_RDONLY, 0);
        if (src_fd < 0) {
            _outBuffer.clear();
            handleError(_fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        void *mmapRet = mmap(nullptr, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
        close(src_fd);
        if (mmapRet == (void *) -1) {
            munmap(mmapRet, sbuf.st_size);
            _outBuffer.clear();
            handleError(_fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        char *src_addr = static_cast<char *>(mmapRet);
        _outBuffer += std::string(src_addr, src_addr + sbuf.st_size);;
        munmap(mmapRet, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    }
    return ANALYSIS_ERROR;
}

void HttpData::handleError(int fd, int err_num, std::string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[4096];
    std::string body_buff, header_buff;
    body_buff += "<html><title>Error occurred</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em> LinYa's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "Server: Simple Web Server\r\n";;
    header_buff += "\r\n";
    // 错误处理不考虑writen不完的情况
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

void HttpData::handleClose() {
    _connectionState = H_DISCONNECTED;
    if (_holder.lock()) {
        std::shared_ptr<Epoll> holder(_holder.lock());
        holder->epoll_del(_fd,  (EPOLLIN | EPOLLET | EPOLLONESHOT));
    }
}





