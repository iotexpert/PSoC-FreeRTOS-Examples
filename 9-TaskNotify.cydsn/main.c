#include "project.h"
#include "FreeRTOS.h"
#include "timers.h"


extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);
#define CORTEX_INTERRUPT_BASE          (16)

TaskHandle_t uartTaskHandle;

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

CY_ISR(uartHandler)
{
    BaseType_t xHigherPriorityTaskWoken;

    // disable the interrupt
    UART_SetRxInterruptMode(0);
    
    vTaskNotifyGiveFromISR(uartTaskHandle,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void UART_Task( void *arg)
{
    (void)arg;
    char c;
    UART_Start();
    UART_SetCustomInterruptHandler(uartHandler);
    while(1)
    {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        while(UART_SpiUartGetRxBufferSize())
        {
            c = UART_UartGetChar();
            UART_UartPutChar(c);
        }
        // clear & re-enable the interrupt
        UART_ClearRxInterruptSource(UART_INTR_RX_NOT_EMPTY);
        UART_SetRxInterruptMode(UART_INTR_RX_NOT_EMPTY);
    }
}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    setupFreeRTOS();
  
    /* Create LED task, which will blink the RED LED */
    xTaskCreate(
        LED_Task,       /* Task function */
        "LED Blink",    /* Task name (string) */
        200,            /* Task stack, allocated from heap */
        0,              /* No param passed to task function */
        1,              /* Low priority */
        0 );            /* Not using the task handle */
     
    xTaskCreate(UART_Task,"UART Task",400,0,1,&uartTaskHandle);
    
    vTaskStartScheduler();
    while(1); // get rid of the stupid warning
}

