#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include "c_hashmap/hashmap.h"

/// prints error buffer and exits
/// with error status
void error(char const * buffer) {
    perror(buffer);
    exit(0);
}

int main(int argc, char * argv[]) {
    // check number of arguments
    if (argc < 3)
        error("usage: travel_client ip_address\n");
     
    // assign command line arguments
    char * ip_address = argv[1];
    int port = atoi(argv[2]);
    
    // socket handler
    int client_fd;

    // client socket address info
    struct sockaddr_in server_address;

    // stores host information
    struct hostent * server;

    while (1) {
        // create a new socket stream
        if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            error("error: opening client socket");

        int enable = 1;
        if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
            error("error: cannot set socket options");

        if ((server = gethostbyname(ip_address)) == NULL) 
            error("error: no host at ip addresss");

        // initialize server_address object
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET; // set socket type
        server_address.sin_port = htons(port); // set port number
        inet_aton(ip_address, &server_address.sin_addr); // set socket ip address

        // connect socket to server address 
        if (connect(client_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
            error("error: connecting to server");

        // stores incoming data from server
        int const BUFFER_SIZE = 256;
        char buffer[BUFFER_SIZE];

        // initialize buffer for input
        memset(&buffer, 0, sizeof(buffer));
        
        // read input from terminal to be sent
        // prompt user
        printf("\n>>> ");
        fgets(buffer, sizeof(buffer), stdin);

        char * command_token = NULL;
        char * command = strtok_r(buffer, "\n", &command_token); // remove \n newline from buffer

        // if log off in command
        if (strstr(command, "LOGOFF")) {
            printf("\nclosing client\n");
            close(client_fd);
            break;
        }

        // send input to server
        if (write(client_fd, command, strlen(command) + 1) < 0)
            error("error: writing to socket");
        

        // recieve response from server
        if (read(client_fd, buffer, sizeof(buffer)) < 0)
            error("error: reading from socket");

        printf("%s", buffer);

        close(client_fd);
    }

    return 0;
} 
