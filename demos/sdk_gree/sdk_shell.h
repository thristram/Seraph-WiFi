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

	/* led ��״̬��ʾwifiģ��Ĺ���״̬ 
	0: ����
	1: ÿ��1s��1�Σ�wifiģ����SH�������ӣ�������������ģʽ
	2: ÿ��1s��2�Σ�wifiģ��ɹ�����·�ɣ�����ʱwifiģ����SH��δ��������()
	3: ÿ��1s��3�Σ�wifiģ������·�ɲ��ɹ����������ģʽ������60s
		
	ff: ������wifiģ����������·��

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


//��ʾ����ʱ��
void show_time(void);

void printf_char_HEX(char *buf, int len);

void printf_ip_format(A_UINT32 IP, A_UINT32 Mask, A_UINT32 GW, A_UINT16 Port);

void LED_Change(void);
void LED_Init(void);


/* ����ADK�е�API����ȡʣ���ڴ� */
extern unsigned int mem_heap_get_free_size(void);




#endif

