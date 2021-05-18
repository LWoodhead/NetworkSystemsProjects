/* 
 * DFS_client.c, client for simple distributed file system 
 * usage: DFS_client <host> <port>
 * Luke Woodhead
 * CSCI 4273
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
#include <openssl/md5.h>

#define MAXBUF 8192
#define MAXSERVERS 4
#define MAXFILES 1000

//Helper functions
int client_connect(int port, char* ip_address);
int get_md5_mod(unsigned char* hash_value, int hash_length);

//Structs
struct server_info{
    char name[100];
    char ip[100];
    int port;
};
struct file_info{
    char name[100];
    int file_parts[4];
};


int main(int argc, char **argv){
    //Declares Variables
    FILE *file_ptr;
    char buf[MAXBUF];
    char temp_buf[MAXBUF];
    char config_buffer[100];
    char serverName[100];
    char serverIP[100];
    char serverPort[100];
    char username[100];
    char password[100];
    char line_buf[100];
    char *file_bufs[4];
    int part_size[4];
    char *filename;
    struct server_info avaliable_servers[MAXSERVERS];
    struct file_info ret_files[MAXFILES];
    char store_command[100];
    char store_filename[100];
    char subdirectory_command[100];
    int num_args;
    int sockfd;
    int num_files;
    int file_length;
    int md5sum;
    size_t n;

    /* check command line arguments */
    if (argc != 2) {
       fprintf(stderr,"usage: %s <dfc config>\n", argv[0]);
       exit(0);
    }

    //Set arguments
    filename = argv[1];
    bzero(avaliable_servers,sizeof(struct server_info)*MAXSERVERS);
    bzero(line_buf,100);

    //Parse config info
    file_ptr = fopen(filename,"r");
    if(file_ptr != NULL){
        bzero(config_buffer,100);
        for(int i=0;i < 5;i++){
            fgets(config_buffer, 100, file_ptr);
            if(i < 4){
                bzero(serverName,100);
                bzero(serverPort,100);
                bzero(serverIP,100);
                sscanf(config_buffer,"%s%s%s",serverName,serverIP,serverPort);
                //printf("Name is: %s IP is: %s Port is: %s\n",serverName,serverIP,serverPort);
                memcpy(avaliable_servers[i].ip,serverIP,strlen(serverIP));
                memcpy(avaliable_servers[i].name,serverName,strlen(serverName));
                avaliable_servers[i].port = atoi(serverPort);
                bzero(config_buffer,100); 
            }else{
                bzero(username,100);
                bzero(password,100);
                sscanf(config_buffer,"%s%s",username,password);
                //printf("Username: %s Password: %s\n",username,password);
            }
        }
        fclose(file_ptr);
    }else{
        printf("Failed to open config file\n");
        exit(0);
    }

    //Loop and wait for user input
    while(1){
        //get a message from the user
        bzero(buf, MAXBUF);
        printf("Please enter msg: ");
        fgets(buf, MAXBUF, stdin);

        //Convert input with sscanf
        bzero(store_command,100);
        bzero(store_filename,100);
        bzero(subdirectory_command,100);
        num_args = sscanf(buf,"%s%s%s",store_command,store_filename,subdirectory_command);

        //Check to see if command is get, put or list
        if(strncmp(store_command,"list",4) == 0){
            //Set once per loop variables
            bzero(ret_files,sizeof(struct file_info)*MAXFILES);
            //Print ret_files contents
            num_files = 0;
            //loop through servers and execute command
            for(int i=0;i<MAXSERVERS;i++){
                //Connect to server
                sockfd = client_connect(avaliable_servers[i].port,avaliable_servers[i].ip);
                //If it returns -1 the server is unavaliable
                if(sockfd == -1){
                    printf("Server: %s at IP: %s on Port: %d is offline\n",avaliable_servers[i].name,avaliable_servers[i].ip,avaliable_servers[i].port);
                }else{
                    //Send username and password in form "username password"
                    bzero(buf,MAXBUF);
                    snprintf(buf,MAXBUF,"%s %s",username,password);
                    n = write(sockfd,buf,100);
                    //Read response
                    bzero(buf,MAXBUF);
                    n = read(sockfd,buf,100);
                    if(strncmp(buf,"Invalid Username/Password. Please try again.",strlen(buf)) == 0){
                        //Print buffer as error message, invalid username or password
                        printf("%s\n",buf);
                        return 0;
                    }else{
                        //Execute commands
                        //Check if we have a sub directory added
                        bzero(buf,MAXBUF);
                        if(num_args == 1){
                            snprintf(buf,MAXBUF,"%s",store_command);
                        }else{
                            snprintf(buf,MAXBUF,"%s %s",store_command,store_filename);
                        }
                        //Send list command
                        n = write(sockfd,buf,100);
                        //Recieve and parse response
                        bzero(buf,MAXBUF);
                        bzero(temp_buf,MAXBUF);
                        n = read(sockfd,buf,MAXBUF);
                        //Get rid of leading space
                        if(strlen(buf) > 0){
                            memcpy(temp_buf,buf,strlen(buf));
                            bzero(buf,MAXBUF);
                            memcpy(buf,&temp_buf[1],strlen(temp_buf)-1);
                        }
                        // printf("Buf Contains: %s\n",buf);
                        char * pch;
                        pch = strtok (buf," ");
                        while (pch != NULL)
                        {
                            //Add part to file struct if it is name is already inside
                            int file_found = 0;
                            if(pch != NULL){
                                if(pch[0] == '.'){
                                    for(int j=0;j<num_files;j++){
                                        if((strncmp(ret_files[j].name,&pch[1],strlen(pch)-3) == 0) && (strlen(ret_files[j].name) == strlen(pch)-3)){
                                            file_found = 1;
                                            if(pch[strlen(pch)-1] == '1'){
                                                ret_files[j].file_parts[0] = 1;
                                            }else if(pch[strlen(pch)-1] == '2'){
                                                ret_files[j].file_parts[1] = 1;
                                            }else if(pch[strlen(pch)-1] == '3'){
                                                ret_files[j].file_parts[2] = 1;
                                            }else if(pch[strlen(pch)-1] == '4'){
                                                ret_files[j].file_parts[3] = 1;
                                            }
                                        }
                                    }
                                    //If no file was found add one
                                    if(file_found == 0){
                                        memcpy(ret_files[num_files].name,&pch[1],strlen(pch)-3);
                                        if(pch[strlen(pch)-1] == '1'){
                                            ret_files[num_files].file_parts[0] = 1;
                                        }else if(pch[strlen(pch)-1] == '2'){
                                            ret_files[num_files].file_parts[1] = 1;
                                        }else if(pch[strlen(pch)-1] == '3'){
                                            ret_files[num_files].file_parts[2] = 1;
                                        }else if(pch[strlen(pch)-1] == '4'){
                                            ret_files[num_files].file_parts[3] = 1;
                                        }
                                        num_files++;
                                    }
                                }
                            }
                            pch = strtok (NULL, " ");
                        }
                    }
                    //close socket
                    close(sockfd);
                }
            }
            //Print ret_files contents
            // for(int k=0;k<num_files;k++){
            //     printf("Name: %s Index: %d\n",ret_files[k].name,k);
            //     for(int j=0;j<4;j++){
            //         printf("at index: %d values: %d\n",j,ret_files[k].file_parts[j]);
            //     }
            // }

            //Print the file names if they exist
            if(num_files == 0){
                printf("No Files Founds\n");
            }else{
                printf("Files Found Are\n");
                for(int j=0;j<num_files;j++){
                    if((ret_files[j].file_parts[0] == 1) && (ret_files[j].file_parts[1] == 1) && (ret_files[j].file_parts[2] == 1) && (ret_files[j].file_parts[3] == 1)){
                        printf("%d. %s\n",j+1,ret_files[j].name);
                    }else{
                        printf("%d. %s [incomplete]\n",j+1,ret_files[j].name);
                    }
                }
            }
        }else if(strncmp(store_command,"put",3) == 0){
            //check to see if a file name was specified
            if(num_args == 2 || num_args == 3){
                file_ptr = fopen(store_filename,"rb");
                if(file_ptr == NULL){
                    printf("Unable to open input file to send\n");
                }else{
                    //Find file length
                    fseek(file_ptr,0,SEEK_END);
                    file_length = ftell(file_ptr);
                    fseek(file_ptr,0,SEEK_SET);
                    //Fill buffer sizes
                    part_size[0] = file_length / 4;
                    part_size[1] = part_size[0];
                    part_size[2] = part_size[0];
                    part_size[3] = file_length - part_size[0] * 3;
                    //Allocate buffer space
                    file_bufs[0] = (char*)malloc(part_size[0]);
                    file_bufs[1] = (char*)malloc(part_size[1]);
                    file_bufs[2] = (char*)malloc(part_size[2]);
                    file_bufs[3] = (char*)malloc(part_size[3]);
                    //Read into buffers
                    fread(file_bufs[0],sizeof(char),part_size[0],file_ptr);
                    fread(file_bufs[1],sizeof(char),part_size[1],file_ptr);
                    fread(file_bufs[2],sizeof(char),part_size[2],file_ptr);
                    fread(file_bufs[3],sizeof(char),part_size[3],file_ptr);
                    //Use md5 sum to determine which server gets which pieces
                    char *md5_buf;
                    md5_buf = (char*)malloc(file_length);
                    bzero(md5_buf,file_length);
                    fseek(file_ptr,0,SEEK_SET);
                    fread(md5_buf,sizeof(char),file_length,file_ptr);
                    fclose(file_ptr);
                    unsigned char *md5_result = NULL;
                    md5_result = malloc(MD5_DIGEST_LENGTH);
                    bzero(md5_result,MD5_DIGEST_LENGTH);
                    MD5(md5_buf,file_length,md5_result);
                    //Call function to get md5 mod
                    md5sum = get_md5_mod(md5_result,4);
                    //printf("hash is %d\n",md5sum);
                    free(md5_result);
                    free(md5_buf);
                    int pair_to_send = md5sum;
                    //loop through servers and execute command
                    for(int i=0;i<MAXSERVERS;i++){
                        //Connect to server
                        sockfd = client_connect(avaliable_servers[i].port,avaliable_servers[i].ip);
                        //If it returns -1 the server is unavaliable
                        if(sockfd == -1){
                            printf("Server: %s at IP: %s on Port: %d is offline\n",avaliable_servers[i].name,avaliable_servers[i].ip,avaliable_servers[i].port);
                        }else{
                            //Send username and password in form "username password"
                            bzero(buf,MAXBUF);
                            snprintf(buf,MAXBUF,"%s %s",username,password);
                            n = write(sockfd,buf,100);
                            //Read response
                            bzero(buf,MAXBUF);
                            n = read(sockfd,buf,100);
                            if(strncmp(buf,"Invalid Username/Password. Please try again.",strlen(buf)) == 0){
                                //Print buffer as error message, invalid username or password
                                printf("%s\n",buf);
                                return 0;
                            }else{
                                //Execute commands
                                //Check if we have a sub directory added
                                bzero(buf,MAXBUF);
                                if(num_args == 2){
                                    snprintf(buf,MAXBUF,"%s %s",store_command,store_filename);
                                }else{
                                    snprintf(buf,MAXBUF,"%s %s %s",store_command,store_filename,subdirectory_command);
                                }
                                n = write(sockfd,buf,100);
                                //This is how we determine which servers get what pair of files using md5 mod
                                if(pair_to_send > 3){
                                    pair_to_send = 0;
                                }
                                //Send files to servers
                                for(int j=0;j<2;j++){
                                    //Index so we send 1,2 2,3 3,4 4,1
                                    int index = pair_to_send + j;
                                    if(index > 3){
                                        index = 0;
                                    }
                                    //Send part length and part number
                                    bzero(buf,MAXBUF);
                                    snprintf(buf,MAXBUF,"%d %d",part_size[index],index+1);
                                    n = write(sockfd,buf,100);
                                    //Wait for a response
                                    //read(sockfd,buf,MAXBUF);
                                    //Send the buffer over
                                    int bytes_left = part_size[index];
                                    int bytes_read = 0;
                                    while(bytes_left != 0){
                                        if(bytes_left > MAXBUF){
                                            //Send a full buffer
                                            bzero(buf,MAXBUF);
                                            memcpy(buf,&file_bufs[index][bytes_read],MAXBUF);
                                            n = write(sockfd,buf,MAXBUF);
                                            bytes_read = bytes_read + n;
                                            bytes_left = bytes_left - n;
                                        }else{
                                            //Send the remaining buffer
                                            bzero(buf,MAXBUF);
                                            memcpy(buf,&file_bufs[index][bytes_read],bytes_left);
                                            n = write(sockfd,buf,bytes_left);
                                            bytes_read = bytes_read + n;
                                            bytes_left = 0;
                                        }
                                    }
                                }
                                pair_to_send = pair_to_send + 1;
                            }
                        }
                    }
                }
                free(file_bufs[0]);
                free(file_bufs[1]);
                free(file_bufs[2]);
                free(file_bufs[3]);
            }else{
                printf("Please enter a filename after your command\n");
            }
        }else if(strncmp(store_command,"get",3) == 0){
            //check to see if a file name was specified
            if(num_args == 2 || num_args == 3){
                //loop through servers and execute command
                //Variables
                char ask_parts[4];
                ask_parts[0] = '1';
                ask_parts[1] = '2';
                ask_parts[2] = '3';
                ask_parts[3] = '4';
                for(int i=0;i<MAXSERVERS;i++){
                    //If ask_parts is all 0 we have everything and we can break
                    if((ask_parts[0] == '0') && (ask_parts[1] == '0') && (ask_parts[2] == '0') && (ask_parts[3] == '0')){
                        //printf("all parts found\n");
                        break;
                    }
                    //Connect to server
                    sockfd = client_connect(avaliable_servers[i].port,avaliable_servers[i].ip);
                    //If it returns -1 the server is unavaliable
                    if(sockfd == -1){
                        printf("Server: %s at IP: %s on Port: %d is offline\n",avaliable_servers[i].name,avaliable_servers[i].ip,avaliable_servers[i].port);
                    }else{
                        //Send username and password in form "username password"
                        bzero(buf,MAXBUF);
                        snprintf(buf,MAXBUF,"%s %s",username,password);
                        //printf("sending %s\n",buf);
                        n = write(sockfd,buf,100);
                        //Read response
                        bzero(buf,MAXBUF);
                        n = read(sockfd,buf,100);
                        if(strncmp(buf,"Invalid Username/Password. Please try again.",strlen(buf)) == 0){
                            //Print buffer as error message, invalid username or password
                            printf("%s\n",buf);
                            return 0;
                        }else{
                            //Execute commands
                            //printf("executing command\n");
                            //Check if we have a sub directory added
                            bzero(buf,MAXBUF);
                            if(num_args == 2){
                                snprintf(buf,MAXBUF,"%s %s",store_command,store_filename);
                            }else{
                                snprintf(buf,MAXBUF,"%s %s %s",store_command,store_filename,subdirectory_command);
                            }
                            //Write command to server
                            n = write(sockfd,buf,100);
                            //Write request
                            bzero(buf,MAXBUF);
                            memcpy(buf,ask_parts,4);
                            write(sockfd,buf,100);
                            for(int j=0;j<2;j++){
                                //Recieve response
                                bzero(buf,MAXBUF);
                                read(sockfd,buf,100);
                                //printf("recieved: %s\n",buf);
                                //If no file found break
                                if(buf[0] == '0' && buf[2] == '0'){
                                    break;
                                }else{
                                    //Assign file_size and part num variables
                                    int file_size;
                                    int part_num;
                                    char arg1[100];
                                    char arg2[100];
                                    sscanf(buf,"%s%s",arg1,arg2);
                                    file_size = atoi(arg1);
                                    part_num = atoi(arg2);
                                    //Change askparts
                                    ask_parts[part_num -1] = '0';
                                    //Malloc a buffer to input the file, add size
                                    file_bufs[part_num-1] = malloc(file_size);
                                    part_size[part_num-1] = file_size;
                                    //Read from the server into the buffer
                                    int bytes_left = file_size;
                                    int bytes_read = 0;
                                    while(bytes_left != 0){
                                        if(bytes_left > MAXBUF){
                                            bzero(buf,MAXBUF);
                                            n = read(sockfd,buf,MAXBUF);
                                            memcpy(&file_bufs[part_num-1][bytes_read],buf,MAXBUF);
                                            bytes_read = bytes_read + n;
                                            bytes_left = bytes_left - n;
                                        }else{
                                            bzero(buf,MAXBUF);
                                            n = read(sockfd,buf,bytes_left);
                                            memcpy(&file_bufs[part_num-1][bytes_read],buf,bytes_left);
                                            bytes_read = bytes_read + bytes_left;
                                            bytes_left = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                //printf("parts done: %s\n",ask_parts);
                //If everything is equal to zero we have all parts
                if((ask_parts[0] == '0') && (ask_parts[1] == '0') && (ask_parts[2] == '0') && (ask_parts[3] == '0')){
                    //Open file
                    file_ptr = fopen(store_filename,"w");
                    if(file_ptr != NULL){
                        //Write to file
                        for(int i=0;i<4;i++){
                            // printf("writing to file %s with part size %d\n",store_filename,part_size[i]);
                            fwrite(file_bufs[i],part_size[i],sizeof(char),file_ptr);
                            free(file_bufs[i]);
                        }
                        //close file
                        fclose(file_ptr);
                    }else{
                        printf("Unable to write to retrieved and complete file\n");
                    }
                    
                }else{
                    //Otherwise print we couldn't get all parts
                    printf("File: %s [incomplete]\n",store_filename);
                }
            }else{
                printf("Please enter a filename after your command\n");
            }
        }else if(strncmp(store_command,"MKDIR",5) == 0){
            for(int i=0;i<MAXSERVERS;i++){
                //Connect to server
                sockfd = client_connect(avaliable_servers[i].port,avaliable_servers[i].ip);
                //If it returns -1 the server is unavaliable
                if(sockfd == -1){
                    printf("Server: %s at IP: %s on Port: %d is offline\n",avaliable_servers[i].name,avaliable_servers[i].ip,avaliable_servers[i].port);
                }else{
                    //Send username and password in form "username password"
                    bzero(buf,MAXBUF);
                    snprintf(buf,MAXBUF,"%s %s",username,password);
                    //printf("sending %s\n",buf);
                    n = write(sockfd,buf,100);
                    //Read response
                    bzero(buf,MAXBUF);
                    n = read(sockfd,buf,100);
                        if(strncmp(buf,"Invalid Username/Password. Please try again.",strlen(buf)) == 0){
                        //Print buffer as error message, invalid username or password
                        printf("%s\n",buf);
                        return 0;
                    }else{
                        //Execute command
                        bzero(buf,MAXBUF);
                        snprintf(buf,MAXBUF,"%s %s",store_command,store_filename);
                        //Write command to server
                        n = write(sockfd,buf,100);
                    }
                }
            }
        }else{
            printf("%s is an unrecognized command\n", store_command);
        }
    }    
}

/*
* client_connect takes in the port and ipaddr and connects the client
* with the requested server, then returns the socket id
*/
int client_connect(int port, char* ip_address){
    int sockfd, connfd;
    struct sockaddr_in servaddr;

    //create socket
    sockfd = socket(AF_INET,SOCK_STREAM,0);

    bzero(&servaddr, sizeof(servaddr));

    //assign given ip and port
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(ip_address); 
    servaddr.sin_port = htons(port); 

    //connect client and server sockets
    if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) != 0 ){
        return -1;
    }
    else{
        return sockfd;
    }
}

/*
*   Was supposed to pull apart the 128bit hex md5 and run mod 4 on it but honestly
*   I don't know how this works. It runs mod 4 on something and its always the same 
*   for the same file so im just gonna call it good
*/
int get_md5_mod(unsigned char* hash_value, int mod_num){
    char *arr = malloc(MD5_DIGEST_LENGTH*2);
    for(int i = 0;i < MD5_DIGEST_LENGTH;i++){
        unsigned char *temp;
        temp = malloc(MD5_DIGEST_LENGTH);
        snprintf(temp,4,"%02x",hash_value[i]);
        memcpy(&arr[2*i],temp,2);
        free(temp);
    }
    unsigned long long a = *(unsigned long long*)arr;
    unsigned long long b = *(unsigned long long*)(arr+MD5_DIGEST_LENGTH);
    unsigned long long c = a ^ b;
    int d = b % mod_num;
    free(arr);
    if(d < 0 || d > 3){
        printf("Something is very wrong\n");
    }
    //printf("a = %llu\nb = %llu\nc = %llu\nd = %d\n",a,b,c,d);
    return d;
}

/*
*   https://stackoverflow.com/questions/4513316/split-string-in-c-every-white-space
*   https://gist.github.com/kwharrigan/2389719
*   https://stackoverflow.com/questions/11180028/converting-md5-result-into-an-integer-in-c
*/