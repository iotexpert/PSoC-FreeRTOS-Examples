/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);
#define CORTEX_INTERRUPT_BASE          (16)
void setupFreeRTOS()
{
    /* Handler for Cortex Supervisor Call (SVC, formerly SWI) - address 11 */
    CyIntSetSysVector( CORTEX_INTERRUPT_BASE + SVCall_IRQn,
        (cyisraddress)vPortSVCHandler );
    
    /* Handler for Cortex PendSV Call - address 14 */
	CyIntSetSysVector( CORTEX_INTERRUPT_BASE + PendSV_IRQn,
        (cyisraddress)xPortPendSVHandler );    
    
    /* Handler for Cortex SYSTICK - address 15 */
	CyIntSetSysVector( CORTEX_INTERRUPT_BASE + SysTick_IRQn,
        (cyisraddress)xPortSysTickHandler );
}


void LED_Task(void *arg)
{
    (void)arg;
    
	while(1) {
        RED_Write(~RED_Read());
        vTaskDelay(500);
	}
}

void UART_Task(void *arg)
{
    (void)arg;
    static char buff[500];
    UART_Start();
    UART_UartPutString("Started UART\n\r");
    char c;
    
	while(1) {
        c = UART_UartGetChar();
        switch(c)
        {
            case 'a':
            UART_UartPutString("Working\n\r");
            break;
            
            case 't':
                UART_UartPutString("********************************************\n\r");
                UART_UartPutString("Task          State   Prio    Stack    Num\n\r"); 
                UART_UartPutString("********************************************\n\r");
                vTaskList(buff);
                UART_UartPutString(buff);
                UART_UartPutString("*********************************************\n\r");
   
            break;
        }
        taskYIELD();
    }
}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    setupFreeRTOS();
  
    /* Create LED task, which will blink the LED */
    xTaskCreate(
        LED_Task,       /* Task function */
        "LED Blink",    /* Task name (string) */
        200,            /* Task stack, allocated from heap */
        0,              /* No param passed to task function */
        1,              /* Low priority */
        0 );            /* Not using the task handle */
   
    
    /* Create UART Task which will control the serial port */
    xTaskCreate(
        UART_Task,       /* Task function */
        "UART",          /* Task name (string) */
        0x400,           /* Task stack, allocated from heap */
        0,               /* No param passed to task function */
        1,               /* Low priority */
        0 );             /* Not using the task handle */
   
    vTaskStartScheduler();
    while(1);
}

