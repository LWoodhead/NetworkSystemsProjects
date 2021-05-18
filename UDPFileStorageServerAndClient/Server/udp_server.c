/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 * Luke Woodhead
 * CSCI 4273
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 5120

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  char store_command[20];//Contains the parsed command
  char store_filename[100];//Contains the parsed filename
  FILE *file;//File pointer
  struct dirent *de;//Directory pointer

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
     sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client containing the command 
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
     (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
        sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
     hostp->h_name, hostaddrp);
    printf("server received %zu/%d bytes: %s\n", strlen(buf), n, buf);
    
    //Use scanf to format input into two strings
    sscanf(buf,"%s%s",store_command,store_filename);

    //get command
    if(strncmp("get",store_command,3) == 0){
      //printf("in command%s\n", store_command);
      //Try and open the file the client asked for
      file = fopen(store_filename,"rb");
      if(file != NULL){
        //If the file exists find out how big it is 
        fseek(file,0,SEEK_END);
        int file_size = ftell(file);

        //Check to make sure the file size is the 2-5kb we can send, if it is too large transmit only part of it
        if(file_size > BUFSIZE){
          file_size = BUFSIZE;
        }

        //Read the file into the buffer
        fseek(file,0,SEEK_SET);
        bzero(buf,BUFSIZE);
        fread(buf,sizeof(char),file_size,file);
        fclose(file);

        //Send the buffer to the client, make sure arg3 in sendto is file_size because strlen doesnt work with jpeg data
        n = sendto(sockfd, buf, file_size, 0, 
           (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in sendto");

      }else{
        printf("File couldn't be opened or doesn't exist\n");
      }
    }

    //put command
    if(strncmp("put",store_command,3) == 0){
      //printf("in command%s\n", store_command);

      //Wait for the client to send over the file 
      bzero(buf, BUFSIZE);
      n = recvfrom(sockfd, buf, BUFSIZE, 0,
       (struct sockaddr *) &clientaddr, &clientlen);
      if (n < 0)
        error("ERROR in recvfrom");

      //Create a file and write the buffer into it
      file = fopen(store_filename,"wb");
      if(file != NULL){
        fwrite(buf,n,sizeof(char),file);
      fclose(file);

      }else{
        printf("Unable to create file\n");
      }
    }

    //ls command
    if(strncmp("ls",store_command,2) == 0){
      //printf("in command%s\n", store_command);

      //create a directory list using opendir, some code copied from https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
      bzero(buf,BUFSIZE);
      DIR *dir;
      dir = opendir(".");
      if(dir != NULL){
        while((de = readdir(dir)) != NULL){
          strcat(buf,de->d_name);
          strcat(buf,"\n");
        }
        
      }
      closedir(dir);
      n = sendto(sockfd, buf,strlen(buf),0,(struct sockaddr *) &clientaddr, clientlen);

    }

    //delete command
    if(strncmp("delete",store_command,6) == 0){
      //printf("in command%s\n", store_command);

      //Attempt to delete the file given by the client
      if(remove(store_filename) == -1){
        printf("File doesn't exist or couldn't be deleted\n");
      }else{
        bzero(buf,BUFSIZE);
        printf("File deleted\n");
      }
    }

    //exit command
    if(strncmp("exit",store_command,4) == 0){
      //printf("in command%s\n", store_command);

      //Send command to client then exit, gracefully
      bzero(buf,BUFSIZE);
      strcpy(buf,"safe");
      n = sendto(sockfd, buf, strlen(buf), 0, 
        (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) 
        error("ERROR in sendto");
      printf("Server exiting\n");
      break;
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