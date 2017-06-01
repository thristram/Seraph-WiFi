#ifndef  __WIFI_CONFIG_H__
#define  __WIFI_CONFIG_H__

#include <qcom/qcom_common.h>
#include <qcom/qcom_internal.h>
#include <qcom/qcom_wlan.h>
#include "AES.h"


#define PRINTF_EN		0

#if PRINTF_EN


#define tcp_printf(...)   		cmnos_printf( __VA_ARGS__ )
#define udp_printf(...)   		cmnos_printf( __VA_ARGS__ )

#define uart_printf(...)   		cmnos_printf( __VA_ARGS__ )
#define wifi_printf(...)   		cmnos_printf( __VA_ARGS__ )


#else

#define tcp_printf(...)   		
#define udp_printf(...)   		

#define uart_printf(...)   		
#define wifi_printf(...)   			

#endif




//�û���¼����
typedef struct {
	A_UCHAR	uNameLen;				
	A_UCHAR 	uName[15];				
	A_UCHAR	uPwdLen;				
	A_UCHAR 	uPassword[15];		
} Login_Info_t;


#define SSID_LEN 	32   
#define KEY_LEN 	64  


//�������ò���
typedef struct {
	A_UCHAR 	uSwitchFlag;		//WiFi���� bit0:1�� 0 ��     bit4:�ָ���������
	QCOM_WLAN_DEV_MODE 	uMode;			//WiFiģʽ:0/sta       1/ap

	
	A_CHAR  	ssid1[SSID_LEN];	/* ��������·��������Ϣ */
	A_CHAR  	ssid2[SSID_LEN];

	A_CHAR  	key1[KEY_LEN];
	A_CHAR  	key2[KEY_LEN];
	
} Wlan_Info_t;


//�豸���в���
typedef struct {

	A_UCHAR	uVerify[6];			//����ƥ��
	
	A_UCHAR 	uMAC[6];					//�յ�MAC��ַ

	A_UINT32	ip;			//�������ӳɹ���ip����������ֱ��ʹ��
		
	Wlan_Info_t 	tWlanInfo;				//wifi ��ز���

} Device_Info_t;


//�������в���
typedef struct {
	A_UINT8 	devMode;		
	A_UINT8  	ipObtained;	/* �Ƿ��ȡip */
	A_UINT8  	getIPtimes;	/* ��ȡip���� */
	A_UINT8  	linkQuality;	/* wifi���ӵ��ź�ǿ�ȣ�Խ��Խǿ */
	
	A_UINT32 	ipAddress;
	A_UINT32 	submask;
	A_UINT32 	gateway;
	A_UINT32 	dns;
	QCOM_BSS_SCAN_INFO tScanInfo;
} Network_Info_t;


//ip  ping
typedef struct {

	ULONG	timePrev;
	ULONG	timeNow;
	ULONG	time;

} ip_ping_t;







extern Device_Info_t tDeviceInfo;

extern Network_Info_t tNetworkInfo;



extern ip_ping_t IPPing;

void wifi_connect(A_CHAR *ssid, A_CHAR *passwd);
void wifi_process(void);
void kuaifi_confirm(void);

A_STATUS ip_get(void);
void ip_obtained_confirm(void);

A_STATUS STA_mode_set(A_CHAR *ssid, A_CHAR *passwd);
void STA_mode_callback(void);
A_STATUS network_info_get(void);
void ip_ping_process(void);





#endif


