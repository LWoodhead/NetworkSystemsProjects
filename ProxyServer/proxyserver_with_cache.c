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
#include <sys/time.h>    /* for timestamps */
#include <semaphore.h> 
#include <signal.h>  

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

struct Node{
    struct Node *next_node;
    char hostname[100]; 
    char ip_address[100];
    char filename[100];
    struct timeval tv;
    int blacklisted;
};

int open_listenfd(int port);
int client_connect(int port, char* ip_address);
void proxyGet(int connfd);
void *thread(void *vargp);
void add_host(char *host, char *ipaddr, char *file, int blacklist);
struct Node *search_hosts(char *host_or_ipaddr);
struct Node *search_host_files(char *filename);
void print_node(struct Node* node_to_print);
int timestamp_difference(struct timeval tv, int difference);
void free_host_list();
void create_blacklist(char *filename);

//Global variables
struct Node* head;
int timeout;

//Semaphores
sem_t host_list_access;
sem_t file_write;

int main(int argc, char **argv) 
{
    //Assign variables
    head = (struct Node*)malloc(sizeof(struct Node));
    head->next_node = NULL;
    sem_init(&host_list_access,0,1);
    sem_init(&file_write,0,1);
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 4) {
	fprintf(stderr, "usage: %s <port> %s <timeout value> <blacklist filename>\n", argv[0],argv[1]);
	exit(0);
    }
    port = atoi(argv[1]);
    timeout = atoi(argv[2]);
    create_blacklist(argv[3]);

    listenfd = open_listenfd(port);
    while (1) {
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
    sem_destroy(&host_list_access);
    sem_destroy(&file_write);
    free_host_list();
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
    struct Node* current_host;
    int sockfd;
    FILE *file_ptr; 

    char buf[MAXLINE];
    char request_method[100];
    char request_uri[100];
    char request_version[100];
    char formated_hostname[100]; //hostname
    char requested_path[100]; //path for requested file
    char formated_filename[100];
    char temp_buffer[100];
    char hostbuffer[256];
    char string_port[100];
    char client_request[1000];
    char message_body[1000]; //Contains whatever else comes after request method, uri and version
    char *IPbuffer;
    char *first_host;

    int num_arg;
    int file_transfered;
    int hostname;
    int file_length;
    int last_co_fs_in;
    int proxy_port; //port number, defined in url, after Host: <hostname>:<port> or defaults to 80
    size_t n; 

    //Set variables
    proxy_port = -1;
    file_transfered = 0; 

    //Recieve request
    bzero(buf,BUFSIZ);
    n = read(connfd, buf, MAXLINE);
    //Save the request
    bzero(client_request,1000);
    memcpy(client_request,buf,strlen(buf));

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
        //printf("server received the following request:\n%s\n",buf);
        first_host = strstr(buf,"Host:");
        if(first_host != NULL){
            //Add a sscanf to get first string past host
            //Break down string into hostname and port if the port numberhttps://stackoverflow.com/questions/3501338/c-read-file-line-by-line
        }
        //Check to see if http version is undefined and send back a 500 internal server error
        if(strncmp(request_version,"undefined",9) == 0){
            bzero(buf,MAXBUF);
            strcpy(buf,"HTTP/1.1 500 Internal Server Error");
            write(connfd, buf,strlen(buf));
            printf("Undefined http version\n");
            return;
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
        if(proxy_port == -1){
            proxy_port = 80;
        }
        //format file name
        bzero(formated_filename,100);
        memcpy(formated_filename,request_uri,strlen(request_uri));
        for(int i=0;i<strlen(formated_filename);i++){
            if(formated_filename[i] == '.' || formated_filename[i] == ':'|| formated_filename[i] == '/'){
                formated_filename[i] = '_';
            }
        }
        //printf("filename is: %s\n",formated_filename);
        //Check to see if the hostname is in the linkedlist
        current_host = search_hosts(formated_hostname);
        //Check to make sure host is not null
        if(current_host != NULL){
            //Check to make sure the current host is not blacklisted
            // printf("Contents of found hostname node\n");
            // print_node(current_host);
            if(current_host->blacklisted == 0){
                //Find the node with the correct file
                current_host = search_host_files(formated_filename);
                //Check if the node exists
                if(current_host != NULL){
                    // printf("Contents of found filename node\n");
                    // print_node(current_host);
                    //Check if the file is within the timeout
                    if(timestamp_difference(current_host->tv,timeout) == 0){
                        //Read the file and write it to the client
                        file_ptr = fopen(current_host->filename,"rb");
                        if(file_ptr != NULL){
                            //Find the length of the file
                            fseek(file_ptr,0L,SEEK_END);
                            file_length = ftell(file_ptr);
                            //Seek back to the beginning
                            fseek(file_ptr,0,SEEK_SET);
                            //Write from the file until less than maxbuf remains
                            int bytes_left = file_length;
                            while(bytes_left > MAXBUF){
                                bzero(buf,MAXBUF);
                                fread(buf,sizeof(char),MAXBUF,file_ptr);
                                write(connfd,buf,MAXBUF);
                                // if(bytes_left == file_length){
                                //     printf("%s",buf);
                                // }
                                bytes_left = bytes_left - MAXBUF;
                            }
                            //Write the rest
                            bzero(buf,MAXBUF);
                            fread(buf,sizeof(char),bytes_left,file_ptr);
                            write(connfd,buf,bytes_left);
                            printf("cached copy sent\n");
                            // print_node(current_host);
                            //close the file
                            fclose(file_ptr);
                        }else{
                            //If the file_ptr is null send a 500 internal server error, log to console
                            printf("Could not open cached copy\n");
                            bzero(buf,MAXBUF);
                            snprintf(buf,MAXBUF,"%s 500 Internal Server Error",request_version);
                            write(connfd, buf,strlen(buf));
                        }
                    }else{
                        //Connect to the server and get a new copy to cache if the file is timedout
                        //Open file
                        file_ptr = fopen(current_host->filename,"wb");
                        if(file_ptr != NULL){
                            //If we can open a file
                            //Connect to server
                            sockfd = client_connect(proxy_port,current_host->ip_address);
                            //Send request to server
                            write(sockfd,client_request,strlen(client_request));
                            //Read response, write to file, send to client
                            n = MAXBUF;
                            while(n != 0){
                                bzero(buf,MAXBUF);
                                n = read(sockfd,buf,MAXBUF);
                                fwrite(buf,n,sizeof(char),file_ptr);
                                write(connfd,buf,n);
                            }
                            printf("Created new cached copy with same node and sent to client\n");
                            //close sockstream and file
                            close(sockfd);
                            fclose(file_ptr);
                            //Update the timesmap
                            sem_wait(&host_list_access);
                            //Update timestamp
                            gettimeofday(&current_host->tv,NULL);
                            sem_post(&host_list_access);
                        }else{
                            //If the file_ptr is null send a 500 internal server error, log to console
                            printf("Could not open a file to write new cache copy into\n");
                            bzero(buf,MAXBUF);
                            snprintf(buf,MAXBUF,"%s 500 Internal Server Error",request_version);
                            write(connfd, buf,strlen(buf));
                        }
                    }
                }else{
                    //If the file doesn't exist, connect to the server, create and send new cached copy, create a new node
                    current_host = search_hosts(formated_hostname);
                    //Open file
                    file_ptr = fopen(formated_filename,"wb");
                    if(file_ptr != NULL){
                        //If we can open a file
                        //Connect to server
                        sockfd = client_connect(proxy_port,current_host->ip_address);
                        //Send request to server
                        write(sockfd,client_request,strlen(client_request));
                        //Read response, write to file, send to client
                        n = MAXBUF;
                        while(n != 0){
                            bzero(buf,MAXBUF);
                            n = read(sockfd,buf,MAXBUF);
                            fwrite(buf,n,sizeof(char),file_ptr);
                            write(connfd,buf,n);
                        }
                        printf("Created new cached copy with new node and sent to client\n");
                        //close sockstream and file
                        close(sockfd);
                        fclose(file_ptr);
                        //Create a new node
                        add_host(formated_hostname,current_host->ip_address,formated_filename,0);
                    }else{
                        //If the file_ptr is null send a 500 internal server error, log to console
                        printf("Could not create a new file to cache a copy into\n");
                        bzero(buf,MAXBUF);
                        snprintf(buf,MAXBUF,"%s 500 Internal Server Error",request_version);
                        write(connfd, buf,strlen(buf));
                    }
                }
            }else{
                //return a 403 forbidden error
                printf("Hostname exists and is blacklisted\n");
                bzero(buf,MAXBUF);
                snprintf(buf,MAXBUF,"%s 403 Forbidden",request_version);
                write(connfd, buf,strlen(buf));
            }
        }else{
            //If null the hostname doesn't exist in cache and we must use gethostname and then create a node
            host_entry = gethostbyname(formated_hostname);
            if(host_entry != NULL){
                //The name resolves to an ip address
                IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
                current_host = search_hosts(IPbuffer);
                if(current_host != NULL && current_host->blacklisted == 1){
                    //We could have a blacklisted host added only by ip, send 403 forbiden
                    printf("IpAddress exists and is blacklisted\n");
                    bzero(buf,MAXBUF);
                    snprintf(buf,MAXBUF,"%s 403 Forbidden",request_version);
                    write(connfd, buf,strlen(buf));
                }else{
                    //Connect to server, read response, create file, create new node
                    file_ptr = fopen(formated_filename,"wb");
                    if(file_ptr != NULL){
                        //If we can open a file
                        //Connect to server
                        sockfd = client_connect(proxy_port,IPbuffer);
                        //Send request to server
                        write(sockfd,client_request,strlen(client_request));
                        //Read response, write to file, send to client
                        n = MAXBUF;
                        while(n != 0){
                            bzero(buf,MAXBUF);
                            n = read(sockfd,buf,MAXBUF);
                            fwrite(buf,n,sizeof(char),file_ptr);
                            write(connfd,buf,n);
                        }
                        printf("First cached copy created and sent to client\n");
                        //close sockstream and file
                        close(sockfd);
                        fclose(file_ptr);
                        //Create a new node
                        add_host(formated_hostname,IPbuffer,formated_filename,0);
                    }else{
                        //If the file_ptr is null send a 500 internal server error, log to console
                        printf("Could not create an original new file to cache a copy into\n");
                        bzero(buf,MAXBUF);
                        snprintf(buf,MAXBUF,"%s 500 Internal Server Error",request_version);
                        write(connfd, buf,strlen(buf));
                    }
                }           
            }else{
                //The hostname doesn't exist send a 404 error
                printf("Hostname does not match an ip address\n");
                bzero(buf,MAXBUF);
                snprintf(buf,MAXBUF,"%s 404 Not Found",request_version);
                write(connfd, buf,strlen(buf));
            }
        }
    }else{
        //Return an error, command was not GET
        bzero(buf,MAXBUF);
        strcpy(buf,"HTTP/1.1 500 Internal Server Error");
        write(connfd, buf,strlen(buf));
    }
    return;
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
        //printf("Failed to connect proxy to server with ip: %s and port %d\n",ip_address,port);
    }
    else{
        //printf("Connected proxy to server with ip: %s and port %d\n",ip_address,port);
        return sockfd;
    }
}

/*
*   add_host takes in the values required to generate a new host node
*   and creates one at the end of the linked list
*/
void add_host(char *host, char *ipaddr, char *file, int blacklist){
    sem_wait(&host_list_access);
    struct Node* current_node = head;
    struct Node* new_node = NULL;
    //Find the end of the linked list
    while(current_node->next_node != NULL){
        current_node = current_node->next_node;
    }
    //Create a new node and fill it
    new_node = (struct Node*)malloc(sizeof(struct Node));
    memcpy(new_node->hostname,host,strlen(host));
    memcpy(new_node->ip_address,ipaddr,strlen(ipaddr));
    memcpy(new_node->filename,file,strlen(file));
    gettimeofday(&new_node->tv,NULL);
    new_node->blacklisted = blacklist;
    new_node->next_node = NULL;
    //Point the previous last node at the new last node
    current_node->next_node = new_node;
    sem_post(&host_list_access);
}


/*
*   search_hosts takes in a string which is either an ipaddr or hostname
*   and searches the linked list for a matching node, it either returns
*   the pointer to the node if it exists or a Null pointer
*/
struct Node *search_hosts(char *host_or_ipaddr){
    sem_wait(&host_list_access);
    struct Node* current_node = head;
    struct Node* result_node = NULL;
    //Search the list for a node with a matching hostname or ip address
    while(current_node != NULL){
        if(strcmp(current_node->hostname,host_or_ipaddr) == 0){
            result_node = current_node;
            break;
        }
        if(strcmp(current_node->ip_address,host_or_ipaddr) == 0){
            result_node = current_node;
            break;
        }
        current_node = current_node->next_node;
    }
    sem_post(&host_list_access);
    return result_node;
}
/*
*   Prints the contents of a given node
*/

void print_node(struct Node* node_to_print){
    if(node_to_print == NULL){
        printf("Cannot print NULL pointer\n");
    }else{
        printf("Node Contains\nhostname: %s\nipaddr: %s\nfilename: %s\nblacklist: %d\n",node_to_print->hostname,node_to_print->ip_address,node_to_print->filename,node_to_print->blacklisted);
    }
}

/*
*   Given a timestamp and an int representing an acceptable difference in seconds
*   return 0 if the difference between the current time and the timestamp is less
*   than or equal to the given difference and 1 if it is greater
*/
int timestamp_difference(struct timeval tv, int difference){
    struct timeval current_time;
    gettimeofday(&current_time,NULL);
    // printf("difference is %d\n",(int)(current_time.tv_sec - tv.tv_sec));
    if(current_time.tv_sec - tv.tv_sec <= difference){
        // printf("Difference is smaller\n");
        return 0;
    }else{
        // printf("Difference is greater\n");
        return 1;
    } 
}
void free_host_list(){
    struct Node * current_node = head;
    struct Node * next = head->next_node;
    while(current_node != NULL){
        free(current_node);
        if(next == NULL){
            break;
        }else{
            current_node = next;
            next = next->next_node;
        }
    }
}

/*
*   Searchs through the host's to find a matching filename and returns it
*
*/
struct Node *search_host_files(char *filename){
    sem_wait(&host_list_access);
    struct Node* current_node = head;
    struct Node* result_node = NULL;
    //Search the list for a node with a matching filename
    while(current_node != NULL){
        if(strcmp(current_node->filename,filename) == 0){
            result_node = current_node;
            break;
        }
        current_node = current_node->next_node;
    }
    sem_post(&host_list_access);
    return result_node;
}

/*
*   Given a filename add blacklist nodes
*/
void create_blacklist(char *filename){
    FILE* file;
    char buffer[100];
    char hostname[100];
    bzero(hostname,100);
    bzero(buffer,100);
    file = fopen(filename, "r");

    while(fgets(buffer, 100, file)) {
        memcpy(hostname,buffer,strlen(buffer)-1);
        //printf("hostname: %s with size: %zu\n",hostname,strlen(hostname));
        add_host(hostname,hostname,filename,1);
        bzero(buffer,100);
        bzero(hostname,100);
    }
    fclose(file);
}

/*Resources
http://www.cplusplus.com/reference/cstdio/snprintf/
https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
https://stackoverflow.com/questions/3501338/c-read-file-line-by-line
https://www.geeksforgeeks.org/write-a-c-program-that-doesnt-terminate-when-ctrlc-is-pressed/
*/