#define _XOPEN_SOURCE 500
#define _POSIX_SOURCE
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <vector>
#include <string.h>
#include <cstdio>
#include <errno.h>
#include <wait.h>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <iostream>

using namespace std;

int spawn(const char* file, char* const argv[])
{
    pid_t process_id = fork();

    if (process_id == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    if (process_id != 0)
    {
        int status;
        waitpid(process_id, &status, 0);

        if (!WIFEXITED(status))
        {
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }

        return WEXITSTATUS(status);
    }

    int result = execvp(file, argv);

    if (result == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    return -1; //Never reaches
}

pid_t new_spawn(const char* file, char* const argv[])
{
    pid_t process_id = fork();

    if (process_id == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    if (process_id != 0)
    {
        int status;
        waitpid(process_id, &status, 0);

        if (!WIFEXITED(status))
        {
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }

        return process_id;
    }

    int result = execvp(file, argv);
    return process_id;

    if (result == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    return -1; //Never reaches
}

struct execargs_t
{
    char* program;
    char** args;
    int count;
};

struct execargs_t *exec_new(char* _program, char** _args, int _count)
{
    char* tmp = (char*) malloc(4096);

    if (tmp == NULL)
        return NULL;

    int len = -1;

    do
    {
        len++;
        tmp[len] = _program[len];
    } while (_program[len] != '\0');

    char** tmp_args = (char**) malloc(sizeof(char*) * (_count + 1));

    for (int i = 0; i <= _count; i++)
        tmp_args[i] = (char*) malloc(4096);

    if (tmp_args == NULL)
    {
        free(tmp);
        return NULL;
    }

    for (int i = 0; i < _count; i++)
    {
        len = -1;

        do {
            len++;
            tmp_args[i][len] = _args[i][len];
        } while (_args[i][len] != '\0');
    }

    tmp_args[_count] = NULL;
    execargs_t* res = (execargs_t*) malloc(sizeof(execargs_t));

    if (res == NULL)
    {
        free(tmp);
        free(tmp_args);
        return NULL;
    }

    res -> program = tmp;
    res -> args = tmp_args;
    res -> count = _count;

    return res;
}

void exec_free(struct execargs_t* exec)
{
    if (exec != NULL)
        free(exec);
}

int exec(execargs_t* args)
{
    int process_id = new_spawn(args -> program, args -> args);
    if (process_id < 0)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    return process_id;
}

int* pids_global;
int pids_count;

void action(int sig)
{
    for (int i = 0; i < pids_count; i++)
        kill(pids_global[i], SIGKILL);

    pids_count = 0;
}

int runpiped(execargs_t** programs, size_t n)
{
    if (n == 0)
        return 0;

    int pipes[n - 1][2];
    int pids[n];

    for (int i = 0; i < n - 1; i++)
    {
        int res = pipe2(pipes[i], O_CLOEXEC);

        if (res == -1)
        {
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }

    for (int i = 0; i < n; i++)
    {
        pids[i] = fork();

        if (pids[i] == -1)
            return -1;

        if (pids[i] != 0)
            continue;

        if (i > 0)
            dup2(pipes[i - 1][0], STDIN_FILENO);

        if (i < n - 1)
            dup2(pipes[i][1], STDOUT_FILENO);

        int res = execvp(programs[i] -> program, programs[i] -> args);

        if (res == -1)
        {
            fprintf(stderr, "%s\n", strerror(errno));
            _exit(-1);
        }

        _exit(0);
    }

    for (int i = 0; i < n - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    pids_global = (int*) pids;
    pids_count = n;
    struct sigaction act;
    act.sa_handler = &action;

    if (sigaction(SIGINT, &act, NULL) < 0)
        return -1;

    int status;

    for (int i = 0; i < n; i++)
        waitpid(pids[i], &status, 0);

    pids_count = 0;

    return 0;
}

typedef struct addrinfo addrinfo;
typedef struct sockaddr sockaddr;
typedef struct epoll_event epoll_event;

vector <execargs_t*> programs;

void parse(char* command, int len) {
    vector <char*> args;
    char* arg = (char*) malloc(len);
    char* program = (char*) malloc(len);
    int i = -1;
    int curr = 0;
    char found_program = 0;
    bool is_quote = 0;
    args.clear();
    programs.clear();

    while (command[++i] == ' ') {}

    for (; i < len; i++) {
        if (command[i] == '\0') {
            args.clear();
            programs.clear();
            return;
        } else if ((command[i] == ' ' && !is_quote) || command[i] == '\n') {
            if (curr == 0) {
                break;
            }

            if (!found_program) {
                program[curr] = '\0';
                found_program = 1;
            }

            arg[curr] = '\0';
            char* tmp = (char*) malloc(len);
            strcpy(tmp, arg);
            args.push_back(tmp);
            curr = 0;

            if (command[i] == '\n') {
                break;
            }

            while (command[++i] == ' ') {}

            i--;
        } else if (command[i] == '|') {
            if (!found_program) {
                program[curr] = '\0';
            }

            if (curr != 0) {
                arg[curr] = '\0';
                char* tmp = (char*) malloc(len);
                strcpy(tmp, arg);
                args.push_back(tmp);
                curr = 0;
            }

            while (command[++i] == ' ') {}

            i--;
            found_program = 0;
            programs.push_back(exec_new(program, args.data(), (int) args.size()));
            args.clear();
        } else {
            if (command[i] == '\'' || command[i] == '\"') {
                is_quote = !is_quote;
                continue;
            }

            if (!found_program) {
                program[curr] = command[i];
            }

            arg[curr++] = command[i];
        }
    }

    programs.push_back(exec_new(program, args.data(), (int) args.size()));
}

int main(int argc, char** argv) {
    char* s = (char *) "cat a.txt";
    parse(s, 9);

    runpiped(programs.data(), 1);
    return 0;
}
