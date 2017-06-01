#ifndef  __NVRAM_CONFIG_H__
#define  __NVRAM_CONFIG_H__


#include <qcom/qcom_common.h>
#include "wifi_config.h"



#define DEFAULT_INFORMATION_ADDR_D		0x0007E000
#define USER_INFORMATION_ADDR_D		0x0007F000

extern char* seraph;


void device_information_load(Device_Info_t *ptDeviceInfo);
void device_information_set(Device_Info_t *ptDeviceInfo);
void device_information_get(Device_Info_t *ptDeviceInfo);
void ResetSystem(void);
void generate_smartConnect_key(Device_Info_t *ptDeviceInfo);


#endif


	

