/* 
 * DFS_server.c, distributed file system server 
 * usage: DFS_server <portnumber> <server name>
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
#include <sys/stat.h> /* to create directories */
#include <dirent.h> /* to view directories */

#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MAXUSERS 10

int open_listenfd(int port);
void handle_request(int connfd);
void *thread(void *vargp);

//Structs
struct user{
    char user[100];
    char pass[100];
};

//Global Variables
char *server_name;
int number_of_users;
struct user avaliable_users[MAXUSERS]; 

int main(int argc, char **argv) 
{
    //Declare Variables
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;
    FILE * file_ptr;
    char username[100];
    char password[100];
    char config_buffer[100];
    int i;
    int directory_check;

    /* check command line arguments */
    if (argc != 3) {
	fprintf(stderr, "usage: <port> <server name>\n");
	exit(0);
    }

    //Set arguments
    port = atoi(argv[1]);
    server_name = argv[2];
    i = 0;
    bzero(avaliable_users,sizeof(struct user)*MAXUSERS);

    //Parse config info
    file_ptr = fopen("serverconfig.txt","r"); /* Input whatever your server config filename is here*/
    if(file_ptr != NULL){
        bzero(config_buffer,100);
        while(fgets(config_buffer, 100, file_ptr)){
            bzero(username,100);
            bzero(password,100);
            sscanf(config_buffer,"%s%s",username,password);
            //printf("Username: %s Password: %s\n",username,password);
            memcpy(avaliable_users[i].user,username,strlen(username));
            memcpy(avaliable_users[i].pass,password,strlen(password));
            bzero(config_buffer,100);
            i++;
        }
        fclose(file_ptr);
        number_of_users = i;
    }else{
        printf("Unable to open server config file\n");
        exit(0);
    }
    // for(int j=0;j<i;j++){
    //     printf("At index: %d username: %s password: %s\n",j,avaliable_users[j].user,avaliable_users[j].pass);
    // }

    //Move into servers directory
    directory_check = chdir(server_name);
    if(directory_check == -1){
        //if it doesn't exist create it
        directory_check = mkdir(server_name,0777);
        if(directory_check == -1){
            printf("Error in creating directory\n");
            exit(0);
        }else{
            directory_check = chdir(server_name);
        }
    }

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
    handle_request(connfd); //-> handles list, put or get request
    close(connfd);
    return NULL;
}

/*
 * handles list, put or get request
 */
void handle_request(int connfd) 
{
    //Variables
    size_t n;
    char buf[MAXBUF];
    char temp_buf[MAXBUF];
    int valid_user;
    int directory_check;
    char client_username[100];
    char client_password[100];
    char store_command[100];
    char one_argument[100];
    char two_argument[100];
    char file_size[100];
    char file_part[100];
    char filepath[100];
    char temp_filepath[100];
    char part_filepath[100];
    struct dirent *de;
    DIR *dir;
    char s[100];
    int num_args; 

    //Set variables
    valid_user = 0;

    //Recieve username and password
    bzero(buf,MAXBUF);
    n = read(connfd,buf,100);
    bzero(client_password,100);
    bzero(client_username,100);
    sscanf(buf,"%s%s",client_username,client_password);
    for(int i=0;i<number_of_users;i++){
        if(strcmp(avaliable_users[i].user,client_username) == 0){
            if(strcmp(avaliable_users[i].pass,client_password) == 0){
                valid_user = 1;
                break;
            }
        }
    }
    //If the user is invalid send an error and close, else execute command
    if(valid_user == 0){
        bzero(buf,MAXBUF);
        snprintf(buf,MAXBUF,"Invalid Username/Password. Please try again.");
        write(connfd,buf,100);
    }else{
        bzero(buf,MAXBUF);
        snprintf(buf,MAXBUF,"Valid account");
        write(connfd,buf,100);

        //Check to see if we have a subdirectory under the username
        directory_check = 0;
        dir = opendir(".");
        if(dir != NULL){
            while((de = readdir(dir)) != NULL){
                if(strncmp(de->d_name,client_username,strlen(client_username)) == 0){
                    directory_check = 1;
                    break;
                }
            }
            closedir(dir);
        }else{
            printf("Unable to search for client directory\n");
            exit(0);
        }
        
        //Create a directory if none exists
        if(directory_check == 0){
            directory_check = mkdir(client_username,0777);
            if(directory_check == -1){
                printf("Unable to create a directory for user");
                exit(0);
            }
        }

        //Set a base file path
        snprintf(filepath,100,"%s/",client_username);
        //Execute command
        //Read command from client
        bzero(buf,MAXBUF);
        n = read(connfd,buf,100);
        //Parse with sscanf
        num_args = sscanf(buf,"%s%s%s",store_command,one_argument,two_argument);

        //Do list command
        if(strncmp(store_command,"list",4) == 0){
            if(num_args == 2){
                //change filepath to include subdirectory
                bzero(temp_filepath,100);
                memcpy(temp_filepath,filepath,strlen(filepath));
                bzero(filepath,100);
                snprintf(filepath,100,"%s%s/",temp_filepath,one_argument);
                //Make the directory, command fails if it already exists
                mkdir(filepath,0777);
            }
            //Find all file names and add them to buffer with a space inbetween each one
            bzero(buf,MAXBUF);
            dir = opendir(filepath);
            if(dir != NULL){
                while((de = readdir(dir)) != NULL){
                    //If the file isn't .. or . add to the list
                    if((strcmp(de->d_name,"..") != 0) && (strcmp(de->d_name,".") != 0)){
                        bzero(temp_buf,MAXBUF);
                        memcpy(temp_buf,buf,strlen(buf));
                        bzero(buf,MAXBUF);
                        snprintf(buf,MAXBUF,"%s %s",temp_buf,de->d_name);
                    }
                }
                closedir(dir);
            }else{
                printf("Unable to search for client directory\n");
                exit(0);
            }
            //Write list to client
            write(connfd,buf,MAXBUF);
        }

        //Do put command
        if(strncmp(store_command,"put",3) == 0){
            if(num_args == 3){
                //change filepath to include subdirectory
                bzero(temp_filepath,100);
                memcpy(temp_filepath,filepath,strlen(filepath));
                bzero(filepath,100);
                snprintf(filepath,100,"%s%s/",temp_filepath,two_argument);
                //Make the directory, command fails if it already exists
                mkdir(filepath,0777);
            }
            //Recieve and save the two file parts being sent
            for(int j=0;j<2;j++){
                //Read file size and part number
                bzero(buf,MAXBUF);
                n = read(connfd,buf,100);
                bzero(file_part,100);
                bzero(file_size,100);
                sscanf(buf,"%s%s",file_size,file_part);
                //Send a response
                //Create file
                bzero(temp_filepath,100);
                memcpy(temp_filepath,filepath,strlen(filepath));
                bzero(part_filepath,100);
                snprintf(part_filepath,100,"%s.%s.%s",temp_filepath,one_argument,file_part);
                FILE * file_ptr;
                file_ptr = fopen(part_filepath,"w");
                if(file_ptr != NULL){
                    //Read in the file part
                    int bytes_left = atoi(file_size);
                    while(bytes_left != 0){
                        if(bytes_left > MAXBUF){
                            bzero(buf,MAXBUF);
                            n = read(connfd,buf,MAXBUF);
                            fwrite(buf,MAXBUF,sizeof(char),file_ptr);
                            bytes_left = bytes_left - n;
                        }else{
                            bzero(buf,MAXBUF);
                            n = read(connfd,buf,bytes_left);
                            fwrite(buf,bytes_left,sizeof(char),file_ptr);
                            bytes_left = 0;
                        }
                    }
                    fclose(file_ptr);
                }else{
                    printf("Could not open file at %s\n",filepath);
                }
            }
        }

        //Do get command
        if(strncmp(store_command,"get",3) == 0){
            //Variables
            char bezero[1];
            bezero[0] = '0';
            if(num_args == 3){
                //change filepath to include subdirectory
                bzero(temp_filepath,100);
                memcpy(temp_filepath,filepath,strlen(filepath));
                bzero(filepath,100);
                snprintf(filepath,100,"%s%s/",temp_filepath,two_argument);
                //Make the directory, command fails if it already exists
                mkdir(filepath,0777);
            }
            //Read request
            bzero(buf,MAXBUF);
            read(connfd,buf,100);
            //Save buf
            char requested_files[4];
            bzero(requested_files,4);
            memcpy(requested_files,buf,4);
            for(int j=0;j<2;j++){
                int file_found = 0;
                //Move through directory to find matching files
                char part_filename[100];
                bzero(part_filename,100);
                dir = opendir(filepath);
                if(dir != NULL){
                    while((de = readdir(dir)) != NULL){
                        if((strncmp(&de->d_name[1],one_argument,strlen(one_argument)) == 0) && (strlen(one_argument) == strlen(de->d_name)-3)){
                            if(de->d_name[strlen(de->d_name)-1] == requested_files[0]){
                                memcpy(part_filename,de->d_name,strlen(de->d_name));
                                memcpy(&requested_files[0],bezero,1);
                                //printf("Found matching file %s\n",part_filename);
                                file_found = 1;
                                break;
                            }else if(de->d_name[strlen(de->d_name)-1] == requested_files[1]){
                                memcpy(part_filename,de->d_name,strlen(de->d_name));
                                memcpy(&requested_files[1],bezero,1);
                                //printf("Found matching file %s\n",part_filename);
                                file_found = 1;
                                break;
                            }else if(de->d_name[strlen(de->d_name)-1] == requested_files[2]){
                                memcpy(part_filename,de->d_name,strlen(de->d_name));
                                memcpy(&requested_files[2],bezero,1);
                                //printf("Found matching file %s\n",part_filename);
                                file_found = 1;
                                break;
                            }else if(de->d_name[strlen(de->d_name)-1] == requested_files[3]){
                                memcpy(part_filename,de->d_name,strlen(de->d_name));
                                memcpy(&requested_files[3],bezero,1);
                                //printf("Found matching file %s\n",part_filename);
                                file_found = 1;
                                break;
                            }
                        }
                    }
                    closedir(dir);
                    bzero(buf,MAXBUF);
                    if(file_found == 1){
                        //Set filepath
                        bzero(temp_filepath,100);
                        memcpy(temp_filepath,filepath,strlen(filepath));
                        bzero(part_filepath,100);
                        snprintf(part_filepath,100,"%s%s",temp_filepath,part_filename);
                        int file_size;
                        FILE * file_ptr = fopen(part_filepath,"rb");
                        if(file_ptr != NULL){
                            //Find file size
                            fseek(file_ptr,0,SEEK_END);
                            file_size = ftell(file_ptr);
                            //printf("%d is filesize\n",file_size);
                            fseek(file_ptr,0,SEEK_SET);
                            //Send file size and part num and write them
                            snprintf(buf,MAXBUF,"%d %c",file_size,part_filename[strlen(part_filename)-1]);
                            write(connfd,buf,100);
                            int bytes_left = file_size;
                            while(bytes_left != 0){
                                bzero(buf,MAXBUF);
                                if(bytes_left > MAXBUF){
                                    fread(buf,sizeof(char),MAXBUF,file_ptr);
                                    n = write(connfd,buf,MAXBUF);
                                    bytes_left = bytes_left - n;
                                }else{
                                    fread(buf,sizeof(char),bytes_left,file_ptr);
                                    write(connfd,buf,bytes_left);
                                    bytes_left = 0;
                                }
                            }
                            fclose(file_ptr);
                        }else{
                            printf("Unable to open %s to read to client\n",part_filepath);
                            snprintf(buf,MAXBUF,"0 0");
                            write(connfd,buf,100);
                        }
                    }else{
                        //No files on this server so we break
                        snprintf(buf,MAXBUF,"0 0");
                        write(connfd,buf,100);
                        //printf("Sent %s\n",buf);
                        break;
                    }
                }else{
                    printf("Unable to open directory %s in get\n",filepath);
                }
            }
            //printf("Final buf is %s\n",requested_files);
        }

        //MKDIR command
        if(strncmp(store_command,"MKDIR",5) == 0){
            //Alter filepath
            bzero(temp_filepath,100);
            memcpy(temp_filepath,filepath,strlen(filepath));
            bzero(filepath,100);
            snprintf(filepath,100,"%s%s/",temp_filepath,one_argument);
            //Create directory
            mkdir(filepath,0777);
        }
    }
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