#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    
    char buff[512];
    int done = 0;
    int total_done = 0;
    char text;
    while (1) {
        done = read(0, buff, 512); 
        total_done += done;
        text += buff;
        if (done == 0) {
            break;
        }
    }
    
    while (1) {
        done = write(1, text, 512);
        total_done -= done;
        if (total_done == 0) {
            break;
        }
    }

    return 0;
}
