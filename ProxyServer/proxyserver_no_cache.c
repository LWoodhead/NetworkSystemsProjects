/* 
 * proxyserver.c
 * CSCI 4273 proxyserver with threads
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
int client_connect(int port, char* ip_address);
void proxyGet(int connfd);
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
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    proxyGet(connfd); 
    close(connfd);
    return NULL;
}

/*
 * proxyGet -> sends client get request to ip and sends back response
 */
void proxyGet(int connfd) 
{
    //Variables
    struct hostent *host_entry;
    int sockfd; 

    char buf[MAXLINE];
    char request_method[100];
    char request_uri[100];
    char request_version[100];
    char formated_hostname[100]; //hostname
    char requested_path[100]; //path for requested resource
    char temp_buffer[100];
    char hostbuffer[256];
    char string_port[100];
    char message_body[1000]; //Contains whatever else comes after request method, uri and version
    char *IPbuffer;
    char *first_host;

    int num_arg;
    int hostname;
    int last_co_fs_in;
    int proxy_port; //port number, defined in url, after Host: <hostname>:<port> or defaults to 80
    size_t n; 

    //Set variables
    proxy_port = -1;

    //Recieve request
    bzero(buf,BUFSIZ);
    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);

    //Determine if request is a get
    bzero(request_method,100);
    bzero(request_uri,100);
    bzero(request_version,100);
    num_arg = sscanf(buf,"%s%s%s",request_method,request_uri,request_version);
    bzero(message_body,1000);
    strcpy(message_body,&buf[strlen(request_uri)+strlen(request_method)+strlen(request_version)+2]);
    bzero(formated_hostname,100);
    strcpy(formated_hostname,request_uri);
    if(strncmp(request_method,"GET",3) == 0){
        //Check to see if we have a Host: and default port
        first_host = strstr(buf,"Host:");
        if(first_host != NULL){
            //Add a sscanf to get first string past host
            //Break down string into hostname and port if the port number exists
            //Else just use the uri
        }

        //Remove initial /
        if(strncmp(formated_hostname,"/",1) == 0){
            bzero(temp_buffer,100);
            memcpy(temp_buffer,&formated_hostname[1],strlen(formated_hostname));
            bzero(formated_hostname,100);
            memcpy(formated_hostname,temp_buffer,strlen(temp_buffer));
        }
        //Remove http://
        if(strncmp(formated_hostname,"http://",7) == 0){
            bzero(temp_buffer,100);
            memcpy(temp_buffer,&formated_hostname[7],strlen(formated_hostname));
            bzero(formated_hostname,100);
            memcpy(formated_hostname,temp_buffer,strlen(temp_buffer));
        }
        //Remove https://
        if(strncmp(formated_hostname,"https://",8) == 0){
            bzero(temp_buffer,100);
            memcpy(temp_buffer,&formated_hostname[8],strlen(formated_hostname));
            bzero(formated_hostname,100);
            memcpy(formated_hostname,temp_buffer,strlen(temp_buffer));
        }
        //Remove everything after .edu or .com or etc by dumping everything from the last / or :
        last_co_fs_in = 0;
        for(last_co_fs_in; last_co_fs_in < strlen(formated_hostname); last_co_fs_in++){
            //If there is a / break the loop
            if(formated_hostname[last_co_fs_in] == '/'){
                break;
            }

            //If there is a : find the port number following it then break, can't get to work come back later
            if(strncmp(&formated_hostname[last_co_fs_in],":",1) == 0){
                int temp_index = last_co_fs_in;
                for(temp_index; temp_index < strlen(formated_hostname); temp_index++){
                    if(formated_hostname[temp_index] == '/'){
                        break;
                    }
                }
                bzero(string_port,100);
                memcpy(string_port,&formated_hostname[last_co_fs_in+1],temp_index-last_co_fs_in-1);
                printf("string_port contains the port: %s\n",string_port);
                proxy_port = atoi(string_port);
                break;
            }
        }
        if(last_co_fs_in != strlen(formated_hostname)){
            bzero(temp_buffer,100);
            memcpy(temp_buffer,formated_hostname,last_co_fs_in);
            bzero(formated_hostname,100);
            memcpy(formated_hostname,temp_buffer,strlen(temp_buffer));
            formated_hostname[last_co_fs_in] = '\0';
        }

        //Determine if the hostname is valid
        host_entry = gethostbyname(formated_hostname);
        if(host_entry != NULL){
            IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
            //printf("%s\n",formated_hostname);
            //printf("Host IP: %s\n", IPbuffer);
            // bzero(buf,MAXBUF);
            // strcpy(buf,"HTTP/1.1 500 Internal Server Error");
            // write(connfd, buf,strlen(buf));
            // We now have a real hostname so we construct a tcp connection to a server
            if(proxy_port == -1){
                proxy_port = 80;
            }
            //connect to server
            sockfd = client_connect(proxy_port,IPbuffer);
            //send header to client 
            //bzero(buf,MAXBUF);
            //snprintf(buf,MAXBUF,"%s %s %s\r\n",request_method,request_uri,request_version);
            //printf("buffer contains \n\n %s",buf);
            write(sockfd,buf,strlen(buf));
            //read from server and send back to client in a loop
            n = MAXBUF;
            while(n != 0){
                bzero(buf,MAXBUF);
                n = read(sockfd,buf,MAXBUF);
                printf("server response contains \n\n %s",buf);
                write(connfd,buf,n);
            }
            //close socket when done
            close(sockfd); 
        }else{
            //Return an error
            printf("no host found for hostname: %s\n",formated_hostname);
            bzero(buf,MAXBUF);
            strcpy(buf,"HTTP/1.1 500 Internal Server Error");
            write(connfd, buf,strlen(buf));
        }
    }else{
        //Return an error
        bzero(buf,MAXBUF);
        strcpy(buf,"HTTP/1.1 500 Internal Server Error");
        write(connfd, buf,strlen(buf));
    }
    
    //Execute the get request to the the host name and send the result to client

    
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

/*
* client_connect takes in the port and ipaddr and connects the proxy
* with the requested client, the returns the socket id
*/
int client_connect(int port, char* ip_address){
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    //create socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);

    bzero(&servaddr, sizeof(servaddr));

    //assign given ip and port
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(ip_address); 
    servaddr.sin_port = htons(port); 

    //connect client and server sockets
    if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) != 0 ){
        printf("Failed to connect proxy to server with ip: %s and port %d\n",ip_address,port);
    }
    else{
        printf("Connected proxy to server with ip: %s and port %d\n",ip_address,port);
        return sockfd;
    }
}


/*Resources
http://www.cplusplus.com/reference/cstdio/snprintf/
https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
*/