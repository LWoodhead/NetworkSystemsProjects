Luke Woodhead
Csci 4273
Programming Assigment 3

makefile
    Creates executables

proxyserver_no_cache.c 
    Contains code for a proxy server with no caching. Proxy server takes get requests from a web browser using the correct port 
    using multiple threads each of which has a tcp connection with the browser. The proxy server breaks down the get requests
    and then uses gethostbyname to see if a connection is possible. If it is the proxy forwards the get request from the browser
    to server and then sends the response back to the browser as it recieves it.

proxyserver_with_cache.c
    Contains code for a proxy server that implements timeouts, caching and a blacklist. The proxy server caches server responses
    inside a linked list. The linked list contains values for the server Hostname, IP Address, Filename(that contents are stored
    in), Timestamp(to measure timeout value against) and Blacklist(which determines whether a host is blacklisted or not). The 
    nodes of the linked list can contain repeats of Hostname or IP Address but there is only one node corresponding to each file 
    and thus to each resource. The proxyserver begins by creating nodes for each of the blacklisted sites using the function
    create_blacklist(). Then it creates a listening socket. The listeing socket accepts requests and spins off a thread to
    deal with each request seperatly. 
    Inside each thread the control is a follows.
        1. Parse inputs to get the Hostname, Port and to Create a Filename
        2. Determines if the current hostname is already cached
            1. Determines if the current host is blacklisted, return 403 to browser if it is
                1. Determines if the request itself is cached
                    1. If the requst is cached, check if it is within the timeout
                        1. If it is within the timeout then read the cached file to the browser
                        2. If it is not within the timeout connect to the server with Port and Hostname and retrieve a new copy,
                           save the new copy to the existing node by writing it into the file and send it to the browser, then
                           update the node timestamp
                2. If the request is not cached 
                    1. Connect to the server with Port and Hostname, use the filename to write the data from the server into
                       the file and send it to the browser, then create a new node containing the file, hostname and IP
        3. If the current hostname isn't cached
            1. Use gethostbyname to find the hosts ipaddress
                1. If the ipaddress exists
                    1. Check to make sure the ipaddress isn't blacklisted, if it is send a 403 to the browser
                    2. If ipaddress isn't blacklisted
                        1. Connect to server using ipaddress, open a file, forward browser request to the server, write server
                           response into both the file and to the browser, then create a new node with the hostname, ipaddress
                           and filename
                2. If it doesn't exist
                    1. return 500 error
        3. Return a 500 error if the request was anything but GET

    Helper Functions
        open_listenfd
            Borrowed function from pa2 that creates a listening socket in main
        client_connect
            Creates a tcp connection to a server from the proxy
        ProxyGet
            Function described above, sends, recieves, caches, etc
        *thread
            Creates threads
        add_host
            Adds a node to the linked list, access limited to one use at a time via semaphore
        search_hosts
            Searchs linked list for a node with a matching ip or hostname, access limited to one use at a time via semaphore
        search_host_files
            Searchs linked list for a node with a matching filename, access limited to one use at a time via semaphore
        print_node
            Prints a node, used to test 
        timestamp_difference
            Used to determine if a cached resource is older than the timeout value
        free_host_list
            Frees the memory assigned to the linked list
        create_blacklist
            Uses the given filename to add blacklisted hostname and ipaddress nodes to the linked list

Instructions to use:
    Use 
        make
    In terminal to create cache_proxy and no_cache_proxy executables
    To run proxy without cache use
        ./no_cache_proxy <port>
    To run with cache use
        ./cache_proxy <port> <timeout value in seconds> <blacklist file name>
    The blacklist file must be 1 blacklisted host or ip per line followed by a new line character

Notes
    Only tested this using firefox proxy, is slow to retrieve resources
