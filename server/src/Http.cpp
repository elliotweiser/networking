#include "Http.h"

HttpStatusCode OK(200, "OK");
HttpStatusCode NotFound(404, "Not Found");
HttpStatusCode Forbidden(403, "Forbidden");
HttpStatusCode BadRequest(400, "Bad Request");
HttpStatusCode RequestEntityTooLarge(413, "Request Entity Too Large");
HttpStatusCode InternalServerError(500, "Internal Server Error");
HttpStatusCode NotImplemented = HttpStatusCode(501, "Not Implemented");
HttpStatusCode RequestTimeout(408, "Request Timeout");

HttpProtocol HTTP_1_0 = "HTTP/1.0";
HttpProtocol HTTP_1_1 = "HTTP/1.1";
