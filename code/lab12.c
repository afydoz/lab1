//gcc lab12.c -o cpr
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main (int argc, char** argv) {
    int fds[2];
    pid_t p;
    pipe(fds);
    
    if (argc != 3) {
        fprintf(stderr, "Usage: cpr input_directory output_directory\n");
        exit(1);
    }
    //printf("Original program,  pid=%d\n",  getpid());
    p = fork();
    if(p) {
        //printf("In parent,  pid=%d,  fork returned=%d\n",getpid(),  p);
        //fprintf(stderr, "%s", "fork 1\n");
        dup2(fds[1], 1); //1 - WRITE_END; 1 - STDOUT_FILENO
        close(fds[0]);
        close(fds[1]);
        execl("/bin/tar", "tar", "cf", "-", argv[1], NULL);
        fprintf(stderr, "Failed to execute 'tar cf - %s'\n", argv[1]);
        exit(4);
    }
    else {
        //fprintf(stderr, "%s", "fork 0\n");
        //printf("In child process,  pid=%d,  ppid=%d\n",      getpid(),  getppid());
        dup2(fds[0], 0); //0 - READ_END; 0 - STDIN_FILENO
        close(fds[0]);
        close(fds[1]);
        if (chdir(argv[2]) != 0) {
            //fprintf(stderr, "\n%s: %s", argv[2], strerror(errno));
            perror(argv[2]);
            //fflush(stderr);
            exit(3);
        }
        execl("/bin/tar", "tar", "xf", "-", NULL);
        fprintf(stderr, "Failed to execute 'tar xf -'\n");
        exit(4);
    }
    return(0);
}