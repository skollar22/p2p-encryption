#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// misc posix stuff
#include <unistd.h>

// error handling
#include <errno.h>

// types library
#include <sys/types.h>

// sockets library
#include <sys/socket.h>

// net library??
#include <netdb.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/wait.h>

#include <signal.h>

#define PORT "5678"

#define BACKLOG 10

// bignum

typedef struct bignum {
    unsigned int   size;   // number of bytes in data
    unsigned char* data;   // actual number
    unsigned char  sign;   // sign (0 means pos, otherwise neg)

    
} *bignum_t;

// bignum functions
bignum_t b_sub(bignum_t a, bignum_t b);
bignum_t b_add(bignum_t a, bignum_t b);
void b_print(bignum_t a);
bignum_t b_trim(bignum_t a);
int b_comp(bignum_t a, bignum_t b);