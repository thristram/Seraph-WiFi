
#ifndef  __BASE64_H__
#define  __BASE64_H__

#include <qcom/stdint.h>










//API
void base64_encode(const char* data, const int size, uint8_t* outputBuffer);



int base64_decode(const uint8_t* data, const int size, uint8_t* outputBuffer);







#endif




