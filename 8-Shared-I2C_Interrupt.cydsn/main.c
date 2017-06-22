#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "semphr.h"
#include "i2cmaster.h"
#include "kxtj2.h"

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

SemaphoreHandle_t accelIntSemaphore;

TaskHandle_t accelHandle;

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

CY_ISR(accelIntHandler)
{
    accel_int_pin_ClearInterrupt();
    xSemaphoreGiveFromISR(accelIntSemaphore,NULL);
}

CY_PACKED typedef struct accel {
    int16_t x;
    int16_t y;
    int16_t z;
} CY_PACKED_ATTR accel_t;


// This task manages the aceleromter
// 
char buff[128];
void accel_Task(void *arg)
{
    (void)arg;
    SemaphoreHandle_t mySemaphore;
    i2cm_transaction_t mytransaction;
    int x,y,z;
    int xs,ys,zs;
    int byteCount;
    accel_t myData;
    mySemaphore = xSemaphoreCreateBinary();
    accelIntSemaphore = xSemaphoreCreateBinary(); // Signal from the Accel that it is ready
    accel_int_StartEx(accelIntHandler);
    
    // True of all read/writes
    mytransaction.i2cm_address = 0x0F;
    mytransaction.i2cm_regType = I2CM_BIT8;
    mytransaction.i2cm_bytesProcessed = &byteCount;
   
    // Setup for innitialization
    uint8_t setupData[1];
    mytransaction.i2cm_method = I2CM_WRITE;
    mytransaction.i2cm_byteNum = 1;
    mytransaction.i2cm_doneSemaphore = mySemaphore;
    mytransaction.i2cm_bytes = setupData;
    
    // reset the accelerometer
    setupData[0] = 0b00000000; 
    mytransaction.i2cm_register = KXTJ2_CTRL_REG1; 
    i2cm_runTransaction(&mytransaction);
  
    setupData[0] = KXTJ2_CTRL_REG2_SRST; // Software reset
    mytransaction.i2cm_register = KXTJ2_CTRL_REG2; 
    i2cm_runTransaction(&mytransaction);

    vTaskDelay(10); // From the datasheet power up time

    while(1) // delay until the accelerometer is reset
    {
        mytransaction.i2cm_method = I2CM_READ;  
        mytransaction.i2cm_register = KXTJ2_CTRL_REG2;
        i2cm_runTransaction(&mytransaction);
        
        if(setupData[0] & KXTJ2_CTRL_REG2_SRST)
        {
            vTaskDelay(10); // backup 10 ms
        }
        else
            break;  
    }
    
    mytransaction.i2cm_method = I2CM_WRITE; // Go back to writing for setup
    
    // initialize the interrupt 
    setupData[0] = KXTJ2_INT_CTRL_REG1_IEN | KXTJ2_INT_CTRL_REG1_IEA | KXTJ2_INT_CTRL_REG1_IEL;
    mytransaction.i2cm_register = KXTJ2_INT_CTRL_REG1; 
    i2cm_runTransaction(&mytransaction);
  
    // initialize the  DATA_CTRL_REG
    setupData[0] = KXTJ2_DATA_CTRL_REG_OSA_12P5;      
    mytransaction.i2cm_register = KXTJ2_DATA_CTRL_REG; 
    i2cm_runTransaction(&mytransaction);
 
    // Initalize the acceleromter CTRL_REG_1
    setupData[0] =   KXTJ2_CTRL_REG1_PC | KXTJ2_CTRL_REG1_RES | KXTJ2_CTRL_REG1_DRDYE | KXTJ2_CTRL_REG1_GSEL_2G  ; 
    mytransaction.i2cm_register = KXTJ2_CTRL_REG1; 
    i2cm_runTransaction(&mytransaction);
    
    // Setup for repeated reads
    mytransaction.i2cm_method = I2CM_READ;
    mytransaction.i2cm_register = 0x06;
    mytransaction.i2cm_byteNum = sizeof(myData);
    mytransaction.i2cm_doneSemaphore = mySemaphore;
    mytransaction.i2cm_bytes = (uint8_t *)&myData;
        
    while(1)  // Run the same transaction over and over
    {
        xSemaphoreTake(accelIntSemaphore,portMAX_DELAY);
        
        i2cm_runTransaction(&mytransaction);
        myData.x /= 16;  // The KXTJ2 Datasheet say lower 4 bits are X
        myData.y /= 16;  // The KXTJ2 Datasheet say lower 4 bits are X
        myData.z /= 16;  // The KXTJ2 Datasheet say lower 4 bits are X
        xs = (myData.x < 0? -1:1);  // Find the XSign
        ys = (myData.y < 0? -1:1);  // Find the YSign
        zs = (myData.z < 0? -1:1);  // Find the ZSign
            
        // The 1024 is a hardcode based on the range of the acceleromter
        x = (int)((float)myData.x / 1024.0 * 10.0) * xs; // Turn it into integer G * 10
        y = (int)((float)myData.y / 1024.0 * 10.0) * ys; // Turn it into integer G * 10
        z = (int)((float)myData.z / 1024.0 * 10.0) * zs; // Turn it into integer G * 10
       
        // Format data in x.x  
        sprintf(buff,"x=%s%d.%dg\ty=%s%d.%dg\tz=%s%d.%dg\n",(xs<0?"-":""),x/10,x%10,(ys<0?"-":""),y/10,y%10,(zs<0?"-":""),z/10,z%10);
        UART_UartPutString(buff); 
    }
}

void UART_Task(void *arg)
{
    (void)arg;
    static char buff[500];
    uint8_t data[10];
    UART_Start();
    UART_UartPutString("Started Project\n");
    UART_UartPutString("Press ? for help\n");
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
                    mytransaction.i2cm_register = 0x0102;
                    mytransaction.i2cm_method = I2CM_READ;
                    mytransaction.i2cm_bytesProcessed = &byteCount;
                    mytransaction.i2cm_bytes = data;
                    mytransaction.i2cm_doneSemaphore = mySemaphore;
                    i2cm_runTransaction(&mytransaction);
                    sprintf(buff,"Bytes=%d 0=%d 1=%d 2=%d 3=%d\n",byteCount,data[0],data[1],data[2],data[3]);
                    UART_UartPutString(buff);                  
            break;
            case 'i': // This will increment the values (that you read presumably from the FRAM
                    data[0] += 1;
                    data[1] += 1;
                    data[2] += 1;
                    data[3] += 1;
                    sprintf(buff,"0=%d 1=%d 2=%d 3=%d\n",data[0],data[1],data[2],data[3]);
                    UART_UartPutString(buff);
            break;
           
            case 'w': //This will write 4 values to register 0 in the FRAM
                    mytransaction.i2cm_address = 0x50;
                    mytransaction.i2cm_regType = I2CM_BIT16;
                    mytransaction.i2cm_byteNum = 4;
                    mytransaction.i2cm_register = 0x0102;
                    mytransaction.i2cm_method = I2CM_WRITE;
                    mytransaction.i2cm_bytesProcessed = &byteCount;
                    mytransaction.i2cm_bytes = data;
                    mytransaction.i2cm_doneSemaphore = mySemaphore;
                    i2cm_runTransaction(&mytransaction);
                    sprintf(buff,"Wrote %d bytes\n",byteCount);
                    UART_UartPutString(buff);
            break;
                    
            case 's': // stop the accel task 
                    vTaskSuspend(accelHandle);
            break;
                    
            case 'S':  // start the accel task back up
                    vTaskResume(accelHandle);
            break;
                    
            case 'p':
                    mytransaction.i2cm_method = I2CM_READ;
                    mytransaction.i2cm_address = 0x0F;            // Acelleromter I2C Address
                    mytransaction.i2cm_regType = I2CM_BIT8;       // 8-bit addressing
                    mytransaction.i2cm_register = 0x0F;           // WHO_AM_I Register
               
                    mytransaction.i2cm_byteNum = 1;               // One byte
                    mytransaction.i2cm_bytes = data;
               
                    mytransaction.i2cm_bytesProcessed = &byteCount;
                    mytransaction.i2cm_doneSemaphore = mySemaphore;
                    i2cm_runTransaction(&mytransaction);
                    
                    sprintf(buff,"Bytes=%d WHO_AM_I=0x%02X\n",byteCount,data[0]);
                    UART_UartPutString(buff);          
            break;
            case '?':
                    UART_UartPutString(" ----------------------------\n");
                    UART_UartPutString(" Help Commands\n");
                    UART_UartPutString(" ----------------------------\n");
                    UART_UartPutString("r-change the led color to red\n");
                    UART_UartPutString("g-change the led color to green\n");
                    UART_UartPutString("b-change the led color to blue\n");
                    UART_UartPutString("t-dump the FreeRTOS Task List\n");
                    UART_UartPutString("q-Read 4 values from the FRAM into data[0-3]\n");
                    UART_UartPutString("i-Increment the values in data[0-3] by 1\n");
                    UART_UartPutString("w-Write 4 values from data[0-3] to the FRAM at location 0 \n");
                    UART_UartPutString("s-Stop Acceleromter Task\n");
                    UART_UartPutString("S-Start Acceleromter Task\n");
                    UART_UartPutString("?-List the commands\n");
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
        750,           /* Task stack, allocated from heap */
        0,               /* No param passed to task function */
        1,               /* Low priority */
        0 );             /* Not using the task handle */

  
    /* To print when the counting semaphore is taken */
    xTaskCreate(
        i2cm_Task,  /* Task function */
        "i2c Master",    /* Task name (string) */
        0x100,           /* Task stack, allocated from heap */
        (void *)5,       /* Size of the queue */
        1,               /* Low priority */
        0 );
    
    xTaskCreate(
        accel_Task,     /* Task function */
        "Accel Master",   /* Task name (string) */
        0x100,           /* Task stack, allocated from heap */
        0,               /* No Argument */
        1,               /* Low priority */
        &accelHandle );
    
        vTaskSuspend(accelHandle); // Start with the acceleromter task suspended
    
    vTaskStartScheduler();
    while(1);
}

