#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<netdb.h>	//hostent
#include<sys/select.h>

static int MAXLINE=100;

int max(int a, int b)
{
	if(a>b)
		return a;
	return b;
}

int main()
{
    int sockfd1,sockfd2,newsockfd;
    struct sockaddr_in cli_addr, serv_addr1,serv_addr2;
    if((sockfd1 = socket(AF_INET, SOCK_STREAM,0))<0)
    {
        perror("Socket Creation failed\n");
        exit(0);
    }
    sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);  //syscall (type_of_network(ipv4 -> AF_INET), type_of_socket, 0) defines socket and returns int identifier
    if ( sockfd2 < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    if(setsockopt(sockfd1,SOL_SOCKET,SO_REUSEADDR, &(int){1},sizeof(int))<0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if(setsockopt(sockfd2,SOL_SOCKET,SO_REUSEADDR, &(int){1},sizeof(int))<0)
        perror("setsockopt(SO_REUSEADDR) failed");

      
    memset(&serv_addr1, 0, sizeof(serv_addr1));//  
    memset(&serv_addr2, 0, sizeof(serv_addr2));//  
    memset(&cli_addr, 0, sizeof(cli_addr)); 
    serv_addr1.sin_family = AF_INET;
    serv_addr1.sin_addr.s_addr = INADDR_ANY;
    serv_addr1.sin_port = htons(21100);

    serv_addr2.sin_family = AF_INET;
    serv_addr2.sin_addr.s_addr = INADDR_ANY;
    serv_addr2.sin_port = htons(31100);

    if(bind(sockfd1,(struct sockaddr *) &serv_addr1,
            sizeof(serv_addr1))<0)
    {
        perror("Unable to bind local address\n");
        exit(0);
    }
    if ( bind(sockfd2, (const struct sockaddr *)&serv_addr2,  
            sizeof(serv_addr2)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    listen(sockfd1, 5);
    fd_set readfs;
    int nfds;
	while (1) {
		FD_ZERO(&readfs);
		FD_SET(sockfd1,&readfs);
		FD_SET(sockfd2,&readfs);
		nfds = max(sockfd1,sockfd2)+1;
		int r=select(nfds,&readfs,0,0,0);
		if(r<0)
		{
			perror("Select failed\n");
			exit(0);
		}
		if(FD_ISSET(sockfd1,&readfs))
		{
			if(fork()==0)
			{
				printf("HERE");
				int clilen = sizeof(cli_addr);
				newsockfd = accept(sockfd1, (struct sockaddr *) &cli_addr,
							&clilen) ;
				if (newsockfd < 0) {
					perror("Accept error\n");
					exit(0);
				}

			    char read_buf[100];
			    int sz=1;
		    	int fd = open("word.txt", O_RDONLY | O_EXCL); 
		    	if(fd>=0)
		    	{
		    		char c;
		    		int counter;
			    	while(sz!=0)
			    	{
		    			memset(read_buf,0,sizeof(read_buf));
			    		counter=0;
			    		while(sz!=0 && c!='\n')
			    		{
			    			sz = read(fd, &c, 1);
				    		if(sz<=0)
				    			break;
				    		if(c=='\n')
				    			break;
				    		read_buf[counter++]=c;
				    	}
				    	read_buf[counter]='\0';
				    	send(newsockfd, read_buf,strlen(read_buf),0);
				    }
			    }
			    else 
			    {
			    	perror("File not present\n");
			    	exit(0);
			    }
			    exit(0);
			}
			
		}
		if(FD_ISSET(sockfd2,&readfs))
		{
			if(fork()==0)
			{
				int n; 
			    socklen_t len;
			    char buffer[MAXLINE],ip[MAXLINE]; 
			    len = sizeof(cli_addr); // initialize len to avoid errors .Note - len is inout parameter
			    n = recvfrom(sockfd2, (char *)buffer, MAXLINE, 0, 
			            ( struct sockaddr *) &cli_addr, &len); 
			    

		    	struct hostent *he;
		    	struct in_addr **addr_list;
		    	int i;
		    		
		    	if ( (he = gethostbyname( buffer ) ) == NULL) 
		    	{
		    		perror("gethostbyname");
		    		exit(0);
		    	}

		    	addr_list = (struct in_addr **) he->h_addr_list;
		    	strcpy(ip,inet_ntoa(*addr_list[0]));
		    	
		    	sendto(sockfd2, (const char *)ip, strlen(ip)+1, 0, 
				(const struct sockaddr *) &serv_addr2, sizeof(serv_addr2));
				printf("in 2 %s",ip);
			exit(0);
			}

		}
		break;
		
	}
    return 0;

}