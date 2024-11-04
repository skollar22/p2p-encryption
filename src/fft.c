#include "main.h"

# define M_PIl		3.141592653589793238462643383279502884L
#include <math.h>

#define EVEN 0
#define ODD 1

/**
 * FFT multiplication function
 */
bignum_t b_fftmul(bignum_t a, bignum_t b) {

    fftmul_calls++;

    clock_t s = clock();
    clock_t e;

    // find the size of vector we need to do the fft
    unsigned int mask = 1;
    unsigned int size = ((a->size > b->size) ? a->size : b->size) * 2;
    while (size >= mask) mask = mask << 1;

    // split the bignums into vectors
    bncomp_t inita = b_split_init(a, mask);
    bncomp_t initb = b_split_init(b, mask);


    // ie exp(-2*i*pi/n)
    bncplx_t omega = c_construct(cosl(2 * M_PIl / mask), - sinl(2 * M_PIl / mask));

    // Perform the FFT on both vectors
    bncomp_t resa = b_fftr(inita, omega);
    bncomp_t resb = b_fftr(initb, omega);

    // multiply values together and then divide by our n (mask)
    for (int i = 0; i < mask; i++) {
        c_mulip(resa->values[i], resb->values[i]);
        resa->values[i]->real /= mask;
        resa->values[i]->cplx /= mask;
    }

    // to get the actual values again, fft with negative exponent
    // ie exp(2*i*pi/n)
    c_freec(omega);
    omega = c_construct(cosl(2 * M_PIl / mask), sinl(2 * M_PIl / mask));

    // perform the FFT on the result vector
    clock_t temp = clock();
    bncomp_t res = b_fftr(resa, omega);
    msub_time += (double) (clock() - temp) / CLOCKS_PER_SEC;

    bignum_t final = b_init(mask + 5);
    unsigned long long carry = 0;

    // Now we convert the result vector back into a bignum
    for (int i = 0; i < mask; i++) {
        carry += (long long) (res->values[i]->real + 0.5L);
        final->data[i] = (unsigned char)(carry & 0xFF);
        carry = carry >> 8;
    }
    for (int i = mask; carry > 0; i++) {
        final->data[i] = carry & 0xFF;
        carry = carry >> 8;
    }

    c_free(res);
    c_free(resb);
    c_freec(omega);
    
    b_trim(final);

    e = clock();
    fftmul_time += (double) (e - s) / CLOCKS_PER_SEC;

    return final;

}

/**
 * Print a vector used in the FFT
 */
void b_print_comp(bncomp_t a) {
    printf("printing bncomp_t at %p (length=%u):\n", a, a->length);
    for (int i = 0; i < a->length; i++) {
        printf("Element #%d (ie %Lf's place): %Lf + %Lf i\n", i, powl(256.0L, i), a->values[i]->real, a->values[i]->cplx);
    }
    printf("\n");
}

/**
 * Split a bignum into an FFT vector
 * @param a - the bignum
 * @param n - the vector max size (should be a power of 2)
 */
bncomp_t b_split_init(bignum_t a, unsigned int n) {
    bncomp_t ret = c_allocarr(n);
    for (int i = 0; i < a->size; i++) {
        ret->values[i]->real = (long double) a->data[i];
        ret->values[i]->cplx = 0.0L;
    }
    for (int i = a->size; i < n; i++) {
        ret->values[i]->real = 0.0L;
        ret->values[i]->cplx = 0.0L;
    }
    return ret;
}

/**
 * Get the even entries from a FFT vector
 */
bncomp_t b_split_even(bncomp_t a) {
    bncomp_t ret = c_allocarr(a->length >> 1);
    for (int i = 0; i < a->length; i += 2) {
        ret->values[i / 2]->real = a->values[i]->real;
        ret->values[i / 2]->cplx = a->values[i]->cplx;
    }
    return ret;
}

/**
 * Get the odd entries from a FFT vector
 */
bncomp_t b_split_odds(bncomp_t a) {
    bncomp_t ret = c_allocarr(a->length >> 1);
    for (int i = 1; i < a->length; i += 2) {
        ret->values[i / 2]->real = a->values[i]->real;
        ret->values[i / 2]->cplx = a->values[i]->cplx;
    }
    return ret;
}


/**
 * Recursive FFT function
 * @param a - the vector to be transformed
 * @param omega - The root of unity used
 */
bncomp_t b_fftr(bncomp_t a, bncplx_t omega) {

    unsigned int n = a->length;

    // base case
    if (n == 1) {
        return a;
    }

    // Get the even and odd entries of the input vector
    bncomp_t a_even = b_split_even(a);
    bncomp_t a_odd  = b_split_odds(a);

    // check
    if (a_even->length != a_odd->length) printf("fuck\n");

    // Perform the FFT on both even and odd vectors, with omega_squared as the root of unity
    bncplx_t omega_sqr = c_mul(omega, omega);
    bncomp_t y_even = b_fftr(a_even, omega_sqr);
    bncomp_t y_odd = b_fftr(a_odd, omega_sqr);

    // parameters for FFT combination
    bncplx_t x = c_construct(1.0L, 0.0L);
    bncplx_t temp_mul = NULL;
    bncplx_t temp_low = NULL;
    bncplx_t temp_high = NULL;

    // Combine the two recursed FFT vectors
    for (int i = 0; i < n/2; i++) {
        // optimisation
        temp_mul = c_mul(x, y_odd->values[i]);

        // y[i] = y_even[i] + x*y_odd[i]
        temp_low = c_add(y_even->values[i], temp_mul);

        // y[i + bl/2] = y_even[i] - x*y_odd[i] // reflective
        temp_high = c_sub(y_even->values[i], temp_mul);

        a->values[i]->real = temp_low->real;
        a->values[i]->cplx = temp_low->cplx;

        a->values[i + n/2]->real = temp_high->real;
        a->values[i + n/2]->cplx = temp_high->cplx;

        // Free variables for next round
        c_freec(temp_high);
        c_freec(temp_low);
        c_freec(temp_mul);

        // x *= omega
        c_mulip(x, omega);
    }

    // Free the even and odd FFT vectors
    c_free(y_even);
    c_free(y_odd);

    return a;
}