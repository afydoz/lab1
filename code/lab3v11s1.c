#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <ifaddrs.h>

#define SERVER_PORT 8081
#define MAX_COMMAND_SIZE 4

#define PORT "PORT %s,31,145\r\n"
#define USER "USER %s\r\n"
#define PASS "PASS %s\r\n"
#define PWD  "PWD\r\n"
#define STOR "STOR %s\r\n"
#define RETR "RETR %s\r\n"
#define LIST "LIST %s\r\n"

#define PRODUCTION

char buf[1024];
int bytes_read;
char *myIP;

typedef void (*sighandler) (int);

struct Connection {
    int sock;
    struct addrinfo *info;
} *toListen = NULL;

void toDefault(struct Connection *connection) {
    if (connection != NULL) {
        connection->sock = -1;
        connection->info = NULL;
    }
}

typedef void (*Commands) (struct Connection *);

int isOneAnswer(char *buf, int len) {
    int i = 0;
    while(i < len && !(buf[i - 1] == '\r' && buf[i] == '\n')) 
        ++i;
    if (i + 2 == len)
        return 0;
    if (i + 1 == len)
        return 0;
    return i + 1;
}

void extremeClose (int c){
    if (toListen != NULL && toListen->sock >= 0) {
        close(toListen->sock);
    }
    printf("Interupting programm. Closing connection.\n");
    exit(c);
}


void changePointsToCommas(char *str) {
    int len = strlen(str);
    int i;
    for (i = 0; i < len; ++i) {
        if (str[i] == '.')
            str[i] = ',';
    }
}

void changeCommasToPoints(char *str) {
    int len = strlen(str);
    int i;
    for (i = 0; i < len; ++i) {
        if (str[i] == ',')
            str[i] = '.';
    }
}

struct Connection *createConnection(const char *port, const char *ipAdress) {

    struct Connection *NewConnection = (struct Connection*)malloc(sizeof(struct Connection));
    toDefault(NewConnection);

    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (ipAdress == NULL)
        hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(ipAdress, port, &hints, &(NewConnection->info));

    NewConnection->sock = socket(NewConnection->info->ai_family, NewConnection->info->ai_socktype, NewConnection->info->ai_protocol);
    if(NewConnection->sock < 0) {
        perror("socket");
        extremeClose (1);
    }
    return NewConnection;
}

void createListeningPort(){
    if(toListen != NULL && toListen->sock >= 0) {
        close(toListen->sock);
        free(toListen);
    }

    changeCommasToPoints(myIP);
    toListen = createConnection("8081", myIP);

    if(bind(toListen->sock, toListen->info->ai_addr, toListen->info->ai_addrlen) < 0)
    {
        perror("bind");
        extremeClose (2);
    }

    listen(toListen->sock, 3);
    changePointsToCommas(myIP);

}


int putFile(struct Connection *connection, char* fileName) {

    int getFile = open(fileName, O_RDWR, 0664);
    int sock = accept(connection->sock, NULL, NULL);
    if(sock < 0)
    {
        perror("accept. Second socket.");
        extremeClose (3);
    }
    char buf[1024];
    int bytes_read;
    while(1)
    {
        bytes_read = read(getFile, buf, 1024);
        if(bytes_read <= 0){
            break;
        }
        bytes_read = send(sock, buf, bytes_read, 0);
    }

    close(getFile);

    close(sock);
    return 0;
}

int getList(struct Connection *connection) {

    int sock = accept(connection->sock, NULL, NULL);
    if(sock < 0)
    {
        perror("accept. Second socket.");
        extremeClose (3);
    }
    char buf[1024];
    int bytes_read;
    while(1)
    {
        bytes_read = recv(sock, buf, 1024, 0);
        if(bytes_read <= 0){
            break;
        }
        write(1, buf, bytes_read);
    }

    close(sock);
    return 0;
}

int getFile(struct Connection *connection, char* fileName) {

    int fileNameDescriptor = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0664);
    int sock = accept(connection->sock, NULL, NULL);
    if(sock < 0) {
        perror("accept. Second socket.");
        extremeClose (3);
    }
    char buf_here[1024];
    int bytes_read_here = 0;
    while((bytes_read_here = recv(sock, buf_here, 1000, 0)) > 0) {
        write(fileNameDescriptor, buf_here, bytes_read_here);
    }
    close(fileNameDescriptor); 
    close(sock);
    return 0;
}

int getCommand() {

    char* command = (char*) malloc (sizeof(char) * MAX_COMMAND_SIZE);
    int commandNum = -1;
    printf("ftp> ");
    scanf("%s", command);
    if (strcmp(command, "open") == 0) {
        commandNum = 1;
    }
    if (strcmp(command, "get") == 0) {
        commandNum = 2;
    }
    if (strcmp(command, "pwd") == 0) {
        commandNum = 3;
    }
    if (strcmp(command, "help") == 0) {
        commandNum = 4;
    }
    if (strcmp(command, "ls") == 0) {
        commandNum = 5;
    }
    if (strcmp(command, "quit") == 0) {
        commandNum = 0;
    }
    free(command);
    return commandNum;
}


int length(char* str) {
    int count = 0;
    while(str[count++] != '\n');
    return count;
}


void getAnswer(char *buf, int *bytes_read, int sock, int fileDescriptor) {
    *bytes_read = recv(sock, buf, 1024, 0);
    buf[(*bytes_read)++] = '\n';
    if(fileDescriptor > -1)
        write(fileDescriptor, buf, *bytes_read);
}

void help(struct Connection *toSend) {
    printf("open - open connection\n");
    printf("pwd\n");
    printf("get - get file\n");
    printf("put - put file\n");
    printf("quit\n");
    printf("help - help\n");
    printf("ls - list directory\n");
}

void openConnection(struct Connection *toSend){

    char* ipAdress = (char*) malloc (sizeof(char) * 15);
    do {
        scanf("%s", ipAdress);
    } while (strlen(ipAdress) == 0);

    *toSend = *(createConnection("21", ipAdress));

    if(connect(toSend->sock, toSend->info->ai_addr, toSend->info->ai_addrlen) < 0) {
        perror("connect");
        extremeClose (2);
    }
    free(ipAdress);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "220", 3) != 0) {
        toDefault(toSend);
    } else {


        printf("Write your login: ");
        char* username = (char*) malloc (sizeof(char) * 256);
        scanf("%s", username);
        char* user = (char*) malloc (sizeof(char) * (4 + 1 + strlen(username) + 2));
        sprintf(user, USER, username);
        free(username);

        send(toSend->sock, user, length(user)* sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);
        free(user);

        if (strncmp(buf, "331", 3) != 0) {
            toDefault(toSend);
        } else { 

            printf("Write your password: ");
            //В идеале тут перевести в режим неканонического ввода =)
            char* password = (char*) malloc (sizeof(char) * 256);
            scanf("%s", password);
            char* pass = (char*) malloc (sizeof(char) * (4 + 1 + strlen(password) + 2));
            sprintf(pass, PASS, password);
            free(password);

            send(toSend->sock, pass, length(pass) * sizeof(char), 0);
            free(pass);
            getAnswer(buf, &bytes_read, toSend->sock, 1);

            if (strncmp(buf, "230", 3) != 0) {
                toDefault(toSend);
            }
        }
    }

}

void ls(struct Connection *toSend) {

    if (toSend == NULL || toSend->info == NULL) {
        return;
    }
    char* dir = (char*) malloc (sizeof(char) * 15);
    scanf("%s", dir);

    char *port = (char*) malloc(sizeof(char) * (4 + 1 + 23 + 2));
    sprintf(port, PORT, myIP);

    char *list = (char*) malloc (sizeof(char) * (4 + 2));
    sprintf(list, LIST, dir);

    send(toSend->sock, port, length(port) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "200", 3) == 0) {

        send(toSend->sock, list, length(list) * sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);

        int wasSend = 0;

        if (strncmp(buf, "150", 3) == 0) {
            getList(toListen);
            if((wasSend = isOneAnswer(buf, bytes_read / (sizeof(char)))) == 0) {
                getAnswer(buf + wasSend, &bytes_read, toSend->sock, 1);
            }
        }
    }

    free(port);
    free(list);
    free(dir);
}

void get(struct Connection *toSend) {

    if (toSend == NULL || toSend->info == NULL) {
        return;
    }

    char* fileName = (char*) malloc (sizeof(char) * 256);

    scanf("%s", fileName);

    char *port = (char*) malloc(sizeof(char) * (4 + 1 + 23 + 2));
    sprintf(port, PORT, myIP);

    char *retr = (char*) malloc (sizeof(char) * (4 + 1 + strlen(fileName) + 2));
    sprintf(retr, RETR, fileName);
    char type[] = "TYPE I\r\n";
    send(toSend->sock, type, length(type) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    printf("%s\n", port);
    send(toSend->sock, port, length(port) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "200", 3) == 0) {

        send(toSend->sock, retr, length(retr) * sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);

        int wasSend = 0;

        if (strncmp(buf, "150", 3) == 0) {
            getFile(toListen, fileName);
            if((wasSend = isOneAnswer(buf, bytes_read / (sizeof(char)))) == 0) {
                getAnswer(buf + wasSend, &bytes_read, toSend->sock, 1);
            }
        }
    }

    free(fileName);
    free(port);
    free(retr);
}

void pwd(struct Connection *toSend) {

    if (toSend == NULL || toSend->info == NULL) {
        return;
    }

    char* pwd = (char*) malloc(sizeof(char) * 3 + 2);
    sprintf(pwd, PWD);

    send(toSend->sock, pwd, length(pwd) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    free(pwd);
}

void quit(struct Connection *toSend) {

    printf("Bye-bye\n");
}

void cycle(char *myIP) {

    struct Connection toSend;
    Commands masOfCommands[] = {quit, openConnection, get, pwd, help, ls};
    int commandNum;
    do {
        commandNum = getCommand();
        if (commandNum > -1)
            masOfCommands[commandNum](&toSend);
    } while(commandNum != 0);

    close(toSend.sock);
    close(toListen->sock);

    freeaddrinfo(toSend.info);
}

void setMyIP(char *myIP){
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                extremeClose (-4);
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
            }
            if (strcmp(ifa->ifa_name, "lo") != 0) {
                sprintf(myIP, "%s", host);
                return;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    myIP = (char*) malloc(sizeof(char) * 23);
    setMyIP(myIP);

    createListeningPort();
    //changePointsToCommas(myIP);

    signal (SIGINT, (sighandler) extremeClose);

    cycle(myIP);
    if(toListen != NULL)
        freeaddrinfo(toListen->info);
    return 0;
}