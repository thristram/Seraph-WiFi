#include "task_manage.h"

extern unsigned int mem_heap_get_free_size(void);

#ifdef DEBUG_TASK
#define DBG_TSK(fmt...) printf("[DBG@%14s:%d]", __FUNCTION__, __LINE__); printf(fmt);
#else
#define DBG_TSK(fmt...)
#endif

TX_THREAD *thr[MAX_TASK_NUM];
char *thrStack[MAX_TASK_NUM];
char thrStatus[MAX_TASK_NUM];
char taskName[MAX_TASK_NUM * 2];

TX_MUTEX mutex_tsk;
#define TSK_SEM_INIT tx_mutex_create(&mutex_tsk, "mutex tsk", TX_NO_INHERIT);
#define TSK_SEM_LOCK {int status =  tx_mutex_get(&mutex_tsk, TX_WAIT_FOREVER); \
  if (status != TX_SUCCESS) {\
    DBG_TSK("lock the task mutex failed !!!\n");\
  }\
}

#define TSK_SEM_UNLOCK { int status =  tx_mutex_put(&mutex_tsk);\
  if (status != TX_SUCCESS) {\
    DBG_TSK("unlock the task mutex failed !!!\n");\
  }\
}

int task_init(void)
{
    static int inited = 0;
    int i;
    if (0 == inited) {
        TSK_SEM_INIT;
        for (i = 0; i < MAX_TASK_NUM; i++) {
            thr[i] = 0;
            thrStatus[i] = THR_TASK_STS_NULL;
            thrStack[i] = 0;
        }
        inited = 1;
    }
}

int get_empty_task_index(void)
{
    int i;
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 == thr[i]) {
            return i;
        }
    }

    return -1;
}

int mark_exit_task(TX_THREAD * thread_ptr)
{
    int i;
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 != thr[i]) {
            if (strcmp(thr[i]->tx_thread_name, thread_ptr->tx_thread_name) == 0) {
                thrStatus[i] = THR_TASK_STS_EXITED;
                DBG_TSK("---> marked exit task, index %d, %s, addr %p %p\n", i,
                        thr[i]->tx_thread_name, thr[i], thread_ptr);
                return i;
            }
        }
    }
}

int free_task_mem()
{
    int i;

#ifdef DEBUG_TASK
    unsigned int mem_free_size = mem_heap_get_free_size();
#endif
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if ((THR_TASK_STS_EXITED == thrStatus[i])) {
            if ((thr[i]) && (thrStack[i])) {
                DBG_TSK("---> Free %d, thr:%p, stk:%p.\n", i, thr[i], thrStack[i]);
                DBG_TSK("@@1 MEM FREE : %d.\n", mem_free_size);
                //tx_thread_delete(thr[i]);
                mem_free(thr[i]);
                mem_free(thrStack[i]);
                DBG_TSK("@@2 MEM FREE : %d.\n", mem_free_size);
                thr[i] = 0;
                thrStatus[i] = THR_TASK_STS_NULL;
                thrStack[i] = 0;
                taskName[2 * i] = 0;

            } else {
                DBG_TSK("!!!> Free %d Failed, thr[i] %p, thrStack[i] %p\n", i, thr[i], thrStack[i]);
            }
        }
    }
}

/*
  * qcom_task_del - delete the user task
*/
void qcom_task_del(void)
{
    int i;

    for (i = 0; i < MAX_TASK_NUM; i++) {
        if ((THR_TASK_STS_EXITED == thrStatus[i])) {
            if ((thr[i]) && (thrStack[i])) {
                tx_thread_delete(thr[i]);
            }
        }
    }

    return;
}

/*
  * qcom_task_start - create the user task
*/
int qcom_task_start(TX_THREAD *pthr, void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms)
{
    static int taskId = 0;
    int ret = 0;
    int taskIdx = 0;
    void *stk = NULL;

    task_init();
    qcom_task_del();
    TSK_SEM_LOCK;
    free_task_mem();
    taskIdx = get_empty_task_index();
    TSK_SEM_UNLOCK;
    if (-1 == taskIdx) {
        qcom_printf("task index is full.\n");
        ret = -1;
        goto errout;

    }

    stk = mem_alloc(stk_size);
    if (stk == NULL) {
        qcom_printf("malloc stack failed.\n");
        ret = -1;
        goto errout;

    }

    TSK_SEM_LOCK;
    thrStack[taskIdx] = (char *)stk;
    thr[taskIdx] = pthr;

    DBG_TSK("---> malloc %d, thr:%p, stk:%p.\n", taskIdx, thr[taskIdx], thrStack[taskIdx]);

    /* default priority is 4 */
    /* set the task name */
    taskName[2 * taskIdx] = '0' + taskId % 255;
    taskName[2 * taskIdx + 1] = 0;
    DBG_TSK("thr[%d]:%p stk:%p.\n", taskIdx, thr[taskIdx], stk);
    TSK_SEM_UNLOCK;
    ret = tx_thread_create(pthr, (char *) &taskName[2 * taskIdx], (void (*)(unsigned long)) fn, arg,
                           stk, stk_size, 16, 16, 4, TX_AUTO_START);

    if (0 != ret) {
        qcom_printf("thread create failed, return value : 0x%x.\n", ret);
        goto errout;
    } else {
        TSK_SEM_LOCK;
        thrStatus[taskIdx] = THR_TASK_STS_CREATED;
        taskId++;
        TSK_SEM_UNLOCK;
    }
    //extern unsigned int allocram_remaining_bytes;
  errout:
    if (0 != ret) {
        if (pthr != NULL) {
            mem_free(pthr);
        }

        if (stk != NULL) {
            mem_free(stk);
        }

        if ((taskIdx > 0) && (taskIdx < MAX_TASK_NUM)) {
            TSK_SEM_LOCK;
            thrStack[taskIdx] = NULL;
            thr[taskIdx] = NULL;
            TSK_SEM_UNLOCK;
        }
    }
    qcom_printf("### MEM FREE : %d.\n", mem_heap_get_free_size());
    return ret;
}

/*
  *  yield from user task and back to WiFi main loop check_idle().
*/
void qcom_task_yield()
{
	TX_THREAD *thread_ptr;

	TX_THREAD_GET_CURRENT(thread_ptr)

	tx_thread_suspend(thread_ptr);
}

/*
  * quit and delete the user task.  It is called in user task
*/
void qcom_task_exit()
{
    TX_THREAD *thread_ptr;

    TX_THREAD_GET_CURRENT(thread_ptr)
	qcom_printf("--->tsk exit : %s.\n", thread_ptr->tx_thread_name);
    qcom_task_del();
    TSK_SEM_LOCK;
    free_task_mem();            /*free old task */
    mark_exit_task(thread_ptr);
    TSK_SEM_UNLOCK;
    tx_thread_terminate(thread_ptr);

}


void mark_exit_all_task(void)
{
    int i;
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 != thr[i]) {
            thrStatus[i] = THR_TASK_STS_EXITED;
        }
    }

    return;
}

void qcom_task_terminate_all(void)
{
    int i;
    for (i = 0; i < MAX_TASK_NUM; i++) {
        if (0 != thr[i]) {
            tx_thread_terminate(thr[i]);
        }
    }

    return;
}

/*
  * quit and delete the user task.  It is called in polling task
*/
void qcom_task_kill_all(void)
{
    qcom_task_terminate_all();
    TSK_SEM_LOCK;
    mark_exit_all_task();
    TSK_SEM_UNLOCK;

    qcom_task_del();

    TSK_SEM_LOCK;
    free_task_mem();
    TSK_SEM_UNLOCK;

    return;
}

/*
  *  wakeup user task. It is called in WiFi main loop
 */
void qcom_task_wakeup()
{
}

