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
int X = 50000, Y ;


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
	memset(cmd_arg,0,100);
	strncpy(cmd_arg,user_command+i+1,strlen(user_command)-i-2);
	i=0;
	while(cmd_arg[i]!=' ' && cmd_arg[i]!='\t' && cmd_arg[i]!='\n' && cmd_arg[i]!='\0' && i<strlen(cmd_arg))
		i++;
	// printf("i -> %d\n", i);
	if(i!=strlen(cmd_arg))
		*code_type=501;
	
	// strcpy(cmd_arg,"word.txt");
}
int verify_first_cmd(char *user_command)
{
	int code_type,port_num;
	char cmd_arg_str[100];
	char *cmd_arg_buf=cmd_arg_str;	
	get_code_argument(user_command,&code_type,cmd_arg_buf);
	if(code_type==1)
	{
		// sscanf(cmd_arg_buf, "%d", &port_num);
		// memcpy(&port_num,cmd_arg_buf,sizeof(int));
		port_num = atoi(cmd_arg_buf);
		if(port_num>1024 && port_num<65535)
			return port_num;
		else return -1;
	}
	else
		return 0;
	
}
int get_put(int code_type,char cmd_arg[],int *error_code)
{
	int sockfd_child,clilen_child,sockfd;
	struct sockaddr_in serv_addr,cli_addr;
	
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("Unable to create socket\n");
		// exit(0);
		return 550;
	}
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1",&serv_addr.sin_addr);
	serv_addr.sin_port = htons(Y);

	sleep(1);
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		return 550;
		// exit(0);
	}
	if(code_type==3)
	{
		int fd = open(cmd_arg, O_RDONLY | O_EXCL),sz;
		if(fd>=0)
		{
			if(fork()==0)
			{
				int parity=1;
				short sz_0=1,sz_1=1,temp;
				char read_buf_0[101],read_buf_1[101],rb0[101],rb1[101],tp='P';
	    		sz_0 = read(fd, read_buf_0, 97);
	    		read_buf_0[sz_0]='\0';
				memcpy(rb0,&tp,sizeof(char));
				temp=htons(sz_0);
	    		memcpy(rb0+sizeof(char),&temp,sizeof(short));
	    		memcpy(rb0+sizeof(char)+sizeof(short),read_buf_0,sz_0);
				printf("%s\n",read_buf_0);
				while(sz_0!=0)
		    	{
		    		// memset(read_buf,0,sizeof(read_buf))
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
			    		send(sockfd, rb1,sizeof(char)+sizeof(short)+sz_1,0);
			    	}
			    	else 
			    	{
			    		send(sockfd,rb0,sizeof(char)+sizeof(short)+sz_0,0);
			    	}
			    	parity++;
		    		if(sz_0<=0 || sz_1<=0)
		    			break;
			    }
			    close(sockfd);
			    close(fd);
			    exit(EXIT_FAILURE);
			}
			else
			{
				int status = wait(&status);

				*error_code=250;
				close(fd);
			}
		}
		else
		{
			printf("file not opened again\n");
			return 550;
		}
	}
	else if(code_type==4)
	{
		if(fork()==0)
		{
			char buf[100],num_bytes[5],temp_buf[100];
			int fd = open(cmd_arg, O_WRONLY| O_TRUNC | O_CREAT, 0666);
			int bytes_received=1,msg_len;
			int flag=1;
			while(flag && bytes_received>0)
			{
				bytes_received=recv(sockfd, buf, 100, 0);

				// printf("buf-> %s \n",buf+3);
				if(buf[0]=='L')
					flag=0;
				short byte_cnt;
				// strncpy(num_bytes,buf+1,1);
				memcpy(num_bytes,buf+sizeof(char),sizeof(short));
				// printf("num bytes -> %s\n",num_bytes );

				// int rets=sscanf(num_bytes, "%hi", &byte_cnt);
				memcpy(&byte_cnt,num_bytes,sizeof(short));
				// perror("rets   ");
				// printf("byte_cnt-> %"SCNd16"\n", byte_cnt);
				// strncpy(temp_buf,buf+3,ntohs(byte_cnt)-1);
				memcpy(temp_buf,buf+sizeof(char)+sizeof(short),ntohs(byte_cnt));
				// printf("byte cnt ->   %hi\n",ntohs(byte_cnt) );
				// printf("temp_buf-> %s\nbuf -> %s",temp_buf,buf );
				write(fd,temp_buf,ntohs(byte_cnt));
			}
			close(fd);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		else
		{
			int status = wait(&status);
				*error_code = 250;
			
		}
	}
	close(sockfd);
}


int main()
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
	serv_addr.sin_port = htons(X);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd,5);
	while(1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
			&clilen);
		if(newsockfd<0)
		{
			perror("Accept Error\n");
			exit(0);
		}
		int cmd_cnt=0;
		while(1)
		{
			int error_code=-1,client_port,code_type=-1,get_status=-1;
			int msg_len,count=0;
			char command[100],buf[100],cmd_arg[100],command_str[100],cmd_arg_str[100];
			char *command_buf=command_str;
			char *cmd_arg_buf=cmd_arg_str;
			do
			{
				msg_len=recv(newsockfd, buf, 100, 0);
				for(i=0;i<msg_len;i++,count++)
				{
					command[count]=buf[i];
				}
				count--;
			}
			while(buf[count]!='\0');
			if(cmd_cnt==0)
			{
				printf("first command\n");
				client_port = verify_first_cmd(command);
				printf("cli port -> %d\n",client_port );
				if(client_port>0)
				{
					error_code=200;
					Y = client_port;
					cmd_cnt++;
				}
				else if(client_port==0)
					error_code=503;
				else
					error_code=550;
			}
			else
			{
				printf("not first\n");
				get_code_argument(command,&code_type,cmd_arg_buf);
				// printf("cmd arg -> %s code type -> %d\n",cmd_arg_buf,code_type );
				if(code_type==2)
				{
					int dir_change=chdir(cmd_arg_buf);
					if(dir_change==0)
						error_code=200;
					else 
						error_code=501;
				}
				else if(code_type==3)
				{
					// printf("In serv 3\n");
					int fd = open(cmd_arg_buf, O_RDONLY | O_EXCL);
					printf("%s\n",cmd_arg_buf
					 );
					close(fd);
					if(fd>=0)
						get_status=get_put(code_type,cmd_arg_buf,&error_code);
					else
						error_code=550;
					
				}
				else if(code_type==4)
				{
					printf("In serv 4 \n");

					get_status = get_put(code_type,cmd_arg_buf,&error_code);
				}
				else if(code_type==5)
				{
					// todo
					error_code=421;
					close(newsockfd);
				}
				else 
				{
					error_code=code_type;
				}	
			}
			// send(sockfd, rb1,sizeof(char)+sizeof(short)+sz_1,0);
			send(newsockfd,&error_code,sizeof(int),0);
			if(error_code==421 || error_code==503 || error_code==550)
			{
				close(newsockfd);
				break;
			}
		}
		
	}
	close(sockfd);
	return 0;
}