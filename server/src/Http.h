#ifndef HTTP
#define HTTP

#include <map>
#include <string>

typedef std::pair<unsigned short, const char*> HttpStatusCode;

extern HttpStatusCode OK; // 200 OK
extern HttpStatusCode NotFound; // 404 Not Found
extern HttpStatusCode RequestTimeout; // 408 Request Timeout
extern HttpStatusCode Forbidden; // 403 Forbidden
extern HttpStatusCode BadRequest; // 400 Bad Request
extern HttpStatusCode RequestEntityTooLarge; // 413 Request Entity Too Large
extern HttpStatusCode InternalServerError; // 500 Internal Server Error
extern HttpStatusCode NotImplemented; // 501 Not Implemented

typedef std::string HttpProtocol;
    
extern HttpProtocol HTTP_1_0; // HTTP/1.0
extern HttpProtocol HTTP_1_1; // HTTP/1.1

#endif
