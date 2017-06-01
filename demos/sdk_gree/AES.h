
#ifndef __AES_H__
#define __AES_H__



#define AES_KEY_LEN 	16 

#ifndef AES_BLOCK_SIZE
#define	AES_BLOCK_SIZE	16
#endif



extern char *AES_N2;
extern uint8_t g_uAesKey[16];
extern A_BOOL bAesValid;



int aes_cbc_decrypt(uint8_t *key, uint8_t * cipher, int cipLen, uint8_t * plain);
int _aes_cbc_invCipher(const uint8_t *key, const uint8_t *iv, uint8_t *cipher, int cip_len, uint8_t *plain);
int aes_PKCS5_dePadding(uint8_t *plain, int len);









int aes_cbc_encrypt(uint8_t *key, uint8_t *plain, int plain_len, uint8_t *cipher);
int aes_PKCS5_padding(uint8_t *plain, int plen);
int _aes_cbc_cipher(const uint8_t *key, const uint8_t *iv, uint8_t *plain, int plain_len, uint8_t *cipher);












#endif 
