#include<fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


int main()
{
	int	sockfd ;
	struct sockaddr_in	serv_addr;

	int i;
	char buf[100];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}
	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(21100);

	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}
	for(i=0; i < 100; i++) buf[i] = '\0';
	

	int bytes_received=1,word_count=0;

	while(bytes_received>0)
	{
		bytes_received=recv(sockfd, buf, 100, 0);
		if(bytes_received==0)
			break;
		
		for(i=0;i<bytes_received;i++)
		{
			if(buf[i]=='\n')
			{
				word_count++;
			}
		}
	}
	if(word_count>0)
	{
		printf("Words recieved = %d\n",word_count-1);
	}
	else
	{
		perror("File Not Found\n");
	}	
	close(sockfd);
	return 0;
}
