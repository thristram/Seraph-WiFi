/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
#ifndef  __LIST_H__
#define  __LIST_H__



/* �����б� */
typedef struct slnode_t{

	struct slnode_t *next;

	uint len;		/* data�����ݳ��� */
	uint hasWrite;	/* data�Ѿ����͵����ݳ��� */
	char *data;

} slnode_t;



/* uart���͵������SingleList�ı�ͷ */
extern slnode_t *uartTxSLHead;

/* uart���͵������SingleList�ı�β */
extern slnode_t *uartTxSLLast;


/* uart���յ������SingleList�ı�ͷ */
extern slnode_t *uartRxSLHead;

/* uart���յ������SingleList�ı�β */
extern slnode_t *uartRxSLLast;


extern TX_EVENT_FLAGS_GROUP uartTxSLEventGroup;
extern ulong uartTxSLEventFlag;
extern ulong uartTxSLActualFlag;

extern TX_EVENT_FLAGS_GROUP uartRxSLEventGroup;
extern ulong uartRxSLEventFlag;
extern ulong uartRxSLActualFlag;


int addNodeToUartTxSLLast(char *psave, int length);
int addNodeToUartRxSLLast(char *psave, int length);
int deleteNodeFromUartTxSLHead(void);
int deleteNodeFromUartRxSLHead(void);
void clearUartTxSL(void);
void clearUartRxSL(void);



#endif

