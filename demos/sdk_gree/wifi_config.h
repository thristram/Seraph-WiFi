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




//用户登录参数
typedef struct {
	A_UCHAR	uNameLen;				
	A_UCHAR 	uName[15];				
	A_UCHAR	uPwdLen;				
	A_UCHAR 	uPassword[15];		
} Login_Info_t;


#define SSID_LEN 	32   
#define KEY_LEN 	64  


//网络设置参数
typedef struct {
	A_UCHAR 	uSwitchFlag;		//WiFi开关 bit0:1开 0 关     bit4:恢复出厂设置
	QCOM_WLAN_DEV_MODE 	uMode;			//WiFi模式:0/sta       1/ap

	
	A_CHAR  	ssid1[SSID_LEN];	/* 保存两组路由器的信息 */
	A_CHAR  	ssid2[SSID_LEN];

	A_CHAR  	key1[KEY_LEN];
	A_CHAR  	key2[KEY_LEN];
	
} Wlan_Info_t;


//设备所有参数
typedef struct {

	A_UCHAR	uVerify[6];			//参数匹配
	
	A_UCHAR 	uMAC[6];					//空调MAC地址

	A_UINT32	ip;			//保存连接成功的ip，供重启后直接使用
		
	Wlan_Info_t 	tWlanInfo;				//wifi 相关参数

} Device_Info_t;


//网络运行参数
typedef struct {
	A_UINT8 	devMode;		
	A_UINT8  	ipObtained;	/* 是否获取ip */
	A_UINT8  	getIPtimes;	/* 获取ip次数 */
	A_UINT8  	linkQuality;	/* wifi连接的信号强度，越大越强 */
	
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


