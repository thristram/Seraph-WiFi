#ifndef __ARWLIB_H__
#define __ARWLIB_H__

#include "qcom/base.h"

typedef enum {
	ARW_BDR_300 = 300,
	ARW_BDR_1200 = 1200,
	ARW_BDR_2400 = 2400,
	ARW_BDR_4800 = 4800,
	ARW_BDR_9600 = 9600,
	ARW_BDR_19200 = 19200,
	ARW_BDR_38400 = 38400,
	ARW_BDR_57600 = 57600,
	ARW_BDR_115200 = 115200,
	ARW_BDR_230400 = 230400,
	ARW_BDR_460800 = 460800,
	ARW_BDR_921600 = 921600,
	ARW_BDR_INV = -1
} ARW_BDR;




typedef enum {
	SOCKET_INIT_OK = 100,
	SOCKET_BIND_OK ,
	SOCKET_LISTEN_OK ,
	SOCKET_ACCEPT_OK,
	SOCKET_CONNECT_OK,
	SOCKET_CLOSE_OK,
	SOCKET_INIT_FAILE=110,
	SOCKET_BIND_FAILE,
	SOCKET_CLOSE_FAILE,
	SOCKET_LISTEN_FAILE,
	SOCKET_CONNECT_FAILE,
	SOCKET_INV = -1
} NET_WORK_STATE;


//echo display function
void at_help_extern(void);

//sdk lib version
int at_version();

NET_WORK_STATE network_state_get();


void arw_ATmode_set(int flags);
int arw_ATmode_get(void);

typedef int (*arw_pkt_cb)(char* p, int* len);

/**
 * This function is used to init arwlib core.
 * @return 	0 on success. else on error.
 */
A_STATUS arwlib_init(void);

/**
 * This function is used to init arwlib core.
 * @return 	0 on success. else on error.
 */
A_STATUS arwlib_deinit(void);

void arwlib_comm_port_set(int id);
int arwlib_comm_port_get(void);

void arwlib_comm_baudrate_set(int bdr);
int arwlib_comm_baudrate_get(void);


/* TCP API */
void arwlib_tcp_configure_get(int* ip, int* port);
void arwlib_tcp_configure_set(int ip, int port);

void arwlib_tcp_port_set(int port);
int arwlib_tcp_port_get(void);

void arwlib_tcp_addr_set(int ip);
int arwlib_tcp_addr_get(void);

A_STATUS arwlib_tcp_deinit(void);



int arw_net_atfilter(char* p, int* l);

void arw_tcp_set_rxcb(arw_pkt_cb cb);
void arw_tcp_set_fwdcb(arw_pkt_cb cb);
void arw_tcp_set_atcb(arw_pkt_cb cb);

int arw_net_atfilter(char* p, int* l);



/* UDP SERVER and CLENT */
void arwlib_udp_configure_get(int* ip, int* port);
void arwlib_udp_configure_set(int ip, int port);
void arwlib_udp_port_set(int port);
int arwlib_udp_port_get(void);
void arwlib_udp_addr_set(int ip);
int arwlib_udp_addr_get(void);
A_STATUS arwlib_udp_deinit(void);
void arw_udp_set_rxcb(arw_pkt_cb cb);
void arw_udp_set_fwdcb(arw_pkt_cb cb);
int arw_udp_daemon_start(void);
void arw_udp_daemon_stop(void);
/**
 * This function is used to start smartconfig (sniffer mode).
 * and
 * @return 	0 on success.
 */
typedef struct _smart_config_head{
	unsigned char version;
	unsigned char encrypt;
	unsigned char flags;
	unsigned char len;
}smart_config_head;

typedef struct _smart_config_data{
	unsigned char	ap_ssid[32];	//ssid
	unsigned char	ap_passkey[64];	//psk
	unsigned char	ap_index_key;
	unsigned char	ap_mode;
	unsigned char	ap_enath_mode;
	unsigned char	ap_crpty_type;
	A_UINT32 ip;
	A_UINT32 mask;
	A_UINT32 gw;
	A_UINT32 dns;
}smart_config_data;

typedef enum {
	sm_mode_sniffer_enhance = 0,
	sm_mode_sniffer,
	sm_mode_ap,
	sm_mode_sta,
	sm_mode_protocol,
	sm_mode_invalid,
} kf_sm_mode;

//nv 
typedef struct {
	char	ssid[32];	//ssid
	char	key[64];	//psk
	A_INT32   ssid_len;
	A_INT32   key_len;
	unsigned char	ap_mode;        //AP or Station
	A_UINT32 	server_ip;		//server ip and port	
	A_UINT32	server_port;
//	unsigned char   server_name[253];//server name 
	A_UINT32 ip;
	A_UINT32 mask;
	A_UINT32 gw;
	A_UINT32 dns;
	A_UINT32 baudrate;	
}kf_cfg;


typedef struct {
	int magic;
	int mode;
	kf_cfg cfg;
}kfnvram;

enum {
	STATE_INIT = 0,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTED,
	STATE_CONFIGMODE,
	STATE_FACTORY,
	STATE_INVALID,
};


//led_state callback
int get_led_state();
int set_led_state(int state);

//int kf_read_nvcfg(char* ssid, char* key);
void kfnv_write(kf_cfg *p);
int kfnv_read(kf_cfg *p);


void kf_set_tmpcfg(char *ssid, char* key);
void kf_set_working_mode(int mode);
int kf_get_working_mode(void);



typedef void (*kf_status_ssid_key_callback)(char *ssid, char* key);
typedef void (*kf_status_callback)(void *buf);

void kf_set_ssid_key_callback(kf_status_ssid_key_callback cb);
void kf_set_callback(kf_status_callback cb);

void kf_set_connect_callback( kf_status_callback cb);

int arw_kf_daemon_start(int p);

int arw_kf_daemon_stop(void);

void kf_set_callback(kf_status_callback cb);
#endif // __ARWLIB_H__
