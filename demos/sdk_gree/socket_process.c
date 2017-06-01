
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


/* 服务器 */
server_t udpServer;

/* 建立的tcp连接 */
tcp_accept_t tcpAccept;

client_t tcpClient;



char *packLeft   = "<<<";
char *packRight = ">>>";




/*-------------------------------------------------------------------------
	获取ip成功后，马上创建udp server,不同的端口不同的服务器
	返回值: 0:成功；<0:错误 
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
    获取ip成功后，马上创建TCP server
    返回值: 0:成功；<0:错误
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
    创建tcp client
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
    初始化服务器
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
	/* 重新创建 */
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

	/* tcp关闭 */
	if(tcpClient.tcpInit == 0){
		if(tcpClient.tcpSock > 0){
			qcom_close(tcpClient.tcpSock);
			tcpClient.tcpSock = 0;
			memset(&tcpClient, 0, sizeof(client_t));		/* 初始化客户端 */
			tcpClient.port = TCP_LISTEN_PORT;
			LEDShow.ledState = 2;
		}
				
	}

	/* tcp重新创建 */
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
			tcpClient.tcpInit = 5;		/* 需连接服务器 */	
			tcpClient.connectFailTimes = 0;	
		}else{
			qcom_thread_msleep(1000);	/* 重新创建 */			
		}
	}
	
	/* tcp连接 */
	if(tcpClient.tcpInit == 5){
		ret = tcpClient_connectServer(&tcpClient);
		tcp_printf("\n tcpclient(%d) connect ret(%d)", tcpClient.tcpSock, ret);
		if(ret == 0){		/* 连接成功 */
			tcpClient.connectFailTimes = 0;		
			
			if(udpServer.ctrlflag == 3){
				udpServer.ctrlflag = 4;		/* */
				tcpClient.tcpInit = 1;		/* 重连 */
			}else{

				if(udpServer.ctrlflag == 2){
					udpServer.rxReply = 0;
					udpServer.ctrlflag = 0xff;		/* udp回复 */		
				}else if(udpServer.ctrlflag == 4){
					udpServer.rxReply = 1;
					udpServer.ctrlflag = 0xff;		/* udp回复 */		
				}else{

				}
					
				tcpClient.tcpInit = 0xff;
				LEDShow.ledState = 1;

				/* 保存ip */
				if(tDeviceInfo.ip != tcpClient.ip){
					tDeviceInfo.ip = tcpClient.ip;
					device_information_set(&tDeviceInfo);
				}

			}
			
		}
		else{		/* 连接失败 */
			tcpClient.connectFailTimes++;

			if(udpServer.ctrlflag == 3){
				udpServer.ctrlflag = 4;		/* */
				tcpClient.tcpInit = 1;		/* 重连 */
			}else{
				if(tcpClient.connectFailTimes > 3){		/* 重连四次之后重新创建 */
					tcpClient.tcpInit = 1;	
				}else{
					tcpClient.tcpInit = 3;		/* 等待重连 */
					tcpClient.reconnectCounts = 0;
					tcpClient.reconnectWaitSecond = 3;
				}

				if(udpServer.ctrlflag == 2 || udpServer.ctrlflag == 4){ 	/* 如果有udp在等待回复则回复失败 */
					udpServer.rxReply = 3;
					udpServer.ctrlflag = 0xff;
				}			

			}
		
		}
	
	}

	/* tcp正常通讯  */
	if(tcpClient.tcpInit == 0xff){
		if(udpServer.ctrlflag == 3){
			udpServer.ctrlflag = 4; 	/* */
			tcpClient.tcpInit = 1;		/* 重连 */
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

		/* 关闭旧的连接 */
		if(tcpAccept.socket > 0){
			qcom_close(tcpAccept.socket);
		}
		memset(&tcpAccept, 0, sizeof(tcp_accept_t));

		/* 保存新的socket */
		tcpAccept.socket = newSocket;
		tcpAccept.ip = ip;
		tcpAccept.port = port;
		tcp_printf(" ip:0x%08x port:%d.", (uint)tcpAccept.ip, tcpAccept.port);
		tcpAccept.status = 0xff;		/* socket正常使用 */
		ret = 0;
		LEDShow.ledState = 1;

	}
	else{
		tcp_printf("Accept new socket fail!");
		pTCP->initflag = 2; 	/* 需重新初始化 */
		ret = -1;	
	}
 	return ret;
	
}


/*   P A R S E _ H E X 2   */
/*-------------------------------------------------------------------------
	将最多连续两个字符转化为16进制数
	不检测第三个字节的存在性
-------------------------------------------------------------------------*/
unsigned char  string_tohex2(const char *str)
{
    	unsigned char h = 0;

    	if (*str>='0' && *str<='9') { h += (*str)-'0';}
    	else if (*str>='A' && *str<='F') { h += 10+(*str)-'A';}
    	else if (*str>='a' && *str<='f') { h += 10+(*str)-'a';}
    	else { return 0;}

	str++;	
	if(*str != 0){	/* 还有第二个字符 */
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
    udp服务器的接收
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
			
			if(pUDP->ctrlflag == 0){		/* 开始新的udp流程 */
				if(ip > 0){
					if (ip == tcpClient.ip) {		/* ip相同 */
						if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){ 	/* 已建立连接 */
							pUDP->ctrlflag = 0xff;
							pUDP->rxReply = 2;
						}else{
							pUDP->ctrlflag = 2; 				
						}
					
					} else {	/* ip 不同 */
					
						tcpClient.ip = ip;					
						
						if (tcpClient.tcpInit == 0) {	/* 第一次接收到udp数据 */
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
			else{	/* 前面的udp还未发送回复 */
				if(ip > 0){
					if (ip == tcpClient.ip) {		/* 不做操作，由于ip相同，故只回复一个即可 */

					} else {	/* ip 不同 */
					
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
    udp服务器的接收
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
			
			if(memcmp(macTemp, tDeviceInfo.uMAC, 6) == 0){		/* 检测到mac一致，若还没保存路由信息，则进行保存 */
				udp_printf(" find mac. ");
				return code = 0x00;
			}			
		}

		if(code != 0x00){	/* 没有检测到MAC,自动退出 */
			udp_printf(" cann't find mac. ");
			code = 0x01;
			
			memset(tDeviceInfo.tWlanInfo.ssid1, 0, SSID_LEN);	/* 擦除该路由信息 */
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
    将tcp接收的数据进行分包
    rxbuf，接收的数据
    rxlen，接收数据的字节数
    offset，从接收数据中的那个字节开始分包
    sav，分包后数据的保存起始地址
    len，sav中有效数据字节数
    
    返回:0:分包结束，没有完整数据据包   >0:完整包的长度
    
    char *strstr(const char *string, const char *strSearch); 
	在字符串string中查找strSearch子串.  返回子串strSearch在string中首次出现位置的指针. 
	如果没有找到子串strSearch, 则返回NULL. 如果子串strSearch为空串, 函数返回string值. 
	执行效率差不多。
-------------------------------------------------------------------------*/
int tcp_subPackage(char *rxbuf, int rxlen, int *offset, char *sav, int *len)
{
	int packlen = 0;
	char *paddr;
	
	/* 从前面已处理过(没有处理过的即从0开始)的包的后面开始查询 */
	paddr = strstr(rxbuf + *offset, packLeft);		/* <<< */
	if(paddr){	
		packlen = (int)paddr - (int)(rxbuf + *offset);
		if(packlen == 0){	/* 数据包开头在开始查询的位置 */
			paddr = strstr(rxbuf + *offset, packRight);		/* >>> */
			if(paddr){		/* 找到数据包结尾的位置 */
				packlen = (int)paddr - (int)(rxbuf + *offset) + 3;	/* 结束符有3个字节*/
				memcpy(sav, rxbuf + *offset, packlen);
				*offset += packlen;
				*(sav + packlen) = 0;
				*len = 0;				
			}
			else{	/* 没找到数据包的结尾，则将此部分的数据保存下来 */
				*len = rxlen - *offset;
				memcpy(sav, rxbuf + *offset, *len);
				*offset = rxlen;
				packlen = 0;				
			}
		}else{	/* 数据包头不在开始查询的位置，即前面有部分数据不完整，提取出前面部分数据 */
			packlen = (int)paddr - (int)(rxbuf + *offset);
			memcpy(sav + *len, rxbuf + *offset, packlen);
			*offset += packlen;
			packlen += *len;
			*(sav + packlen) = 0;
			*len = 0;
		}
	}
	else{
		/* 没有数据包的开头 */ 
		paddr = strstr(rxbuf + *offset, packRight); 	/* >>> */
		if(paddr){		/* 找到数据包结尾的位置 */
			packlen = (int)paddr - (int)(rxbuf + *offset) + 3;	/* 结束符有3个字节*/
			memcpy(sav + *len, rxbuf + *offset, packlen);			
			*offset += packlen;
			packlen += *len;
			*(sav + packlen) = 0;
			*len = 0;				
		}else{	/* 没找到数据包结尾位置，即数据包在 <<< 处被分割 */
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
    tcp服务器已建立连接的socket的接收
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
    初始化socket
-------------------------------------------------------------------------*/
void nearSocketTask_init(void)
{

	memset(&udpServer, 0, sizeof(server_t));
	udpServer.port = UDP_LISTEN_PORT;
	udpServer.initflag = 0x02;	/* 需创建 */
	
	memset(&tcpClient, 0, sizeof(client_t));		/* 初始化客户端 */
	if(tDeviceInfo.ip > 0){	//有ip保存，使用该ip进行连接
		tcpClient.ip = tDeviceInfo.ip;	
		tcpClient.tcpInit = 1;
	}

	tcpClient.port = TCP_LISTEN_PORT;
}


/*   N E A R   S E L E C T _ H A N D L E R   */
/*-------------------------------------------------------------------------
    服务器统一接收函数
    只在serverSock_process线程调用，避免重入
-------------------------------------------------------------------------*/
void nearSelect_handler(void)
{
	static int  		socketMax;	
	static q_fd_set 	fdSockSet;
	static int    		fd_act;	
	static struct timeval timeout;	

	socketMax = -1;
	FD_ZERO(&fdSockSet);	
	/* 入口要用标志把控 */
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
			
			/* select阻塞后标志可能被更改，故不能用标志判断 */								
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
	1、初始化UDP、TCP服务器和客户端
	2、建立TCP连接
	3、socket接收和发送
    
-------------------------------------------------------------------------*/
void nearSocketTask(unsigned int input_thread)
{
	nearSocketTask_init(); 

//	qcom_set_connect_timeout(TCP_CLIENT_CONNECT_TIME);	

	while (1) {
		
		if(tNetworkInfo.ipObtained == 1){
			nearSocket_update();
			
			nearSelect_handler();

			if(udpServer.ctrlflag == 0xff){		/* 发送应答  */
				udpServer_send(&udpServer);
				
				if(udpServer.ctrlflag == 0xff){		/* 发送时可能标志已被修改，要重新判断 */
					udpServer.ctrlflag = 0;
				}
			}

//			ip_ping_process();


		}else{
			if(LEDShow.ledState == 0x03){	/* 正在快联 */
				kuaifi_confirm();
			}else{
				qcom_thread_msleep(100);
			}
		}
		


	}
}


