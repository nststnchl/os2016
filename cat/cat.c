#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    
    char buff[4096];
    int done = 0;
    int total_done = 0;
    while (1) {
        done = read(STDIN_FILENO, buff, 4096); 
        if (done == -1) {
            fprintf(stderr, "Reading error occured");
        }
        total_done += done;
        if (done == 0) {
            break;
        }
        while(1) {
            done = write(STDOUT_FILENO, buff, done);
            total_done -= done;
            if (total_done == 0) {
                break;
            }
            if (done == -1 || (total_done > 0 && done == 0)) {
                fprintf(stderr, "Writing error occured");
            }
        }
    }

    return 0;
}
