#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <time.h>

#define MAXBUFSIZE	(1024)

void put_file(int, struct sockaddr_in);
void get_file(int, struct sockaddr_in);
void list_directory(int, struct sockaddr_in);
void delete(int, struct sockaddr_in);

typedef enum
{
  Get_e = 1,
  Put_e,
  Delete_e,
  List_e,
  Exit_e
}command_e;

//Structure for transmitter and receiver packet. 1 packet = 576 bytes (MTU for IPv4)
typedef struct 
{
	int index;
	int data_length;
	char buffer[MAXBUFSIZE];
}packet_t;

struct timeval timeout;
char file_name[10];
int pktsz, key_len; 
char message[100];
int file_size;
struct sockaddr_in remote;      	//"Internet socket address structure"
unsigned int remote_length;         	//length of the sockaddr_in structure
int nbytes;                        	//number of bytes we receive in our message
void *buffer;             		//a buffer to store our received message
packet_t *pkt = NULL, *pkt_ack = NULL;
FILE *fp;
char key[] = "123456789098765432123456789098765432123456789098765432123456789";
FILE *fp_ls;
char reading_list =0;

void get_file(int sockfd, struct sockaddr_in remote)
{
	int index_req, errsv, count, sent_index;
	long num_pkts, num_bytes;
	int loop_count = 0;
	printf("Entering get function\n");
	remote_length = sizeof(remote);
	nbytes = recvfrom(sockfd, file_name, sizeof(file_name), 0, (struct sockaddr *)&remote, &remote_length);
	printf("%S",file_name);
	fp = fopen(file_name, "r+");

	if(fp == NULL)
	{
		errsv = errno;
		printf("ERRNO is %d\n", errsv);
		perror("Could not open file");
		//send NACK to client
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	printf("Length of file is %d bytes\n", file_size);
	nbytes = sendto(sockfd, &file_size, (sizeof(file_size) + 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	nbytes = recvfrom(sockfd, message, sizeof(message), 0, (struct sockaddr *)&remote, &remote_length);
	printf("File name is %s\n", file_name);

	if(strstr(message, "Client :: Ready to Receive"))
	{
		memset(message, 0, sizeof(message));
		
		printf("Size of data buffer is %ld\n", sizeof(pkt->buffer));
		pkt = (packet_t *)malloc(pktsz);
		pkt_ack = (packet_t *)malloc(pktsz);

		num_pkts = file_size / pktsz;
		printf("Number of packets needed is %ld\n", num_pkts);

		pkt->index = 0;
		count = 0;

		//Setting timeout using setsockopt()
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;		//500ms timeout
		setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

		while(file_size > 0)
		{
			loop_count = 0;
			pkt->index++;
			sent_index = pkt->index;
			num_bytes = fread(pkt->buffer, 1, (int)MAXBUFSIZE, fp);

			//64-bit encryption. XOR every byte
			if (!reading_list){
				while(loop_count<num_bytes)
				{
					pkt->buffer[loop_count] ^= key[loop_count % (key_len-1)];
					++loop_count;
				}
			}			

			pkt->data_length = num_bytes;

			nbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			printf("Sent index is %d\n", pkt->index);
			memset(pkt_ack, 0, pktsz);
			nbytes = recvfrom(sockfd, pkt_ack, pktsz, 0, (struct sockaddr *)&remote, &remote_length);
			while((nbytes < 0) || (pkt_ack->index != sent_index)) {
				errsv = errno;
				memset(pkt_ack, 0, pktsz);
				printf("The error number is %d\n", errsv);
				//Sending data packet again if timeout occurs
				nbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
				printf("Send index is %d\n", pkt->index);
				nbytes = recvfrom(sockfd, pkt_ack, pktsz, 0, (struct sockaddr *)&remote, &remote_length);
			}

			printf("ACK received :: %d \n", pkt->index);
			file_size = file_size - num_bytes;
			count++;
		}
		fclose(fp);
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	}
	printf("Number of ACKs is %d\n", count);
}


void put_file(int sockfd, struct sockaddr_in remote)
{
	int index_req, errsv, count, sent_index;
	long num_pkts, num_bytes;
	int loop_count = 0;
	printf("Entering put function\n");
	remote_length = sizeof(remote);
	nbytes = recvfrom(sockfd, file_name, sizeof(file_name), 0, (struct sockaddr *)&remote, &remote_length);
	printf("%S",file_name);
	fp = fopen(file_name, "w+");

	if(fp == NULL)
	{
		errsv = errno;
		printf("ERRNO is %d\n", errsv);
		perror("Could not create file");
		exit(1);
	}
	pkt = (packet_t *)malloc(pktsz);
	pkt_ack = (packet_t *)malloc(pktsz);

	strcpy(message, "SERVER :: Ready to Receive");
	nbytes = sendto(sockfd, message, (sizeof(file_size) + 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	nbytes = recvfrom(sockfd, &file_size, sizeof(file_size), 0, (struct sockaddr *)&remote, &remote_length);
	printf("File size is %d\n", file_size);
	memset(message,0, sizeof(message));
	strcpy(message, "SERVER :: Got Size");
	nbytes = sendto(sockfd, message, (sizeof(message) + 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	
	index_req = 1;
	pkt_ack->index = 1;
	//Setting the timeout using setsockopt()
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;  //500ms timeout
	setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	while(file_size > 0)
	{
		loop_count = 0;
		nbytes = recvfrom(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, &remote_length);
		while((nbytes < 0) || (pkt->index < index_req)) {
			errsv = errno;
			memset(pkt_ack, 0, pktsz);
			printf("The error number is %d\n", errsv);
			pkt_ack->index = index_req - 1; 
			nbytes = sendto(sockfd, pkt_ack, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));			
			nbytes = recvfrom(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, &remote_length);
		}
		printf("Correct data received\n");
		printf("Received index is %d\n", pkt->index);

		//64-bit encryption. XOR every byte
		while(loop_count<pktsz)
		{
			pkt->buffer[loop_count] ^= key[loop_count % (key_len-1)];
			++loop_count;
		}		
		
		fwrite(pkt->buffer, 1, pkt->data_length, fp);
		file_size = file_size - pkt->data_length;
		printf("Current file size is %d\n", file_size);
		memset(pkt, 0, pktsz);
		pkt_ack->index = index_req;
		nbytes = sendto(sockfd, pkt_ack, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		index_req++;
	}
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	fclose(fp);
	printf("Client to Server file transfer complete\n");
}


void list_directory(int sockfd, struct sockaddr_in remote)
{
	printf("Entering ls function\n");
	// strcpy(file_name, "ls_dir");
	fp_ls = fopen("ls_dir", "w+");
	reading_list = 1;
	if(fp_ls == NULL)
	{
		int errsv = errno;
		printf("ERRNO is %d\n", errsv);
		perror("could not open file");
		exit(1);
	}

	system("ls -la> ls_dir");
	get_file(sockfd, remote);
	reading_list = 0;
}

void delete(int sockfd,struct sockaddr_in remote)
{

	remote_length = sizeof(remote);
	nbytes = recvfrom(sockfd, file_name, sizeof(file_name), 0, (struct sockaddr *)&remote, &remote_length);
	printf("%s",file_name);
	if (remove(file_name) == 0)
          printf(" %s deleted successfully\n", file_name);
        else
          printf("Unable to delete the file");
}

int main (int argc, char * argv[] )
{

	int sockfd;                           //This will be our socket
	struct sockaddr_in sin;
	// struct sockaddr_in sin, remote;      //"Internet socket address structure"
	// unsigned int remote_length;         //length of the sockaddr_in structure
	// int nbytes;                        //number of bytes we receive in our message
	// void *buffer;             //a buffer to store our received message
	// PACKET *pkt = NULL;
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	memset(&sin,0, sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create server socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);
	pktsz = sizeof(pkt->buffer) + sizeof(pkt->index) + sizeof(pkt->data_length);
	key_len = strlen(key);
	while(1)
	{
		printf("Waiting for a request...\n");
		nbytes = recvfrom(sockfd, message, (int)MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
		printf("Request :: %s\n", message);	
		if(strstr(message, "Get_e") != NULL)
		{
			memset(message,0, sizeof(message));
			strcpy(message, "ready_get");
			printf("Server :: %s\n", message);
			nbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			get_file(sockfd, remote);
		}
		else if(strstr(message, "Put_e") != NULL)
		{
			memset(message,0, sizeof(message));
			strcpy(message, "ready_put");
			printf("Server :: %s\n", message);
			nbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			put_file(sockfd, remote);
		}

		else if(strstr(message, "List_e") != NULL)
		{
			memset(message,0, sizeof(message));
			strcpy(message, "ready_ls");
			printf("Server :: %s\n", message);
			nbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			list_directory(sockfd, remote);
		}	

		else if(strstr(message, "Delete_e") != NULL)
		{
			memset(message,0, sizeof(message));
			strcpy(message, "ready_delete");
			printf("Server :: %s\n", message);
			nbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			delete(sockfd,remote);
		}	

		else if(strstr(message, "Exit_e") != NULL)
		{
			memset(message, 0, sizeof(message));
			strcpy(message, "ready_exit");
			printf("Server :: %s\n", message);
			nbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			printf("Server stoped\n");
			break;
		}
		else {
			strcpy(message, "CMD_Not_Valid");
			printf("Server :: %s\n", message);
			nbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		}
	}
	close(sockfd);
}
