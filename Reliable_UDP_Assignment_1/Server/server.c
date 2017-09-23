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

int isfilepresent(const char *filename);

//Structure for transmitter and receiver packet. 1 packet = 576 bytes (MTU for IPv4)
typedef struct 
{
	int index;
	int data_length;
	int buffer[MAXBUFSIZE];
}PACKET;


int main (int argc, char * argv[] )
{
	int index_req; 
	char file_name[10];
	char msg[100];
	int pkt_size;
	int file_size;
	int sockfd;                           //This will be our socket
	struct sockaddr_in sin, remote;      //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	void *buffer;             //a buffer to store our received message
	PACKET *pkt = NULL;
	FILE *fp;
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

	// while(1)
	// {
	// 	memset(msg,0, sizeof(msg));
	// 	nbytes = recvfrom(sockfd, msg, (int)MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
	// 	printf("Client: %s\n", msg);	
	// 	if(strstr(msg, "get") != NULL)
	// 	{
	// 		strcpy(file_name, (msg + 3));
	// 		// printf("File name is %s\n", file_name);
	// 		// result = isfilepresent(file_name);
	// 		fp = fopen(file_name, "r");
	// 		if(fp == NULL)
	// 		{
	// 			perror("could not open file");
	// 			exit(1);
	// 		}

	// 		fseek(fp, 0, SEEK_END);
	// 		file_size[0] = ftell(fp);
	// 		// sbytes = sendto(sockfd, file_size, (sizeof(file_size) + 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	// 		fseek(fp, 0, SEEK_SET);
	// 		buffer = malloc(sizeof(file_size[0]));
	// 		fread(buffer, 1, file_size[0], fp);
	// 		printf("Sending requested file\n");
	// 		sbytes = sendto(sockfd, buffer, file_size[0], 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));

	// }
	memset(msg,0, sizeof(msg));
	// nbytes = recvfrom(sockfd, msg, (int)MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
	nbytes = recvfrom(sockfd, msg, sizeof(pkt), 0, (struct sockaddr *)&remote, &remote_length);
	printf("Client: %s\n", msg);

	memset(msg,0, sizeof(msg));
	strcpy(msg, "Okay");
	nbytes = sendto(sockfd, msg, (sizeof(msg) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));

	memset(&file_size, 0, sizeof(file_size));
	nbytes = recvfrom(sockfd, &file_size, sizeof(file_size), 0, (struct sockaddr *)&remote, &remote_length);
	printf("The size of the file to be received is %ld\n", file_size);
	
	pkt_size = sizeof(pkt->buffer) + sizeof(pkt->index) + sizeof(pkt->data_length);
	pkt = (PACKET *)malloc(pkt_size);

	printf("Writing received data in foo1\n");
	fp = fopen("foo1", "w");

	index_req = 1;
	while(file_size > 0)
	{
		for (long long i = 0; i < 1000000000; ++i)
		{
			/* code */
		}
		nbytes = recvfrom(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&remote, &remote_length);
		if(pkt->index == index_req)
		{
			printf("Correct data received\n");
			printf("Received index is %d\n", pkt->index);
			fwrite(pkt->buffer, 1, pkt->data_length, fp);
			file_size = file_size - pkt->data_length;
			printf("Current file size is %ld\n", file_size);
			memset(pkt, 0, pkt_size);
			pkt->index = index_req;
			nbytes = sendto(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			index_req++;
		}
		else
		{
			printf("Incorrect data received. Send data again\n");
			strcpy(pkt->buffer, "Incorrect index. Send again");
			nbytes = sendto(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			// printf("Incorrect Received index is %d\n", pkt->index);
			// break;
		}
		memset(pkt, 0, pkt_size);
	}

	close(sockfd);
}


int isfilepresent(const char *filename)
{

}
