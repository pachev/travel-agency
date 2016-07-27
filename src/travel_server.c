/// file travel_server.c
/// Travel agent server application
/// https://gitlab.com/openhid/travel-agency

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>

/// @brief error handler
/// prints error and exits with error status
void error(char * const message) {
    perror(message);
    exit(1);
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

    // socket handlers
    int server_fd, client_fd;

    // socket address information 
    struct sockaddr_in server_address, client_address; 

    // open server socket 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("error: opening server socket");
    
    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error("error: cannot set socket options");

    // initialize server_address (sin_zero) to 0
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET; // set socket type, INET sockets are bound to IP 
    server_address.sin_port = htons(start_port); // assign port number
    inet_aton(ip_address, &server_address.sin_addr); // assign socket ip address

    // bind socket information to socket handler
    if (bind(server_fd, (struct sockaddr *) & server_address, sizeof(server_address)) < 0)
        error("error: binding socket to address");
    
    // maximum number of input connections queued at a time
    int const MAX_CLIENTS = 10;

    // listen for connections on sockets
    if (listen(server_fd, MAX_CLIENTS) < 0)
        error("error: listening to connections");

    printf("listening to connection %s:%d\n", ip_address, start_port);
    
    // accept incoming connection from client
    socklen_t client_length = sizeof(client_address);
    if ((client_fd = accept(server_fd, (struct sockaddr *) & client_address, &client_length)) < 0)
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
    char buffer[BUFFER_SIZE];

    // read data sent from socket into buffer
    if (read(client_fd, buffer, sizeof(buffer)) < 0)
        error("error: reading from connection");

    printf("server: read %s from client\n", buffer);

    // write to connection with reply
    if (write(client_fd, buffer, sizeof(buffer)) < 0) 
        error("error: writing to connection");

    printf("server: sent %s to client\n", buffer);

    // close sockets
    close(client_fd);
    close(server_fd);

    return 0; 
}
