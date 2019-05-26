#include "func.h" 
/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */ 
#include 	<sys/types.h>
#include 	<sys/stat.h>
#include 	<termios.h>
#include 	<fcntl.h>
#include 	<sys/socket.h>
#include 	<ctype.h>
#include 	<unistd.h>
#include 	<string.h>
#include 	<sys/ioctl.h>
/* define macros*/
#define MAXBUF	1024
#define STDIN_FILENO	0
#define STDOUT_FILENO	1

/* define FTP reply code */
#define USERNAME	220
#define PASSWORD	331
#define LOGIN		230
#define PATHNAME	257
#define CLOSEDATA	226
#define ACTIONOK	250


// 创建与ftp之间的socket连接
int socket_connect(char *host,int port)
{
    struct sockaddr_in address;// socket套接字
    int s, opvalue;
    socklen_t slen;
    
    opvalue = 8;
    slen = sizeof(opvalue);
    memset(&address, 0, sizeof(address));
    
	// 初始化
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        setsockopt(s, IPPROTO_IP, IP_TOS, &opvalue, slen) < 0)
        return -1;
    
    struct timeval timeo = {15, 0};
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
    
	// socket设置连接目标
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    
	// 获取主机地址
    struct hostent* server = gethostbyname(host);
    if (!server)
        return -1;
    
    memcpy(&address.sin_addr.s_addr, server->h_addr, server->h_length);
    
	// 连接
    if (connect(s, (struct sockaddr*) &address, sizeof(address)) == -1)
        return -1;
    
    return s;
}
 
 // 连接ftp
int connect_server( char *host, int port )
{    
    int       ctrl_sock;
    char      buf[512];
    int       result;
    ssize_t   len;
    
    // 创建一个socket
    ctrl_sock = socket_connect(host, port);
    if (ctrl_sock == -1) {
	printf("socket connect error.\n");
        return -1;
    }
    
    // 接收数据
    len = recv( ctrl_sock, buf, 512, 0 );
    buf[len] = 0;
    printf("%s",buf);
    sscanf( buf, "%d", &result );
    if ( result != 220 ) {
	printf("connect error.\n");
        close( ctrl_sock );
        return -1;
    }
    
    return ctrl_sock;
}
 
// 向ftp发送命令,用于被ftp_sendcmd调用
int ftp_sendcmd_re( int sock, char *cmd, void *re_buf, ssize_t *len)
{
    char        buf[512];
    ssize_t     r_len;
    
    // send指令发送数据
    if ( send( sock, cmd, strlen(cmd), 0 ) == -1 )
        return -1;
    
	//  recv接收返回数据
    r_len = recv( sock, buf, 512, 0 );
    if ( r_len < 1 ) return -1;
    buf[r_len] = 0;
    
    if (len != NULL) *len = r_len;
    if (re_buf != NULL) sprintf(re_buf, "%s", buf);
    
    return 0;
}
 
// 向ftp发送命令
int ftp_sendcmd( int sock, char *cmd )
{
    char     buf[512];
    int      result;
    ssize_t  len;
    
    result = ftp_sendcmd_re(sock, cmd, buf, &len);
    if (result == 0)
    {
        sscanf( buf, "%d", &result );
    }
    
    return result;
}
 
 // 登录ftp
int login_server( int sock, char *user, char *pwd )
{
    char    buf[128];
    int     result;
    
    // 用户名
    sprintf( buf, "USER %s\r\n", user );
    result = ftp_sendcmd( sock, buf );
    if ( result == 230 ) return 0;
    else if ( result == 331 ) {// 331代表等待输入密码
        sprintf( buf, "PASS %s\r\n", pwd );
        if ( ftp_sendcmd( sock, buf ) != 230 ) return -1; //不为230表示登录失败
		printf("Welcome to FTP \n");
        return 0;
    }
    else
        return -1;
}
int inputArrlist(char *arr, int size,int flag)
{
    int c, n = 0;
    do
    {
        c = termiosCh();
        if (c != '\n' && c != '\r' && c != 127)
        {
            arr[n] = c;
			if (flag==1){
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
int termiosCh()
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
 // PORT 主动模式
int create_datasock( int ctrl_sock )
{
    int     lsn_sock;
    int     port;
    int     len;
    struct sockaddr_in sin;
    char    cmd[128];
    
    lsn_sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( lsn_sock == -1 ) return -1;
    memset( (char *)&sin, 0, sizeof(sin) );
    sin.sin_family = AF_INET;
    sin.sin_port = 13303 ;

    if( bind(lsn_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1 ) {
        close( lsn_sock );
        return -1;
    }
    
    if( listen(lsn_sock, 2) == -1 ) {
        close( lsn_sock );
        return -1;
    }
    
    len = sizeof( struct sockaddr );
    if ( getsockname( lsn_sock, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( lsn_sock );
        return -1;
    }
    port = sin.sin_port;
    
    if( getsockname( ctrl_sock, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        close( lsn_sock );
        return -1;
    }
    
    // 通知服务端已开放的端口 (port>>8,port&0xff)
    sprintf( cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
            sin.sin_addr.s_addr&0x000000FF,
            (sin.sin_addr.s_addr&0x0000FF00)>>8,
            (sin.sin_addr.s_addr&0x00FF0000)>>16,
            (sin.sin_addr.s_addr&0xFF000000)>>24,
            port>>8,port&0xff);
    printf("%s",cmd);
    if ( ftp_sendcmd( ctrl_sock, cmd ) != 200 ) {
        close( lsn_sock );
        return -1;
    }
    return lsn_sock;
}
 
// PASV 被动模式开启
int ftp_pasv_connect( int c_sock )
{
    int     r_sock;
    int     send_re;
    ssize_t len;
    int     addr[6];
    char    buf[512];
    char    re_buf[512];
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "PASV\r\n");
	// 向服务端请求其开放的端口
    send_re = ftp_sendcmd_re( c_sock, buf, re_buf, &len);
    if (send_re == 0) {
        sscanf(re_buf, "%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    	printf("%s\n",re_buf);
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    r_sock = socket_connect(buf,addr[4]*256+addr[5]);
    return r_sock;
}
 
int 
ftp_type( int c_sock, char mode )
{
    char    buf[128];
    sprintf( buf, "TYPE %c\r\n", mode );
    if ( ftp_sendcmd( c_sock, buf ) != 200 )
        return -1;
    else
        return 0;
}

/// pwd命令
int 
ftp_pwd( int c_sock)
{
	char buf[128];
	int re;
	sprintf( buf, "PWD");
	re = ftp_sendcmd(c_sock, buf);
	if (re!=250)
		return -1 ;
	return 0 ;
} 


// cwd命令
int 
ftp_cwd( int c_sock, char *path )
{
    char    buf[128];
    int     re;
    sprintf( buf, "CWD %s\r\n", path );
    re = ftp_sendcmd( c_sock, buf );
    if ( re != 250 )
        return -1;
    else
        return 0;
}
 
// cdup命令
int 
ftp_cdup( int c_sock )
{
    int     re;
    re = ftp_sendcmd( c_sock, "CDUP\r\n" );
    if ( re != 250 )
        return re;
    else
        return 0;
}
// mkdir命令 
int 
ftp_mkd( int c_sock, char *path )
{
    char    buf[512];
    int     re;
    sprintf( buf, "MKD %s\r\n", path );
    re = ftp_sendcmd( c_sock, buf );
    if ( re != 257 )
        return re;
    else
        return 0;
}

// ls dir ll 命令
int 
ftp_list( int c_sock, char *path, void **data, unsigned long long *data_len)
{
    int     d_sock;
    char    buf[512];
    int     send_re;
    int     result;
    ssize_t     len,buf_len,total_len; 	

    d_sock = ftp_pasv_connect(c_sock);
    if (d_sock == -1) {
    	printf("pasv error");
	    return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "LIST %s\r\n");
    send_re = ftp_sendcmd( c_sock, buf );
    printf("%d\n",send_re);
    if (send_re >= 300 || send_re == 0)
        return send_re;   
    len=total_len = 0;
    buf_len = 512;
    void *re_buf = malloc(buf_len);
    while ( (len = recv( d_sock, buf, 512, 0 )) > 0 )
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
    close( d_sock );
   	
    bzero(buf, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &result );
    if ( result != 226 )
    {
        free(re_buf);
        return result;
    }
    
    return 0;
}
 
// 上传put文件到服务端
int 
ftp_retrfile( int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop)
{
    int     d_sock;
    ssize_t     len,write_len;
    char    buf[512];
    int     handle;
    int     result;
    
    handle = open( d,  O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE );
    printf("handle %d\n",handle);
    if ( handle == -1 ) return -1;
    
    ftp_type(c_sock, 'I');
    
    d_sock = ftp_pasv_connect(c_sock);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "RETR %s\r\n", s );
    result = ftp_sendcmd( c_sock, buf );
    printf("%d\n",result);
    if (result >= 300 || result == 0)
    {
        close(handle);
        return result;
    }
    
    bzero(buf, sizeof(buf));
    while ( (len = recv( d_sock, buf, 512, 0 )) > 0 ) {
        write_len = write( handle, buf, len );
        if (write_len != len)
        {
            close( d_sock );
            close( handle );
            return -1;
        }
        
        
    }
    close( d_sock );
    close( handle );
    
    bzero(buf, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &result );
    if ( result >= 300 ) {
        return result;
    }
    return 0;
}
 
 
// 下载get文件到服务端
int 
ftp_storfile( int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop)
{
    int     d_sock;
    ssize_t     len,send_len;
    char    buf[512];
    int     handle;
    int send_re;
    int result;
    
    handle = open( s,  O_RDONLY);
    if ( handle == -1 ) return -1;
    
    ftp_type(c_sock, 'I');
    
    d_sock = ftp_pasv_connect(c_sock);
    if (d_sock == -1)
    {
        close(handle);
        return -1;
    }
    
    bzero(buf, sizeof(buf));
    sprintf( buf, "STOR %s\r\n", d );
    send_re = ftp_sendcmd( c_sock, buf );
    if (send_re >= 300 || send_re == 0)
    {
        close(handle);
        return send_re;
    }
    bzero(buf, sizeof(buf));
    while ( (len = read( handle, buf, 512)) > 0)
    {
        send_len = send(d_sock, buf, len, 0);
        if (send_len != len )
        {
            close( d_sock );
            close( handle );
            return -1;
        }
    }
    close( d_sock );
    close( handle );
    bzero(buf, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &result );
    if ( result >= 300 ) {
        return result;
    }
    return 0;
}
 
// rename命令
int 
ftp_renamefile( int c_sock, char *s, char *d )
{
    char    buf[512];
    int     re;
    
    sprintf( buf, "RNFR %s\r\n", s );
    re = ftp_sendcmd( c_sock, buf );
    if ( re != 350 ) return re;
    sprintf( buf, "RNTO %s\r\n", d );
    re = ftp_sendcmd( c_sock, buf );
    if ( re != 250 ) return re;
    return 0;
}
 
// 删除文件
int 
ftp_deletefile( int c_sock, char *s )
{
    char    buf[512];
    int     re;
    
    sprintf( buf, "DELE %s\r\n", s );
    re = ftp_sendcmd( c_sock, buf );
    if ( re != 250 ){
	printf("delete %s.\n",s);
	 return re;
    }
	return 0;
}
 
// 删除文件夹
int 
ftp_deletefolder( int c_sock, char *s )
{
    char    buf[512];
    int     re;
    sprintf( buf, "RMD %s\r\n", s );
    re = ftp_sendcmd( c_sock, buf );
    if ( re != 250 ) {
	printf("delete %s.\n",s);
	}
	return re;
    return 0;
}


// 连接ftp，调用connect_server
int 
ftp_connect( char *host, int port, char *user, char *pwd )
{
    int     c_sock;
    
    c_sock = connect_server( host, port );
    if ( c_sock == -1 ) return -1;
    
    if ( login_server( c_sock, user, pwd ) == -1 ) {
       	printf("Login error.\n");
	close( c_sock );
        return -1;
    }
    
    return c_sock;
}
 
// 退出
int 
ftp_quit( int c_sock)
{
    int re = 0;
    re = ftp_sendcmd( c_sock, "QUIT\r\n" );
    close( c_sock );
    return re;
}


int main(int argc,char **argv){

	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}
	int ret = connect_server(argv[1],21);
	char username[255] ;
	char password[255] ;
	if(ret==-1){
		printf("error\n");
	}
	printf("Username:");
	inputArrlist(username,255,1);
	printf("\n");
	printf("Password:");
	inputArrlist(password,255,0);
	printf("\n");
	ret = ftp_connect(argv[1],21,username,password);
	if(ret!=-1){
		printf("%s connect successful!\n",argv[1]);
	}else {
		return 0 ;
	}
	printf("---------欢迎使用FTP-Client---------\n");
	
	printf("\tls/dir/ll ： 显示FTP服务器的文件列表\n");
	printf("\tmkdir <directory> ： 在服务器上创建指定目录\n");
	printf("\trename <old name> <new name>： 对文件重命名\n");
	printf("\tcd <name>： 改变服务器上的工作目录\n");
	printf("\tcdup ： 返回到服务器的上一层目录，等同cd ..\n");
	printf("\tfiledel <name>： 删除FTP服务器的文件\n");
	printf("\tfolderdel <name>： 删除FTP服务器的文件夹\n");
	printf("\tget <name>： 下载FTP服务器的文件\n");
	printf("\tput <name>： 上传文件到FTP服务器\n");
	printf("\tquit： 离开FTP服务器\n"); 
	printf("--------------------------------------\n");
	char command[2000] ;
	int flag = 1 ;
	// 开始解析命令
	while(1){
		if(flag){
			printf("(%s)myftp>",argv[1]);
		}
		scanf("%s",command);
		
		if(strcmp(command,"mkdir")==0){
			scanf("%s",command);
			ftp_mkd(ret,command);
		}else if((strcmp(command,"dir")==0)||(strcmp(command,"ls")==0)||(strcmp(command,"ll")==0)){
			void ** data ;
			unsigned long long *data_len ;
			ftp_list(ret,"\\",data,data_len);
		}else if(strcmp(command,"cd")==0){
			scanf("%s",command);
			ftp_cwd(ret,command);
		}else if(strcmp(command,"rename")==0){
			scanf("%s",command);
			char command2[1000] ;
			scanf("%s",command2);
			ftp_renamefile(ret,command,command2);
		}else if(strcmp(command,"cdup")==0){
			ftp_cdup(ret);
		}else if(strcmp(command,"quit")==0){
			ftp_quit(ret);
			return 0 ;
		}else if(strcmp(command,"get")==0){
			scanf("%s",command);
		  	unsigned long long * stor_size ;
			int * stop ;
			ftp_retrfile(ret,command,command,stor_size,stop);
		}else if(strcmp(command,"put")==0){
			scanf("%s",command);
			unsigned long long * stor_size ;
			int * stop ;
			ftp_storfile(ret,command,command,stor_size,stop);
		}else if(strcmp(command,"filedel")==0){
			scanf("%s",command);
			ftp_deletefile(ret,command);
		}else if(strcmp(command,"folderdel")==0){
			scanf("%s",command);
			ftp_deletefolder(ret,command);
		}else if(strcmp(command,"help")==0){
			printf("\tmkdir\tdir\tcd\trename\tcdup\r\n\tget\tput\tfiledel\tfolderdel\thelp\r\n\tquit\r\n");
		}else{
			printf("%s command not found.\n",command);
		}
		
	}
	return 0 ;
}
 
