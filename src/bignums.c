#include "main.h"

#include <math.h>

const unsigned char sign_mask = 0x01;

bignum_t b_gen() {

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
    if (b_bytes(a) == b_bytes(b)) {
        bignum_t retval = (b_msb(a) > b_msb(b))? a:b;
        return retval;
    }
    if (b_bytes(a) < b_bytes(b)) return b;
    return a;
}

bignum_t b_smallest(bignum_t a, bignum_t b) {
    if (b_bytes(a) == b_bytes(b)) {
        bignum_t retval = (b_msb(a) < b_msb(b))? a:b;
        return retval;
    }
    if (b_bytes(a) > b_bytes(b)) return b;
    return a;
}

unsigned int b_smallest_bytes(bignum_t a, bignum_t b) {
    return (b_bytes(b) < b_bytes(a)) ? b_bytes(b) : b_bytes(a);
}

// finds the size for storing an addition of the 2 bignums
unsigned int b_largest_bytes(bignum_t a, bignum_t b) {
    return (b_bytes(b) < b_bytes(a)) ? b_bytes(a) : b_bytes(b);
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


    return res;
}

void b_free(bignum_t a) {
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
bignum_t b_trim(bignum_t a) {


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

    return a;
}

bignum_t b_pad(bignum_t a, unsigned int size) {
    unsigned char *temp = realloc(a->data, size);
    if (temp == NULL) {
        fprintf(stderr, "Error in padding bignum! Program not exiting\n");
        return a;
    }

    if (size > a->size) {
        for (int i = a->size; i < size; i++) temp[i] = 0;
    }

    a->data = temp;
    a->size = size;
    return a;
}

/**
 * Or Immediate In Place
 */
void b_oriip(bignum_t a, unsigned char val) {
    a->data[0] |= val;
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
        smallest = b_pad(smallest, largest->size);
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

    res = b_trim(res);

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
    
    b = b_pad(b, a->size);

    for (int i = 0; i < a->size; i++) {

        res->data[i] = a->data[i] - b->data[i] - carry;

        if (b->data[i] + carry > a->data[i]) {
            // we need to borrow from the next bit
            carry = 1;
        } else carry = 0;
    }

    return res;
}

bignum_t b_lshift(bignum_t a, unsigned int shift) {
    unsigned int bitsnum = a->size * 8 + shift;
    unsigned int bytesnum = bitsnum / 8 + 1;

    bignum_t res = b_init(bytesnum);

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

    res = b_trim(res);

    return res;
}

/**
 * Left Shift In Place
 */
void b_lsip(bignum_t a, unsigned int shift) {
    bignum_t res = b_lshift(a, shift);
    b_free(a);
    a = res;
}


// not complete
bignum_t b_rshift(bignum_t a, unsigned int shift) {
    bignum_t res = b_init(b_bytes(a));

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
 */
void b_rsip(bignum_t a, unsigned int shift) {
    bignum_t res = b_rshift(a, shift);
    b_free(a);
    a = res;
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
    res->sign = a->sign ^ b->sign;

    for (int i = 0; i < smallest->size; i++) {
        for (int j = 0; j < 8; j++) {
            if (smallest->data[i] & (1 << j)) {
                // need to add the shift
                bignum_t temp = b_lshift(largest, i * 8 + j);
                res = b_add(res, temp);
                b_free(temp);
            }
        }
    }

    b_free(smallest);
    b_free(largest);

    res = b_trim(res);

    return res;
}

/**
 * n - numerator
 * d - denominator
 * r - remainder (mod) - this will be overwritten
 * 
 * @returns q, the quotient of the division
 */
bignum_t b_div(bignum_t n, bignum_t d, bignum_t r) {
    b_trim(n);
    b_trim(d);

    if (r != NULL) b_free(r);

    bignum_t q = b_init(b_bytes(n));

    q->sign = n->sign ^ d->sign;

    int shift = b_bytes(n) - b_bytes(d);
    if (shift < 0) return q; // if the denom is bigger, doesn't divide into

    // try align bytes in the copies
    bignum_t ds = b_lshift(d, shift); ds->sign = 0;
    bignum_t ns = b_copy(n); ns->sign = 0;

    shift *= 8; // convert to bits shifted

    while(b_largest(ns, ds) == ns) {
        b_lsip(ds, 1);
        shift++;
    }
    // we now know ds > ns, so shift back one place
    b_rsip(ds, 1); shift--;

    bignum_t t;
    for (int i = 0; i < shift; i++) {
        if (b_largest(ns, ds) == ns) {
            t = b_sub(ns, ds);
            b_free(ns);
            ns = t;
            b_oriip(q, 1);
        }
        b_rsip(ds, 1);
        b_lsip(q, 1);
    }

    r = ns;

    return q;
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

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: ./test <num1> <op> <num2>");
        exit(1);
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[3]);

    bignum_t b_a = b_initv(a); b_a = b_trim(b_a);
    bignum_t b_b = b_initv(b); b_b = b_trim(b_b);

    printf("Initial inputs as bignum:\nA:\n");
    b_print(b_a);
    printf("B:\n");
    b_print(b_b);

    int res;
    bignum_t b_res;
    bignum_t b_res_2;

    printf("inputted: %d %d\n", a, b);
    printf("hex: 0x%04X 0x%04X\n\n", a, b);

    printf("recieved %s\n", argv[2]);

    if (strcmp(argv[2], "ls") == 0) {
        printf("left shifting...\n\n");

        res = a << b;
        b_res = b_lshift(b_a, b);
    } else if (strcmp(argv[2], "rs") == 0) {
        printf("right shifting...\n\n");

        res = a >> b;
        b_res = b_rshift(b_a, b);
    } else if (strcmp(argv[2], "mul") == 0) {
        printf("multiplying...\n\n");

        res = a * b;
        b_res = b_mul(b_a, b_b);
    } else if (strcmp(argv[2], "div") == 0) {
        printf("dividing...\n\n");

        res = a / b;
        b_res = b_div(b_a, b_b, b_res_2);

        printf("ints mod: %d\n", a % b);
        printf("bignum mod: \n");
        b_print(b_res_2);
        printf("\n\nquotient:\n");
    }
    else {
        printf("Invalid operation!\n");
        return 1;
    }

    printf("ints: %d (0x%04X)\n", res, res);
    b_print(b_res);
}