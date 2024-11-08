#include "main.h"
#include <unistd.h>

/**
 * Key generation
 * Keys are stored in key and key.pub in the folder that the program was run in
 */
void keygen() {
     // check if the key files exist
    if ((access("key", F_OK) == 0) && (access("key.pub", F_OK) == 0)) {
        printf("Keys already exist in this directory. Overwrite? [y/n]: ");
        char overwrite;
        scanf("%c", &overwrite);
        if (overwrite != 'y' && overwrite != 'Y') return;
    }

    printf("working...\n");

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

    FILE *pub = fopen("key.pub", "w");
    FILE *prv = fopen("key", "w");

    fprintf(pub, "%s %s\n", b_tohex(pq), b_tohex(e));
    fprintf(prv, "%s %s\n", b_tohex(pq), b_tohex(d));

    fclose(pub);
    fclose(prv);

    return;
}

/**
 * Gets the length of a string
 * (I know there is a std library version, but I didn't trust it with my scuffed strings)
 */
unsigned int strlength(char *str) {
    unsigned int text_length = 0;
    for (int i = 0; str[i] != '\0'; i++) text_length++;
    return text_length;
}

/**
 * Encrypt function
 * @param plaintext - initial message as a string
 * @param block_no - the number of blocks the message was broken into is stored in here
 * @returns a list of unsigned char values that is the encrypted string
 */
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

/**
 * Encrypt block function
 * @param plaintext - plaintext block as a string
 * @param pq - the modulus for encryption
 * @param e - public encryption exponent
 */
char * encryptb(char *plaintext, bignum_t pq, bignum_t e) {

    printf("encrypting block...\n");

    if (plaintext == NULL) {
        printf("Nothing to encrypt, exiting...\n");
        return NULL;
    }
    
    // find the size of the text (in bytes)
    unsigned int text_length = BLOCK_LEN;

    bignum_t bnmessage = b_fromstr(plaintext, text_length);

    bignum_t cypher = b_mexp(bnmessage, e, pq);

    b_free(bnmessage);

    // return cyphertext
    char *ret = cypher->data;
    free(cypher);
    return ret;
}

/**
 * Decrypt function
 * @param ciphertext - initial ciphertext as a string
 * @param blocks - the number of blocks the message was broken into
 * @returns the unencrypted message as a a string
 */
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

/**
 * Decrypt block
 * @param ciphertext - the ciphertext block to decrypt
 * @param pq - the modulus for decryption
 * @param d - the private decryption exponent
 */
char * decryptb(char *ciphertext, bignum_t pq, bignum_t d) {

    printf("decrypting block...\n");

    if (ciphertext == NULL) {
        printf("Nothing to decrypt, exiting...\n");
        return "";
    }

    bignum_t bnmessage = b_fromstr(ciphertext, KEY_LENGTH);

    bignum_t plain = b_mexp(bnmessage, d, pq);

    b_free(bnmessage);

    char *ret = plain->data;
    free(plain);
    return ret;
}