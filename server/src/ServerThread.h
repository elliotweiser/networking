#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <queue>
#include <list>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "ErrorHandling.h"
#include "HttpRequest.h"
#include "Http.h"

class ServerThread 
{
    private:
        unsigned int id; // Index of this object in worker pool
        pthread_t tid; // Handle to the worker thread
        pthread_mutex_t* mutex; // This thread's mutex for avoiding races
        std::list<int>* pending_requests; // A list of pending connections
        std::list<int>* active_requests; // A list of active connections
        int pipes[2]; // For sending receiving notifications (non-blocking)
        int blocking_pipes[2]; // Blocking version of pipes

        /*
            A thread function for handling all connections (pending and active)
            Parameter(s): void*, pointer to the ServerThread
            Returns: void*
        */
        static void *connection_routine(void*); // Thread function

        /*
            A function for closing an active connection and removing it from
            the active connection list (for convenience)
            Parameter(s): std::list<int>::iterator&, ref to a list iterator
            Returns: void
        */
        void close_connection(std::list<int>::iterator&);
    public:
        
        /* Constructor */
        ServerThread();

        /* Destructor */
        ~ServerThread();

        /*
            Get the number of pending connections
            Parameter(s): void
            Returns: size_t
        */
        size_t GetPendingRequestCount();
        
        /*
            Place a new connection at the back of the pending connections list
            Parameter(s): int, a new connection file descriptor
            Returns: void
        */
        void PushBackPendingRequest(int);

        /*
            Get the file descriptor at the front of the pending connection list
            Parameter(s): void
            Returns: int
        */
        int FrontPendingRequest();

        /*
            Pops the front of the pending connection list
            Parameter(s): void
            Returns: void
        */
        void PopFrontPendingRequest();

        /*
            Get the number of active connections
            Parameter(s): void
            Returns: size_t
        */
        size_t GetActiveRequestCount();

        /*
            Get the number of pending and active connections
            Parameter(s): void
            Returns: size_t
        */
        size_t GetTotalConnections();

        /*
            Locks this thread's mutex
            Parameter(s): void
            Returns: int
        */
        int Lock();
        
        /*
            Unlocks this thread's mutex
            Parameter(s): void
            Returns: int
        */
        int Unlock();

        /*
            Process all active connections.  Receive requests, send responses
            and close the connection as needed.

            NOTE: 
            
            HTTP/1.0 by default says close connections when done unless 
            specified otherwise (Connection: Keep-Alive).
            
            HTTP/1.1 by default says keep connections alive unless specified
            otherwise (Connection: Close).
            
            Requests that don't specify a protocol will be closed.

            Parameter(s): void
            Returns: void
        */
        void ProcessConnections();

        /*
            Write a char to the pipe to indicate that a pending connection has
            been added.  If there are no pending or active connections, then
            write to the blocking pipe.
        */
        void Notify();
};

#endif
