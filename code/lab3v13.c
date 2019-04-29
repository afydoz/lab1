#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <netdb.h>

#include <fcntl.h>

#include <pthread.h>

#define BF_SIZE 128

int sck_new = 0;

int sck1, sck2, i, pid;

int buf_len, buf_new_len, qry_len, rqv_len, fdt_len = 1;

int ip1, ip2, ip3, ip4, pthr_fl = 0;

socklen_t clnt1_name_len, clnt2_name_len;

char buf[BF_SIZE], buf_new[BF_SIZE], qry[BF_SIZE], rqv[BF_SIZE], fdt[BF_SIZE], fl_name[BF_SIZE];

char c, * host;

struct sockaddr_in clnt1_name, srv_name, clnt2_name;

struct hostent * hp;

void * read_sock1 (void * arg_p)

{

while (1)

{

buf_len = recv(sck1, buf, BF_SIZE, 0);

write(1, buf, buf_len);

}

}

void * read_sock_new (void * arg_p)

{

while (1)

{

buf_new_len = recv(sck_new, buf_new, BF_SIZE, 0);

write(1, buf_new, buf_new_len);

}

}

void * listen_port2 (void * arg_p)

{

while(1)

{

listen(sck2, 5);

sck_new = accept(sck2, (struct sockaddr *)&clnt1_name, &clnt1_name_len);

}

}

int main(int argc, char* argv[])

{

int ret, fd, pthr_cnt = 0;

pthread_t fsid_listen_sock2, fsid_read_sock1, fsid_read_sock_new;

pthread_attr_t pattr;

pthread_attr_init (&pattr);

pthread_attr_setscope (&pattr, PTHREAD_SCOPE_PROCESS);

if (argc > 1) // нужно задать адрес в аргументах

{

write(1, "Connect to ", 11);

write(1, argv[1], strlen(argv[1]));

write(1, " (Y/N)? ", 8); // выбор, хотим ли подключиться

while (1)

{

read (0, &c, 1);

if (c == 'Y' || c == 'y')

break;

if (c == 'N' || c == 'n')

{

read (0, &c, 1);

exit(0);

}

}

read (0, &c, 1);

}

else

{

write (1, "No address\n", 11);

exit(0);

}

memset ((char *)&clnt1_name, '\0', sizeof(clnt1_name));

memset ((char *)&clnt2_name, '\0', sizeof(clnt2_name));

memset ((char *)&srv_name, '\0', sizeof(srv_name));

hp = gethostbyname(argv[1]);

sck1 = socket(AF_INET, SOCK_STREAM, 0);

clnt1_name.sin_family = AF_INET;

clnt1_name.sin_addr.s_addr = INADDR_ANY;

clnt1_name.sin_port = htons(21211);

bind(sck1, (struct sockaddr *)&clnt1_name, sizeof(clnt1_name));

srv_name.sin_family = AF_INET;

bcopy(hp->h_addr, (char *)&srv_name.sin_addr, hp->h_length);

srv_name.sin_port = htons(21);

connect(sck1, (struct sockaddr *)&srv_name, sizeof(srv_name));

buf_len = recv(sck1, buf, BF_SIZE, 0);

write(1, buf, buf_len);

usleep(1000);

clnt1_name_len = sizeof(clnt1_name);

clnt2_name_len = sizeof(clnt2_name);

getsockname(sck1, (struct sockaddr *)&clnt2_name, &clnt2_name_len);

sscanf(inet_ntoa((struct in_addr)clnt2_name.sin_addr), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

sck2 = socket(AF_INET, SOCK_STREAM, 0);

clnt2_name.sin_family = AF_INET;

clnt2_name.sin_port = htons(21212);

bind(sck2, (struct sockaddr *)&clnt2_name, sizeof(clnt2_name));

if ( ret = pthread_create(&fsid_listen_sock2, &pattr, listen_port2, NULL) )

perror("pthread_create");

if ( ret = pthread_create(&fsid_read_sock1, &pattr, read_sock1, NULL) )

perror("pthread_create");

{

while (1)// прием комманд

{

strcpy (qry, "");

qry_len = read(0, qry, BF_SIZE);

if (!strncmp(qry, "end", 3))

{

send(sck1, "QUIT\n", 5, 0);

usleep(1000);

break;

}

if (!strncmp(qry, "test", 4))

{

send(sck1, "NOOP\n", 5, 0);

usleep(1000);

continue;

}

if (!strncmp(qry, "sys", 3))

{ send(sck1, "SYST\n", 5, 0);

usleep(1000);

continue;

}

if (!strncmp(qry, "login", 5))

{

strcpy(rqv, "USER ");

for (i = 0; i < qry_len; i++)

rqv[5 + i] = qry[6 + i];

send(sck1, rqv, 5 + qry_len, 0);

send(sck1, "\n", 1, 0);

usleep(250000);

write(1, "password ", 9);

qry_len = read(0, qry, BF_SIZE);

send(sck1, "PASS ", 5, 0);

send(sck1, qry, qry_len, 0);

usleep(1000);

continue;

}

if (!strncmp(qry, "dir", 3))

{

if (pthr_cnt) shutdown(sck_new, 2);

usleep(1000);

sprintf(rqv, "PORT %d,%d,%d,%d,82,220\r\n", ip1, ip2, ip3, ip4);

send(sck1, rqv, strlen(rqv), 0);

if (!pthr_cnt)

if ( ret = pthread_create(&fsid_read_sock_new, &pattr, read_sock_new, NULL) )

perror("pthread_create");

pthr_cnt++;

usleep(1000);

send(sck1, "LIST\n", 5, 0);

continue;

}

if (!strncmp(qry, "put ", 4))

{

sscanf(qry, "put %s\n", fl_name);

fd = open(fl_name, O_RDONLY);

if (fd == -1)

{

write (1, "Mini-ftp: Can't open file\n", 26);

}

else

{

sprintf(rqv, "PORT %d,%d,%d,%d,82,220\r\n", ip1, ip2, ip3, ip4);

send(sck1, rqv, strlen(rqv), 0);

usleep(1000);

send(sck1, "TYPE I\n", 7, 0);

usleep(1000);

sprintf(rqv, "STOR %s\r\n", fl_name);

send(sck1, rqv, strlen(rqv), 0);

sleep(1);

while (fdt_len > 0)

{

fdt_len = read(fd, fdt, BF_SIZE);

send(sck_new, fdt, fdt_len, 0);

//write(1, fdt, fdt_len);

}

shutdown(sck_new, 2);

write(1, "Mini-ftp: Transfer over\n", 24);

close (fd);

}

continue;

}

if (!strncmp(qry, "cdw ", 4))

{

sscanf(qry, "cdir %s\n", fl_name);

sprintf(rqv, "CWD %s\r\n", fl_name);

send(sck1, rqv, strlen(rqv), 0);

continue;

}

else

{

send(sck1, qry, qry_len, 0);

}

}

}

shutdown(sck1, 2);

close(sck1);

shutdown(sck2, 2);

close(sck2);

shutdown(sck_new, 2);

close(sck_new);

write(1, "Mini-ftp: Close\n", 16);

exit(0);

}