#include "HttpServer.h"

HttpServer::HttpServer(const char* root, unsigned int num_workers)
{
    struct stat st;
    if(stat(root, &st) != 0)
        error(1, "Error: file does not exist\n");
    else if(!S_ISDIR(st.st_mode))
        error(1, "Error: file is not a directory\n");
    else if(!(st.st_mode & S_IRUSR)) 
        error(1, "Error: you don't have permission to read this directory\n");
    if(chdir(root) == -1)
        error(1, "Error: failed to change directory to %s\n", root);
    if(num_workers < 1)
        error(1, "Error: must have at least one worker to process requests\n");
    _num_workers = num_workers;
    for(unsigned int i=0; i<_num_workers; i++)
    {
        _workers.push_back(new ServerThread());
    }
}

HttpServer::~HttpServer()
{
    StopListening();
    for(unsigned int i=0; i<_num_workers; i++)
    {
        delete _workers[i];
    }
}

void HttpServer::ListenOnPort(unsigned int port)
{
    if(IsListening())
    {
        printf("Server is already listening on port %u\n", _port);
        return;
    }
    _sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(_sockfd == -1)
        error(1, "Error: failed to create TCP socket\n");
    printf("Created TCP socket: %d\n", _sockfd);
    struct sockaddr_in saddr;
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(_sockfd, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in)))
        error(1, "Error: port is busy\n");
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = 0;
    if(setsockopt(_sockfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(struct linger)))
        error(1, "Error: failed to set socket option (linger off)\n");
    if(listen(_sockfd, 100))
        error(1, "Error: failed to begin listening on port\n");
    printf("Listening on port %u\n", port);
    if(pthread_create(&_listener, NULL, _listen_routine, this) != 0)
        error(1, "Error: failed to spawn new thread\n");
}

void* HttpServer::_listen_routine(void *arg)
{
    HttpServer* server = (HttpServer*)arg;
    struct sockaddr_in paddr;
    unsigned int size = sizeof(paddr);
    while(true)
    {
        int confd = accept(server->_sockfd, (struct sockaddr*)&paddr, &size);
        if(confd == -1)
            error(1, "Error: failed to accept new connection\n");
        ServerThread* min_thread = server->_workers[0];
        size_t min_connections = min_thread->GetActiveRequestCount() +
            min_thread->GetPendingRequestCount();
        for(unsigned int i=1; i<server->_num_workers; i++)
        {
            ServerThread* curr = server->_workers[i];
            size_t num_connections = curr->GetActiveRequestCount() +
                curr->GetPendingRequestCount();
            if(num_connections < min_connections)
            {
                min_thread = curr;
                min_connections = num_connections;
            }
        }
        min_thread->Lock();
        min_thread->Notify();
        min_thread->PushBackPendingRequest(confd);
        min_thread->Unlock();
    }
    return NULL;
}

bool HttpServer::IsListening()
{
    struct stat st;
    return (fstat(_sockfd, &st) == 0 && S_ISSOCK(st.st_mode));
}

void HttpServer::StopListening()
{
    if(IsListening())
    {
        if(pthread_cancel(_listener) != 0)
            error(1, "Error: failed to cancel listener\n");
        if(pthread_join(_listener, NULL) != 0)
            error(1, "Error: failed to join sender thread\n");
        if(shutdown(_sockfd, SHUT_RDWR) == -1)
            error(1, "Error: failed to shutdown socket\n");
        if(close(_sockfd) == -1)
            error(1, "Error: failed to close socket\n");
    }
}

void HttpServer::Profile()
{
    for(unsigned int i=0; i<_num_workers; i++)
    {
        _workers[i]->Lock();
        size_t active = _workers[i]->GetActiveRequestCount();
        size_t pending = _workers[i]->GetPendingRequestCount();
        _workers[i]->Unlock();
        printf("%u: Active: %lu, Pending: %lu\n", i, active, pending);
    }
}
