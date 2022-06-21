/*
 * MIT License
 *
 * Copyright (c) 2019-2022 agdsdl <agdsdl@sina.com.cn>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace DLNetwork {
namespace HTTP {
std::vector<std::string> split(const char* buf, size_t bufLen, const std::string& delim) noexcept;
std::vector<std::string> split(const std::string& str, const std::string& delim) noexcept;

std::string concat(const std::vector<std::string>& strings, const std::string& delim = "") noexcept;

constexpr static std::string_view LINE_END = "\r\n";

enum class Method
{
    HTTP_UNKNOWN,
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_TRACE,
    HTTP_OPTIONS,
    HTTP_CONNECT,
    HTTP_PATCH
};

std::string to_string(Method method);

Method method_from_string(const std::string& method) noexcept;

enum class Version
{
    HTTP_UNKNOWN,
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0
};

std::string to_string(Version version);

Version version_from_string(const std::string& version) noexcept;

class Header
{
public:
    std::string key;
    std::string value;

public:
    Header() {}
    Header(std::string&& key, std::string&& value) noexcept : key(std::move(key)), value(std::move(value)) {
    }

    void set_value(const std::string& value) noexcept {
        this->value = value;
    }

    const std::string& get_key() const noexcept {
        return this->key;
    }

    std::string serialize() const noexcept {
        std::string header;
        header += this->key;
        header += ": ";
        header += this->value;
        header += LINE_END;

        return header;
    }

    static Header deserialize(const std::string& header) {
        auto pos = header.find(':');
        if (pos == -1) {
            return Header();
        }
        std::string key = header.substr(0, pos);
        std::string value = header.substr(pos + 2);
        return Header(std::move(key), std::move(value));
    }
    static std::string serialize(const std::string& key, const std::string& value) {
        std::string header;
        header += key;
        header += ": ";
        header += value;
        header += LINE_END;
        return header;
    }
};

class Request
{
public:
    Version version = Version::HTTP_UNKNOWN;
    Method method = Method::HTTP_UNKNOWN;
    std::string resource; // /path?para=val
    std::string path; // /path
    std::map<std::string, std::string> urlParams;
    std::map<std::string, std::string> headers;
    std::string body;

    enum class ErrorCode {
        OK = 0,
        RequestLineInsufficent = -1,
        RequestFirstLineError = -2
    };
public:
    Request() :version(Version::HTTP_1_1), method(Method::HTTP_GET) {}
    Request(Method method, const std::string& resource, const std::map<std::string, std::string>& headers, Version version = Version::HTTP_1_1) noexcept : version(version), method(method), resource(resource), headers(headers) {
    }

    std::string serialize() const noexcept;

    ErrorCode deserialize(const char* buf, size_t bufLen);
};

class Response
{
public:
    int responseCode = -1;
    Version version = Version::HTTP_UNKNOWN;
    std::map<std::string, std::string> headers;
    std::string body;

public:
    constexpr static int OK = 200;
    constexpr static int CREATED = 201;
    constexpr static int ACCEPTED = 202;
    constexpr static int NO_CONTENT = 203;
    constexpr static int BAD_REQUEST = 400;
    constexpr static int FORBIDDEN = 403;
    constexpr static int NOT_FOUND = 404;
    constexpr static int REQUEST_TIMEOUT = 408;
    constexpr static int INTERNAL_SERVER_ERROR = 500;
    constexpr static int BAD_GATEWAY = 502;
    constexpr static int SERVICE_UNAVAILABLE = 503;

    static std::string respCode2string(int respCode) {
        switch (respCode) {
        case OK:
            return "OK";
        case CREATED:
            return "CREATED";
        case ACCEPTED:
            return "ACCEPTED";
        case NO_CONTENT:
            return "NO_CONTENT";
        case BAD_REQUEST:
            return "BAD_REQUEST";
        case FORBIDDEN:
            return "FORBIDDEN";
        case NOT_FOUND:
            return "NOT_FOUND";
        case REQUEST_TIMEOUT:
            return "REQUEST_TIMEOUT";
        case INTERNAL_SERVER_ERROR:
            return "INTERNAL_SERVER_ERROR";
        case BAD_GATEWAY:
            return "BAD_GATEWAY";
        case SERVICE_UNAVAILABLE:
            return "SERVICE_UNAVAILABLE";
        default:
            return std::to_string(respCode);
        }
    }

    Response() :responseCode(0) {}
    Response(int responseCode, Version version, const std::map<std::string, std::string>& headers, const std::string& body) noexcept : version(version), responseCode(responseCode), headers(headers), body(body) {
    }

    int get_response_code() const noexcept {
        return this->responseCode;
    }

    const std::string& get_body() const noexcept {
        return this->body;
    }

    const std::map<std::string, std::string> get_headers() const noexcept {
        return this->headers;
    }

    std::string serialize() const noexcept;

    void deserialize(const char* buf, size_t bufLen) noexcept;
};
} //HTTP
} // ns DLNetwork