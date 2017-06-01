
#include <qcom/qcom_common.h>
#include "qcom_nvram.h"

#include "sdk_shell.h"
#include "task_manage.h"
#include "wifi_config.h"
#include "socket_process.h"
#include "nvram_config.h"

				
char* seraph =  "seraph";	


/*   D E V I C E _ I N F O R M A T I O N _ L O A D   */
/*-------------------------------------------------------------------------
    ��ȡ�洢��·����Ϣ��mac
-------------------------------------------------------------------------*/
void device_information_load(Device_Info_t *ptDeviceInfo)
{
	A_STATUS status;
	A_UCHAR uMAC[6];
	
	status = qcom_nvram_read(USER_INFORMATION_ADDR_D, (A_UCHAR *)ptDeviceInfo, sizeof(Device_Info_t));
	A_PRINTF("\n Read user info:%d.", status);

	if (memcmp(ptDeviceInfo->uVerify, seraph, 6) == 0) {		
		A_PRINTF("\n Get device info OK...");
		
		qcom_mac_get(uMAC);
		A_PRINTF("\n MAC = %02x-%02x-%02x-%02x-%02x-%02x.", uMAC[0], uMAC[1], uMAC[2], uMAC[3], uMAC[4], uMAC[5]);
		memcpy(ptDeviceInfo->uMAC, uMAC, 6);
		
		A_PRINTF("\n ssid1(%s), key(%s).", ptDeviceInfo->tWlanInfo.ssid1, ptDeviceInfo->tWlanInfo.key1);
		A_PRINTF("\n ssid2(%s), key(%s).", ptDeviceInfo->tWlanInfo.ssid2, ptDeviceInfo->tWlanInfo.key2);
		
	} else {		
		A_PRINTF("\n Get device info fail...");
		/* �������Ҫһһ���㣬����ʹ��memsetֱ���������ṹ�壬�ƺ���bug */
		memcpy(ptDeviceInfo->uVerify, seraph, 6);	/* ��һ��ִ�б�־ */
		ptDeviceInfo->ip = 0;
		ptDeviceInfo->tWlanInfo.uMode = 0;
		ptDeviceInfo->tWlanInfo.uSwitchFlag = 1;
		memset(ptDeviceInfo->tWlanInfo.ssid1, 0, SSID_LEN);
		memset(ptDeviceInfo->tWlanInfo.ssid2, 0, SSID_LEN);
		memset(ptDeviceInfo->tWlanInfo.key1, 0, KEY_LEN);
		memset(ptDeviceInfo->tWlanInfo.key2, 0, KEY_LEN);
		
//		strcpy(ptDeviceInfo->tWlanInfo.ssid1, "360");
//		strcpy(ptDeviceInfo->tWlanInfo.key1, "12345678");

		device_information_set(ptDeviceInfo);
		
		qcom_mac_get(uMAC);
		A_PRINTF("\n MAC = %02x-%02x-%02x-%02x-%02x-%02x.", uMAC[0], uMAC[1], uMAC[2], uMAC[3], uMAC[4], uMAC[5]);
		memcpy(ptDeviceInfo->uMAC, uMAC, 6);
	
	}
	
}



/*   D E V I C E _ I N F O R M A T I O N _ S E T   */
/*-------------------------------------------------------------------------
    �洢��Ϣ
-------------------------------------------------------------------------*/
void device_information_set(Device_Info_t *ptDeviceInfo)
{
	A_STATUS status;
	status = qcom_nvram_erase(USER_INFORMATION_ADDR_D, 0x1000);	/* ����4096���ֽ� */

	A_PRINTF("\n Erase user flash(%d)bytes.", status);

	status = qcom_nvram_write(USER_INFORMATION_ADDR_D, (A_UCHAR *)ptDeviceInfo, sizeof(Device_Info_t));

	A_PRINTF("\n Write to user flash(%d)bytes.", status);

}

/*   D E V I C E _ I N F O R M A T I O N _ G E T   */
/*-------------------------------------------------------------------------
    ��ȡ�洢��Ϣ
-------------------------------------------------------------------------*/
void device_information_get(Device_Info_t *ptDeviceInfo)
{
	
    	A_STATUS status = qcom_nvram_read(USER_INFORMATION_ADDR_D, (A_UCHAR *)ptDeviceInfo, sizeof(Device_Info_t));

    	A_PRINTF("\n Read user flash:%d.", status);
	
}


/*   R E S E T   S Y S T E M   */
/*-------------------------------------------------------------------------
    ��������
-------------------------------------------------------------------------*/
void ResetSystem(void)
{
	device_information_set(&tDeviceInfo);
	A_PRINTF("\n--- system restart ---\n");
	qcom_sys_reset(); 

}






