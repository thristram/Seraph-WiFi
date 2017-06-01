
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/socket.h>
#include <qcom/qcom_common.h>
#include <qcom/qcom_gpio.h>

#include "qcom_internal.h"
#include "qcom_nvram.h"
#include "arwlib/arwlib.h"

#include "task_manage.h"
#include "sdk_shell.h"
#include "cjson.h"
#include "socket_process.h"
#include "nvram_config.h"
#include "wifi_config.h"
#include "list.h"


/* 保存从存储获取的信息 */
Device_Info_t tDeviceInfo;

/* 保存运行时的网络状态信息 */
Network_Info_t tNetworkInfo;

ip_ping_t IPPing;




/*   W I F I _   C O N F I G   */
/*-------------------------------------------------------------------------
	qcom_disconnect();	//断开连接

 
-------------------------------------------------------------------------*/
void wifi_connect(A_CHAR *ssid, A_CHAR *passwd)
{
	A_STATUS status;
		
	tNetworkInfo.ipObtained = 0;	
	tNetworkInfo.getIPtimes = 5;
	status = STA_mode_set(ssid, passwd);
	if (status == A_OK) {
		A_PRINTF(" waiting...");	
		qcom_thread_msleep(12000);						

		ip_obtained_confirm();

	} else {							
		A_PRINTF(" fail!");
	}
	
}



/*   W I F I _   C O N F I G   */
/*-------------------------------------------------------------------------
	qcom_disconnect();	//断开连接

-------------------------------------------------------------------------*/
void wifi_process(void)
{
	static int connectCtrl = 1;
	uchar ssid_temp[SSID_LEN] = {0};
	uchar key_temp[KEY_LEN] = {0};
	
	tNetworkInfo.devMode = 0;	/* 默认sta模式，加路由 */

	if(connectCtrl == 1){	/* 连接ssid1的路由 */
		if(tDeviceInfo.tWlanInfo.ssid1[0]){ 
			A_PRINTF("\n Connect to ssid1:%s", tDeviceInfo.tWlanInfo.ssid1);
			wifi_connect(tDeviceInfo.tWlanInfo.ssid1, tDeviceInfo.tWlanInfo.key1);
			
			if(tNetworkInfo.ipObtained == 1){	/* 成功获取ip */
				connectCtrl = 0;	/* 连接成功，退出连接 */
				LEDShow.ledState = 2;
				LEDShow.ledTimeStart = tx_time_get();
			}else{
				connectCtrl = 2;
			}				
			
		}else{
			A_PRINTF("\n ssid1 isn't exsit!");
			connectCtrl = 2;
		}
	}

	if(connectCtrl == 2){	/* 连接ssid2的路由 */
		if(tDeviceInfo.tWlanInfo.ssid2[0]){ 
			A_PRINTF("\n Connect to ssid2:%s", tDeviceInfo.tWlanInfo.ssid2);
			wifi_connect(tDeviceInfo.tWlanInfo.ssid2, tDeviceInfo.tWlanInfo.key2);
			
			if(tNetworkInfo.ipObtained == 1){	/* 成功获取ip */
				connectCtrl = 0;	/* 连接成功，退出连接 */
				LEDShow.ledState = 2;
				LEDShow.ledTimeStart = tx_time_get();
				/* 将ssid2 保存至ssid1的位置 */
				memcpy(ssid_temp, tDeviceInfo.tWlanInfo.ssid1, SSID_LEN);
				memcpy(key_temp, tDeviceInfo.tWlanInfo.key1, KEY_LEN);
				memcpy(tDeviceInfo.tWlanInfo.ssid1, tDeviceInfo.tWlanInfo.ssid2, SSID_LEN);
				memcpy(tDeviceInfo.tWlanInfo.key1, tDeviceInfo.tWlanInfo.key2, KEY_LEN);
				memcpy(tDeviceInfo.tWlanInfo.ssid2, ssid_temp, SSID_LEN);
				memcpy(tDeviceInfo.tWlanInfo.key2, key_temp, KEY_LEN);
				device_information_set(&tDeviceInfo);
				
			}else{
				connectCtrl = 3;
			}				
			
		}else{
			A_PRINTF("\n ssid2 isn't exsit!");
			connectCtrl = 3;
		}

	}

	if(connectCtrl == 3){	/* 快联 */
		LEDShow.ledState = 3;
		LEDShow.ledTimeStart = tx_time_get();		
		memset(kuaifiSave.ap_ssid, 0, SSID_LEN);
		memset(kuaifiSave.ap_passkey, 0, KEY_LEN);	
		show_time();
		A_PRINTF("\n kuaifiSave.ap_ssid(%s), kuaifiSave.ap_passkey(%s).", kuaifiSave.ap_ssid, kuaifiSave.ap_passkey);		
		A_PRINTF("\n ------- kuaifi starting -------");
		extern void kuaifi(void);
		kuaifi();
		
	}
	
}


/*   K U A I F I _ C O N F I R M   */
/*-------------------------------------------------------------------------
    控制快联时间 100s，1s执行一次
    
-------------------------------------------------------------------------*/
void kuaifi_confirm(void)
{
	static int i = 0;
	
	if(i <= 1000){
		i++;
		if(kuaifiSave.ap_ssid[0]){		/* 抓包成功 */				
			if((strlen((char *)kuaifiSave.ap_ssid) < 32) && (strlen((char *)kuaifiSave.ap_passkey) < 64)) {	/* 长度判断 */

				qcom_gpio_set_pin_high(led_gpio);	
				LEDShow.ledState = 0xff;					
				A_PRINTF("\n Connect to new ssid:%s", kuaifiSave.ap_ssid);
				wifi_connect((char *)kuaifiSave.ap_ssid, (char *)kuaifiSave.ap_passkey);
				
				if(tNetworkInfo.ipObtained == 1){	/* 成功获取ip */
					LEDShow.ledState = 2;
					LEDShow.ledTimeStart = tx_time_get();
					
					/* 保存快联成功的路由信息 */
					memcpy(tDeviceInfo.tWlanInfo.ssid2, tDeviceInfo.tWlanInfo.ssid1, SSID_LEN);
					memcpy(tDeviceInfo.tWlanInfo.key2, tDeviceInfo.tWlanInfo.key1, KEY_LEN);
					memcpy(tDeviceInfo.tWlanInfo.ssid1, kuaifiSave.ap_ssid, SSID_LEN);
					memcpy(tDeviceInfo.tWlanInfo.key1, kuaifiSave.ap_passkey, KEY_LEN);
					device_information_set(&tDeviceInfo);

				}else{
					A_PRINTF("\n ------->> cannot get ip!");
				}
			}
			else{
				A_PRINTF("\n ------->> invalid ssid or key len!");
			}
			
			/* 抓包成功后连接失败，则重新开始抓吧 */
			if(LEDShow.ledState != 2){
				qcom_sys_reset();	/* 重新启动*/			
			}
					
		}
		else{
			qcom_thread_msleep(100);
		}
		
	}else{
		qcom_sys_reset();	/* 快联时间到，重新启动 */
	}

}




/*   I   P _   G E T   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
A_STATUS ip_get(void)
{
	A_STATUS status;

	//判断网络是否稳定，即是否已获取IP	
	if(tNetworkInfo.ipObtained == 0){
		status = qcom_sta_get_rssi(&tNetworkInfo.linkQuality);
		
		/* 网络状态指数:*/
		A_PRINTF("linkQuality:%d , status:%d.", tNetworkInfo.linkQuality, status);	
					
		if ((tNetworkInfo.linkQuality > 0) && (tNetworkInfo.linkQuality < 100)) {
			status = network_info_get(); 
			if(tNetworkInfo.ipAddress > 0){
				tNetworkInfo.ipObtained = 1;
				A_PRINTF("IP obtained.");					
			}				
		} else {
			if(tNetworkInfo.linkQuality == 0){	/* 返回值只是确定加网时网络是否存在或密码错误 */
				A_PRINTF("password error!");
			}
			if(tNetworkInfo.linkQuality == 128){	/* 返回值只是确定加网时网络是否存在或密码错误 */
				A_PRINTF("network isn't exsit!");
			}
			return A_ERROR;
		}								
				
	}
	return A_OK;

}


/*   I   P _   O B T A I N E D _   C O N F I R M   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
void ip_obtained_confirm(void)
{
	//判断网络是否稳定，即是否已获取IP	
	while(tNetworkInfo.getIPtimes){
		A_PRINTF("\n GetIPtimes(%d).", 6-tNetworkInfo.getIPtimes);

		if(ip_get() == A_ERROR){	/* 网络错误或密码错误 */
			tNetworkInfo.getIPtimes = 0;
			return;
		}else{		/* 网络稳定 */
			if(tNetworkInfo.ipObtained == 1){														
				return;			
			}else{
				qcom_thread_msleep(2000);
			}
		}
		tNetworkInfo.getIPtimes--;		
	}

}


/*   S   T   A _   M O D E _   C O N N E C T   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
A_STATUS STA_mode_set(A_CHAR *ssid, A_CHAR *passwd)
{
	A_STATUS ret = A_ERROR;

	ret = qcom_disconnect();
	if ( ret != A_OK ) {
		A_PRINTF("\n STA disconnect fail!");
		return ret;
	}

	ret = qcom_op_set_mode(QCOM_WLAN_DEV_MODE_STATION);
	if ( ret != A_OK ) {
		A_PRINTF("\n STA set mode fail!");
		return ret;
	}
	A_PRINTF("\n WIFI set STA mode...");

    	ret = qcom_sec_set_passphrase(passwd);
    	if ( ret != A_OK ) {
    		A_PRINTF("\n STA set passphrase fail!");
    		return ret;
    	}

	ret = qcom_sta_connect_with_scan(ssid);
	if ( ret != A_OK ) {
		A_PRINTF("\n STA connect fail!");
		return ret;
	}
	A_PRINTF("\n STA connect result:%d",ret);
	qcom_dhcpc_enable(TRUE);
	qcom_dnsc_enable(TRUE);
	qcom_set_connect_callback(STA_mode_callback);
	return ret;
	
}


/*   S   T   A _   M O D E _   C A L L B A C K   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
void STA_mode_callback(void)
{
	A_UINT8 uConnectState = 0;

	qcom_get_state(&uConnectState);
	A_PRINTF("\n STA connect(%d).", uConnectState);

	if(uConnectState == 4){

		
	}else{	//连接异常
		qcom_sys_reset();	/* 网络连接断开，重新启动连接网络 */
	}

}

/*   N E T W O R K _   I N F O _   G E T   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
A_STATUS network_info_get(void)
{
	A_STATUS ret;

	tNetworkInfo.ipAddress = 0;
	tNetworkInfo.submask= 0;
	tNetworkInfo.gateway= 0;
	
	ret = qcom_ip_address_get(&tNetworkInfo.ipAddress, &tNetworkInfo.submask, &tNetworkInfo.gateway);

	printf_ip_format(tNetworkInfo.ipAddress,tNetworkInfo.submask,tNetworkInfo.gateway,0);	

	return ret;
}



/*   I P _ P I N G _ C O N F I G   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
A_STATUS ip_ping_config(int IP, ULONG timeout)
{
	A_STATUS ret = A_ERROR;

	IPPing.timePrev = tx_time_get();
//	A_PRINTF("\n Prev %d!",IPPing.timePrev);

	qcom_ip_ping(IP, 32,1,0);
	
	IPPing.timeNow = tx_time_get();
//	A_PRINTF("\n Now %d!",IPPing.timeNow);

	if(IPPing.timeNow >= IPPing.timePrev){
		IPPing.time = IPPing.timeNow - IPPing.timePrev;			
	}else{
		IPPing.time = 0xffffffff + IPPing.timeNow - IPPing.timePrev;
	}
	if(IPPing.time < timeout){			
		ret = A_OK;
	}

	return ret;

}

/*   I P _ P I N G _ P R O C E S S   */
/*-------------------------------------------------------------------------
	//处理近程突然断开WiFi时的情况
	//ping不成功表示连接断开
-------------------------------------------------------------------------*/
void ip_ping_process(void)
{

	if(tcpClient.tcpSock > 0 && tcpClient.tcpInit == 0xff){					
		if(ip_ping_config(tcpClient.ip, 2500) == A_ERROR){
			
			A_PRINTF("\n tcpClient ip ping fail!");
			tcpClient.tcpInit = 1;

		}			
	}


}


