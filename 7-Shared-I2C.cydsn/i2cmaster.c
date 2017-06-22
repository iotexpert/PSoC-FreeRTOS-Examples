#include "i2cmaster.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include "semphr.h"

// This is the main queue for holding I2C transactions
// It is private and should only be used by the I2C Master
// you can add tasks with the public interface 
static QueueHandle_t i2cm_transactionQueue;


//
//
// i2cmaster_Task
//
//
// This is the main thread for processing I2C transactions
// It waits for a transaction to be put into the queue
//
//
// On the I2C Bus all Write transactions are of the form of:
//  7-bit I2C address + write bit
//  16-bit high register address (if using 16 bit mode)   
//  16-bit low register address
//  n-bytes
//  stop
//
//
// On the I2C Bus all Reads transactions are of the form of:
//  7-bit I2C address + write bit
//  16-bit high register address (if using 16 bit mode)   
//  16-bit low register address
//  Restart with read
//  n-1 bytes with ACKs
//  nth byte with NAK
//  stop


void i2cm_Task(void *arg)
{
    
    // The argument is the size of the queue
    uint32_t queueSize = (uint32_t)arg; // cast the arge
    
    I2C_Start(); // turn on the I2C Master
     
    i2cm_transaction_t i2cm_transaction; // A scratch transaction
    i2cm_transactionQueue = xQueueCreate(queueSize, sizeof(i2cm_transaction_t));
    
    uint32_t rval; // all of the I2C functions return a uint32_t
    
    while(1)
    {
        int i;
 
        // Wait until something gets put in the queue
        if(xQueueReceive(i2cm_transactionQueue,&i2cm_transaction,portMAX_DELAY) == pdTRUE)
        {
            // Begining of an I2C transaction
            *i2cm_transaction.i2cm_bytesProcessed = 0;
            // First write the register address
            rval = I2C_I2CMasterSendStart(i2cm_transaction.i2cm_address,I2C_I2C_WRITE_XFER_MODE);
            if(rval != I2C_I2C_MSTR_NO_ERROR) goto cleanup;
            
            if(i2cm_transaction.i2cm_regType == I2CM_BIT16) // if a 16 bit register then send the high
            {
                rval = I2C_I2CMasterWriteByte((i2cm_transaction.i2cm_register >> 8) & 0xFF);
                if(rval != I2C_I2C_MSTR_NO_ERROR) goto cleanup;
            }
            rval = I2C_I2CMasterWriteByte((i2cm_transaction.i2cm_register ) & 0xFF); // send the low byte
            if(rval != I2C_I2C_MSTR_NO_ERROR) goto cleanup;
            
            // if w send ...
            switch(i2cm_transaction.i2cm_method) // Look at the type of transaction they want to do...
            {
                case I2CM_WRITE:  // If it is a write... then just send out the bytes
                for(i=0; i < i2cm_transaction.i2cm_byteNum;i++)
                {
                    rval = I2C_I2CMasterWriteByte(i2cm_transaction.i2cm_bytes[i]);
                    if(rval != I2C_I2C_MSTR_NO_ERROR) goto cleanup;
                    *i2cm_transaction.i2cm_bytesProcessed += 1;
                }
                break;
            
                case I2CM_READ: // If it is a read.. then do a restart with READ... then read n-1 bytes
                rval = I2C_I2CMasterSendRestart(i2cm_transaction.i2cm_address,I2C_I2C_READ_XFER_MODE);
                if(rval != I2C_I2C_MSTR_NO_ERROR) goto cleanup;
                for(i=0; i < i2cm_transaction.i2cm_byteNum-1;i++)
                {
                    i2cm_transaction.i2cm_bytes[i] = I2C_I2CMasterReadByte(I2C_I2C_ACK_DATA);
                    if(i2cm_transaction.i2cm_bytes[i] & 0x10000000) goto cleanup;
                    *i2cm_transaction.i2cm_bytesProcessed += 1;
                }
                // Then read the last byte and NAK (saying you are done)
                i2cm_transaction.i2cm_bytes[i2cm_transaction.i2cm_byteNum-1] = I2C_I2CMasterReadByte(I2C_I2C_NAK_DATA);
                if(i2cm_transaction.i2cm_bytes[i2cm_transaction.i2cm_byteNum-1] & 0x10000000) goto cleanup;
                *i2cm_transaction.i2cm_bytesProcessed += 1;
                break;
            }

            rval = I2C_I2CMasterSendStop();
            if(rval != I2C_I2C_MSTR_NO_ERROR) goto cleanup;

            cleanup:
            CYASSERT(rval == 0); // something really bad happend
          
           // If they ask for a semphore when done... then give it
            if(i2cm_transaction.i2cm_doneSemaphore)
                xSemaphoreGive(i2cm_transaction.i2cm_doneSemaphore);     
        }
    }
    
}

// the i2cm_runTransaction function is the public interface to the system
// This function is thread safe (it can be called from any thread)
// It will queue up an I2C tranaction and then wait (if there is a semaphore handle)
void i2cm_runTransaction(i2cm_transaction_t *i2cm_transaction)
{
    if(xQueueSend(i2cm_transactionQueue,i2cm_transaction,portMAX_DELAY) != pdTRUE)
    {
        // Highly bad
        CYASSERT(0); // 
    }
    if(i2cm_transaction->i2cm_doneSemaphore)
        xSemaphoreTake(i2cm_transaction->i2cm_doneSemaphore,portMAX_DELAY);
}