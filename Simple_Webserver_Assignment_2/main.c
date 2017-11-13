#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<time.h>
#include<sys/time.h>
#include<signal.h>

#define CONNMAX 1000
#define BYTES 1024

int mylisten, clients[CONNMAX];
int client_num=0;

/*This function extracts the size information of the file*/
int filesize(int file_desc)
{
  struct stat fileinfo;
  if(fstat(file_desc, &fileinfo) == -1)
  {
    printf("Could not get the file info\n\r");
    return -1;
  }
  return (int)fileinfo.st_size;
}

/*Convert ascii to integer*/
char* itoa(int num,char *str)
{
   if(str == NULL)
   	return NULL;
   sprintf(str,"%d",num);
   return str;
}

/*timeout after the keepalive timer closes the client on which timeout occured*/
void handle_timeout(int sig)
{
     printf("!!!Timeout!!!\n");
     printf("closing socket %d\n",client_num); 
     shutdown (clients[client_num], SHUT_RDWR);         
     close(clients[client_num]);
     clients[client_num]=-1;
     exit(0);    
}

/*Helper function to reverse string*/
char* stringreverse(char *str)
{
    char *start,*end;
    if(! str || ! *str)
    	return str;
    for(start=str,end=str+strlen(str)-1; end>start; ++start,--end)
    {
    	*start ^= *end;
    	*end ^= *start;
    	*start ^= *end;
    }
    return str;
}

/*All the config values from w.conf are read using string parsing and finding a particular search string.*/
char* read_config(char *search_string)
{
 int file;
 char filename[] = "ws.conf";
 char buffer[4000];
 char *searchresult = NULL;
 char *info = NULL;
 file = open("ws.conf",O_RDONLY);
 if(file == -1){
 	printf("Could not open configuration file\n\r");
	return info;
 }
 read(file,buffer,filesize(file));
 if((searchresult = strstr(buffer,search_string)) != NULL){
 	info = strtok(searchresult," \t\n");
 	info = strtok(NULL," \t\n");
 }
 close(file);
 return info;
}

/*All the supported files from w.conf are read using string parsing and finding a particular filetype.*/
char* read_fileformat(char* file_info,char *file_type)
{
 int file;
 char filename[] = "ws.conf";
 char buffer[4000];
 char *searchresult = NULL;
 char *info = NULL;
 char *parse = NULL;
 file = open("ws.conf",O_RDONLY);
 if(file == -1) {
 	printf("Could not open configuration file\n\r");
	return info;
 }
 read(file,buffer,filesize(file));
 if((searchresult = strstr(buffer,file_info)) != NULL) {
	parse = strstr(searchresult,file_type);
 	info = strtok(parse," \t\n");
 	info = strtok(NULL," \t\n");
 }
 close(file);
 return info;   
}

void send_to_client(int socket_desc,char *msg)
{
   printf("\n\r");
   printf("______________Server Response______________\n\r");
   printf("%s\n",msg);
   printf("___________________________________________\n\r");
   printf("\n\r");
   int length = strlen(msg);
   if(send(socket_desc,msg,length,0) == -1) {
   	printf("failed to send\n");
   }
}

void Start_Server(char *port)
{
	struct addrinfo hints, *res, *j;
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0) {
		perror ("error while getting addr info \n\r");
		exit(1);
	}
	for (j = res; j!=NULL; j=j->ai_next) {
		mylisten = socket (j->ai_family, j->ai_socktype, 0);
		if (mylisten == -1) 
			continue;
		if (bind(mylisten, j->ai_addr, j->ai_addrlen) == 0) 
			break;
	}
	if (j==NULL) {
		perror ("socket error\n\r");
		exit(1);
	}
	freeaddrinfo(res);
	if ( listen(mylisten, 1000000) != 0 ) {
		perror("could not listen to incoming info \n\r");
		exit(1);
	}
}

void handle_post(char* response,char* errmsg,int n, char *http){
     char data_to_send[BYTES];
     int  bytes_read, fd;
     printf("handle post request\n"); 
     strcpy(response,http);
     strcat(response,"200 Document Follows\n");
     strcat(response,"Content-Size : NONE \n");
     strcat(response,"Content-Type : Invalid \n\n");
     send_to_client(clients[n],response);
     strcpy(errmsg,"<html><body><pre><h1>");
     strcat(errmsg,"POSTDATA");
     strcat(errmsg,"</h1></pre> ");
     strcat(errmsg,"</BODY></html>");
     strcat(errmsg,"\n\n");
     send_to_client(clients[n],errmsg);
     if ( (fd=open("/home/vikhyat/Netsys/PA2/www/index.html", O_RDONLY))!=-1 )    {	
        while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
	   write (clients[n], data_to_send, bytes_read);
     }
}

void handle_head(char* response,char* errmsg,int n, char *http,char *clientrequest){
     printf("handle head request\n"); 
     strcpy(response,http);
     strcat(response,"501 Not Implemented\n");
     strcat(response,"Content-Size : NONE \n");
     strcat(response,"Content-Type : Invalid \n\n");
     send_to_client(clients[n],response);
     strcpy(errmsg,"<HEAD><TITLE>501 Not Implemented Reason</TITLE></HEAD>\n");
     strcat(errmsg,"<html><BODY>>501 Not Implemented");
     strcat(errmsg,clientrequest);
     strcat(errmsg,"\n");
     strcat(errmsg,"</BODY></html>");
     send_to_client(clients[n],errmsg);
}                   

void handle_nofile(char* response,char* errmsg,int n, char *http, char *path ){
    printf("handle request for unknown file\n");
    strcpy(response,http);
    strcat(response,"404 Not Found\n");
    strcat(response,"Content-Size : NONE \n");
    strcat(response,"Content-Type : Invalid \n\n");
    send_to_client(clients[n],response);
    strcpy(errmsg,"<HEAD><TITLE>404 File not found Reason</TITLE></HEAD>\n");
    strcat(errmsg,"<html><BODY>400 File not found Request URL doesn't exist:");
    strcat(errmsg,path);
    strcat(errmsg,"\n");
    strcat(errmsg,"</BODY></html>");
    send_to_client(clients[n],errmsg);
}

void handle_httpversion(char* response,char* errmsg,int n, char *http,char *clientrequest){
     printf("handle wrong http request\n"); 
     strcpy(response,http);
     strcat(response,"400 Not Found\n");
     strcat(response,"Content-Size : NONE \n");
     strcat(response,"Content-Type : Invalid \n\n");
     send_to_client(clients[n],response);
     strcpy(errmsg,"<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>\n");
     strcat(errmsg,"<html><BODY>>400 Bad Request Reason: Invalid HTTP-Version:");
     strcat(errmsg,clientrequest);
     strcat(errmsg,"\n");
     strcat(errmsg,"</BODY></html>");
     send_to_client(clients[n],errmsg);
}

/* Main handler for all the client requests*/
void process_client_request(int n,char* ROOT)
{
	char mesg[99999], *clientrequest[3], data_to_send[BYTES], keepalivestring[10000];
	int rcvd, fd, bytes_read;
        char size[7];
        char *defaultload = malloc(15);
        char *file_name = malloc(30);
        char *format_type = malloc(5);
        char *token = malloc(20);
        char *file_type = malloc(20);
        char *content_type = malloc(10);
        char *errmsg = malloc(3000);
	char *response = malloc(5000);
	char *post_mesg = malloc(5000);
        char *path = malloc(5000);
        char *http = "HTTP/1.1 ";
	char *time_out;
	int connection = 1;
	while(connection){
 	 memset( (void*)mesg, (int)'\0', 10000 );
 	 rcvd=recv(clients[n], mesg, 10000, 0);
         client_num = n;
	if (rcvd<0)   
		fprintf(stderr,("error while receive\n"));
	else if (rcvd==0)    
		fprintf(stderr,"No request from client\n");
	else    
	{
		printf("%s \n\r", mesg);
		strcpy(keepalivestring,mesg);
		clientrequest[0] = strtok (mesg, " \t\n");
		if ( strncmp(clientrequest[0], "GET\0", 4)==0 )
		{
			clientrequest[1] = strtok (NULL, " \t");
			clientrequest[2] = strtok (NULL, " \t\n");
			if ( strncmp( clientrequest[2], "HTTP/1.0", 8) == 0)
                           http = "HTTP/1.0 ";
                        if(strncmp( clientrequest[2], "HTTP/1.1", 8) == 0 )
                           http = "HTTP/1.1 ";  
                        if( strncmp( clientrequest[2], "HTTP/1.0", 8) != 0 && strncmp( clientrequest[2], "HTTP/1.1", 8) != 0 ){
                           handle_httpversion(response,errmsg,n,http,clientrequest[2]);
			}
			else {
			   if (strncmp(clientrequest[1], "/\0", 2)==0 ) {
                                        strcpy(defaultload,"/");
                                        strcat(defaultload,read_config("DirectoryIndex"));
                                        strcpy(clientrequest[1],defaultload);
                                }
                                   
                                strcpy(file_name,clientrequest[1]);
				strncpy(path, ROOT+1,strlen(ROOT)-2);
				strcpy(&path[strlen(ROOT)-2], clientrequest[1]);
                                strcpy(format_type,path);
                                format_type = stringreverse(format_type);
                                token = strtok(format_type,".");
                                strcpy(file_type,".");
                                strcat(file_type,stringreverse(token));
                                content_type = read_fileformat("#Content-Type which the server handles",file_type);
                               
		                if ( (fd=open(path, O_RDONLY))!=-1 )    
				{
					itoa(filesize(fd),size);
                                        strcpy(response,http);
                                        strcat(response,"200 Document Follows\n");
                                        strcat(response,"Content-Size : ");
                                        strcat(response,size);
                                        strcat(response,"\n");
                                        strcat(response,"Content-Type : ");
                                        strcat(response,content_type);
                                        strcat(response,"\n\n");
                                        send_to_client(clients[n],response);
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
						write (clients[n], data_to_send, bytes_read);
				}
				else{
                                   handle_nofile(response,errmsg,n,http,path);      
                                }
			}
		}
                else if ( strncmp(clientrequest[0], "HEAD\0", 4)==0 ) {
		    handle_head(response,errmsg,n,http,clientrequest[0]);
                }
                else if ( strncmp(clientrequest[0],"POST\0",4)==0) {
		    handle_post(response,errmsg,n,http);
                }
		printf("keepalive %s \n\r", keepalivestring);
		if(strstr(keepalivestring,"Connection")!= NULL)
                {
                 time_out = read_config("Alivetime");
                 printf("time_out interval is %d\n",atoi(time_out));
                 printf("timer started\n");
                 alarm(atoi(time_out));
                }
                else //also for else if(strstr(keepalivestring,"Connection: close")!= NULL)
                {
		 printf("no keep alive in request, close session");
	         shutdown (clients[n], SHUT_RDWR);         
	         close(clients[n]);
                 clients[n]=-1;
		 connection = 0;
                 exit(0);
                }
	    }
  }

	shutdown (clients[n], SHUT_RDWR);         
	close(clients[n]);
	clients[n]=-1;
        exit(0);
}


int main(int argc, char* argv[])
{
	struct sockaddr_in clnt_addr;
	socklen_t addrlen;
        char* ROOT = malloc(30);
	char* PORT = malloc(6);
	int slot=0;
	int i;
        printf("Waiting for client request.....\n");
        strcpy(ROOT,read_config("DocumentRoot"));
        strcpy(PORT,read_config("Listen"));
	printf("Server started with port no- %s and root directory at %s \n\r",PORT,ROOT); 

	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	Start_Server(PORT);
	addrlen = sizeof(clnt_addr);
        signal(SIGALRM, handle_timeout);
	while (1) {
	  clients[slot] = accept (mylisten, (struct sockaddr *) &clnt_addr, &addrlen);
	  if (clients[slot]<0)
		printf ("could not accept request");
    	  else {
		if ( fork()==0 ) {
		  process_client_request(slot,ROOT);
		  exit(0);
		}
	  }
	  while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
	}
	return 0;
}


