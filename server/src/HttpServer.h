#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "ServerThread.h"
#include "ErrorHandling.h"

using std::vector;

class HttpServer 
{
    private:
        int _sockfd; // Socket file descriptor for listening for connections
        pthread_t _listener; // Thread ID of the listener routine
        unsigned int _port; // Port to which the socket is bound
        unsigned int _num_workers; // Number of threads working for the server
        vector<ServerThread*> _workers; // The collection of worker threads
        
        /*
            A thread function that accepts incoming connections and distributes
            work to the most available thread (has the least work)
        */
        static void *_listen_routine(void*); // The listener thread function
    public:
        /* 
            Constructor
            Parameter(s): const char*, the root directory from which
                          to serve files to clients
                          unsigned int, the number of threads
                          that will process client connections
        */
        HttpServer(const char*, unsigned int);
        
        /*
            Destructor
        */
        ~HttpServer();

        /*
            Creates a socket, binds to a port, and begins listening
            Parameters(s): unsigned int, a port number (must be at least 1024)
            Returns: void
        */
        void ListenOnPort(unsigned int port);

        /*
            Prints out how work is distributed across the threads
            Parameter(s): void
            Returns: void
        */
        void Profile(); // View how the worker is distributed among threads
        
        /*
            Checks to see if _sockfd is an active socket
            Parameter(s): void
            Returns: bool
        */
        bool IsListening(); 
        
        /* 
            If listening, cancel listener thread, shutdown and close socket 
            Parameter(s): void
            Returns: void
        */
        void StopListening(); 
};

#endif
