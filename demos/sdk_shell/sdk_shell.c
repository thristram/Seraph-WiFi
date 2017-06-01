/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_system.h"
#include "threadx/tx_api.h"
#include "qcom_cli.h"
#include "qcom_common.h"
#include "qcom_uart.h"
#include "qcom_utils.h"
#include <qcom/qcom_gpio.h>

TX_THREAD host_thread;
TX_BYTE_POOL pool;

#define BYTE_POOL_SIZE (5*1024)
#define PSEUDO_HOST_STACK_SIZE (4 * 1024)   /* small stack for pseudo-Host thread */

extern void Uart_Rx_Pin_Set(int pin0, int pin1);         //Paramenter 1: UART0; Parameter 2: UART1
extern void Uart_Tx_Pin_Set(int pin0, int pin1);
void
shell_host_entry(ULONG which_thread)
{
#if 1
#if 0
        //uart1 as debug
        Uart_Rx_Pin_Set(10, 6);		 
		Uart_Tx_Pin_Set(11, 7);
#else
        //uart0 as debug
        Uart_Rx_Pin_Set(6,10);		 
		Uart_Tx_Pin_Set(7,11);
#endif
	qcom_uart_init();
    qcom_uart_open((char *)"UART0");
	qcom_uart_open((char *)"UART1");
    extern void user_pre_init(void);
    user_pre_init();

    extern console_cmd_t cust_cmds[];

    extern int cust_cmds_num;

    extern void task_execute_cli_cmd();

    console_setup();

    console_reg_cmds(cust_cmds, cust_cmds_num);

    //A_PRINTF("cli started ---------------\n");

    task_execute_cli_cmd();
#else    
    extern void user_pre_init(void);
    user_pre_init();

    extern console_cmd_t cust_cmds[];

    extern int cust_cmds_num;

    extern void task_execute_cli_cmd();

    console_setup();

    console_reg_cmds(cust_cmds, cust_cmds_num);

    //A_PRINTF("cli started ---------------\n");

    task_execute_cli_cmd();
    /* Never returns */
#endif
}


void user_main(void)
{
    int i;
    
    extern void task_execute_cli_cmd();
    
    //A_PRINTF("tx_application_define: \n");
    tx_byte_pool_create(&pool, "cdrtest pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);

    {
        CHAR *pointer;
        tx_byte_allocate(&pool, (VOID **) & pointer, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);

        tx_thread_create(&host_thread, "cdrtest thread", shell_host_entry,
                         i, pointer, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);
    }
    
}

