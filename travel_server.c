/// https://gitlab.com/openhid/travel-agency

#define _GNU_SOURCE

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
#include "travel_server.h"

#define MAX_SEATS 40
#define MAX_CLIENTS 8



/// @brief socket server handlers 
/// used as thread args
socket_info * servers;
int no_ports = 0;


int main (int argc, char * argv[]) {
    //
    /// initialize multithreading objects
    pthread_mutex_init(&flight_map_mutex, NULL);
    no_ports = atoi(argv[3]);
    // if not enough arguments
    if (argc < 6 ) {
        error("usage: talent_server ip_address start_port no_ports in_file out_file\n");
    }

    if(no_ports > MAX_CLIENTS) {
        printf("Trying to open too many ports\n");
        exit(1);
    }

    printf("no_ports: %d",atoi(argv[3]));

    // read command arguments
    char * ip_address = argv[1];
    int start_port = atoi(argv[2]);
    char * in_filename = argv[4];
    char * out_filename = argv[5]; 

    servers = (socket_info *) malloc(sizeof(socket_info) * no_ports);

    // initialize flight map
    // Holds flight/seat pairs of clients
    map_t flight_map = hashmap_new(); 
    map_t user_map = hashmap_new(); 

    read_flight_map_file(in_filename, flight_map);


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

        printf("\nenter command: ");
        fgets(input, sizeof(input), stdin);

        if (strstr(input, "LIST")) {
            printf("%s\n", list_all_users());
        }
        if (strstr(input, "LIST_CHAT")){
            printf("%s\n", list_chat_users());
        } 
        if(strstr(input, "WRITE"))
            write_flight_map_file(out_filename, flight_map);

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
    hashmap_free(user_map);
    free(servers);

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

    int curr_seat_value = 0;
    char *checking;
    char * flight = malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = malloc(sizeof(char) * strlen(seats_token) + 1);
    char * new_seats = malloc(sizeof(char) * strlen(seats_token) + 1);


    int map_find_result = 0; 
    //
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);

    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    map_find_result = hashmap_get(flight_map, flight, (void **)&checking); 
    pthread_mutex_unlock(&flight_map_mutex);

    if(map_find_result == MAP_OK) {

        curr_seat_value = atoi(checking);
        curr_seat_value -= atoi(seats);
        if(curr_seat_value < 0)
            return 2;

        sprintf(new_seats, "%d", curr_seat_value);

        pthread_mutex_lock(&flight_map_mutex);
        assert(hashmap_remove(flight_map, flight) == MAP_OK);
        assert(hashmap_put(flight_map, flight, new_seats) == MAP_OK);
        pthread_mutex_unlock(&flight_map_mutex);


        printf("Reserved %s seats  on flight %s. Updated Seats: %d   Retrieved: %d\n", seats, flight, curr_seat_value, atoi(checking));
        return 1;

    }
    else 
        return 0;
}


int return_flight(map_t flight_map, char * flight_token, char * seats_token) {
    //0 is not found, 1 is ok , 2 is full 

    int curr_seat_value = 0;
    char *checking;
    char * flight = malloc(sizeof(char) * strlen(flight_token) + 1);
    char * seats = malloc(sizeof(char) * strlen(seats_token) + 1);
    char * new_seats = malloc(sizeof(char) * strlen(seats_token) + 1);


    int map_find_result = 0; 
    //
    // copy temp strings into memory
    strcpy(flight, flight_token);
    strcpy(seats, seats_token);

    // acquire lock to flight_map
    pthread_mutex_lock(&flight_map_mutex);
    map_find_result = hashmap_get(flight_map, flight, (void **)&checking); 
    pthread_mutex_unlock(&flight_map_mutex);

    if(map_find_result == MAP_OK) {

        curr_seat_value = atoi(checking);
        curr_seat_value += atoi(seats);
        if(curr_seat_value  > MAX_SEATS)
            return 2;

        sprintf(new_seats, "%d", curr_seat_value);

        pthread_mutex_lock(&flight_map_mutex);
        assert(hashmap_remove(flight_map, flight) == MAP_OK);
        assert(hashmap_put(flight_map, flight, new_seats) == MAP_OK);
        pthread_mutex_unlock(&flight_map_mutex);


        printf("server: Reserved %s on flight %s. Updated Seats: %d   Retrieved: %d\n", seats, flight, curr_seat_value, atoi(checking));
        return 1;

    }
    else 
        return 0;
}

void * hashmap_foreach(map_t flight_map, void(* callback)(char *, void *, void *)) {

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
            callback(map->data[index].key, map->data[index].data, (void*) info);
        }
    }

    return(void*) info;
}

void * hashmap_foreach_n(map_t flight_map, int n, void(* callback)(char *, void *, void *)) {

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

    int count = 0;

    struct hashmap_map * map = (struct hashmap_map *) flight_map;

    // iterate each hashmap, invoke callback if index in us
    for (int i = 0; i < map->table_size; i++) {
        if (map->data[i].in_use != 0) {
            count++;
            callback(map->data[i].key, map->data[i].data, (void*)info);
            if(count >=n)
                break;
        }
    }

    return (void*) info; //keep track of all info in hash map

} // hashmap_foreach_n to iterate over first n elements



void free_flight(char * key, void * data, void * info) {
    free(key);
    free(data);
}

void  free_flight_map_data(map_t flight_map) {
    hashmap_foreach(flight_map, &free_flight);
} // free_flight_map_data

void broadcast_message (char * arg) {

    printf("Broadcasting Message\n");

    for (int i = 0; i < no_ports; i++) {

        pthread_mutex_lock(&flight_map_mutex);
        if(servers[i].chatmode) 
            write(servers[i].clientfd, arg, strlen(arg) + 1);
        pthread_mutex_unlock(&flight_map_mutex);
    }

}

void * server_handler(void * handler_args) {
    // retrieve socket_info from args
    socket_info * inet_socket = (socket_info *) handler_args;

    // listen for connections on sockets 
    if (listen(inet_socket->fd, MAX_CLIENTS) < 0) {
        error("error: listening to connections");
    }
    printf("\n%d: listening on %s:%d\n", inet_socket->port, inet_socket->ip_address, inet_socket->port);


    while (inet_socket->enabled) {
        // socket address information
        struct sockaddr_in client_address;
        char * current_data = (char*) malloc(sizeof(char) * 256);
        char * input_tokens = NULL;
        char * message = NULL;
        char * chat_response = NULL;

        // accept incoming connection from client
        socklen_t client_length = sizeof(client_address);
        if ((inet_socket->clientfd = accept(inet_socket->fd, (struct sockaddr *) &client_address, &client_length)) < 0) {
            error("error: accepting connection");
        }

        // get incoming connection information
        struct sockaddr_in * incoming_address = (struct sockaddr_in *) &client_address;
        char incoming_ip_address[INET_ADDRSTRLEN]; // holds ip address string
        // get ip address string from address object
        inet_ntop(AF_INET, &incoming_address->sin_addr, incoming_ip_address, sizeof(incoming_ip_address));

        // holds incoming data from socket
        int const BUFFER_SIZE = 256;
        char input[BUFFER_SIZE];

        // read data sent from socket into input
        if (read(inet_socket->clientfd, input, sizeof(input)) < 0) {
            error("error: reading from connection");
        }

        printf("%d: read %s from client\n", inet_socket->port, input);

        if(inet_socket->chatmode)
            current_data = process_chat_request(input, inet_socket); //process chat commands
        else
            current_data = process_flight_request(input, inet_socket); //process commands



        // exit command breaks loop
        if (string_equal(current_data, "EXIT")) {
            break;
        }
        //
        //TODO: Figure how to extract broadcasting from string
        //

        if(inet_socket->broadcasting) {

            chat_response = strtok_r(current_data, "~", &input_tokens);
            message = strtok_r(NULL, "\0", &input_tokens);

            if (string_equal(chat_response, "Broadcast")) {
                broadcast_message(message);

            }
            // Echoing back
            if (write(inet_socket->clientfd, message, strlen(message) + 1) < 0) {
                error("error: writing to connection");
            }

            close(inet_socket->clientfd);

            inet_socket->broadcasting = false;

        }
        else {

            // write to connection with reply
            if (write(inet_socket->clientfd, current_data, strlen(current_data) + 1) < 0) {
                error("error: writing to connection");
            }

            printf("%d: sent response to client\n", inet_socket->port);

            // close socket
            close(inet_socket->clientfd);

        }
    }

    // close server socket
    close(inet_socket->fd);
    inet_socket->c_u->loggedon = false;
    inet_socket->chatmode= false;
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

    //handle ClientUser
    inet_socket->c_u = ClientUser_new();
    inet_socket->chatmode = false;
    inet_socket->broadcasting = false;


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

void write_flight_map_file(char * file_name, map_t flight_map) {
    // read input data
    FILE * file;
    char * formatted = (char*)malloc(sizeof(char) * 4096);
    if (!(file = fopen(file_name, "w"))) {
        perror("error: cannot open outpute file"); 
    }

    formatted = (char*) hashmap_foreach(flight_map, &print_flight);

    fprintf(file, "%s\n", formatted);

    printf("Saving to file %s\n",file_name);

    fclose(file);

    free(formatted);

} // write_flight_map_file

int string_equal(char const * string, char const * other_string) {
    return strcmp(string, other_string) == 0; 
} // string_equal

void print_flight(char * flight, void * seats, void * args) {
    printf("%s %s\n", flight, seats);
    sprintf((args + strlen((char *) args)),"%s %s\n", flight, seats);
}

void print_avail_flight(char * flight, void * seats, void * args) {
    if(atoi(seats)) {
        printf("%s %d\n", flight, atoi(seats));
        sprintf((args + strlen((char *) args)),"%s %s\n", flight, seats);
    }
    else
        return;
}

char * process_chat_request(char * input, socket_info * soc_info) {
    printf("====================\nChat Room\n======================\n");
    char * input_tokens = NULL; // used to save tokens when splitting string  
    char * command = NULL; 
    char * message = NULL; 

    ClientUser * c_u = soc_info->c_u; 

    if (!(command= strtok_r(input, " ", &input_tokens))) {
        return "error: cannot proccess chat command"; 
    }

    if (string_equal(command, "TEXT")) {
        soc_info->broadcasting = true;

        char * info = malloc(sizeof(char) * 256); 

        if (!(message= strtok_r(NULL, "\0", &input_tokens))) {

            return "Please enter a message";
        }

        sprintf(info, "Broadcast~ %s: %s", c_u->username, message);

        return info;

    }

    if (string_equal(command, "LIST_ALL")) {

        char * info = list_all_users();
        return info;
    }

    if (string_equal(command, "LIST_OFFLINE")) {

        char * info = malloc(sizeof(char) * 256); 
        int count = 0;

        printf("*** Offline Users ***\n");
        sprintf(info, "*** Offline Users ***\n");
        //
        //TODO: move this to its own function
        //
        for (int i = 0; i < no_ports; i++) {

            pthread_mutex_lock(&flight_map_mutex);
            if(!servers[i].chatmode && servers[i].c_u->loggedon){
                sprintf(info + strlen(info), "%s\n", servers[i].c_u->username);
                count++;
            }
            pthread_mutex_unlock(&flight_map_mutex);
        }

        sprintf(info + strlen(info), "Total Users: %d\n", count);

        return info;
    }


    if (string_equal(command, "LIST")) {

        char * info = list_chat_users();
        return info;
    }


    if (string_equal(command, "EXIT")) {


        if (!(message= strtok_r(NULL, " ", &input_tokens))) {

            return "Please enter a valid command";
        }

        if(string_equal(message, "CHAT")) {

            soc_info->chatmode = false;
            return "Thanks for chatting";

        }

        return "Something went wrong";

    }

    if (string_equal(command, "EXIT")) {


        if (!(message= strtok_r(NULL, " ", &input_tokens))) {

            return "Please enter a valid command";
        }

        if(string_equal(message, "CHAT")) {

            soc_info->chatmode = false;
            return "Thanks for chatting";

        }

        return "Something went wrong";

    }



    return "Command Not Recognized";
}
char * process_flight_request(char * input, socket_info * soc_info) {
    // parse input for commands
    // for now we're just taking the direct command
    // split command string to get command and arguments

    char * input_tokens = NULL; // used to save tokens when splitting string  
    char * command = NULL; 
    char * username = NULL; 
    map_t flight_map = soc_info->map;
    ClientUser * c_u = soc_info->c_u; 

    if (!(command= strtok_r(input, " ", &input_tokens))) {
        return "error: cannot proccess server command"; 
    }


    if (string_equal(command, "LOGON")) {
        if(c_u->loggedon)
            return "Already logged on";
        else {

            char * info = malloc(sizeof(char) * 100); 


            if (!(username = strtok_r(NULL, " ", &input_tokens))) {
                return "Please enter a username ";
            }

            for (int i = 0; i < no_ports; i++) {

                pthread_mutex_lock(&flight_map_mutex);
                if(strstr(servers[i].c_u->username, username)){
                    c_u->unique_num ++;
                }
                pthread_mutex_unlock(&flight_map_mutex);

            }
            if(c_u->unique_num > 0)
                sprintf(username, "%s%d",username, c_u->unique_num);

            strcpy(c_u->username, username);
            c_u->loggedon = true;
            sprintf(info, "Sucessfully logged on as %s", c_u->username);

            return info;

        }

    }

    if (string_equal(command, "ENTER")) {
        if(!c_u->loggedon)
            return "You need to be looged on to perform this action";

        char * info = malloc(sizeof(char) * 100); 

        if (!(info = strtok_r(NULL, " ", &input_tokens))) {
            return "Command Unavailable";
        }

        if(string_equal(info, "CHAT")) {

            soc_info->chatmode = true;
            return "****** Welcome to the chat *******";

        }


        return "Something went wrong";


    }

    if (string_equal(command, "QUERY")) {

        if(!c_u->loggedon)
            return "You need to be looged on to perform this action";

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

        if(!c_u->loggedon)
            return "You need to be looged on to perform this action";

        printf("server: listing flights\n");

        char *info = NULL;
        char *n = strtok_r(NULL, " ", &input_tokens);

        if (!(n)) {
            info = (char*) hashmap_foreach(flight_map, &print_flight);
        }
        else {

            info = (char*) hashmap_foreach_n(flight_map, atoi(n), &print_flight);
        }

        return info;
    }

    if (string_equal(command, "LIST_AVAILABLE") || string_equal(command, "L_A")) {

        if(!c_u->loggedon)
            return "You need to be looged on to perform this action";

        printf("server: listing flights\n");

        char *info = NULL;
        char * n = NULL; 

        if (!(n = strtok_r(NULL, " ", &input_tokens))) {
            info = (char*) hashmap_foreach(flight_map, &print_avail_flight);
        }
        else {

            info = (char*) hashmap_foreach_n(flight_map, atoi(n), &print_avail_flight);
        }

        return info;
    }

    if (string_equal(command, "RESERVE")) {

        if(!c_u->loggedon)
            return "You need to be looged on to perform this action";


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

        if(!c_u->loggedon)
            return "You need to be looged on to perform this action";

        char * flight = NULL;
        char * seats = NULL;
        char * info = malloc(sizeof(char) * 256);

        if (!(flight = strtok_r(NULL, " ", &input_tokens))) {
            return "error: cannot process flight to return";
        }

        if (!(seats = strtok_r(NULL, " ", &input_tokens))) {
            return "error: cannot process seats to return"; }

        int result = return_flight(flight_map, flight, seats);

        switch (result) {
            case 0:
                sprintf(info, "An error occured"); //Format text to be sent back
                break;
            case 1:
                sprintf(info, "Returned %s seats on flight %s", seats, flight); //Format text to be sent back
                break;
            case 2:
                sprintf(info, "Returning Too many flights, stop trying to cheat the system"); //Format text to be sent back
                break;

            default:
                sprintf(info, "An error occured"); //Format text to be sent back

        }

        return info;

    }

    if (string_equal(command, "LOGOFF")) {

        soc_info->c_u->loggedon = false;
        soc_info->chatmode= false;

        return "EXIT";

    }

    if (string_equal(command, "EXIT"))
        return "EXIT";



    return "error: cannot recognize command"; 
} // process_flight_request

ClientUser* ClientUser_new() {
    ClientUser *c = (ClientUser *) malloc(sizeof(ClientUser));
    if(!c)
        return NULL;

    c->unique_num = 0;
    c->loggedon = false;
    c->in_chat = false;
    c->username = (char*) malloc(sizeof(char) * 100);


    return c;

}

char * list_all_users () {
    char * info = malloc(sizeof(char) * 256); 
    int count = 0;

    printf("*** ALL USERS ***\n");
    sprintf(info, "*** ALL USERS ***\n");

    for (int i = 0; i < no_ports; i++) {

        pthread_mutex_lock(&flight_map_mutex);
        if(servers[i].c_u->loggedon){
            if(servers[i].chatmode)
                sprintf(info + strlen(info), "%s - online\n", servers[i].c_u->username);
            else
                sprintf(info + strlen(info), "%s\n", servers[i].c_u->username);
            count++;
        }
        pthread_mutex_unlock(&flight_map_mutex);
    }

    sprintf(info + strlen(info), "Total Users: %d\n", count);

    return info;

}


char * list_chat_users() {

    char * info = malloc(sizeof(char) * 256); 
    int count = 0;

    printf("Listing Users in Chat \n");
    sprintf(info, "*** Users in Chat ***\n");

    for (int i = 0; i < no_ports; i++) {

        pthread_mutex_lock(&flight_map_mutex);
        if(servers[i].chatmode){
            sprintf(info + strlen(info), "%s\n", servers[i].c_u->username);
            count++;
        }
        pthread_mutex_unlock(&flight_map_mutex);
    }

    sprintf(info + strlen(info), "Total online: %d\n", count);

    return info;
}
