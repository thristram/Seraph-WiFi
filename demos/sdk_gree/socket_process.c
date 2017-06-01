
#include <qcom/qcom_common.h>
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/socket.h>

#include "task_manage.h"
#include "wifi_config.h"
#include "nvram_config.h"
#include "sdk_shell.h"
#include "cjson.h"

#include "uart_process.h"
#include "socket_process.h"
#include "list.h"
#include "AES.h"


/* ������ */
server_t udpServer;

/* ������tcp���� */
tcp_accept_t tcpAccept;

client_t tcpClient;



char *packLeft   = "<<<";
char *packRight = ">>>";




/*-------------------------------------------------------------------------
	��ȡip�ɹ������ϴ���udp server,��ͬ�Ķ˿ڲ�ͬ�ķ�����
	����ֵ: 0:�ɹ���<0:���� 
-------------------------------------------------------------------------*/
int create_udpServer(server_t *pUDP)
{
	int ret = 0;
	
	/* socket exsit */
	if(pUDP->socket > 0) { return ret = -1;}	
	if(pUDP->port <= 0){ return ret = -2;}	/* port err */

	pUDP->socket = qcom_socket(AF_INET, SOCK_DGRAM, 0);
	if(pUDP->socket <= 0) {
		return ret = pUDP->socket;
	}
	pUDP->sockServer.sin_addr.s_addr = htonl(INADDR_ANY);
	pUDP->sockServer.sin_port = htons(pUDP->port);
	pUDP->sockServer.sin_family = AF_INET;

	if (qcom_bind(pUDP->socket, (struct sockaddr *)&pUDP->sockServer, sizeof(struct sockaddr_in)) < 0) {	
		udp_printf("\n udp bind err!");
		ret = -3;
	}
	if (ret < 0 ){
		if(pUDP->socket > 0){
			qcom_close(pUDP->socket);
		}
		pUDP->socket = 0;
	}
	return ret;
	
}



/*-------------------------------------------------------------------------
    ��ȡip�ɹ������ϴ���TCP server
    ����ֵ: 0:�ɹ���<0:����
-------------------------------------------------------------------------*/
int create_tcpServer(server_t *pTCP)
{
	int ret = 0;
	
	/* socket exsit */
	if(pTCP->socket > 0) { return ret = -1;}	
	if(pTCP->port <= 0) { return ret = -2;}
	
	pTCP->socket = qcom_socket(AF_INET, SOCK_STREAM, 0);
	if(pTCP->socket <= 0) {
		return ret = pTCP->socket;
	}
	pTCP->sockServer.sin_addr.s_addr = htonl(INADDR_ANY);
	pTCP->sockServer.sin_port = htons(pTCP->port);
	pTCP->sockServer.sin_family = AF_INET;

	if(qcom_bind(pTCP->socket, (struct sockaddr *)&pTCP->sockServer, sizeof(struct sockaddr_in)) < 0){
		tcp_printf("\n tcp bind err!");
		ret = -3;
	}
	if(qcom_listen(pTCP->socket, 4) < 0){
		tcp_printf("\n tcp listen err!");
		ret = -4;
	}

	if (ret < 0 ){
		if(pTCP->socket > 0) {
			qcom_close(pTCP->socket);
		}		
		pTCP->socket = 0;
	}
	
	return ret;
	
}


/*-------------------------------------------------------------------------
    ����tcp client
    sec,connect time second
-------------------------------------------------------------------------*/
int create_tcpClient(client_t *pTCP)
{
	int ret = 0;
	struct linger sLinger;

	/* socket exit */
	if(pTCP->tcpSock > 0) { return ret = -1;}	
	if(pTCP->ip <= 0 && pTCP->port <= 0){ return ret = -2;}		/* IP or port err */
	
	pTCP->tcpSock = qcom_socket(AF_INET, SOCK_STREAM, 0);
	if(pTCP->tcpSock <= 0) {
		return ret = pTCP->tcpSock;
	}	
	pTCP->tcpServer.sin_addr.s_addr = htonl(pTCP->ip);
	pTCP->tcpServer.sin_port = htons(pTCP->port);
	pTCP->tcpServer.sin_family = AF_INET;


	sLinger.l_onoff = 0;
	ret = qcom_setsockopt(pTCP->tcpSock, SOL_SOCKET, SO_LINGER, &sLinger, sizeof(struct linger));
	if (ret < 0) {
		A_PRINTF("\n tcpclient set linger option fail!");

		if(pTCP->tcpSock > 0) {
			qcom_close(pTCP->tcpSock);
		}		
		pTCP->tcpSock = 0;		
	}
	
	return ret;	
	
}


/*-------------------------------------------------------------------------

-------------------------------------------------------------------------*/
int tcpClient_connectServer(client_t * pTCP)
{
	int ret = -1;

	if(pTCP->tcpSock > 0){
		ret = qcom_connect(pTCP->tcpSock, (struct sockaddr *)&pTCP->tcpServer, sizeof(struct sockaddr_in));
	}
	return ret;
}



/*-------------------------------------------------------------------------
    ��ʼ��������
-------------------------------------------------------------------------*/
void nearSocket_update(void)
{
	int ret;
	
	/* udp server close */
	if(udpServer.initflag == 0x01){
		if(udpServer.socket > 0){
			qcom_close(udpServer.socket);		
		}
		memset(&udpServer, 0, sizeof(server_t));
	}
	/* ���´��� */
	if(udpServer.initflag == 0x02){
		if(udpServer.socket > 0){
			qcom_close(udpServer.socket);
			udpServer.socket = 0;	
			qcom_thread_msleep(100);
		}
		ret = create_udpServer(&udpServer);
		udp_printf("\n create udpServer(%d) ret(%d).", udpServer.socket, ret);

		if(ret == 0){			
			udpServer.initflag = 0xff;			
		}else{
			qcom_thread_msleep(1000);		
		}
	}

	/* tcp�ر� */
	if(tcpClient.tcpInit == 0){
		if(tcpClient.tcpSock > 0){
			qcom_close(tcpClient.tcpSock);
			tcpClient.tcpSock = 0;
			memset(&tcpClient, 0, sizeof(client_t));		/* ��ʼ���ͻ��� */
			tcpClient.port = TCP_LISTEN_PORT;
			LEDShow.ledState = 2;
		}
				
	}

	/* tcp���´��� */
	if(tcpClient.tcpInit == 1){
		if(tcpClient.tcpSock > 0){
			qcom_close(tcpClient.tcpSock);
			tcpClient.tcpSock = 0;
			qcom_thread_msleep(100);
		}
		LEDShow.ledState = 2;
		ret = create_tcpClient(&tcpClient);
		tcp_printf("\n create new tcpclient(%d), ret(%d)", tcpClient.tcpSock, ret);

		if(ret == 0){
			tcpClient.tcpInit = 5;		/* �����ӷ����� */	
			tcpClient.connectFailTimes = 0;	
		}else{
			qcom_thread_msleep(1000);	/* ���´��� */			
		}
	}
	
	/* tcp���� */
	if(tcpClient.tcpInit == 5){
		ret = tcpClient_connectServer(&tcpClient);
		tcp_printf("\n tcpclient(%d) connect ret(%d)", tcpClient.tcpSock, ret);
		if(ret == 0){		/* ���ӳɹ� */
			tcpClient.connectFailTimes = 0;		
			
			if(udpServer.ctrlflag == 3){
				udpServer.ctrlflag = 4;		/* */
				tcpClient.tcpInit = 1;		/* ���� */
			}else{

				if(udpServer.ctrlflag == 2){
					udpServer.rxReply = 0;
					udpServer.ctrlflag = 0xff;		/* udp�ظ� */		
				}else if(udpServer.ctrlflag == 4){
					udpServer.rxReply = 1;
					udpServer.ctrlflag = 0xff;		/* udp�ظ� */		
				}else{

				}
					
				tcpClient.tcpInit = 0xff;
				LEDShow.ledState = 1;

				/* ����ip */
				if(tDeviceInfo.ip != tcpClient.ip){
					tDeviceInfo.ip = tcpClient.ip;
					device_information_set(&tDeviceInfo);
				}

			}
			
		}
		else{		/* ����ʧ�� */
			tcpClient.connectFailTimes++;

			if(udpServer.ctrlflag == 3){
				udpServer.ctrlflag = 4;		/* */
				tcpClient.tcpInit = 1;		/* ���� */
			}else{
				if(tcpClient.connectFailTimes > 3){		/* �����Ĵ�֮�����´��� */
					tcpClient.tcpInit = 1;	
				}else{
					tcpClient.tcpInit = 3;		/* �ȴ����� */
					tcpClient.reconnectCounts = 0;
					tcpClient.reconnectWaitSecond = 3;
				}

				if(udpServer.ctrlflag == 2 || udpServer.ctrlflag == 4){ 	/* �����udp�ڵȴ��ظ���ظ�ʧ�� */
					udpServer.rxReply = 3;
					udpServer.ctrlflag = 0xff;
				}			

			}
		
		}
	
	}

	/* tcp����ͨѶ  */
	if(tcpClient.tcpInit == 0xff){
		if(udpServer.ctrlflag == 3){
			udpServer.ctrlflag = 4; 	/* */
			tcpClient.tcpInit = 1;		/* ���� */
		}
	}

}


/*   T C P   S E R V E R _ A C C E P T   C O N N E C T   */
/*-------------------------------------------------------------------------

-------------------------------------------------------------------------*/
int tcpServer_acceptConnect(server_t * pTCP)
{
	int newSocket, ret;
	socklen_t addrlen;
	struct linger sLinger;	
	uint ip;
	ushort port;
	int keepAlive = 1;

	newSocket = -1;
	newSocket = qcom_accept(pTCP->socket, (struct sockaddr *)&pTCP->sockClient, (socklen_t *)&addrlen);	
	tcp_printf("\n tcp(%d) accept new connect:%d.", pTCP->port, newSocket);

	if(newSocket>0){
		ip = ntohl(pTCP->sockClient.sin_addr.s_addr);
		port = ntohs(pTCP->sockClient.sin_port); 		
		sLinger.l_onoff = 0;
		qcom_setsockopt(newSocket, SOL_SOCKET, SO_LINGER, &sLinger, sizeof(struct linger));
		qcom_setsockopt(newSocket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));

		/* �رվɵ����� */
		if(tcpAccept.socket > 0){
			qcom_close(tcpAccept.socket);
		}
		memset(&tcpAccept, 0, sizeof(tcp_accept_t));

		/* �����µ�socket */
		tcpAccept.socket = newSocket;
		tcpAccept.ip = ip;
		tcpAccept.port = port;
		tcp_printf(" ip:0x%08x port:%d.", (uint)tcpAccept.ip, tcpAccept.port);
		tcpAccept.status = 0xff;		/* socket����ʹ�� */
		ret = 0;
		LEDShow.ledState = 1;

	}
	else{
		tcp_printf("Accept new socket fail!");
		pTCP->initflag = 2; 	/* �����³�ʼ�� */
		ret = -1;	
	}
 	return ret;
	
}


/*   P A R S E _ H E X 2   */
/*-------------------------------------------------------------------------
	��������������ַ�ת��Ϊ16������
	�����������ֽڵĴ�����
-------------------------------------------------------------------------*/
unsigned char  string_tohex2(const char *str)
{
    	unsigned char h = 0;

    	if (*str>='0' && *str<='9') { h += (*str)-'0';}
    	else if (*str>='A' && *str<='F') { h += 10+(*str)-'A';}
    	else if (*str>='a' && *str<='f') { h += 10+(*str)-'a';}
    	else { return 0;}

	str++;	
	if(*str != 0){	/* ���еڶ����ַ� */
	    	h=h<<4; 
	    	if (*str>='0' && *str<='9') {h += (*str)-'0';}       	
	    	else if (*str>='A' && *str<='F') {h += 10+(*str)-'A';}      	
	    	else if (*str>='a' && *str<='f') {h += 10+(*str)-'a';}       	
	    	else { return 0;}
	}  

	return h;

}

/*   U D P   S E R V E R _ R E C V   */
/*-------------------------------------------------------------------------
    udp�������Ľ���
-------------------------------------------------------------------------*/
void udpServer_recv(server_t * pUDP)
{
	static char rxbuf[SOCKET_UDP_MIN] = {0};
	int rxlen, i;
	uint ip;
	socklen_t addrSize;

	addrSize = sizeof(struct sockaddr_in);

	memset(rxbuf, 0, SOCKET_UDP_MIN);
	
	rxlen = qcom_recvfrom(pUDP->socket, rxbuf, SOCKET_UDP_MIN, 0, (struct sockaddr *)&pUDP->sockClient, &addrSize); 
	
	udp_printf("\n udp recv %d:", rxlen);
	for(i=0; i< rxlen; i++){
		udp_printf(" %02x", rxbuf[i]);
	}
	
	if(rxlen > 0){

		if(rxbuf[0] == 0xaa && rxbuf[1] == 0xaa && (rxbuf[2]*256 + rxbuf[3]) == (rxlen - 4) && rxbuf[4] == 0x02){
			ip = 	(rxbuf[5] << 24) + (rxbuf[6] << 16) + (rxbuf[7] << 8) + rxbuf[8];
			udp_printf("\n ip = %x", ip);
			
			if(pUDP->ctrlflag == 0){		/* ��ʼ�µ�udp���� */
				if(ip > 0){
					if (ip == tcpClient.ip) {		/* ip��ͬ */
						if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){ 	/* �ѽ������� */
							pUDP->ctrlflag = 0xff;
							pUDP->rxReply = 2;
						}else{
							pUDP->ctrlflag = 2; 				
						}
					
					} else {	/* ip ��ͬ */
					
						tcpClient.ip = ip;					
						
						if (tcpClient.tcpInit == 0) {	/* ��һ�ν��յ�udp���� */
							tcpClient.tcpInit = 1;
							pUDP->ctrlflag = 2;
						} else {
							pUDP->ctrlflag = 3;
						}
					
					}

				}else{
					pUDP->ctrlflag = 0xff;
					pUDP->rxReply = 3;
				}							

				
			}
			else{	/* ǰ���udp��δ���ͻظ� */
				if(ip > 0){
					if (ip == tcpClient.ip) {		/* ��������������ip��ͬ����ֻ�ظ�һ������ */

					} else {	/* ip ��ͬ */
					
						tcpClient.ip = ip;
						pUDP->ctrlflag = 3;
					}

				}				
			}
			
		}
	
	}
	else {
		udp_printf("\n recreat udp.");
		pUDP->ctrlflag = 0;
		pUDP->initflag = 2;
	}

}



/*   U D P   S E R V E R _ R E C V   */
/*-------------------------------------------------------------------------
    udp�������Ľ���
-------------------------------------------------------------------------*/
void udpServer_send(server_t * pUDP)
{
	char txbuf[64] = {0};
	int txlen, ret;
	socklen_t addrSize;

	addrSize = sizeof(struct sockaddr_in);

	memset(txbuf, 0, 64);
	txlen = 22;
	txbuf[0] = 0xaa;
	txbuf[1] = 0xaa;
	txbuf[2] = (txlen -4) / 256;
	txbuf[3] = (txlen -4) % 256;
	txbuf[4] = 0x03;
	txbuf[5] = pUDP->rxReply;
	memcpy(&txbuf[6], kuaifiKey.idKey, AES_KEY_LEN);
	 	
	ret = qcom_sendto(pUDP->socket , txbuf, txlen, 0, (struct sockaddr *)&(pUDP->sockClient), addrSize);
	udp_printf("\n udp send %d:", txlen);


}





/*   U D P   S E R V E R _ P R O C E   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
int udpServer_proce(char * rxbuf, int rxlen)
{
	int code = 0x09, i;
	char * macStart;
	uchar macTemp[6] = {0};

	macStart = strstr(rxbuf, "MAC=");
	if(macStart){
		udp_printf("\n MAC= OK");
		macStart += 4;
		while(*macStart){

			for(i=0; i<6; i++){
				macTemp[i] = string_tohex2(macStart);
				macStart += 3;												
			}
			
			if(memcmp(macTemp, tDeviceInfo.uMAC, 6) == 0){		/* ��⵽macһ�£�����û����·����Ϣ������б��� */
				udp_printf(" find mac. ");
				return code = 0x00;
			}			
		}

		if(code != 0x00){	/* û�м�⵽MAC,�Զ��˳� */
			udp_printf(" cann't find mac. ");
			code = 0x01;
			
			memset(tDeviceInfo.tWlanInfo.ssid1, 0, SSID_LEN);	/* ������·����Ϣ */
			memset(tDeviceInfo.tWlanInfo.key1, 0, KEY_LEN);
			device_information_set(&tDeviceInfo);

		}

	}else{
		udp_printf("\n MAC= XX");
		code = 0x02;
	}	
	return code;

}




/*   T C P _ S U B   P A C K A G E   */
/*-------------------------------------------------------------------------
    ��tcp���յ����ݽ��зְ�
    rxbuf�����յ�����
    rxlen���������ݵ��ֽ���
    offset���ӽ��������е��Ǹ��ֽڿ�ʼ�ְ�
    sav���ְ������ݵı�����ʼ��ַ
    len��sav����Ч�����ֽ���
    
    ����:0:�ְ�������û���������ݾݰ�   >0:�������ĳ���
    
    char *strstr(const char *string, const char *strSearch); 
	���ַ���string�в���strSearch�Ӵ�.  �����Ӵ�strSearch��string���״γ���λ�õ�ָ��. 
	���û���ҵ��Ӵ�strSearch, �򷵻�NULL. ����Ӵ�strSearchΪ�մ�, ��������stringֵ. 
	ִ��Ч�ʲ�ࡣ
-------------------------------------------------------------------------*/
int tcp_subPackage(char *rxbuf, int rxlen, int *offset, char *sav, int *len)
{
	int packlen = 0;
	char *paddr;
	
	/* ��ǰ���Ѵ����(û�д�����ļ���0��ʼ)�İ��ĺ��濪ʼ��ѯ */
	paddr = strstr(rxbuf + *offset, packLeft);		/* <<< */
	if(paddr){	
		packlen = (int)paddr - (int)(rxbuf + *offset);
		if(packlen == 0){	/* ���ݰ���ͷ�ڿ�ʼ��ѯ��λ�� */
			paddr = strstr(rxbuf + *offset, packRight);		/* >>> */
			if(paddr){		/* �ҵ����ݰ���β��λ�� */
				packlen = (int)paddr - (int)(rxbuf + *offset) + 3;	/* ��������3���ֽ�*/
				memcpy(sav, rxbuf + *offset, packlen);
				*offset += packlen;
				*(sav + packlen) = 0;
				*len = 0;				
			}
			else{	/* û�ҵ����ݰ��Ľ�β���򽫴˲��ֵ����ݱ������� */
				*len = rxlen - *offset;
				memcpy(sav, rxbuf + *offset, *len);
				*offset = rxlen;
				packlen = 0;				
			}
		}else{	/* ���ݰ�ͷ���ڿ�ʼ��ѯ��λ�ã���ǰ���в������ݲ���������ȡ��ǰ�沿������ */
			packlen = (int)paddr - (int)(rxbuf + *offset);
			memcpy(sav + *len, rxbuf + *offset, packlen);
			*offset += packlen;
			packlen += *len;
			*(sav + packlen) = 0;
			*len = 0;
		}
	}
	else{
		/* û�����ݰ��Ŀ�ͷ */ 
		paddr = strstr(rxbuf + *offset, packRight); 	/* >>> */
		if(paddr){		/* �ҵ����ݰ���β��λ�� */
			packlen = (int)paddr - (int)(rxbuf + *offset) + 3;	/* ��������3���ֽ�*/
			memcpy(sav + *len, rxbuf + *offset, packlen);			
			*offset += packlen;
			packlen += *len;
			*(sav + packlen) = 0;
			*len = 0;				
		}else{	/* û�ҵ����ݰ���βλ�ã������ݰ��� <<< �����ָ� */
			*len = rxlen - *offset;
			memcpy(sav, rxbuf + *offset, *len);
			*offset = rxlen;
			packlen = 0;
		}
	}

	return packlen;

}



/*   T C P   A C C E P T _ R E C V   */
/*-------------------------------------------------------------------------
    tcp�������ѽ������ӵ�socket�Ľ���
-------------------------------------------------------------------------*/
void tcpAccept_recv(tcp_accept_t * pAccept)
{	
	pAccept->len = qcom_recv(pAccept->socket, pAccept->recv, SOCKET_TCP_MIN, 0);
	tcp_printf("\n tcp recv:%d.", pAccept->len);
	
	if(pAccept->len > 0){		

		tx_event_flags_get(&uartTxSLEventGroup, uartTxSLEventFlag, TX_AND_CLEAR, &uartTxSLActualFlag, 100*1000);
		addNodeToUartTxSLLast(pAccept->recv, pAccept->len);
		uart_printf("save.");
		tx_event_flags_set(&uartTxSLEventGroup, uartTxSLEventFlag, TX_OR);
		
	}
	else{		
		tcp_printf("R XX! close tcp accept!");
		pAccept->status = 1;
	}
	
}


/*-------------------------------------------------------------------------
    tcp client recv.
-------------------------------------------------------------------------*/
void tcpClient_recv(client_t * pClient)
{	
	pClient->len = qcom_recv(pClient->tcpSock, pClient->recv, SOCKET_TCP_MIN, 0);
	tcp_printf("\n tcpClient recv:%d.", pClient->len);
	
	if(pClient->len > 0){		

		tx_event_flags_get(&uartTxSLEventGroup, uartTxSLEventFlag, TX_AND_CLEAR, &uartTxSLActualFlag, 100*1000);
		addNodeToUartTxSLLast(pClient->recv, pClient->len);
		uart_printf("save.");
		tx_event_flags_set(&uartTxSLEventGroup, uartTxSLEventFlag, TX_OR);
		
	}
	else{		
		tcp_printf("recreat tcpClient!");
		pClient->tcpInit = 1;
	}
	
}




/*   N E A R _   S O C K E T _   T A S K _   I N I T   */
/*-------------------------------------------------------------------------
    ��ʼ��socket
-------------------------------------------------------------------------*/
void nearSocketTask_init(void)
{

	memset(&udpServer, 0, sizeof(server_t));
	udpServer.port = UDP_LISTEN_PORT;
	udpServer.initflag = 0x02;	/* �贴�� */
	
	memset(&tcpClient, 0, sizeof(client_t));		/* ��ʼ���ͻ��� */
	if(tDeviceInfo.ip > 0){	//��ip���棬ʹ�ø�ip��������
		tcpClient.ip = tDeviceInfo.ip;	
		tcpClient.tcpInit = 1;
	}

	tcpClient.port = TCP_LISTEN_PORT;
}


/*   N E A R   S E L E C T _ H A N D L E R   */
/*-------------------------------------------------------------------------
    ������ͳһ���պ���
    ֻ��serverSock_process�̵߳��ã���������
-------------------------------------------------------------------------*/
void nearSelect_handler(void)
{
	static int  		socketMax;	
	static q_fd_set 	fdSockSet;
	static int    		fd_act;	
	static struct timeval timeout;	

	socketMax = -1;
	FD_ZERO(&fdSockSet);	
	/* ���Ҫ�ñ�־�ѿ� */
	if(udpServer.socket > 0 && udpServer.initflag == 0xff){
		FD_SET(udpServer.socket, &fdSockSet); 		
		if(socketMax < udpServer.socket) {socketMax = udpServer.socket;}
	}
	if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){
		FD_SET(tcpClient.tcpSock, &fdSockSet); 		
		if(socketMax < tcpClient.tcpSock) {socketMax = tcpClient.tcpSock;}
	}

	if(socketMax > 0){
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		fd_act = 0;
		fd_act = qcom_select(socketMax+1, &fdSockSet, NULL, NULL, &timeout);		
		if (fd_act > 0) {
			
			/* select�������־���ܱ����ģ��ʲ����ñ�־�ж� */								
			if(udpServer.socket > 0){
				if (FD_ISSET(udpServer.socket, &fdSockSet)) {
					 udpServer_recv(&udpServer);		
				}
			}	
			
			if(tcpClient.tcpSock > 0){
				if (FD_ISSET(tcpClient.tcpSock, &fdSockSet)) {
					 tcpClient_recv(&tcpClient);		
				}
			}
	
		}
	}
	else{
		qcom_thread_msleep(1000);
	}	
	
}


/*   N E A R _   S O C K E T _   T A S K   */
/*-------------------------------------------------------------------------
	1����ʼ��UDP��TCP�������Ϳͻ���
	2������TCP����
	3��socket���պͷ���
    
-------------------------------------------------------------------------*/
void nearSocketTask(unsigned int input_thread)
{
	nearSocketTask_init(); 

//	qcom_set_connect_timeout(TCP_CLIENT_CONNECT_TIME);	

	while (1) {
		
		if(tNetworkInfo.ipObtained == 1){
			nearSocket_update();
			
			nearSelect_handler();

			if(udpServer.ctrlflag == 0xff){		/* ����Ӧ��  */
				udpServer_send(&udpServer);
				
				if(udpServer.ctrlflag == 0xff){		/* ����ʱ���ܱ�־�ѱ��޸ģ�Ҫ�����ж� */
					udpServer.ctrlflag = 0;
				}
			}

//			ip_ping_process();


		}else{
			if(LEDShow.ledState == 0x03){	/* ���ڿ��� */
				kuaifi_confirm();
			}else{
				qcom_thread_msleep(100);
			}
		}
		


	}
}


