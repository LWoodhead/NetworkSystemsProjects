/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 * Luke Woodhead
 * CSCI 4273
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 5120

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    int number_of_arguments;//Holds the number of arguments parsed by sscanf
    char store_command[20];//Contains the parsed command
    char store_filename[100];//Contains the parsed filename
    FILE *file;//File pointer

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    printf("Avaliable Commands Are\nget <filename>\nput <filename>\ndelete <filename>\nls\nexit\n");

    while(1){
        /* get a message from the user */
        bzero(buf, BUFSIZE);
        printf("Please enter msg: ");
        fgets(buf, BUFSIZE, stdin);

        //Use scanf to format input into two strings
        number_of_arguments = sscanf(buf,"%s%s",store_command,store_filename);


        //get command
        if(strncmp(store_command,"get",3) == 0){
            //printf("in command get\n");


            //check to see we have something to send the server
            if(number_of_arguments == 2){
                //execute get command

                //send get command to server and wait for it to send us back the data we need
                serverlen = sizeof(serveraddr);
                n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
                if (n < 0) 
                    error("ERROR in sendto");

                bzero(buf,BUFSIZE);
                n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
                if (n < 0) 
                    error("ERROR in recvfrom");

                //Write the data into a file
                file = fopen(store_filename,"wb");
                if(file != NULL){
                    fwrite(buf,n,sizeof(char),file);
                fclose(file);
                }else{
                    printf("Error in creating file\n");
                }

            }else{
                printf("Please enter a filename after your command\n");
            } 
        }

        //put command
        else if(strncmp(store_command,"put",3) == 0){
            //printf("in command put\n");

            //check to see we have something to send the server
            if(number_of_arguments == 2){

                //execute put command

                //open file and check it exists
                file = fopen(store_filename,"rb");//Open with b flag to get binary data
                if(file != NULL){

                    /*transmit the command to the server*/
                    serverlen = sizeof(serveraddr);
                    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
                    if (n < 0) 
                      error("ERROR in sendto");

                    //Determine file size for fwrite
                    fseek(file,0,SEEK_END);
                    int file_size = ftell(file);

                    //Check to make sure the file size is the 2-5kb we can send, if it is too large transmit only part of it
                    if(file_size > BUFSIZE){
                        file_size = BUFSIZE;
                    }

                    //Seek back to the beginning of the file then write it into the buffer
                    fseek(file,0,SEEK_SET);
                    bzero(buf,BUFSIZE);
                    fread(buf,sizeof(char),file_size,file);
                    fclose(file);
                    //printf("%s\n",buf);

                    //Send the file to the server and make sure arg3 in sendto is file_size because strlen doesnt work with jpeg data
                    serverlen = sizeof(serveraddr);
                    n = sendto(sockfd, buf, file_size, 0, (struct sockaddr *)&serveraddr, serverlen);
                    if (n < 0) 
                      error("ERROR in sendto");
                    
                    // /* print the server's reply */
                    // n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, &serverlen);
                    // if (n < 0) 
                    //   error("ERROR in recvfrom");
                    // printf("Echo from server: %s", buf);


                }else{
                    printf("File either can't be opened or doesn't exist\n");
                }
                //turn the file into a buffer so it can be sent by the datagram

                //print if the file was succesfflly sent

            }else{
                printf("Please enter a filename after your command\n");
            }    
        }

        //delete command
        else if(strncmp(store_command,"delete",6) == 0){
            //printf("in command delete\n");

            //check to see we have something to send the server
            if(number_of_arguments == 2){

                //Send command to the server
                serverlen = sizeof(serveraddr);
                n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
                if (n < 0) 
                    error("ERROR in sendto");
            }else{
                printf("Please enter a filename after your command\n");
            } 
        }

        //exit command
        else if(strncmp(store_command,"exit",4) == 0){
            // printf("in command exit\n");
            // printf("exit exiting\n");

            //Send exit command to the server, wait for the response and then break
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");

            n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, &serverlen);
            if (n < 0) 
                error("ERROR in recvfrom");

            printf("Echo from server: %s", buf);
            break;
        }

        //ls command
        else if(strncmp(store_command,"ls",2) == 0){

            //send buffer to server and server sends back a list of files which we print
            serverlen = sizeof(serveraddr);
            n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
            if (n < 0) 
                error("ERROR in sendto");

            bzero(buf,BUFSIZE);
            n = recvfrom(sockfd, buf, 300, 0, (struct sockaddr *)&serveraddr, &serverlen);
            if (n < 0) 
                error("ERROR in recvfrom");
            //printf("transmited %d bytes\n", n);
            printf("Directory is%s",buf);
        }

        //unrecognized command
        else{
            printf("%s is an unrecognized command\n", buf);
        }
    }
    close(sockfd);
}

/*
THINGS I BORROWED FROM
https://stackoverflow.com/questions/60917491/how-to-test-that-an-input-string-in-c-is-of-the-correct-format
https://www.tutorialspoint.com/c_standard_library/c_function_sscanf.htm
https://www.geeksforgeeks.org/c-program-find-size-file/
http://developerweb.net/viewtopic.php?id=3916
https://www.tutorialspoint.com/c_standard_library/c_function_remove.html
https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
https://pubs.opengroup.org/onlinepubs/009695399/functions/opendir.html
https://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.htm
*/