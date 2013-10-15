#include "src/HttpServer.h"
#include "src/ErrorHandling.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

/*
   This function prints an error message in case the user enters invalid input
   Parameter(s): void
   Returns: void
*/
void usage()
{
    fprintf(stderr, "Usage: ./lab2 <port> <dir>\n");
}

/*
   This function prints a list of recognized commands
   Parameter(s): void
   Returns: void
*/
void help()
{
    printf("`profile': view load distribution across server threads\n");
    printf("`help': see this list of commands\n");
    printf("`quit': terminate the server and exit\n");
}

unsigned int get_num_cores()
{
    int proc_pipes[2];
    if(pipe(proc_pipes) != 0)
        error(1, "Error: failed to open pipes\n");
    pid_t pid = fork();
    if(pid != 0) // parent
    {
        if(close(proc_pipes[1]) == -1)
            error(1, "Error: failed to close the write pipe\n");
        char buff[80];
        memset(buff, 0, sizeof(buff));
        if(read(proc_pipes[0], buff, sizeof(buff)-1) < 0)
            error(1, "Error: failed to read from pipe\n");
        int status;
        if(waitpid(pid, &status, 0) != pid)
            error(1, "Error: failed to wait for the child process\n");
        if(close(proc_pipes[0]) == -1)
            error(1, "Error: failed to close the read pipe\n");
        return atoi(buff);
    }
    else // child
    {
        if(close(proc_pipes[0]) == -1)
            error(1, "Error: failed to close the read pipe\n");
        dup2(proc_pipes[1], 1);
        extern char** environ;
        char nproc[6];
        memset(nproc, 0, sizeof(nproc));
        strncpy(nproc, "nproc", 5);
        char* command[2];
        command[0] = nproc;
        command[1] = NULL;
        if(execvpe("nproc", command, environ) == -1)
        {
            fprintf(stderr, "`nproc' unsupported or not in PATH\n");
            fprintf(stderr, "Defaulting to 4 worker threads\n");
            write(proc_pipes[1], "4", 1);
            if(close(proc_pipes[1]) == -1)
                error(1, "Error: failed to close the write pipe\n");
        }
        exit(EXIT_SUCCESS);
    }
}

/*
   MAIN FUNCTION (ENTRY POINT)
*/
int main(int argc, char** argv)
{
    if(argc < 3)
    {
        usage();
        error(1, "Error: expected 3 command line arguments\n");
    }
    unsigned int port = atoi(argv[1]);
    const char* root = argv[2];
    unsigned int num_workers = get_num_cores();
    HttpServer* server = new HttpServer(root, num_workers);
    server->ListenOnPort(port);
    help();
    fflush(stdout);
    char buff[80];
    bool time_to_go = false;
    while(!time_to_go)
    {
        printf("> ");
        fflush(stdout);
        memset(buff, 0, sizeof(buff));
        if(!fgets(buff, sizeof(buff)-1, stdin))
            error(1, "Error: failed to read from stdin\n");
        if(strcmp(buff, "profile\n") == 0)
            server->Profile();
        else if(strcmp(buff, "help\n") == 0)
            help();
        else if(strcmp(buff, "quit\n") == 0)
            time_to_go = true;
        else
            printf("unrecognized command\n");
    }
    printf("Shutting down...");
    fflush(stdout);
    delete server;
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}

