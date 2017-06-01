#ifndef  __SDK_SHELL_H__
#define  __SDK_SHELL_H__


#include "arwlib/arwlib.h"
#include "wifi_config.h"

#include "AES.h"


typedef unsigned char  	uchar;
typedef unsigned short  	ushort;
typedef unsigned int  	uint;


//#define led_gpio 	2

//seraph
#define led_gpio 	9


typedef struct {
	ULONG	ledTimeNow;
	ULONG	ledTimeStart;

	/* led 的状态显示wifi模块的工作状态 
	0: 不亮
	1: 每隔1s闪1次，wifi模块与SH建立连接，进入正常工作模式
	2: 每隔1s闪2次，wifi模块成功连接路由，但此时wifi模块与SH还未建立连接()
	3: 每隔1s闪3次，wifi模块连接路由不成功，进入快联模式，运行60s
		
	ff: 常亮，wifi模块正在连接路由

	*/
	A_UINT8 ledState;
	A_UINT8 ledOnTimes;

}LED_Info_t;

typedef struct {

	A_UINT8 idKey[AES_KEY_LEN];
	
	A_UINT8 msgKey[AES_KEY_LEN];

} kuaifi_key_t;



extern char *country_code;

extern LED_Info_t LEDShow;

extern smart_config_data kuaifiSave;

extern kuaifi_key_t kuaifiKey;


//显示运行时间
void show_time(void);

void printf_char_HEX(char *buf, int len);

void printf_ip_format(A_UINT32 IP, A_UINT32 Mask, A_UINT32 GW, A_UINT16 Port);

void LED_Change(void);
void LED_Init(void);


/* 来自ADK中的API，获取剩余内存 */
extern unsigned int mem_heap_get_free_size(void);




#endif

