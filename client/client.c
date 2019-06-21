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
#define LOCAL_IP "127.0.0.1"
//#define LOCAL_IP "18.139.78.88"
#define SERVER_PORT 2049
#define DATA_PORT 2048
#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1

int TYPE = 0;
const char *MODE[] = {"BINARY","ASCII"};
char buffer[FILEBUF_SIZE];
char line_in[MAX_INPUT_SIZE+1];
char *server_ip, *local_ip;
char port_addr[24] = "";
int server_port, local_port;
int server_sock, client_sock = -1, data_sock;
int z = 0;
struct sockaddr_in server_addr, client_addr, data_addr;
struct hostent *server_host;

/*
通过字符串进行匹配 在不超过限度的情况下 进行内容相关 替换
*/
int replace(char *str, char *what, char *by, int max_length);

//向服务器 发送信息
void send_msg(char *command, char *msg, int flag);

// 基础命令
void help_info();// 帮助
void user_login();// 用户登录
void command_quit();//用户退出
void command_syst(); //系统的提示信息
void command_type();// type命令
void command_pwd();// 显示当前服务器路径 
void command_cd();// 切换路劲
int command_port(); // 主动模式
int data_conn(char *ip, int port); //建立连接
void command_list(); // 显示当前信息
void command_get(char *filename); // 下载文件
void command_put(char *filename); //  上传文件


// 构建主函数 
int main(int argv, char **argc)
{
	// 如果 没有参数
    if(argv == 1)
    {
	server_ip = LOCAL_IP;
	server_port = SERVER_PORT;
    }
    else if(argv == 2)
    {
    	// 有一个参数
	server_ip = argc[1];
	server_port = SERVER_PORT;
    }
    else if(argv == 3)
    {
    	// 有两个参数
	server_ip = argc[1];
	server_port = atoi(argc[2]);
    }
    //connect to the ftp server
    server_host = gethostbyname(server_ip);
    if(server_host == (struct hostent *)NULL)
    {
	printf("(ftp)>gethostbyname failed\n");
	exit(1);
    }
    //建立ftp 连接
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, server_host->h_addr, server_host->h_length);
    server_addr.sin_port = htons(server_port);
    //获取socket端口
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(server_sock < 0)
    {
	printf("(ftp)>error on socket()\n");
	exit(1);
    }
    //连接到服务器
    if(connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
	printf("(ftp)>error on connect()\n");
	close(server_sock);
	exit(1);
    }
    
    //连接成功
    printf("(ftp)>Connected to %s:%d\n", server_ip, server_port);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    //printf("z = %d, buffer is '%s'\n",z,buffer);
    
    //登录信息
    user_login();
    //开始进入系统进行操作
    while(1)
    {
    	// 获取输入信息
	printf("(ftp)>");
	fgets(line_in, MAX_INPUT_SIZE, stdin);
	line_in[strlen(line_in)-1] = '\0';
	//执行命令
	// 退出
	if(strncmp("quit", line_in, 4) == 0)
	{
	    command_quit();
	    break;
	}
	else if((strncmp("?", line_in, 1) == 0) || (strncmp("help", line_in, 4) == 0))
	{
		//  帮助命令
	    help_info();
	}
	else if(strncmp("syst", line_in, 4) == 0)
	{
		// 系统参数命令
	    command_syst();
	}
	else if(strncmp("type", line_in, 4) == 0)
	{
		// 种类命令
	    command_type();
	}
	else if(strncmp("pwd", line_in, 3) == 0)
	{
		// 显示当前服务器命令
	    command_pwd();
	}
	else if(strncmp("cd", line_in, 2) == 0)
	{
		// 切换服务器路径命令
	    command_cd();
	}
	else if(strncmp("port", line_in, 4) == 0)
	{
		// port模式
	    command_port();
	}
	else if((strncmp("ls", line_in, 4) == 0) || (strncmp("dir", line_in, 3) == 0))
	{
		// 显示服务器文件
	    command_list();
	}
	else if(strncmp("get", line_in, 3) == 0)
	{
		// 下载文件
	    command_get(&line_in[4]);
	}
	else if(strncmp("put", line_in, 3) == 0)
	{
		// 上传文件
	    command_put(&line_in[4]);
	}
	
    }
    // 关闭socket连接
    close(server_sock);
    return 0;
}


/*
搜寻相关的命令进行 匹配替换 （不要超过指定的最长长度 max_login）
*/
int replace(char *str, char *what, char *by, int max_length)
{
    char *foo, *bar = str;
    int i = 0;
    int str_length, what_length, by_length;
    
    // 做一个检查 
    if (! str) return 0;
    if (! what) return 0;
    if (! by) return 0;
    
    what_length = strlen(what);
    by_length = strlen(by);
    str_length = strlen(str);
    
    foo = strstr(bar, what);
    /* 
    如果能替换且没有发生一处就可以替换 否则不可以替换
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

//向服务器发送信息
void send_msg(char *command, char *msg, int flag)
{
    char reply[MAX_INPUT_SIZE+1];
    if(flag == 0)
	sprintf(reply, "%s\r\n", command);
    else
	sprintf(reply, "%s %s\r\n", command, msg);
    write(server_sock, reply, strlen(reply));
    printf("%d-->%s",strlen(reply), reply);
    return;
}
// 发送帮助信息
void help_info()
{
	//直接客户端打印信息就好
    printf("?\tcd\tdir\tls\n");
    printf("help\tsyst\ttype\tport\n");
    printf("pwd\tget\tput\tquit\n");
}
//发送用户登录信息
void user_login()
{
	//发送登录信息到服务器
    printf("(ftp)>Name:");
    fgets(line_in, MAX_INPUT_SIZE, stdin);
    line_in[strlen(line_in)-1] = '\0';
    send_msg("USER", line_in, 1); // 发送用户名的信息
    
    z = read(server_sock, buffer, sizeof(buffer)); // 读取服务器中用户名的信息
    buffer[z-2] = 0;
    printf("%s\n", buffer); // 发音相关的返回信息
    //printf("z = %d, buffer is '%s'\n",z,buffer);
    if(strncmp("331", buffer, 3) == 0)
    {
    	// 输入密码界面
	printf("(ftp)>Password:");
	fgets(line_in, MAX_INPUT_SIZE, stdin);
	line_in[strlen(line_in)-1] = '\0';
	send_msg("PASS", line_in, 1);
	
	z = read(server_sock, buffer, sizeof(buffer));
	buffer[z-2] = 0;
	printf("%s\n", buffer);
	//printf("z = %d, buffer is '%s'\n",z,buffer);
    }
    line_in[0] = 0;
    command_syst();
    return;
}

// 显示系统信息 
void command_syst()
{
    send_msg("SYST", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

// 退出
void command_quit()
{
    send_msg("QUIT", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

//  显示类型
void command_type()
{
    char msg[2];
    msg[1] = 0;
    if(strncmp("ascii", &line_in[5], 5) == 0)
    {
	msg[0] = 'A';
	TYPE = TYPE_A;
    }
    else if(strncmp("binary", &line_in[5], 6) == 0)
    {
	msg[0] = 'I';
	TYPE = TYPE_I;
    }
    send_msg("TYPE", msg, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

// 显示服务器的当前路径
void command_pwd()
{
    send_msg("PWD", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

// 切换目录
void command_cd()
{
    send_msg("CWD", &line_in[3], 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

// 主动模式port
int command_port()
{
    if(client_sock < 0)
    {
	struct sockaddr_in local_addr;
	socklen_t local_addr_len = sizeof local_addr;
	memset(&local_addr, 0, sizeof local_addr);
	if(getsockname(server_sock, (struct sockaddr *)&local_addr, &local_addr_len) != 0)
	{
	    printf("error when trying to get the local addr\n");
	    return -1;
	}
	local_ip = inet_ntoa(local_addr.sin_addr);
	local_port = local_addr.sin_port;
	printf("local addr  %s:%d\n", local_ip, local_port);
	char client_port[8] = "";
	sprintf(client_port, "%d.%d", (int)(local_port/256), (int)(1 + local_port - 256 * (int)(local_port/256)));
	sprintf(port_addr, "%s.%s", local_ip, client_port);
	replace(port_addr, ".", ",", 24);
	
	if(data_conn(local_ip, local_port + 1) == -1) return -1;
    }
    
    send_msg("PORT", port_addr, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    //accept an request
    socklen_t data_addr_len = sizeof data_addr;
    memset(&data_addr, 0, sizeof data_addr);
    data_sock = accept(client_sock, (struct sockaddr *)&data_addr, &data_addr_len);
    if(data_sock < 0)
    {
	//close(client_sock);
	printf("data_sock accept failed!\n");
	return -1;
    }
    //close(client_sock);
    return 0;
}


// 数据连接过程
int data_conn(char *ip, int port)
{
    client_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(client_sock == -1)	//create socket failed
    {
	close(client_sock);
	printf("data socket() failed");
	return -1;
    }
    
    //configure server address,port
    memset(&client_addr, 0 ,sizeof(client_addr));
    socklen_t client_addr_len = sizeof client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    //client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_addr.s_addr = inet_addr(ip);
    if(client_addr.sin_addr.s_addr == INADDR_NONE)
    {
	printf("INADDR_NONE");
	return -1;
    }
    
    //bind
    z = bind(client_sock, (struct sockaddr *)&client_addr, client_addr_len);
    if(z == -1)
    {
	close(client_sock);
	perror("data_sock bind() failed\n");
	return -1;
    }
    
    //listen
    z = listen(client_sock, 1);
    if(z < 0)
    {
	close(client_sock);
	printf("data_sock listen() failed!\n");
	return -1;
    }
    printf("data conn listening...\n");
    return 0;
}


// 显示命令列表
void command_list()
{
    unsigned char databuf[PIPE_BUF];
    int bytes = 0, bytesread = 0;
    
    send_msg("TYPE", "A", 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    if(command_port() == -1) return;
    send_msg("LIST", "", 0);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    /* Read from the socket and write to stdout */
    while ((bytes = read(data_sock, databuf, sizeof(databuf)) > 0)) {
	//write(fileno(stdout), databuf, bytes);
	printf("%s", databuf);
	bytesread += bytes;
    }
    
    /* Close the socket */
    close(data_sock);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
}

// 下载文件
void command_get(char *filename)
{
    FILE *outfile;
    short file_open=0;
    unsigned char databuf[FILEBUF_SIZE];
    int bytes = 0, bytesread = 0;
    
    if(command_port() == -1) return;
    send_msg("RETR", filename, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    /* Read from the socket and write to the file */
    while ((bytes = read(data_sock, databuf, FILEBUF_SIZE)) > 0) {
	if (file_open == 0) {
	    /* Open the file the first time we actually read data */
	    if ((outfile = fopen(filename, "w")) == 0) {
		printf("fopen failed to open file");
		close(data_sock);
		return;
	    }
	    file_open = 1;
	}
	write(fileno(outfile), databuf, bytes);
	bytesread += bytes;
    }
    
    /* Close the file and socket */
    if (file_open != 0) fclose(outfile);
    close(data_sock);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    printf("%d bytes get.\n", bytesread);
}

//  上传文件
void command_put(char *filename)
{
    FILE *infile;
    unsigned char databuf[FILEBUF_SIZE] = "";
    int bytes, bytessend;
    
    infile = fopen(filename,"r");
    if(infile == 0)
    {
	perror("fopen() failed");
	//close(data_sock);
	return;
    }
    
    if(command_port() == -1) return;
    send_msg("STOR", filename, 1);
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
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
	bytessend += bytes;
	memset(&databuf, 0, FILEBUF_SIZE);
    }
    memset(&databuf, 0, FILEBUF_SIZE);
    
    fclose(infile);
    close(data_sock);
    
    z = read(server_sock, buffer, sizeof(buffer));
    buffer[z-2] = 0;
    printf("%s\n", buffer);
    
    printf("%d bytes send\n", bytessend);
    return;
}
