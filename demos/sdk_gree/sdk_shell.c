
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



char *country_code = "US";	/* ���ù��Ҵ��룬��ʹ��apģʽʱʹ�ã�Ŀǰֻ��staģʽ */

LED_Info_t LEDShow;

/* ����ץ�������Ϣ */
smart_config_data kuaifiSave;

kuaifi_key_t kuaifiKey;



TX_THREAD 	 	host_thread;	//�߳�
TX_THREAD 	 	uart_thread;
TX_THREAD	 	near_thread;	/* socket �߳� */

TX_BYTE_POOL pool;
#define BYTE_POOL_SIZE 			(2 * 1024)
#define PSEUDO_HOST_STACK_SIZE	(1 * 1024)




/*-------------------------------------------------------------------------
    ����kuaifi��Կ
-------------------------------------------------------------------------*/
void generate_kuaifiKey(void)
{









}





/*   S H E L L _ H O S T _ E N T R Y   */
/*-------------------------------------------------------------------------
    ���̣߳��û��������
-------------------------------------------------------------------------*/
void shell_host_entry(ULONG which_thread)
{
    	ulong	uCurtTime = 0;
	ulong	uPrevTime = 0;
	ulong	uTime = 0;

	/* ��ʼ������ */
	uartInitAll();	
	LED_Init();
	
//	qcom_watchdog(1, 5);	
	qcom_gpio_set_pin_high(led_gpio);	/* �ϵ�֮��Ĭ������·���� */
	LEDShow.ledState = 0xff;		/* �ϵ�Ĭ�����ȿ�ʼ����·�� */

	memset((uchar *)&tDeviceInfo, 0, sizeof(Device_Info_t));		//����
	device_information_load(&tDeviceInfo);		//��ȡ�豸����
	
	generate_kuaifiKey();  		//����ץ��ʱ������Կ

	memset((uchar *)&tNetworkInfo, 0, sizeof(Network_Info_t));	/* ����״̬���� */

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

	
	wifi_process();		/* ����·�� */
	
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

			//����WDT
			//qcom_watchdog_feed();

			/* ÿ���ӻ�ȡһ�����������ź�ǿ�� */
			qcom_sta_get_rssi(&tNetworkInfo.linkQuality);


			if(tcpClient.tcpInit == 3){
				tcpClient.reconnectCounts++;
				if(tcpClient.reconnectCounts > tcpClient.reconnectWaitSecond){
					tcpClient.tcpInit = 5;
				}
			}


//			A_PRINTF("\n mem free %d", mem_heap_get_free_size());	
			//��Ҫ���ڴ���ɷŴ˴�
			
		}		



			
		LED_Change();

		qcom_thread_msleep(10);   //�л�����


	}

}












/*   U S E R _ M A I N   */
/*-------------------------------------------------------------------------
    ���������޸�
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
    ��ʾʱ��
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
    ��ӡip����Ϣ
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
    ʹ��gpio9  ��2�����ţ�������ʱ��gpio2
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
		
		case 1:	/* ÿ��1s��1�� */			
			if(uTime < 200){
				qcom_gpio_set_pin_high(led_gpio);							
			}else if(uTime < 1700){
				qcom_gpio_set_pin_low(led_gpio);
			}else{
				LEDShow.ledTimeStart = LEDShow.ledTimeNow;
			}
			break;
			
		case 2:	/* ÿ��1s��2�� */
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
		case 3:	/* ÿ��1s��3�� */
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







