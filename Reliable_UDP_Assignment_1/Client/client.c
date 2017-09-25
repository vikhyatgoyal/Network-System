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

void put_file(char *, int, 	struct sockaddr_in, struct sockaddr_in);
void get_file(char *, int, 	struct sockaddr_in, struct sockaddr_in);
void list_directory(int, struct sockaddr_in, struct sockaddr_in);
void delete(char *,int, struct sockaddr_in, struct sockaddr_in);
int execute_transfer(char* server_response, char* fl, struct sockaddr_in remote, struct sockaddr_in from_addr );

//Structure for transmitter and receiver packet. 1 packet = 576 bytes (MTU for IPv4)
typedef struct 
{
	int index;
	int data_length;
	char buffer[MAXBUFSIZE];
}packet_t;

typedef enum
{
  Get_e = 1,
  Put_e,
  Delete_e,
  List_e,
  Exit_e
}command_e;

int sent_index, count, error, pktsz, key_len;
long num_bytes, num_pkts;
char message[100];
char option[25];
char file_name[10];
int len;
int errsv;				//Store errno
int sbytes = -1;                        //number of bytes send by sendto()
int rbytes = -1;                        //number of bytes send by recvfrom()
int sockfd;                             //this will be our socket
struct timeval timeout;
packet_t *pkt = NULL, *pkt_ack = NULL;
FILE *fp;
FILE *fp_temp;
char key[] = "123456789098765432123456789098765432123456789098765432123456789"; //64-byte key for XOR encryption
char reading_list = 0;


char* Select_Option()
{
	char *result = malloc(10*sizeof(char));
	printf("_____________________________________\n"); 
	printf("What you want to do? Select an Option\n");
	printf("_____________________________________\n");
	printf(" 1)	get <file>\n");
	printf(" 2)	put <file>\n");
	printf(" 3)	delete <file>\n");
	printf(" 4)	ls\n");
	printf(" 5)	exit\n");
	printf("_____________________________________\n");
	fgets(option, sizeof(option), stdin);
	option[strcspn(option, "\n")] = 0;

	if(strstr(option, "get")) {
	strcpy(result, "Get_e");
	}
	else if(strstr(option,"put")) {
	strcpy(result, "Put_e");
	}
	else if(strstr(option,"delete")) {
	strcpy(result, "Delete_e");
	}
	else if(strstr(option,"ls")) {
	strcpy(result, "List_e");
	}
	else if(strstr(option,"exit")) {
	strcpy(result, "Exit_e");
	}
	else {
	printf("Not a Valid Option, try again\n");
	return(Select_Option(option));
	}
	return(result);
}


int main (int argc, char * argv[])
{	
	char *filename = malloc(25*(sizeof(char)));
	char *choice = malloc(10*sizeof(char));
	int close_loop = 0;
	struct sockaddr_in remote;  //"Internet socket address structure"

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
	
	memset(message, 0, sizeof(message));
	pktsz = sizeof(pkt->buffer) + sizeof(pkt->index) + sizeof(pkt->data_length);
	key_len = strlen(key);
	printf("Size of each packet is %d\n", pktsz);

	while(!close_loop)
	{
		choice = Select_Option();
		printf("Requesting Server Functionality.....\n");
		printf("%s",option);
	    	sbytes = sendto(sockfd, choice, (sizeof(choice)), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		rbytes = recvfrom(sockfd, message, (int)MAXBUFSIZE, 0, (struct sockaddr *)&from_addr, &addr_length);  
		printf("Server Response :: %s \n", message);	
		close_loop = execute_transfer(message,option, remote, from_addr);		
	}
	close(sockfd);
}

int execute_transfer(char* server_response, char* fl, struct sockaddr_in remote, struct sockaddr_in from_addr ){
	int result = 0;
	if(strstr(server_response, "ready_put")) {
		memset(server_response, 0, sizeof(server_response));
		strcpy(file_name, (fl + 4));
		put_file(file_name, sockfd, remote, from_addr);
	}

	if(strstr(server_response, "ready_get")) {
		memset(server_response, 0, sizeof(server_response));
		strcpy(file_name, (fl + 4));
		get_file(file_name, sockfd, remote, from_addr);
	}

	if(strstr(server_response, "ready_ls")) {
		memset(server_response, 0, sizeof(server_response));
		list_directory(sockfd, remote, from_addr);
	}

	if(strstr(server_response, "ready_delete")) {
		memset(server_response, 0, sizeof(server_response));
		strcpy(file_name, (fl + 7));
		delete(file_name, sockfd, remote, from_addr);
	}

	if(!strcmp(server_response, "ready_exit")) {
		memset(server_response, 0, sizeof(server_response));
		printf("Server wants to stop\n");
		strcpy(server_response, "Okay do it");
	   	sbytes = sendto(sockfd, server_response, (sizeof(server_response) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		printf("Client is all alone now....\n");
		printf("Client is exiting because it is useless\n");
		result = 1;
	}
	return(result);
}

void get_file(char *file_name, int sockfd, struct sockaddr_in remote, struct sockaddr_in from_addr)
{
	int index_req, old;
	long file_size;
	int loop_count = 0;
	socklen_t addr_length = sizeof(struct sockaddr);
	strcpy(message, file_name);
	sbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));

	memset(&file_size, 0, sizeof(file_size));
	rbytes = recvfrom(sockfd, &file_size, sizeof(file_size), 0, (struct sockaddr *)&from_addr, &addr_length);  
	printf(" Size of file :: %ld \n", file_size);
	
	pktsz = sizeof(pkt->buffer) + sizeof(pkt->index) + sizeof(pkt->data_length);
	pkt = (packet_t *)malloc(pktsz);

	fp = fopen(file_name, "w");

	strcpy(message, "Client :: Ready to Receive");
	sbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));

	index_req = 1;

	/*Need to set timeout at server only*/
	while(file_size > 0)
	{
		loop_count = 0;
		rbytes = recvfrom(sockfd, pkt, pktsz, 0, (struct sockaddr *)&from_addr, &addr_length);  
		if(pkt->index == index_req) {
			printf("Pkt :: correct\n");
			printf("Received index :: %d\n", pkt->index);

        		//64-bit decryption. double XOR every byte in packet to recover original data
			if (!reading_list){
				while(loop_count<rbytes)
				{
					pkt->buffer[loop_count] ^= key[loop_count % (key_len-1)] ^ key[loop_count % (key_len-1)];
					++loop_count;
				}
			}

			fwrite(pkt->buffer, 1, pkt->data_length, fp);
			file_size = file_size - pkt->data_length;
			printf("Current file size is %ld\n", file_size);
			memset(pkt, 0, pktsz);
			pkt->index = index_req;
			sbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			index_req++;
		}
		else if (pkt->index < index_req){
			printf("Pkt :: old\n");
			printf("Received index :: %d\n", pkt->index);
			old =pkt->index; 
			memset(pkt, 0, pktsz);
			pkt->index = old;
			sbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
		}
		else {
			printf("Pkt :: incorrect\n");
			printf("Received index :: %d\n", pkt->index); 
			memset(pkt, 0, pktsz);
			pkt->index = 0;
			sbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));	
		}
		memset(pkt, 0, pktsz);
	}
	fclose(fp);
	printf("Server to Client file transfer complete\n");
}

void put_file(char *file_name, int sockfd, struct sockaddr_in remote, struct sockaddr_in from_addr)
{
	int index_req, old;
	long file_size;
	int loop_count = 0;
	socklen_t addr_length = sizeof(struct sockaddr);
	strcpy(message, file_name);
	sbytes = sendto(sockfd, message, (sizeof(message) - 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	memset(&message, 0, sizeof(message));

	rbytes = recvfrom(sockfd, &message, sizeof(message), 0, (struct sockaddr *)&from_addr, &addr_length);  
	printf(" %s \n", message);
	
	pktsz = sizeof(pkt->buffer) + sizeof(pkt->index) + sizeof(pkt->data_length);
	pkt = (packet_t *)malloc(pktsz);
	printf("%s",file_name);
	fp = fopen(file_name, "r+");

	if(fp == NULL)
	{
		errsv = errno;
		printf("ERRNO is %d\n", errsv);
		perror("could not open file");
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	printf("Length of file is %ld bytes\n", file_size);
	sbytes = sendto(sockfd, &file_size, (sizeof(file_size) + 1), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	memset(message, 0, sizeof(message));
	sbytes = recvfrom(sockfd, message, sizeof(message), 0, (struct sockaddr *)&remote, &addr_length);

	if(!(strcmp(message, "SERVER :: Got Size")))
	{
		pkt = (packet_t *)malloc(pktsz);
		pkt_ack = (packet_t *)malloc(pktsz);

		num_pkts = file_size / pktsz;
		printf("Number of packets needed is %ld\n", num_pkts);
		
		pkt->index = 0;
		count = 0;

		while(file_size > 0)
		{
			loop_count = 0;
			pkt->index++;
			sent_index = pkt->index;

			num_bytes = fread(pkt->buffer, 1, (int)MAXBUFSIZE, fp);


			//64-bit encryption. XOR every byte in packet
			while(loop_count<nbytes)
			{
				pkt->buffer[loop_count] ^= key[loop_count % (key_len-1)];
				++loop_count;
			}

			pkt->data_length = num_bytes;

			sbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
			printf("Sent index is %d\n", pkt->index);
			memset(pkt_ack, 0, pktsz);
			rbytes = recvfrom(sockfd, pkt_ack, pktsz, 0, (struct sockaddr *)&from_addr, &addr_length);  
			while((rbytes < 0) || (pkt_ack->index < sent_index)){
				memset(pkt_ack, 0, pktsz);
				sbytes = sendto(sockfd, pkt, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
				printf("Sent index is %d\n", pkt->index);
				rbytes = recvfrom(sockfd, pkt_ack, pktsz, 0, (struct sockaddr *)&from_addr, &addr_length); 
			}
			printf("ACK received : %d \n",pkt_ack->index);
			file_size = file_size - num_bytes;
			count++;
		}
		fclose(fp);
	}
	printf("Number of ACKs is %d\n", count);
}


void list_directory(int sockfd, struct sockaddr_in remote, struct sockaddr_in from_addr)
{
	char i;
	reading_list = 1;
	get_file("ls_dir", sockfd, remote, from_addr);
	printf("Listing files in server.....\n");
	fp = fopen("ls_dir", "r");
	if(fp)
	{	
		while((i = getc(fp)) != EOF)
			putchar(i);
		fclose(fp);
	}
	reading_list = 0;
}

void delete(char *file_name,int sockfd, struct sockaddr_in remote, struct sockaddr_in from_addr)
{
	socklen_t addr_length = sizeof(struct sockaddr);
	strcpy(message, file_name);
	sbytes = sendto(sockfd, message, pktsz, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr));
	printf("Deleting requested file in server\n");
}

