#include<fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<stdint.h>
#include<sys/wait.h> 
#include<inttypes.h>

int X = 50000, Y = 53000;

// void get_code_argument(char *command,int *code_type,char *cmd_arg);

void get_filename(char *cmd_arg,char *filename)
{
	int i,sz=0;
	memset(filename,0,sizeof(filename));
	for(i=strlen(cmd_arg)-1;i>=0;i--)
		if(cmd_arg[i]=='/')
			break;
		else sz++;
	memcpy(filename,cmd_arg+i+1,sz*sizeof(char));
	filename[sz]='\0';
	// strncpy(filename,cmd_arg+i+1,strlen(cmd_arg)-i-2);
}

void get_code_argument(char *user_command,int *code_type,char *cmd_arg)
{
	int i=0;
	char code[100];
	while(user_command[i]!=' ' && i<strlen(user_command))
		i++;
	snprintf(code,i+1,"%s",user_command);
	if(strcmp(code,"port")==0)
		*code_type=1;
	else if(strcmp(code,"cd")==0)
		*code_type=2;
	else if(strcmp(code,"get")==0)
		*code_type=3;
	else if(strcmp(code,"put")==0)
		*code_type=4;
	else if(strcmp(code,"quit")==0)
		*code_type=5;
	else 
	{
		*code_type=502;
		return;
	}
	memset(cmd_arg,0,strlen(cmd_arg));
	strncpy(cmd_arg,user_command+i+1,strlen(user_command)-i-2);
	// printf("cmd arg -> %s  fufodfjd %d  cmd_arg end\n",cmd_arg,(int)strlen(cmd_arg) );
	i=0;
	while(cmd_arg[i]!=' ' && cmd_arg[i]!='\t' && cmd_arg[i]!='\n' && cmd_arg[i]!='\0' && i<strlen(cmd_arg))
		i++;
	// printf("i -> %d\n", i);
	if(i!=strlen(cmd_arg))
		*code_type=501;
	
	// strcpy(cmd_arg,"word.txt");
}

void get_put_client(int code_type, char cmd_arg[],int *error_code)
{

	int sockfd,clilen,newsockfd,i;
	struct sockaddr_in serv_addr,cli_addr;
	
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("Unable to create socket\n");
		exit(0);
	}
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1",&serv_addr.sin_addr);
	serv_addr.sin_port = htons(Y);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd,5);
	if(code_type==3)
	{
		if(fork()==0)
		{
			int clilen = sizeof(cli_addr);
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
						&clilen) ;
			char buf[100],num_bytes[5],temp_str[100];
			printf("opening file inside fork\n");
			char file_buf[100];
			char *filename=file_buf;
			get_filename(cmd_arg,filename);
			printf("filename -:> %s\n",filename );
			int fd = open(filename, O_WRONLY| O_TRUNC | O_CREAT, 0666);
			int bytes_received=1,msg_len;
			int flag=1;
			while(flag && bytes_received>0)
			{
				bytes_received=recv(newsockfd, buf, 100, 0);
				printf("%s\n %d", buf,bytes_received);
				if(buf[0]=='L')
					flag=0;
				short byte_cnt;
				memcpy(num_bytes,buf+sizeof(char),sizeof(short));
				// strncpy(num_bytes,buf+1,1);
				memcpy(&byte_cnt,num_bytes,sizeof(short));			
				// strncpy(temp_str,buf+3,ntohs(byte_cnt)-1);
				memcpy(temp_str,buf+sizeof(char)+sizeof(short),ntohs(byte_cnt));
				write(fd,temp_str,ntohs(byte_cnt));
			}
			close(fd);
			close(newsockfd);
			close(sockfd);
			exit(0);
		}
		// else wait(NULL);
	}
	else if(code_type==4)
	{
		int clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;
		int fd = open(cmd_arg, O_RDONLY | O_EXCL),sz;
		if(fd>=0)
		{
			if(fork()==0)
			{
				int parity=1;
				short sz_0=1,sz_1=1,temp;
				char read_buf_0[100],read_buf_1[100],rb0[101],rb1[101],tp='P',tl='L';
	    		sz_0 = read(fd, read_buf_0, 97);
	    		read_buf_0[sz_0]='\0';
	    		memcpy(rb0,&tp,sizeof(char));
	    		temp=htons(sz_0);
	    		// memcpy(rb0+1,&temp,sizeof(temp));
	    		// printf("readb0 buff-> %s\n",read_buf_0 );
	    		// memcpy(rb0+3,read_buf_0,sz_0);
	    		// printf("readb0 rb0-> %s\n",rb0+3 );
	    		memcpy(rb0+sizeof(char),&temp,sizeof(short));
	    		memcpy(rb0+sizeof(char)+sizeof(short),read_buf_0,sz_0);	

				// sprintf(rb0, "P%"SCNd16"%s",htons(sz_0),read_buf_0);
				while(sz_0!=0)
		    	{
		    		// memset(read_buf,0,sizeof(read_buf));
		    		if(parity%2==0)
		    		{
		    			sz_0 = read(fd, read_buf_0, 97);
	    				read_buf_0[sz_0]='\0';
						memcpy(rb0,&tp,sizeof(char));
			    		temp=htons(sz_0);
	    				memcpy(rb0+sizeof(char),&temp,sizeof(short));
			    		memcpy(rb0+sizeof(char)+sizeof(short),read_buf_0,sz_0);
		    		}
		    		else
		    		{
		    			sz_1 = read(fd,read_buf_1,97);
	    				read_buf_1[sz_1]='\0';
						memcpy(rb1,&tp,sizeof(char));
			    		temp=htons(sz_1);
	    				memcpy(rb1+sizeof(char),&temp,sizeof(short));
			    		memcpy(rb1+sizeof(char)+sizeof(short),read_buf_1,sz_1);
					}
		    		if(sz_0<=0)
		    			rb1[0]='L';
		    		if(sz_1<=0)
		    			rb0[0]='L';
		    		if(parity%2==0)
		    		{
		    			// write(1,rb1,sz_1+3 );
			    		send(newsockfd, rb1,sz_1+sizeof(char)+sizeof(short),0);
		    		}
			    	else 
			    	{
			    		send(newsockfd,rb0,sz_0+sizeof(char)+sizeof(short),0);
			    	}
			    	parity++;
		    		if(sz_0<=0 || sz_1<=0)
		    			break;
			    }
			    close(newsockfd);
			    close(sockfd);
			    close(fd);
			    exit(EXIT_FAILURE);
			}
			else
			{
				int status = wait(&status);

				if(WEXITSTATUS(status) == EXIT_FAILURE) {
					*error_code = 550;
				}
				else
					*error_code = 250;
				close(fd);
			}
		}
		else
		{
			printf("file not opened again\n");
			*error_code = 550;
			// return 550;
		}
	}
	close(sockfd);

}

int main()
{
	int sockfd,bytes_received;
	struct sockaddr_in serv_addr;
	
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("Unable to create socket\n");
		exit(0);
	}
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1",&serv_addr.sin_addr);
	serv_addr.sin_port = htons(X);
	sleep(1);
	if((connect(sockfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)))<0)
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
	char *buf="hey\0",bu[100];
	int ch;
	while(1)
	{
		char user_command[100];
		printf("> ");
		int code_type,error_code=-1;
		char cmd_arg[100],user_command_str[100],cmd_arg_str[100];
		char *user_command_buf=user_command_str;
		char *cmd_arg_buf=cmd_arg_str;
		size_t user_command_len=100;
		getline(&user_command_buf,&user_command_len,stdin);	
		get_code_argument(user_command_buf,&code_type,cmd_arg_buf);
		// code_type=3;
		// strcpy(cmd_arg,"word.txt");

		printf("cmd arg -> %s, code type -> %d  \n",cmd_arg_buf,code_type );
		int reply_code;
		if(code_type==1)
		{
			send(sockfd,user_command_buf, strlen(user_command_buf)+1, 0);
			recv(sockfd, &reply_code, sizeof(int), 0);
			if(reply_code == 200){

					printf("Port set succesfully\n");
					continue;
				}
			else if(reply_code==550 || reply_code==503){
				printf("Closing the connection please enter port properly\n");
				close(sockfd);
				exit(0);
			}
		}
		else if(code_type==2)
		{
			send(sockfd,user_command_buf, strlen(user_command_buf)+1, 0);
			recv(sockfd, &reply_code, sizeof(int), 0);
			if(reply_code == 200) 
				printf("Directory changed succesfully\n");
			else
				printf("Directory not changed\n");
		}
		else if(code_type==3)
		{
			send(sockfd,user_command_buf, strlen(user_command_buf)+1, 0);
			get_put_client(code_type,cmd_arg_buf,&error_code);
			recv(sockfd, &reply_code, sizeof(int), 0);
			if(reply_code == 250)
				printf("File received succesfully\n");
			else if(reply_code == 550)
				printf("Cannot transfer file\n");
			else
				printf("Give arguments properly\n");
			// sleep(1);
		}
		else if(code_type==4)
		{
			send(sockfd,user_command_buf, strlen(user_command_buf)+1, 0);
			get_put_client(code_type,cmd_arg_buf,&error_code);
			recv(sockfd, &reply_code, sizeof(int), 0);
			if(reply_code == 250)
				printf("File sent succesfully\n");
			else if(reply_code == 550)
				printf("Cannot transfer file\n");
			else
				printf("Give arguments properly\n");
		}
		else
		{
			send(sockfd,user_command_buf, strlen(user_command_buf)+1, 0);
			recv(sockfd, &reply_code, sizeof(int), 0);
			if(reply_code == 421) {
				printf("Closing the connection\n");
				close(sockfd);
				exit(0);
			}
			else {
				printf("Arguments not given properly\n");
				continue;
			}
		}
		// bytes_received=recv(sockfd, &ch, sizeof(ch), 0);
		// printf("%d\n", ch);
	}
	close(sockfd);
	return 0;
}