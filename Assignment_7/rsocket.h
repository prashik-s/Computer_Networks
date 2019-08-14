#ifndef _MYLIB_H_
#define _MYLIB_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> // socket         
#include <sys/socket.h> //socket
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h> 

#define T 2
#define P 0.5
#define SOCK_MRP 100

typedef struct recv_tab
{
	int message_id,valid_bit;
}recv_table;

typedef struct unack_tab
{
	int message_id,message_len;
	struct timeval last_send_time;
	char message[100];
	struct sockaddr destination_ip_port;
}unack_table;


typedef struct recv_buf
{
	char message[100];
	int message_id,message_len;
}recv_buffer;


int dropMessage(float p);
void send_ack(int message_id,int sockfd, struct sockaddr cliaddr);
void send_message(int sockfd,const char *message,int message_len,int flag,const struct sockaddr *cliaddr,int cliaddr_len,int retransmit_id);
float gettimedifference(struct timeval prev_t);
void HandleRetransmit(int sockfd);
void HandleACKMsgRecv(int recv_ack_id);
void HandleAppMsgRecv(int recv_msg_id,char *recv_msg,int recv_msg_len, int sockfd, struct sockaddr cliaddr);
void HandleReceive(int sockfd);
void * func_x(void *sfd);
void init();
void create_x(int sockfd);
int r_socket(int address_family,int type,int flag);
int r_bind(int sockfd, const struct sockaddr * servaddr,int serv_len);
void r_sendto(int sockfd, const char *message,int message_len,int flag, const struct sockaddr * cliaddr, int cliaddr_len);
int r_recvfrom(int sockfd, char * buffer,int buffer_len, int flag, struct sockaddr * cliaddr,int *len);
void r_close(int sockfd);
void free_mem(int sockfd);

#endif