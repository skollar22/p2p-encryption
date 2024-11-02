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

// RSA
#define PRIME_LENGTH 32
#define KEY_LENGTH 64
#define BLOCK_LEN 63


extern double madd_time;
extern long long madd_calls;

extern double mmul_time;
extern long long mmul_calls;

extern double subip_time;
extern long long subip_calls;

extern double msub_time;
extern long long msub_calls;

extern double comp_time;
extern long long comp_calls;

extern double mexp_time;
extern long long mexp_calls;

extern double modip_time;
extern long long modip_calls;

extern double mlsip_time;
extern long long mlsip_calls;

extern double fftmul_time;
extern long long fftmul_calls;


// bignum

typedef struct bignum {
    unsigned int   size;   // number of bytes in data
    unsigned char* data;   // actual number
    unsigned char  sign;   // sign (0 means pos, otherwise neg)

    
} *bignum_t;


// complex bignum

typedef struct bignum_complex {
    long double real;
    long double cplx;
} *bncplx_t;

typedef struct bignum_comp_chain {
    bncplx_t *values;
    unsigned int length;
} *bncomp_t;

// bignum functions

// creation and deletion


bignum_t b_gen(unsigned int size); // non-functional yet
bignum_t b_init(unsigned int size);
bignum_t b_initv(int initial);
bignum_t b_initc(unsigned char initial);
bignum_t b_initl(long long initial);
void b_free(bignum_t a);
bignum_t b_copy(bignum_t a);
bignum_t b_acopy(bignum_t a);
bignum_t b_fromstr(char* str, unsigned int n);
bignum_t b_fromhex(char *str, unsigned int n);
unsigned char num_from_hex(unsigned char hexval);
bignum_t b_minus(bignum_t a);
bignum_t b_mmi(bignum_t a, bignum_t b);
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
bignum_t b_ori(bignum_t a, unsigned char val);
void b_oriip(bignum_t a, unsigned char val);
unsigned char b_andi(bignum_t a, unsigned char val);
bignum_t b_lshift(bignum_t a, unsigned int shift);
void b_lsip(bignum_t a, unsigned int shift);
void b_mlsip(bignum_t a, unsigned int shift, bignum_t n);
bignum_t b_rshift(bignum_t a, unsigned int shift);
void b_rsip(bignum_t a, unsigned int shift);

// arithmetic operations
bignum_t b_add(bignum_t a, bignum_t b);
void b_madd(bignum_t a, bignum_t b, bignum_t n);
void b_addip(bignum_t a, bignum_t b);

bignum_t b_sub(bignum_t a, bignum_t b);
void b_subip(bignum_t a, bignum_t b);
bignum_t b_msub(bignum_t a, bignum_t b, bignum_t n);

bignum_t b_mul(bignum_t a, bignum_t b);
void b_mmul(bignum_t a, bignum_t b, bignum_t n);

bignum_t b_div(bignum_t n, bignum_t d, bignum_t *r);

bignum_t b_mod(bignum_t a, bignum_t m);
void b_modip(bignum_t a, bignum_t m);

bignum_t b_exp(bignum_t b, bignum_t e);
bignum_t b_mexp(bignum_t b, bignum_t e, bignum_t m);

// conversions
int b_toi(bignum_t a);
char* b_tostr(bignum_t a);
char* b_tohex(bignum_t a);

// prime stuff
int b_is_prime(bignum_t n, bignum_t n_minus);
bignum_t b_gen_prime(unsigned int size);

// complex
bncplx_t c_construct(long double real, long double cplx);
bncomp_t c_allocarr(unsigned int n);
bncplx_t c_copy(bncplx_t a);
void c_free(bncomp_t a);
void c_freec(bncplx_t a);

void c_print(bncplx_t a, const char *name);

bncplx_t c_mul(bncplx_t a, bncplx_t b);
void c_mulip(bncplx_t a, bncplx_t b);
bncplx_t c_sub(bncplx_t a, bncplx_t b);
void c_subip(bncplx_t a, bncplx_t b);
bncplx_t c_add(bncplx_t a, bncplx_t b);
void c_addip(bncplx_t a, bncplx_t b);

void c_magph(bncplx_t a);


// fft
bignum_t b_fftmul(bignum_t a, bignum_t b);
bncomp_t b_split_init(bignum_t a, unsigned int n);
bncomp_t b_split_even(bncomp_t a);
bncomp_t b_split_odds(bncomp_t a);
bncomp_t b_fftr(bncomp_t a, bncplx_t omega);

void b_print_comp(bncomp_t a);

// rsa
void keygen();
char * encrypt(char *plaintext, unsigned int *block_no);
char * encryptb(char *plaintext, bignum_t pq, bignum_t e);
char * decrypt(char *ciphertext, unsigned int blocks);
char * decryptb(char *plaintext, bignum_t pq, bignum_t d);
unsigned int strlength(char *str);









void printhex(char *str, unsigned int length);