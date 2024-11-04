#include "main.h"

#define BIT_LENGTH 32

#include <math.h>

const unsigned char sign_mask = 0x01;

double madd_time = 0.0;
long long madd_calls = 0;

double mmul_time = 0.0;
long long mmul_calls = 0;

double subip_time = 0.0;
long long subip_calls = 0;

double msub_time = 0.0;
long long msub_calls = 0;

double comp_time = 0.0;
long long comp_calls = 0;

double mexp_time = 0.0;
long long mexp_calls = 0;

double modip_time = 0.0;
long long modip_calls = 0;

double mlsip_time = 0.0;
long long mlsip_calls = 0;

double fftmul_time = 0.0;
long long fftmul_calls = 0.0;

/**
 * TODO currently
 * - there is a segfault somewhere in bigmod.c
 * - optimise add and sub functions
 * - convert stuff to bit manipulation to speed up
 */

/**
 * Generates a number (n - 1), for testing primality of n
 */
bignum_t b_gen(unsigned int size) {
    bignum_t res = b_init(size);
    bignum_t zero = b_initc(0);

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    srand((unsigned int)(ts.tv_nsec % (unsigned int)(0xFFFFFFFF)));
    
    res->data[0] = (rand() % 0x7F) << 1;

    for (int i = 1; i < size; i++) {
        res->data[i] = rand() % 0xFF;
    }

    if (b_comp(res, zero) == 0) {
        b_free(zero);
        b_free(res);
        return b_gen(size);
    }

    b_free(zero);

    return res;
}

bignum_t b_gen_prime(unsigned int size) {
    bignum_t potential_prime = NULL;
    bignum_t composite = NULL;
    bignum_t two = b_initc(2);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    int i = 0;

    composite = b_gen(size);
    potential_prime = b_ori(composite, 1);

    do {

        i++;

        // printf("%d\n", i);

        b_addip(composite, two);
        b_free(potential_prime);
        potential_prime = b_ori(composite, 1);

        
    } while (!b_is_prime(potential_prime, composite));

    b_free(composite);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_t = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

    uint64_t us_pt = delta_t / i;

    // printf("\n\n\ntries = %d \t us/try = %lu \t us tot = %lu \t", i, us_pt, delta_t);

    return potential_prime;
}

int b_is_prime(bignum_t n, bignum_t n_minus) {

    // test val
    bignum_t one = b_initc(1);

    // Fermat test
    bignum_t fermat_a = b_gen(1);
    bignum_t res = b_mexp(fermat_a, n_minus, n);

    if (b_comp(res, one) != 0) return 0;

    b_free(fermat_a);   fermat_a = NULL;
    b_free(res);        res = NULL;


    // Miller-Rabin test
    bignum_t mr_num = b_copy(n_minus);

    int s = 0;
    unsigned char mask = 0x01;
    while (b_andi(mr_num, mask) == 0) {
        s++;
        b_rsip(mr_num, 1);
    }

    bignum_t mr_a = NULL;
    bignum_t mr_x = NULL;
    bignum_t mr_y = NULL;

    for (int i = 0; i < 10; i++) {


        b_free(mr_a);
        b_free(mr_x);

        mr_a = b_gen(BIT_LENGTH);

        mr_x = b_mexp(mr_a, mr_num, n);

        // squaring 1 or -1 will only result in 1
        if ((b_comp(mr_x, one) == 0) || (b_comp(mr_x, n_minus) == 0)) {
            continue;
        }

        for (int j = 0; j < s - 1; j++) {
            mr_y = b_fftmul(mr_x, mr_x);
            b_modip(mr_y, n);
            bignum_t temp = mr_x;
            mr_x = mr_y;
            b_free(temp);

            // we know the previous root was not 1 or -1, so we have found a different sqrt of 1
            if (b_comp(mr_x, one) == 0) {
                return 0;
            }

            // squaring -1 will just give us 1
            if (b_comp(mr_x, n_minus) == 0) {
                continue;
            }
        }

        return 0;
    }

    return 1;
}

unsigned int b_bytes(bignum_t a) {
    return a->size;
}

unsigned char b_msb(bignum_t a) {
    return a->data[a->size - 1];
}

// checks if the addition with the specified carry results in an overflow
unsigned int b_overflow(unsigned char a, unsigned char b, unsigned char carry) {
    if (carry) carry = 1;
    //printf("overflow check: 0x%02X 0x%02X %u => 0x%02X\n", a, b, carry, a+b+carry);
    if (((a + b + carry) & 0xFFFFFF00)) return 1;
    return 0;
}

bignum_t b_largest(bignum_t a, bignum_t b) {
    return (b_comp(a, b) >= 0) ? a : b;
}

bignum_t b_smallest(bignum_t a, bignum_t b) {
    return (b_comp(a, b) >= 0) ? b : a;
}

unsigned int b_smallest_bytes(bignum_t a, bignum_t b) {
    return (b_bytes(b) < b_bytes(a)) ? b_bytes(b) : b_bytes(a);
}

// finds the size for storing an addition of the 2 bignums
unsigned int b_largest_bytes(bignum_t a, bignum_t b) {
    return (b_bytes(b) < b_bytes(a)) ? b_bytes(a) : b_bytes(b);
}

/**
 * Compares two bignums by magnitude only (not sign)
 * @returns 1 if the first is bigger, -1 if the second is bigger, 0 if equal
 */
int b_comp(bignum_t a, bignum_t b) {
    b_trim(a);
    b_trim(b);
    if (b_bytes(a) != b_bytes(b)) return (b_bytes(a) > b_bytes(b)) ? 1 : -1;
    // same number of bytes
    // compate byte by byte until we get to a different one
    for (int i = a->size - 1; i >= 0; i--) {

        if (a->data[i] == b->data[i]) {
            continue;
        }
        int temp = (a->data[i] > b->data[i]) ? 1 : -1;
        return temp;
    }
    // all bytes are the same
    return 0;
}

/**
 * Initialisation of a bignum
 * @param size - the number of bytes to initialise
 * 
 * bignum is initialised with all bytes equal to zero
 */
bignum_t b_init(unsigned int size) {
    // init memory for struct
    bignum_t res = malloc(sizeof(struct bignum));

    //init size
    res->size = size;

    // init data array
    res->data = malloc(sizeof(unsigned char) * size);
    for (int i = 0; i < size; i++) res->data[i] = 0;

    // init sign
    res->sign = 0;
}

/**
 * Initialise with Value
 * @param initial - the initial value for the bignum (as an int)
 * 
 */
bignum_t b_initv(int initial) {
    unsigned int mask = 0xFF;

    bignum_t res = b_init(4);

    if (initial < 0) {
        res->sign = 1;
        initial = -initial;
    }

    res->data[0] = initial & mask;
    res->data[1] = (initial & (mask << 8)) >> 8;
    res->data[2] = (initial & (mask << 16)) >> 16;
    res->data[3] = (initial & (mask << 24)) >> 24;

    b_trim(res);

    return res;
}

bignum_t b_initl(long long initial) {
    unsigned int mask = 0xFF;

    bignum_t res = b_init(8);

    if (initial < 0) {
        res->sign = 1;
        initial = -initial;
    }

    for (int i = 0; i < 8; i++) {
        res->data[i] = (initial & (mask << i * 8)) >> i * 8;
    }

    b_trim(res);

    return res;
}

/**
 * Initialise with Character
 * @param initial - the initial value for the bignum (as an unsigned char)
 * 
 */
bignum_t b_initc(unsigned char initial) {
    bignum_t res = malloc(sizeof(struct bignum));

    res->size = 1;
    res->sign = 0;
    res->data = malloc(sizeof(unsigned char));

    res->data[0] = initial;
    
    return res;
}

/**
 * Free Bignum
 * @param a - the bignum to be free'd
 * 
 * Frees all associated memory with the given bignum \n
 * Null-safe
 */
void b_free(bignum_t a) {
    if (a == NULL) return;
    free(a->data);
    free(a);
}

/**
 * Copy Constructor
 * @param a - the number to be copied
 * @returns a new bignum with the exact same values, but entirely different memory
 */
bignum_t b_copy(bignum_t a) {
    bignum_t res = malloc(sizeof(struct bignum));

    res->sign = a->sign;
    res->size = a->size;
    res->data = malloc(sizeof(unsigned char) * res->size);

    for (int i = 0; i < res->size; i++) {
        res->data[i] = a->data[i];
    }

    // how tf did it work without this
    return res;
}

/**
 * Absolute Value Copy
 * @param a - the number to be copied
 * @returns a new bignum with the exact same values, but entirely different memory
 * 
 * The sign of the new bignum will always be 0 (positive)
 */
bignum_t b_acopy(bignum_t a) {
    bignum_t res = malloc(sizeof(struct bignum));

    res->sign = 0;
    res->size = a->size;
    res->data = malloc(sizeof(unsigned char) * res->size);

    for (int i = 0; i < res->size; i++) {
        res->data[i] = a->data[i];
    }

    // how tf did it work without this
    return res;
}

bignum_t b_fromstr(char* str, unsigned int n) {
    bignum_t res = b_init(n);
    for (int i = 0; i < n; i++) {
        res->data[i] = str[i];
    }
    return res;
}

unsigned char num_from_hex(unsigned char hexval) {
    if (hexval >= '0' && hexval <= '9') {
        return hexval - '0';
    } else if (hexval >= 'A' && hexval <= 'F') {
        return hexval - 'A' + 0x0A;
    } else if (hexval >= 'a' && hexval <= 'f') {
        return hexval - 'a' + 0x0A;
    } else return 0;
}

bignum_t b_fromhex(char *str, unsigned int n) {
    bignum_t res = b_init(n / 2 + 1);

    // printf("n=%u\n", n);

    unsigned int even = (n % 2 == 0) ? 0 : 1;

    for (int i = n - 2; i > 0; i -= 2) {
        unsigned char lower = num_from_hex(str[i]);
        unsigned char upper = num_from_hex(str[i-1]);

        // printf("i=%d\tupper=%u\tlower=%u\n", i, upper, lower);
        // printf("str[%d] = %c\tstr[%d] = %c\n", i, str[i], i-1, str[i-1]);

        res->data[(n-i-1)/2] = lower | (upper << 4);

        // printf("(n-i)/2=%d\tres[%d] = %02X\n", (n-i)/2, (n-i)/2, res->data[(n-i)/2]);
    }

    if (!even) {
        // printf("hello\n");
        res->data[(n-1)/2] |= (num_from_hex(str[0]));
    }

    b_trim(res);

    // b_print(res);
    // printf("hex: %s\n", b_tohex(res));

    return res;
}

/**
 * Gets the p_minus value - if a is odd, truncates the final 1, otherwise returns a new copy of a
 * @param a - the number to operate on
 */
bignum_t b_minus(bignum_t a) {
    bignum_t res = b_acopy(a);

    res->data[0] &= 0xFE;

    return res;
}

/**
 * Modular Multaplicative Inverse (using the Extended Euclidian Algorithm)
 * @param a - mmi of the result
 * @param b - modulo
 */
bignum_t b_mmi(bignum_t a, bignum_t b) {
    bignum_t r0 = b_acopy(a);
    bignum_t r1 = b_acopy(b);

    bignum_t s0 = b_initc(1);
    bignum_t s1 = b_initc(0);

    bignum_t r2 = NULL;
    bignum_t s2 = NULL;

    bignum_t qi = NULL;
    bignum_t temp = NULL;
    bignum_t zero = b_initc(0);

    int i = 0;

    while (1) {
        qi = b_div(r0, r1, &r2);
        b_free(r2);
        temp = b_mul(qi, r1);
        r2 = b_sub(r0, temp);
        b_free(temp);

        

        temp = b_mul(qi, s1);
        s2 = b_sub(s0, temp);
        b_free(temp);

        if (b_comp(r2, zero) == 0) {

            b_free(qi);
            b_free(r0);
            b_free(r1);
            b_free(r2);

            b_free(s0);

            if (s1->sign) {
                b_subip(s1, b);
            }

            b_free(zero);

            bignum_t res = b_acopy(s1);

            b_free(s1);

            return res;
        }

        b_free(r0);
        b_free(s0);

        r0 = r1;
        s0 = s1;

        r1 = r2;
        s1 = s2;

        i++;
    }
}

/**
 * trims leading 0 bytes in place
 */
void b_trim(bignum_t a) {

    // no trim needed
    if (a->data[a->size - 1]) return;

    int i;
    for (i = a->size - 1; i > 0; i--) {
        if (a->data[i]) break;
    }

    unsigned char *temp = realloc(a->data, i + 1);
    if (temp == NULL) {
        fprintf(stderr, "Error reallocating bignum size!\n");
        exit(0);
    }


    a->data = temp;
    a->size = i + 1;
}

/**
 * Pads the bignum with leading zero bytes
 * @param a - the bignum to be padded
 * @param size - the new padded size (in bytes)
 */
void b_pad(bignum_t a, unsigned int size) {
    if (size <= b_bytes(a)) return;
    unsigned char *temp = realloc(a->data, sizeof(unsigned char) * size);
    if (temp == NULL) {
        fprintf(stderr, "Error in padding bignum! Program not exiting\n");
        return;
    }

    if (size > a->size) {
        for (int i = a->size; i < size; i++) temp[i] = 0;
    }

    a->data = temp;
    a->size = size;
}

/**
 * Or Immediate
 */
bignum_t b_ori(bignum_t a, unsigned char val) {
    if (a == NULL || a->size < 1) return a;
    bignum_t res = b_copy(a);
    b_oriip(res, val);
    return res;
}

/**
 * Or Immediate In Place
 */
void b_oriip(bignum_t a, unsigned char val) {
    if (a->size >= 1) a->data[0] |= val;
}

/**
 * And Immediate
 */
unsigned char b_andi(bignum_t a, unsigned char val) {
    if (a->size >= 1) {
        unsigned int temp = (a->data[0] & val);
        return temp;
    }
    return 0;
}

bignum_t b_add(bignum_t a, bignum_t b) {

    // both neg => (-a) + (-b)
    if (a->sign != 0 && b->sign != 0) {
        // same as - (a + b)
        // re-call function and then change sign
        bignum_t t1 = b_acopy(a);
        bignum_t t2 = b_acopy(b);
        bignum_t res = b_add(t1, t2);
        b_free(t1);
        b_free(t2);
        res->sign = 1;
        return res;
    }

    // a neg => (-a) + b
    if (a->sign != 0 && b->sign == 0) {
        // same as b - a
        bignum_t t1 = b_acopy(a);
        bignum_t res = b_sub(b, t1);
        b_free(t1);
        return res;
    }

    // b neg => a + (-b)
    if (a->sign == 0 && b->sign != 0) {
        // same as a - b
        bignum_t t1 = b_acopy(b);
        bignum_t res = b_sub(a, t1);
        b_free(t1);
        return res;
    }

    // from here on, we know both numbers are positive

    // gets the smallest and largest numbers
    bignum_t smallest = b_smallest(a, b);
    bignum_t largest = b_largest(a, b);

    // do we need to pad?
    if (b_bytes(smallest) != b_bytes(largest)) {
        b_pad(smallest, largest->size);
    }

    // init variables for addition
    unsigned char carry = 0;
    bignum_t res = b_init(largest->size + 1);

    // add bytewise
    for (int i = 0; i < smallest->size; i++) {

        unsigned char a_byte = a->data[i];
        unsigned char b_byte = b->data[i];

        res->data[i] = a_byte + b_byte + carry;

        if (b_overflow(a_byte, b_byte, carry)) carry = 1;
        else carry = 0;
        
    }

    // last bit
    if (carry == 1) {
        res->data[res->size - 1] = 1;
    }

    b_trim(res);

    return res;

}

bignum_t b_sub(bignum_t a, bignum_t b) {

    // both neg => (-a) - (-b)
    if (a->sign != 0 && b->sign != 0) {
        // same as b - a
        bignum_t t1 = b_acopy(a);
        bignum_t t2 = b_acopy(b);

        bignum_t res = b_sub(t2, t1);
        b_free(t1);
        b_free(t2);
        return res;
    }

    // a neg => (-a) - b
    if (a->sign != 0 && b->sign == 0) {
        // same as -(a + b)
        bignum_t t1 = b_acopy(a);
        bignum_t t2 = b_acopy(b);

        bignum_t res = b_add(t1, t2);
        b_free(t1);
        b_free(t2);
        res->sign = 1;
        return res;
    }

    // b neg => a - (-b)
    if (a->sign == 0 && b->sign != 0) {
        // same as a + b
        bignum_t t1 = b_acopy(b);
        bignum_t res = b_add(a, t1);
        b_free(t1);
        return res;
    }

    // both pos => a - b
    // basic case, the rest of the fn

    // gets the smallest and largest numbers
    bignum_t smallest = b_smallest(a, b);
    bignum_t largest = b_largest(a, b);

    // if a and b are the same, return 0
    if (largest == smallest) {
        printf("bad things, potter\n");
        return b_initc(0);
    }

    if (largest == b) {
        // subtracting a bigger number
        // can do this by recalling fn with args swapped, then make neg
        bignum_t res = b_sub(b, a);
        res->sign = 1;
        return res;
    }

    // from here, we know that a is the larger number

    bignum_t res = b_init(a->size);

    // init variables for addition
    unsigned char carry = 0;
    
    b_pad(b, a->size);

    for (int i = 0; i < a->size; i++) {

        res->data[i] = a->data[i] - b->data[i] - carry;

        if (b->data[i] + carry > a->data[i]) {
            // we need to borrow from the next bit
            carry = 1;
        } else carry = 0;
    }

    return res;
}

/**
 * Add In Place
 * @param a - first number (result is stored here)
 * @param b - second number
 */
void b_addip(bignum_t a, bignum_t b) {

    unsigned int largest_bytes = b_largest_bytes(a, b);

    // we need to make sure there is enough room in a to store the result
    b_pad(a, largest_bytes + 1);
    b_pad(b, largest_bytes + 1);

    // init variables for addition
    unsigned char carry = 0;
    unsigned char a_byte = 0;
    unsigned char b_byte = 0;
    unsigned int a_i = 0;
    unsigned int b_i = 0;
    unsigned int c_i = 0;
    unsigned int overflow_check = (unsigned int) 0xFF;

    // add bytewise
    for (int i = 0; i < a->size; i++) {

        a_byte = a->data[i];
        b_byte = b->data[i];

        a->data[i] = a_byte + b_byte + carry;

        a_i = (unsigned int) a_byte;
        b_i = (unsigned int) b_byte;
        c_i = (unsigned int) carry;

        if (a_i + b_i + c_i > overflow_check) carry = 1;
        else carry = 0;
        
    }

    b_trim(a);
}

/**
 * Left Shift (logical)
 * (shift is in bits)
 */
bignum_t b_lshift(bignum_t a, unsigned int shift) {
    unsigned int bitsnum = a->size * 8 + shift;
    unsigned int bytesnum = bitsnum / 8 + 1;

    bignum_t res = b_init(bytesnum); res->sign = a->sign;

    for (int i = 0; i < a->size; i++) {
        unsigned int bitshift = (shift % 8);
        unsigned int byteshift = shift / 8; // truncated we hope
        unsigned char mask = 0xFF >> bitshift;
        unsigned char lower = (a->data[i] & mask) << bitshift;
        mask = ~mask;
        unsigned char upper = (a->data[i] & mask) >>(8 - bitshift);

        res->data[i + byteshift] |= lower;
        res->data[i + byteshift + 1] |= upper;
    }

    b_trim(res);

    return res;
}

/**
 * Left Shift In Place
 * (shift is in bits)
 */
void b_lsip(bignum_t a, unsigned int shift) {
    // bignum_t res = b_lshift((*a), shift);
    // b_free((*a));
    // *a = res;
    unsigned int oldSize = a->size;
    unsigned int bitsnum = a->size * 8 + shift;
    unsigned int bytesnum = bitsnum / 8 + 1;
    unsigned int bitshift = (shift % 8);
    unsigned int byteshift = shift / 8; // truncated we hope

    // bignum_t res = b_init(bytesnum); res->sign = a->sign;

    b_pad(a, bytesnum);

    unsigned char temp = 0;

    for (int i = oldSize - 1; i >= 0; i--) {
        unsigned char mask = 0xFF >> bitshift;
        unsigned char lower = (a->data[i] & mask) << bitshift;
        mask = ~mask;
        unsigned char upper = (a->data[i] & mask) >>(8 - bitshift);

        temp |= upper;
        a->data[i + byteshift + 1] = temp;
        temp = lower;

        // a->data[i + byteshift] |= lower;
        // a->data[i + byteshift + 1] |= upper;
    }

    a->data[byteshift] = temp;

    // make sure we put in the new zeros
    for (int i = 0; i < shift; i++) {
        unsigned char temp = ~(0x01 << (i % 8));
        a->data[i / 8] &= temp;
    }

    b_trim(a);
}

/**
 * Right Shift (logical)
 * (shift is in bits)
 */
bignum_t b_rshift(bignum_t a, unsigned int shift) {
    bignum_t res = b_init(b_bytes(a)); res->sign = a->sign;

    unsigned int bitshift = shift % 8;
    unsigned int byteshift = shift / 8;
    unsigned char mask = 0xFF >> (8 - bitshift);

    for (int i = 0; i < a->size; i++) {


        if (i - byteshift < 0) {
            continue;
        }

        unsigned char lower = (a->data[i] & mask) << (8 - bitshift);
        unsigned char upper = (a->data[i] & (~mask)) >> (bitshift);


        // note to self - negating an unsigned does not make it signed again ;(
        if (0 <= (int)(i - byteshift - 1)) {
            res->data[i - byteshift - 1] |= lower;
        }
        if (0 <= (int)(i - byteshift)) {
            res->data[i - byteshift] |= upper;
        }
    }

    b_trim(res);

    return res;
}

/**
 * Right Shift In Place
 * (shift is in bits)
 */
void b_rsip(bignum_t a, unsigned int shift) {
    // if (a == NULL) printf("oh no\n");
    // bignum_t res = b_rshift((*a), shift);
    // b_free((*a));
    // *a = res;

    unsigned int bitshift = shift % 8;
    unsigned int byteshift = shift / 8;
    unsigned char mask = 0xFF >> (8 - bitshift);

    unsigned char temp = 0;

    for (int i = 0; i < a->size; i++) {
        if (i - byteshift < 0) {
            continue;
        }

        unsigned char lower = (a->data[i] & mask) << (8 - bitshift);
        unsigned char upper = (a->data[i] & (~mask)) >> (bitshift);

        // note to self - negating an unsigned does not make it signed again ;(
        if (0 <= (int)(i - byteshift - 1)) {
            temp |= lower;
            a->data[i - byteshift - 1] = temp;
            temp = upper;

        } else temp = upper;

        // this should never fail thanks to the continue above
        // if (0 <= (int)(i - byteshift)) {
        //     res->data[i - byteshift] |= upper;
        // }

    }


    if ((int)(a->size - 1 - byteshift) >= 0) {
        a->data[a->size - 1 - byteshift] = temp;
    }


    // make sure we erase the old stuff
    for (int i = 0; i < shift; i++) {
        unsigned char temp = ~(0x80 >> (i % 8));
        a->data[a->size - 1 - (i / 8)] &= temp;
    }

    b_trim(a);
}

bignum_t b_mul(bignum_t a, bignum_t b) {
    // gets the smallest and largest numbers
    bignum_t smallest = b_smallest(a, b);
    smallest = b_acopy(smallest);
    bignum_t largest = b_largest(a, b);
    largest = b_acopy(largest);

    // TODO fix pls
    if (smallest == largest) {
        smallest = (smallest == a) ? b : a;
    }

    bignum_t res = b_init((b_bytes(a) + b_bytes(b)));

    for (int i = 0; i < smallest->size; i++) {
        for (int j = 0; j < 8; j++) {
            if (smallest->data[i] & (1 << j)) {
                // need to add the shift
                bignum_t temp = b_lshift(largest, i * 8 + j); bignum_t temp2 = res;
                res = b_add(res, temp); b_free(temp2);
                b_free(temp);
            }
        }
    }

    b_free(smallest);
    b_free(largest);

    b_trim(res);

    res->sign = a->sign ^ b->sign;

    return res;
}



/**
 * Division function
 * 
 * @param n - numerator
 * @param d - denominator
 * @param r - remainder (mod) - this will be overwritten
 * 
 * @returns q, the quotient of the division
 */
bignum_t b_div(bignum_t n, bignum_t d, bignum_t *r) {
    b_trim(n);
    b_trim(d);

    // currently just discards anything currently held in r
    // if (r != NULL) b_free(r);
    // printf("hi\n");

    bignum_t q = b_init(b_bytes(n));

    q->sign = n->sign ^ d->sign;

    int shift = b_bytes(n) - b_bytes(d) + 1; // in bytes

    if (shift < 1) return q; // if the denom is bigger, doesn't divide into

    shift *= 8; // convert to bits shifted

    // try align bytes in the copies
    bignum_t ds = b_lshift(d, shift); ds->sign = 0;
    bignum_t ns = b_acopy(n);


    // we currently know that ds is bigger
    // want to make it smaller or equal
    while(b_comp(ns, ds) < 0) {
        b_rsip(ds, 1);
        shift--;
    }

    // need one extra shift for some reason
    shift++;


    bignum_t t;
    for (int i = 0; i < shift; i++) {
        if (b_comp(ns, ds) >= 0) {
            t = b_sub(ns, ds);
            b_free(ns);
            ns = t;
            b_oriip(q, 1);
        }
        b_rsip(ds, 1);
        b_lsip(q, 1);
    }

    *r = ns;
    b_free(ds);
    // right shift once (not sure why, but we need to)
    b_rsip(q, 1);

    return q;
}

/**
 * Exponent function
 * @param b - base
 * @param e - exponent
 * @returns b^(e)
 */
bignum_t b_exp(bignum_t b, bignum_t e) {
    bignum_t zero = b_initc(0);
    bignum_t base = b_acopy(b);
    bignum_t exp = b_acopy(e);

    // set our result to 1
    bignum_t res = b_initc(1);

    int i = 0;
    while(b_comp(exp, zero) >= 1) {
        if (b_andi(exp, 1)) {
            bignum_t temp = res;
            res = b_mul(res, base);
            b_free(temp);
        }

        bignum_t temp = base;
        base = b_mul(base, base);
        b_free(temp);

        i++;

        b_rsip(exp, 1);
    }

    if (b->sign) res->sign = (b_andi(e, 1)) ? 1 : 0;

    return res;
}


int b_toi(bignum_t a) {
    if (a->size > 4) return (a->sign == 0) ? __INT32_MAX__ : -__INT32_MAX__;

    int total = 0;

    for (int i = 0; i < a->size; i++) {
        total += a->data[i] * pow(2, 8*i);
    }

    return total;
}

void b_print(bignum_t a) {
    unsigned int total = 0;
    printf("printing bignum...\n");
    for (int i = 0; i < a->size; i++) {
        total += a->data[i] * pow(2, 8*i);
        printf("%d: 0x%02X\n", i, a->data[i]);
    }

    printf("bignum: %s%u\n", ((a->sign == 0) ? "+" : "-"), total);
}

void b_prints(bignum_t a) {
    unsigned int total = 0;
    for (int i = 0; i < a->size; i++) {
        total += a->data[i] * pow(2, 8*i);
    }

    printf("%s%u", ((a->sign == 0) ? "+" : "-"), total);
}

char* b_tostr(bignum_t a) {
    bignum_t zero = b_initc(0);
    if (b_comp(a, zero) == 0) return " 0";

    bignum_t n = b_copy(a);
    bignum_t ten = b_initc(10);
    bignum_t rem;
    int len = 0;

    while (b_comp(n, zero) >= 1) {
        len++;
        bignum_t temp = n;
        n = b_div(n, ten, &rem);
        b_free(rem);
        b_free(temp);
    }

    b_free(n);
    n = b_copy(a);

    char *str = malloc(sizeof(char) * (len + 2));

    int neg_flag = 0;

    if (a->sign) {
        str[0] = '-';
    } else {
        str[0] = ' ';
    }

    for (int i = 0; i < len; i++) {
        bignum_t temp = n;
        n = b_div(n, ten, &rem);
        b_free(temp);
        if (b_toi(rem) >= 10) printf("aaaaaa %d\n", b_toi(rem));
        str[len - i] = b_toi(rem) + '0';
        b_free(rem);
    }
    str[len + 1] = '\0';
    b_free(zero); 
    b_free(ten); 
    b_free(n); 
    return str;
}

char* b_tohex(bignum_t a) {
    bignum_t zero = b_initc(0);
    if (b_comp(a, zero) == 0) return "0";

    bignum_t n = b_copy(a);
    bignum_t sixteen = b_initc(16);
    bignum_t rem;
    int len = 0;
    char characters[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    while (b_comp(n, zero) >= 1) {
        len++;
        bignum_t temp = n;
        n = b_div(n, sixteen, &rem);
        b_free(rem);
        b_free(temp);
    }

    b_free(n);
    n = b_copy(a);

    char *str = malloc(sizeof(char) * (len + 1));

    for (int i = 0; i < len; i++) {
        bignum_t temp = n;
        n = b_div(n, sixteen, &rem);
        b_free(temp);
        if (b_toi(rem) >= 16) printf("aaaaaa %d\n", b_toi(rem));
        str[len - 1 - i] = characters[b_toi(rem)];
        b_free(rem);
    }
    str[len] = '\0';
    b_free(zero); 
    b_free(sixteen); 
    b_free(n); 
    return str;
}

void printhex(char *str, unsigned int length) {
    for (int i = length - 1; i >= 0; i--) printf("%02X", (unsigned char)str[i]);
}

unsigned char * hextochar(char *hexstr, unsigned int length) {
    char *ret = malloc(sizeof(unsigned char) * length / 2 + 5);
    for (int i = 0; i < length; i++) {
        ret[(length - i - 1)/2] = num_from_hex(hexstr[i]) | (num_from_hex(hexstr[i - 1]) << 4);
    }
    ret[(length + 1)/2] = '\0';
    return ret;
}

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

        // char *dec = decrypt(cypher, blocks);

        // unsigned int length_dec = strlength(dec);

        // bignum_t res2 = b_fromstr(dec, length_dec);

        // b_trim(res2);

        // printf("\ndecrypted string            : %s\n", b_tohex(res2));
        // printf("raw string                  : %s\n", res2->data);
        // printf("without bignum interference : %s\n", dec);

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

        // bignum_t res2 = b_fromstr(dec, length_dec);

        // b_trim(res2);

        // printf("\ndecrypted string            : %s\n", b_tohex(res2));
        // printf("raw string                  : %s\n", res2->data);
        printf("decrypted string : %s\n", dec);

        return 0;
    }

    if (strcmp(argv[1], "thex") == 0) {
        printf("enter the decimal number: ");
        long long a;
        scanf("%lld", &a);

        bignum_t res = b_initl(a);

        b_print(res);
        printf("hex: %s\n", b_tohex(res));


        return 0;
    }

    if (strcmp(argv[1], "fhex") == 0) {
        printf("Enter the hex string: ");
        char *in = malloc(sizeof(char) * 128);
        for (int i = 0; (in[i] = getchar()) != '\n'; i++) {
            ; ;
        }

        printf("recieved: %s\n", in);

        unsigned int text_length = 0;
        for (int i = 0; in[i] != '\0'; i++) text_length++;

        b_fromhex(in, text_length);
        return 0;
    }

    if (strcmp(argv[1], "keygen") == 0) {
        keygen();
        return 0;
    }

    if (strcmp(argv[1], "mmi") == 0) {
        long long a;
        long long b;

        printf("enter 2 nums: ");

        scanf("%lld %lld", &a, &b);

        bignum_t ba = b_initl(a);
        bignum_t bb = b_initl(b);

        bignum_t bres = b_mmi(ba, bb);

        printf("mmi = %s\n", b_tostr(bres));
        return 0;
    }

    if (strcmp(argv[1], "mul") == 0) {
        printf("enter 2 nums: ");

        long long a;
        long long b;

        scanf("%lld %lld", &a, &b);

        bignum_t ba = b_initl(a);
        bignum_t bb = b_initl(b);

        bignum_t bres;
        bignum_t fres;
        long long res;

        printf("multiplying...\n\n");

        res = (a * b);

        bres = b_mul(ba, bb);
        fres = b_fftmul(ba, bb);

        printf("ints: %lld\n", res);
        printf("regular: %s\n", b_tostr(bres));
        printf("fft: %s\n", b_tostr(fres));

        return 0;
    } else if (strcmp(argv[1], "multest") == 0) {

        bignum_t ba = b_gen(32);
        bignum_t bb = b_gen(32);

        bignum_t bres;
        bignum_t fres;
        // long long res;

        printf("multiplying...\n\n");

        // res = (a * b);

        bres = b_mul(ba, bb);
        fres = b_fftmul(ba, bb);

        // printf("ints: %lld\n", res);
        printf("regular\t: %s\n", b_tostr(bres));
        printf("fft    \t: %s\n", b_tostr(fres));

        return 0;
    }

    if (strcmp(argv[1], "prime") == 0) {
        bignum_t temp = NULL;
        for (int i = 0; i < 1; i++) {
            temp = b_gen_prime(BIT_LENGTH);
            char *str = b_tohex(temp);
            printf("%s\n\n", str);
            free(str);
            b_free(temp); temp = NULL;
        }
        b_free(temp);

        printf("STATS\n");
        // printf("madd:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", madd_calls, madd_time, (double)(madd_time * 1000000 / madd_calls));
        printf("mmul:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", mmul_calls, mmul_time, (double)(mmul_time * 1000000 / mmul_calls));
        printf("modip:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", modip_calls, modip_time, (double)(modip_time * 1000000 / modip_calls));
        // printf("subip:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", subip_calls, subip_time, (double)(subip_time * 1000000 / subip_calls));
        // printf("msub:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", msub_calls, msub_time, (double)(msub_time * 1000000 / msub_calls));
        // printf("mexp:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", mexp_calls, mexp_time, (double)(mexp_time * 1000000 / mexp_calls));
        // printf("mlsip:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", mlsip_calls, mlsip_time, (double)(mlsip_time * 1000000 / mlsip_calls));
        printf("fftmul:\n\tcalls = %lld\n\ttime = %lf\n\tFOM = %lf\n", fftmul_calls, fftmul_time, (double)(fftmul_time * 1000000 / fftmul_calls));
        return 0;
    } else if (strcmp(argv[1], "mod") == 0) {
        printf("enter command: ");

        int a;
        int b;
        int m;
        char op;

        scanf("%d %c %d %d", &a, &op, &b, &m);

        bignum_t ba = b_initv(a);
        bignum_t bb = b_initv(b);
        bignum_t bm = b_initv(m);

        bignum_t bres;
        int res;
        int nmres = 0;

        if (op == '+') {
            printf("adding...\n\n");
            nmres = (a + b);

            bres = b_acopy(ba);
            b_madd(bres, bb, bm);
        } else if (op == '*') {
            printf("multiplying...\n\n");

            nmres = (a * b);

            bres = b_acopy(ba);
            b_mmul(bres, bb, bm);
        } else if (op == '^') {
            printf("exponentiating...\n\n");

            nmres = (int)(pow(a, b));

            bres = b_mexp(ba, bb, bm);
        } else if (op == '-') {
            printf("subtracting...\n\n");

            nmres = (a - b);

            bres = b_acopy(ba);
            b_msub(bres, bb, bm);
        } else if (op == '%') {
            printf("moduloing...\n\n");

            nmres = a;

            bres = b_acopy(ba);
            b_modip(bres, bm);
        }

        res = nmres % m;
        printf("int result (no mod) = %d\n", nmres);
        printf("int result = %d\n", res);
        printf("bignum result = %s\n", b_tostr(bres));
        return 0;
    }

    

    if (argc != 4) {
        fprintf(stderr, "usage: ./test <num1> <op> <num2>");
        exit(1);
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[3]);

    bignum_t b_a = b_initv(a);
    bignum_t b_b = b_initv(b);

    printf("inputted: %d %d\n", a, b);
    printf("hex: 0x%04X 0x%04X\n\n", a, b);

    printf("recieved %s\n", argv[2]);

    printf("Initial inputs as bignum:\nA:\n");
    b_print(b_a);
    printf("B:\n");
    b_print(b_b);

    int res;
    bignum_t b_res;
    bignum_t b_res_2;


    if (strcmp(argv[2], "ls") == 0) {
        printf("left shifting...\n\n");

        res = a << b;
        b_res = b_lshift(b_a, b);
    } else if (strcmp(argv[2], "lsip") == 0) {
        printf("left shifting in place...\n\n");

        res = a << b;
        b_res = b_copy(b_a);
        b_lsip(b_res, b);
    } else if (strcmp(argv[2], "rs") == 0) {
        printf("right shifting...\n\n");

        res = a >> b;
        b_res = b_rshift(b_a, b);
    } else if (strcmp(argv[2], "rsip") == 0) {
        printf("right shifting in place...\n\n");

        res = a >> b;

        b_res = b_copy(b_a);
        b_rsip(b_res, b);
    } else if (strcmp(argv[2], "mul") == 0) {
        printf("multiplying...\n\n");

        res = a * b;
        b_res = b_mul(b_a, b_b);
    } else if (strcmp(argv[2], "div") == 0) {
        printf("dividing...\n\n");

        res = a / b;
        b_res = b_div(b_a, b_b, &b_res_2);

        printf("\nmodulus:\n");
        printf("ints: %d\n", a % b);
        b_print(b_res_2);
        printf("\n\nquotient:\n");
    } else if (strcmp(argv[2], "exp") == 0) {
        printf("exponentiating...\n\n");

        res = pow(a, b);
        b_res = b_exp(b_a, b_b);
    } else if (strcmp(argv[2], "rand") == 0) {
        printf("randoming...\n\n");

        res = 0;
        b_res = b_gen(a);
    } else if (strcmp(argv[2], "mod") == 0) {
        printf("mod...\n\n");

        res = a % b;
        b_res = b_mod(b_a, b_b);
    } else if (strcmp(argv[2], "mexp") == 0) {
        printf("modulo exponentiation...\n\nenter the modulus: ");

        int m;
        scanf("%d", &m);

        res = (int)(pow(a, b)) % m;

        bignum_t b_m = b_initv(m);

        b_res = b_mexp(b_a, b_b, b_m);

        int temp = (int)(pow(a, b));
        printf("via regular methods:\nints exponent = %d\n", temp);
        b_print(b_exp(b_a, b_b));
        printf("now mod:\nints = %d\n", temp % m);
        b_print(b_mod(b_exp(b_a, b_b), b_m));
    }
    else {
        printf("Invalid operation!\n");
        return 1;
    }

    printf("ints: %d (0x%04X)\n", res, res);
    b_print(b_res);
    printf("bignum: %s%s\n", ((b_res->sign == 0) ? "+" : "-"), b_tostr(b_res));
}