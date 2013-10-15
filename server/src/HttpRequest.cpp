#include "HttpRequest.h"

using std::string;
using std::map;
using std::pair;

HttpRequest::HttpRequest(int confd) 
{
    size_t total = 0;
    ssize_t received;
    char message[kB10], buff[kB2], *end_of_request = NULL;
    memset(message, 0, sizeof(message));
    memset(buff, 0, sizeof(buff));
    timed_out = false;
    bool found_end_of_request = false;
    while(!found_end_of_request 
            && !timed_out 
            && !client_closed_connection
            && !request_entity_too_large)
    {
        fd_set curr;
        FD_SET(confd, &curr);
        struct timeval tv;
        tv.tv_sec = 8;
        tv.tv_usec = 0;
        if(select(confd+1, &curr, NULL, NULL, &tv) == 1)
        {
            received = recv(confd, buff, sizeof(buff)-1, MSG_DONTWAIT);
            while(received > 0)
            {
                if((total + received) >= sizeof(message))
                {
                    request_entity_too_large = true;
                    break;
                }
                memmove(message + total, buff, received);
                total += received;
                if(!found_end_of_request)
                {
                    end_of_request = strstr(message, "\r\n\r\n");
                    found_end_of_request = (end_of_request != NULL);
                }

                received = recv(confd, buff, sizeof(buff)-1, MSG_DONTWAIT);
            }
            if(received == 0)
                client_closed_connection = true;
            else if(received == -1 && errno != EWOULDBLOCK)
                error(1, "Error: failed to receive from client\n");
        }
        else
            timed_out = true;
    }
    headers = new map<string, string>();
    char method_buff[10], 
    path_buff[kB2], 
    protocol_buff[10], 
    headers_buff[kB2];
    memset(method_buff, 0, 10);
    memset(path_buff, 0, kB2);
    memset(protocol_buff, 0, 10);
    memset(headers_buff, 0, kB2);
    sscanf(message, "%s %s %s\r\n%s\r\n\r\n", 
           method_buff, path_buff, protocol_buff, headers_buff);
    method = string(method_buff);
    action = string(path_buff);
    protocol = string(protocol_buff); 
    connection_fd = confd;

    char* header = strtok(headers_buff, "\r\n");
    while(header != NULL)
    {
        char key[kB1], value[kB1];
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        sscanf(header, "%s: %s", key, value);
        headers->insert(pair<string, string>(string(key), string(value)));
        header = strtok(NULL, "\r\n");
    }
}

HttpRequest::~HttpRequest()
{ 
    delete headers; 
}

const char* HttpRequest::get_content_type(const char* filename)
{
    const char* ext = strrchr(filename, '.');
    if(ext == NULL)
        return "text/plain";
    if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".JPG") == 0 || 
       strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".JPEG") == 0)
        return "image/jpeg";
    if(strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0)
        return "image/png";
    if(strcmp(ext, ".gif") == 0 || strcmp(ext, ".GIF") == 0)
        return "image/gif";
    if(strcmp(ext, ".css") == 0 || strcmp(ext, ".CSS") == 0)
        return "text/css";
    if(strcmp(ext, ".ico") == 0 || strcmp(ext, ".ICO") == 0)
        return "image/ico";
    if(strcmp(ext, ".js") == 0 || strcmp(ext, ".JS") == 0)
        return "text/javascript";
    if(strcmp(ext, ".html") == 0 || strcmp(ext, ".HTML") == 0)
        return "text/html";
    if(strcmp(ext, ".pdf") == 0 || strcmp(ext, ".PDF") == 0)
        return "application/pdf";
    if(strcmp(ext, ".txt") == 0 || strcmp(ext, ".TXT") == 0)
        return "text/plain";
    return "application/octet-stream";
}

size_t HttpRequest::get_file_size(const char* filename)
{
    struct stat st;
    if(stat(filename, &st) != 0)
        error(1, "Error: stat() failed\n");
    return (size_t)st.st_size;
}

void HttpRequest::send_header(const HttpProtocol& protocol, 
            const HttpStatusCode& status_code, 
            const char* content_type, size_t content_length) const
{
    char header[kB10];
    memset(header, 0, sizeof(header));
    snprintf(header, 
            kB10-1, 
            "%s %u %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n",
            protocol.c_str(), 
            status_code.first, 
            status_code.second, 
            content_type, 
            content_length);
    send_bytes(header, strlen(header));
}

void HttpRequest::send_file_contents(const char* filename) const
{
    char buff[kB10];
    memset(buff, 0, sizeof(buff));
    int fd = open(filename, O_RDONLY);
    if(fd == -1)
        error(1, "Error: failed to open file");
    ssize_t bytes_read;
    while((bytes_read = read(fd, buff, kB10)) > 0)
    {
        send_bytes(buff, bytes_read);
        memset(buff, 0, sizeof(buff));
    }
    if(bytes_read < 0)
        error(1, "Error: failed to read from file");
    if(close(fd) != 0)
        error(1, "Error: failed to close file");
}

void HttpRequest::send_bytes(const char* bytes, size_t num_bytes) const
{
    size_t left = num_bytes;
    ssize_t sent;
    while(left > 0)
    {
        sent = send(connection_fd, bytes + num_bytes - left, left, 0);
        if(sent < 0){
            perror("Send");
            error(1, "Error: send failed\n");
        }
        left -= sent;
    }
}

string HttpRequest::GetMethod() const
{
    return method;
}

string HttpRequest::GetAction() const
{
    return action;
}

string HttpRequest::GetProtocol() const
{
    return protocol;
}

bool HttpRequest::HeaderContainsKey(const string& key) const
{
    return (headers->find(key) != headers->end());
}

string HttpRequest::GetHeaderValue(const string& key) const
{
    return (*headers)[key];
}

void HttpRequest::SendRequestTimeoutResponse() const
{
    if(protocol != "HTTP/1.1" && protocol != "HTTP/1.0")
        send_message_for_file("HTTP/1.1", RequestTimeout, "../Http/408.html");
    else
        send_message_for_file(protocol, RequestTimeout, "../Http/408.html");
}

void HttpRequest::SendRequestEntityTooLargeResponse() const
{
    if(protocol != "HTTP/1.1" && protocol != "HTTP/1.0")
        send_message_for_file("HTTP/1.1", 
                RequestEntityTooLarge, "../Http/413.html");
    else
        send_message_for_file(protocol, 
                RequestEntityTooLarge, "../Http/413.html");
}

void HttpRequest::SendBadRequestResponse() const
{
    if(protocol != "HTTP/1.1" && protocol != "HTTP/1.0")
        send_message_for_file("HTTP/1.1", BadRequest, "../Http/400.html");
    else
        send_message_for_file(protocol, BadRequest, "../Http/400.html");
}

void HttpRequest::SendNotFoundResponse() const
{
    send_message_for_file(protocol, NotFound, "../Http/404.html");
}

void HttpRequest::SendForbiddenResponse() const
{
    send_message_for_file(protocol, Forbidden, "../Http/403.html");
}

void HttpRequest::SendInternalServerErrorResponse() const
{
    send_message_for_file(protocol, InternalServerError, "../Http/500.html");
}

void HttpRequest::SendNotImplementedResponse() const
{
    send_message_for_file(protocol, NotImplemented, "../Http/501.html"); 
}

void HttpRequest::SendOKResponse(const char* filename, bool is_dir) const
{
    if(is_dir)
    {
        string index_file(filename);
        if(index_file.find_last_of('/') != (index_file.size() - 1))
            index_file += "/";
        index_file += "index.html";
        struct stat st;
        if(stat(index_file.c_str(), &st) == 0 && st.st_mode & S_IRUSR)
            send_message_for_file(protocol, OK, index_file.c_str());
        else
        {
            size_t pos = index_file.find_last_of('.');
            index_file.replace(index_file.begin()+pos, index_file.end(),".php");
            if(stat(index_file.c_str(), &st) == 0 && st.st_mode & S_IRUSR)
                send_message_for_file(protocol, OK, index_file.c_str());
            else
                send_dir_page(filename);
        }
    }
    else
        send_message_for_file(protocol, OK, filename);
}

void HttpRequest::send_message_for_file(const HttpProtocol& protocol, 
        const HttpStatusCode& status_code, const char* filename) const
{
    size_t len = get_file_size(filename);
    const char* content_type = get_content_type(filename);
    send_header(protocol.c_str(), status_code, content_type, len);
    send_file_contents(filename);
}

void HttpRequest::send_dir_page(const char* filename) const
{
    string body;
    body += "<!DOCTYPE html><html><head><title>";
    body += action;
    body += "</title></head><body><h1>";
    body += action;
    body += "</h1><ul>";
    DIR *dir = opendir(filename);
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL)
    {
        string child_file(filename);
        child_file += ent->d_name;
        body += "<li><a href=\"";
        body += action;
        body += ent->d_name;
        struct stat st;
        if(stat(child_file.c_str(), &st) != 0)
            error(1, "Error: stat() failed\n");
        if(S_ISDIR(st.st_mode))
            body += "/";
        body += "\">";
        body += ent->d_name;
        if(S_ISDIR(st.st_mode))
            body += "/";
        body += "</a></li>";
    }
    body += "</ul></body></html>";
    if(closedir(dir) != 0)
        error(1, "Error: failed to close directory");
    size_t len = body.size();
    const char* content_type = "text/html";
    send_header(protocol.c_str(), OK, content_type, len);
    send_bytes(body.c_str(), body.size());
}

void HttpRequest::Process()
{
    if(!timed_out && !request_entity_too_large && !client_closed_connection)
    {
        string filename;
        if(action.find_first_of('/') == 0)
            filename += ".";
        filename += action;
        if(protocol != HTTP_1_0 && protocol != HTTP_1_1)
            SendBadRequestResponse();
        else if(method == "GET")
        {
            struct stat st;
            int stat_ret = stat(filename.c_str(), &st);
            if(stat_ret == 0 && (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)))
            {
                if(st.st_mode & S_IRUSR)
                {
                    if(S_ISDIR(st.st_mode) && 
                            filename.find_last_of('/') != filename.size() - 1)
                        SendNotFoundResponse();
                    else
                        SendOKResponse(filename.c_str(), S_ISDIR(st.st_mode));
                }
                else
                    SendForbiddenResponse();
            }
            else
                SendNotFoundResponse();
        }
        else
            SendNotImplementedResponse();
    }
    else if(timed_out)
        SendRequestTimeoutResponse();
    else if(request_entity_too_large)
        SendRequestEntityTooLargeResponse();
}

bool HttpRequest::ClientClosedConnection() const
{
    return client_closed_connection;
}
