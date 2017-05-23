#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "semphr.h"
#include <stdio.h>

extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);
#define CORTEX_INTERRUPT_BASE          (16)


typedef enum Color_t {
    RED,
    GREEN,
    BLUE
} Color_t;

QueueHandle_t colorQueue;
SemaphoreHandle_t switchSemaphore;
SemaphoreHandle_t countingSemaphore;


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
    
    Color_t currentColor = GREEN;
    
	while(1) {
        
        if(xQueueReceive(colorQueue,&currentColor,0) == pdTRUE)
        {
            RED_Write(1); BLUE_Write(1); GREEN_Write(1);
        }
        if(currentColor == RED)
            RED_Write(~RED_Read());
        else if (currentColor == BLUE)
            BLUE_Write(~BLUE_Read());
        else
            GREEN_Write(~GREEN_Read());
        
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
    Color_t tempColor;
    
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
                
            case 'r':
                tempColor = RED;
                if(xQueueSend(colorQueue,&tempColor,0) != pdTRUE)
                    UART_UartPutString("queue error\n\r");
            break;
                
            case 'b':
                tempColor = BLUE;
                if(xQueueSend(colorQueue,&tempColor,0) != pdTRUE)
                    UART_UartPutString("queue error\n\r");
            break;

            case 'g':
                tempColor = GREEN;
                if(xQueueSend(colorQueue,&tempColor,0) != pdTRUE)
                    UART_UartPutString("queue error\n\r");
            break;
                    
            case 's':
                    xSemaphoreGive(switchSemaphore);
            break;
                    
            case '1':
                    xSemaphoreGive(countingSemaphore);
            break;
            
            case '2':
                    xSemaphoreGive(countingSemaphore);
                     xSemaphoreGive(countingSemaphore);
            break;

            case '3':
                xSemaphoreGive(countingSemaphore);
                xSemaphoreGive(countingSemaphore);
                xSemaphoreGive(countingSemaphore);
            break;
        }
        taskYIELD();
    }
}

void semaphore_Task(void *arg)
{
    (void)arg;
    while(1)
    {
        
        xSemaphoreTake(switchSemaphore,portMAX_DELAY);
        UART_UartPutString("Taken Switch Semaphore\n");
    }
}

void countingSemaphore_Task(void *arg)
{
   (void)arg;
    char buff[16];
    while(1)
    {
        xSemaphoreTake(countingSemaphore,portMAX_DELAY);
        sprintf(buff,"Count = %d\n",(int)uxSemaphoreGetCount( countingSemaphore ));
        UART_UartPutString(buff);
    }
}


CY_ISR(isr_1_Handler)
{
    SW_ClearInterrupt();
    xSemaphoreGiveFromISR(switchSemaphore,NULL);
}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    setupFreeRTOS();
  
    colorQueue = xQueueCreate(1, sizeof(Color_t)); // 1 item queue that can hold colors
    switchSemaphore = xSemaphoreCreateBinary();
    isr_1_StartEx(isr_1_Handler);
    
    countingSemaphore = xSemaphoreCreateCounting(5,0);
    
    
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

        /* To print when the semaphore is taken */
    xTaskCreate(
        semaphore_Task,       /* Task function */
        "semaphore",          /* Task name (string) */
        0x100,           /* Task stack, allocated from heap */
        0,               /* No param passed to task function */
        1,               /* Low priority */
        0 );
  
    
          /* To print when the semaphore is taken */
    xTaskCreate(
        countingSemaphore_Task,       /* Task function */
        "countsema",          /* Task name (string) */
        0x100,           /* Task stack, allocated from heap */
        0,               /* No param passed to task function */
        1,               /* Low priority */
        0 );
  
    
    vTaskStartScheduler();
    while(1);
}

