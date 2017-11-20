#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#define BUFLEN 10248   // Choosing the packet size to 10248

// supported functionalities
#define GET   1  
#define PUT   2
#define LIST  3
#define CLOSE 4

#define DFS_SERVER1  0
#define DFS_SERVER2  1
#define DFS_SERVER3  2
#define DFS_SERVER4  3
#define TRUE 	     1
#define FALSE 	     0

#define SUCCESS 1
#define FAIL 0
#define TIMEOUT 1
#define MAX_CONNECTIONS 4
struct sockaddr_in  servAddr[MAX_CONNECTIONS];
socklen_t servlen[MAX_CONNECTIONS];
int sockfd[MAX_CONNECTIONS], slen=sizeof(servAddr);
struct hostent *server;
char Config_File[100];
/*****************************************************
 * change string to integer
 *****************************************************/
int str2int(char *str) {
int number = 0;
int i=0;
  for (i = 0; str[i] != '\0'; i++){
      number = number * 10 + str[i] - '0';
  }
  return number;
}

/*****************************************************
 * Client Menu
 *****************************************************/
void client_menu()
{
	printf("\n\n");
	printf("**************************************\n");
	printf("* Enter a command\n");
	printf("* get: get a file from server\n");
	printf("* put: put a file to server\n");
	printf("* ls: list files on the server\n");
	printf("* exit: close the socket\n");
	printf("**************************************\n");
	printf("\n\n");

}


/*****************************************************
 Decides the command to send based on use input
 *****************************************************/
char user_commands() {
	char choice[6];
	char choiceValue;
	client_menu();
	
	while(1) {
		scanf("%s", choice);
		if (strcmp(choice, "get") == 0) {
			choiceValue = GET;  
			break;
		}
		else if (strcmp(choice, "put") == 0) {
			choiceValue = PUT;   
			break;
		}
		else if (strcmp(choice, "ls") == 0) {
			choiceValue = LIST; 
			break;
		}
		else if(strcmp(choice, "exit") == 0) {
			choiceValue = CLOSE;  
			break;
		}
		else {
			printf("Wrong Choice %s \n", choice);
			client_menu();
		}
	}
	return choiceValue;
}

/***********************
find file size
************************/
static unsigned int find_size (FILE * fileDescriptor) {
    unsigned int size;
    fseek(fileDescriptor, 0L, SEEK_END);
    size = ftell(fileDescriptor);
    fseek(fileDescriptor, 0L, SEEK_SET);
    return size;
}


/***********************************
Read the dfc.conf
-Extracts DFS server information
-Extracts Username and Password
************************************/ 

char available_servers[4][100], USERNAME[100], PASSWORD[100];

void read_Dfc_config(int check) {
    FILE *fp;
    char webbuffer[200];
    char *val1;
    int i = 0;

    fp=fopen(Config_File,"r");

    if (fp == NULL){
        perror("Config_File");
        exit(1);
    }
    else {
        unsigned int wsConfig_FileSize = find_size (fp);
        while(fgets(webbuffer,wsConfig_FileSize,fp)!=NULL) {
            if (check){
	            if((strncmp(webbuffer,"Server",6)==0)  || (strncmp(webbuffer,"SERVER",6)==0) ) {
	                val1=strtok(webbuffer," \t\n");
	                val1 = strtok(NULL, " \t\n");
	                if (val1[3] == '1'){
	                	val1 = strtok(NULL, " \t\n");
	                	strcpy(available_servers[DFS_SERVER1],val1);
	                	i = 0;
	                }
	                if (val1[3] == '2'){
	                	val1 = strtok(NULL, " \t\n");
	                	strcpy(available_servers[DFS_SERVER2],val1);
	                	i = 1;
	                }
	                if (val1[3] == '3'){
	                	val1 = strtok(NULL, " \t\n");
	                	strcpy(available_servers[DFS_SERVER3],val1);
	                	i =2;
	                }
	                if (val1[3] == '4'){
	                	val1 = strtok(NULL, " \t\n");
	                	strcpy(available_servers[DFS_SERVER4],val1);
	                	i =3;
	                }
	                printf("%s\n",available_servers[i]);
	                bzero(webbuffer, sizeof(webbuffer));
	                i = i%4;
	            }
        	}
        	else
        	{
	            if(strncmp(webbuffer,"Username",8)==0) {
	                printf("webbuffer: %s",webbuffer);
	                val1=strtok(webbuffer," \t\n");
	                val1 = strtok(NULL, " \t\n");
	                strcpy(USERNAME, val1);
	            	bzero(webbuffer, sizeof(webbuffer));
	            }
	            if(strncmp(webbuffer,"Password",8)==0) {
	                printf("webbuffer: %s",webbuffer);
	                val1=strtok(webbuffer," \t\n");
	                val1 = strtok(NULL, " \t\n");
	                strcpy(PASSWORD, val1);
	            	bzero(webbuffer, sizeof(webbuffer));
            	}
            }

        }
        fclose(fp);
    }
}

/*****************************************************
 *Transfers the file from Client to Server
 *****************************************************/
int send_file(int sockfd, char *filename, struct sockaddr_in servAddr, char *subfolder){
   FILE *filepart;                          
   char send_buffer[1024], read_buffer[256];  
   size_t read_size, stat;   
   int size;
   struct timeval timeout = {2,0};
   fd_set fds;   
   int buffer_fd, buffer_out, flags;

   socklen_t servlen = sizeof(servAddr);
   while(1) {
	if (!(filepart = fopen(filename, "r"))) {
	    perror("fopen");	    	
            printf("Re enter the file name\n");
	    scanf("%s", filename);
	}
	else {
  	    break;
	}
   }
 
   fseek(filepart, 0, SEEK_END);
   size = ftell(filepart);
   fseek(filepart, 0, SEEK_SET);
   printf("Total file size is: %d\n",size);
   printf("Sending file SIZE from Client to Server....\n\r");
   stat = send(sockfd, &size, sizeof(int), 0);\
   if (stat < 0) {
	perror("Error sending size");
   	exit(1);
   }
   // send file name
   printf("Sending file NAME from Client to Server....\n\r");
   stat = send(sockfd, filename, 100,0);
   if (stat < 0) {
	perror("Error sending filename");
   	exit(1);
   }
   //send subfolder
   printf("Sending subfolder name from Client to Server....\n\r");
   stat = send(sockfd, subfolder, 100,0);
   if (stat < 0) {
	perror("Error sending subfolder");
   	exit(1);
   }
   printf("Starting the File Transimission...\n\r");
   while(!feof(filepart)) {
      read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, filepart);  // reading a buffer of size 1024     
      do{  
	  stat = send(sockfd, send_buffer, read_size, 0);
	  if (stat < 0)  {
	    	perror("Error in sending image");
	  }
      }while (stat < 0);  

      printf(" \n");

      // zeroing out the send_buffer
      bzero(send_buffer, sizeof(send_buffer));
     }
}


/*****************************************************************
 * Receives the file from serverc
 ******************************************************************/
int receive_file(int sockfd, char *filename, struct sockaddr_in servAddr, socklen_t clientlen, char *subfolder)
{ 
  int buffersize = 0, recv_size = 0, read_size = 0, write_size,stat;
  int size = 1;
  char *filepartarray;
  filepartarray = malloc(300241);
  FILE *image;
  stat = 0;
  char filename_received[100];
  printf("Entering the receive_file function...\n");
  bzero(filename_received,sizeof(filename_received));	
  printf("sending subfolder name as %s\n", subfolder);
  stat = send(sockfd, subfolder, 100, 0);
  if (stat < 0) {
      perror("subfolder sending failed");
  }
  stat = recv(sockfd, filename_received, sizeof(filename_received), 0);
  if (stat < 0) {
    perror("Error receiving filename");
  }
  int filestatus;
  printf("getting the status of file at the subfolder \n\r");
  stat = recv(sockfd, &filestatus, sizeof(int), 0);  // reading the status of file at the subfolder
  if (stat < 0) {
    perror("Error receiving filestatus");
  }
  if (filestatus == 0) {
  	printf("*****************************\n");
  	printf("FILE DOES NOT EXIST ON THE SERVER\n");
  	printf("*****************************\n");
  }
  if (filestatus == 1 || filestatus == -1)
  {
	stat = recv(sockfd, &size, sizeof(int), 0);  // reading size of the image
	if (stat < 0) {
	    perror("Error receiving Size");
	}
	printf("size of the file is %d\n", size);
	strncat(filename_received, "_received",100);
	printf("Filename received is %s\n", filename_received);
	  
	// opeing an image in write mode
	image = fopen(filename_received, "w");
        if( image == NULL) {
	    printf("Error has occurred. Image file could not be opened/ created\n");
	    return -1; 
	  }

	// receiving the file from client
	while(recv_size < size) {
	    read_size = recv(sockfd, filepartarray, 300241, 0);
	    if (read_size == 0) {
	    	perror("Problem with the server");
	    	break;
	    }
	    printf("readsize is %d \n", read_size);
	    printf("size is %d \n", size);
	    write_size = fwrite(filepartarray,1,read_size, image);
	    printf("Written File size: %d\n",write_size); 
	    recv_size += read_size;
	    printf("Total received File size: %i\n",recv_size);
	    printf(" \n");
	    printf(" \n");

	  }
	  fclose(image);
	  return 1;
	}
	else {
		return 0;
	}
}

void opensockets()
{
    int i = 0;
    int portNum;
    char *serverName;
    char *portStr;
    for (i=0;i<MAX_CONNECTIONS;i++)
    {
    	servlen[i] = sizeof(servAddr[i]);
    	serverName = strtok(available_servers[i], ":");
	portStr = strtok(NULL,"");
	printf("server %s\n", serverName);
	printf("portStr %s\n", portStr);
	portNum = atoi(portStr);
	if ((sockfd[i]=socket(AF_INET, SOCK_STREAM, 0))==-1) {
	    perror("Error opening socket");
	}
	server = gethostbyname(serverName);  
	memset((char *)&servAddr[i], 0, sizeof(servAddr[i]));
	servAddr[i].sin_family = AF_INET;
	servAddr[i].sin_port = htons(portNum);
	bcopy((char *)server->h_addr, 
	(char *)&servAddr[i].sin_addr.s_addr,
	server->h_length);     
	
        if (connect(sockfd[i],(struct sockaddr *) &servAddr[i],sizeof(servAddr[i])) < 0)  {
	    	perror("ERROR connecting socket");
	    	//exit(1);
	}
	struct timeval timeout;      
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;
	if (setsockopt (sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
	        error("setsockopt failed\n");
	}
}

/*****************************************************
sends username and password read from config file to server
sockfd1 socket descriptor]
return : ACK from serder for USER credentals validity
 *****************************************************/
int sendUserDetails(int sockfd1)
{
    int n, ack_putfile;
    read_Dfc_config(0);  // only reads username and password
    n = send(sockfd1, USERNAME, 100, 0);
    if (n < 0) {
	perror("choice sending failed");
    }
    n = send(sockfd1, PASSWORD, 100, 0);
    if (n < 0) {
	perror("choice sending failed");
    }
    n = recv(sockfd1, &ack_putfile, sizeof(int), 0);
    if (n < 0) {
      perror("ack receiving failed");
    }  
    if (!ack_putfile) {
    	printf("INAVLID USERNAME/PASSWORD");
    	return 0;
    }
    printf("User details sent and validated\n");
    return 1;
}

/*****************************************************
Computes md5sum of given file name 
 *****************************************************/
void computeMd5sum(char *filename, char md5sum[100])  {
    //char md5sum[100];
    char systemmd5Cmd[100];
    strncpy(systemmd5Cmd, "md5sum ", sizeof("md5sum ")); 
    strncat(systemmd5Cmd, filename, strlen(filename));
    FILE *f = popen(systemmd5Cmd, "r");
    while (fgets(md5sum, 100, f) != NULL) {
	strtok(md5sum,"  \t\n");
    }
    pclose(f);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN); 
    char md5sum[100];
    int md5sumInt;
    int choicesendfailed[MAX_CONNECTIONS];
    int listCheck[MAX_CONNECTIONS] = {0, 0, 0, 0};
    char *subFolder;
    int i;
    int choice = 0;      
    char getFileName[20];
    char putFileName[20];
    int n;
    FILE *filepart;
    int ack_putfile;
    int md5sumIndex;
    int finalIndex;
    char *val1;
    char subfolder[50];
    char fileNameList[10];
    int dummy = 100;
    char fileLs[100];
    char systemLSgetFiles[100];
    char decryptSystemCmd[300];

    if (argc < 2) {
       fprintf(stderr,"configuration file absent\n");
       exit(1);
    }
    sprintf(Config_File,"%s",argv[1]);
    FILE *fp;
    fp=fopen(Config_File,"r");
    if (fp == NULL) {
        perror(Config_File);
        exit(1);
    }
    fclose(fp);
    read_Dfc_config(1);
    opensockets();
    while(1) {
         choice = user_commands(); // selecting the command received
	 printf("choice entered is %d\n", choice);
	 for (i=0;i<MAX_CONNECTIONS;i++) {
	      printf("**choice Sent !!\n");
	      int n = send(sockfd[i], (void *)&choice, sizeof(int), 0);	
	      if (n < 0) {
		  choicesendfailed[i] = TRUE;
		  perror("Writing to socket: choice sending failed");
	      }
   	      else {
		  choicesendfailed[i] = FALSE;
	      }
	}
        switch(choice)	{
	      case GET:
	      // collect information of which servers are active:VERY IMPORTANT
	      // sending the dummy comand to all the servers
	           for (i=0;i<MAX_CONNECTIONS;i++) {
			printf("**Test Server %d !!\n",i);
			int n = send(sockfd[i], (void *)&dummy, sizeof(int), 0);	
			printf(" ....%d\n", n);
			if (n < 0) {
  			   choicesendfailed[i] = TRUE;
  			   perror("Writing to socket: choice sending failed");
			}
			else {
  			   choicesendfailed[i] = FALSE;
			}
		  }
	  	  printf("Enter the file you wish to get\n");
		  scanf("%s", getFileName);
		  printf("Enter the folder containg the file\n");
		  scanf("%s", subfolder);
		  int serverToUse[MAX_CONNECTIONS ] = {-1,-1,-1,-1};
	          if (!choicesendfailed[0]) { //if server 0 is alive
		      serverToUse[0] = 1;
		      if (!choicesendfailed[2]) { //if server 2 is alive
			  serverToUse[2] = 1;
			  printf("COMPLETE 1\n");
		       }
		       else { //if server 2 is dead
	   		     if (!choicesendfailed[1] && !choicesendfailed[3])	{
			    	serverToUse[1] = 1;	
			    	serverToUse[3] = 1;	
			      }
			      else {
			   	printf("INCOMPLETE 1\n"); 
			      }
			    }
			}
		   else if (!choicesendfailed[1]) { //if server 0 is dead but server 1 is alive
			serverToUse[1] = 1;
			if (!choicesendfailed[3]) { //if server 3 is alive
			   printf("COMPLETE 2\n");
			   serverToUse[3] = 1;
			}
		    	else { //if server 3 is dead
	   		printf("INCOMPLETE 2\n"); 
			}
		   }
		   else { //if server 0 is dead and server 1 is dead too
			printf("INCOMPLETE 3\n");
		   }

		   for (i =0;i<MAX_CONNECTIONS;i++) {
			printf("using server %d\n",serverToUse[i]);
		   }	   
	           // sending packet to notify server to start or not
		   int usingyou;
		   for (i=0;i<MAX_CONNECTIONS;i++) {
		       if (serverToUse[i] == 1) {
			  usingyou = TRUE;
		    	  n = send(sockfd[i], (void *)&usingyou, sizeof(int), 0);
			  if (n < 0) {
	 		     perror("choice sending failed");
		 	  }
			}
			else {
	  		  usingyou = FALSE;
		    	  n = send(sockfd[i], (void *)&usingyou, sizeof(int), 0);
  			  if (n < 0) {
			      perror("choice sending failed");
			   }
			}					
		    }
	            for (i = 0; i< MAX_CONNECTIONS; i++) {
			if (serverToUse[i] == 1) {
			   if (sendUserDetails(sockfd[i])){
			       n = send(sockfd[i], getFileName, 50, 0);
			       if (n < 0) {
				   perror("choice sending failed");
			        }
				receive_file(sockfd[i], getFileName, servAddr[i], servlen[i], subfolder);
				printf("**********************\n");
				n = send(sockfd[i], getFileName, 50, 0);
				if (n < 0) {
		 		    perror("choice sending failed");
				}
				receive_file(sockfd[i], getFileName, servAddr[i], servlen[i], subfolder);
				printf("**********************\n");
			     }
			}
		     }
		     for (i = 0;i<MAX_CONNECTIONS;i++)	{
			   serverToUse[i] = -1;
		     }
    		     bzero(systemLSgetFiles, sizeof(systemLSgetFiles));
		     strncpy(systemLSgetFiles, "ls -a .", strlen("ls -a .")); // ls 
		     strncat(systemLSgetFiles, getFileName, strlen(getFileName)); // ls [filename]
		     strncat(systemLSgetFiles, "*_rec*", strlen("*_rec*"));
		     char fileList[4][100];
		     printf("systemLSgetFiles %s\n", systemLSgetFiles);
		     FILE *f = popen(systemLSgetFiles, "r");
		     int i = 0;
		     while (fgets(fileLs, 100, f) != NULL) {
		           bzero(fileList[i], sizeof(fileList[i]));	
			   strtok(fileLs,"  \t\n");
			   strncpy(fileList[i], fileLs, sizeof(fileLs));
			   printf( "%s %lu\n", fileList[i], strlen(fileList[i]) );
			   i++;
		     }
		     pclose(f);
		     bzero(decryptSystemCmd, sizeof(decryptSystemCmd));
		     read_Dfc_config(0);
		     char catCommand[300];
		     for(i=0;i<MAX_CONNECTIONS;i++) {
		       sprintf(decryptSystemCmd,"openssl enc -d -aes-256-cbc -in %s -out de%s -k %s", fileList[i], fileList[i], PASSWORD);
		       printf("here %s\n", decryptSystemCmd);
		       system(decryptSystemCmd);
		     }
		     sprintf(catCommand,"cat de%s de%s de%s de%s > %s_received", fileList[0],fileList[1],fileList[2],fileList[3], getFileName);
		     printf("%s\n",catCommand);
		     system(catCommand);
		     printf("FILE CONCAT SUCCESSFUL\n");
		     bzero(catCommand, sizeof(catCommand));
		     system("rm .foo10_received .foo11_received .foo12_received .foo13_received");
  		     bzero(decryptSystemCmd,sizeof(decryptSystemCmd));
		     for (i=0;i<MAX_CONNECTIONS;i++) {
			bzero(fileList[i],sizeof(fileList[i]));
		     }
		    printf("Exiting get function\n");
		    break;
	       case PUT:
		    printf("Enter the file name and subfolder..\n");
		    scanf("%s", putFileName);
		    scanf("%s", subfolder);
		    printf("putFileName %s\n", putFileName);
	  	    printf("subfolder %s\n", subfolder);
		    while(1) {
	    	       if (!(filepart = fopen(putFileName, "r"))) {
			   perror("fopen");
			   scanf("%s", putFileName);
		        }
		    	else {
 	    		   break;
			}
		    }
	 	    computeMd5sum(putFileName, md5sum);
		    md5sumInt = md5sum[strlen(md5sum)-1] % 4;
		    md5sumIndex = (4-md5sumInt)%4;
	  	    printf("md5sumIndex %d\n", md5sumIndex);
		    printf("Dividing the file into four equal parts...\n");
		    char systemCommand[150];
		    char filename[100];
		    bzero(filename, sizeof(filename));
		    strncpy(filename, putFileName,strlen(putFileName));
		    sprintf(systemCommand,"split -n 4 -a 1 -d %s en%s",putFileName, putFileName);
		    printf("%s\n", systemCommand);
		    system(systemCommand);
		    printf("Dividing done\n\n");
		    printf("Starting Encryption...\n");
		    printf("\nUSING AES Encryption\n");
		    char encryptSystemCmd[200];
		    bzero(encryptSystemCmd,sizeof(encryptSystemCmd));
		    read_Dfc_config(0);
		    for (i=0;i<MAX_CONNECTIONS;i++) {
			sprintf(encryptSystemCmd,"openssl enc -aes-256-cbc -in en%s%d -out %s%d -k %s", putFileName,i, putFileName, i, 		                PASSWORD);
		    	system(encryptSystemCmd);
		    	printf("%s\n", encryptSystemCmd);
		    }
		    char filenameWithIndex[4][100];
		    char fileIndex[1];
		    for (i = 0; i< MAX_CONNECTIONS; i++) {
			if (!choicesendfailed[i])  {
			   if (sendUserDetails(sockfd[i])) {
				//creating the file name with index
			    	// first file
			    	finalIndex = (i+md5sumIndex)%4;
			    	printf("****************** %d  %d\n", finalIndex, (finalIndex+1)%4);
			    	strncpy(filenameWithIndex[finalIndex], putFileName, sizeof(putFileName));
			    	sprintf(fileIndex,"%d",finalIndex);
			    	printf("fileIndex  :   %s\n", fileIndex);
			    	strncat(filenameWithIndex[finalIndex], fileIndex, 1);
			    	printf("filename %s\n", filenameWithIndex[finalIndex]);
				send_file(sockfd[i], filenameWithIndex[finalIndex], servAddr[i], subfolder);
				sleep(1);
				// seconf file
				strncpy(filenameWithIndex[(finalIndex+1)%4], putFileName, sizeof(putFileName));
			    	sprintf(fileIndex,"%d",(finalIndex+1)%4);
			    	printf("fileIndex  :   %s\n", fileIndex);
			    	strncat(filenameWithIndex[(finalIndex+1)%4], fileIndex, 1);
			   	printf("filename %s\n", filenameWithIndex[(finalIndex+1)%4]);
				send_file(sockfd[i], filenameWithIndex[(finalIndex+1)%4], servAddr[i], subfolder);
				bzero(filenameWithIndex[finalIndex], sizeof(filenameWithIndex[finalIndex]));
				bzero(filenameWithIndex[(finalIndex+1)%4], sizeof(filenameWithIndex[(finalIndex+1)%4]));
				bzero(fileIndex, sizeof(fileIndex));
			}
 		    }
		}
		break;
		case LIST:
		// gets files with information of files in the asked subfolder
		// sort the contents files
		// remove redundant information using uniq system command
		system("rm .list0_received .list1_received .list2_received .list3_received");
		printf("Enter the subfolder\n");
		scanf("%s", subfolder);
		int status =0;
		int checkNoCnnections = 0; 
		// this part should be uncommented
		for (i =0; i<MAX_CONNECTIONS;i++) {
		     sprintf(fileNameList, "list%d", i);
		     //printf("fileNameList %s\n", fileNameList);
		     if (!choicesendfailed[i]) { 
			if (sendUserDetails(sockfd[i])) {			
			    n = send(sockfd[i], fileNameList, 50, 0);
	  		    if (n < 0) {
				perror("choice sending failed");
			    }
		 	    status = receive_file(sockfd[i], fileNameList, servAddr[i], servlen[i], subfolder);
 			    if (status == 0) {
				checkNoCnnections++;
			    }
			}
		     }
		     bzero(fileNameList, sizeof(fileNameList));
		 }
		 if (checkNoCnnections == 3) {
		     printf("ALL SERVERS CLOSED\n");
		 }
		 // creating files if not present       
		 system("touch .list0_received");
		 system("touch .list1_received");
		 system("touch .list2_received");
		 system("touch .list3_received");
		 // concat the files present
		 system("cat .list0_received .list1_received .list2_received .list3_received > temp_list");
		 system("sort temp_list | uniq > final_list");
		      
		 FILE *fp;
		 char webbuffer[200];
		 char *val1;
		 char array[256][256];
		 char array_withoutIndex[256][256];
		 char necessary_files[256][256];
	        // checking the various files and displaying their status
	         int p = 0, q = 0;
	         FILE *fr;
		 fr = fopen("final_list","r");
                 if (fr != NULL) {
                    char line_string[500]; /* Buffer to store the contents of each line */
                    int i = 0;
                    char filename_string[500];
                    char filename_string1[500];
                    int subfile_count = 0;
                    while(fgets(line_string, sizeof(line_string), fr) != NULL) {
                        if(strncmp(line_string,".",strlen(".")) == 0) {
                            if(strlen(line_string)>3) {
                                char *line_ptr; /* Pointer to the start of each line */
                                line_ptr = strstr((char *)line_string,".");
                                line_ptr = line_ptr + strlen(".");
                                if (subfile_count == 0) {
                                    bzero(filename_string,sizeof(filename_string));
                                    memcpy(filename_string,line_ptr,strlen(line_ptr)-2);
                                }
                                bzero(filename_string1,sizeof(filename_string1));
                                memcpy(filename_string1,line_ptr,strlen(line_ptr)-2);
                                if(strcmp(filename_string,filename_string1) == 0) {
                                    subfile_count = subfile_count +1;
                                    if(subfile_count == 4)
                                        printf("%s [COMPLETE]\n",filename_string);
                                }
                                else {
                                    if(subfile_count != 4)
                                        printf("%s [INCOMPLETE]\n",filename_string);
                                    subfile_count = 1;
                                    strcpy(filename_string,filename_string1);
                                }
                            }
                         }
                      }
                      if(subfile_count != 4)
                         printf("%s [incomplete]\n",filename_string);
                  }
                  fclose(fr);
		  break;
	    default:
		  break;
	  }
       }
       close(sockfd);
       return 0;
    }

