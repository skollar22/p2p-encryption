#include "main.h"
#include <unistd.h>

#define PRIME_LENGTH 32
#define KEY_LENGTH 64
#define BLOCK_LEN 63

void keygen() {
    // if (plaintext == NULL) return "";
    
    // // find the size of the text (in bytes)
    // unsigned int text_length = 0;
    // for (int i = 0; plaintext[i] != '\0'; i++) text_length++;

     // check if the key files exist
    if ((access("key", F_OK) == 0) && (access("key.pub", F_OK) == 0)) {
        printf("Keys already exist in this directory. Overwrite? [y/n]: ");
        char overwrite;
        scanf("%c", &overwrite);
        if (overwrite != 'y' && overwrite != 'Y') return;
    }

    printf("working...");

    // generate p and q
    bignum_t p = b_gen_prime(PRIME_LENGTH);
    bignum_t q = b_gen_prime(PRIME_LENGTH);

    // generate the 1-minus p and q
    bignum_t p_minus = b_minus(p);
    bignum_t q_minus = b_minus(q);

    // compute both products
    bignum_t pq = b_fftmul(p, q);
    bignum_t pq_minus = b_fftmul(p_minus, q_minus);

    // choose e
    bignum_t e = b_gen_prime(2);

    // find d
    bignum_t d = b_mmi(e, pq_minus);

    printf("\n");

    printf("p   = %s\n", b_tostr(p));
    printf("pm  = %s\n", b_tostr(p_minus));
    printf("q   = %s\n", b_tostr(q));
    printf("qm  = %s\n", b_tostr(q_minus));
    printf("pq  = %s\n", b_tostr(pq));
    printf("pqm = %s\n", b_tostr(pq_minus));
    printf("e   = %s\n", b_tostr(e));
    printf("d   = %s\n", b_tostr(d));


    FILE *pub = fopen("key.pub", "w");
    FILE *prv = fopen("key", "w");

    fprintf(pub, "%s %s\n", b_tohex(pq), b_tohex(e));
    fprintf(prv, "%s %s\n", b_tohex(pq), b_tohex(d));

    fclose(pub);
    fclose(prv);

    char message[12] = "hello there";

    bignum_t bnmessage = b_fromstr(message, 12);

    printf("created\n");

    // segfault somewhere here

    bignum_t cypher = b_mexp(bnmessage, e, pq);

    printf("encrypted: %s\n", b_tostr(cypher));

    bignum_t decode = b_mexp(cypher, d, pq);

    printf("decrypted: ");

    printf("%s\n", decode->data);

    unsigned int blocks;

    char * encrypted = encrypt(message, &blocks);

    printf("cypher at %p\n", cypher);

    // printf("in-house encrypt: %s\n", b_tohex(cypher));
    // printf("encrypt function: %s\n", b_tohex(encrypted));

    char *decrypted = decrypt(encrypted, blocks);

    printf("decrypted: %s\n", decrypted);

    return;

}

unsigned int strlength(char *str) {
    unsigned int text_length = 0;
    for (int i = 0; str[i] != '\0'; i++) text_length++;
    return text_length;
}

char * encrypt(char * plaintext, unsigned int *block_no) {

    printf("\nencrypting...\n");

    // check if the key files exist
    if ((!access("key", F_OK) == 0) || (!access("key.pub", F_OK) == 0)) {
        printf("Keys do not exist in this directory, generating new keys...\n");
        keygen();
    }

    // Read the encryption keys from key.pub
    FILE *pub = fopen("key.pub", "r");
    fseek(pub, 0L, SEEK_END);
    long int length = ftell(pub);
    fseek(pub, 0L, SEEK_SET);

    char *pq_str = malloc(sizeof(char) * length);
    char *e_str = malloc(sizeof(char) * length);
    int index = 0;
    for (; (pq_str[index] = fgetc(pub)) != ' '; index++) {
        ; ;
    }

    pq_str[index] = '\0';
    bignum_t pq = b_fromhex(pq_str, index + 1);

    index = 0;

    for (; (e_str[index] = fgetc(pub)) != '\n'; index++) {
        ; ;
    }

    e_str[index] = '\0';
    bignum_t e = b_fromhex(e_str, index + 1);

    fclose(pub);

    // split the text into blocks
    unsigned int text_length = strlength(plaintext);
    unsigned int blocks = text_length / ((unsigned int)BLOCK_LEN) + 1;
    if (text_length % BLOCK_LEN == 0) blocks--;

    *block_no = blocks;

    char *plaintext_aligned = calloc(blocks * BLOCK_LEN, sizeof(char));
    for (int i = 0; i < text_length; i++) {
        plaintext_aligned[i] = plaintext[i];
    }

    unsigned char * ciphertext = malloc(sizeof(unsigned char) * blocks * KEY_LENGTH);

    for (int i = 0; i < blocks; i++) {
        unsigned char * temp = encryptb(&(plaintext_aligned[i * BLOCK_LEN]), pq, e);
        for (int j = 0; j < KEY_LENGTH; j++) {
            ciphertext[i * KEY_LENGTH + j] = temp[j];
        }
        free(temp);
    }

    b_free(pq);
    b_free(e);

    return ciphertext;

}

char * encryptb(char *plaintext, bignum_t pq, bignum_t e) {

    printf("encrypting block...\n");

    if (plaintext == NULL) {
        printf("Nothing to encrypt, exiting...\n");
        return NULL;
    }
    
    // find the size of the text (in bytes)
    unsigned int text_length = BLOCK_LEN;

    bignum_t bnmessage = b_fromstr(plaintext, text_length);

    // from here

    bignum_t cypher = b_mexp(bnmessage, e, pq);

    b_free(bnmessage);

    // return cypher;
    char *ret = cypher->data;
    free(cypher);
    return ret;
}

char * decrypt(char * ciphertext, unsigned int blocks) {
    printf("\ndecrypting...\n");

    // check if the key files exist
    if ((!access("key", F_OK) == 0) || (!access("key.pub", F_OK) == 0)) {
        printf("Keys do not exist in this directory, generating new keys...\n");
        keygen();
    }

    // read the decryption keys
    FILE *prv = fopen("key", "r");
    fseek(prv, 0L, SEEK_END);
    long int length = ftell(prv);
    fseek(prv, 0L, SEEK_SET);

    char *pq_str = malloc(sizeof(char) * length);
    char *d_str = malloc(sizeof(char) * length);
    int index = 0;
    for (; (pq_str[index] = fgetc(prv)) != ' '; index++) ;;

    pq_str[index] = '\0';
    bignum_t pq = b_fromhex(pq_str, index + 1);

    index = 0;

    for (; (d_str[index] = fgetc(prv)) != '\n'; index++) ;;

    d_str[index] = '\0';
    bignum_t d = b_fromhex(d_str, index + 1);

    fclose(prv);

    unsigned char * plaintext = malloc(sizeof(unsigned char) * blocks * BLOCK_LEN);

    for (int i = 0; i < blocks; i++) {
        unsigned char * temp = decryptb(&(ciphertext[i * KEY_LENGTH]), pq, d);
        for (int j = 0; j < BLOCK_LEN; j++) {
            plaintext[i * BLOCK_LEN + j] = temp[j];
        }

        free(temp);
    }

    b_free(pq);
    b_free(d);

    return plaintext;
}

char * decryptb(char *ciphertext, bignum_t pq, bignum_t d) {

    printf("decrypting block...\n");

    if (ciphertext == NULL) {
        printf("Nothing to decrypt, exiting...\n");
        return "";
    }

    bignum_t bnmessage = b_fromstr(ciphertext, KEY_LENGTH);

    // from here

    bignum_t plain = b_mexp(bnmessage, d, pq);

    // b_pad(plain, BLOCK_LEN);

    b_free(bnmessage);

    char *ret = plain->data;
    free(plain);
    return ret;
}