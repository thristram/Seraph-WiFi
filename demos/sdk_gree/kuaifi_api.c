#include "qcom_system.h"
#include "threadx/tx_api.h"
#include "qcom_cli.h"
#include "qcom_common.h"
#include "qcom/socket_api.h"
#include "qcom/select_api.h"
#include "qcom/qcom_time.h"
#include "tx_event_flags.h"
#include "qcom_nvram.h"  
#include "qcom_uart.h"
#include "qcom_mem.h"

#include "arwlib/arwlib.h"
#include "sdk_shell.h"
#include "wifi_config.h"

#include "AES.h"

//#include "arw_sm.h"  //lyh



int custom_flag_filter( unsigned char * buf);
void custom_encrypt(const unsigned char *in, unsigned len, unsigned char *out);
void custom_decrypt(const unsigned char *in, unsigned len, unsigned char *out);

extern int kf_set_kf_autoconnect(int enable);
extern int kf_set_kf_connect_times(int try);

/*api */
/*kuaifi mode:*/
/*smart config flag filter process function*/
/*return 0 , the date is inavalidate */
/*return 1 , the data is validate */
/*head->flag =0 custom_flag_filter not be process.*/

int custom_flag_filter( unsigned char * buf)
{
     int ret = 0;
     

     return ret;
}


/*api */
/*kuaifi mode:*/
/*smart config data encrypt process function*/
/*head->encrypt =0 custom_decrypt not be process.*/

void custom_encrypt(const unsigned char *in, unsigned len, unsigned char *out)
{
	//unsigned char mask = 0xc5;
	unsigned i;
	for(i = 0; i < len; i++){
		out[i] = (in[i] ^ 0xc5) + 0xc5;
	}
}

void custom_decrypt(const unsigned char *in, unsigned len, unsigned char *out)
{
	//unsigned char mask = 0xc5;
	unsigned i;
	for(i = 0; i < len; i++){
		out[i] = (in[i] - 0xc5) ^ 0xc5;
	}
}

/*set reconnect count */
/*default is 6 */
/*if the reconnect count lager than six, the kuaifi state machine will be quit ,*/
/*and quit the kuaifi mode.*/

void kuaifi_autoconnect_times_set(int count)
{
	kf_set_kf_connect_times(count);
}

/*set autoconnect mode */
/*Default is not connect ,kuaifi only parase the config info ,not to connect the ap */

void kuaifi_autoconnect_set(int enable)
{
	kf_set_kf_autoconnect(enable);
	
}




/*extern int arw_ArowP_process(char* data, int* inout_len);*/
/*extern int arw_tcp_fwd_write(char* p, int* l);*/
/*extern void arw_tcp_set_rxcb(arw_pkt_cb cb);*/
/*extern void arw_tcp_set_fwdcb(arw_pkt_cb cb);*/
/*extern int arw_kf_daemon_start(int p);*/
/*extern int arw_uart_comm_fwd_write(char* p, int* l);*/
void kf_cb(void *buf)
{
	uchar i, ssidlen = 0, keylen = 0;
	uchar zero[AES_BLOCK_SIZE] = {0};
	uchar temp[64];
	int ret;
	
	smart_config_head *head = (smart_config_head *)buf;
	smart_config_data *data = (smart_config_data *)(head + 1);

	i = 0;
	while(memcmp(data->ap_ssid+i*AES_BLOCK_SIZE, zero, AES_BLOCK_SIZE)){
		i++;
   		if(i == 2){ break;}		//32
	}
	ssidlen = i*AES_BLOCK_SIZE;
	A_PRINTF(" ----- ssid(%d):\n", ssidlen);
	for(i=0; i< ssidlen; i++){
		A_PRINTF(" %02x", data->ap_ssid[i]);
	}
	
	i = 0;
	while(memcmp(data->ap_passkey+i*AES_BLOCK_SIZE, zero, AES_BLOCK_SIZE)){
		i++;
		if(i == 4){ break;}		//64
	}
	keylen = i*AES_BLOCK_SIZE;
	A_PRINTF(" ----- psk(%d):\n", keylen);
	for(i=0; i< keylen; i++){
		A_PRINTF(" %02x", data->ap_passkey[i]);
	}

	if(keylen > 0 && ssidlen > 0){
		if(!memcmp(data->ap_passkey, kuaifiKey.idKey, AES_BLOCK_SIZE)){		/* id key ÏàÍ¬ */
			//½âÃÜ

			memset(temp, 0, 64);
			memset(kuaifiSave.ap_passkey, 0, 64);
			ret = 0;
			if(keylen > AES_BLOCK_SIZE){
				ret = aes_cbc_decrypt(kuaifiKey.msgKey, data->ap_passkey + AES_BLOCK_SIZE, keylen -AES_BLOCK_SIZE, temp);		
				memcpy(kuaifiSave.ap_passkey, temp, ret);
			}
			A_PRINTF("\n key:%s", kuaifiSave.ap_passkey);

			memset(temp, 0, 64);
			ret = aes_cbc_decrypt(kuaifiKey.msgKey, data->ap_ssid, ssidlen, temp);		
			memset(kuaifiSave.ap_ssid, 0, 32);
			memcpy(kuaifiSave.ap_ssid, temp, ret);
			A_PRINTF("\n ssid:%s", kuaifiSave.ap_ssid);

		}
	}	

		
}


//A_INT32 kuaifi(A_INT32 argc, A_CHAR *argv[])
void kuaifi(void)
{

	arw_kf_daemon_start(0);
	kf_set_callback(kf_cb);

}


A_INT32 kuaifi_mode(A_INT32 argc, A_CHAR *argv[])
{
	int mode;

	if (argc != 2)
		return -1;

	mode = atoi((char*)argv[1]);

	extern void kf_set_working_mode(int mode);
	kf_set_working_mode(mode);

	return 0;
}

A_INT32 kuaifi_set(A_CHAR * tmp_ssid, A_CHAR *tmp_psk)
{
	char ssid[33];
	char key[65];
	if (strlen(tmp_ssid) > 32) {
		printf("invalid ssid\n");
	}
	if (strlen(tmp_psk) > 64) {
		printf("invalid key\n");
	}
	memset(ssid, 0, sizeof(ssid));
	memset(key, 0, sizeof(key));
	strcpy(ssid, tmp_ssid);
	strcpy(key, tmp_psk);
	extern void kf_set_tmpcfg(char *ssid, char* key);
	kf_set_tmpcfg(ssid, key);
	return 0;
}

A_INT32 kuaifi_stop(A_INT32 argc, A_CHAR *argv[])
{
	extern int arw_kf_daemon_stop();
	arw_kf_daemon_stop();
	return 0;
}


