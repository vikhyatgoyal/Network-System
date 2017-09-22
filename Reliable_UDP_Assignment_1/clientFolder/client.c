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
#include <errno.h>
#include <string.h>
#include <time.h>

#define MAXBUFSIZE	(1024)

//Structure for transmitter and receiver packet. 1 packet = 576 bytes (MTU for IPv4)
typedef struct 
{
	int index;
	int data_length;
	char buffer[MAXBUFSIZE];
}PACKET;

int main (int argc, char * argv[])
{	
	int sent_index, count, error;
	long pkt_size, num_bytes, num_pkts;
	char command[100];
	char data[512];
	int len;
	int errsv;									 //Store errno
	int sbytes = -1;                             // number of bytes send by sendto()
	int rbytes = -1;                             // number of bytes send by recvfrom()
	int sockfd;                                  //this will be our socket
	void *buffer;
	struct timeval timeout;
	PACKET *pkt = NULL, *pkt_ack = NULL;
	FILE *fp;
	FILE *fp_temp;

	struct sockaddr_in remote;              //"Internet socket address structure"

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	memset(&remote,0, sizeof(remote));
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create client socket");
		exit(1);
	}

	struct sockaddr_in from_addr;
	socklen_t addr_length = sizeof(struct sockaddr);
	
	memset(command, 0, sizeof(command));
	pkt_size = sizeof(pkt->buffer) + sizeof(pkt->index) + sizeof(pkt->data_length);
	printf("Size of each packet is %ld\n", pkt_size);

	// while(1)
	// {
	// 	printf("\r\n     *** Main Menu ***\n\r");
	// 	printf("Enter the following commands for file transfers / handling\n");
 //        printf("get <file_name>\n");
 //        printf("put <file_name>\n");
 //        printf("delete <file_name>\n");
 //        printf("ls\n");
 //        printf("exit\n");
 //        printf("Type in the command followed by the <file_name>, if the command requires\n");
 //        // scanf("%s", command);
 //        // gets(command);
 //        fgets(command, sizeof(command), stdin);
 //        printf("Sending entered command to server\n");
 //    	sbytes = sendto(sockfd, command, (sizeof(command) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	// }

	strcpy(command, "Sending a text file");
	printf("%s\n", command);
	sbytes = sendto(sockfd, command, (sizeof(command) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	
	memset(command, 0, sizeof(command));
	rbytes = recvfrom(sockfd, command, (int)MAXBUFSIZE, 0, (struct sockaddr *)&from_addr, &addr_length);  
	printf("Server: %s\n", command);
	

	if(strcmp(command, "Okay") == 0)
	{
		memset(command, 0, sizeof(command));
		// sbytes = sendto(sockfd, len, (sizeof(command) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		fp = fopen("foo2", "r");		//open file in read mode
		fp_temp = fopen("foo1_temp", "w");

		if(fp == NULL)
		{
			perror("could not open file");
			exit(1);
		}

	   	fseek(fp, 0, SEEK_END);
   		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		
		printf("Length of file is %d bytes\n", len);
		sbytes = sendto(sockfd, &len, (sizeof(len) + 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		printf("Size of data buffer is %ld\n", sizeof(pkt->buffer));
		pkt = (PACKET *)malloc(pkt_size);
		pkt_ack = (PACKET *)malloc(pkt_size);

		num_pkts = len / pkt_size;
		printf("Number of packets needed is %ld\n", num_pkts);
		
		pkt->index = 0;
		count = 0;

		//Setting the timeout using setsockopt()
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;  //500ms timeout
		setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
		while(len > 0)
		{
			pkt->index++;
			sent_index = pkt->index;

			num_bytes = fread(pkt->buffer, 1, (int)MAXBUFSIZE, fp);
			pkt->data_length = num_bytes;

			sbytes = sendto(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			printf("Sent index is %d\n", pkt->index);
			memset(pkt_ack, 0, pkt_size);
			rbytes = recvfrom(sockfd, pkt_ack, pkt_size, 0, (struct sockaddr *)&from_addr, &addr_length);  
			while(rbytes < 0)
			{
				error = errno;
				memset(pkt_ack, 0, pkt_size);
				printf("The error number is %d\n", error);
				//Sending data packet again if timeout occurs
				sbytes = sendto(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
				printf("Sent index is %d\n", pkt->index);
				rbytes = recvfrom(sockfd, pkt_ack, pkt_size, 0, (struct sockaddr *)&from_addr, &addr_length);  
			}
			
			if(pkt_ack->index == sent_index)
			{
				printf("ACK received\n");
				len = len - num_bytes;
				count++;
			}
			else if(strcmp(pkt_ack->buffer, "Incorrect index. Send again"))
			{
				sbytes = sendto(sockfd, pkt, pkt_size, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			}
			else
			{
				printf("ACK not received\n");
				break;
			}
			// len = len - num_bytes;
		}
	}
	printf("Number of ACKs is %d\n", count);

	close(sockfd);
}