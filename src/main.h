#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

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

// creation and deletion
bignum_t b_gen(unsigned int size); // non-functional yet
bignum_t b_init(unsigned int size);
bignum_t b_initv(int initial);
void b_free(bignum_t a);
bignum_t b_copy(bignum_t a);
// bignum_t b_create_local_copy(bignum_t a);
// bignum_t b_create_local(unsigned int size);

// utility
unsigned int b_bytes(bignum_t a);
unsigned char b_msb(bignum_t a);
unsigned int b_overflow(unsigned char a, unsigned char b, unsigned char carry);
bignum_t b_largest(bignum_t a, bignum_t b);
bignum_t b_smallest(bignum_t a, bignum_t b);
unsigned int b_smallest_bytes(bignum_t a, bignum_t b);
unsigned int b_largest_bytes(bignum_t a, bignum_t b);
int b_comp(bignum_t a, bignum_t b);
void b_print(bignum_t a);
void b_prints(bignum_t a);

// manipulation
void b_trim(bignum_t a);
void b_pad(bignum_t a, unsigned int size);

// logical operations
void b_oriip(bignum_t a, unsigned char val);
unsigned char b_andi(bignum_t a, unsigned char val);
bignum_t b_lshift(bignum_t a, unsigned int shift);
void b_lsip(bignum_t a, unsigned int shift);
bignum_t b_rshift(bignum_t a, unsigned int shift);
void b_rsip(bignum_t a, unsigned int shift);

// arithmetic operations
bignum_t b_add(bignum_t a, bignum_t b);
bignum_t b_sub(bignum_t a, bignum_t b);
bignum_t b_mul(bignum_t a, bignum_t b);
bignum_t b_div(bignum_t n, bignum_t d, bignum_t *r);
bignum_t b_mod(bignum_t a, bignum_t m);
void b_modip(bignum_t *a, bignum_t m);
bignum_t b_exp(bignum_t b, bignum_t e);
bignum_t b_mexp(bignum_t b, bignum_t e, bignum_t m);

// conversions
int b_toi(bignum_t a);
char* b_tostr(bignum_t a);


