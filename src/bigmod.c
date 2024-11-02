#include "main.h"

/**
 * Modular addition
 * @param a - first number to be added (result is stored in here)
 * @param b - second number to be added (guaranteed not clobbered)
 * @param n - modular number (guaranteed not clobbered)
 * 
 * NB both numbers are assumed to be positive, and the absolute values will be added
 */
void b_madd(bignum_t a, bignum_t b, bignum_t n) {

    madd_calls++;

    // printf("\t\t\t\ta = %s\t", b_tostr(a));
    // printf("b = %s\n", b_tostr(b));

    b_modip(a, n);
    b_modip(b, n);

    // printf("\t\t\t\ta = %s\t", b_tostr(a));
    // printf("b = %s\n", b_tostr(b));

    // we need to make sure there is enough room in a to store the result
    b_pad(a, b_bytes(n) + 1);
    b_pad(b, b_bytes(n) + 1);

    // clock_t s = clock();

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

        // printf("\t\t\t\ti=%d: a=%d, b=%d, carry=%d ", i, a_byte, b_byte, carry);

        if (a_i + b_i + c_i > overflow_check) carry = 1;
        else carry = 0;

        // printf("next carry = %d (a_i=%d, b_i=%d, c_i=%d)\n", carry, a_i, b_i, c_i);
        
    }

    // clock_t e = clock();

    // madd_time += (double)(e - s) / CLOCKS_PER_SEC;


    b_modip(a, n);

    b_trim(a);
}

/**
 * Subtraction in place
 * @param a - number subtracted from (result is stored in here)
 * @param b - number to be subtracted (guaranteed not clobbered)
 * 
 * NB both numbers are assumed to be positive, and absolute values are subtracted
 */
void b_subip(bignum_t a, bignum_t b) {

    subip_calls++;

    // if the numbers are equal, then the result is obviously 0
    if (b_comp(a, b) == 0) {
        free(a->data);
        a->size = 1;
        a->data = calloc(1, sizeof(unsigned char));
        return;
    }

    if (b_comp(a, b) < 0) {
        // subtracting a bigger number
        // can do this by recalling fn with args swapped, then make neg
        bignum_t temp = b_copy(b);
        b_subip(temp, a);
        free(a->data);
        a->size = temp->size;
        a->data = temp->data;
        a->sign = 1;
        free(temp);
        return;
    }

    // clock_t s = clock();

    // from here, we know that a is the larger number
    

    // init variables for subtraction
    unsigned char carry = 0;
    unsigned char a_byte = 0;
    unsigned char b_byte = 0;

    b_pad(b, b_bytes(a));

    for (int i = 0; i < b->size; i++) {

        a_byte = a->data[i];
        b_byte = b->data[i];

        a->data[i] = a_byte - b_byte - carry;

        if ((int)(b_byte + carry) > (int)a_byte) {
            // we need to borrow from the next bit
            carry = 1;
        } else carry = 0;
    }

    // clock_t e = clock();

    // subip_time += (double)(e - s) / CLOCKS_PER_SEC;

    b_trim(b);
}

/**
 * Modular Subtraction
 * @param a - number subtracted from (result is stored in here)
 * @param b - number to be subtracted
 * @param n - modular number
 * 
 * NB both numbers are assumed to be positive, and absolute values are subtracted
 */
bignum_t b_msub(bignum_t a, bignum_t b, bignum_t n) {
    b_modip(a, n);
    b_modip(b, n);

    msub_calls++;

    // if the numbers are equal, then the result is obviously 0
    if (b_comp(a, b) == 0) return b_initc(0);

    if (b_comp(a, b) < 0) {
        // subtracting a bigger number
        // can do this by recalling fn with args swapped, then make neg
        bignum_t intermediate = b_copy(b);
        b_msub(intermediate, a, n);
        bignum_t res = b_copy(n);
        b_subip(res, intermediate);
        b_free(intermediate);
        return res;
    }

    b_subip(a, b);

    b_modip(a, n);
}

/**
 * Modulo Left Shift In Place
 * @param a - the number to shift (result is stored here)
 * @param shift - the shift in bits
 * @param n - the modulus
 */
void b_mlsip(bignum_t a, unsigned int shift, bignum_t n) {

    mlsip_calls++;

    // clock_t s = clock();

    unsigned int oldSize = a->size;
    unsigned int bitsnum = a->size * 8 + shift;
    unsigned int bytesnum = bitsnum / 8 + 1;
    unsigned int bitshift = (shift % 8);
    unsigned int byteshift = shift / 8; // truncated we hope

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
    }

    a->data[byteshift] = temp;

    // make sure we put in the new zeros
    for (int i = 0; i < shift; i++) {
        unsigned char temp = ~(0x01 << (i % 8));
        a->data[i / 8] &= temp;
    }

    // clock_t e = clock();

    // mlsip_time += (double)(e - s) / CLOCKS_PER_SEC;

    b_modip(a, n);

}

/**
 * Modular Multiply
 * @param a - the first number to be multiplied (result is stored here)
 * @param b - the second number to be multiplied
 * @param n - the modulus
 * 
 * NB all numbers are assumed to be positive, and absolute values are multiplied
 */
void b_mmul(bignum_t a, bignum_t b, bignum_t n) {

    mmul_calls++;
    clock_t s = clock();
    clock_t e;


    b_modip(a, n);
    b_modip(b, n);

    // gets the smallest and largest numbers
    bignum_t smallest = b_acopy(b_smallest(a, b));
    bignum_t largest = b_acopy(b_largest(a, b));

    // set a to zero
    b_subip(a, a);

    // printf("\t\tlargest = %s, smallest = %s, a = %s\n", b_tostr(largest), b_tostr(smallest), b_tostr(a));

    // printf("\t\tsmallest hex: %s\n", b_tohex(smallest));

    // s = clock();

    int since_last_shift = 0;

    for (int i = 0; i < smallest->size; i++) {
        for (int j = 0; j < 8; j++) {
            if (smallest->data[i] & (1 << j)) {
                // printf("\t\t\ti=%d, j=%d: shit just got real: shifting largest = %s by %d mod %s\n", i, j, b_tostr(largest), since_last_shift, b_tostr(n));
                // ls the value we are adding one place

                // e = clock();
                // mmul_time += (double)(e - s) / CLOCKS_PER_SEC;

                b_mlsip(largest, since_last_shift, n);

                // printf("\t\t\tAdding largest = %s to a = %s mod %s\n", b_tostr(largest), b_tostr(a), b_tostr(n));

                // add the shift
                b_madd(a, largest, n);

                // printf("\t\t\tnow a = %s\n", b_tostr(a));

                // s = clock();

                since_last_shift = 0;
            }
            since_last_shift++;
        }
    }

    e = clock();
    mmul_time += (double)(e - s) / CLOCKS_PER_SEC;

    b_free(largest);
    b_free(smallest);
}

/**
 * Modulo operation
 * @param a - the value
 * @param m - the modulo base
 * @returns a new pointer to the result
 * 
 * NB All values are assumed to be positive, and all values returned are positive
 */
bignum_t b_mod(bignum_t a, bignum_t m) {

    if (b_comp(a, m) < 0) {
        // if m is bigger than a already, return a
        return b_acopy(a);
    }

    bignum_t value = b_acopy(a);
    bignum_t base = b_acopy(m);

    // we need to make base bigger than value, so get the difference in bytes to guarantee base is bigger
    int bits_dif = (b_msb(value) >= b_msb(base)) ? (b_bytes(value) - b_bytes(base) + 1) * 8 : (b_bytes(value) - b_bytes(base)) * 8;

    b_lsip(base, bits_dif);

    while(b_comp(value, base) < 0) {
        b_rsip(base, 1);
        bits_dif--;
    }

    for (int i = 0; i <= bits_dif; i++) {
        // if we can subtract the base from the value, do so
        if (b_comp(value, base) >= 0) {
            b_subip(value, base);
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
 * @param value - the value - will be reduced to the result
 * @param m - the modulo base
 */
void b_modip(bignum_t value, bignum_t base) {

    modip_calls++;
    clock_t s;
    clock_t e;

    // bignum_t temp = (*a);
    // bignum_t res = b_mod((*a), m);
    // (*a) = res;
    // b_free(temp);

    if (b_comp(value, base) < 0) {
        // if m is bigger than a already, return a
        return;
    }

    s = clock();


    value->sign = 0;
    base->sign = 0;

    // we need to make base bigger than value, so get the difference in bytes to guarantee base is bigger
    int bits_dif = (b_bytes(value) - b_bytes(base) + 1) * 8;

    b_lsip(base, bits_dif);

    while(b_comp(value, base) < 0) {
        b_rsip(base, 1);
        bits_dif--;
    }

    b_lsip(base, 1);

    for (int i = 0; i <= bits_dif; i++) {
        // shift the base down one
        b_rsip(base, 1);

        // if we can subtract the base from the value, do so
        if (b_comp(value, base) >= 0) {
            // e = clock();

            // modip_time += (double)(e - s) / CLOCKS_PER_SEC;
            b_subip(value, base);
            // s = clock();
        }
    }

    b_trim(value);

    e = clock();

    modip_time += (double)(e - s) / CLOCKS_PER_SEC;
}

/**
 * Modulo Exponent function
 * @param b - base
 * @param e - exponent
 * @param m - the modulo base
 * @returns b^(e) mod m
 */
bignum_t b_mexp(bignum_t b, bignum_t e, bignum_t m) {

    // printf("b = %s\n", b_tostr(b));
    // printf("e = %s\n", b_tostr(e));
    // printf("m = %s\n", b_tostr(m));

    mexp_calls++;
    clock_t s = clock();
    clock_t end;

    /**
     * Using the property that (a * b) mod n = (a mod n) * (b mod n) mod n,
     * we just mod everything before multiplication to keep the values down a bit
     * otherwise it should be the exact same as regular exponentiation
     */

    bignum_t zero = b_initc(0);
    bignum_t base = b_mod(b, m);
    bignum_t exp = b_acopy(e);

    // set our result to 1
    bignum_t res = b_initc(1);

    // printf("mod = %s\n", b_tostr(m));

    // printf("zero = %s, base = %s, exp = %s, res = %s\n", b_tostr(zero), b_tostr(base), b_tostr(exp), b_tostr(res));

    int i = 0;

    // printf("\nmexp\n");

    bignum_t bc = b_acopy(base);
    bignum_t ec = b_acopy(exp);
    bignum_t mc = b_acopy(m);

    while(b_comp(exp, zero) >= 1) {
        // printf("\niteration %d:\n", i);
        if (b_andi(exp, 1)) {
            // printf("doing\n");
            // end = clock();
            // printf("\tmod = %s\n", b_tostr(m));
            // printf("\tmultiplying res = %s by base = %s:\n", b_tostr(res), b_tostr(base));

            // mexp_time += (double)(end - s) / CLOCKS_PER_SEC;
            // multiply by base mod m
            

            // bignum_t res2 = b_acopy(res);
            // bignum_t resi = b_acopy(res);
            // bignum_t base2 = b_acopy(base);

            bignum_t temp = res;
            res = b_fftmul(res, base);
            // b_mmul(temp, temp, m);
            b_free(temp);

            // temp = res;
            // res = b_mul(res, base);
            // b_free(temp);

            // if (b_comp(res, res2) != 0) {
            //     printf("ok wtf\n");

            //     printf("m           = %s\n", b_tostr(m));

            //     printf("normal      = %s\n", b_tostr(res));
            //     b_print(res);
            //     printf("fft         = %s\n", b_tostr(res2));
            //     b_print(res2);

            //     printf("initial res = %s\n", b_tostr(resi));
            //     printf("normal base = %s\n", b_tostr(base));
            //     printf("fft base    = %s\n", b_tostr(base2));


            //     printf("initial base     = %s\n", b_tostr(bc));
            //     printf("initial exponent = %s\n", b_tostr(ec));
            //     printf("initial modulus  = %s\n", b_tostr(mc));
            //     exit(1);
            // }

            b_modip(res, m);

            // b_free(base2);
            // b_free(res2);
            // b_free(resi);

            // printf("\tnew res = %s\n", b_tostr(res));
            // s = clock();
        }
        // end = clock();

        // mexp_time += (double)(end - s) / CLOCKS_PER_SEC;
        // square base mod m
        bignum_t temp = base;
        base = b_fftmul(base, base);
        b_modip(base, m);
        // b_mmul(temp, temp, m);
        b_free(temp);

        // printf("new base = %s\n", b_tostr(base));

        // s = clock();

        // move the exponent down
        b_rsip(exp, 1);

        i++;
    }

    b_free(bc);
    b_free(ec);
    b_free(mc);

    if (b->sign) res->sign = (b_andi(e, 1)) ? 1 : 0;

    b_free(zero);
    b_free(base);
    b_free(exp);

    end = clock();

    mexp_time += (double)(end - s) / CLOCKS_PER_SEC;

    return res;
}