#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //for exit(0);
#include<sys/socket.h>
#include<errno.h> //For errno - the error number
#include<arpa/inet.h>
#include <netinet/in.h> 
#include <sys/types.h> 

static int MAXLINE=100;


int main(int argc , char *argv[])
{	
	char ip[100],hostname[100];
	printf("Enter hostname: ");
	scanf("%s",hostname);

	int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(31100); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len; 
      
    sendto(sockfd, (const char *)hostname, strlen(hostname)+1, 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr));

    socklen_t msg_len;
    char buffer[MAXLINE]; 
 	len = sizeof(servaddr); 
    printf("recieving\n");
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
			( struct sockaddr *) &servaddr, &len);
    printf("recieved");
    if(n>0)
		printf("The IP address is: %s\n" ,buffer);
	else
		printf("IP address not found\n");	
	return 0;
}
/*
	Get ip from domain name
 */
