Luke Woodhead
CSCI 4273

This directory contains my code in webserver.c to run the index.html file as a client on a browser, although I have only tested it with chrome and firefox. webserver.c is a multi-threaded approach that uses pthreads c library to generate a thread with a socket connection to the browser which while retrieve the file the client requests and send it back with the correct http header.

To execute this code use the included makefile,

make
./webserver <portnumber>

will run the webserver, then connect using a browser either with localhost:<port number>/index.html or with <server name>:<port number>/index/html. /index.html is optional and the space after <portnumber> can simply be left blank.

