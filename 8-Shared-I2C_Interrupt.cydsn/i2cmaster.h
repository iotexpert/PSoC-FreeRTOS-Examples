#include <project.h>
#include "FreeRTOS.h"
#include "semphr.h"

#ifndef I2C_MASTER_H
#define I2C_MASTER_H

    
typedef enum i2cm_transactionMethod {
    I2CM_READ,
    I2CM_WRITE
} i2cm_transactionMethod_t;

typedef enum i2cm_registerAddresType {
    I2CM_BIT8,
    I2CM_BIT16
 
} i2cm_registerAddressType_t;


typedef struct i2cm_transaction {
    i2cm_transactionMethod_t i2cm_method;        // I2CM_READ or I2CM_WRITE
    uint8_t i2cm_address;                        // The I2C Address of the slave
    i2cm_registerAddressType_t i2cm_regType;     // I2CM_8BIT or I2CM_16BIT
    uint16_t i2cm_register;                      // The register in the slave
    
    uint8_t *i2cm_bytes;                         // A pointer to the data to be written (or a place to save the data)
    int i2cm_byteNum;                            // How many bytes are in the request
 
    int *i2cm_bytesProcessed;                    // A return value with the number of bytes written
    SemaphoreHandle_t i2cm_doneSemaphore;        // If you want a semaphore flagging that the transaction is done
 
} i2cm_transaction_t;

void i2cm_Task(void *arg);                  // The main thread function... you should start it in the RTOS
void i2cm_runTransaction(i2cm_transaction_t *i2cm_Transaction); // The public interace

#endif