#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "semphr.h"
#include "i2cmaster.h"

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
SemaphoreHandle_t i2cSemaphore;
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
    uint8_t data[10];
    UART_Start();
    UART_UartPutString("Started UART\n\r");
    char c;
    Color_t tempColor;
    int byteCount;
    SemaphoreHandle_t mySemaphore;
    
    i2cm_transaction_t mytransaction;
    
    mySemaphore = xSemaphoreCreateBinary();
    
	while(1) {
        c = UART_UartGetChar();
        switch(c)
        {
            
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
                    
                    // The CY8CKIT-044 has an FRAM on I2C Address 0x50
                    // This will read 4 bytes from registers 0,1,2,3
                    // then print them on the terminal
            case 'q':
                    
                    mytransaction.i2cm_address = 0x50;
                    mytransaction.i2cm_regType = I2CM_BIT16;
                    mytransaction.i2cm_byteNum = 4;
                    mytransaction.i2cm_register = 0x00;
                    mytransaction.i2cm_method = I2CM_READ;
                    mytransaction.i2cm_bytesProcessed = &byteCount;
                    mytransaction.i2cm_bytes = data;
                    mytransaction.i2cm_doneSemaphore = mySemaphore;
                    i2cm_runTransaction(&mytransaction);
                    sprintf(buff,"Bytes=%d 0=%d 1=%d 2=%d 3=%d\n",byteCount,data[0],data[1],data[2],data[3]);
                    UART_UartPutString(buff);                  
            break;
            case 'i':
                    // This will increment the values (that you read presumably from the FRAM
                    data[0] += 1;
                    data[1] += 1;
                    data[2] += 1;
                    data[3] += 1;
                    sprintf(buff,"0=%d 1=%d 2=%d 3=%d\n",data[0],data[1],data[2],data[3]);
                    UART_UartPutString(buff);
            break;
           
            case 'w':
        
                    // This will write 4 values to register 0 in the FRAM
                    mytransaction.i2cm_address = 0x50;
                    mytransaction.i2cm_regType = I2CM_BIT16;
                    mytransaction.i2cm_byteNum = 4;
                    mytransaction.i2cm_register = 0x00;
                    mytransaction.i2cm_method = I2CM_WRITE;
                    mytransaction.i2cm_bytesProcessed = &byteCount;
                    mytransaction.i2cm_bytes = data;
                    mytransaction.i2cm_doneSemaphore = mySemaphore;
                    i2cm_runTransaction(&mytransaction);
                    sprintf(buff,"Wrote %d bytes\n",byteCount);
                    UART_UartPutString(buff);
            break;
        }
        taskYIELD();
    }
}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    setupFreeRTOS();
  
    colorQueue = xQueueCreate(1, sizeof(Color_t)); // 1 item queue that can hold colors
    
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

  
    /* To print when the counting semaphore is taken */
    xTaskCreate(
        i2cmaster_Task,  /* Task function */
        "i2c Master",    /* Task name (string) */
        0x100,           /* Task stack, allocated from heap */
        (void *)5,       /* Size of the queue */
        1,               /* Low priority */
        0 );
    
    vTaskStartScheduler();
    while(1);
}

