/// file travel_server.c
/// Travel agent server application
/// https://gitlab.com/openhid/travel-agency

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include "hash_map.h"

/// @brief Wraps strcmp to compare in @p string & @p other_string equality
/// @returns 1 if true, false otherwise.
int string_equal(char const * string, char const * other_string);

/// @brief Processes command string from client and provides response
/// @returns NULL if command is invalid, else string response
char * process_flight_request(char * command, HashMapPtr flight_map);

// Holds flight/seat pairs of clients
HashMapPtr flight_map;

// initialize connection threads
pthread_t * connection_threads;

// semaphore used to close connections
int accepting_connections = 1;

// Synchronizes access to flight_map
pthread_mutex_t flight_map_mutex;

// thread attributes
pthread_attr_t thread_attr;

/// @brief contains socket information
/// includes file descriptor port pair.
typedef struct {
    int fd;
    int port;
    char * ip_address;
} socket_info;

/// @brief accepts socket connection handler
/// handler is run on separate thread
/// @param [in] socket_info  contains socket info
/// of type socket_info
void * accept_socket_handler(void * socket_info);

/// @brief initializes inet socket with given address information
/// @param sockfd socket file descriptor
/// @param ip_address ipv4 addresss
/// @param port adderess port
void init_inet_socket(socket_info * sockfd, char * ip_address, int port);

/// @brief error handler
/// prints error and exits with error status
void error(char * const message) {
    perror(message);
    pthread_exit(NULL);
}
                          
int main (int argc, char * argv[]) {
    // if not enough arguments
    if (argc < 6)
        error("usage: talent_server ip_address start_port no_ports in_file out_file\n");

    // read command arguments
    char * ip_address = argv[1];
    int start_port = atoi(argv[2]);
    int no_ports = atoi(argv[3]);
    char * in_filename = argv[4];
    char * out_filename = argv[5]; 

    // initialize flight map
    flight_map = HashMap_New(NULL, NULL, NULL);
    
    /// initialize multithreading objects
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&flight_map_mutex, NULL);
    connection_threads = malloc(sizeof(pthread_t) * no_ports);
    accepting_connections = 1;
    
    // initialize sockets
    socket_info servers[no_ports];
    int index = 0;
    for (index = 0; index < no_ports; index++) {
        int port = start_port + index;
        
        // intialize socket info
        init_inet_socket(&servers[index], ip_address, port);
        
        // Start accept handler on another thread
        int thread_id;
        if ((thread_id = pthread_create(&connection_threads[index], &thread_attr, accept_socket_handler, &servers[index])))
        {}
        
        printf("server: started connection handler thread %d", thread_id);
    }

    // loop until exit command
    while (1) {
        // buffer incoming commands from command line
        int const BUFFER_SIZE = 256;
        char input[BUFFER_SIZE];
        memset(&input, 0, sizeof(input));

        printf("\nserver: enter command \n");
        fgets(input, sizeof(input), stdin);

        // if exit in command
        if (strstr(input, "EXIT"))
            break;
    }
   
    /// destroy multithreading objects
    accepting_connections = 0;
    pthread_attr_destroy(&thread_attr);
    pthread_mutex_destroy(&flight_map_mutex);
    for (index = 0; index < no_ports; index++) {
        pthread_join(connection_threads[index], NULL);
    }

    // free hash map memory
    HashMap_Free(flight_map);

    return 0; 
}

void * accept_socket_handler(void * handler_args) {
    // retrieve socket_info from args
    socket_info * inet_socket = (socket_info *) handler_args;
    
    int clientfd;
    
    // TODO: replace 1 with semaphore
    while (accepting_connections) {
        
        printf("\nserver: listening on %s:%d\n", inet_socket->ip_address, inet_socket->port);
        // socket address information
        struct sockaddr_in client_address;
        
        // accept incoming connection from client
        socklen_t client_length = sizeof(client_address);
        if ((clientfd = accept(inet_socket->fd, (struct sockaddr *) &client_address, &client_length)) < 0)
            error("error: accepting connection");

        // get incoming connection information
        struct sockaddr_in * incoming_address = (struct sockaddr_in *) &client_address;
        int incoming_port = ntohs(incoming_address->sin_port); // get port
        char incoming_ip_address[INET_ADDRSTRLEN]; // holds ip address string
        // get ip address string from address object
        inet_ntop(AF_INET, &incoming_address->sin_addr, incoming_ip_address, sizeof(incoming_ip_address));

        printf("server: incoming connection from %s:%d\n", incoming_ip_address, incoming_port);
        
        // holds incoming data from socket
        int const BUFFER_SIZE = 256;
        char input[BUFFER_SIZE];

        // read data sent from socket into input
        if (read(clientfd, input, sizeof(input)) < 0)
            error("error: reading from connection");

        printf("server: read %s from client\n", input);
        
        // process commands
        char * current_data = process_flight_request(input, flight_map);
        
        // exit command breaks loop
        if (string_equal(current_data, "EXIT"))
            break;

        // write to connection with reply
        if (write(clientfd, current_data, strlen(current_data) + 1) < 0)
            error("error: writing to connection");

        printf("server: sent %s to client\n", current_data);
    }
    
    // close server socket
    close(inet_socket->fd);
    printf("server: exiting\n");
    
    pthread_exit(handler_args);
}

void init_inet_socket(socket_info * inet_socket, char * ip_address, int port) {
    
    // copy ip_address to socket_info object
    inet_socket->ip_address = malloc(sizeof(char) * strlen(ip_address));
    strcpy(inet_socket->ip_address, ip_address);
    
    struct sockaddr_in server_address;
    
    // open server socket 
    if ((inet_socket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("error: opening server socket");
    
    int enable = 1;
    if (setsockopt(inet_socket->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error("error: cannot set socket options");

    // initialize server_address (sin_zero) to 0
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET; // set socket type, INET sockets are bound to IP 
    server_address.sin_port = htons(port); // assign port number
    inet_aton(ip_address, &server_address.sin_addr); // assign socket ip address

    // bind socket information to socket handler
    if (bind(inet_socket->fd, (struct sockaddr *) & server_address, sizeof(server_address)) < 0)
        error("error: binding socket to address");
        
    // maximum number of input connections queued at a time
    int const MAX_CLIENTS = 10;

    // listen for connections on sockets
    if (listen(inet_socket->fd, MAX_CLIENTS) < 0)
        error("error: listening to connections");
}

int string_equal(char const * string, char const * other_string) {
    return strcmp(string, other_string) == 0; 
}


char * process_flight_request(char * input, HashMapPtr flight_map) {
    // split command string to get command and arguments
    char * command_token;
    char * command = strtok_r(input, " ", &command_token);
    
    printf("server: processing command %s\n", command);

    // get seats from flight map
    if (string_equal(command, "QUERY")) {
        char * flight = strtok(NULL, " "); 
        printf("server: queryin flight %s\n", flight);

        char * seats;
        
        // acquire lock to flight_map
        pthread_mutex_lock(&flight_map_mutex);
        
        // retrieve seats from map
        if ((seats = (char *) HashMap_GetValue(flight_map, flight))) {
            printf("server: flight %s has %s seats\n", flight, seats);
            return seats;
        }
        pthread_mutex_unlock(&flight_map_mutex);
    } 
    // add seats to flight map
    else if (string_equal(command, "RESERVE")) {
        char * flight = strtok(NULL, " ");
        char * seats = strtok(NULL, " ");
        
        // acquire lock to flight_map
        pthread_mutex_lock(&flight_map_mutex);
        HashMap_Add(flight_map, flight, seats);
        pthread_mutex_unlock(&flight_map_mutex);
        
        printf("server: reserving %s seats on flight %s\n", seats, flight);
        
        return "RESERVED";
    }
    // handle other commands
    if (string_equal(command, "RETURN")) {}
    if (string_equal(command, "LIST")) {}
    if (string_equal(command, "LIST_AVAILABLE")) {}
    if (string_equal(command, "EXIT")) {
        return "EXIT";
    }

    return "ERROR"; 
}