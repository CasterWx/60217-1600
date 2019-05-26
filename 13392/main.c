#include	<stdio.h>
#include 	<termios.h>
#include	<stdlib.h>
#include	<netdb.h>
#include	<errno.h>
#include 	<sys/types.h>
#include 	<sys/stat.h>
#include 	<fcntl.h>
#include 	<sys/socket.h>
#include 	<ctype.h>
#include 	<unistd.h>
#include 	<string.h>
#include 	<sys/ioctl.h>


char	*ftphostName;
int getpasswd(char *pwd, int size);
int getusername(char *user, int size);
int mygetch();
int cliopen( char *ftphostName);
int quit_toFtp( int sockCs);
int ml_type( int sockCs, char modeType );
int ml_cwd( int sockCs, char *pathInFtp );
int ml_cdup( int sockCs );
int ml_mkd( int sockCs, char *pathInFtp );
int ml_list( int sockCs);
int ml_get( int sockCs, char *s);
int ml_put( int sockCs, char *s);
int ml_renamefile( int sockCs, char *s, char *d );
int ml_deletefile( int sockCs, char *s );


 
int mygetch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
 
int getpasswd(char *pwd, int size)
{
    int c, n = 0;
    do
    {
        c = mygetch();
        if (c != '\n' && c != '\r' && c != 127)
        {
            pwd[n] = c;
            printf("*");
            n++;
        }
        else if ((c != '\n' | c != '\r') && c == 127)//判断是否是回车或则退格
        {
            if (n > 0)
            {
                n--;
                printf("\b \b");//输出退格
            }
        }
    }while (c != '\n' && c != '\r' && n < (size - 1));
    pwd[n] = '\0';//消除一个多余的回车
    return n;
}
int getusername(char *user, int size)
{
    int c, n = 0;
    do
    {
        c = mygetch();
        if (c != '\n' && c != '\r' && c != 127)
        {
            user[n] = c;
            printf("%c",c);
            n++;
        }
        else if ((c != '\n' | c != '\r') && c == 127)//判断是否是回车或则退格
        {
            if (n > 0)
            {
                n--;
                printf("\b \b");//输出退格
            }
        }
    }while (c != '\n' && c != '\r' && n < (size - 1));
    user[n] = '\0';//消除一个多余的回车
    return n;
}


// 连接指定host
int 
cliopen( char *ftphostName)
{
    int     sockCs; 
	printf("Connect to %s .\n",ftphostName);
    sockCs = connect_server( ftphostName, 21 );
    if ( sockCs == -1 ) return -1;
   
	char userArr[126] ;
	char pwdArr[126] ;
	// 输入用户名密码
	int index = 0;
	printf("USER:");
	getusername(userArr, 126);
	printf("\n");
	printf("PWD:");
    getpasswd(pwdArr, 126);
	printf("\n");
    if ( login_server( sockCs, userArr, pwdArr ) == -1 ) {
       	printf("Login error.\n");
		close( sockCs );
        return -1;
    }
	printf("230 Login successful.");
    return sockCs;
}
 

/* 创建TCP连接 */
int socket_connect(char *ftphostName,int port)
{
    struct sockaddr_in addressSock;
    int s, useOpValue;
    socklen_t slenNum;
    
    useOpValue = 8;
    slenNum = sizeof(useOpValue);
    memset(&addressSock, 0, sizeof(addressSock));
    
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        setsockopt(s, IPPROTO_IP, IP_TOS, &useOpValue, slenNum) < 0)
        return -1;
    
    struct timeval timeo = {15, 0};
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
    
    addressSock.sin_family = AF_INET;
    addressSock.sin_port = htons((unsigned short)port);
    
    struct hostent* server = gethostbyname(ftphostName);
    if (!server)
        return -1;
    
    memcpy(&addressSock.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(s, (struct sockaddr*) &addressSock, sizeof(addressSock)) == -1)
        return -1;
    
    return s;
}
 
 // 连接服务端
int connect_server( char *ftphostName, int port )
{    
    int       sockCtrlNum;
    char      bufArr[512];
    int       returnNum;
    ssize_t   len;
    
    sockCtrlNum = socket_connect(ftphostName, port);
    if (sockCtrlNum == -1) {
	printf("socket connect error.\n");
        return -1;
    }
    
    len = recv( sockCtrlNum, bufArr, 512, 0 );
    bufArr[len] = 0;
    printf("%s",bufArr);
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum != 220 ) {
	printf("connect error.\n");
        close( sockCtrlNum );
        return -1;
    }
    
    return sockCtrlNum;
}
 
 // 发送命令
int sendCommandToftp_re( int sock, char *cmd, void *re_bufArr, ssize_t *len)
{
    char        bufArr[512];
    ssize_t     r_len;
    
    if ( send( sock, cmd, strlen(cmd), 0 ) == -1 )
        return -1;
    
    r_len = recv( sock, bufArr, 512, 0 );
    if ( r_len < 1 ) return -1;
    bufArr[r_len] = 0;
    
    if (len != NULL) *len = r_len;
    if (re_bufArr != NULL) sprintf(re_bufArr, "%s", bufArr);
    
    return 0;
}
 
 // 发送命令
int sendCommandToftp( int sock, char *cmd )
{
    char     bufArr[512];
    int      returnNum;
    ssize_t  len;
    
    returnNum = sendCommandToftp_re(sock, cmd, bufArr, &len);
    if (returnNum == 0)
    {
        sscanf( bufArr, "%d", &returnNum );
    }
    
    return returnNum;
}
 
 // 登录
int login_server( int sock, char *user, char *pwd )
{
    char    bufArr[128];
    int     returnNum;
    
    sprintf( bufArr, "USER %s\r\n", user );
    returnNum = sendCommandToftp( sock, bufArr );
    if ( returnNum == 230 ) return 0;
    else if ( returnNum == 331 ) {
        sprintf( bufArr, "PASS %s\r\n", pwd );
        if ( sendCommandToftp( sock, bufArr ) != 230 ) return -1;
        return 0;
    }
    else
        return -1;
}
 
 // PORT 模式
int portMode( int sockCtrlNum )
{
    int     sockLsnNum;
    int     port;
    int     len;
    struct sockaddr_in sin;
    char    cmd[128];
    
    sockLsnNum = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( sockLsnNum == -1 ) return -1;
    memset( (char *)&sin, 0, sizeof(sin) );
    sin.sin_family = AF_INET;
	// 主动模式开放端口
    sin.sin_port = 13392 ;

    if( bind(sockLsnNum, (struct sockaddr *)&sin, sizeof(sin)) == -1 ) {
        close( sockLsnNum );
        return -1;
    }
    
    if( listen(sockLsnNum, 2) == -1 ) {
        close( sockLsnNum );
        return -1;
    }
    
    len = sizeof( struct sockaddr );
    if ( getsockname( sockLsnNum, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( sockLsnNum );
        return -1;
    }
    port = sin.sin_port;
    
    if( getsockname( sockCtrlNum, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( sockLsnNum );
        return -1;
    }
    
    sprintf( cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
            sin.sin_addr.s_addr&0x000000FF,
            (sin.sin_addr.s_addr&0x0000FF00)>>8,
            (sin.sin_addr.s_addr&0x00FF0000)>>16,
            (sin.sin_addr.s_addr&0xFF000000)>>24,
             port>>8,port&0xff );
    printf("%s(%d*256+%d=%d)\n",cmd,port&0xff,port>>8,((port>>8)*256)+(port&0xff));
    if ( sendCommandToftp( sockCtrlNum, cmd ) != 200 ) {
        close( sockLsnNum );
        return -1;
    }
    return sockLsnNum;
}
 
 // 被动模式
int pasvMode( int sockCs )
{
    int     r_sock;
    int     send_re;
    ssize_t len;
    int     addr[6];
    char    bufArr[512];
    char    re_bufArr[512];
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "PASV\r\n");
    send_re = sendCommandToftp_re( sockCs, bufArr, re_bufArr, &len);
    if (send_re == 0) { 
	// 服务端开放端口
        sscanf(re_bufArr, "%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    	printf("%s\n",re_bufArr);
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    r_sock = socket_connect(bufArr,addr[4]*256+addr[5]);
    return r_sock;
}
 
int 
ml_type( int sockCs, char modeType )
{
    char    bufArr[128];
    sprintf( bufArr, "TYPE %c\r\n", modeType );
    if ( sendCommandToftp( sockCs, bufArr ) != 200 )
        return -1;
    else
        return 0;
}

// pwd命令
int 
ftp_pwd( int sockCs)
{
	char bufArr[128];
	int re;
	sprintf( bufArr, "PWD");
	re = sendCommandToftp(sockCs, bufArr);
	if (re!=250)
		return -1 ;
	return 0 ;
} 

// cwd命令
int 
ml_cwd( int sockCs, char *pathInFtp )
{
    char    bufArr[128];
    int     re;
    sprintf( bufArr, "CWD %s\r\n", pathInFtp );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 250 )
        return -1;
    else
        return 0;
}
 
 // cdup命令
int 
ml_cdup( int sockCs )
{
    int     re;
    re = sendCommandToftp( sockCs, "CDUP\r\n" );
    if ( re != 250 )
        return re;
    else
        return 0;
}
 
 // mkdir 命令
int 
ml_mkd( int sockCs, char *pathInFtp )
{
    char    bufArr[512];
    int     re;
    sprintf( bufArr, "MKD %s\r\n", pathInFtp );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 257 )
        return re;
    else
        return 0;
}


// ll命令
int 
ml_list( int sockCs)
{
    int     d_sock;
    char    bufArr[512];
    int     send_re;
    int     returnNum;
    ssize_t     len,bufArr_len,total_len; 	
	void ** data ;
	unsigned long long *data_len ;
    d_sock = pasvMode(sockCs);
    if (d_sock == -1) {
    	printf("pasv error");
	    return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "LIST %s\r\n");
    send_re = sendCommandToftp( sockCs, bufArr );
    printf("%d\n",send_re);
    if (send_re >= 300 || send_re == 0)
        return send_re;   
    len=total_len = 0;
    bufArr_len = 512;
    void *re_bufArr = malloc(bufArr_len);
    while ( (len = recv( d_sock, bufArr, 512, 0 )) > 0 )
    {
        if (total_len+len > bufArr_len)
        {
            bufArr_len *= 2;
            void *re_bufArr_n = malloc(bufArr_len);
            memcpy(re_bufArr_n, re_bufArr, total_len);
            free(re_bufArr);
            re_bufArr = re_bufArr_n;
        }
	if (bufArr!=NULL)
		printf("%s\n",bufArr);
        memcpy(re_bufArr+total_len, bufArr, len);
	total_len += len;
    }
    close( d_sock );
   	
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum != 226 )
    {
        free(re_bufArr);
        return returnNum;
    }
    
    return 0;
}
 
 // 下载文件
int 
ml_get( int sockCs, char *s )
{
    int     d_sock;
    ssize_t     len,write_len;
    char    bufArr[512];
    int     handle;
    int     returnNum;
    handle = open( s,  O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE );
    printf("handle %d\n",handle);
    if ( handle == -1 ) return -1;
    
    ml_type(sockCs, 'I');
    
    d_sock = pasvMode(sockCs);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "RETR %s\r\n", s );
    returnNum = sendCommandToftp( sockCs, bufArr );
    printf("%d\n",returnNum);
    if (returnNum >= 300 || returnNum == 0)
    {
        close(handle);
        return returnNum;
    }
    
    bzero(bufArr, sizeof(bufArr));
    while ( (len = recv( d_sock, bufArr, 512, 0 )) > 0 ) {
        write_len = write( handle, bufArr, len );
        if (write_len != len )
        {
            close( d_sock );
            close( handle );
            return -1;
        }
        
    }
    close( d_sock );
    close( handle );
    
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum >= 300 ) {
        return returnNum;
    }
    return 0;
}
 
 
 // 上传文件
int 
ml_put( int sockCs, char *s)
{
    int     d_sock;
    ssize_t     len,send_len;
    char    bufArr[512];
    int     handle;
    int send_re;
    int returnNum;

    handle = open( s,  O_RDONLY);
    if ( handle == -1 ) return -1;
    
    ml_type(sockCs, 'I');
    
    d_sock = pasvMode(sockCs);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(bufArr, sizeof(bufArr));
    sprintf( bufArr, "STOR %s\r\n", s );
    send_re = sendCommandToftp( sockCs, bufArr );
    if (send_re >= 300 || send_re == 0)
    {
        close(handle);
        return send_re;
    }
    bzero(bufArr, sizeof(bufArr));
    while ( (len = read( handle, bufArr, 512)) > 0)
    {
        send_len = send(d_sock, bufArr, len, 0);
        if (send_len != len )
        {
            close( d_sock );
            close( handle );
            return -1;
        }
    }
    close( d_sock );
    close( handle );
    bzero(bufArr, sizeof(bufArr));
    len = recv( sockCs, bufArr, 512, 0 );
    bufArr[len] = 0;
    sscanf( bufArr, "%d", &returnNum );
    if ( returnNum >= 300 ) {
        return returnNum;
    }
    return 0;
}
 
// 修改文件名
int 
ml_renamefile( int sockCs, char *s, char *d )
{
    char    bufArr[512];
    int     re;
    
    sprintf( bufArr, "RNFR %s\r\n", s );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 350 ) return re;
    sprintf( bufArr, "RNTO %s\r\n", d );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 250 ) return re;
    return 0;
}
 
// 删除文件
int 
ml_deletefile( int sockCs, char *s )
{
    char    bufArr[512];
    int     re;
    
    sprintf( bufArr, "DELE %s\r\n", s );
    re = sendCommandToftp( sockCs, bufArr );
    if ( re != 250 ){
	printf("delete %s.\n",s);
	 return re;
    }
	return 0;
}

// 退出
int 
quit_toFtp( int sockCs)
{
    int re = 0;
    re = sendCommandToftp( sockCs, "QUIT\r\n" );
    close( sockCs );
    return re;
}


int 
main(int argc,char **argv){

	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}
	
	ftphostName = argv[1]; 
	 
	int csock = cliopen(argv[1]);
	
	if(csock!=-1){
		printf("230 Login successful.\n");
		printf("-----------------------------------------\n");
		printf("mkdir---Create folder: mkdir <filename>\n");
		printf("dir---Show filename: dir\n");
		printf("cd---Enter file path: cd <filename>\n");
		printf("ren---Change File name: ren <oldName> <newName>\n");
		printf("cdup---Back up to the next level: cdup \n");
		printf("get---Download file: get <filename> \n");
		printf("put---Upload file: put <filename> \n");
		printf("del---Delete the file: del <filename> \n");
		printf("help---Help information: help \n");
		printf("quit---exit: quit \n");
		printf("-----------------------------------------\n");
	}else {
		return 0 ;
	}

	char command[2000] ; 
	for(;;){
		printf("ftp(%s)>",ftphostName); 
		scanf("%s",command);
		
		if(strcmp(command,"mkdir")==0){
			scanf("%s",command);
			ml_mkd(csock,command);
		}else if(strcmp(command,"dir")==0){
			ml_list(csock);
		}else if(strcmp(command,"cd")==0){
			scanf("%s",command);
			ml_cwd(csock,command);
		}else if(strcmp(command,"ren")==0){
			scanf("%s",command);
			char command2[1000] ;
			scanf("%s",command2);
			ml_renamefile(csock,command,command2);
		}else if(strcmp(command,"cdup")==0){
			ml_cdup(csock);
		}else if(strcmp(command,"quit")==0){
			quit_toFtp(csock);
			return 0 ;
		}else if(strcmp(command,"get")==0){
			scanf("%s",command);
			ml_get(csock,command);
		}else if(strcmp(command,"put")==0){
			scanf("%s",command);
			ml_put(csock,command);
		}else if(strcmp(command,"del")==0){
			scanf("%s",command);
			ml_deletefile(csock,command);
		}else if(strcmp(command,"help")==0){
			printf("mkdir---Create folder: mkdir <filename>\n");
			printf("dir---Show filename: dir\n");
			printf("cd---Enter file path: cd <filename>\n");
			printf("ren---Change File name: ren <oldName> <newName>\n");
			printf("cdup---Back up to the next level: cdup \n");
			printf("get---Download file: get <filename> \n");
			printf("put---Upload file: put <filename> \n");
			printf("del---Delete the file: del <filename> \n");
			printf("help---Help information: help \n");
			printf("quit---exit: quit \n");
		}else{
			printf("-ftp: %s: command not found.\n",command);
		}
		
	}
	return 0 ;
}
 
