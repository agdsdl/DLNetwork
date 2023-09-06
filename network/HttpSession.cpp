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
#include "HttpSession.h"
#include <urlcodec.h>
#include <string.h>
#include <memory>

using namespace DLNetwork;
using namespace HTTP;

std::vector<std::string> HTTP::split(const char* buf, size_t bufLen, const std::string& delim) noexcept {
    std::vector<std::string> tokens = std::vector<std::string>();
    std::string strCopy(buf, bufLen);

    std::size_t pos = 0;
    std::string token;

    while ((pos = strCopy.find(delim)) != std::string::npos) {
        token = strCopy.substr(0, pos);
        strCopy.erase(0, pos + delim.length());

        tokens.push_back(token);
    }

    if (strCopy.length() > 0) {
        tokens.push_back(strCopy);
    }

    return tokens;
}

std::vector<std::string> HTTP::split(const std::string& str, const std::string& delim) noexcept {
    return split(str.c_str(), str.size(), delim);
}

std::string HTTP::concat(const std::vector<std::string>& strings, const std::string& delim) noexcept {
    std::string result;

    for (std::size_t i = 0; i < strings.size(); i++) {
        result += strings[i];

        if ((i + 1) != strings.size()) {
            result += delim;
        }
    }

    return result;
}

std::string HTTP::to_string(Method method) {
    switch (method) {
    case Method::HTTP_GET:
        return "GET";
    case Method::HTTP_HEAD:
        return "HEAD";
    case Method::HTTP_POST:
        return "POST";
    case Method::HTTP_PUT:
        return "PUT";
    case Method::HTTP_DELETE:
        return "DELETE";
    case Method::HTTP_TRACE:
        return "TRACE";
    case Method::HTTP_OPTIONS:
        return "OPTIONS";
    case Method::HTTP_CONNECT:
        return "CONNECT";
    case Method::HTTP_PATCH:
        return "PATCH";
    }
    return "UNKNOWN";
}

Method HTTP::method_from_string(const std::string& method) noexcept {
    if (method == to_string(Method::HTTP_GET)) {
        return Method::HTTP_GET;
    }
    else if (method == to_string(Method::HTTP_HEAD)) {
        return Method::HTTP_HEAD;
    }
    else if (method == to_string(Method::HTTP_POST)) {
        return Method::HTTP_POST;
    }
    else if (method == to_string(Method::HTTP_PUT)) {
        return Method::HTTP_PUT;
    }
    else if (method == to_string(Method::HTTP_DELETE)) {
        return Method::HTTP_DELETE;
    }
    else if (method == to_string(Method::HTTP_TRACE)) {
        return Method::HTTP_TRACE;
    }
    else if (method == to_string(Method::HTTP_OPTIONS)) {
        return Method::HTTP_OPTIONS;
    }
    else if (method == to_string(Method::HTTP_CONNECT)) {
        return Method::HTTP_CONNECT;
    }
    else if (method == to_string(Method::HTTP_PATCH)) {
        return Method::HTTP_PATCH;
    }
    return Method::HTTP_UNKNOWN;
}

std::string HTTP::to_string(Version version) {
    switch (version) {
    case Version::HTTP_1_0:
        return "HTTP/1.0";

    case Version::HTTP_1_1:
        return "HTTP/1.1";

    case Version::HTTP_2_0:
        return "HTTP/2.0";
    }
    return "UNKNOWNVER";
}

Version HTTP::version_from_string(const std::string& version) noexcept {
    if (version == to_string(Version::HTTP_1_0)) {
        return Version::HTTP_1_0;
    }
    else if (version == to_string(Version::HTTP_1_1)) {
        return Version::HTTP_1_1;
    }
    else if (version == to_string(Version::HTTP_2_0)) {
        return Version::HTTP_2_0;
    }
    return Version::HTTP_UNKNOWN;
}

void DLNetwork::HTTP::urlParamParse(const std::string& url, std::string& path, std::map<std::string, std::string>& urlParams) noexcept
{
    std::vector<std::string> part = split(url, "?");
    path = part[0];
    if (part.size() > 1) {
        char* sp;
        char* orip = (char*)malloc(part[1].size() + 1);
        char* p = orip;
        strcpy(p, part[1].c_str());
        do {
            sp = strchr(p, '=');
            if (!sp) {
                break;
            }
            *sp = NULL;
            char* key = p;
            p = sp + 1;
            char* val = p;
            sp = strchr(p, '&');
            if (sp) {
                *sp = NULL;
                p = sp + 1;
            }
            urlParams.emplace(key, val);
        } while (sp);
        free(orip);
    }
}

std::string HTTP::Request::serialize() const noexcept {
    std::string request;
    request += to_string(this->method);
    request += " ";
    request += this->resource;
    request += " ";
    request += to_string(this->version);
    request += LINE_END;

    for (auto& header : headers) {
        request += Header::serialize(header.first, header.second);
    }

    request += LINE_END;

    if (body.length()) {
        //request += LINE_END;
        request += body;
    }
    return request;
}

Request::ErrorCode HTTP::Request::deserialize(const char* buf, size_t bufLen) {
    /*
    GET /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22} HTTP/1.1\r\n
    Host: 120.55.94.78:12345\r\n
    Connection: keep-alive\r\n
    Upgrade-Insecure-Requests: 1\r\n
    User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.146 Safari/537.36\r\n
    Accept-Encoding: gzip, deflate\r\n
    Accept-Language: zh-CN, zh; q=0.9, en; q=0.8\r\n
    \r\n
    */
    std::unique_ptr<char[]> buf2 = std::make_unique<char[]>(bufLen + 100);
    url_decode(buf, bufLen, buf2.get(), bufLen + 100);
    int nLen = strlen(buf2.get());
    //std::cout << "url in:" << buf << "decode:" << buf2.get() << std::endl;
    std::vector<std::string> segments = split(buf2.get(), nLen, std::string(LINE_END) + std::string(LINE_END));
    //std::vector<std::string> segments = split(buf, bufLen, std::string(LINE_END) + std::string(LINE_END));
    if (segments.size() < 1) {
        std::cout << "HTTP Request line insufficent" << std::string(buf, bufLen > 40 ? 40 : bufLen) << std::endl;
        return ErrorCode::RequestLineInsufficent;
    }

    std::string headerSegment = std::move(segments[0]);
    segments.erase(segments.begin());
    body = concat(segments);

    std::vector<std::string> lines = split(headerSegment, std::string(LINE_END));
    if (lines.size() < 1) {
        std::cout << "HTTP Request line insufficent" << std::string(buf, bufLen>40 ? 40 : bufLen) << std::endl;
        return ErrorCode::RequestLineInsufficent;
    }

    std::vector<std::string> headerSegments = split(lines[0], " ");

    if (headerSegments.size() != 3) {
        std::cout << "HTTP Request first line error" << lines[0].substr(0, 40) << std::endl;
        return ErrorCode::RequestFirstLineError;
    }

    method = method_from_string(headerSegments[0]);
    resource = headerSegments[1];
    version = version_from_string(headerSegments[2]);

    urlParamParse(resource, path, urlParams);

    for (std::size_t i = 1; i < lines.size(); i++) {
        if (lines[i].size() > 0) {
            Header header = Header::deserialize(lines[i]);
            headers.insert_or_assign(std::move(header.key), std::move(header.value));
        }
    }

    return ErrorCode::OK;
}

std::string HTTP::Response::serialize() const noexcept {
    std::string response;
    response += to_string(version);
    response += " ";
    response += std::to_string(responseCode);
    response += " ";
    response += respCode2string(responseCode);
    response += LINE_END;

    for (auto& header : headers) {
        response += Header::serialize(header.first, header.second);
    }
    response += LINE_END;

    if (body.length()) {
        response += LINE_END;
        response += body;
    }
    return response;
}

void HTTP::Response::deserialize(const char* buf, size_t bufLen) noexcept {
    std::vector<std::string> segments = split(buf, bufLen, std::string(LINE_END) + std::string(LINE_END));

    std::string headerSegment = segments[0];
    segments.erase(segments.begin());
    body = concat(segments);

    std::vector<std::string> headerLines = split(headerSegment, std::string(LINE_END));
    const std::string& responseCodeLine = headerLines[0];
    std::vector<std::string> responseCodeSegments = split(responseCodeLine, " ");
    version = version_from_string(responseCodeSegments[0]);
    responseCode = std::stoi(responseCodeSegments[1]);

    headerLines.erase(headerLines.begin());
    for (const std::string& line : headerLines) {
        Header header = Header::deserialize(line);
        if (header.key.length()) {
            headers.insert_or_assign(std::move(header.key), std::move(header.value));
        }
    }
}
