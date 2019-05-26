/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */ 
#include 	<sys/types.h>
#include 	<sys/stat.h>
#include 	<fcntl.h>
#include 	<sys/socket.h>
#include 	<ctype.h>
#include 	<unistd.h>
#include 	<string.h>
#include 	<termios.h>
#include 	<sys/ioctl.h>
 
int	cliopen(char *host, int port);
void strtosrv(char *str, char *host, char *port);
int	sendCMD_Tcp(int sock,char *cmd); 
int ftpCmdList( int sockfd);
int ftpCmdGet( int sockfd, char *s);
int ftpCmdPut( int sockfd, char *s);
int ftpCmdRename( int sockfd, char *s, char *d );
int ftpCmdQuit( int sockfd);
int changeCmdMode() ;
int getInputData(char *arr, int size,int f);
int ftpCmdDelete( int sockfd, char *s );
char	*host;	

// 连接指定host
int 
cliopen( char *host, int port)
{
    int     sockfd; 
	printf("Connect to %s .\n",host); 
    char      buf[512];
    int       returnCode;
    ssize_t   len;
    
    struct sockaddr_in address;
    int  opcValue;
    socklen_t slen;
    
    opcValue = 8;
    slen = sizeof(opcValue);
    memset(&address, 0, sizeof(address));
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        setsockopt(sockfd, IPPROTO_IP, IP_TOS, &opcValue, slen) < 0)
        return -1;
    
    struct timeval timeo = {15, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
    
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    
    struct hostent* serverHost = gethostbyname(host);
    if (!serverHost)
        return -1;
    
    memcpy(&address.sin_addr.s_addr, serverHost->h_addr, serverHost->h_length);
    
    if (connect(sockfd, (struct sockaddr*) &address, sizeof(address)) == -1)
        return -1;
    
    if (sockfd == -1) {
		printf("socket connect error.\n");
        return -1;
    } 
    len = recv( sockfd, buf, 512, 0 );
    buf[len] = 0;
    printf("%s",buf);
    sscanf( buf, "%d", &returnCode );
    if ( returnCode != 220 ) {
	printf("connect error.\n");
        close( sockfd );
        return -1;
    } 
    if ( sockfd == -1 ) return -1;
    char user[56] ;
    char pwd[56] ;
    printf("USERNAME:");
    getInputData(user, 56 , 1);
    printf("\n");
    sprintf( buf, "USER %s\r\n", user );
    returnCode = sendCMD_Tcp( sockfd, buf );
    if ( returnCode == 331 ){
	printf("331 Please specify the password.\n");
	printf("PWD:");
	getInputData(pwd, 56,0);
	printf("\n");
	sprintf( buf, "PASS %s\r\n", pwd );
        if ( sendCMD_Tcp( sockfd, buf ) != 230 ){ 
	   printf("Login error [%d].\n",returnCode);
            return -1;
        } 
        return sockfd ; 
    }else{
       	printf("Login error [%d].\n",returnCode);
	close( sockfd );
        return -1;
    }
}
 
 
 // 发送命令
int sendCMDcode( int sock, char *cmd, void *re_buf, ssize_t *len)
{
    char        tarr[512];
    ssize_t     r_len;
    
    if ( send( sock, cmd, strlen(cmd), 0 ) == -1 )
        return -1;
    
    r_len = recv( sock, tarr, 512, 0 );
    if ( r_len < 1 ) return -1;
    tarr[r_len] = 0;
    
    if (len != NULL) *len = r_len;
    if (re_buf != NULL) sprintf(re_buf, "%s", tarr);
    
    return 0;
}
 
 // 发送命令
int sendCMD_Tcp( int sock, char *cmd )
{
    char     tarr[512];
    int      returnCode;
    ssize_t  len;
    
    returnCode = sendCMDcode(sock, cmd, tarr, &len);
    if (returnCode == 0)
    {
        sscanf( tarr, "%d", &returnCode );
    }
    
    return returnCode;
}
 
  
int 
ftp_type( int codesock, char mode )
{
    char    tarr[128];
    sprintf( tarr, "TYPE %c\r\n", mode );
    if ( sendCMD_Tcp( codesock, tarr ) != 200 )
        return -1;
    else
        return 0;
}


 // PORT 模式
int portConMode( int codeCtrlsock )
{
    int     lsnSockc;
    int     port;
    int     len;
    struct sockaddr_in sin;
    char    cmd[128];
    
    lsnSockc = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( lsnSockc == -1 ) return -1;
    memset( (char *)&sin, 0, sizeof(sin) );
    sin.sin_family = AF_INET;
	// 主动模式开放端口
    sin.sin_port = 13272 ;

    if( bind(lsnSockc, (struct sockaddr *)&sin, sizeof(sin)) == -1 ) {
        close( lsnSockc );
        return -1;
    }
    
    if( listen(lsnSockc, 2) == -1 ) {
        close( lsnSockc );
        return -1;
    }
    
    len = sizeof( struct sockaddr );
    if ( getsockname( lsnSockc, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( lsnSockc );
        return -1;
    }
    port = sin.sin_port;
    
    if( getsockname( codeCtrlsock, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( lsnSockc );
        return -1;
    }
    
    sprintf( cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
            sin.sin_addr.s_addr&0x000000FF,
            (sin.sin_addr.s_addr&0x0000FF00)>>8,
            (sin.sin_addr.s_addr&0x00FF0000)>>16,
            (sin.sin_addr.s_addr&0xFF000000)>>24,
             port>>8,port&0xff );
	printf("%s",cmd);
    if ( sendCMD_Tcp( codeCtrlsock, cmd ) != 200 ) {
        close( lsnSockc );
        return -1;
    }
    return lsnSockc;
}

// 连接服务端socket
 int socket_connect(char *host,int port)
{
    struct sockaddr_in address;
    int s, opcValue;
    socklen_t slen;
    
    opcValue = 8;
    slen = sizeof(opcValue);
    memset(&address, 0, sizeof(address));
    
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        setsockopt(s, IPPROTO_IP, IP_TOS, &opcValue, slen) < 0)
        return -1;
    
    struct timeval timeo = {15, 0};
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
    
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    
    struct hostent* server = gethostbyname(host);
    if (!server)
        return -1;
    
    memcpy(&address.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(s, (struct sockaddr*) &address, sizeof(address)) == -1)
        return -1;
    
    return s;
}

 // 被动模式
int pasvConMode( int c_sock )
{
    int     rSocket;
    int     sendCodere;
    ssize_t len;
    int     addr[6];
    char    buf[512];
    char    re_buf[512];
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "PASV\r\n");
    sendCodere = sendCMDcode( c_sock, buf, re_buf, &len);
    if (sendCodere == 0) {
        sscanf(re_buf, "%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    	printf("%s\n",re_buf);
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    rSocket = socket_connect(buf,addr[4]*256+addr[5]);
    return rSocket;
}
 
 
 // ll命令
int 
ftpCmdList( int c_sock)
{
    int     dSocket;
    char    buf[512];
    int     sendCodere;
    int     returnCode;
    ssize_t     len,bufLength,total_len; 	
	void ** data ;
	unsigned long long *data_len ;
    dSocket = pasvConMode(c_sock);
    if (dSocket == -1) {
    	printf("pasv error\n");
	    return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "LIST %s\r\n");
    sendCodere = sendCMD_Tcp( c_sock, buf );
    printf("%d\n",sendCodere);
    if (sendCodere >= 300 || sendCodere == 0)
        return sendCodere;   
    len=total_len = 0;
    bufLength = 512;
    void *re_buf = malloc(bufLength);
    while ( (len = recv( dSocket, buf, 512, 0 )) > 0 )
    {
        if (total_len+len > bufLength)
        {
            bufLength *= 2;
            void *re_buf_n = malloc(bufLength);
            memcpy(re_buf_n, re_buf, total_len);
            free(re_buf);
            re_buf = re_buf_n;
        }
	if (buf!=NULL)
		printf("%s\n",buf);
        memcpy(re_buf+total_len, buf, len);
	total_len += len;
    }
    close( dSocket );
   	
    bzero(buf, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &returnCode );
    if ( returnCode != 226 )
    {
        free(re_buf);
        return returnCode;
    }
    
    return 0;
}
 
 // 上传文件
int 
ftpCmdGet( int c_sock, char *s )
{
    int     dSocket;
    ssize_t     len,write_len;
    char    buf[512];
    int     handleCode;
    int     returnCode;
    handleCode = open( s,  O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE );
  
    if ( handleCode == -1 ) return -1;
    
    ftp_type(c_sock, 'I');
    
    dSocket = pasvConMode(c_sock);
    if (dSocket == -1)
    {
        close(handleCode);
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "RETR %s\r\n", s );
    returnCode = sendCMD_Tcp( c_sock, buf );
    printf("%d\n",returnCode);
    if (returnCode >= 300 || returnCode == 0)
    {
        close(handleCode);
        return returnCode;
    }
    
    bzero(buf, sizeof(buf));
    while ( (len = recv( dSocket, buf, 512, 0 )) > 0 ) {
        write_len = write( handleCode, buf, len );
        if (write_len != len )
        {
            close( dSocket );
            close( handleCode );
            return -1;
        }
        
    }
    close( dSocket );
    close( handleCode );
    
    bzero(buf, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &returnCode );
    if ( returnCode >= 300 ) {
        return returnCode;
    }
    return 0;
}
 
 
 // 下载文件
int 
ftpCmdPut( int c_sock, char *s)
{
    int     dSocket;
    ssize_t     len,send_len;
    char    buf[512];
    int     handleCode;
    int send_re;
    int returnCode;

    handleCode = open( s,  O_RDONLY);
    if ( handleCode == -1 ) return -1;
    
    ftp_type(c_sock, 'I');
    
    dSocket = pasvConMode(c_sock);
    if (dSocket == -1)
    {
        close(handleCode);
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "STOR %s\r\n", s );
    send_re = sendCMD_Tcp( c_sock, buf );
    if (send_re >= 300 || send_re == 0)
    {
        close(handleCode);
        return send_re;
    }
    bzero(buf, sizeof(buf));
    while ( (len = read( handleCode, buf, 512)) > 0)
    {
        send_len = send(dSocket, buf, len, 0);
        if (send_len != len )
        {
            close( dSocket );
            close( handleCode );
            return -1;
        }
    }
    close( dSocket );
    close( handleCode );
    bzero(buf, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &returnCode );
    if ( returnCode >= 300 ) {
        return returnCode;
    }
    return 0;
}
 
 // 重命名
int 
ftpCmdRename( int c_sock, char *s, char *d )
{
    char    buf[512];
    int     re;
    
    sprintf( buf, "RNFR %s\r\n", s );
    re = sendCMD_Tcp( c_sock, buf );
    if ( re != 350 ) return re;
    sprintf( buf, "RNTO %s\r\n", d );
    re = sendCMD_Tcp( c_sock, buf );
    if ( re != 250 ) return re;
    return 0;
}
 
 // 删除文件
int 
ftpCmdDelete( int c_sock, char *s )
{
    char    buf[512];
    int     re;
    
    sprintf( buf, "DELE %s\r\n", s );
    re = sendCMD_Tcp( c_sock, buf );
    if ( re != 250 ){
	printf("delete %s.\n",s);
	 return re;
    }
	return 0;
}
int getInputData(char *arr, int size,int f)
{
    int c, n = 0;
    do
    {
        c = changeCmdMode();
        if (c != '\n' && c != '\r' && c != 127)
        {
            arr[n] = c;
			if(f==1){
				printf("%c",c);
			}
            n++;
        }else if ((c != '\n' | c != '\r') && c == 127)
        {
            if (n > 0)
            {
                n--;
            }
        }
    }while (c != '\n' && c != '\r' && n < (size - 1));
    arr[n] = '\0';
    return n;
}
int changeCmdMode()
{
    struct termios old, new;
    int ch;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return ch;
}
 
// 退出
int 
ftpCmdQuit( int cpsock)
{
    int re = 0;
    re = sendCMD_Tcp( cpsock, "QUIT\r\n" );
    close( cpsock );
    return re;
}


int 
main(int argc,char **argv){

	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}
	
	host = argv[1]; 
	 
	int csock = cliopen(argv[1],21);
	
	if(csock!=-1){
		printf("230 Login successful.\n");
	}else {
		return 0 ;
	}
	printf("-----------------------------------------\n");
	printf("		dir \n");
	printf("		quit\n");
	printf("		get\n");
	printf("		put\n");
	printf("		delete\n");
	printf("		rename\n");
	printf("-----------------------------------------\n");
	char input[2000] ; 
	while(1){ 
		printf("ftp(%s)>",host); 
		scanf("%s",input);
		
		if(strcmp(input,"dir")==0){
			ftpCmdList(csock);
		}else if(strcmp(input,"quit")==0){
			ftpCmdQuit(csock);
			return 0 ;
		}else if(strcmp(input,"get")==0){
			scanf("%s",input);
			ftpCmdGet(csock,input);
		}else if(strcmp(input,"put")==0){
			scanf("%s",input);
			ftpCmdPut(csock,input);
		}else if(strcmp(input,"delete")==0){
			scanf("%s",input);
			ftpCmdDelete(csock,input);
		}else if(strcmp(input,"rename")==0){
			scanf("%s",input);
			char *newFilename ;
			scanf("%s",newFilename);
			ftpCmdRename(csock,input,newFilename);
		}else{
			printf("%s: command not found.\n",input);
		}
		
	}
	return 0 ;
}
 
