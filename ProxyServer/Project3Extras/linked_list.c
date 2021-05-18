/* 
 * testing linked list
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
void print_node(struct Node* node_to_print);
int timestamp_difference(struct timeval tv, int difference);


//Global variables
struct Node* head;
int stop_thread_creation = 0;

/*
*   add_host takes in the values required to generate a new host node
*   and creates one at the end of the linked list
*/
void add_host(char *host, char *ipaddr, char *file, int blacklist){
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
}


/*
*   search_hosts takes in a string which is either an ipaddr or hostname
*   and searches the linked list for a matching node, it either returns
*   the pointer to the node if it exists or a Null pointer
*/
struct Node *search_hosts(char *host_or_ipaddr){
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
    if(current_time.tv_sec - tv.tv_sec <= difference){
        return 0;
    }else{
        return 1;
    } 
}

int main(){
    head = (struct Node*)malloc(sizeof(struct Node));
    head->next_node = NULL;
    printf("in main\n");
    add_host("www.yahoo.com","0.0.0.0","yahoo",0);
    add_host("www.google.com","0.0.0.1","google",0);
    add_host("www.msm.com","0.0.0.2","msm",0);
    add_host("www.cnn.com","0.0.0.3","cnn",0);
    struct Node *result = search_hosts("www.yahoo.com");
    struct Node *result2 = search_hosts("0.0.0.3");
    print_node(result);
    print_node(result2);
    sleep(1);
    if(timestamp_difference(result->tv,2) == 1){
        printf("the difference was greater");
    }else{
        printf("the difference is smaller");
    }
}