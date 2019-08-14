#include "rsocket.h"
#define N 100	// denotes maximum message and buffer length 
#define M 10	// denotes additional space added to the buffer to include headers
#define NUM_LOCKS 4
#define debug_bit 0	// denotes if the code is run in debug mode or not
#define show_retransmits 0	// To display number of retransmits

int glob_msg_id,num_retransmits=0;
recv_table *receive_message_id;
unack_table *unacknowledged_message_table;
recv_buffer *receive_buffer;
int queue_start,queue_len,unack_cnt;
int l_retab = 0,l_unack = 1,l_rebuf = 2,l_glob_unack_cnt = 3;
pthread_mutex_t lock[NUM_LOCKS];
sem_t sem_buf; 

int dropMessage(float p)
{
	float rand_num = (double)rand() / (double)RAND_MAX;
	if(rand_num < p)
		return 1;
	else return 0;
}

void send_ack(int message_id,int sockfd, struct sockaddr cliaddr)
{
	char ack_buffer[N],a;
	a = 'a';
	int cliaddr_len = sizeof(cliaddr);
	memcpy(ack_buffer,&a,sizeof(char));
	memcpy(ack_buffer+sizeof(char),&message_id,sizeof(int));
	sendto(sockfd, ack_buffer, sizeof(char)+sizeof(int),0,
			 &cliaddr, cliaddr_len);
}

void send_message(int sockfd,const char *message,int message_len,int flag,const struct sockaddr *cliaddr,int cliaddr_len,int retransmit_id)
{
	char final_message[N+M];
	char m = 'm';
	int asgn_message_id;
	if(retransmit_id == -1)
	{
		pthread_mutex_lock(&lock[l_glob_unack_cnt]);
		unack_cnt++;
		pthread_mutex_unlock(&lock[l_glob_unack_cnt]);
		asgn_message_id = glob_msg_id;
		glob_msg_id++;
	}
	else
	{
		asgn_message_id = retransmit_id;
	}
  	pthread_mutex_lock(&lock[l_unack]); 
	unacknowledged_message_table[asgn_message_id].message_id = asgn_message_id;
	gettimeofday(&unacknowledged_message_table[asgn_message_id].last_send_time,NULL);

	strcpy(unacknowledged_message_table[asgn_message_id].message,message);
	unacknowledged_message_table[asgn_message_id].message_len = message_len;
	unacknowledged_message_table[asgn_message_id].destination_ip_port = *cliaddr;
	pthread_mutex_unlock(&lock[l_unack]);

	memcpy(final_message,&m,sizeof(char));
	memcpy(final_message+sizeof(char),(char *)&asgn_message_id,sizeof(int));
	memcpy(final_message+sizeof(char)+sizeof(int),&message[0],message_len*sizeof(char));
	sendto(sockfd, final_message, message_len*sizeof(char)+sizeof(char)+sizeof(int),0,
			 cliaddr, cliaddr_len);	
}

float gettimedifference(struct timeval prev_t)
{
	struct timeval cur_t;
	gettimeofday(&cur_t,NULL);
	float time_diff;
	time_diff = cur_t.tv_sec-prev_t.tv_sec + (cur_t.tv_usec-prev_t.tv_usec)/(1.0*(1000000));
	return time_diff;
}


void HandleRetransmit(int sockfd)
{
	if(debug_bit)
		printf("handle retransmit called: unack_cnt - %d\n",unack_cnt );

	for(int i = 0;i < N;i++)
	{
		pthread_mutex_lock(&lock[l_unack]);
		if(unacknowledged_message_table[i].message_id != -1 && gettimedifference(unacknowledged_message_table[i].last_send_time)>T)
		{
			num_retransmits++;
			if(show_retransmits)
				printf("num_retransmits -> %d\n",num_retransmits);
			char message[100];
			memcpy(message,unacknowledged_message_table[i].message,strlen(unacknowledged_message_table[i].message)*sizeof(char));
			int message_len = unacknowledged_message_table[i].message_len;
			struct sockaddr cliaddr = unacknowledged_message_table[i].destination_ip_port;
			int cliaddr_len = sizeof(cliaddr);
			int retransmit_id = unacknowledged_message_table[i].message_id;
			pthread_mutex_unlock(&lock[l_unack]);
			send_message(sockfd,message,message_len,0,&cliaddr,cliaddr_len,retransmit_id);
		}
		else 
		{
			pthread_mutex_unlock(&lock[l_unack]);
		}
	}
}

void HandleACKMsgRecv(int recv_ack_id)
{
	pthread_mutex_lock(&lock[l_unack]);
	if(unacknowledged_message_table[recv_ack_id].message_id != -1)
	{
		if(debug_bit)
			printf("ack received -> %d\n",recv_ack_id);
		unacknowledged_message_table[recv_ack_id].message_id = -1;
		pthread_mutex_lock(&lock[l_glob_unack_cnt]);
		unack_cnt--;
		pthread_mutex_unlock(&lock[l_glob_unack_cnt]);
	}
	pthread_mutex_unlock(&lock[l_unack]);
}
void HandleAppMsgRecv(int recv_msg_id,char *recv_msg,int recv_msg_len, int sockfd, struct sockaddr cliaddr)
{
	pthread_mutex_lock(&lock[l_retab]);
	if(receive_message_id[recv_msg_id].valid_bit == 1)
	{
		send_ack(recv_msg_id,sockfd,cliaddr);
	}
	else
	{
		receive_message_id[recv_msg_id].valid_bit = 1;
		receive_message_id[recv_msg_id].message_id = recv_msg_id;
		if(queue_len>N)
		{
			sem_post(&sem_buf); 
			perror("Buffer Full\n");
		}
		else
		{
			pthread_mutex_lock(&lock[l_rebuf]);
			int buf_id = queue_start+queue_len;
			memcpy(receive_buffer[buf_id].message,recv_msg,recv_msg_len*sizeof(char));
			receive_buffer[buf_id].message_id = recv_msg_id;
			receive_buffer[buf_id].message_len = recv_msg_len;
			sem_post(&sem_buf); 
			queue_len++;
			pthread_mutex_unlock(&lock[l_rebuf]);
			send_ack(recv_msg_id,sockfd,cliaddr);
		}
	}
	pthread_mutex_unlock(&lock[l_retab]);
}

void HandleReceive(int sockfd)
{
	int recv_msg_len = -1;
	errno = 0;
	char recv_buf[N+M];
	struct sockaddr recv_cliaddr;
	int cli_len = sizeof(recv_cliaddr);
	recv_msg_len = recvfrom(sockfd, (char *)recv_buf, N, MSG_DONTWAIT, 
    ( struct sockaddr *) &recv_cliaddr, &cli_len);
	if(dropMessage(P) == 1)
		return;
	if(recv_msg_len < 0)
		perror("Locking error\n");

	if(errno == EWOULDBLOCK || errno == EAGAIN)
	{
		perror("Error in message receive\n");
	}
	else{
		char recv_message_type[M],recv_msg[N],temp_id[M];
		int recv_msg_id;
		// for(int j = 0;j < recv_msg_len;j++)
		memcpy(recv_message_type,recv_buf,sizeof(char));
		memcpy(temp_id,recv_buf+sizeof(char),sizeof(int));
		memcpy(&recv_msg_id,(int *)temp_id,sizeof(int));
		if(recv_message_type[0] == 'm')
		{
			memcpy(recv_msg,recv_buf+sizeof(char)+sizeof(int),(recv_msg_len)*sizeof(char)-sizeof(char)-sizeof(int));
			recv_msg[(recv_msg_len-sizeof(char)-sizeof(int))*sizeof(char)] = '\0';
			HandleAppMsgRecv(recv_msg_id,recv_msg,recv_msg_len-sizeof(char)-sizeof(int),sockfd,recv_cliaddr);
		}
		else if(recv_message_type[0] == 'a')
		{
			HandleACKMsgRecv(recv_msg_id);
		}
		else 
		{
			perror("Message received not in proper format");
		}
	}
}

void * func_x(void *sfd)
{
	int *sockfdptr = (int *)sfd;
	int sockfd = sockfdptr[0];

	fd_set select_fd;
	int num_nfds = sockfd+1, msg_recv=0;
	struct timeval wait_time;

	while(1)
	{
		FD_ZERO(&select_fd);
		if(msg_recv==0)
		{
			wait_time.tv_sec = 2; //todo
			wait_time.tv_usec = 0;
		}
		FD_SET(sockfd,&select_fd);
		int r = select(num_nfds,&select_fd,0,0,&wait_time);
		if(r < 0)
		{
			perror("Select failed\n");
			exit(EXIT_FAILURE);
		}
		if(FD_ISSET(sockfd,&select_fd))
		{
			HandleReceive(sockfd);
			msg_recv=1;
		}
		else
		{
			HandleRetransmit(sockfd);
			msg_recv=0;
		}
	}
}

void init()
{
	receive_message_id = (recv_table *)malloc(N*sizeof(recv_table));
	unacknowledged_message_table = (unack_table *)malloc(N*sizeof(unack_table));
	receive_buffer = (recv_buffer *)malloc(N*sizeof(recv_buffer));
	unack_cnt = 0;
	srand(time(NULL));
	for(int i = 0;i < NUM_LOCKS;i++)
	{
		if(pthread_mutex_init(&lock[i], NULL) != 0) 
		{ 
		perror("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE);
		}
	} 
	sem_init(&sem_buf, 0, 0);
	pthread_mutex_lock(&lock[l_retab]);
	pthread_mutex_lock(&lock[l_unack]);
	queue_start = 0;
	queue_len = 0;
	for(int i = 0;i < N;i++)
	{
		receive_message_id[i].valid_bit = -1;
		unacknowledged_message_table[i].message_id = -1;
	}
	pthread_mutex_unlock(&lock[l_unack]);
	pthread_mutex_unlock(&lock[l_retab]);
}

void create_x(int sockfd)
{
	int *temp_sockfd = malloc(sizeof(int));
	temp_sockfd[0] = sockfd; 
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&tid,&attr,func_x,temp_sockfd);
	if(debug_bit)
		printf("create x %d\n",temp_sockfd[0]);
}

int r_socket(int address_family,int type,int flag)
{
	int sockfd;
	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		perror("Unable to create socket\n");
		return -1;
	}
	init();
	create_x(sockfd);
	return sockfd;
}

int r_bind(int sockfd, const struct sockaddr * servaddr,int serv_len)
{
	if(bind(sockfd,servaddr,serv_len) < 0)
	{
		perror("Bind failed");
		return -1;
	}
	return 0;
}

void r_sendto(int sockfd, const char *message,int message_len,int flag, const struct sockaddr * cliaddr, int cliaddr_len)
{
	char temp_message[100];
	memcpy(temp_message,message,message_len*sizeof(char));
	send_message(sockfd,temp_message,message_len,flag,cliaddr,cliaddr_len,-1);
	if(debug_bit)
		printf("msg %s\n",message);
}

int r_recvfrom(int sockfd, char * buffer,int buffer_len, int flag, struct sockaddr * cliaddr,int *len)
{
	int temp_queue_len;
	sem_wait(&sem_buf);
	pthread_mutex_lock(&lock[l_rebuf]);
	memcpy(buffer,&receive_buffer[queue_start].message[0],receive_buffer[queue_start].message_len*sizeof(char));
	int recv_msg_len = receive_buffer[queue_start].message_len;
	queue_start = (queue_start+1)%N;
	queue_len--;
	pthread_mutex_unlock(&lock[l_rebuf]);

	return recv_msg_len;
}

void free_mem(int sockfd)
{
	free(receive_message_id);
	free(unacknowledged_message_table);
	free(receive_buffer);
	sem_destroy(&sem_buf);
	for(int i=0;i<NUM_LOCKS;i++)
		pthread_mutex_destroy(&lock[i]);
	close(sockfd);
}

void r_close(int sockfd)
{
	struct timeval close_req_time;
	gettimeofday(&close_req_time,NULL);
	pthread_mutex_lock(&lock[l_glob_unack_cnt]);
	int prev_unack_cnt=unack_cnt;
	pthread_mutex_unlock(&lock[l_glob_unack_cnt]);
	int num_timeout=0;

	while(num_timeout<3)
	{
		pthread_mutex_lock(&lock[l_glob_unack_cnt]);
		if(unack_cnt>0)
		{
			if(prev_unack_cnt!=unack_cnt)
				num_timeout=0;
			else
				num_timeout++;
			prev_unack_cnt=unack_cnt;
			pthread_mutex_unlock(&lock[l_glob_unack_cnt]);
			sleep(1);
		}
		else
		{
			pthread_mutex_unlock(&lock[l_glob_unack_cnt]);
			break;
		}
	}
	free_mem(sockfd);
}
