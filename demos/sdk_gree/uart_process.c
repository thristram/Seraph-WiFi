
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




//用于UART    接收和发送的数据
uart_rw_t uart_rw;


char netWorkState[16] = {0xaa, 0xaa, 0x00, 0x06, 0x01, 0x00, 0x00, 0x01, 0x00};




/*   U A R T _ I N I T _ A L L   */
/*-------------------------------------------------------------------------
    初始化放在主线程(程序的开始)，不然，无法打印信息
-------------------------------------------------------------------------*/
void uartInitAll(void)
{
	qcom_uart_para tUartParam;
	char uart0_name[] = "UART0";

	//UART0一直为通信端口。第一个参数为UART0，第二为UART1
#if 0
	//10 11 为通信，6 7 为调试
	qcom_uart_rx_pin_set(6,10);	
	qcom_uart_tx_pin_set(7,11);

#else
	//7 6 为通信，10 11 为调试
	qcom_uart_rx_pin_set(6,10);	
	qcom_uart_tx_pin_set(7,11);

#endif

	qcom_uart_init();
	extern void user_pre_init(void);
	user_pre_init();
	qcom_uart_init();
	uart_printf("\n---Init serial port OK---\n");

	memset(&uart_rw, 0, sizeof(uart_rw_t));

	//UART0一直为通信端口
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
    生成BCC异或校验码
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
    uart 数据发送到tcp
-------------------------------------------------------------------------*/
void uart0_to_tcp(char *data, int len)
{
	int hassend, txlen, ret;

	hassend = 0; 	//已经发送多少字节
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

				qcom_sys_reset();	/* 重新启动*/	

//				qcom_thread_msleep(10*1000);	
//				uart0_recv_clear();

			}else{
				tcpClient.tcpInit = 1;
				hassend = len;		//结束发送
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
    主要处理uart的接收
    一接收到数据则向tcp/udp发送
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
	if(*handleflag == 0){			//第一次处理

		if(uart_rw.rxbuf[0] == 0xbb && uart_rw.rxbuf[1] == 0xbb){		//数据透传
			if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){ 				
				uart0_to_tcp(uart_rw.rxbuf+2, uart_rw.rxlen-2); 					
			}else{
				uart_printf("tcp no connet, don't send!");
			}						
		}
		else if(uart_rw.rxbuf[0] == 0xaa && uart_rw.rxbuf[1] == 0xaa){	//wifi设置
		
			switch(uart_rw.rxbuf[4]){
				case 2:				// 重新启动
					qcom_sys_reset();	
					break;
					
				case 3:				//删除保存的路由信息，然后重启(进入快联模式)
					memset(tDeviceInfo.tWlanInfo.ssid1, 0, SSID_LEN);
					memset(tDeviceInfo.tWlanInfo.ssid2, 0, SSID_LEN);
					memset(tDeviceInfo.tWlanInfo.key1, 0, KEY_LEN);
					memset(tDeviceInfo.tWlanInfo.key2, 0, KEY_LEN);
					ResetSystem();				
					break;

				case 4:				//清除ip信息，断开tcp连接
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
	else{				//非第一次处理
	
		if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){ 				
			uart0_to_tcp(uart_rw.rxbuf, uart_rw.rxlen); 					
		}else{
			uart_printf("tcp no connet, don't send!");
		}
	}

}




/*   U A R T 0 _ R E C V   */
/*-------------------------------------------------------------------------
    接收函数
    接收完直接发送
    开始接收后30s内没有接收到数据则结束本次接收
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
		
			if(uart_rw.rxlen > 0){	/* 接收完成 */
				uart_printf("ro(%d).", uart_rw.rxlen);
				
				uart0_recv_handle(&handleflag);
				return;
				
			}else{
				count++;
				if(count >= 10){		/* 3*10=30s 内没有数据接收 */
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

	if(uartTxSLHead){	/* 有tcp数据需要发送 */

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
	else{	/* 没有tcp数据发送，则发送网络状态数据 */

		netWorkState[5] = tNetworkInfo.linkQuality;
		netWorkState[6] = LEDShow.ledState;
		
		writeByte = 10;		/* 网络状态数据长度 */
		
		ret = qcom_uart_write(uart_rw.fd, netWorkState, &writeByte);	

		qcom_thread_msleep(2);

	}

}




void uart0_send_finish_confirm(void)
{


}






























