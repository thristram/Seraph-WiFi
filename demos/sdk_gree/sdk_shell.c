
#include <qcom/qcom_common.h>
#include "qcom_uart.h"
#include <qcom/qcom_gpio.h>
#include <qcom/qcom_wdt.h>
#include <qcom/qcom_nvram.h>

#include "arwlib/arwlib.h"

#include "task_manage.h"
#include "wifi_config.h"
#include "nvram_config.h"
#include "sdk_shell.h"
#include "socket_process.h"
#include "uart_process.h"
#include "list.h"



char *country_code = "US";	/* 设置国家代码，在使用ap模式时使用，目前只有sta模式 */

char *id_public_key = ">ff?gQ=d=~585E{";

char *msg_public_key = "77]\"9u0tdbsWAXf";



LED_Info_t LEDShow;

/* 保存抓包后的信息 */
smart_config_data kuaifiSave;

kuaifi_key_t kuaifiKey;



TX_THREAD 	 	host_thread;	//线程
TX_THREAD 	 	uart_thread;
TX_THREAD	 	near_thread;	/* socket 线程 */

TX_BYTE_POOL pool;
#define BYTE_POOL_SIZE 			(2 * 1024)
#define PSEUDO_HOST_STACK_SIZE	(1 * 1024)







/*   H E X 2 _ T O   C H A R   */
/*-------------------------------------------------------------------------
    将16进制数转化为两个字符
-------------------------------------------------------------------------*/
void hex2_toCapitalChar(uchar d, char *p)
{
	uchar i;
	
	i = d /16;
	if(i <= 9){*p = '0' + i;}
	else{*p = 'A' + i - 10;}
	
	i = d %16;
	if(i <= 9){*(p+1) = '0' + i;}
	else{*(p+1) = 'A' + i - 10;}

}


/*-------------------------------------------------------------------------
    生成kuaifi秘钥
-------------------------------------------------------------------------*/
void generate_kuaifiKey(void)
{
	int i;
	int mac_address_len;
	char mac_address[20] = {0};
	char temp[64] = {0};
	char md5ret[64] = {0};

	for(i=0; i < 6; i++){
		hex2_toCapitalChar(tDeviceInfo.uMAC[i], &mac_address[3*i]);
		mac_address[3*i + 2] = ':';
	}	
	mac_address_len = 12+5;
	wifi_printf("\n mac_address:%s", mac_address);	

	//id key 
	memcpy(temp, mac_address, mac_address_len);
	memcpy(temp+mac_address_len, id_public_key, AES_KEY_LEN);	

	qcom_sec_md5_init();
	qcom_sec_md5_update((uchar*)temp, mac_address_len+AES_KEY_LEN -1);
	qcom_sec_md5_final(md5ret);
		
	memset(temp, 0, 64);
	memcpy(temp, md5ret + 8, 15);
	memcpy(temp+15, mac_address, mac_address_len);

	memset(md5ret, 0, 64);	
	qcom_sec_md5_init();
	qcom_sec_md5_update((uchar*)temp, 15+mac_address_len);
	qcom_sec_md5_final(md5ret);
	wifi_printf("\n id key:%s", md5ret); 

	for(i=0; i < AES_KEY_LEN; i++){
		kuaifiKey.idKey[i] = string_tohex2(md5ret + i*2);
	}
	printf_char_HEX((char*)kuaifiKey.idKey, AES_KEY_LEN);

	//msg key 
	memset(temp, 0, 64);
	memcpy(temp, mac_address, mac_address_len);
	memcpy(temp+mac_address_len, msg_public_key, AES_KEY_LEN);	

	memset(md5ret, 0, 64);
	qcom_sec_md5_init();
	qcom_sec_md5_update((uchar*)temp, mac_address_len+AES_KEY_LEN -1);
	qcom_sec_md5_final(md5ret);

	memset(temp, 0, 64);
	memcpy(temp, md5ret + 3, 17);
	memcpy(temp+17, mac_address, mac_address_len);

	memset(md5ret, 0, 64);	
	qcom_sec_md5_init();
	qcom_sec_md5_update((uchar*)temp, 17+mac_address_len);
	qcom_sec_md5_final(md5ret);
	wifi_printf("\n msg key:%s", md5ret); 

	for(i=0; i < AES_KEY_LEN; i++){
		kuaifiKey.msgKey[i] = string_tohex2(md5ret + i*2);
	}
	printf_char_HEX((char*)kuaifiKey.msgKey, AES_KEY_LEN);


}





/*   S H E L L _ H O S T _ E N T R Y   */
/*-------------------------------------------------------------------------
    主线程，用户函数入口
-------------------------------------------------------------------------*/
void shell_host_entry(ULONG which_thread)
{
    	ulong	uCurtTime = 0;
	ulong	uPrevTime = 0;
	ulong	uTime = 0;

	/* 初始化串口 */
	uartInitAll();	
	LED_Init();
	
//	qcom_watchdog(1, 5);	
	qcom_gpio_set_pin_high(led_gpio);	/* 上电之后默认连接路由器 */
	LEDShow.ledState = 0xff;		/* 上电默认首先开始连接路由 */

	memset((uchar *)&tDeviceInfo, 0, sizeof(Device_Info_t));		//清零
	device_information_load(&tDeviceInfo);		//读取设备数据
	
	generate_kuaifiKey();  		//生成抓包时解析秘钥

	memset((uchar *)&tNetworkInfo, 0, sizeof(Network_Info_t));	/* 网络状态清零 */

	memset((uchar *)&kuaifiSave, 0, sizeof(smart_config_data));


	tx_event_flags_create(&uartTxSLEventGroup, "uart tx event group");	
	tx_event_flags_set(&uartTxSLEventGroup, uartTxSLEventFlag, TX_OR);
//	tx_event_flags_get(&uartTxSLEventGroup, uartTxSLEventFlag, TX_AND_CLEAR, &uartTxSLActualFlag, 100*1000);
	
	tx_event_flags_create(&uartRxSLEventGroup, "uart rx event group");
	tx_event_flags_set(&uartRxSLEventGroup, uartRxSLEventFlag, TX_OR);
//	tx_event_flags_get(&uartRxSLEventGroup, uartRxSLEventFlag, TX_AND_CLEAR, &uartRxSLActualFlag, 100*1000);

	show_time();
	A_PRINTF("Create Uart_Recv_Task.");	
	qcom_task_start(&uart_thread, uartProcessTask , 1, 1024*3, 10);
	
	show_time();
	A_PRINTF("Create Near_Socket_Task.");	
	qcom_task_start(&near_thread, nearSocketTask , 2, 1024*3, 10);

	
	wifi_process();		/* 连接路由 */
	
	while(1)
	{	
		uCurtTime = tx_time_get();
		if(uCurtTime >= uPrevTime){
			uTime = uCurtTime - uPrevTime;			
		}else{
			uTime = 0xffffffff - uPrevTime + uCurtTime;
		}
		if(uTime > 1000){			
			uPrevTime = uCurtTime;

			//清零WDT
			//qcom_watchdog_feed();

			/* 每秒钟获取一次网络连接信号强度 */
			qcom_sta_get_rssi(&tNetworkInfo.linkQuality);


			if(tcpClient.tcpInit == 3){
				tcpClient.reconnectCounts++;
				if(tcpClient.reconnectCounts > tcpClient.reconnectWaitSecond){
					tcpClient.tcpInit = 5;
				}
			}


//			A_PRINTF("\n mem free %d", mem_heap_get_free_size());	
			//需要周期处理可放此处
			
		}		



			
		LED_Change();

		qcom_thread_msleep(10);   //切换任务，


	}

}












/*   U S E R _ M A I N   */
/*-------------------------------------------------------------------------
    函数不可修改
-------------------------------------------------------------------------*/
void user_main(void)
{
    	int i;
	tx_byte_pool_create(&pool, "cdrtest pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);

	CHAR *pointer;
	tx_byte_allocate(&pool, (VOID **) & pointer, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);
	tx_thread_create(&host_thread, "cdrtest thread", shell_host_entry, i, pointer, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);
}



/*   S W A T _ T I M E   */
/*-------------------------------------------------------------------------
    显示时间
-------------------------------------------------------------------------*/
void show_time(void)
{
	ulong ms;
	ms = time_ms();
	A_PRINTF("\n[%dh %dm %ds.%d ] ", (ms /1000) /3600,  ((ms /1000) % 3600) /60, (ms /1000) % 60,ms % 1000);
}



/*   P R I N T F _ C H A R _   H   E   X   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
void printf_char_HEX(char *buf, int len)
{	
	int i;
	A_PRINTF("\n");
	for(i=0; i<len; i++){
		A_PRINTF("%02x ", *(buf+i));
	}

}



/*   P R I N T F _   I   P _ F O R M A T   */
/*-------------------------------------------------------------------------
    打印ip等信息
-------------------------------------------------------------------------*/
void printf_ip_format(A_UINT32 IP,A_UINT32 Mask,A_UINT32 GW,A_UINT16 Port)
{
	if(IP){	A_PRINTF("IP:%d.%d.%d.%d  ", (IP)>>24&0xFF, (IP)>>16&0xFF, (IP)>>8&0xFF, (IP)&0xFF);}
	if(Mask){	A_PRINTF("SubMask:%d.%d.%d.%d  ", (Mask)>>24&0xFF, (Mask)>>16&0xFF, (Mask)>>8&0xFF, (Mask)&0xFF);}
	if(GW){	A_PRINTF("GateWay:%d.%d.%d.%d  ", (GW)>>24&0xFF, (GW)>>16&0xFF, (GW)>>8&0xFF, (GW)&0xFF);}
	if(Port){	A_PRINTF("Port:%d. ", Port);}
	
}


/*   L   E   D _   I N I T   */
/*-------------------------------------------------------------------------
    使用gpio9  第2个引脚，调试暂时用gpio2
-------------------------------------------------------------------------*/
void LED_Init(void)
{
	qcom_gpio_config_pin(led_gpio, 0, 0, QCOM_GPIO_PIN_PULLNONE);
	qcom_gpio_set_pin_dir(led_gpio, 0);
	memset(&LEDShow, 0, sizeof(LED_Info_t));

}



void LED_Change(void)
{
	ULONG	uTime = 0;

	LEDShow.ledTimeNow = tx_time_get();
	if(LEDShow.ledTimeNow > LEDShow.ledTimeStart){
		uTime = LEDShow.ledTimeNow - LEDShow.ledTimeStart;
	}else{
		uTime = LEDShow.ledTimeNow;
	}
	
	switch (LEDShow.ledState){
		
		case 1:	/* 每隔1s闪1次 */			
			if(uTime < 200){
				qcom_gpio_set_pin_high(led_gpio);							
			}else if(uTime < 1700){
				qcom_gpio_set_pin_low(led_gpio);
			}else{
				LEDShow.ledTimeStart = LEDShow.ledTimeNow;
			}
			break;
			
		case 2:	/* 每隔1s闪2次 */
			if(uTime < 150){
				qcom_gpio_set_pin_high(led_gpio);							
			}else if(uTime < 300){
				qcom_gpio_set_pin_low(led_gpio);
			}else if(uTime < 450){
				qcom_gpio_set_pin_high(led_gpio);
			}else if(uTime < 1950){
				qcom_gpio_set_pin_low(led_gpio);
			}else{
				LEDShow.ledTimeStart = LEDShow.ledTimeNow;
			}	
			break;		
		case 3:	/* 每隔1s闪3次 */
			if(uTime < 150){
				qcom_gpio_set_pin_high(led_gpio);							
			}else if(uTime < 300){
				qcom_gpio_set_pin_low(led_gpio);
			}else if(uTime < 450){
				qcom_gpio_set_pin_high(led_gpio);
			}else if(uTime < 600){
				qcom_gpio_set_pin_low(led_gpio);
			}else if(uTime < 750){
				qcom_gpio_set_pin_high(led_gpio);
			}else if(uTime < 2250){
				qcom_gpio_set_pin_low(led_gpio);
			}else{
				LEDShow.ledTimeStart = LEDShow.ledTimeNow;
			}		
			break;
	
		default :
			break;
		
	}

}







