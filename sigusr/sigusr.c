#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

int was = 0;
int pid_n;
int sig_n;

void act_handler(int sig, siginfo_t *sig_i, void *tmp)         
{
    was = 1;
    pid_n = sig_i->si_pid;
    sig_n = sig;
}

int main(int argc, char *argv[]) 
{    
    struct sigaction act = (struct sigaction) {
        .sa_sigaction = act_handler,
        .sa_flags = SA_SIGINFO
    };
    int i;
    
    sigemptyset(&act.sa_mask);

    for(i = 1; i < 31; i++) {
        if (i != 9 && i != 19) {
            sigaddset(&act.sa_mask, i);
            sigaction(i, &act, NULL);
        }
    }
    sleep(10); 

    if (was == 0) 
        printf("No signals were caught\n");
    else 
        printf("%d from %d\n", sig_n, pid_n);

    return 0;
}
