/*
 * Copyright (C) 1996-2015 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef _SQUID_SRC_HTTP_ONE_REQUESTPARSER_H
#define _SQUID_SRC_HTTP_ONE_REQUESTPARSER_H

#include "http/one/Parser.h"
#include "http/RequestMethod.h"

namespace Parser {
class Tokenizer;
}

namespace Http {
namespace One {

/** HTTP/1.x protocol request parser
 *
 * Works on a raw character I/O buffer and tokenizes the content into
 * the major CRLF delimited segments of an HTTP/1 request message:
 *
 * \item request-line (method, URL, protocol, version)
 * \item mime-header (set of RFC2616 syntax header fields)
 */
class RequestParser : public Http1::Parser
{
public:
    RequestParser();
    virtual ~RequestParser() {}

    /* Http::One::Parser API */
    virtual void clear() {*this = RequestParser();}
    virtual Http1::Parser::size_type firstLineSize() const;
    virtual bool parse(const SBuf &aBuf);

    /// the HTTP method if this is a request message
    const HttpRequestMethod & method() const {return method_;}

    /// the request-line URI if this is a request message, or an empty string.
    const SBuf &requestUri() const {return uri_;}

private:
    void skipGarbageLines();
    int parseRequestFirstLine();
    int parseMethodField(Http1::Tokenizer &, const CharacterSet &);
    int parseUriField(Http1::Tokenizer &);
    int parseHttpVersionField(Http1::Tokenizer &);

    /// what request method has been found on the first line
    HttpRequestMethod method_;

    /// raw copy of the original client request-line URI field
    SBuf uri_;

    /// amount of garbage bytes tolerantly skipped inside the request-line
    /// may be -1 if sender only omitted CR on terminator
    int64_t firstLineGarbage_;
};

} // namespace One
} // namespace Http

#endif /*  _SQUID_SRC_HTTP_ONE_REQUESTPARSER_H */

