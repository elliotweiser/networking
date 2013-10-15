#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string>
#include <map>
#include "Http.h"
#include "Memory.h"
#include "ErrorHandling.h"

using std::string;
using std::map;

class HttpRequest 
{
    private:
        string method; // The HTTP request method
        string action; // The action (or path) given by the client
        string protocol; // The HTTP protocol
        map<string, string>* headers; // The header fields
        bool client_closed_connection; // Did recv() return 0?
        bool request_entity_too_large; // Did received data fit in buffer?
        bool timed_out; // Did the client take to long to give full request?
        int connection_fd; // The connection file descriptor

        /*
            Get the Content-Type header required for the given file name
            Parameter(s): const char*, the file name
            Returns: const char*
        */
        static const char* get_content_type(const char*);

        /*
            Gets the size (in bytes) of the requested file
            Parameter(s): const char*, the file name
            Returns: size_t
        */
        static size_t get_file_size(const char*);

        /*
            Sends the HTTP response to the client for a given file name
            Parameter(s): const HttpProtocol&, a valid HTTP protocol
                          const HttpStatusCode&, e.g. (200, "OK")
                          const char*, the file name
            Returns: void
        */
        void send_message_for_file(
                const HttpProtocol&, 
                const HttpStatusCode&, 
                const char*) const;
        
        /*
            Sends the HTTP header to the client for a given file name
            Parameter(s): const HttpProtocol&, a valid HTTP protocol
                          const HttpStatusCode&, e.g. (200, "OK")
                          const char*, the content-type
                          size_t, the content-length
            Returns: void
        */
        void send_header(
                const HttpProtocol&, 
                const HttpStatusCode&, 
                const char*, 
                size_t) const;

        /*
            Send the contents of the file to the client
            Parameter(s): const char*, the file name
            Returns: void
        */
        void send_file_contents(const char*) const;
        
        /*
            Sends a byte buffer to the client
            Parameter(s): const char*, the byte buffer
                          size_t, the number of bytes to send
            Returns: void
        */
        void send_bytes(const char*, size_t) const;

        /*
            If index.html or index.php does not exist in the directory, send
            a listing of the directory's contents as HTML
            Parameter(s): const char*, the directory name
            Returns: void
        */
        void send_dir_page(const char*) const;
    
    public:
        
        /*
            Constructor
            Parameter(s): int, the connection file descriptor
        */
        HttpRequest(int);

        /*
            Destructor
        */
        ~HttpRequest();

        /*
            Get the HTTP method associated with the request
            Parameter(s): void
            Returns: string
        */
        string GetMethod() const;

        /*
            Get the action associated with the request
            Parameter(s): void
            Returns: string
        */
        string GetAction() const;

        /*
            Get the HTTP protocol associated with the request
            Parameter(s): void
            Returns: string
        */
        string GetProtocol() const;
        
        /*
            Checks if the given header key was given in the request
            Parameter(s): const string&, the name of the header key
            Returns: bool
        */
        bool HeaderContainsKey(const string& key) const;
        
        /*
            Get the value of the header associated with a key (if it exists)
            Parameter(s): const string&, the name of the header key
            Returns: string
        */
        string GetHeaderValue(const string&) const;
        
        /*
            Processes the request by sending the appropriate response to the
            client, including telling the server how to handle the connection
            Parameter(s): void 
            Returns: void
        */
        void Process();

        /*
            Checks if the client closed the connection (i.e. recv() returned 0)
            Parameter(s): void
            Returns: bool
        */
        bool ClientClosedConnection() const;

        /*
            Send a 200 response to the client
            Parameter(s): const char*, the file name
                          bool, is it a directory?
            Returns: void
        */
        void SendOKResponse(const char*, bool) const;

        /*
            Send a 408 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendRequestTimeoutResponse() const;

        /*
            Send a 400 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendBadRequestResponse() const;

        /*
            Send a 403 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendForbiddenResponse() const;

        /*
            Send a 404 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendNotFoundResponse() const;

        /*
            Send a 413 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendRequestEntityTooLargeResponse() const;

        /*
            Send a 500 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendInternalServerErrorResponse() const;

        /*
            Send a 501 response to the client
            Parameter(s): void
            Returns: void
        */
        void SendNotImplementedResponse() const;
};

#endif
