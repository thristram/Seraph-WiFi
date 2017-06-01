#include <qcom/qcom_common.h>
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/qcom_i2s.h>
#include <qcom/qcom_i2c.h>

#include "threadx/tx_api.h"
#include "threadx/tx_thread.h"

extern A_INT32 uda1380_read(A_UINT8 addr, A_UINT16 * val);
extern A_INT32 uda1380_write(A_UINT8 addr, A_UINT16 val);

extern void qcom_thread_msleep(unsigned long ms);
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();


#define I2S_BUF_SIZE 1024
#define I2S_ERR 0
#define I2S_OK 1

#define UDA1380_CNT (9)
#define TCP_RX_BUF_LEN  I2S_BUF_SIZE
#define TCP_TX_BUF_LEN  (I2S_BUF_SIZE + 16)

#define IPv4_STR_PRT(addr) printf("%d.%d.%d.%d", (addr)>>24&0xff, (addr)>>16&0xff, (addr)>>8&0xff, (addr)&0xff)

static int gl4_port_for_i2s = 0;
static unsigned int gip_for_i2s = 0;
static int socket_conn = -1;

int i2s_ctl_flag = 0;
#define I2S_INIT 			0x8
#define I2S_SEND_FINI 	0x4
#define I2S_SEND_STOP 	0x2
#define I2S_REC_STOP 	0x1

static int cnt_i2s_blk = 0;
typedef enum {
    UDA1380_SPEAKER = 1,
    UDA1380_LINEIN	 =2
} CODEC_MODE;
//extern void k_msleep(int ms);
void tcp_recv_from_i2s(unsigned char * bufp, unsigned int size);

#if 0
#define I2S_PRINTF printf
#else
#define I2S_PRINTF(fmt, ...) 
#endif

int uda1380_config(CODEC_MODE mode)
{
	static unsigned char uda1380_reg[UDA1380_CNT] =
	{0x7F,0x01,0x13,0x14,0x00,0x02,0x22,0x23,0x21};

	static unsigned short int uda1380_cfg1[UDA1380_CNT] =
	{0,0,0,0,0x0f02,0xa5df,0x040e,0x1,0x0303 };
	
	static unsigned short int uda1380_cfg2[UDA1380_CNT] =
	{0,0,0,0,0x0f02,0xa5df,0x0401,0x1,0x0303};
	
	if(!(i2s_ctl_flag&I2S_INIT)){
	if(mode == UDA1380_SPEAKER)
	{
		if(I2S_ERR==(qcom_i2s_init(1, I2S_FREQ_48K,16,4,1,1024,NULL)))
//		if(I2S_ERR==(qcom_i2s_init(1, I2S_FREQ_48K,8,4,1,1024,NULL)))
		{
				I2S_PRINTF("%s: qcom_i2s_init error!\n", __func__);
				return I2S_ERR;
		}
	}
	else
	{
		if(I2S_ERR==(qcom_i2s_init(1, I2S_FREQ_48K,16,1,4,1024,NULL)))
		{
				I2S_PRINTF("%s: qcom_i2s_init error!\n", __func__);
				return I2S_ERR;
		}
	}
		qcom_i2c_init(I2C_SCK_3,I2C_SDA_5,I2C_FREQ_400K);
			//return I2S_ERR;
		i2s_ctl_flag|=I2S_INIT;
	}
	int i;
	if(UDA1380_SPEAKER == mode){
		for(i=0;i<(UDA1380_CNT);i++){
			if(I2S_ERR == uda1380_write(uda1380_reg[i], uda1380_cfg1[i]))
				return I2S_ERR;
			//printf("\n");
		}
	}
	else if(UDA1380_LINEIN == mode){
		for(i=0;i<(UDA1380_CNT);i++){
			if(I2S_ERR == uda1380_write(uda1380_reg[i], uda1380_cfg2[i]))
				return I2S_ERR;
		}
	}

#if 0
	printf("==============================\n");
	unsigned short val;
		for(i=0;i<(UDA1380_CNT);i++){
			if(I2S_ERR == uda1380_read(uda1380_reg[i], &val))
				return I2S_ERR;
			if(uda1380_cfg1[i] != val)
				printf("%s: i2c check error, index %d, original 0x%08x, fail 0x%08x\n", __func__, i, uda1380_reg[i], val);
		}
#endif

	qcom_thread_msleep(100);
	return I2S_OK;
}

void audio_socket_addr_init(struct sockaddr_in *psrv_add)
{
    	memset(psrv_add, 0, sizeof(struct sockaddr_in));
    	psrv_add->sin_addr.s_addr = htonl(gip_for_i2s);
    	psrv_add->sin_port = htons(gl4_port_for_i2s);
    	psrv_add->sin_family = AF_INET;
	return;
}



/*static char recvBuf[TCP_RX_BUF_LEN] = {0};*/


void audio_play(unsigned int no_used)
{
	int i=0;
	i=0;
	
	if(I2S_ERR == uda1380_config(UDA1380_SPEAKER)){
		I2S_PRINTF("%s: config error!\n", __func__);
		 qcom_task_exit();
		return ;
	}

	I2S_PRINTF("%s: %d run ok\n", __func__, i);
	
	int ret = -1, nRecv, nSend,file_size, len, nTime=0;
//   	char recvBuf[TCP_RX_BUF_LEN] = {0};
	char	*sendBuf, file_name[32];
    	q_fd_set sockSet;
    	int fd_act=0, err_times, nBytes=0, nCount;
    	struct timeval tmo;
    	int socket_serv = -1;
    	int socket_clnt = -1;
    	struct sockaddr_in sock_add;
    	struct sockaddr_in clnt_addr;
    	char ack[3]= "OK";

	char *recvBuf=NULL;

	recvBuf = qcom_mem_alloc(TCP_RX_BUF_LEN);
	if(!recvBuf)
		goto audio_s_task_exit;
		
   	 /* create TCP socket */
    	socket_serv = qcom_socket(AF_INET, SOCK_STREAM, 0);
    	if(socket_serv <=0)
    	{
    		I2S_PRINTF("%s: socket error!\n", __func__);
       	 goto done;
    	}
	
   	 /* bind to a port */
    	audio_socket_addr_init(&sock_add);
    	ret = qcom_bind(socket_serv, (struct sockaddr *)&sock_add, sizeof(struct sockaddr_in));
	if(ret<0)
		I2S_PRINTF("%s: bind error!\n", __func__);
	
    	/* listen on the port */
    	ret = qcom_listen(socket_serv, 10);
	if(ret<0)
		I2S_PRINTF("%s: listen error!\n", __func__);

    	printf("Tcp server is listenning on port %d...\n", gl4_port_for_i2s);

    	/* wait for connection */
    	FD_ZERO(&sockSet);
    	FD_SET(socket_serv, &sockSet);
    	tmo.tv_sec = 120;
   	tmo.tv_usec = 0;

    	fd_act = qcom_select(2, &sockSet, NULL, NULL, &tmo);
    	if(fd_act <= 0)
    	{
        	I2S_PRINTF("No connection in %ld seconds\n", tmo.tv_sec);
        	goto done;
    	}

    	/* accept connection from client */
    	socket_clnt = qcom_accept(socket_serv, (struct sockaddr *)&clnt_addr, &len);
    	if(socket_clnt<=0)
        	goto done;
    
    	printf("Accept connection from ");
    	IPv4_STR_PRT((int)(ntohl(clnt_addr.sin_addr.s_addr)));
    	printf(": %d\n", ntohs(clnt_addr.sin_port));

    	/* recv first packet */
    	qcom_thread_msleep(500);
    	nRecv = qcom_recv(socket_clnt, recvBuf, TCP_RX_BUF_LEN, 0);
    	I2S_PRINTF("nRecv = %d\n", nRecv);
    	file_size = ntohl(*(int*)recvBuf);
    	memcpy(file_name, recvBuf+4, nRecv-4);
    	file_name[nRecv-4] = 0;
    	I2S_PRINTF("Sending file %s to i2s, size = %d\n", file_name, file_size);
    	qcom_thread_msleep(500);

    	/* send ACK */
    	nSend = qcom_send(socket_clnt, ack, 3, 0);
    	qcom_thread_msleep(800);

    	/* start the loop */
    	nTime = time_ms();
    	while(1)
    	{
	       /* init fd_set */
	       FD_ZERO(&sockSet);
	       FD_SET(socket_clnt, &sockSet);
	       tmo.tv_sec = 1; //10;
	       tmo.tv_usec = 0;
       	fd_act = qcom_select(2, &sockSet, NULL, NULL, &tmo);
	       if(fd_act <=0)
	       {
	            I2S_PRINTF("No packet received in %ld seconds\n", tmo.tv_sec);
	            err_times++;
	       }
       else
       {
            if(FD_ISSET(socket_clnt, &sockSet))
            {
			nRecv = qcom_recv(socket_clnt, recvBuf, TCP_RX_BUF_LEN, 0);
                	if(nRecv > 0)
                    	nSend = qcom_send(socket_clnt, ack, 3, 0);
                	else
                	{
                        	I2S_PRINTF("\nRecv %s done\n", file_name);
                        	break;
                	}  
                	/* write data to i2s */
                	sendBuf = recvBuf;
			if(nRecv > 0){
				while(qcom_i2s_xmt_data(1, (A_UINT8 *)sendBuf,nRecv))
					;
				nBytes += nRecv;
				nRecv = 0;
			}
			if(nRecv > 0)
				err_times++;
			else
			{
				nCount++;
				if((nCount%200) == 0)
					printf("* ");
				if((nCount%4000) == 0)
	                        	printf("\n");
			}
		}
       }
       if(err_times > 5)
       	{
       	printf("%s: too many error, break.\n", __func__);
            break;
       	}
    }    

   	nTime = time_ms()-nTime;
    	printf("\ntcp_2_i2s test done. Write %d bytes to i2s in %d seconds\n", nBytes, nTime/1000);
done:
    /* close TCP sockets */
    	if (socket_clnt > 0)
        	qcom_close(socket_clnt);

    	if (socket_serv > 0)
        	qcom_close(socket_serv);

	qcom_mem_free(recvBuf);
	recvBuf = NULL;
	
audio_s_task_exit:    
    	qcom_task_exit();


    	return;
}

void cli_audio_paly(void)
{

	gip_for_i2s = INADDR_ANY;
    	gl4_port_for_i2s = 6000;
    	qcom_task_start(audio_play, 0, 2048, 50);
	I2S_PRINTF("%s: retrun ok.\n", __func__);
    	return;
}



/**********************************************************
 **********************************************************
 ********************   recorder  process   ********************
 **********************************************************
 **********************************************************/

unsigned int i2s_data_arrived=0;
void i2s_rx_intr_cb(void *ctx ,A_UINT8 * bufp, A_UINT32 size)
{
	i2s_data_arrived=1;
}

void audio_data_send(unsigned char * bufp, unsigned int size)
{	
	char szbuf[TCP_TX_BUF_LEN] = {0};
	int ret =-1;
	int i2s2tcp_buf_len;
	q_fd_set fd_set_i2s2tcp;
	int fd_act_i2s2tcp=0;
	struct timeval timeout;
	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
		if((i2s_ctl_flag&I2S_SEND_FINI)&&(i2s_ctl_flag&I2S_SEND_STOP)){
			qcom_i2s_rcv_control(1, 1);
			i2s_ctl_flag |= I2S_REC_STOP;
		}
		else{
			i2s2tcp_buf_len = size;
			memset(szbuf, 0, i2s2tcp_buf_len);
			memcpy(szbuf, bufp, i2s2tcp_buf_len);
			if (!(i2s_ctl_flag&I2S_SEND_STOP))
			{
				qcom_send(socket_conn, szbuf, i2s2tcp_buf_len, 0);
				qcom_thread_msleep(5);
				/* wait for ack */
				#if 1
				QCA_FD_ZERO(&fd_set_i2s2tcp);
				QCA_FD_SET(socket_conn, &fd_set_i2s2tcp);
				fd_act_i2s2tcp = qcom_select(socket_conn + 1, &fd_set_i2s2tcp, NULL, NULL, &timeout);
				if(fd_act_i2s2tcp != 0)
				{
					if(QCA_FD_ISSET(socket_conn, &fd_set_i2s2tcp)) 
					{
						memset(szbuf, 0, sizeof(szbuf));
						ret = qcom_recv(socket_conn, szbuf, 16, 0);
						if (ret > 0)
						{

						}
					}
				}
				else
				{
					I2S_PRINTF("select socket timeout\n");
				}
				#endif
				cnt_i2s_blk++;
				if((cnt_i2s_blk%50) == 0)
					printf("* ");
				if((cnt_i2s_blk%1000) == 0)
	                        	printf("\n");
			}
				  /* no more data */
			else
			{
				memset(szbuf, 0, sizeof(szbuf));
				sprintf(szbuf, "end of file");
				i2s2tcp_buf_len = strlen(szbuf);
				qcom_send(socket_conn, szbuf, i2s2tcp_buf_len, 0);
				i2s_ctl_flag |= I2S_SEND_FINI;
			}
        	}
	
}


void audio_data_process(void)
{
	unsigned char data_rcv[1024];
	unsigned int size=0;
	int ret=-1;

	//if(i2s_data_arrived)
	if (1)
	{
		i2s_data_arrived = 0;
		ret = qcom_i2s_rcv_data(1, data_rcv, 1024, &size);
//		printf("%s: ret %d, size %d\n", __func__, ret, size);

	}


	if(size)
		
	{


	         audio_data_send( data_rcv, 1024);

#if 0
			 if(size != 1024)
			 {
			 	printf("%s: size %d not match, why ???\n", __func__, size);
			 }
#endif
		//printf("%s: size %d \n", __func__, size);

	}

	if(!ret)
	{
//		printf("%s: waiting for some data ...\n", __func__);
		qcom_thread_msleep(5);
	}
}

void audio_recorder_main(unsigned int no_used)
{
	int ret = -1;
	int socket_serv = -1;
	int i2s2tcp_buf_len = 0;
	struct sockaddr_in sock_add;
	struct sockaddr_in clnt_addr;

	i2s_ctl_flag &= I2S_INIT;
	cnt_i2s_blk = 0;
	socket_conn = -1;
	socket_serv = qcom_socket(AF_INET, SOCK_STREAM, 0);
	if (socket_serv < 0)
	{
		goto done;
	}

	//printf("#########create socket %d.#########\n",socket_serv);
	audio_socket_addr_init(&sock_add);
	ret = qcom_bind(socket_serv, (struct sockaddr *)&sock_add, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		I2S_PRINTF("Failed to bind socket %d.\n", socket_serv);
		goto done;
	}

	ret = qcom_listen(socket_serv, 10);
	if (ret < 0)
	{
		I2S_PRINTF("Failed to listen socket %d.\n", socket_serv);
		goto done;
	}

	printf("Tcp server is listenning on port %d...\n", gl4_port_for_i2s);

	socket_conn = qcom_accept(socket_serv, (struct sockaddr *)&clnt_addr, &i2s2tcp_buf_len);
	if (socket_conn < 0)
	{
		goto done;
	}

	printf("Accept connection from ");
	IPv4_STR_PRT((int)(ntohl(clnt_addr.sin_addr.s_addr)));
	printf(": %d\n", ntohs(clnt_addr.sin_port));
	printf("please start speaking....\n");
	int nTime = 0;
	nTime = time_ms();
	while(!(i2s_ctl_flag&I2S_REC_STOP))
	{

#if 0
		printf("audio_data_process\n");
		qcom_thread_msleep(200);
#else
		audio_data_process();
#endif

	}
	printf("recorder test end....\n");
	nTime = time_ms()-nTime;
	printf("\nrecv %d bytes in %d ms\n",cnt_i2s_blk*I2S_BUF_SIZE,nTime);
	qcom_thread_msleep(200);
	
done:
	if (socket_conn > 0)
	{
		qcom_close(socket_conn);
	}
	if (socket_serv > 0)
	{
		qcom_close(socket_serv);
	}


	qcom_task_exit();
	return;
}
void cli_audio_recorder(void)
{
	uda1380_config(UDA1380_LINEIN);
	qcom_thread_msleep(2000);
	qcom_i2s_rcv_control(1,0);
	qcom_thread_msleep(1);
	gip_for_i2s = INADDR_ANY;
	gl4_port_for_i2s = 6000;
	qcom_task_start(audio_recorder_main, 0, 4096, 10);
}


/**********************************************************
 **********************************************************
 **********************     loopback test      *******************
 **********************************************************
 **********************************************************/

void audio_loopback_test(unsigned int unused)
{

	int i, ret;
	A_UINT32 len;

	A_UINT8 audio_pattern_written[1024];
	A_UINT8 audio_pattern_read[1024];

	A_UINT8 *p=audio_pattern_read;
	p=audio_pattern_read;

	for(i=0; i<1024;i++)
		audio_pattern_written[i] = 0x5a;

	i=0;
	qcom_i2s_init(1, 1, 16, 3, 3, 1024, NULL);


	ret = qcom_i2s_xmt_data(1, (A_UINT8 *)audio_pattern_written,1024);

	qcom_i2s_rcv_control(1,0);
	while(1)
	{
		ret = qcom_i2s_xmt_data(1, (A_UINT8 *)audio_pattern_written,1024);
		if(!ret)
		{
			A_MEMSET(audio_pattern_read, 0, 1024);
			ret = qcom_i2s_rcv_data(1, audio_pattern_read, 1024, &len);

			if(len)
			{
				i++;
				ret = A_MEMCMP(audio_pattern_written, audio_pattern_read, 1024);
				if(ret)
					I2S_PRINTF("%s: not equal!0x%08x\n", __func__, *(A_UINT32 *)p);
				else
				{
					if(!(i%10))
						I2S_PRINTF("%s: ==> 0x%08x\n", __func__, *(A_UINT32 *)p);
				}

			}
			else
			{
				qcom_thread_msleep(5);
			}
		}
		else
		{
			qcom_thread_msleep(5);
		}
		
		
	}

}


void cli_audio_loopback_test(void)
{
	qcom_task_start(audio_loopback_test, 0, 2048, 50);
}


char *i2s_help="i2s { speak | recorder} { start | stop }\n";

int cmd_i2s_test(int argc, char *argv[])
{
	if(argc <2)
    	{
	        printf("%s", i2s_help);
	        return -1;
   	}
	if(!A_STRCMP(argv[1], "speak"))
	{
		if (argc<3)
		{
			printf("%s", i2s_help);
			return -1;
		}

		if(!A_STRCMP(argv[2], "start"))
		{
			extern void cli_audio_paly(void);
			cli_audio_paly();
			I2S_PRINTF("i2s speaker test start...\n");
		}
		else if (!A_STRCMP(argv[2], "stop"))
		{
			I2S_PRINTF("i2s speaker test stop...\n");
		}
	}
#if 1
	else if(!A_STRCMP(argv[1], "recorder"))
	{
		if (argc<3)
		{
			printf("%s", i2s_help);
			return -1;
		}

		if(!A_STRCMP(argv[2], "start"))
		{
			extern void cli_audio_recorder(void);

			printf("for sdi pin confilict, please inser the sdi pin after 10 seconds...\n");
			qcom_thread_msleep(10000);
			
			I2S_PRINTF("i2s recorder test start...\n");
			cli_audio_recorder();
		}
		else if (!A_STRCMP(argv[2], "stop"))
		{
			extern int i2s_ctl_flag;
			i2s_ctl_flag|=0x2;
			I2S_PRINTF("i2s recorder test stop...\n");
		}
	}

#endif
	else if(!A_STRCMP(argv[1], "loopback"))
		{
			extern void cli_audio_loopback_test(void);

			cli_audio_loopback_test();
			I2S_PRINTF("i2s loopback test ...\n");

		}
	else{
		printf("%s", i2s_help);
	}

	return;
}
