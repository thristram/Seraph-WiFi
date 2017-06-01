
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


#include "base64.h"



/* base64 to binary lookup table */
static const uint8_t map[128] =
{
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
    255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
    7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51, 255, 255, 255, 255, 255
};

static const char BASE64_TABLE[65] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '='
};

/******************************************************************


******************************************************************/
int getNumberOfPaddingChars(const int nSize)
{
	int rem = nSize%3;
	return (rem==0?0:3-rem);
}


/******************************************************************


******************************************************************/
void base64_encode(const char * data, const int size, uint8_t * outputBuffer)
{
	if(!data)
		return;

	if(size <= 0)
		return;

	int i = 0, j = 0, iter = size - (size%3);
	unsigned char c;
	int k = getNumberOfPaddingChars(size);

	while(i < iter)
	{
		c = ((unsigned char)data[i] >> 2) & 0x3F;
		outputBuffer[j] = BASE64_TABLE[c];
		c = (((unsigned char)data[i] << 4) & 0x30) | ((unsigned char)data[i+1] >> 4);
		outputBuffer[j+1] = BASE64_TABLE[c];
   		c = (((unsigned char)data[i+1] << 2) & 0x3C) | ((unsigned char)data[i+2] >> 6);
		outputBuffer[j+2] = BASE64_TABLE[c];
   		c = ((unsigned char)data[i+2]) & 0x3F;
		outputBuffer[j+3] = BASE64_TABLE[c];
		
		i+=3;
		j+=4;
	}
	if(k)
	{
		c = ((unsigned char)data[i] >> 2) & 0x3F;
		outputBuffer[j]= BASE64_TABLE[c];
		c = (i+1)<size?(((unsigned char)data[i] << 4) & 0x30) | ((unsigned char)data[i+1] >> 4):(((unsigned char)data[i] << 4) & 0x30);
		outputBuffer[j+1] = BASE64_TABLE[c];
		c = (i+2)==size?(((unsigned char)data[i+1] << 2) & 0x3C):64;
		outputBuffer[j+2] = BASE64_TABLE[c];
		outputBuffer[j+3] = BASE64_TABLE[64];
	}
}


/******************************************************************


******************************************************************/
int base64_decode(const uint8_t* data, const int size, uint8_t* outputBuffer)
{
   int bytes_converted = 0;	

   if(size%4 != 0)
	{
        printf("base64_decode length error!\n");
		return bytes_converted;
	}

	int i = 0, j = 0, iter = size - 4;

	while(j < iter)
	{
		//Check if the character is valid base64
		outputBuffer[i] = ((map[data[j]] << 2)&0xFC)|((map[data[j+1]] >> 4)&0x03);
		outputBuffer[i+1] = ((map[data[j+1]] << 4)&0xF0)|((map[data[j+2]] >> 2)&0x0F);
		outputBuffer[i+2] = ((map[data[j+2]] << 6)&0xC0)|(map[data[j+3]]);
		i += 3;
		j += 4;
	}
	//last 4 bytes check
	outputBuffer[i] = ((map[data[j]] << 2)&0xFC)|((map[data[j+1]] >> 4)&0x03);
	if(data[j+2] != '=')
	{
		i++;
		outputBuffer[i] = ((map[data[j+1]] << 4)&0xF0)|((map[data[j+2]] >> 2)&0x0F);
	}
	if(data[j+3] != '=')
	{
		i++;
		outputBuffer[i] = ((map[data[j+2]] << 6)&0xC0)|(map[data[j+3]]);
	}

	bytes_converted = i+1;
        
        return bytes_converted;
}
















