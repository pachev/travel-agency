/// https://gitlab.com/openhid/travel-agency

#define _GNU_SOURCE

#include <assert.h>
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

#include "c_hashmap/hashmap.h"


// Synchronizes access to flight_map
pthread_mutex_t flight_map_mutex;

/// @brief contains socket information
/// includes file descriptor port pair.
typedef struct {
    int fd;
    int port;
    char * ip_address;
    int enabled;
    map_t * map;
} socket_info;

/// @brief accepts socket connection handler
/// handler is run on separate thread
/// @param [in] socket_info  contains socket info
/// of type socket_info
void * server_handler(void * socket_info);

/// @brief Add flight to flight map
/// @param [in] flight_map hash map appended 
/// @param [in] flight flight string used as index
/// @param [in] seats on flight
void add_flight(map_t flight_map, char * flight, char * seats);

/// @brief frees memory associated with map items 
void free_flight_map_data(map_t flight_map);

/// @brief initializes inet socket with given address information
/// @param sockfd socket_info object to tbe initialized
/// @param ip_address ipv4 addresss
/// @param port adderess port
/// @returns 1 if error, 0 otherwise
int init_inet_socket(socket_info * sockfd, char * ip_address, int port); 

/// @brief binds socket info to sockddr object
/// @param socketinfo socket info source
/// @param sockaddr inet socket address 
void bind_inet_socket(socket_info * sockfd, struct sockaddr_in * socket_address);

/// @brief launches server on @p ports on a thread,
/// listens on ports [start_port, start_port + no_ports].
/// @returns 1 if error, 0 otherwise
int launch_server(socket_info * server, pthread_t * thread);

/// @brief joins server threads, and destroys 
/// related synchronization objects.
void close_servers(socket_info * server, pthread_t * thread, int no_ports);

/// @brief Processes command string from client and provides response
/// @returns NULL if command is invalid, else string response
char * process_flight_request(char * command, map_t flight_map);

/// @brief reads each line from @file_name expecting (flight, seats)
/// pair. flight pairs are added to @p flight_map
void read_flight_map_file(char * file_name, map_t flight_map);

/// @brief Wraps strcmp to compare in @p string & @p other_string equality
/// @returns 1 if true, false otherwise.
int string_equal(char const * string, char const * other_string);

/// @brief iterates @p map and invokes @p callback with key/value
/// pair of each map item
/// @param map hashmap iterated
/// @param callback function pointer taking as arguments
void * hashmap_foreach(map_t map, void(* callback)(char * key, void * data));

void * hashmap_foreach_n(map_t map, int n,  void(* callback)(char * key, void * data));

/// @brief error handler
/// prints error and exits with error status
void error(char * const message) {
    perror(message);
    pthread_exit(NULL);
}

/// @brief iterates @p map and invokes @p callback with key/value
/// pair of each map item
/// @param map hashmap iterated
/// @param callback function pointer taking as arguments
void * hashmap_foreach(map_t map, void(* callback)(char * key, void * data));

                          
int main (int argc, char * argv[]) {
    // if not enough arguments
    if (argc < 6) {
        error("usage: talent_server ip_address start_port no_ports in_file out_file\n");
    }

    // read command arguments
    char * ip_address = argv[1];
    int start_port = atoi(argv[2]);
    int no_ports = atoi(argv[3]);
    char * in_filename = argv[4];
    char * out_filename = argv[5]; 

    // initialize flight map
    // Holds flight/seat pairs of clients
    map_t flight_map = hashmap_new(); 

    read_flight_map_file(in_filename, flight_map);

    /// @brief socket server handlers 
    /// used as thread args
    socket_info servers[no_ports];

    // initialize connection threads
    pthread_t * connection_threads = calloc(no_ports, sizeof(pthread_t));

    int index;
    for (index = 0; index < no_ports; index++) {
        int port = start_port + index;

        // set server map to flight map
        servers[index].map = flight_map;

        // initialize inet sockets
        if (init_inet_socket(&servers[index], ip_address, port) != 0) {
            printf("error: cannot open socket on %s:%d", ip_address, port);
            break;
        }

        // launch servers on sockets
        if (launch_server(&servers[index], &connection_threads[index]) != 0) {
            printf("error: cannot launch server on %s:%d", ip_address, port);
            break;
        }
    }

    // data input
    while (1) {
        // buffer incoming commands from command line
        size_t const BUFFER_SIZE = 256;
        char input[BUFFER_SIZE];
        memset(&input, 0, sizeof(input));

        printf("\nserver: enter command \n");
        fgets(input, sizeof(input), stdin);

        // if exit in command
        if (strstr(input, "EXIT")) {
            break;
        } 
    }

    printf("server: closing servers\n");

    // join server threads
    close_servers(servers, connection_threads, no_ports);
    
    free(connection_threads);

    // free memory of items in hashmap
    free_flight_map_data(flight_map);

    // free hashmap handle
    hashmap_free(flight_map);

    return 0; 
} // main

void add_flight(map_t flight_map, char * flight_token, char * seats_token) {
    // assign persistent memory for hash node
    char * flight = malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = malloc(sizeof(char) * strlen(seats_token) + 1);

    // TODO: free memory in request on server thread
    
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);

    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    assert(hashmap_put(flight_map, flight, seats) == MAP_OK);
    pthread_mutex_unlock(&flight_map_mutex);

    printf("server: Adding fllight %s with %s seats\n", flight, seats);
} // add_flight

int reserve_flight(map_t flight_map, char * flight_token, char * seats_token) {
     //0 is not found, 1 is ok , 2 is full 

    size_t const MAX_SEATS = 40;
    char * flight = malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = malloc(sizeof(char) * strlen(seats_token) + 1);
    int curr_seat_value = 0;
    char *checking = malloc(sizeof(char) * strlen(seats_token + 1));
    
   
    int map_find_result = 0; 
    //
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);

    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    map_find_result = hashmap_get(flight_map, flight, (char*)checking); 
    pthread_mutex_unlock(&flight_map_mutex);

    if(map_find_result == MAP_OK) {

        if(curr_seat_value < 0)
            return 2;

        pthread_mutex_lock(&flight_map_mutex);
        assert(hashmap_put(flight_map, flight, seats) == MAP_OK);
        pthread_mutex_unlock(&flight_map_mutex);


        printf("server: Reserved %s on flight %s. Updated Seats: %d   Retrieved: %s", seats, flight, &curr_seat_value, checking);
        return 1;
        
    }
    else 
        return 0;
}

void * hashmap_foreach(map_t flight_map, void(* callback)(char *, void *)) {

    // data stuctures used to access hashmap internals 
    struct hashmap_element {
        char* key;
        int in_use;
        any_t data;
    };

    struct hashmap_map {
        int table_size;
        int size;
        struct hashmap_element *data;
    };

    char *info = malloc(sizeof(char) * 256);//TODO fix possible memory issues
    info[0] = '\n';

    struct hashmap_map * map = (struct hashmap_map *) flight_map;

    // iterate each hashmap, invoke callback if index in us
    int index;
    for (index = 0; index < map->table_size; index++) {
        if (map->data[index].in_use != 0) {
            callback(map->data[index].key, map->data[index].data);
            sprintf(info + strlen(info), "%s Flight: %s Seats\n", map->data[index].key, (char*)map->data[index].data);
        }
    }

void * hashmap_foreach_n(map_t flight_map, int n, void(* callback)(char *, void *)) {

    // data stuctures used to access hashmap internals 
    struct hashmap_element {
        char* key;
        int in_use;
        any_t data;
    };

    struct hashmap_map {
        int table_size;
        int size;
        struct hashmap_element *data;
    };

    char *info = malloc(sizeof(char) * 256);//TODO fix possible memory issues
    info[0] = '\n';

    struct hashmap_map * map = (struct hashmap_map *) flight_map;

    // iterate each hashmap, invoke callback if index in us
    for (n = 0; n < map->table_size; n++) {
        if (map->data[n].in_use != 0) {
            callback(map->data[n].key, map->data[n].data);
            sprintf(info + strlen(info), "%s Flight: %s Seats\n", map->data[n].key, (char*)map->data[n].data);
        }
    }

    return (void*) info; //keep track of all info in hash map

} // hashmap_foreach_n to iterate over first n elements

    return (void*) info; //keep track of all info in hash map

} // hashmap_foreach

void free_flight(char * key, void * data) {
    free(key);
    free(data);
}

void  free_flight_map_data(map_t flight_map) {
    hashmap_foreach(flight_map, &free_flight);
} // free_flight_map_data

void * server_handler(void * handler_args) {
    // retrieve socket_info from args
    socket_info * inet_socket = (socket_info *) handler_args;
    
    int clientfd;
        
    // maximum number of input connections queued at a time
    int const MAX_CLIENTS = 10;

    // listen for connections on sockets 
    if (listen(inet_socket->fd, MAX_CLIENTS) < 0) {
        error("error: listening to connections");
    }
    printf("\n%d: listening on %s:%d\n", inet_socket->port, inet_socket->ip_address, inet_socket->port);

    // TODO: replace 1 with semaphore
    while (inet_socket->enabled) {
        // socket address information
        struct sockaddr_in client_address;
        
        // accept incoming connection from client
        socklen_t client_length = sizeof(client_address);
        if ((clientfd = accept(inet_socket->fd, (struct sockaddr *) &client_address, &client_length)) < 0) {
            error("error: accepting connection");
        }

        // get incoming connection information
        struct sockaddr_in * incoming_address = (struct sockaddr_in *) &client_address;
        int incoming_port = ntohs(incoming_address->sin_port); // get port
        char incoming_ip_address[INET_ADDRSTRLEN]; // holds ip address string
        // get ip address string from address object
        inet_ntop(AF_INET, &incoming_address->sin_addr, incoming_ip_address, sizeof(incoming_ip_address));

        printf("%d: incoming connection from %s:%d\n", inet_socket->port, incoming_ip_address, incoming_port);
        
        // holds incoming data from socket
        int const BUFFER_SIZE = 256;
        char input[BUFFER_SIZE];

        // read data sent from socket into input
        if (read(clientfd, input, sizeof(input)) < 0) {
            error("error: reading from connection");
        }

        printf("%d: read %s from client\n", inet_socket->port, input);
        
        // exit command breaks loop
        if (string_equal(input, "EXIT")) {
            break;
        }

        // process commands
        char * current_data = process_flight_request(input, inet_socket->map);

        // write to connection with reply
        if (write(clientfd, current_data, strlen(current_data) + 1) < 0) {
            error("error: writing to connection");
        }

        printf("%d: sent %s to client\n", inet_socket->port, current_data);

        // close socket
        close(clientfd);
    }
    
    // close server socket
    close(inet_socket->fd);
    printf("%d: exiting\n", inet_socket->port);
    
    pthread_exit(handler_args);
} // server_handler

void close_servers(socket_info * servers, pthread_t * threads, int no_ports) {
    int index;
    for (index = 0; index < no_ports; index++) {
        /// signal threads sockets are disabled
        servers[index].enabled = 0;
        pthread_join(threads[index], NULL);
    }

    pthread_mutex_destroy(&flight_map_mutex);
} // close_servers

int init_inet_socket(socket_info * inet_socket, char * ip_address, int port) {
    // enable socket
    inet_socket->enabled = 1;
    inet_socket->port = port;

    // copy ip_address to socket_info object
    inet_socket->ip_address = malloc(sizeof(char) * strlen(ip_address) + 1);
    strcpy(inet_socket->ip_address, ip_address);
    
    struct sockaddr_in server_address;
    
    // open server socket 
    if ((inet_socket->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error: opening server socket");
        return 1;
    }
    
    if (setsockopt(inet_socket->fd, 
                   SOL_SOCKET, 
                   SO_REUSEADDR, 
                   &inet_socket->enabled, 
                   sizeof(int)) < 0) {
        perror("error: cannot set socket options");
        return 1;
    }

    // initialize server_address (sin_zero) to 0
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET; // set socket type, INET sockets are bound to IP 
    server_address.sin_port = htons(port); // assign port number
    inet_aton(ip_address, &server_address.sin_addr); // assign socket ip address

    // bind socket information to socket handler
    if (bind(inet_socket->fd, (struct sockaddr *) & server_address, sizeof(server_address)) < 0) {
        perror("error: binding socket to address");
        return 1;
    }

    return 0;
} // init_inet_socket

int launch_server(socket_info * server, pthread_t * thread) {
    /// initialize multithreading objects
    pthread_mutex_init(&flight_map_mutex, NULL);

    if (pthread_create(thread, NULL, server_handler, server) == 0) {
        printf("server: started server on port %d\n", server->port);
        return 0;
    }

    return 1;
} // launch_server

void read_flight_map_file(char * file_name, map_t flight_map) {
    // read input data
    FILE * file;
    if (!(file = fopen(file_name, "ra+"))) {
        perror("error: cannot open input file"); 
        exit(1);
    }

    // read all lines from input
    size_t length = 0;
    ssize_t read;
    char * input;
    while ((read = getline(&input, &length, file)) != -1) {
        // get tokens from input
       	char * input_tokens = NULL;
	    char * flight = strtok_r(input, " ", &input_tokens);
        char * seats = strtok_r(NULL, "\n", &input_tokens);

        add_flight(flight_map, flight, seats);
    }
} // read_flight_map_file

int string_equal(char const * string, char const * other_string) {
    return strcmp(string, other_string) == 0; 
} // string_equal

void print_flight(char * flight, void * seats) {
    printf("%s %s\n", flight, seats);
}

char * process_flight_request(char * input, map_t flight_map) {
    // parse input for commands
    // for now we're just taking the direct command
    // split command string to get command and arguments

    char * input_tokens = NULL; // used to save tokens when splitting string  
    char * command = NULL; 

    if (!(command= strtok_r(input, " ", &input_tokens))) {
		return "error: cannot proccess server command"; 
    }

    if (string_equal(command, "QUERY")) {
		// get flight to query 
		char * flight = NULL; 
		if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
			return "error: cannot proccess flight to query";
		}

		char * seats;
		int result;
		printf("server: querying flight %s\n", flight);
		pthread_mutex_lock(&flight_map_mutex);
		// retrieve seats from map
		result = hashmap_get(flight_map, flight, (void**) &seats);
		pthread_mutex_unlock(&flight_map_mutex);

		if (result != MAP_OK) {
			return "error: query failed"; 
		}

		return seats;
    }	

    if (string_equal(command, "LIST")) {
        printf("server: listing flights\n");

        char *info = NULL;
		char * n = NULL; 

		if (!(n = strtok_r(NULL, " ", &input_tokens))) {
            info = (char*) hashmap_foreach(flight_map, &print_flight);
		}
        else {

            info = (char*) hashmap_foreach_n(flight_map, atoi(n), &print_flight);
        }
    
        return info;
    }

    if (string_equal(command, "RESERVE")) {
        char * flight = NULL;
        char * seats = NULL;
        char * info = (char*)malloc(sizeof(char) * 256); //holder for message

        if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
            return "error: cannot process flight to reserve";
        }

        if (!(seats = strtok_r(NULL, " ", &input_tokens))) {
            return "error: cannot process seats to reserve";
        }



        int result = reserve_flight(flight_map, flight, seats);
        
        switch (result) {
            case 0:
                 sprintf(info, "An error occured"); //Format text to be sent back
                break;
            case 1:
                 sprintf(info, "Reserved %s seats on flight %s", seats, flight); //Format text to be sent back
                break;
            case 2:
                 sprintf(info, "Flight is Full"); //Format text to be sent back
                break;

            default:
                 sprintf(info, "An error occured"); //Format text to be sent back
                
        }

        return info;

    }

    if (string_equal(command, "RETURN")) {
      char * flight = NULL;
      char * seats = NULL;
      char * info = malloc(sizeof(char) * 256);

      if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
        return "error: cannot process flight to return";
      }

      if (!(seats = strtok_r(NULL, " ", &input_tokens))) {
        return "error: cannot process seats to return"; }

    //TODO figure out how to retrieve value from hashmap
    //
    int result = reserve_flight(flight_map, flight, seats);

    switch (result) {
        case 0:
            sprintf(info, "An error occured"); //Format text to be sent back
            break;
        case 1:
            sprintf(info, "Reserved %s seats on flight %s", seats, flight); //Format text to be sent back
            break;
        case 2:
            sprintf(info, "Flight is Full"); //Format text to be sent back
            break;

        default:
            sprintf(info, "An error occured"); //Format text to be sent back

    }

    return info;

    }


    return "error: cannot recognize command"; 
} // process_flight_request
