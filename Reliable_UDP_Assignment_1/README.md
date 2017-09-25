# Network-System-Assignment 1
# This is an implementation of UDP transfer layer with reliability using handshaking and encyrption.
# There are five functionality supported in the server which client can request:
____________________________________________________________________________
# S.No.:	Functionality 		Explanation 
____________________________________________________________________________

# 1		Get			Transfer a file form server to client
# 2		Put			Transfer a file to server from client
# 3		ls			list all the file the server 
# 4		delete			delete a file on the server
# 5		exit			close the server
____________________________________________________________________________

# Folder Structure: 
|Reliable_UDP_Assignment_1
||--> Readme.md
||--> Client
|||	--> client.c
|||	--> makefile
|||	--> foo1
|||	--> foo2
|||	--> foo3
||--> Server 
|||	--> server.c
|||	--> makefile

# How to Run:

Server: 
1) Go to the server folder.
2) Run "make" on command line.
3) Run ./server <port_number>

Client:
1) Go to the client folder.
2) Run "make" on command line.
3) Run ./client <server_ip_address> <port_number>

Note: 
1) Server IP Address can be found by runnig "ifconfig" on the server
2) same port number should be used between the server and client

# Functionality : get <file_name>
Description: The client can request for any file on the server using "get" functionality. 
Usage: type on client "get <file_name>" and hit enter.
HANDSHAKING AND RELAIBILITY:
1) Client sends "Get_e" signal to teh server.
2) Server responds to clients with "ready_get".
3) Clint send the file name if wants to copy.
4) Server responds with the size of the file.
5) Client responds with "Client :: Ready to Receive" signal
6) Server starts sending encrypted data chunks of the file to the client.
7) On each recive of the packet, the client sends back the acknoledgemnet signal. This signal contains the packet index recived by teh client.
8) If the server does not recive a acknolegdement within a 500 msec timeout, It sends the packet again.
9) If the client recives any packet index which it is not expecting, it drops it and sends the acknoledgment as the expected index number.

# Functionality : put <file_name>
Description: The client can transfer any file to the server using "put" functionality. 
Usage: type on client "put <file_name>" and hit enter.
HANDSHAKING AND RELAIBILITY:
1) Client sends "Put_e" signal to the server.
2) Server responds to clients with "ready_put".
3) Clint send the file name it wants to transfer.
4) Server responds with "SERVER :: Ready to Receive" signal.
4) Client sends the size of the file.
5) Server responds with "SERVER :: Got Size" signal
6) Client starts sending encrypted data chunks of the file to the client.
7) On each recive of the packet, the server sends back the acknoledgemnet signal. This signal contains the packet index recived by the server.
8) If the server does not recive a acknolegdement within a 500 msec timeout, It sends the acknolegement as the index of the last packet recived.
9) If the client recives any acknoledgemnet as packet index which it is not expecting, it sends the last send packet again.

# Functionality : delete <file_name>
Description: The client can detele any file on the server using "delete" functionality. 
Usage: type on client "delete <file_name>" and hit enter.
HANDSHAKING AND RELAIBILITY:
1) Client sends "Delete_e" signal to the server.
2) Server responds to clients with "ready_delete".
3) Clint send the file name it wants to delete.
4) Server deletes the file.

# Functionality : ls
Description: The client can list the files present at the server using "ls" functionality. 
Usage: type on client "ls" and hit enter.
HANDSHAKING AND RELAIBILITY:
1) Client sends "Ls_e" signal to the server.
2) Server responds to clients with "ready_ls".
3) Server pipes the system shell command "ls" and creates a file with the list of files in its directory.
4) Server then sends the file list file using the "get" functionality.
5) Server deletes the list file from it's directory.


# Functionality : exit
Description: The client can close the server gracefully by using teh "exit" functionality. 
Usage: type on client "exit" and hit enter.
HANDSHAKING AND RELAIBILITY:
1) Client sends "Exit_e" signal to the server.
2) Server responds to clients with "ready_exit".
3) Server closes its socket and exits the execution.
