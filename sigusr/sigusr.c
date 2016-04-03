#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

int was = 0;

void act_handler(int sig, siginfo_t *sig_i, void *tmp)         
{
    was = 1;
    char* sig_name;
    if (sig == SIGUSR1) {
        sig_name = "SIGUSR1";
    } else {
        sig_name = "SIGUSR2";
    } 
    printf("%s from %d\n", sig_name, sig_i->si_pid);   
    exit(0);
}

int main(int argc, char *argv[]) 
{    
    struct sigaction act = (struct sigaction) {
        .sa_sigaction = act_handler,
        .sa_flags = SA_SIGINFO
    };
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sleep(10);    
    if (was == 0) 
        printf("No signals were caught\n");
    return 0;
}
