#ifndef  __UART_PROCESS_H__
#define  __UART_PROCESS_H__

#include "qcom_uart.h"


struct	bit_def {
	unsigned char	b0:1;
	unsigned char	b1:1;
	unsigned char	b2:1;
	unsigned char	b3:1;
	unsigned char	b4:1;
	unsigned char	b5:1;
	unsigned char	b6:1;
	unsigned char	b7:1;
};
union	byte_def{
	struct	bit_def	bit;
	unsigned char	byte;
};



#define UART_RX_BUF_LEN 	3* 1024
#define UART_RX_TRY_LEN 	1024
#define UART_RX_SAV_LEN 	(UART_RX_BUF_LEN -UART_RX_TRY_LEN)


typedef struct  {

	A_INT32  	fd;
	
	A_INT32 	txEn;	/* 发送使能，当为1是表示有数据包正在发送 */

	A_UINT32 	rxlen;
	
	A_CHAR   	rxbuf[UART_RX_BUF_LEN];

	qcom_timer_t timer;
	
}uart_rw_t;


typedef struct {
     	A_UINT16 	preamble; // head bytes of msg head, 0x7e7e
      	A_UINT16 	len;
	A_UCHAR 	type;
     	A_UCHAR  	data[0];    // frame valide data pointer, point to data area
}uart_frame_t;



extern uart_rw_t uart_rw;


void uartInitAll(void);
void uart0_to_tcp(char *data, int len);

void uart0_recv_clear(void);

void uart0_recv(void);

void uart0_send(void);

void uartProcessTask(unsigned int input_thread);


#endif

