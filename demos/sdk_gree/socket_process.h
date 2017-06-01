
#ifndef __SOCKET_MANAGE_H__
#define __SOCKET_MANAGE_H__

#include <qcom/basetypes.h>
#include <qcom/base.h>
#include <qcom/socket.h>


#define  UDP_LISTEN_PORT  	1888
//#define  UDP_LISTEN_PORT  	838
#define  TCP_LISTEN_PORT  		1838


#define  SOCKET_TCP_MAX  		2048		/* < 1500-20-20=1460 近程通讯不需要加密，有效数据长度使用最大值*/
#define  SOCKET_TCP_MIN   		1024	

#define  SOCKET_UDP_DEF   		1024
//#define  SOCKET_UDP_MIN   		512  		/* < 576-8-20=548 适用于远程 */
#define  SOCKET_UDP_MIN   		256  		/* < 576-8-20=548 适用于远程 */


#define  TCP_CLIENT_CONNECT_TIME   		3000  


typedef struct {
	
	int socket;
	ushort port;	/* 服务器端口 */
	uchar initflag;	/* 0:无使用,要全部清零  1:需要关闭  2:需要重新创建    ff:初始化完成 */
	
	uchar ctrlflag;	/* 0.没有操作或流程结束  
				   1.接收到广播，开始流程 
				   2.第一次接收到ip或接收到相同ip后等待连接完成
				   3.接收到ip不一致 
				   4.接收到ip不一致后重新建立连接 
				   0xff.udp发送回复 
				   */
				   
	uchar rxReply;		/* 0.收到指令，并连接到新的TCP Server，连接正确
					   1.收到指令，退出当前TCP Server 并重新连接到新的TCP Server
					   2.收到指令，但由于目标IP 地址跟目前连接的IP 地址一样，不操作
					   3.无法连接到TCP Server
					   */
	struct sockaddr_in sockServer;
	struct sockaddr_in sockClient;

} server_t;


typedef struct {

	int socket;
	
	uint ip;
	ushort port;
	uchar status;		/* 结合socket，确保socket的正常使用 0:无使用  1:需要关闭   0xff:正常使用 */

	int len;
	char recv[SOCKET_TCP_MIN];
	
} tcp_accept_t;



typedef struct {

	int tcpSock;

	int tcpInit;	/* 0:无使用  1:需要关闭然后重新创建客户端  3.等待reconnectWaitTime时间到  5:需要连接服务器   ff:成功连接服务器，正常通讯 */

	int connectFailTimes;	/* 连接失败次数，达到预定次数后重新创建socket  */

	int reconnectCounts;		/* 计数，每秒+1 */
	int reconnectWaitSecond;	/* 等待多少秒重连 */
	time_t tcpCtrlTime;
	time_t reconnectWaitTime;	/* 原则上udp受tcp的状态控制，暂不做处理*/
	

	uint ip;
	ushort port;

	int len;
	char recv[SOCKET_TCP_MIN];
	
    	struct sockaddr_in tcpServer;
 
} client_t;







extern server_t udpServer;

/* 建立与SH的连接 */
extern tcp_accept_t tcpAccept;

extern client_t tcpClient;


unsigned char  string_tohex2(const char *str);

void nearSocketTask(unsigned int input_thread);
int udpServer_proce(char * rxbuf, int rxlen);



#endif

