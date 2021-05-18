Luke Woodhead
CSCI 4273
PA1

udp_server.c
Contains code to run a udp server that recieves inputs from a client to store, transfer, display and delete small files.
to run the udp_server.c code simply navigate to the directory containing it in terminal and type
make
./server <port number>


udp_client.c 
Contains code to send 5 commands to the above udp server.
get <filename> to retrieve a file from the server if their is one
put <filename> to upload a file to the server
delete <filename> to delete a file from the server
ls to display the files in the server directory
exit to close the connection between the client and server and exit both programs
to run the udp_client.c code simply navigate to the directory containing it in terminal and type
make
./client <ip address> <port number>

