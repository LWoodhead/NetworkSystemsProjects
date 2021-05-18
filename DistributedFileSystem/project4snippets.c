    
    //Prints contents of server storages for client
    for(int i=0;i<4;i++){
        printf("At index: %d\nServername: %s\nServerIP: %s\nServerport: %d\n",i,avaliable_servers[i].name,avaliable_servers[i].ip,avaliable_servers[i].port);
    }
    //Check file directory
    printf("Directory before check Directory: %s\n", getcwd(s, 100));

    //Print ret_files contents
                for(int k=0;k<num_files;k++){
                            printf("Name: %s Index: %d\n",ret_files[k].name,k);
                            for(int j=0;j<4;j++){
                                printf("at index: %d values: %d\n",j,ret_files[k].file_parts[j]);
                            }
                        }
    //print buffer sizes
    printf("buf sizes are 1:%d 2:%d 3:%d 4:%d\n",one_size,two_size,three_size,four_size);


    int get_md5_mod(unsigned char* hash_value, int mod_num){
    unsigned char v1[8], v2[8], v3[8], v4[8];
    memcpy(&v1,&hash_value[0],8);
    memcpy(&v2,&hash_value[8],8);
    memcpy(&v3,&hash_value[16],8);
    memcpy(&v4,&hash_value[24],8);
    printf("v1:%s\nv2:%s\nv3:%s\nv4:%s\n",v1,v2,v3,v4);
    return 1;
}

int get_md5_mod(unsigned char* hash_value, int mod_num){
    char *arr = malloc(MD5_DIGEST_LENGTH*2);
    for(int i = 0;i < MD5_DIGEST_LENGTH;i++){
        unsigned char *temp;
        temp = malloc(MD5_DIGEST_LENGTH);
        snprintf(temp,4,"%02x",hash_value[i]);
        printf("%s\n",temp);
        //printf("%02x:",hash_value[i]);
        memcpy(&arr[2*i],temp,2);
        free(temp);
    }
    printf("%c\n",arr[1]);
    char brr[32];
    memcpy(brr,arr,32);
    printf("brr:%s\n",brr);
    return 1;
}

                    for(int i=0; i < MD5_DIGEST_LENGTH; i++)
                    {
                        printf("%02x",  md5_result[i]);
                    }

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