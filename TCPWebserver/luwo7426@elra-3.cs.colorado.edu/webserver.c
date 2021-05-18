/* 
 * webserver.c
 * CSCI 4273 webserver with threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    // printf("thread created\n");
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    echo(connfd); //-> change to handle client requests 
    // printf("after echo\n");
    close(connfd);
    // printf("thread ended\n");
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE];
    char request_method[100];
    char request_uri[100];
    char request_version[100];
    char file_path[100]; 
    char file_buf[MAXLINE];
    int last_period_index;
    char file_type[4];
    char content_type[50];
    char header_buf[200];
    size_t file_length;
    FILE *file_ptr;
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 xd xd Course!</h1>"; 

    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);

    //Read filepath, protocol version, content type,
    bzero(request_method,100);
    bzero(request_uri,100);
    bzero(request_version,100);
    int num_arg = sscanf(buf,"%s%s%s",request_method,request_uri,request_version);
    printf("method:%s\n uri:%s\n version:%s\n",request_method,request_uri,request_version);
    printf("sizeof uri:%zu\n",strlen(request_uri));

    //Attempt to open the referenced file, find its length and write it into a buffer

    //Check to see if the client needs the root directory 
    if((strncmp(request_uri,"/",1) == 0 && strlen(request_uri) == 1) || (strncmp(request_uri,"index.html",10) == 0 && strlen(request_uri) == 10)){
        printf("matched the index.html\n");
        bzero(file_path,100);
        strcpy(file_path,"index.html");
        bzero(content_type,50);
        strcpy(content_type,"text/html");
    }else{
        // \printf("did not match the index.html\n");
        //Parse uri to get the correct file_path
        bzero(file_path,100);
        memcpy(file_path,&request_uri[1],strlen(request_uri));
        // printf("the file path is %s with a length of %zu\n",file_path,strlen(file_path));
        //Find the correct content type
        last_period_index = strlen(request_uri)-1;
        while(request_uri[last_period_index] != '.'){
            last_period_index = last_period_index-1;
        }
        bzero(file_type,4);
        memcpy(file_type,&file_path[last_period_index],strlen(file_path) - last_period_index);
        // printf("the file type is %s\n",file_type);
        bzero(content_type,50);
        if(strncmp(file_type,"html",4) == 0){
            strcpy(content_type,"text/html");
        }else if(strncmp(file_type,"txt",3) == 0){
            strcpy(content_type,"text/plain");
        }else if(strncmp(file_type,"png",3) == 0){
            strcpy(content_type,"image/png");
        }else if(strncmp(file_type,"gif",3) == 0){
            strcpy(content_type,"image/gif");
        }else if(strncmp(file_type,"jpg",3) == 0){
            strcpy(content_type,"image/jpg");
        }else if(strncmp(file_type,"css",3) == 0){
            strcpy(content_type,"text/css");
        }else if(strncmp(file_type,"js",2) == 0){
            strcpy(content_type,"application/javascript");
        }else{
            strcpy(content_type,"text/plain");
        }
        printf("%s\n",content_type);
    }
    // printf("before open\n");
    file_ptr = fopen(file_path,"rb");
    // printf("after open\n");
    if(file_ptr == NULL){
        printf("filepath not found\n");
        bzero(buf,MAXBUF);
        strcpy(buf,"HTTP/1.1 500 Internal Server Error");
        write(connfd, buf,strlen(buf));
    }else{
        //Find the length of the file
        fseek(file_ptr,0L,SEEK_END);
        file_length = ftell(file_ptr);
        // printf("%zu is the filelength\n",file_length);
        //Seek back to the beginning and write the file into its buffer
        fseek(file_ptr,0,SEEK_SET);
        //Find the size of the http header
        bzero(header_buf,200);
        snprintf(header_buf,200,"%s 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",request_version,content_type,file_length);
        int header_size = strlen(header_buf);
        if(file_length + header_size <= MAXBUF){
            //Just write everything into the buf and send
            bzero(file_buf,MAXBUF);
            fread(file_buf,sizeof(char),file_length,file_ptr);
            bzero(buf,MAXBUF);
            snprintf(buf,MAXBUF,"%s 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",request_version,content_type,file_length);
            write(connfd, buf,header_size);
            write(connfd, file_buf,file_length);
            printf("file sent in 1 buffer with size %zu",file_length);
        }else{
            //Otherwise first read the first MAXBUF-header size bytes into buf and send
            //bzero(file_buf,MAXBUF);
            //fread(file_buf,sizeof(char),MAXBUF-header_size,file_ptr);
            bzero(buf,MAXBUF);
            snprintf(buf,MAXBUF,"%s 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",request_version,content_type,file_length);
            write(connfd, buf,header_size);
            //Then read the rest of the file into the buf in a loop sending chunks of it
            int bytes_left = file_length;
            while(bytes_left > MAXBUF){
                bzero(file_buf,MAXBUF);
                fread(file_buf,sizeof(char),MAXBUF,file_ptr);
                bzero(buf,MAXBUF);
                memcpy(buf,file_buf,MAXBUF);
                write(connfd, buf,MAXBUF);
                bytes_left = bytes_left - MAXBUF;
            }
            //Send the last part of the file
            bzero(file_buf,MAXBUF);
            fread(file_buf,sizeof(char),bytes_left,file_ptr);
            bzero(buf,MAXBUF);
            memcpy(buf,file_buf,bytes_left);
            write(connfd, buf,bytes_left);
            printf("this is the number of bytes sent in the last message %d\n",bytes_left);
        }
        fclose(file_ptr);


    }

    //Combine everything into the send buffer with concat and send
    // bzero(buf,MAXBUF);
    // snprintf(buf,MAXBUF,"%s 200 Document Follows\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n%s",request_version,content_type,file_length,file_buf);
    // printf("buf contains\n%s",buf);
    // printf("%zu %s\n",strlen(request_version),request_version);
    // printf("%zu is the filelength\n",file_length);
    // bzero(buf,MAXBUF);
    // strcpy(buf,httpmsg);
    printf("server returning a http message with the following content.\n%s\n",buf);
    // write(connfd, buf,strlen(buf)); 4
    
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */


/*Resources
http://www.cplusplus.com/reference/cstdio/snprintf/
*/