
#include <qcom/qcom_security.h>
#include <qcom/qcom_common.h>

#include "AES.h"

char *AES_N2 = "456123";

uint8_t g_uAesKey[16];

A_BOOL bAesValid = FALSE;


 /*
 ****************************************************************************************
 * function name :  _aes_cbc_invCipher()
 *---------------------------------------------------------------------------------------
 *
 * @key :  aes key (128 bit = 16 Byte)
 * @iv    :  Encryption IV for CBC mode (16 bytes)
 * @cipher :  cipher text to be decrypt
 * @cip_len:  cipher test length, (must be divisible by 16)
 *
 * @plain :  plain text
 * @ret    :  0, on cipher success;  -1 or -2, on cipher fail
 *
********************************************************************************************
*/
int _aes_cbc_invCipher(const uint8_t *key, const uint8_t *iv, uint8_t *cipher, int cip_len, uint8_t *plain)
{
    void *ctx = NULL;
    uint8_t cbc[16], tmp[16];
    uint8_t *pos = cipher;
    int i, j, blocks;

    ctx = secur_aes_decrypt_init(key, 16);

    if (ctx == NULL)
        return -1;
    if((cip_len%16) != 0)
        return -2;

    memcpy(cbc, iv, 16);

    blocks = cip_len / 16;

    for (i = 0; i < blocks; i++)
    {
        memcpy(tmp, pos, 16);
        secur_aes_decrypt(ctx, pos, pos);

        for (j = 0; j < 16; j++)
            pos[j] ^= cbc[j];

        memcpy(cbc, tmp, 16);
        memcpy(plain, pos, 16);

        pos += 16;
        plain += 16;
    }

    secur_aes_decrypt_deinit(ctx);

    return 0;
}


/*
 *********************************************************
 * function name :  aes_PKCS5_dePadding()
 *---------------------------------------------------------
 * input
 * @plain:  plain text to be de_padding
 * @plen :  length of padded plain text
 *
 * output
 * @plain:  plain test without padding
 * @ret   :  length of  plain text without padding
 *
***********************************************************
*/
int aes_PKCS5_dePadding(uint8_t *plain, int len)
{
	int i;

	for (i = (len-1); i >= (len-16); i--) {
		if(plain[i] <= 16)
			plain[i] = 0;
		else
			break;
	}

	return i+1;
}


/*
 ********************************************************************
 * function name :  aes_cbc_decrypt()
 *-------------------------------------------------------------------
 * input
 * @key  :  aes key (128 bit = 16 Byte)
 * @cipher :  cipher text to be decrypt
 * @cipLen :  length of cipher text
 *
 * output
 * @plain :  plain text
 * @ret    :  length of  cipher text
 *
************************************************************************
*/
int aes_cbc_decrypt(uint8_t *key, uint8_t * cipher, int cipLen, uint8_t * plain)
{
     int res = -1;
     int plain_len;

     res = _aes_cbc_invCipher(key, key, cipher, cipLen, plain);

     if(res != 0)
     {
          printf("\n _aes_cbc_invCipher err, ret res = %d", res);
          return res;
     }
     else
     {
          //printf("_aes_cbc_invCipher ok: \n");
          //HEX_BUF_PRINT_ASC(plain, 33);

          //aes PKCS5 de_Padding
          plain_len = aes_PKCS5_dePadding(plain, cipLen);
          //printf("plain back len = %d \n", plain_len);
     }

     return plain_len;
}





/*
 ********************************************************************
 * function name :  aes_cbc_encrypt()
 *-------------------------------------------------------------------
 * input
 * @key  :  aes key (128 bit = 16 Byte)
 * @plain :  plain text to be encrypt
 * @plain_len:  length of plain text to be encrypt
 *
 * output
 * @cipher:  cipher text
 * @ret   :    length of  cipher text
 *
************************************************************************
*/
int aes_cbc_encrypt(uint8_t *key, uint8_t *plain, int plain_len, uint8_t *cipher)
{
	int res = -1;
	int new_len;

	//aes PKCS5 Padding
	new_len = aes_PKCS5_padding(plain, plain_len);

	res = _aes_cbc_cipher(key, key, plain, new_len, cipher);

	if (res != 0) {
		A_PRINTF("_aes_cbc_cipher err. \n");
		return res;
	} else {
		return new_len;
	}
}



/*
 *********************************************************
 * function name :  aes_PKCS5_padding()
 *---------------------------------------------------------
 * input
 * @plain:  plain text to be padding
 * @plen :  length of plain text to be padding
 *
 * output
 * @plain:  plain test padded
 * @ret   :  length of padded plain text
 *
***********************************************************
*/
int aes_PKCS5_padding(uint8_t *plain, int plen)
{
	int newlen;

	newlen = plen + (16 - plen % 16);

	return newlen;
}



 /*
 ****************************************************************************************
 * function name :  _aes_cbc_cipher()
 *---------------------------------------------------------------------------------------
 * @key :  aes key (128 bit = 16 Byte)
 * @iv    :  Encryption IV for CBC mode (16 bytes)
 * @plain:  plain text to be encrypt, (must be padded with PKCS5 padding)
 * @plain_len:   plain_length, (must be divisible by 16)
 *
 * @cipher:  cipher text
 * @ret     :  0, on cipher success;  -1 or -2, on cipher fail
 *
********************************************************************************************
*/
int _aes_cbc_cipher(const uint8_t *key, const uint8_t *iv, uint8_t *plain, int plain_len, uint8_t *cipher)
{
    void *ctx = NULL;
    uint8_t cbc[16];
    uint8_t *pos = plain;
    uint8_t *cip = cipher;
    int i, j, blocks;

    ctx = secur_aes_encrypt_init(key, 16);

    if (ctx == NULL)
        return -1;
    if(plain == NULL ||cipher == NULL)
        return -2;

    memcpy(cbc, iv, 16);

    blocks = plain_len / 16;

    for (i = 0; i < blocks; i++) {
        for (j = 0; j < 16; j++)
            cbc[j] ^= pos[j];

        secur_aes_encrypt(ctx, cbc, cbc);

        memcpy(cip, cbc, 16);
        pos += 16;
        cip += 16;
    }

    secur_aes_encrypt_deinit(ctx);

    return 0;
}
























