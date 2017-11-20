#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>	//inet_addr
#include <dirent.h>

int main(int argc, char const *argv[])
{
	char buffer[1024];
	memset(buffer, 0, 1024);
	char *name = NULL;
	name = malloc(1024);
	strcpy(name, ".text1.txt.3\n");
	printf("Name is:%s\n", name);
	
	char *tk1 = malloc(strlen(name) + 1);
	char *tk2 = malloc(strlen(name) + 1);
	// strncpy(buffer, name+1, strlen(name) - 4);
 //    printf("New name is:%s\n", buffer);
	tk1 = strtok(name, ".");
	printf("tk1:%s\n",tk1);

	tk2 = strtok(NULL, ".");
	printf("tk2:%s\n",tk2);

	tk2 = strtok(NULL, "\n");
	printf("tk3:%s\n",tk2);
	return 0;
}