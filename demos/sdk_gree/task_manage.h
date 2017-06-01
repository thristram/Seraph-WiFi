#ifndef __TASK_MANAGE_H__
#define __TASK_MANAGE_H__

#include "threadx/tx_api.h"
#include "threadx/tx_thread.h"
#include "qcom/qcom_common.h"

//#define DEBUG_TASK
#define MAX_TASK_NUM 20

#define THR_TASK_STS_NULL     0
#define THR_TASK_STS_CREATED  1
#define THR_TASK_STS_EXITED   2

void 	qcom_task_wakeup();
void 	qcom_task_kill_all(void);//É¾³ý
void 	qcom_task_terminate_all(void);//Í£Ö¹
void 	mark_exit_all_task(void);
void 	qcom_task_yield();
void 	qcom_task_del(void);
int 	free_task_mem();
int 	mark_exit_task(TX_THREAD * thread_ptr);
int 	get_empty_task_index(void);
int 	task_init(void);
void	qcom_task_exit();//É¾³ý
int		qcom_task_start(TX_THREAD *pthr, void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
#endif

