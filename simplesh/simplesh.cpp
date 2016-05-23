#include <unistd.h>
#include <vector>
#include <string>
#include <fcntl.h>
#include <cstring>
#include <signal.h>
#include <wait.h>
#include <iostream>

using namespace std;

struct call_my {
    string name;
    vector<string> args;
    call_my(char* s, vector<char*> mas) {
        name = (const char*) s;
        for (int i = 0; i < mas.size(); i++) {
            args.push_back(mas[i]);
        }
    }
    call_my(string s, vector<char*> mas) {
        name = s;
        for (int i = 0; i < mas.size(); i++) {
            args.push_back(mas[i]);
        }
    }
    call_my(string s, vector<string> mas) {
        name = s;
        args = mas;
    }
    call_my(string s) {
        name = s;
    }
    call_my() {
    }
    call_my(char* s) {
        name = (const char*) s;
    }
};

vector <call_my> programs;
vector <call_my> empty;

ssize_t read_until(int fd, char* buf, size_t count)
{
    ssize_t have = 0;
    int res = 0;
    int in_quotes1 = 0;
    int in_quotes2 = 0;

    while (1)
    {
        have = read(fd, buf + res, count);

        for (int i = 0; i < have; i++) {
            if (buf [res + i] == 34) // "
                in_quotes1 = (in_quotes1 + 1) % 2;

            if (buf [res + i] == 39) // '
                in_quotes2 = (in_quotes2 + 1) % 2;

            if (buf[res + i] == '\n' && in_quotes1 == 0 && in_quotes2 == 0) {
                return res + i + 1;
            }
        }

        if (have == 0)
            return res;
        else if (have == -1)
            return -1;
        else if (have == count)
            return have + res;

        res += have;
        count -= have;
    }
}

void add_arg(string s) {
    call_my curr_call;
    curr_call = programs[programs.size() - 1];
    curr_call.args.push_back(s.data());
    programs[programs.size() - 1] = curr_call;
}

void parse(char* text1, int len) {
    string text = text1;
    int found_program = 0;
    programs.clear();
    int last = 0;
    int in_quotes = 0;

    if (text.substr(0, 4) == "exit") {
        exit(0);
    }

    for (int i = 0; i < len; i++) {
        if (!in_quotes && text[i] == ' ' && last - i == 0) {
            last = i + 1;  //случай нескольких пробелов
        }
        if (!in_quotes && text[i] == 39 && text[i - 1] == ' ') {
            in_quotes = 1;
            last = i + 1;
        } else if (in_quotes == 1 && text[i] == 39 && (i == len - 1 || text[i + 1] == ' ' || text[i + 1] == '|')) {
            in_quotes = 0;
            add_arg(text.substr((unsigned) last, (unsigned) i - last));
            last = i + 1;
        } else if (!in_quotes && i - last > 0) {
            if (text[i] == '|') {
                programs.push_back(call_my(text.substr((unsigned) last, (unsigned) i - last)));
                if (found_program == 0) {
                    add_arg(text.substr((unsigned) last, (unsigned) i - last));
                }
                found_program = 0;
                last = i + 1;
            }
            if (text[i] == ' ') {
                if (!found_program) {
                    programs.push_back(call_my(text.substr((unsigned) last, (unsigned) i - last)));
                    add_arg(text.substr((unsigned) last, (unsigned) i - last));
                    found_program = 1;
                } else {
                    add_arg(text.substr((unsigned) last, (unsigned) i - last));
                }
                last = i + 1;
            }
        } else {
            if (!in_quotes && text[i] == '|') {
                found_program = 0;
                last = i + 1;
            }
        }
    }

    if (len - last > 0) {
        if (found_program) {
            add_arg(text.substr((unsigned) last, (unsigned) len - last));
        } else {
            programs.push_back(call_my(text.substr((unsigned) last, (unsigned) len - last)));
            add_arg(text.substr((unsigned) last, (unsigned) len - last));
        }
    }
}

int* pids_global;
size_t pids_count;

void action(int num)
{
    for (int i = 0; i < pids_count; i++)
        kill(pids_global[i], SIGKILL);

    pids_count = 0;
}

int runpiped(call_my** programs, size_t n)
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

        char* arg_[programs[i] -> args.size() + 1];
        for (int j = 0; j < programs[i]->args.size(); j++) {
            arg_[j] = (char*) programs[i]->args[j].data();
        }
        arg_[programs[i] -> args.size()] = NULL;

        int res = execvp(programs[i] -> name.data(), arg_);

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

void nothing(int num) {}

int main(int argc, char** argv) {
    struct sigaction act;
    act.sa_handler = &nothing;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        return -1;
    }

    char buff[4096];

    while(true) {

        write(STDOUT_FILENO, "$ ", 2);

        if (errno != 0) {
            break;
        }

        ssize_t have_read = read_until(STDIN_FILENO, buff, 4096);

        if (have_read == 0) {
            break;
        }

        if (have_read == -1) {
            if (write(STDOUT_FILENO, "\n", 1) == -1) {
                return -1;
            }

            errno = 0;
            continue;
        }

        parse(buff, (int) have_read - 1);

        int size = programs.size();

        call_my* pr[size];
        call_my mas[size];
        for (int i = 0; i < size; i++) {
            mas[i] = call_my(programs[i].name, programs[i].args);
            pr[i] = &(mas[i]);
        }
        runpiped(pr, size);
    }
    return 0;
}
