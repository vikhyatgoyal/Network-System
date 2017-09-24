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


