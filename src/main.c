// #include "main.h"


// for use on WSL
#define _POSIX_C_SOURCE 200112L

#include "main.h"

#define BIT_LENGTH 32


/**
 * Old networking utility function
 */
// void *get_in_addr(struct sockaddr *sa)
// {
//     if (sa->sa_family == AF_INET) {
//         return &(((struct sockaddr_in*)sa)->sin_addr);
//     }

//     return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }


int main(int argc, char** argv) {

    if (argc < 2) return -1;

    if (strcmp(argv[1], "encrypt") == 0) {

        printf("Please enter a string: ");
        char buffer[512];
        int i = 0;
        for (; (buffer[i]=getchar()) != '\n'; i++) ; ;
        buffer[i] = '\0';

        unsigned int blocks;

        char *cypher = encrypt(buffer, &blocks);

        printf("Encrypted String: ");
        printhex(cypher, blocks * KEY_LENGTH);
        printf("\n");

        return 0;
    } else if (strcmp(argv[1], "decrypt") == 0) {

        printf("Please enter a string: ");
        char buffer[2048];
        int i = 0;
        for (; (buffer[i]=getchar()) != '\n'; i++) ; ;
        buffer[i] = '\0';

        unsigned int blocks = i / 128;

        char *ciphertext = hextochar(buffer, i);

        char *dec = decrypt(ciphertext, blocks);

        unsigned int length_dec = strlength(dec);

        printf("decrypted string : %s\n", dec);

        return 0;
    } else if (strcmp(argv[1], "keygen") == 0) {
        keygen();
        return 0;
    } else if (strcmp(argv[1], "prime") == 0) {
        bignum_t temp = NULL;

        clock_t s = clock();

        printf("working...\n");

        // can be run to generate many primes at once
        for (int i = 0; i < 1; i++) {
            temp = b_gen_prime(BIT_LENGTH);
            char *str = b_tohex(temp);
            printf("Prime = %s (in hex)\n\n", str);
            free(str);
            b_free(temp); temp = NULL;
        }
        b_free(temp);

        double time_taken = (double) (clock() - s) / CLOCKS_PER_SEC;

        /**
         * Stats for different function call times and call numbers
         * Some might be disabled, but feel free to uncomment anything you want
         */
        printf("STATS\n");
        printf("Total time taken: %f seconds\n\n", time_taken);
        // printf("madd:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", madd_calls, madd_time, (double)(madd_time * 1000000 / madd_calls));
        printf("mmul:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", mmul_calls, mmul_time, (double)(mmul_time * 1000000 / mmul_calls));
        printf("modip:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", modip_calls, modip_time, (double)(modip_time * 1000000 / modip_calls));
        // printf("subip:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", subip_calls, subip_time, (double)(subip_time * 1000000 / subip_calls));
        // printf("msub:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", msub_calls, msub_time, (double)(msub_time * 1000000 / msub_calls));
        // printf("mexp:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", mexp_calls, mexp_time, (double)(mexp_time * 1000000 / mexp_calls));
        // printf("mlsip:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", mlsip_calls, mlsip_time, (double)(mlsip_time * 1000000 / mlsip_calls));
        printf("fftmul:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", fftmul_calls, fftmul_time, (double)(fftmul_time * 1000000 / fftmul_calls));
        return 0;
    } else {
        printf("Usage: ./bn <arg>\n");
        return 1;
    }

    return 1;

    // Old networking code
    // from https://beej.us/guide/bgnet

    // // variables for getting address info
    // struct addrinfo hints, *servinfo, *p;
    // int status;                                 // for printing errors
    // char ipstr[INET6_ADDRSTRLEN];               // unused?

    // // socket variables
    // int sockfd;

    // // set hints to be empty, then fill in vars
    // memset(&hints, 0, sizeof hints);
    // hints.ai_family = AF_UNSPEC;
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;

    // char *hname = malloc(sizeof(char) * 512);
    // gethostname(hname, 512);
    // printf("host name = %s\n", hname);
    // free(hname);

    // // get address info (for current address, and #define'd port)
    // if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    //     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    //     return 1;
    // }

    // // find a socket we can use from the address list
    // for (p = servinfo; p != NULL; p = p->ai_next) {

    //     void *addr;
    //     char *ipver;

    //     // get the pointer to the address itself,
    //     // different fields in IPv4 and IPv6:
    //     if (p->ai_family == AF_INET) { // IPv4
    //         struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    //         addr = &(ipv4->sin_addr);
    //         ipver = "IPv4";
    //     } else { // IPv6
    //         struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
    //         addr = &(ipv6->sin6_addr);
    //         ipver = "IPv6";
    //     }

    //     // convert the IP to a string and print it:
    //     inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    //     printf("  %s: %s\n", ipver, ipstr);

    //     // get the socket and error check
    //     if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
    //         perror("server: socket");
    //     }

    //     // something about reusing ports that zombie sockets are holding
    //     int yes=1;
    //     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    //         perror("setsockopt");
    //         exit(1);
    //     }

    //     // try to bind the socket to the port - might not be necessary, but doing it anyway
    //     if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
    //         close(sockfd);
    //         perror("server: bind");
    //         continue;
    //     }

    //     break;
    // }

    // // free the list of addresses, since we have a bound socket now
    // freeaddrinfo(servinfo);

    // // actually check if we have a bound socket now
    // if (p == NULL) {
    //     fprintf(stderr, "server: failed to bind\n");
    // }

    // // listen on the socket we have
    // if (listen(sockfd, BACKLOG) == -1) {
    //     perror("listen");
    //     exit(1);
    // }

    // printf("waiting for connections...\n");

    // socklen_t sin_size;
    // struct sockaddr_storage their_addr;
    // int new_fd;
    // char s[INET6_ADDRSTRLEN];

    // while(1) {  // main accept() loop
    //     sin_size = sizeof their_addr;
    //     new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    //     if (new_fd == -1) {
    //         perror("accept");
    //         continue;
    //     }

    //     inet_ntop(their_addr.ss_family,
    //         get_in_addr((struct sockaddr *)&their_addr),
    //         s, sizeof s);
    //     printf("server: got connection from %s\n", s);

    //     if (!fork()) { // this is the child process
    //         close(sockfd); // child doesn't need the listener
    //         if (send(new_fd, "Hello, world!", 13, 0) == -1)
    //             perror("send");
    //         close(new_fd);
    //         exit(0);
    //     }
    //     close(new_fd);  // parent doesn't need this
    // }

    // // temporary, for testing
    // while(getchar() != EOF) {

    // }

    // // close the socket since we are exiting
    // close(sockfd);

    // return 0;
}