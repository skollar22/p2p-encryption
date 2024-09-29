#include "main.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// // misc posix stuff
// #include <unistd.h>

// // error handling
// #include <errno.h>

// // types library
// #include <sys/types.h>

// // sockets library
// #include <sys/socket.h>

// // net library??
// #include <netdb.h>

// #include <arpa/inet.h>

// #include <netinet/in.h>

// #include <sys/wait.h>

// #include <signal.h>



int main(int argc, char** argv) {

    // variables for getting address info
    struct addrinfo hints, *servinfo, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    // socket variables
    int sockfd;

    // set hints to be empty, then fill in vars
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        // get the socket and error check
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
        }

        // something about reusing ports that zombie sockets are holding
        int yes=1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // try to bind the socket to the port - might not be necessary, but doing it anyway
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("waiting for connections...\n");

    while(getchar() != EOF) {
        
    }

    close(sockfd);

    return 0;
}