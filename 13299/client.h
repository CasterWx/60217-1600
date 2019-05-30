/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */ 
#include 	<sys/types.h>
#include 	<sys/stat.h>
#include 	<fcntl.h>
#include 	<sys/socket.h>
#include 	<termios.h>
#include 	<ctype.h>
#include 	<unistd.h>
#include 	<string.h>
#include 	<sys/ioctl.h>

char	*host;
int cliopen( char *host);
int commandQuit( int sockCodeNum);
int commandType( int sockCodeNum, char mode );
int commandCwd( int sockCodeNum, char *path );
int commandCdup( int sockCodeNum );
int commandMkd( int sockCodeNum, char *path );
int commandList( int sockCodeNum);
int commandGet( int sockCodeNum, char *s);
int commandPut( int sockCodeNum, char *s);
int commandRenamefile( int sockCodeNum, char *s, char *d );
int commandDeletefile( int sockCodeNum, char *s );
int ftp_pwd(int sockCodeNum) ;

 
int myTermios()
{
    struct termios oldtermios, newtermios;
    int ch;
    tcgetattr(STDIN_FILENO, &oldtermios);
    newtermios = oldtermios;
    newtermios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newtermios);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtermios);
    return ch;
}
 
// 连接指定host目标
int 
cliopen( char *host)
{
    int     sockCodeNum; 
	printf("Connect to %s .\n",host);
    sockCodeNum = connect_server( host, 21 );
    if ( sockCodeNum == -1 ) return -1;
	char user[126] ;
	char pwd[126] ;
	//输入账号密码
	int c, n = 0;
	printf("USER:");
    do
    {
        c = myTermios();
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
    }while (c != '\n' && c != '\r' && n < (126 - 1));
    user[n] = '\0';//消除一个多余的回车
	
	printf("\nPWD:");
	n = 0 ;
	 do
    {
        c = myTermios();
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
    }while (c != '\n' && c != '\r' && n < (126 - 1));
    pwd[n] = '\0';//消除一个多余的回车
	printf("\n");
	// 登录
    if ( loginFtpServer( sockCodeNum, user, pwd ) == -1 ) {
       	printf("Login error.\n");
		close( sockCodeNum );
        return -1;
    }
	printf("230 Login successful.");
    return sockCodeNum;
}
 

// 获取一个socket连接
int socket_connect(char *host,int port)
{
    struct sockaddr_in hostAddress;
    int s, opCodevalue;
    socklen_t slen;
    
    opCodevalue = 8;
    slen = sizeof(opCodevalue);
    memset(&hostAddress, 0, sizeof(hostAddress));
    
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        setsockopt(s, IPPROTO_IP, IP_TOS, &opCodevalue, slen) < 0)
        return -1;
    
    struct timeval timeo = {15, 0};
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
    
    hostAddress.sin_family = AF_INET;
    hostAddress.sin_port = htons((unsigned short)port);
    
    struct hostent* server = gethostbyname(host);
    if (!server)
        return -1;
    
    memcpy(&hostAddress.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(s, (struct sockaddr*) &hostAddress, sizeof(hostAddress)) == -1)
        return -1;
    
    return s;
}
 
// 连接指定ftp
int connect_server( char *host, int port )
{    
    int       sockCtrlNum;
    char      comBuf[512];
    int       returnCode;
    ssize_t   len;
    
    sockCtrlNum = socket_connect(host, port);
    if (sockCtrlNum == -1) {
	printf("socket connect error.\n");
        return -1;
    }
    
    len = recv( sockCtrlNum, comBuf, 512, 0 );
    comBuf[len] = 0;
    printf("%s",comBuf);
    sscanf( comBuf, "%d", &returnCode );
    if ( returnCode != 220 ) {
	printf("connect error.\n");
        close( sockCtrlNum );
        return -1;
    }
    
    return sockCtrlNum;
}
// 向ftp发送命令 ，交由其执行
int sendComFtp_re( int sock, char *cmd, void *re_buf, ssize_t *len)
{
    char        buf[512];
    ssize_t     r_len;
    
    if ( send( sock, cmd, strlen(cmd), 0 ) == -1 )
        return -1;
    
    r_len = recv( sock, buf, 512, 0 );
    if ( r_len < 1 ) return -1;
    buf[r_len] = 0;
    
    if (len != NULL) *len = r_len;
    if (re_buf != NULL) sprintf(re_buf, "%s", buf);
    
    return 0;
}
 
 // 命令传输
int sendComFtp( int sock, char *cmd )
{
    char     buf[512];
    int      returnCode;
    ssize_t  len;
    
    returnCode = sendComFtp_re(sock, cmd, buf, &len);
    if (returnCode == 0)
    {
        sscanf( buf, "%d", &returnCode );
    }
    
    return returnCode;
}
 
 // 登录
int loginFtpServer( int sock, char *user, char *pwd )
{
    char    buf[128];
    int     returnCode;
    
    sprintf( buf, "USER %s\r\n", user );
    returnCode = sendComFtp( sock, buf );
    if ( returnCode == 230 ) return 0;
    else if ( returnCode == 331 ) {
        sprintf( buf, "PASS %s\r\n", pwd );
        if ( sendComFtp( sock, buf ) != 230 ) return -1;
        return 0;
    }
    else
        return -1;
}
 // 主动模式
int usePortMode( int sockCtrlNum )
{
    int     lsnCtrlNum;
    int     port;
    int     len;
    struct sockaddr_in sin;
    char    cmd[128];
    
    lsnCtrlNum = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( lsnCtrlNum == -1 ) return -1;
    memset( (char *)&sin, 0, sizeof(sin) );
    sin.sin_family = AF_INET;
	// 指定向ftp开放的端口
    sin.sin_port = 13299 ;

    if( bind(lsnCtrlNum, (struct sockaddr *)&sin, sizeof(sin)) == -1 ) {
        close( lsnCtrlNum );
        return -1;
    }
    
    if( listen(lsnCtrlNum, 2) == -1 ) {
        close( lsnCtrlNum );
        return -1;
    }
    
    len = sizeof( struct sockaddr );
    if ( getsockname( lsnCtrlNum, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( lsnCtrlNum );
        return -1;
    }
    port = sin.sin_port;
    
    if( getsockname( sockCtrlNum, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( lsnCtrlNum );
        return -1;
    }
    // 设置命令，提交信息给ftp
    sprintf( cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
            sin.sin_addr.s_addr&0x000000FF,(sin.sin_addr.s_addr&0x0000FF00)>>8, (sin.sin_addr.s_addr&0x00FF0000)>>16,
            (sin.sin_addr.s_addr&0xFF000000)>>24,port>>8,port&0xff );
    printf("PORT模式默认端口: %s(%d*256+%d=%d)\n",cmd,port&0xff,port>>8,((port>>8)*256)+(port&0xff));
    if ( sendComFtp( sockCtrlNum, cmd ) != 200 ) {
        close( lsnCtrlNum );
        return -1;
    }
    return lsnCtrlNum;
}
 // 被动模式
int usePasvMode( int sockCodeNum )
{
    int     r_sock;
    int     send_re;
    ssize_t len;
    int     addr[6];
    char    buf[512];
    char    re_buf[512];
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "PASV\r\n");
    send_re = sendComFtp_re( sockCodeNum, buf, re_buf, &len);
    if (send_re == 0) {
        sscanf(re_buf, "%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    	printf("%s\n",re_buf);
    }
    // 获取ftp提供的端口
    bzero(buf, sizeof(buf));
    sprintf( buf, "%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    r_sock = socket_connect(buf,addr[4]*256+addr[5]);
    return r_sock;
}
 
int 
commandType( int sockCodeNum, char mode )
{
    char    buf[128];
    sprintf( buf, "TYPE %c\r\n", mode );
    if ( sendComFtp( sockCodeNum, buf ) != 200 )
        return -1;
    else
        return 0;
}
 // pwd命令
int 
ftp_pwd( int sockCodeNum)
{
	char buf[128];
	int re;
	sprintf( buf, "PWD"); // 发送pwd命令给ftp， 下同
	re = sendComFtp(sockCodeNum, buf);
	if (re!=250)
		return -1 ;
	return 0 ;
} 
// cwd命令
int 
commandCwd( int sockCodeNum, char *path )
{
    char    buf[128];
    int     re;
    sprintf( buf, "CWD %s\r\n", path );
    re = sendComFtp( sockCodeNum, buf );
    if ( re != 250 )
        return -1;
    else
        return 0;
}
int 
commandCdup( int sockCodeNum )
{
    int     re;
    re = sendComFtp( sockCodeNum, "CDUP\r\n" );
    if ( re != 250 )
        return re;
    else
        return 0;
}
 
int 
commandMkd( int sockCodeNum, char *path )
{
    char    buf[512];
    int     re;
    sprintf( buf, "MKD %s\r\n", path );
    re = sendComFtp( sockCodeNum, buf );
    if ( re != 257 )
        return re;
    else
        return 0;
}

// dir命令
int 
commandList( int sockCodeNum)
{
    int     socketDnum;
    char    buf[512];
    int     send_re;
    int     returnCode;
    ssize_t     len,buf_len,total_len; 	
	void ** data ;
	unsigned long long *data_len ;
    socketDnum = usePasvMode(sockCodeNum);
    if (socketDnum == -1) {
    	printf("pasv error");
	    return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "LIST %s\r\n");
    send_re = sendComFtp( sockCodeNum, buf );
    printf("%d\n",send_re);
    if (send_re >= 300 || send_re == 0)
        return send_re;   
    len=total_len = 0;
    buf_len = 512;
    void *re_buf = malloc(buf_len);
    while ( (len = recv( socketDnum, buf, 512, 0 )) > 0 )
    {
        if (total_len+len > buf_len)
        {
            buf_len *= 2;
            void *re_buf_n = malloc(buf_len);
            memcpy(re_buf_n, re_buf, total_len);
            free(re_buf);
            re_buf = re_buf_n;
        }
	if (buf!=NULL)
		printf("%s\n",buf);
        memcpy(re_buf+total_len, buf, len);
	total_len += len;
    }
    close( socketDnum );
   	
    bzero(buf, sizeof(buf));
    len = recv( sockCodeNum, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &returnCode );
    if ( returnCode != 226 )
    {
        free(re_buf);
        return returnCode;
    }
    
    return 0;
}
 
// get命令
int 
commandGet( int sockCodeNum, char *s )
{
    int     socketDnum;
    ssize_t     len,write_len;
    char    buf[512];
    int     handle;
    int     returnCode;
    handle = open( s,  O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE );
    printf("handle %d\n",handle);
    if ( handle == -1 ) return -1;
    
    commandType(sockCodeNum, 'I');
    
    socketDnum = usePasvMode(sockCodeNum);
    if (socketDnum == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "RETR %s\r\n", s );
    returnCode = sendComFtp( sockCodeNum, buf );
    printf("%d\n",returnCode);
    if (returnCode >= 300 || returnCode == 0)
    {
        close(handle);
        return returnCode;
    }
    
    bzero(buf, sizeof(buf));
    while ( (len = recv( socketDnum, buf, 512, 0 )) > 0 ) {
        write_len = write( handle, buf, len );
        if (write_len != len )
        {
            close( socketDnum );
            close( handle );
            return -1;
        }
        
    }
    close( socketDnum );
    close( handle );
    
    bzero(buf, sizeof(buf));
    len = recv( sockCodeNum, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &returnCode );
    if ( returnCode >= 300 ) {
        return returnCode;
    }
    return 0;
}
 
// put命令
int 
commandPut( int sockCodeNum, char *s)
{
    int     socketDnum;
    ssize_t     len,send_len;
    char    buf[512];
    int     handle;
    int send_re;
    int returnCode;

    handle = open( s,  O_RDONLY);
    if ( handle == -1 ) return -1;
    
    commandType(sockCodeNum, 'I');
    
    socketDnum = usePasvMode(sockCodeNum);
    if (socketDnum == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "STOR %s\r\n", s );
    send_re = sendComFtp( sockCodeNum, buf );
    if (send_re >= 300 || send_re == 0)
    {
        close(handle);
        return send_re;
    }
    bzero(buf, sizeof(buf));
    while ( (len = read( handle, buf, 512)) > 0)
    {
        send_len = send(socketDnum, buf, len, 0);
        if (send_len != len )
        {
            close( socketDnum );
            close( handle );
            return -1;
        }
    }
    close( socketDnum );
    close( handle );
    bzero(buf, sizeof(buf));
    len = recv( sockCodeNum, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &returnCode );
    if ( returnCode >= 300 ) {
        return returnCode;
    }
    return 0;
}
 // ren命令
int 
commandRenamefile( int sockCodeNum, char *s, char *d )
{
    char    buf[512];
    int     re;
    
    sprintf( buf, "RNFR %s\r\n", s );
    re = sendComFtp( sockCodeNum, buf );
    if ( re != 350 ) return re;
    sprintf( buf, "RNTO %s\r\n", d );
    re = sendComFtp( sockCodeNum, buf );
    if ( re != 250 ) return re;
    return 0;
}
 // del命令
int 
commandDeletefile( int sockCodeNum, char *s )
{
    char    buf[512];
    int     re;
    
    sprintf( buf, "DELE %s\r\n", s );
    re = sendComFtp( sockCodeNum, buf );
    if ( re != 250 ){
	printf("delete %s.\n",s);
	 return re;
    }
	return 0;
}
// quit退出命令
int 
commandQuit( int sockCodeNum)
{
    int re = 0;
    re = sendComFtp( sockCodeNum, "QUIT\r\n" );
    close( sockCodeNum );
    return re;
}
