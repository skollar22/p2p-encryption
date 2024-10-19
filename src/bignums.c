#include "main.h"

#include <math.h>

const unsigned char sign_mask = 0x01;

/**
 * Generates a number (n - 1), for testing primality of n
 */
bignum_t b_gen(unsigned int size) {
    bignum_t res = b_init(size);

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    srand((unsigned int)(ts.tv_nsec % (unsigned int)(0xFFFFFFFF)));
    
    res->data[0] = (rand() % 0x7F) << 1;

    for (int i = 1; i < size; i++) {
        res->data[i] = rand() % 0xFF;
    }


    return res;
}

int b_is_prime(bignum_t a) {

    
    
    int i = 0;

    bignum_t res = NULL;
    bignum_t fermat_a = NULL;
    bignum_t fermat_e = NULL;
    bignum_t fermat_n = NULL;

    // test val
    bignum_t one = b_initv(1); b_trim(one);

    do {

        i++;

        b_free(res);
        b_free(fermat_n);
        
        // first, we perform a fermat test
        bignum_t fermat_a = b_gen(4);

        bignum_t fermat_e = b_gen(2);

        fermat_n = b_copy(fermat_e);

        b_oriip(fermat_n, 1);

        // actual test
        res = b_mexp(fermat_a, fermat_e, fermat_n);

        b_free(fermat_a); fermat_a = NULL;
        b_free(fermat_e); fermat_e = NULL;

    } while (b_comp(res, one) != 0);

    printf("prime found! %s! tries = %d\n", b_tostr(fermat_n), i);
    b_free(one);
    b_free(fermat_a);
    b_free(fermat_e);
    b_free(fermat_n);
    

    b_free(res);
    return 0;
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

bignum_t b_initv(int initial) {
    bignum_t res = b_init(4);

    if (initial < 0) {
        res->sign = 1;
        initial = -initial;
    }

    unsigned int mask = 0xFF;
    res->data[0] = initial & mask;
    res->data[1] = (initial & (mask << 8)) >> 8;
    res->data[2] = (initial & (mask << 16)) >> 16;
    res->data[3] = (initial & (mask << 24)) >> 24;

    //printf("actual: 0x%02X 0x%02X 0x%02X 0x%02X\n", res->data[3], res->data[2], res->data[1], res->data[0]);
    //printf("wanted: 0x%02X 0x%02X 0x%02X 0x%02X\n", (initial & (mask << 24)) >> 24, (initial & (mask << 16)) >> 16, (initial & (mask << 8)) >> 8, initial & mask);

    b_trim(res);

    return res;
}

void b_free(bignum_t a) {
    if (a == NULL) return;
    free(a->data);
    free(a);
}

bignum_t b_copy(bignum_t a) {
    bignum_t res = b_init(a->size);

    for (int i = 0; i < res->size; i++) {
        res->data[i] = a->data[i];
    }

    res->sign = a->sign;
}

/**
 * trims leading 0 bytes in place
 */
void b_trim(bignum_t a) {


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

void b_pad(bignum_t a, unsigned int size) {
    unsigned char *temp = realloc(a->data, size);
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
        bignum_t t1 = b_copy(a); t1->sign = 0;
        bignum_t t2 = b_copy(b); t2->sign = 0;
        bignum_t res = b_add(t1, t2);
        b_free(t1);
        b_free(t2);
        res->sign = 1;
        return res;
    }

    // a neg => (-a) + b
    if (a->sign != 0 && b->sign == 0) {
        // same as b - a
        bignum_t t1 = b_copy(a); t1->sign = 0;
        bignum_t res = b_sub(b, t1);
        b_free(t1);
        return res;
    }

    // b neg => a + (-b)
    if (a->sign == 0 && b->sign != 0) {
        // same as a - b
        bignum_t t1 = b_copy(b); t1->sign = 0;
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
        bignum_t t1 = b_copy(a); t1->sign = 0;
        bignum_t t2 = b_copy(b); t2->sign = 0;

        bignum_t res = b_sub(t2, t1);
        b_free(t1);
        b_free(t2);
        return res;
    }

    // a neg => (-a) - b
    if (a->sign != 0 && b->sign == 0) {
        // same as -(a + b)
        bignum_t t1 = b_copy(a); t1->sign = 0;
        bignum_t t2 = b_copy(b); t1->sign = 0;

        bignum_t res = b_add(t1, t2);
        b_free(t1);
        b_free(t2);
        res->sign = 1;
        return res;
    }

    // b neg => a - (-b)
    if (a->sign == 0 && b->sign != 0) {
        // same as a + b
        bignum_t t1 = b_copy(b); t1->sign = 0;
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
        return b_init(1);
    }

    if (largest == b) {
        printf("reverseing\n");
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
    smallest = b_copy(smallest);
    smallest->sign = 0;
    bignum_t largest = b_largest(a, b);
    largest = b_copy(largest);
    largest->sign = 0;

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
 * n - numerator
 * d - denominator
 * r - remainder (mod) - this will be overwritten
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
    bignum_t ns = b_copy(n); ns->sign = 0;


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
 * Modulo operation
 * @param a - the value
 * @param m - the modulo base
 * @returns a new pointer to the result
 */
bignum_t b_mod(bignum_t a, bignum_t m) {\
    bignum_t value = b_copy(a); b_trim(value); value->sign = 0;
    bignum_t base = b_copy(m); b_trim(base); base->sign = 0;

    if (b_comp(value, base) < 0) {
        // if m is bigger than a already, return a
        b_free(base);
        return value;
    }

    // we need to make m bigger than a, so get the difference in bytes (plus 1)
    int bits_dif = (b_bytes(value) - b_bytes(base) + 1) * 8;

    b_lsip(base, bits_dif);

    while(b_comp(value, base) < 0) {
        b_rsip(base, 1);
        bits_dif--;
    }

    for (int i = 0; i <= bits_dif; i++) {
        // if we can subtract the base from the value, do so
        if (b_comp(value, base) >= 0) {
            bignum_t temp = value;
            value = b_sub(value, base);
            b_free(temp);
        }

        // shift the base down one
        b_rsip(base, 1);
    }

    b_trim(value);
    b_free(base);

    return value;
}

/**
 * Modulo in place
 * @param a - the value - will be reduced to the result
 * @param m - the modulo base
 */
void b_modip(bignum_t *a, bignum_t m) {
    bignum_t temp = (*a);
    bignum_t res = b_mod((*a), m);
    (*a) = res;
    b_free(temp);
}

/**
 * Exponent function
 * @param b - base
 * @param e - exponent
 * @returns b^(e)
 */
bignum_t b_exp(bignum_t b, bignum_t e) {
    bignum_t zero = b_init(1);
    bignum_t base = b_copy(b); base->sign = 0;
    bignum_t exp = b_copy(e); exp->sign = 0;

    // set our result to 1
    bignum_t res = b_init(1);
    b_oriip(res, 1);

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

/**
 * Modulo Exponent function
 * @param b - base
 * @param e - exponent
 * @param m - the modulo base
 * @returns b^(e) mod m
 */
bignum_t b_mexp(bignum_t b, bignum_t e, bignum_t m) {

    /**
     * Using the property that (a * b) mod n = (a mod n) * (b mod n) mod n,
     * we just mod everything before multiplication to keep the values down a bit
     * otherwise it should be the exact same as regular exponentiation
     */

    bignum_t zero = b_init(1);
    bignum_t base = b_mod(b, m); base->sign = 0;
    bignum_t exp = b_copy(e); exp->sign = 0;


    // set our result to 1
    bignum_t res = b_init(1);
    b_oriip(res, 1);

    int i = 0;
    while(b_comp(exp, zero) >= 1) {
        if (b_andi(exp, 1)) {

            bignum_t temp = res;

            res = b_mul(res, base);

            b_free(temp);
            b_modip(&res, m);
        }

        // square it and mod m
        bignum_t temp = base;
        base = b_mul(base, base);
        b_free(temp);
        b_modip(&base, m);


        i++;
        b_rsip(exp, 1);
    }


    if (b->sign) res->sign = (b_andi(e, 1)) ? 1 : 0;

    b_free(zero);
    b_free(base);
    b_free(exp);

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
    bignum_t zero = b_init(1);
    if (b_comp(a, zero) == 0) return "0";

    bignum_t n = b_copy(a);
    bignum_t ten = b_initv(10);
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

    char *str = malloc(sizeof(char) * (len + 1));

    for (int i = 0; i < len; i++) {
        bignum_t temp = n;
        n = b_div(n, ten, &rem);
        b_free(temp);
        if (b_toi(rem) >= 10) printf("aaaaaa %d\n", b_toi(rem));
        str[len - 1 - i] = b_toi(rem) + '0';
        b_free(rem);
    }
    str[len] = '\0';
    b_free(zero); 
    b_free(ten); 
    b_free(n); 
    return str;
}

int main(int argc, char** argv) {

    printf("hello sir\n");

    bignum_t temp = b_init(1);
    for (int i = 0; i < 10; i++) b_is_prime(temp);
    b_free(temp);
    return 0;

    if (argc != 4) {
        fprintf(stderr, "usage: ./test <num1> <op> <num2>");
        exit(1);
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[3]);

    bignum_t b_a = b_initv(a); b_trim(b_a);
    bignum_t b_b = b_initv(b); b_trim(b_b);

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