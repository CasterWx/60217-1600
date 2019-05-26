#include "client.h"


int 
main(int argc,char **argv){

	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}
	
	// 连接FTP
	int csock = cliopen(argv[1]);
	
	if(csock!=-1){
		printf("230 Login successful.\n");
	}else {
		return 0 ;
	}
	// 登录后提示
	printf("------------Welcome to FTP------------\n");
	printf("mkdir---新建一个目录\n");
	printf("dir---显示服务端的文件\n");
	printf("cd---进入服务端的指定目录\n");
	printf("ren---修改服务端文件夹名称\n");
	printf("get---下载服务端文件\n");
	printf("put---向服务端上传文件\n");
	printf("del---删除服务端文件 \n");
	printf("help---帮助命令 \n");
	printf("quit---退出\n");
	printf("---------------------------------------\n");
	char insertCommand[2000] ; 
	// 循环接受输入的命令
	for(;;){ 
		// 前置提示符
		printf("ftp(%s)>",argv[1]); 
		scanf("%s",insertCommand);
		// 命令解析
		if(strcmp(insertCommand,"mkdir")==0){
			scanf("%s",insertCommand);
			commandMkd(csock,insertCommand);
		}else if(strcmp(insertCommand,"dir")==0){
			commandList(csock);
		}else if(strcmp(insertCommand,"cd")==0){
			scanf("%s",insertCommand);
			commandCwd(csock,insertCommand);
		}else if(strcmp(insertCommand,"ren")==0){
			scanf("%s",insertCommand);
			char command2[1000] ;
			scanf("%s",command2);
			commandRenamefile(csock,insertCommand,command2);
		}else if(strcmp(insertCommand,"quit")==0){
			commandQuit(csock);
			return 0 ;
		}else if(strcmp(insertCommand,"get")==0){
			scanf("%s",insertCommand);
			commandGet(csock,insertCommand);
		}else if(strcmp(insertCommand,"put")==0){
			scanf("%s",insertCommand);
			commandPut(csock,insertCommand);
		}else if(strcmp(insertCommand,"del")==0){
			scanf("%s",insertCommand);
			commandDeletefile(csock,insertCommand);
		}else if(strcmp(insertCommand,"help")==0){
			printf("mkdir---新建一个目录\n");
			printf("dir---显示服务端的文件\n");
			printf("cd---进入服务端的指定目录\n");
			printf("ren---修改服务端文件夹名称\n");
			printf("get---下载服务端文件\n");
			printf("put---向服务端上传文件\n");
			printf("del---删除服务端文件 \n");
			printf("help---帮助命令 \n");
			printf("quit---退出\n");
			printf("---------------------------------------\n");
		}else{
			printf("-ftp: %s: command not found.\n",insertCommand);
		}
		
	}
	return 0 ;
}
 
