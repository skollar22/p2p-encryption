// #include "main.h"


// for use on WSL
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// net library??
#include <netdb.h>

// misc posix stuff
#include <unistd.h>

// error handling
#include <errno.h>

// types library
#include <sys/types.h>

// sockets library
#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/wait.h>

#include <signal.h>

#define PORT "5678"

#define BACKLOG 10

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char** argv) {

    // variables for getting address info
    struct addrinfo hints, *servinfo, *p;
    int status;                                 // for printing errors
    char ipstr[INET6_ADDRSTRLEN];               // unused?

    // socket variables
    int sockfd;

    // set hints to be empty, then fill in vars
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char *hname = malloc(sizeof(char) * 512);
    gethostname(hname, 512);
    printf("host name = %s\n", hname);
    free(hname);

    // get address info (for current address, and #define'd port)
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // find a socket we can use from the address list
    for (p = servinfo; p != NULL; p = p->ai_next) {

        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);

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

    // free the list of addresses, since we have a bound socket now
    freeaddrinfo(servinfo);

    // actually check if we have a bound socket now
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
    }

    // listen on the socket we have
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("waiting for connections...\n");

    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    int new_fd;
    char s[INET6_ADDRSTRLEN];

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    // temporary, for testing
    while(getchar() != EOF) {

    }

    // close the socket since we are exiting
    close(sockfd);

    return 0;
}