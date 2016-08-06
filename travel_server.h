
#ifndef __travel_agency__
#define __travel_agency__

#include <assert.h>
#include <stdbool.h>
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

/// @brief contains socket information
/// includes file descriptor port pair.
typedef struct {
    bool loggedon;
    char * username;
    int unique_num;
    bool in_chat;
    

} ClientUser;

typedef struct {
    int fd;
    bool broadcasting;
    int port;
    char * ip_address;
    bool chatmode;
    int enabled;
    map_t * map;
    int clientfd;
    ClientUser * c_u;
} socket_info;
//
// Synchronizes access to flight_map
pthread_mutex_t flight_map_mutex;


char * process_chat_request(char * input, socket_info * soc_info);


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
char * process_flight_request(char * command, socket_info * soc_info);

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
void * hashmap_foreach(map_t map, void(* callback)(char * key, void * data, void * args));

void * hashmap_foreach_n(map_t map, int n,  void(* callback)(char * key, void * data, void * args));

/// @brief error handler
/// prints error and exits with error status
void error(char * const message) {
    perror(message);
    pthread_exit(NULL);
}

ClientUser *ClientUser_new (); 

char * list_all_users ();
char * list_chat_users ();



#endif /* defined(__lec2__StringList__) */
