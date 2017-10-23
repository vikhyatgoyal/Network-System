# Network-System-Assignment 2
This is an implementation of a simple web server to handle GET, POST request.
The wenserer handles error messages as below:
____________________________________________________________________________
# S.No.:	Functionality 		Explanation 
____________________________________________________________________________

 1		400			invalid file request
 2		401			wrong http version 
 3		500			internal server errors
 4		501			request not implemented
____________________________________________________________________________

# Folder Structure: 
|Simple_Webserver_Assignment_2
||--> Readme.md
||--> main.c
||--> WWW
||--> w.conf
||--> a.out
||-->||-->index.html
||-->||-->jquery-1.4.3.min.js
||-->||-->css
||-->||-->fancybox
||-->||-->files
||-->||-->graphics
||-->||-->images


# How to Run:

Server: 
1) On the terminal execute the a.out file:
-> ./a.out
CLient:
1) Go to the web browser.
2) send request to load teh deafult page "localhost::<portnumber>"
Note: <portnumber" is the listen number in w.conf file


# Functionality : GET
Description: The client sends a GET request to retrive content from the server.
The file requested is extracted from the client request and then appened into the ROOT path. 
If the file is found, its send byte by byte to the client as a response. if not, a invalid response is sent.

# Functionality : POST
Description: The client can use POST request to send a POSTDATA to the server which then sends back the deafult page as response.
The POST request is implemented similar to the GET request but the response header is made to be inline with the format below:
<html><body><pre><h1>POSTDATA</h1></pre> followed by the default index page. Ideally the POSTDATA shoudl be the file client intends to store on the server.

