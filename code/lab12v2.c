#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main (int argc, char** argv) {
    int fds[2];
    pid_t p;
    pipe(fds);
    if (argc != 3) {
        fprintf(stderr, "Usage: cpr input_directory output_directory\n");
        exit(1);
    }
    
    p = fork();
    if(p) {
        
        dup2(fds[1], 1);
        close(fds[0]);
        close(fds[1]);
        execl("/bin/tar", "tar", "cf", "-", argv[1], NULL);
    }
    else {

        dup2(fds[0], 0);
        close(fds[0]);
        close(fds[1]);
        if (chdir(argv[2]) != 0) {
            perror(argv[2]);
            exit(2);
        }

        execl("/bin/tar", "tar", "xf", "-", NULL);
    }
    return(0);
}