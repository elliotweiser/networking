#include "ServerThread.h"

void* ServerThread::connection_routine(void *arg)
{
    char flag;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    ServerThread* worker = (ServerThread*)arg;
    while(true)
    {
        ssize_t read_ret;
        while((read_ret = read(worker->pipes[0], &flag, 1)) > 0)
        {
            worker->Lock();
            int confd = worker->pending_requests->back();
            worker->active_requests->push_front(confd);
            worker->pending_requests->pop_back();
            worker->Unlock();
        }
        if(read_ret == -1 && errno != EWOULDBLOCK) 
            error(1, "Error: failed to read from pipe\n");
        if(worker->GetTotalConnections() == 0)
        {
            if((read_ret = read(worker->blocking_pipes[0], &flag, 1)) > 0)
            {   
                worker->Lock();
                int confd = worker->pending_requests->back();
                worker->active_requests->push_front(confd);
                worker->pending_requests->pop_back();
                worker->Unlock();
            }
        }
        worker->ProcessConnections();
    }
    return NULL;
}

ServerThread::ServerThread()
{
    pending_requests = new std::list<int>();
    active_requests = new std::list<int>();
    mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    if(pipe2(pipes, O_NONBLOCK) == -1)
        error(1, "Error: failed to open pipes\n");
    if(pipe(blocking_pipes) == -1)
        error(1, "Error: failed to open blocking pipes\n");
    if(pthread_create(&tid, NULL, connection_routine, this) != 0)
        error(1, "Error: failed to spawn new thread\n");
}

ServerThread::~ServerThread()
{
    if(pthread_cancel(tid) != 0)
        error(1, "Error: failed to cancel thread\n");
    if(pthread_join(tid, NULL) != 0)
        error(1, "Error: failed to join thread\n");
    std::list<int>::iterator it = pending_requests->begin();
    while(it != pending_requests->end())
    {
        std::list<int>::iterator old = it;
        it++;
        if(close(*old) == -1)
            error(1, "Error: failed to close connection\n");
        pending_requests->erase(old);
    }
    delete pending_requests;
    it = active_requests->begin();
    while(it != active_requests->end())
    {
        close_connection(it);
    }
    delete active_requests;
    if(close(pipes[0]) == -1)
        error(1, "Error: failed to close read pipe\n");
    if(close(pipes[1]) == -1)
        error(1, "Error: failed to close write pipe\n");
    if(close(blocking_pipes[0]) == -1)
        error(1, "Error: failed to close read blocking pipe\n");
    if(close(blocking_pipes[1]) == -1)
        error(1, "Error: failed to close write blocking pipe\n");
    pthread_mutex_destroy(mutex);
    free(mutex);
}

void ServerThread::Notify()
{
    if(GetTotalConnections() == 0)
    {
        if(write(blocking_pipes[1], "1", 1) == -1)
            error(1, "Error: failed to write to blocking pipe\n");
    }
    else
    {
        if(write(pipes[1], "1", 1) == -1)
            error(1, "Error: failed to write to pipe\n");
    }
}

void ServerThread::ProcessConnections()
{
    if(!active_requests->empty())
    {
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        fd_set connection_set;
        int max_confd = -1;
        std::list<int>::iterator it=active_requests->begin(); 
        while(it != active_requests->end())
        {
            int confd = *it;
            FD_SET(confd, &connection_set);
            if(confd > max_confd)
                max_confd = confd;
            ++it;
        }
        int select_ret = select(max_confd+1, &connection_set, NULL, NULL, &tv);
        if(select_ret > 0)
        {
            std::list<int>::iterator it=active_requests->begin();
            while(it != active_requests->end())
            {
                int confd = *it;
                if(FD_ISSET(confd, &connection_set))
                {
                    HttpRequest request(confd);
                    request.Process();
                    string value;
                    if(request.GetProtocol() == "HTTP/1.0")
                    {
                        if(request.HeaderContainsKey("Connection"))
                        {
                            value = request.GetHeaderValue("Connection");
                            if(value != "Keep-Alive")
                                close_connection(it);
                            else
                                ++it;

                        }
                        else
                            close_connection(it);
                   }
                   else if(request.GetProtocol() == "HTTP/1.1")
                   {
                        if(request.HeaderContainsKey("Connection"))
                        {
                            value = request.GetHeaderValue("Connection");
                            if(value == "Close")
                                close_connection(it);
                            else
                                ++it;
                        }
                        else
                            ++it;
                    }
                    else
                        close_connection(it);
                }
                else
                    ++it;
            }
        }
        FD_ZERO(&connection_set);
    }
}

size_t ServerThread::GetPendingRequestCount()
{
    return pending_requests->size();
}

void ServerThread::PushBackPendingRequest(int confd)
{
    pending_requests->push_back(confd);
}

int ServerThread::FrontPendingRequest()
{
    return pending_requests->front();
}

void ServerThread::PopFrontPendingRequest()
{
    pending_requests->pop_front();
}

int ServerThread::Lock()
{
    return pthread_mutex_lock(mutex);
}

int ServerThread::Unlock()
{
    return pthread_mutex_unlock(mutex);
}

size_t ServerThread::GetActiveRequestCount()
{
    return active_requests->size();
}

void ServerThread::close_connection(std::list<int>::iterator& it)
{
    std::list<int>::iterator old = it;
    it++;
    if(close(*old) == -1)
        error(1, "Error: failed to close connection\n");
    active_requests->erase(old);
}

size_t ServerThread::GetTotalConnections()
{
    return pending_requests->size() + active_requests->size();
}
