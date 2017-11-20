#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>

#define BUFLEN 10240
#define GET     1
#define PUT     2 
#define LIST    3
#define CLOSE   4
#define VALID   1
#define INVALID 0

#define FIRSTFILE 0
#define SECONDFILE 1

typedef struct user_credential{
  char USERNAME[100];
  char PASSWORD[100];
}user_credentials;

user_credentials putfileUser;

char dsConfigFile[100];
/*****************************************************
 * int str2int(char *str) Configures String to Integer
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
 *Transfers the file from Server to Client
 *****************************************************/
int send_file(int sockfd, char *filename, struct sockaddr_in clientAddr, int indexrcdport, int fileNumber) {
  FILE *filepart;                   
  char subfolder[100];
  size_t  read_size, stat; 
  int packet_index, size;
  struct timeval timeout = {2,0};
  char send_buffer[1024], read_buffer[256];
  char lscommand[200];
  bzero(lscommand, sizeof(lscommand));
  packet_index = 1;
  fd_set fds;
  int buffer_fd, buffer_out, flags;
  socklen_t servlen = sizeof(clientAddr); 
  char dfsFolder[100];
  bzero(dfsFolder, sizeof(dfsFolder));
  DIR           *d;
  struct dirent *dir;
  int count = 0;
  int index = 0;
  char name[256][256];
  char directorytoopen[100];
  char filename_dummy[100];
  // receive the subfolder name
  stat = recv(sockfd, subfolder, 100, 0);  // reading the subfolder name 
  if (stat < 0) {
    perror("Error receiving subfolder");
  }
  printf("subfolder%s\n", subfolder);
  if (fileNumber == -1) {
    if (strcmp(subfolder,"/") == 0) {
      sprintf(lscommand, "ls -a DFS%d/%s > DFS%d/%s/.%s", indexrcdport, putfileUser.USERNAME, indexrcdport, putfileUser.USERNAME, filename);
      printf("lscommand %s\n", lscommand);
      system(lscommand);
      printf("\n\nEmpty subfolder received\n\n");
    }
    else {
      sprintf(lscommand, "ls -a DFS%d/%s/%s > DFS%d/%s/%s/.%s", indexrcdport, putfileUser.USERNAME, subfolder,indexrcdport,              
      putfileUser.USERNAME, subfolder,filename);
      printf("lscommand %s\n", lscommand);
      system(lscommand);
    }
  }
  switch(indexrcdport){
    case 1:
      strncpy(dfsFolder, "DFS1", sizeof("DFS1"));
      strncpy(directorytoopen, "./DFS1/", sizeof("./DFS1/"));
      strncat(directorytoopen, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
    break;
    case 2:
      strncpy(dfsFolder, "DFS2", sizeof("DFS2"));
      strncpy(directorytoopen, "./DFS2/", sizeof("./DFS2/"));
      strncat(directorytoopen, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
    break;
    case 3:
      strncpy(dfsFolder, "DFS3", sizeof("DFS3"));
      strncpy(directorytoopen, "./DFS3/", sizeof("./DFS3/"));
      strncat(directorytoopen, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
    break;
    case 4:
      printf("I am coming here\n");
      strncpy(dfsFolder, "DFS4", sizeof("DFS4"));
      strncpy(directorytoopen, "./DFS4/", sizeof("./DFS4/"));
      strncat(directorytoopen, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
    break;    
  }
  if (strcmp(subfolder,"/") == 0) {
    printf("\n\nEmpty subfolder received\n\n");
  }
  else {
    sprintf(directorytoopen, "%s/%s", directorytoopen, subfolder);
  }
  printf("directorytoopen %s\n", directorytoopen);
  bzero(filename_dummy, sizeof(filename_dummy));
  sprintf(filename_dummy,".%s",filename);
  int filestatus = -5;
  if (fileNumber != -1) {
    d = opendir(directorytoopen);
    if (d) {
      while ((dir = readdir(d)) != NULL) { 
        if (strncmp(dir->d_name, filename_dummy, strlen(filename)) == 0) {
          printf(" dir->d_name[strlen(filename) %d\n",  dir->d_name[strlen(filename) + 1]);
          if ((dir->d_name[strlen(filename) + 2]>=0) &&  (dir->d_name[strlen(filename) + 2]<=3)) {
            strcpy(name[count],dir->d_name);
            printf("name[count] %s\n", name[count]);
            count++;
          }
        }
      }
     closedir(d);
    }
    else {
      printf("FILE NOT FOUND\n");
    }   
    if (count == 0) {
    filestatus = 0;
    }
  }
  strncat(dfsFolder,"/",sizeof("/")); 
  strncat(dfsFolder,putfileUser.USERNAME,sizeof(putfileUser.USERNAME)); 
  strncat(dfsFolder,"/",sizeof("/")); 
  printf("dfsFolder %s\n", dfsFolder);
  char filenameWithDot[100];
  bzero(filenameWithDot,sizeof(filenameWithDot));
  if (fileNumber != -1)
    strcat(filenameWithDot, name[fileNumber]);
  else
  {
    sprintf(filenameWithDot, ".%s", filename);
    if (strcmp(subfolder,"/") == 0) {
    }
    else
      sprintf(dfsFolder,"%s/%s",dfsFolder, subfolder);
  }
  printf("filename %s\n", filename);
  printf("filenameWithDot%s\n", filenameWithDot);
  send(sockfd, filenameWithDot, sizeof(filenameWithDot), 0);
  strncat(dfsFolder,filenameWithDot, sizeof(filenameWithDot));
  printf("dfs folder latest%s\n", dfsFolder);
  char isfilethere[200];
  bzero(isfilethere, sizeof(isfilethere));
  printf("fileNumber %d\n", fileNumber);
  if (filestatus == 0) {
    printf("FILE UNAVAILABLE\n");
  }
  else if (fileNumber>=-1) {
      if (strcmp(subfolder,"/") == 0) {
        sprintf(isfilethere,"DFS%d/%s/%s",indexrcdport,putfileUser.USERNAME,filenameWithDot);
      }
      else {
       sprintf(isfilethere,"DFS%d/%s/%s/%s",indexrcdport,putfileUser.USERNAME,subfolder,filenameWithDot); 
      }
      printf("isfilethere: %s\n", isfilethere);
      if (access (isfilethere, F_OK) != -1) {
          printf("FILE EXISTS\n");
          filestatus = 1;
          send(sockfd, (void *)&filestatus, sizeof(int), 0);
          if (stat < 0)  {
          perror("Error sending size");
          exit(1);
          }
       }
       else {
        filestatus = 0;
        printf("FILE NOT EXISTS\n");
        send(sockfd, (void *)&filestatus, sizeof(int), 0);
        if (stat < 0) {
          perror("Error sending size");
          exit(1);
         }
       }
  }
  else  {
    filestatus = -1;
    send(sockfd, (void *)&filestatus, sizeof(int), 0);
    if (stat < 0)   {
        perror("Error sending size");
        exit(1);
    }
  }
  if (filestatus == 1 || filestatus == -1) {
      if (!(filepart = fopen(dfsFolder, "r")))  {
        perror("fopen");
      }    
      bzero(dfsFolder, sizeof(dfsFolder));
      printf("Finding the size of the file using fseek\n");  
      fseek(filepart, 0, SEEK_END);
      size = ftell(filepart);
      fseek(filepart, 0, SEEK_SET);
      printf("Total file size is: %d\n",size);
      printf("Sending filepart Size from Server to Client\n");
      send(sockfd, (void *)&size, sizeof(int), 0);
       if (stat < 0) {
        perror("Error sending size");
        exit(1);
       }

     char sendBuf_withpcktSize[BUFLEN];
     while(!feof(filepart)) {
       read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, filepart);
       do{
         stat = send(sockfd, send_buffer, read_size, 0);
       }while (stat < 0);
      printf(" \n");
      printf(" \n");
      bzero(send_buffer, sizeof(send_buffer));
      }
   }
}

/*****************************************************************
 *Receives the file from client to server
 ******************************************************************/

int receive_image(int sockfd, struct sockaddr_in clientAddr, socklen_t clientlen, int indexrcdport)
{ 
  int buffersize = 0, recv_size = 0, read_size, write_size,stat;
  int size = 0;
  char *imagearray;
  imagearray = malloc(300241);
  FILE *image;
  char filename[100];
  char subfolder[100];
  printf("Entering the receive_image function\n");
  read_size = 0;
  size = 1;
  stat = 0;
  stat = recv(sockfd, &size, sizeof(int), 0);  
  if (stat < 0) {
    perror("Error receiving Size");
  }
  printf("size of the file is %d\n", size);
  stat = recv(sockfd, filename, 100, 0); 
  if (stat < 0) {
    perror("Error receiving filename");
  }
  stat = recv(sockfd, subfolder, 100, 0);  
  if (stat < 0) {
    perror("Error receiving subfolder");
  }
  char filename_received[100];
  bzero(filename_received,sizeof(filename_received));
  strncat(filename_received, filename, 100);
  strncat(filename_received, "_received",100);
  printf("Filename received is %s\n", filename_received);
  char dfsFolder[100];
  bzero(dfsFolder, sizeof(dfsFolder));
  char FolderWithUsername[150];
  char FolderWithUsernameSubfolder[200];
  switch(indexrcdport) {
    case 1:
      strncpy(dfsFolder, "DFS1", sizeof("DFS1"));
      system("mkdir -p DFS1");
      strncpy(FolderWithUsername, "mkdir -p DFS1/", strlen("mkdir -p DFS1/"));
    break;
    case 2:
      strncpy(dfsFolder, "DFS2", sizeof("DFS2"));
      system("mkdir -p DFS2");
      strncpy(FolderWithUsername, "mkdir -p DFS2/", strlen("mkdir -p DFS2/"));
    break;
    case 3:
      strncpy(dfsFolder, "DFS3", sizeof("DFS3"));
      system("mkdir -p DFS3");
      strncpy(FolderWithUsername, "mkdir -p DFS3/", strlen("mkdir -p DFS3/"));
    break;
    case 4:
      //printf("I am coming here\n");
      strncpy(dfsFolder, "DFS4", sizeof("DFS4"));
      system("mkdir -p DFS4");
      strncpy(FolderWithUsername, "mkdir -p DFS4/", strlen("mkdir -p DFS4/"));
    break;    
  }
  // creates DFS[]/Username
  strncat(FolderWithUsername, putfileUser.USERNAME, strlen(putfileUser.USERNAME));
  printf("FolderWithUsername %s\n", FolderWithUsername);
  system(FolderWithUsername);

  // creates DFS[]/Username/subfolder
  printf("subfolder %s\n", subfolder);
  if (strcmp(subfolder, "/") == 0) {
    printf("\n\nEmpty subfolder\n\n");
  } 
  else {
    sprintf(FolderWithUsernameSubfolder,"mkdir -p %s/%s",FolderWithUsername, subfolder);
    printf("FolderWithUsername and subfolder%s\n", FolderWithUsernameSubfolder);
    system(FolderWithUsernameSubfolder);
  }
  strncat(dfsFolder,"/",sizeof("/"));
  strncat(dfsFolder,putfileUser.USERNAME, strlen(putfileUser.USERNAME));
  char filenameWithDot[100];
  bzero(filenameWithDot,sizeof(filenameWithDot));
  strncpy(filenameWithDot,".",sizeof("."));
  strcat(filenameWithDot, filename);
  if (strcmp(subfolder, "/") == 0) {
    printf("\n\nEmpty subfolder\n\n");
  } 
  else {
    sprintf(dfsFolder,"%s/%s",dfsFolder,subfolder);
    printf("dfsFolder with sub folder%s\n", dfsFolder);
  }
  strncat(dfsFolder,"/",sizeof("/"));
  strncat(dfsFolder,filenameWithDot, sizeof(filenameWithDot));
  printf("********************dfs folder: %s\n", dfsFolder);
  image = fopen(dfsFolder, "w");
  if( image == NULL) {
    printf("Error has occurred. Image file could not be opened/ created\n");
    return -1; 
  }
  while(recv_size < size)  {
    read_size = recv(sockfd, imagearray, 300241, 0);
    printf("readsize is %d \n", read_size);
    write_size = fwrite(imagearray,1,read_size, image);
    printf("Written File size: %d\n",write_size); 
    recv_size += read_size;
    printf("Total received File size: %i\n",recv_size);
    printf(" \n");
    printf(" \n");
  }
  fclose(image);
  bzero(FolderWithUsername, sizeof(FolderWithUsername));
  return 1;
}

/***********************
Gets file size
************************/
static unsigned int GET_size (FILE * fileDescriptor) {
    unsigned int size;
    fseek(fileDescriptor, 0L, SEEK_END);
    size = ftell(fileDescriptor);
    fseek(fileDescriptor, 0L, SEEK_SET);
    return size;
}

int checkValidFrmDFSconf(char *USERNAME1, char *PASSWORD1) {
    FILE *fp;
    char wsBuf[200];
    char *val1;
    int i = 0;
    fp=fopen(dsConfigFile,"r");
    if (fp == NULL) {
        perror(dsConfigFile);
        exit(1);
    }
    else {
        unsigned int wsConfigFileSize = GET_size (fp);
        printf("dfs.conf size n = %d, filename = dfs.conf \n", wsConfigFileSize);
        while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {
           printf("wsBuf: %s",wsBuf);
           val1=strtok(wsBuf," \t\n");
           if ((strncmp(val1, USERNAME1, strlen(USERNAME1)) == 0)  &&  (strlen(val1)==strlen(USERNAME1))) {
              val1 = strtok(NULL, " \t\n");   
              if ((strncmp(val1, PASSWORD1, strlen(PASSWORD1)) == 0) &&  (strlen(val1)==strlen(PASSWORD1))) {
                  return VALID; // valid 
              }
           }
        }
        fclose(fp);
    }
    return INVALID; // invalid
}

int sendAckforUserdetails(int newsockfd)
{
  int n, ack_putfile;   
  n = recv(newsockfd, putfileUser.USERNAME, 100, 0);    // Reading the choice from user side
  if (n < 0) {
    perror("choice receiving failed");
    exit(1);
  }
  n = recv(newsockfd, putfileUser.PASSWORD, 100, 0);    // Reading the choice from user side
  if (n < 0) {
    perror("choice receiving failed");
    exit(1);
  }  
  ack_putfile = checkValidFrmDFSconf(putfileUser.USERNAME, putfileUser.PASSWORD);
  n = send(newsockfd, &ack_putfile, sizeof(int), 0);  // reading size of the image
  if (n < 0) {
    perror("Error receiving ack for username and password");
  }
  printf("***************8 validity %d\n", ack_putfile);
  return ack_putfile;
}

int main(int argc, char **argv)
{
  signal(SIGPIPE, SIG_IGN);
  int sockfd, newsockfd, portno;
  socklen_t clientlen;
  char buffer[256];
  struct sockaddr_in servAddr, clientAddr;
  int listDummy;
  int n;
  char fileNameList[10];
  if (argc < 3) {
     fprintf(stderr,"ERROR, no port provided or config file missing\n");
     exit(1);
  }
  sprintf(dsConfigFile,"%s",argv[2]);
  FILE *fp;
  fp=fopen(dsConfigFile,"r");
  if (fp == NULL)  {
     perror(dsConfigFile);
     exit(1);
  }
  fclose(fp);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &servAddr, sizeof(servAddr));
  portno = atoi(argv[1]);
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &servAddr,sizeof(servAddr)) < 0) 
     error("ERROR on binding");
  printf("Server Started\n");
  listen(sockfd,5);
  clientlen = sizeof(clientAddr);
  newsockfd = accept(sockfd, 
             (struct sockaddr *) &clientAddr, 
             &clientlen);
  if (newsockfd < 0) 
      error("ERROR on accept");
  int choice;
  char putFileName[50];
  char getFileName[50];
  int stat;
  int n1;
  FILE *checkFile;
  int ack_putfile;
  int indexrcdport = portno%5;
  printf("indexrcdport %d\n", indexrcdport);
  int iCanStart;
  int dummy;
  while(1) {
   n = recv(newsockfd, (void *)&choice, sizeof(int), 0);    // Reading the choice from user side
   if (n < 0) {
      perror("choice receiving failed");
   }          
   switch(choice)
   {
    case GET:
         n = recv(newsockfd, (void *)&dummy, sizeof(int), 0);
         if (n < 0) {
            perror("choice receiving failed");
         }            
         n = recv(newsockfd, (void *)&iCanStart, sizeof(int), 0);
         if (n < 0)  {
            perror("choice receiving failed");
         } 
         if (iCanStart) {
            printf("receiving the file name\n");
            if (sendAckforUserdetails(newsockfd))  {
               stat = recv(newsockfd, getFileName, 50,0);
               if (stat < 0) {
                   perror("Error sending filename");
                   exit(1);
               }
               printf("The file name asked for first time is %s\n", getFileName);
               send_file(newsockfd, getFileName, clientAddr, indexrcdport, FIRSTFILE);  
               printf("Exiting the GET\n");
               sleep(1);
               stat = recv(newsockfd, getFileName, 50,0);
               if (stat < 0) {
                 perror("Error sending filename");
                 exit(1);
               }
               printf("The file name asked for second time is %s\n", getFileName);
               // sending the image
               send_file(newsockfd, getFileName, clientAddr, indexrcdport, SECONDFILE);  
               printf("Exiting the GET\n");
             }
        }
        choice = 10;
        break;
      case PUT:
        if (sendAckforUserdetails(newsockfd))  { 
           receive_image( newsockfd, clientAddr, clientlen,indexrcdport); 
           printf("completed putfile case\n");
           // receiving second image
           receive_image( newsockfd, clientAddr, clientlen,indexrcdport); 
           printf("completed putfile case\n");
        }
        choice = 10;
        break;
      case LIST:
        if (sendAckforUserdetails(newsockfd)) {
          stat = recv(newsockfd, fileNameList, 50,0);
          if (stat < 0) {
            perror("Error sending filename");
            exit(1);
          }
          printf("The file name asked for first time is %s\n", fileNameList);
          send_file(newsockfd, fileNameList, clientAddr, indexrcdport, -1); 
          sleep(1);
        }
        choice = 10;
      break;
      case CLOSE:
        printf("Closing the socket and exiting\n");
        close(sockfd);   // closing the socket and exiting
        exit(1);
        choice = 10;
      break;
      default:
      break;
    }
  }
}

