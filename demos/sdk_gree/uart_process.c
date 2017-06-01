
#include <qcom/qcom_common.h>
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/socket.h>
#include <qcom/qcom_gpio.h>
#include "qcom_nvram.h"

#include "task_manage.h"
#include "wifi_config.h"
#include "nvram_config.h"
#include "sdk_shell.h"
#include "socket_process.h"
#include "AES.h"
#include "base64.h"

#include "qcom_uart.h"
#include "cjson.h"
#include "list.h"

#include "uart_process.h"
#include "wifi_config.h"




//����UART    ���պͷ��͵�����
uart_rw_t uart_rw;


char netWorkState[16] = {0xaa, 0xaa, 0x00, 0x06, 0x01, 0x00, 0x00, 0x01, 0x00};




/*   U A R T _ I N I T _ A L L   */
/*-------------------------------------------------------------------------
    ��ʼ���������߳�(����Ŀ�ʼ)����Ȼ���޷���ӡ��Ϣ
-------------------------------------------------------------------------*/
void uartInitAll(void)
{
	qcom_uart_para tUartParam;
	char uart0_name[] = "UART0";

	//UART0һֱΪͨ�Ŷ˿ڡ���һ������ΪUART0���ڶ�ΪUART1
#if 0
	//10 11 Ϊͨ�ţ�6 7 Ϊ����
	qcom_uart_rx_pin_set(6,10);	
	qcom_uart_tx_pin_set(7,11);

#else
	//7 6 Ϊͨ�ţ�10 11 Ϊ����
	qcom_uart_rx_pin_set(6,10);	
	qcom_uart_tx_pin_set(7,11);

#endif

	qcom_uart_init();
	extern void user_pre_init(void);
	user_pre_init();
	qcom_uart_init();
	uart_printf("\n---Init serial port OK---\n");

	memset(&uart_rw, 0, sizeof(uart_rw_t));

	//UART0һֱΪͨ�Ŷ˿�
	uart_rw.fd  = qcom_uart_open(uart0_name);
	
	uart_printf("\n UART0 opened.");
	
	tUartParam.BaudRate	= 115200;
//	tUartParam.BaudRate	= 57600;	
	tUartParam.parity		= 0;
	tUartParam.number	  	= 8;
	tUartParam.StopBits		= 1;
	tUartParam.FlowControl	= 0;
	if (qcom_set_uart_config(uart0_name, &tUartParam) != A_OK) {
		uart_printf("\n UART0 config fail!");
 	}else{
		uart_printf("\n UART0 config OK.");
	}	
}




/*   B   C   C _ G E N E R A T E   */
/*-------------------------------------------------------------------------
    ����BCC���У����
-------------------------------------------------------------------------*/
uchar BCC_generate(uchar *p, uchar len)
{
	uchar i, bcc = *p;

	for(i=1; i<len; i++){

		p++;
		bcc ^= *p;		
	}

	return bcc;

}

/*   U A R T 0 _ T O _ T C P   */
/*-------------------------------------------------------------------------
    uart ���ݷ��͵�tcp
-------------------------------------------------------------------------*/
void uart0_to_tcp(char *data, int len)
{
	int hassend, txlen, ret;

	hassend = 0; 	//�Ѿ����Ͷ����ֽ�
	while(hassend < len){

		if(len -hassend <= 1024){
			txlen = len -hassend;
		}else{
			txlen = 1024;
		}

		ret = qcom_send(tcpClient.tcpSock, data + hassend, txlen, 0); 

		if(ret == txlen){
			hassend += txlen;
			uart_printf("tcpClient send (%d)%d.", len, ret);
		}else{
		
			if(ret == -100){					
				uart_printf("tcp tx buf not enough!");

				qcom_sys_reset();	/* ��������*/	

//				qcom_thread_msleep(10*1000);	
//				uart0_recv_clear();

			}else{
				tcpClient.tcpInit = 1;
				hassend = len;		//��������
				uart_printf("close tcpClient.");

			}
		}

	}

}


/*
	if(len%1024){		
		sendtime = len /1024 + 1;
	}else{
		sendtime = len /1024;
	}

	for(i=0; i < sendtime; i++){
		
		if(i == sendtime -1){
			txlen = len % 1024;
		}else{
			txlen = 1024;
		}

		hassend = qcom_send(tcpClient.tcpSock, data, txlen, 0);	

		if(hassend == txlen){
			data += txlen;
			uart_printf("tcpClient send (%d)%d.", len, hassend);
		}else{
			if(hassend == -100){					
				uart_printf("tcp tx buf not enough!");

				qcom_sys_reset();	

//				qcom_thread_msleep(10*1000);	
//				uart0_recv_clear();

			}else{
				tcpClient.tcpInit = 1;
				uart_printf("close tcpClient.");

			}
		}

	}

*/



/*		rxlen = 0;
		remain = qcom_uart_read(uart_rw.fd, uart_rw.rxbuf, &rxlen);
		
		if(remain > 0){
			if(remain == uart_rw.rxlen){
				uart_rw.rxlen = UART_RX_BUF_LEN;
				remain = qcom_uart_read(uart_rw.fd, uart_rw.rxbuf, &uart_rw.rxlen);
				
				uart_printf("r(%d)%d.", uart_rw.rxlen, remain);
				
				if(tcpAccept.socket > 0 && tcpAccept.status == 0xff){				
					uart0_to_tcp(uart_rw.rxbuf, uart_rw.rxlen);
					
				}else{
					uart_printf("tcp no connet, don't send!");
				}

				uart_rw.rxlen = 0;

			}else{
				uart_rw.rxlen = remain;
			}
		}
		else{
			uart_rw.rxlen = 0;
		}
*/


/*   U A R T   R E C V   T A S K   */
/*-------------------------------------------------------------------------
    ��Ҫ����uart�Ľ���
    һ���յ���������tcp/udp����
-------------------------------------------------------------------------*/
void uartProcessTask(unsigned int input_thread)
{
	qcom_thread_msleep(50);	

	while(1){

		uart0_send();

		uart0_recv();	//30ms

		qcom_thread_msleep(20);	

	}

}


void uart0_recv_clear(void)
{
	int remain = 0;
	uint rxlen = 0;

	do{
		rxlen = UART_RX_TRY_LEN;
		remain = qcom_uart_read(uart_rw.fd, uart_rw.rxbuf, &rxlen);

	}while(remain);
	
	uart_rw.rxlen = 0;

}


void uart0_timer_handler(uint a, void *p)
{






}



/*   U A R T 0 _ T I M E R _ I N I T   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
void uart0_timer_start(int ms)
{
	qcom_timer_init(&uart_rw.timer, uart0_timer_handler, NULL, ms, ONESHOT);
	qcom_timer_start(&uart_rw.timer); 
}



/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
void uart0_recv_handle(int *handleflag)
{
	if(*handleflag == 0){			//��һ�δ���

		if(uart_rw.rxbuf[0] == 0xbb && uart_rw.rxbuf[1] == 0xbb){		//����͸��
			if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){ 				
				uart0_to_tcp(uart_rw.rxbuf+2, uart_rw.rxlen-2); 					
			}else{
				uart_printf("tcp no connet, don't send!");
			}						
		}
		else if(uart_rw.rxbuf[0] == 0xaa && uart_rw.rxbuf[1] == 0xaa){	//wifi����
		
			switch(uart_rw.rxbuf[4]){
				case 2:				// ��������
					qcom_sys_reset();	
					break;
					
				case 3:				//ɾ�������·����Ϣ��Ȼ������(�������ģʽ)
					memset(tDeviceInfo.tWlanInfo.ssid1, 0, SSID_LEN);
					memset(tDeviceInfo.tWlanInfo.ssid2, 0, SSID_LEN);
					memset(tDeviceInfo.tWlanInfo.key1, 0, KEY_LEN);
					memset(tDeviceInfo.tWlanInfo.key2, 0, KEY_LEN);
					ResetSystem();				
					break;

				case 4:				//���ip��Ϣ���Ͽ�tcp����
					tDeviceInfo.ip = 0;
					device_information_set(&tDeviceInfo);
					tcpClient.tcpInit = 0;					
					
					break;
					
				default:				
					break;

			}
				
		}
		else{
		
		}

		*handleflag = 1;
		
	}
	else{				//�ǵ�һ�δ���
	
		if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){ 				
			uart0_to_tcp(uart_rw.rxbuf, uart_rw.rxlen); 					
		}else{
			uart_printf("tcp no connet, don't send!");
		}
	}

}




/*   U A R T 0 _ R E C V   */
/*-------------------------------------------------------------------------
    ���պ���
    ������ֱ�ӷ���
    ��ʼ���պ�30s��û�н��յ�������������ν���
-------------------------------------------------------------------------*/
void uart0_recv(void)
{
	int remain = 0, count = 0, handleflag = 0;
	uint rxlen = 0;

	uart_rw.rxlen = 0;
	
	while(1){

		rxlen = UART_RX_TRY_LEN;
		remain = qcom_uart_read(uart_rw.fd, uart_rw.rxbuf + uart_rw.rxlen, &rxlen);
				
		if(rxlen){
			uart_rw.rxlen += rxlen;
			
			if(uart_rw.rxlen >= UART_RX_SAV_LEN){
				
				uart_printf("re(%d).", uart_rw.rxlen);
				
				uart0_recv_handle(&handleflag);
				
				uart_rw.rxlen = 0;
			}
			
		}else{	
		
			if(uart_rw.rxlen > 0){	/* ������� */
				uart_printf("ro(%d).", uart_rw.rxlen);
				
				uart0_recv_handle(&handleflag);
				return;
				
			}else{
				count++;
				if(count >= 10){		/* 3*10=30s ��û�����ݽ��� */
					return;
				}

			}				
		}

		qcom_thread_msleep(3);

	}

}


/*   U A R T 0 _ S E N D   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
void uart0_send(void)
{
	int ret;
	uint writeByte = 0;

	if(uartTxSLHead){	/* ��tcp������Ҫ���� */

		while(uartTxSLHead->hasWrite < uartTxSLHead->len){
			
			writeByte = uartTxSLHead->len - uartTxSLHead->hasWrite;

			ret = qcom_uart_write(uart_rw.fd, uartTxSLHead->data + uartTxSLHead->hasWrite, &writeByte);
			
			uartTxSLHead->hasWrite += writeByte;

			if(uartTxSLHead->hasWrite < uartTxSLHead->len){
				qcom_thread_msleep(10);
			}

		}
		
		uart_printf("\n uart(%d) send(%d).", uartTxSLHead->len, uartTxSLHead->hasWrite);
		
		tx_event_flags_get(&uartTxSLEventGroup, uartTxSLEventFlag, TX_AND_CLEAR, &uartTxSLActualFlag, 100*1000);
		deleteNodeFromUartTxSLHead();
		tx_event_flags_set(&uartTxSLEventGroup, uartTxSLEventFlag, TX_OR);	
		
		qcom_thread_msleep((ret*100)/1152 + 1);			
		
		
	}
	else{	/* û��tcp���ݷ��ͣ���������״̬���� */

		netWorkState[5] = tNetworkInfo.linkQuality;
		netWorkState[6] = LEDShow.ledState;
		
		writeByte = 10;		/* ����״̬���ݳ��� */
		
		ret = qcom_uart_write(uart_rw.fd, netWorkState, &writeByte);	

		qcom_thread_msleep(2);

	}

}




void uart0_send_finish_confirm(void)
{


}






























