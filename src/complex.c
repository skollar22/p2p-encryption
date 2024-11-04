#include "main.h"

#include <math.h>

/**
 * Functions for the complex representation used in the FFT multiplication
 */

bncplx_t c_construct(long double real, long double cplx) {
    bncplx_t res = malloc(sizeof(struct bignum_complex));

    res->real = real;
    res->cplx = cplx;
    return res;
}

bncplx_t c_copy(bncplx_t a) {
    return c_construct(a->real, a->cplx);
}

bncomp_t c_allocarr(unsigned int n) {
    bncomp_t res = malloc(sizeof(struct bignum_comp_chain));
    res->length = n;
    res->values = malloc(sizeof(struct bignum_complex) * n);
    for (int i = 0; i < n; i++) {
        res->values[i] = c_construct(0L, 0L);
    }
    return res;
}

void c_free(bncomp_t a) {
    if (a == NULL) return;
    if (a->values == NULL) {
        free(a);
        return;
    }
    for (int i = 0; i < a->length; i++) {
        if (a->values[i] == NULL) continue;
        c_freec(a->values[i]);
    }
    free(a->values);
    free(a);
}

void c_freec(bncplx_t a) {
    if (a == NULL) return;
    free(a);
}

void c_print(bncplx_t a, const char *name) {
    printf("Printing %s\t: %Lf + j %Lf\n", name, a->real, a->cplx);
}

void c_addip(bncplx_t a, bncplx_t b) {
    a->real += b->real;
    a->cplx += b->cplx;
}

bncplx_t c_add(bncplx_t a, bncplx_t b) {
    return c_construct(a->real + b->real, a->cplx + b->cplx);
}

void c_subip(bncplx_t a, bncplx_t b) {
    a->real -= b->real;
    a->cplx -= b->cplx;
}

bncplx_t c_sub(bncplx_t a, bncplx_t b) {
    return c_construct(a->real - b->real, a->cplx - b->cplx);
}

void c_mulip(bncplx_t a, bncplx_t b) {
    long double ar = a->real;
    long double ac = a->cplx;
    long double br = b->real;
    long double bc = b->cplx;
    a->real = (ar * br) - (ac * bc);
    a->cplx = (ar * bc) + (ac * br);
}

bncplx_t c_mul(bncplx_t a, bncplx_t b) {
    return c_construct((a->real * b->real) - (a->cplx * b->cplx), (a->real * b->cplx) + (a->cplx * b->real));
}

/**
 * Turns x + iy into r*exp(theta)
 * r is stored in the real part
 * theta is stored in the complex part
 */
void c_magph(bncplx_t a) {
    long double real = a->real;
    long double cplx = a->cplx;
    a->real = sqrtl(real * real + cplx + cplx);
    a->cplx = atanl(cplx / real);
}