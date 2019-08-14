#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int min(int a, int b)
{
	if(a<b)
		return a;
	return b;
}

int main()
{
	int	sockfd, newsockfd ; 
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(32000);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

		int msg_len,count=0;
		char filename[100];
		do
		{
			msg_len=recv(newsockfd, buf, 100, 0);
			for(i=0;i<msg_len;i++,count++)
			{
				filename[count]=buf[i];
			}
			count--;
		}
		while(buf[count]!='\0');	// If filename comes in multiple packets
		
		filename[count]='\0';
	    char read_buf[100];
	    int sz=1;
    	int fd = open(filename, O_RDONLY | O_EXCL); 
    	if(fd>=0)
    	{
	    	while(sz!=0)
	    	{
	    		memset(read_buf,0,sizeof(read_buf));
	    		sz = read(fd, read_buf, 99);
	    		if(sz<=0)
	    			break;
		    	send(newsockfd, read_buf,sz,0);
		    }
	    }
		close(newsockfd);
	}
	return 0;
}
			

