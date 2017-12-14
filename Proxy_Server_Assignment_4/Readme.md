To run proxy server ./webproxy <port_num> <timeout>

./webproxy 10002 100

Testing Instructions Clear the Mozilla cache and local proxy server cache

For Testing with http browser (Mozilla Firefox) 
1) http://morse.colorado.edu
2) http://www.caida.org/home/ 

The webproxy parses the request from client. 
Checks if its a valid supported request (GET), a valida HTTP versionnd if teh host is blocked.
If any of teh above condition is not voilated, the code proceeds to check if the link has already been catched, 
if present in cache, the file is loaded from the catch folder else, sends the requested information back to client and parallely stores it on cache memory

 
For caching and searching file , MD5 hashing is been used

current time + file modified time is less than timeout- file is fetched from cache

else File fetched from Proxy Server (and replaces the old file if present)

The links present in the file from server are extracted and passed onto server

server responds appropriately with the request and is stored in the cache

The URL is always checked for a port number and if not found default hit on port 80 both from browser and telnet
