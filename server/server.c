#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<dirent.h>

#define MAX_INPUT_SIZE 254
#define SERVER_PORT 2049
#define DATA_PORT 2048
#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1

void command_pwd(char *reply);
void command_cwd(char *dir);
void command_port(char *params, char *reply);
void command_list(char *reply);
void command_type(char *type, char *reply);
void command_retr(char *filename, char *reply);
void command_stor(char *filename, char *reply);

int TYPE;
const char *MODE[] = {"BINARY","ASCII"};
char buffer[FILEBUF_SIZE];
int z;
int server_sock, client_sock, data_sock;
struct sockaddr_in server_addr, client_addr, data_addr;
struct hostent *data_host;
int server_adr_len;
socklen_t client_addr_len;
int i = 0;

void bail(const char *on_what)
{
    perror(on_what);
    exit(1);
}

/*
Search through str looking for what. Replace any what with
the contents of by. Do not let the string get longer than max_length.
*/
int replace(char *str, char *what, char *by, int max_length)
{
    char *foo, *bar = str;
    int i = 0;
    int str_length, what_length, by_length;
    
    /* do a sanity check */
    if (! str) return 0;
    if (! what) return 0;
    if (! by) return 0;
    
    what_length = strlen(what);
    by_length = strlen(by);
    str_length = strlen(str);
    
    foo = strstr(bar, what);
    /* keep replacing if there is somethign to replace and it
    will no over-flow
    */
    while ( (foo) && ( (str_length + by_length - what_length) < (max_length - 1) ) ) 
    {
	bar = foo + strlen(by);
	memmove(bar, foo + strlen(what), strlen(foo + strlen(what)) + 1);
	memcpy(foo, by, strlen(by));
	i++;
	foo = strstr(bar, what);
	str_length = strlen(str);
    }
    return i;
}

int main(int argc, char **argv)
{
    printf("\nWelcome to lihui's ftp server!\n");
    
    //get the server socket
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(server_sock == -1)	//create socket failed
    {
	bail("socket() failed");
    }
    else printf("Socket created!\n");
    
    //configure server address,port
    memset(&server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(server_addr.sin_addr.s_addr == INADDR_NONE)
    {
	bail("INADDR_NONE");
    }
    
    server_adr_len = sizeof server_addr;
    
    //bind
    z = bind(server_sock, (struct sockaddr *)&server_addr, server_adr_len);
    if(z == -1)
    {
	bail("bind()");
    }
    else printf("Bind Ok!\n");
    
    //listen
    z = listen(server_sock, 5);
    if(z < 0)
    {
	printf("Server Listen Failed!");
	exit(1);
    }
    else printf("listening\n");
    
    //loop and wait for connection
    while(1)
    {
	TYPE = TYPE_I;
	//struct sockaddr_in client_addr;
	client_addr_len = sizeof(client_addr);
	i = 0;
	printf("wait for accept...\n");
	//accept an request
	client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
	if(client_sock < 0)
	{
	    //fprintf(stderr,"%s,accept(sock, (struct sockaddr *)&client_addr, &client_len)\n",strerror(errno));
	    //close(sock);
	    //return 5;
	    printf("Server Accept Failed!\n");
	    return 1;
	}
	else printf("Server Accept Succeed!New socket %d\n",client_sock);

	char reply[100] = "220 FTP server ready\r\n";
	write(client_sock, reply, strlen(reply));
	printf("--->%s",reply);
	while(1){
	    
	    //deal with commands
	    z = read(client_sock, buffer, sizeof(buffer));
	    buffer[z-2] = 0;
	    printf("z = %d, buffer is '%s'\n",z,buffer);
	    
	    char command[5];
	    strncpy(command, buffer, 3);
	    command[3] = 0;
	    
	    //char username[25] = "anonymous";
	    if(strcmp(command, "USE") == 0)//USER
	    {
		//stpcpy(username, &buffer[5]);
		//printf("username:%s\n",username);
		stpcpy(reply, "331 Password required\r\n");
		//stpcpy(reply, "230 User logged in\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "PAS") == 0)//PASS
	    {
		stpcpy(reply, "230 User logged in\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "SYS") == 0)//SYST
	    {
		stpcpy(reply, "215 UNIX TYPE: L8\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "PWD") == 0)//PWD
	    {
		stpcpy(reply, "257 ");
		command_pwd(reply);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "CWD") == 0)//CWD
	    {
		stpcpy(reply, "250 CWD command successful\r\n");
		command_cwd(&buffer[4]);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "POR") == 0)//PORT
	    {
		//stpcpy(reply, "200 PORT command successful\r\n");
		command_port(&buffer[5], reply);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "LIS") == 0)//LIST
	    {
		stpcpy(reply, "150 Opening ASCII mode data connection for file list\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
		command_list(reply);
		//stpcpy(reply, "226 Transfer complete\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "TYP") == 0)//TYPE
	    {
		command_type(&buffer[5], reply);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "RET") == 0)//RETR
	    {
		sprintf(reply, "150 Opening %s mode data connection for %s (? bytes)\r\n", MODE[TYPE], &buffer[5]);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
		command_retr(&buffer[5], reply);
		stpcpy(reply, "226 Transfer complete\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "STO") == 0)//STOR
	    {
		sprintf(reply, "150 Opening %s mode data connection for %s (? bytes)\r\n", MODE[TYPE], &buffer[5]);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
		command_stor(&buffer[5], reply);
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
	    }
	    else if(strcmp(command, "QUI") == 0)//QUIT
	    {
		stpcpy(reply, "221 Goodbye.\r\n");
		write(client_sock, reply, strlen(reply));
		printf("%d -->%s", strlen(reply), reply);
		break;
	    }
	}
	
	close(client_sock);
	printf("\n");
    }
    close(server_sock);
    return 0;
}




void command_pwd(char *reply)
{
    char buf[255];
    getcwd(buf,sizeof(buf)-1);
    strcat(reply,buf);
    strcat(reply,"\r\n");
    return;
}

void command_cwd(char *dir)
{
    
    chdir(dir);
}

void command_port(char *params, char *reply)
{
    unsigned long a0, a1, a2, a3, p0, p1, addr;
    sscanf(params, "%lu,%lu,%lu,%lu,%lu,%lu", &a0, &a1, &a2, &a3, &p0, &p1);
    addr = htonl((a0 << 24) + (a1 << 16) + (a2 << 8) + a3);
    if(addr != client_addr.sin_addr.s_addr)
    {
	stpcpy(reply, "500 The given address is not yours\r\n");
	return;
    }
    
    //setup the port for the data connection
    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = addr;
    data_addr.sin_port = htons((p0 << 8) + p1);
    
    //get the data cocket
    data_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(data_sock == -1)
    {
	bail("data_sock failed");
	stpcpy(reply, "500 Cannot get the data socket\r\n");
	return;
    }
    
    //connect to the client data socket
    if(connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
    {
	bail("connect()");
	stpcpy(reply, "500 Cannot connect to the data socket\r\n");
	return;
    }
    
    stpcpy(reply, "200 PORT command successful\r\n");
}


void command_list(char *reply)
{
    FILE *fcmd;
    char databuf[PIPE_BUF];
    int n;
    
    fcmd = popen("ls -l","r");
    if(fcmd == 0)
    {
	bail("popen() failed");
	stpcpy(reply, "500 Transfer error\r\n");
	close(data_sock);
	return;
    }
    memset(databuf, 0, PIPE_BUF - 1);
    while((n = read(fileno(fcmd), databuf, PIPE_BUF)) > 0)
    {
	replace(databuf, "\n", "\r\n", PIPE_BUF-1);
	printf("-->%s", databuf);
	write(data_sock, databuf, strlen(databuf));
	memset(databuf, 0, PIPE_BUF - 1);
    }
    memset(databuf, 0, PIPE_BUF - 1);
    if(pclose(fcmd) != 0)
    {
	bail("Non-zero return value from \"ls -l\"");
    }
    close(data_sock);
    stpcpy(reply, "226 Transfer complete\r\n");
    return;
}

void command_type(char *type, char *reply)
{
    switch(type[0])
    {
	case 'A':
	    TYPE = TYPE_A;
	    stpcpy(reply, "200 Type set to A\r\n");
	    break;
	case 'I':
	    TYPE = TYPE_I;
	    stpcpy(reply, "200 Type set to I\r\n");
	    break;
	default:
	    TYPE = TYPE_I;
	    stpcpy(reply, "200 Type set to I\r\n");
	    break;
    }
    return;
}

void command_retr(char *filename, char *reply)
{
    FILE *infile;
    unsigned char databuf[FILEBUF_SIZE] = "";
    int bytes;
    
    infile = fopen(filename,"r");
    if(infile == 0)
    {
	perror("fopen() failed");
	close(data_sock);
	return;
    }
    
    while((bytes = read(fileno(infile), databuf, FILEBUF_SIZE)) > 0)
    {
	if(TYPE == TYPE_A)
	{
	    replace((char *)databuf, "\r\n", "\n", FILEBUF_SIZE-1);
	    replace((char *)databuf, "\n", "\r\n", FILEBUF_SIZE-1);
	    write(data_sock, (const char *)databuf, strlen((const char *)databuf));
	}
	else if(TYPE == TYPE_I)
	{
	    write(data_sock, (const char *)databuf, bytes);
	}
	memset(&databuf, 0, FILEBUF_SIZE);
    }
    memset(&databuf, 0, FILEBUF_SIZE);
    
    fclose(infile);
    close(data_sock);
    stpcpy(reply, "226 Transfer complete\r\n");
    return;
}

void command_stor(char *filename, char *reply)
{
    FILE *outfile;
    unsigned char databuf[FILEBUF_SIZE];
    struct stat sbuf;
    int bytes = 0;
    
    if(stat(filename, &sbuf) == -1)
    {
	//stpcpy(reply, "450 Cannot create the file\r\n");
	//return;
	printf("File not exist,try to create a new one.\n");
    }

    outfile = fopen(filename, "w");
    if(outfile == 0)
    {
	bail("fopen() failed");
	stpcpy(reply, "450 Cannot create the file\r\n");
	close(data_sock);
	return;
    }
    
    while((bytes = read(data_sock, databuf, FILEBUF_SIZE)) > 0)
    {
	write(fileno(outfile), databuf, bytes);
    }
    
    fclose(outfile);
    close(data_sock);
    stpcpy(reply, "226 Transfer complete\r\n");
    return;
}
