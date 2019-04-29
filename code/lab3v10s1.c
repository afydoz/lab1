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

#define USER "USER %s\r\n"
#define PASS "PASS %s\r\n"
#define PWD  "PWD\r\n"
#define RETR "RETR %s\r\n"
#define LIST "LIST %s\r\n"
#define STOR "STOR %s\r\n"
#define PRODUCTION

char buf[1024];
int bytes_read;
char *myIP;

typedef void (*sighandler) (int);

struct Connection 
{
    int sock;
    struct addrinfo *info;
} *toListen = NULL,*toRecive=NULL;

void toDefault(struct Connection *connection)
{
    if (connection != NULL) {
        connection->sock = -1;
        connection->info = NULL;
    }
}
typedef void (*Commands) (struct Connection *);

int isOneAnswer(char *buf, int len)
{
    int i = 0;
    while(i < len && !(buf[i - 1] == '\r' && buf[i] == '\n')) 
        ++i;
    if (i + 2 == len)
        return 0;
    if (i + 1 == len)
        return 0;
    return i + 1;
}

void extremeClose (int c)
{
    if (toListen != NULL && toListen->sock >= 0)
	{
        close(toListen->sock);
    }
printf("Interupting programm. Closing connection.\n");
    exit(c);
}


void changePointsToCommas(char *str) 
{
    int len = strlen(str);
    int i;
    for (i = 0; i < len; ++i) 
	{
        if (str[i] == '.')
            str[i] = ',';
    }
}

void changeCommasToPoints(char *str) 
{
    int len = strlen(str);
    int i;
    for (i = 0; i < len; ++i)
	{
        if (str[i] == ',')
            str[i] = '.';
    }
}

struct Connection *createConnection(const char *port, const char *ipAdress) // создание сокета для соединения по потру и ip
{

    struct Connection *NewConnection = (struct Connection*)malloc(sizeof(struct Connection));
    toDefault(NewConnection);

    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; //AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (ipAdress == NULL)
        hints.ai_flags = AI_PASSIVE;
    printf("connection port:%s\n",port);
    status = getaddrinfo(ipAdress, port, &hints, &(NewConnection->info));

    NewConnection->sock = socket(NewConnection->info->ai_family, NewConnection->info->ai_socktype, NewConnection->info->ai_protocol);
    if(NewConnection->sock < 0) 
	{
        perror("socket");
        extremeClose (1);
    }
    return NewConnection;
}

int putFile(struct Connection *connection, char* fileName)
{

    int getFile = open(fileName, O_RDWR, 0664);
    char buf[1024];
    int bytes_read;
    while(1)
    {
        bytes_read = read(getFile, buf, 1024);
        if(bytes_read <= 0){
            break;
        }
        bytes_read = send(connection->sock, buf, bytes_read, 0);
    }

    close(getFile);
    close(connection->sock);
    freeaddrinfo(connection->info);
    free(connection);
    return 0;
}


int getList(struct Connection *connection)
{

    char buf[1024];
    int bytes_read;
    while(1)
    {
        bytes_read = recv(connection->sock, buf, 1024, 0);
        if(bytes_read <= 0){
            break;
        }
        write(1, buf, bytes_read);
    }

    close(connection->sock);
    freeaddrinfo(connection->info);
    free(connection);
    return 0;
}

int getFile(struct Connection *connection, char* fileName) 
{

    int fileNameDescriptor = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0664);
    printf("%s\n",fileName);
    char buf_here[1024];
    int bytes_read_here = 0;
    while((bytes_read_here = recv(connection->sock, buf_here, 1000, 0)) > 0) {
        write(fileNameDescriptor, buf_here, bytes_read_here);
    }
    close(fileNameDescriptor); 
    close(connection->sock);
    freeaddrinfo(connection->info);
    free(connection);
    return 0;
}

int getCommand()//получение команды 
{
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
    if (strcmp(command, "put") == 0) {
        commandNum = 3;
    }
    if (strcmp(command, "pwd") == 0) {
        commandNum = 4;
    }
    if (strcmp(command, "help") == 0) {
        commandNum = 5;
    }
    if (strcmp(command, "ls") == 0) {
        commandNum = 6;
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


void getAnswer(char *buf, int *bytes_read, int sock, int fileDescriptor)//
{
    *bytes_read = recv(sock, buf, 1024, 0);
    buf[(*bytes_read)++] = '\n';
    if(fileDescriptor > -1)
        write(fileDescriptor, buf, *bytes_read);
}

void help(struct Connection *toSend)// вывод списка возможных команд
{
    printf("open - open connection\n");
    printf("pwd\n");
    printf("get - get file\n");
    printf("put - put file\n");
    printf("quit\n");
    printf("help - help\n");
    printf("ls - list directory\n");
}

void openConnection(struct Connection *toSend)// создание управляющего соединения
{

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

    if (strncmp(buf, "220", 3) != 0) 
        toDefault(toSend);
	else 
	{

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
        }
		else
		{ 

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

            if (strncmp(buf, "230", 3) != 0)
			    toDefault(toSend);
        }
    }

}

void ls(struct Connection *toSend)// оттображение файлов в текущем каталоге
{
	int i=0;
	int flag = 0;
	int tempNumber = 0;
	int mas[6] = {0, 0, 0, 0, 0, 0};
	int j = 0;
    	char* ipAdress_toRecive = (char*) malloc (sizeof(char) * 15);
	char* port_toRecive = (char*) malloc (sizeof(char) * 5);

    if (toSend == NULL || toSend->info == NULL) 
        return;
    char* dir = (char*) malloc (sizeof(char) * 15);
    scanf("%s", dir);

    char *list = (char*) malloc (sizeof(char) * (4 + 2));
    sprintf(list, LIST, dir);

    char type[] = "TYPE I\r\n";
    send(toSend->sock, type, length(type) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);
    if (strncmp(buf, "200", 3) == 0) 
	{

    char pasv[] ="PASV\r\n";

    send(toSend->sock, pasv, length(pasv) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "227", 3) == 0) {
	for (; i < length(buf); ++i)
	{
		if (buf[i] == '(')
		flag = 1;
		if (isdigit(buf[i]) && flag == 1)
			tempNumber = tempNumber * 10 + buf[i] - '0';
		if (buf[i] == ',' || buf[i] == ')')
		{
			mas[j++] = tempNumber;
			tempNumber = 0;
		}
	}
	sprintf(ipAdress_toRecive,"%d.%d.%d.%d",mas[0],mas[1],mas[2],mas[3]);

  	sprintf(port_toRecive,"%d",mas[4]*256+mas[5]);

	toRecive = createConnection(port_toRecive, ipAdress_toRecive);
	if(connect(toRecive->sock, toRecive->info->ai_addr, toRecive->info->ai_addrlen) < 0)
	{
		perror("connect");
		extremeClose (2);
	}
	
	send(toSend->sock, list, length(list) * sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);

        int wasSend = 0;

        if (strncmp(buf, "150", 3) == 0) 
		{
            getList(toRecive);
            if((wasSend = isOneAnswer(buf, bytes_read / (sizeof(char)))) == 0) 
			{
                getAnswer(buf + wasSend, &bytes_read, toSend->sock, 1);
            }
        }
    }
    }
    free(port_toRecive);
    free(ipAdress_toRecive);
    free(list);
    free(dir);
}

void cwd(struct Connection *toSend)//показать путь до текущей дериктории на сервере
{
	int i=0;
	int flag = 0;
	int tempNumber = 0;
	int mas[6] = {0, 0, 0, 0, 0, 0};
	int j = 0;
    	char* ipAdress_toRecive = (char*) malloc (sizeof(char) * 15);
char* port_toRecive = (char*) malloc (sizeof(char) * 5);


    if (toSend == NULL || toSend->info == NULL) 
        return;

    char* dir = (char*) malloc (sizeof(char) * 15);
    scanf("%s", dir);

    char *list = (char*) malloc (sizeof(char) * (4 + 2));
    sprintf(list, LIST, dir);

    char type[] = "TYPE I\r\n";
    send(toSend->sock, type, length(type) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    char pasv[] ="PASV\r\n";

    send(toSend->sock, pasv, length(pasv) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "227", 3) == 0) 
	{
	for (; i < length(buf); ++i)
	{
		if (buf[i] == '(')
		flag = 1;
		if (isdigit(buf[i]) && flag == 1)
			tempNumber = tempNumber * 10 + buf[i] - '0';
		if (buf[i] == ',' || buf[i] == ')')
		{
			mas[j++] = tempNumber;
			tempNumber = 0;
		}
	}
	sprintf(ipAdress_toRecive,"%d.%d.%d.%d",mas[0],mas[1],mas[2],mas[3]);

  	sprintf(port_toRecive,"%d",mas[4]*256+mas[5]);

	toRecive = createConnection(port_toRecive, ipAdress_toRecive);
	if(connect(toRecive->sock, toRecive->info->ai_addr, toRecive->info->ai_addrlen) < 0) 
	{
		perror("connect");
		extremeClose (2);
	}
	
	send(toSend->sock, list, length(list) * sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);

        int wasSend = 0;

        if (strncmp(buf, "150", 3) == 0) 
		{
            getList(toRecive);
            if((wasSend = isOneAnswer(buf, bytes_read / (sizeof(char)))) == 0) 
			{
                getAnswer(buf + wasSend, &bytes_read, toSend->sock, 1);
            }
        }
    }
    free(port_toRecive);
    free(ipAdress_toRecive);
    free(list);
    free(dir);
}
void get(struct Connection *toSend)//получение файла
{
     	int i=0;
	int flag = 0;
	int tempNumber = 0;
	int mas[6] = {0, 0, 0, 0, 0, 0};
	int j = 0;
    	char* ipAdress_toRecive = (char*) malloc (sizeof(char) * 15);
	char* port_toRecive = (char*) malloc (sizeof(char) * 5);

    if (toSend == NULL || toSend->info == NULL) 
        return;

    char* fileName = (char*) malloc (sizeof(char) * 256);
    char* nameFile = (char*) malloc (sizeof(char) * 256);
    scanf("%s", fileName);

    char *retr = (char*) malloc (sizeof(char) * (4 + 1 + strlen(fileName) + 2));
    sprintf(retr, RETR, fileName);

    char type[] = "TYPE I\r\n"; // передача файла в двоичном формате
    send(toSend->sock, type, length(type) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);
    if (strncmp(buf, "200", 3) == 0) {

    char pasv[] ="PASV\r\n";// перевод сервера в пассивный режим

    send(toSend->sock, pasv, length(pasv) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "227", 3) == 0) 
	{
	for (; i < length(buf); ++i)
	{
		if (buf[i] == '(')
		flag = 1;
		if (isdigit(buf[i]) && flag == 1)
			tempNumber = tempNumber * 10 + buf[i] - '0';
		if (buf[i] == ',' || buf[i] == ')')
		{
			mas[j++] = tempNumber;
			tempNumber = 0;
		}
	}
	sprintf(ipAdress_toRecive,"%d.%d.%d.%d",mas[0],mas[1],mas[2],mas[3]);

  	sprintf(port_toRecive,"%d",mas[4]*256+mas[5]);

	toRecive = createConnection(port_toRecive, ipAdress_toRecive);// создание соединения с полученным портом от сервера для передачи данных
	if(connect(toRecive->sock, toRecive->info->ai_addr, toRecive->info->ai_addrlen) < 0) 
	{
		perror("connect");
		extremeClose (2);
	}
	
	send(toSend->sock, retr, length(retr) * sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);

        int wasSend = 0;
        if (strncmp(buf, "150", 3) == 0)
		{
	       sprintf(nameFile,"%s",basename(fileName));

               getFile(toRecive,nameFile );// запись файла на стороне клиента
            if((wasSend = isOneAnswer(buf, bytes_read / (sizeof(char)))) == 0)
			{
                getAnswer(buf + wasSend, &bytes_read, toSend->sock, 1);
            }
        }
    }
    }
    free(port_toRecive);
    free(ipAdress_toRecive);
    free(fileName);
    free(nameFile);
    free(retr);
}


void put(struct Connection *toSend)//Передача файла на сервер
{

	int i=0;
	int flag = 0;
	int tempNumber = 0;
	int mas[6] = {0, 0, 0, 0, 0, 0};
	int j = 0;
    	char* ipAdress_toRecive = (char*) malloc (sizeof(char) * 15);
	char* port_toRecive = (char*) malloc (sizeof(char) * 5);

    if (toSend == NULL || toSend->info == NULL) 
		return;

    char* fileName = (char*) malloc (sizeof(char) * 256);
    char* nameFile = (char*) malloc (sizeof(char) * 256);
    scanf("%s", fileName);
    
    char *stor = (char*) malloc (sizeof(char) * (4 + 1 + strlen(fileName) + 2));
    sprintf(stor, STOR, fileName);

    char type[] = "TYPE I\r\n";
    send(toSend->sock, type, length(type) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);
    if (strncmp(buf, "200", 3) == 0) 
	{

    char pasv[] ="PASV\r\n";

    send(toSend->sock, pasv, length(pasv) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    if (strncmp(buf, "227", 3) == 0) 
	{
	for (; i < length(buf); ++i)
	{
		if (buf[i] == '(')
		flag = 1;
		if (isdigit(buf[i]) && flag == 1)
			tempNumber = tempNumber * 10 + buf[i] - '0';
		if (buf[i] == ',' || buf[i] == ')')
		{
			mas[j++] = tempNumber;
tempNumber = 0;
		}
	}
	sprintf(ipAdress_toRecive,"%d.%d.%d.%d",mas[0],mas[1],mas[2],mas[3]);

  	sprintf(port_toRecive,"%d",mas[4]*256+mas[5]);

	toRecive = createConnection(port_toRecive, ipAdress_toRecive);
	if(connect(toRecive->sock, toRecive->info->ai_addr, toRecive->info->ai_addrlen) < 0) 
	{
		perror("connect");
		extremeClose (2);
	}
	
	send(toSend->sock, stor, length(stor) * sizeof(char), 0);
        getAnswer(buf, &bytes_read, toSend->sock, 1);

        int wasSend = 0;

        if (strncmp(buf, "150", 3) == 0) {
	      // sprintf(nameFile,"%s",basename(fileName));
               putFile(toRecive,fileName );
            if((wasSend = isOneAnswer(buf, bytes_read / (sizeof(char)))) == 0) {
                getAnswer(buf + wasSend, &bytes_read, toSend->sock, 1);
            }
        }
    }
    }
    free(port_toRecive);
    free(ipAdress_toRecive);
    free(fileName);
    free(nameFile);
    free(stor);
}

void pwd(struct Connection *toSend)//Показать текущую дерикторию
{
    if (toSend == NULL || toSend->info == NULL) {
        return;
    }

    char* pwd = (char*) malloc(sizeof(char) * 3 + 2);
    sprintf(pwd, PWD);

    send(toSend->sock, pwd, length(pwd) * sizeof(char), 0);
    getAnswer(buf, &bytes_read, toSend->sock, 1);

    free(pwd);
}

void quit(struct Connection *toSend)//закрытие соединения с сервером
{
    printf("Bye-bye\n");
	close(toSend->sock);
    freeaddrinfo(toSend->info);
}

void cycle(char *myIP) 
{ //основной цикл работы с сервером

    struct Connection toSend;
    Commands masOfCommands[] = {quit, openConnection, get, put, pwd, help, ls};

    int commandNum;

    do
	{
        commandNum = getCommand();//ввод команды 
        if (commandNum > -1)
            masOfCommands[commandNum](&toSend);// выполнение комманды
    }while(commandNum != 0);

}

void setMyIP(char *myIP)//узнать ip клиента(свой)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) 
	{
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
	{
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) 
		{
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) 
			{
                extremeClose (-4);
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
            }
            if (strcmp(ifa->ifa_name, "lo") != 0) 
			{
                sprintf(myIP, "%s", host);
                return;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    myIP = (char*) malloc(sizeof(char) * 23);
    setMyIP(myIP);//получение своего ip

    signal (SIGINT, (sighandler) extremeClose);// экстренное завершение и закрытие соединения

    cycle(myIP);// цикл обработки команд

    return 0;
}
