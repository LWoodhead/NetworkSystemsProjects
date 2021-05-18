Luke Woodhead
CSCI 4273
Project 4

    DFS_client.c
        To run, use make to run makefile then where clientconfigfile is the config file for a user.
        ./DFS_client <clientconfigfile>
        clientconfigfile must be in the form below, each line should be seperated by a newline, each element should be seperated by a 
        space and it should be 5 lines long. 4 lines of server information and 1 for a username and password. Exit with ctrl + c

        <server name> <server ip> <port>
        <server name> <server ip> <port>
        <server name> <server ip> <port>
        <server name> <server ip> <port>
        <username> <password>

        DFS_client.c runs as the client for a distributed file system and it has 4 commands. If no optional directory is added to 
        a command it defaults to the users base directory.
        list <optional directory>
            Which lists all the files in a specified directory over all servers as complete if all 4 parts are avaliable or incomplete 
            if parts are missing
        put <filename> <optional directory>
            Splits the specified file into 4 parts then sends 2 parts to each server to be put in the specified directory. Which 
            server the parts go to depends on the MD5hash of the file contents
        get <filename> <optional directory>
            Attempts to retrieve all 4 parts of a file from specified directory from each server. If all 4 parts are found then the
            file is written to the client otherwise the client will print filename [incomplete] and do nothing else
        MKDIR <directory> 
            Creates a directory inside the user directory

    DFS_server.c
        Run by using make then where port is specified in the clientconfigfile and should correspond to 1 of the 4 ports and
        directory name is the name of the directory the server should write to. If the directory doesn't exist the server
        will create one. Servers run in background. Exit with fg and ctrl + c.
        ./DFS_server.c <port> <directory name>&
        Also requires a serverconfigfile, where each line should contain a username and password followed by a newline.
        Demonstrated below, max of 10 username password pairs 

        <username> <password>
        <username> <password> 
        <username> <password> 
        <username> <password> 
        <username> <password>

        The server is one of 4 responding to the client. Servers store file pieces from client put and send those file pieces
        when the client makes a get request. The server create extra directories with MKDIR and returns a list of fileparts in 
        response ot list. The servers will not service customers with an incorrect username password pair.
        The server is multithread and can service multiple clients and requests at the same time.

Copy past to run all 4 servers in background
./DFS_server 10001 DFS1&
./DFS_server 10002 DFS2&
./DFS_server 10003 DFS3&
./DFS_server 10004 DFS4&

