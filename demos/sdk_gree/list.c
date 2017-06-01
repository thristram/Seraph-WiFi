/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/

#include <qcom/qcom_common.h>
#include "qcom_uart.h"
#include <qcom/qcom_gpio.h>
#include <qcom/qcom_wdt.h>
#include <qcom/qcom_nvram.h>

#include "task_manage.h"
#include "wifi_config.h"
#include "nvram_config.h"
#include "sdk_shell.h"
#include "socket_process.h"
#include "uart_process.h"
#include "list.h"



/* uart���͵������SingleList�ı�ͷ */
slnode_t *uartTxSLHead = NULL;

/* uart���͵������SingleList�ı�β */
slnode_t *uartTxSLLast = NULL;


/* uart���յ������SingleList�ı�ͷ */
slnode_t *uartRxSLHead = NULL;

/* uart���յ������SingleList�ı�β */
slnode_t *uartRxSLLast = NULL;


TX_EVENT_FLAGS_GROUP uartTxSLEventGroup;
ulong uartTxSLEventFlag = 0x01;
ulong uartTxSLActualFlag;


TX_EVENT_FLAGS_GROUP uartRxSLEventGroup;
ulong uartRxSLEventFlag = 0x02;
ulong uartRxSLActualFlag;


/*
	tx_event_flags_create(&uartTxSLEventGroup, "uart tx event group");	
	tx_event_flags_set(&uartTxSLEventGroup, uartTxSLEventFlag, TX_OR);
	tx_event_flags_get(&uartTxSLEventGroup, uartTxSLEventFlag, TX_AND_CLEAR, &uartTxSLActualFlag, 100*1000);

	
	tx_event_flags_create(&uartRxSLEventGroup, "uart rx event group");
	tx_event_flags_set(&uartRxSLEventGroup, uartRxSLEventFlag, TX_OR);
	tx_event_flags_get(&uartRxSLEventGroup, uartRxSLEventFlag, TX_AND_CLEAR, &uartRxSLActualFlag, 100*1000);


	TX_WAIT_FOREVER	
	TX_NO_WAIT


*/



/*   A D D   N O D E   T O   U A R T   T X   S   L   L A S T   */
/*-------------------------------------------------------------------------
    �˺���������can1TxSLHead UartTxSLLast ֱ�Ӱ�
    ������Ϊͨ�õĺ���
    �������ɳ��룬��˻�������ͬʱ��ס����ͺ���    
-------------------------------------------------------------------------*/
int addNodeToUartTxSLLast(char *psave, int length)
{
	int ret = -1;
	slnode_t * newNode;
	char *pdata;

	newNode = (slnode_t *)mem_alloc(sizeof(slnode_t)); if(!newNode){ return -1;}
	/* BB BB */
	pdata = (char *)mem_alloc(length+4); if(!pdata){mem_free(newNode); return -1;}
	if(newNode && pdata){
		newNode->next = NULL;
		newNode->len = (uint)length + 2;	/* ֡ͷ BB BB */
		newNode->hasWrite = 0;
		*pdata = 0xbb;
		*(pdata + 1)= 0xbb;
		memcpy(pdata + 2, psave, length);
		newNode->data = pdata;

		if(!uartTxSLLast){	/* ����β�Ƿ�Ϊ��? */
			uartTxSLHead = newNode;
			uartTxSLLast = newNode;
		}else{
			uartTxSLLast->next = newNode;	/* add node */
			uartTxSLLast = newNode;	/* new list end */		
		}
		ret = 0;
	}	
	return ret;

}


/*   A D D   N O D E   T O   C A N 0   T X   S   L   L A S T   */
/*-------------------------------------------------------------------------
    �˺���������can1TxSLHead can1TxSLLast ֱ�Ӱ�
    ������Ϊͨ�õĺ���
    �������ɳ��룬��˻�������ͬʱ��ס����ͺ���      
-------------------------------------------------------------------------*/
int addNodeToUartRxSLLast(char *psave, int length)
{
	int ret = -1;
	slnode_t * newNode;
//	char *pdata;

//	newNode = (slnode_t *)mem_alloc(sizeof(slnode_t)); if(!newNode){ return -1;}
//	pdata = (char *)mem_alloc(length+1); if(!pdata){mem_free(newNode); return -1;}
//	if(newNode && pdata){
//		newNode->next = NULL;
//		newNode->len = (uint)length;
//		newNode->hasWrite = 0;
//		memcpy(pdata, psave, length);
//		newNode->data = pdata;

//		if(!uartRxSLLast){	/* ����β�Ƿ�Ϊ��? */
//			uartRxSLHead = newNode;
//			uartRxSLLast = newNode;
//		}else{
//			uartRxSLLast->next = newNode;	/* add node */
//			uartRxSLLast = newNode;	/* new list end */		
//		}
//		ret = 0;
//	}	



newNode = (slnode_t *)mem_alloc(sizeof(slnode_t)); if(!newNode){ return -1;}
if(newNode){
	newNode->next = NULL;
	newNode->len = (uint)length;
	newNode->hasWrite = 0;
	newNode->data = psave;

	if(!uartRxSLLast){	/* ����β�Ƿ�Ϊ��? */
		uartRxSLHead = newNode;
		uartRxSLLast = newNode;
	}else{
		uartRxSLLast->next = newNode;	/* add node */
		uartRxSLLast = newNode; /* new list end */		
	}
	ret = 0;
}	




return ret;
	
}


/*   D E L E T E   N O D E   F R O M   C A N 1   T X   S   L   H E A D   */
/*-------------------------------------------------------------------------
        ������ͷɾ���ڵ�
-------------------------------------------------------------------------*/
int deleteNodeFromUartTxSLHead(void)
{
	int ret = -1;
	slnode_t *newhead;

	if(uartTxSLHead){	/* ����ͷ��Ϊ�� */	
		if(!uartTxSLHead->next){	/* ����ͷ��nextΪ�գ���ֻ��һ���ڵ㣬����ͷ������β��ָ��ýڵ� */
			if(uartTxSLHead->data) {mem_free(uartTxSLHead->data);}
			mem_free(uartTxSLHead);
			uartTxSLHead = NULL;
			uartTxSLLast = NULL;
		}else{	/* ����ͷ��next��Ϊ�գ������������������ϵĽڵ� */
			newhead = uartTxSLHead->next;
			if(uartTxSLHead->data) {mem_free(uartTxSLHead->data);}
			mem_free(uartTxSLHead);
			uartTxSLHead = newhead;			
		}	
		ret = 0;
	}
	return ret;	
}

/*   D E L E T E   N O D E   F R O M   C A N 0   T X   S   L   H E A D   */
/*-------------------------------------------------------------------------
   ������ͷɾ���ڵ�
-------------------------------------------------------------------------*/
int deleteNodeFromUartRxSLHead(void)
{
	int ret = -1;
	slnode_t *newhead;

//	if(uartRxSLHead){	/* ����ͷ��Ϊ�� */	
//		if(!uartRxSLHead->next){	/* ����ͷ��nextΪ�գ���ֻ��һ���ڵ㣬����ͷ������β��ָ��ýڵ� */
//			if(uartRxSLHead->data) {mem_free(uartRxSLHead->data);}
//			mem_free(uartRxSLHead);
//			uartRxSLHead = NULL;
//			uartRxSLLast = NULL;
//		}else{	/* ����ͷ��next��Ϊ�գ������������������ϵĽڵ� */
//			newhead = uartRxSLHead->next;
//			if(uartRxSLHead->data) {mem_free(uartRxSLHead->data);}
//			mem_free(uartRxSLHead);
//			uartRxSLHead = newhead;			
//		}	
//		ret = 0;
//	}

	if(uartRxSLHead){	/* ����ͷ��Ϊ�� */	
		if(!uartRxSLHead->next){	/* ����ͷ��nextΪ�գ���ֻ��һ���ڵ㣬����ͷ������β��ָ��ýڵ� */
			mem_free(uartRxSLHead);
			uartRxSLHead = NULL;
			uartRxSLLast = NULL;
		}else{	/* ����ͷ��next��Ϊ�գ������������������ϵĽڵ� */
			newhead = uartRxSLHead->next;
			mem_free(uartRxSLHead);
			uartRxSLHead = newhead;			
		}	
		ret = 0;
	}


	
	return ret;	
}



/*   C L E A R   C A N 1   T X   S   L   */
/*-------------------------------------------------------------------------
       ʹ�ù���������ִ������������ 
-------------------------------------------------------------------------*/
void clearUartTxSL(void)
{
	slnode_t *pNext;
	
	while(uartTxSLHead){		
		pNext = uartTxSLHead->next;
		if(uartTxSLHead->data){mem_free(uartTxSLHead->data);} 
		mem_free(uartTxSLHead);
		uartTxSLHead = pNext;
	}
	uartTxSLLast = NULL;
}



/*   C L E A R   C A N 0   T X   S   L   */
/*-------------------------------------------------------------------------
    ʹ�ù���������ִ������������ 
-------------------------------------------------------------------------*/
void clearUartRxSL(void)
{
	slnode_t *pNext;
	
	while(uartRxSLHead){
		pNext = uartRxSLHead->next;
		if(uartRxSLHead->data) {mem_free(uartRxSLHead->data);}
		mem_free(uartRxSLHead);
		uartRxSLHead = pNext;
	}
	uartRxSLLast = NULL;
}









