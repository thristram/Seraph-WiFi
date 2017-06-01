
#ifndef __SOCKET_MANAGE_H__
#define __SOCKET_MANAGE_H__

#include <qcom/basetypes.h>
#include <qcom/base.h>
#include <qcom/socket.h>


#define  UDP_LISTEN_PORT  	1888
//#define  UDP_LISTEN_PORT  	838
#define  TCP_LISTEN_PORT  		1838


#define  SOCKET_TCP_MAX  		2048		/* < 1500-20-20=1460 ����ͨѶ����Ҫ���ܣ���Ч���ݳ���ʹ�����ֵ*/
#define  SOCKET_TCP_MIN   		1024	

#define  SOCKET_UDP_DEF   		1024
//#define  SOCKET_UDP_MIN   		512  		/* < 576-8-20=548 ������Զ�� */
#define  SOCKET_UDP_MIN   		256  		/* < 576-8-20=548 ������Զ�� */


#define  TCP_CLIENT_CONNECT_TIME   		3000  


typedef struct {
	
	int socket;
	ushort port;	/* �������˿� */
	uchar initflag;	/* 0:��ʹ��,Ҫȫ������  1:��Ҫ�ر�  2:��Ҫ���´���    ff:��ʼ����� */
	
	uchar ctrlflag;	/* 0.û�в��������̽���  
				   1.���յ��㲥����ʼ���� 
				   2.��һ�ν��յ�ip����յ���ͬip��ȴ��������
				   3.���յ�ip��һ�� 
				   4.���յ�ip��һ�º����½������� 
				   0xff.udp���ͻظ� 
				   */
				   
	uchar rxReply;		/* 0.�յ�ָ������ӵ��µ�TCP Server��������ȷ
					   1.�յ�ָ��˳���ǰTCP Server ���������ӵ��µ�TCP Server
					   2.�յ�ָ�������Ŀ��IP ��ַ��Ŀǰ���ӵ�IP ��ַһ����������
					   3.�޷����ӵ�TCP Server
					   */
	struct sockaddr_in sockServer;
	struct sockaddr_in sockClient;

} server_t;


typedef struct {

	int socket;
	
	uint ip;
	ushort port;
	uchar status;		/* ���socket��ȷ��socket������ʹ�� 0:��ʹ��  1:��Ҫ�ر�   0xff:����ʹ�� */

	int len;
	char recv[SOCKET_TCP_MIN];
	
} tcp_accept_t;



typedef struct {

	int tcpSock;

	int tcpInit;	/* 0:��ʹ��  1:��Ҫ�ر�Ȼ�����´����ͻ���  3.�ȴ�reconnectWaitTimeʱ�䵽  5:��Ҫ���ӷ�����   ff:�ɹ����ӷ�����������ͨѶ */

	int connectFailTimes;	/* ����ʧ�ܴ������ﵽԤ�����������´���socket  */

	int reconnectCounts;		/* ������ÿ��+1 */
	int reconnectWaitSecond;	/* �ȴ����������� */
	time_t tcpCtrlTime;
	time_t reconnectWaitTime;	/* ԭ����udp��tcp��״̬���ƣ��ݲ�������*/
	

	uint ip;
	ushort port;

	int len;
	char recv[SOCKET_TCP_MIN];
	
    	struct sockaddr_in tcpServer;
 
} client_t;







extern server_t udpServer;

/* ������SH������ */
extern tcp_accept_t tcpAccept;

extern client_t tcpClient;


unsigned char  string_tohex2(const char *str);

void nearSocketTask(unsigned int input_thread);
int udpServer_proce(char * rxbuf, int rxlen);



#endif

