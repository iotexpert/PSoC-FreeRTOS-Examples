/* registers */
// output register x
#define KXTJ2_OUTX_L 0x06
#define KXTJ2_OUTX_H 0x07
// output register y
#define KXTJ2_OUTY_L 0x08
#define KXTJ2_OUTY_H 0x09
// output register z
#define KXTJ2_OUTZ_L 0x0A
#define KXTJ2_OUTZ_H 0x0B
// This register can be used to verify proper integrated circuit functionality
#define KXTJ2_DCST_RESP 0x0C
// This register can be used for supplier recognition, as it can be factory written to a known byte value.
#define KXTJ2_WHO_AM_I 0x0F
// This register reports which function caused an interrupt.
#define KXTJ2_INT_SOURCE1 0x16
// This register reports the axis and direction of detected motion
#define KXTJ2_INT_SOURCE2 0x17
// This register reports the status of the interrupt
#define KXTJ2_STATUS_REG 0x18
// Latched interrupt source information
#define KXTJ2_INT_REL 0x1A
// Read/write control register that controls the main feature set
#define KXTJ2_CTRL_REG1 0x1B
// Read/write control register that provides more feature set control
#define KXTJ2_CTRL_REG2 0x1D
// This register controls the settings for the physical interrupt pin
#define KXTJ2_INT_CTRL_REG1 0x1E
// This register controls which axis and direction of detected motion can cause an interrupt
#define KXTJ2_INT_CTRL_REG2 0x1F
// Read/write control register that configures the acceleration outputs
#define KXTJ2_DATA_CTRL_REG 0x21
// This register sets the time motion must be present before a wake-up interrupt is set
#define KXTJ2_WAKEUP_TIMER 0x29
#define KXTJ2_SELF_TEST 0x3A
// This register sets the threshold for wake-up (motion detect) interrupt is se
#define KXTJ2_WAKEUP_THRESHOLD 0x6A
/* registers bits */
// before set
#define KXTJ2_DCST_RESP_COM_TEST_BEFORE (0x55 << 0)
// after set
#define KXTJ2_DCST_RESP_COM_TEST_AFTER (0xAA << 0)
// WHO_AM_I -value
#define KXTJ2_WHO_AM_I_WIA_ID (0x09 << 0)
// indicates that new acceleration data
#define KXTJ2_INT_SOURCE1_DRDY (0x01 << 4)
// Wake up
#define KXTJ2_INT_SOURCE1_WUFS (0x01 << 1)
// x-
#define KXTJ2_INT_SOURCE2_XNWU (0x01 << 5)
// x+
#define KXTJ2_INT_SOURCE2_XPWU (0x01 << 4)
// y-
#define KXTJ2_INT_SOURCE2_YNWU (0x01 << 3)
// y+
#define KXTJ2_INT_SOURCE2_YPWU (0x01 << 2)
// z-
#define KXTJ2_INT_SOURCE2_ZNWU (0x01 << 1)
// z+
#define KXTJ2_INT_SOURCE2_ZPWU (0x01 << 0)
// reports the combined (OR) interrupt information of DRDY and WUFS in the interrupt source register
#define KXTJ2_STATUS_REG_INT (0x01 << 4)
// controls the operating mode of the KXTJ2
#define KXTJ2_CTRL_REG1_PC (0x01 << 7)
// determines the performance mode of the KXTJ2
#define KXTJ2_CTRL_REG1_RES (0x01 << 6)
// enables the reporting of the availability of new acceleration data as an interrupt
#define KXTJ2_CTRL_REG1_DRDYE (0x01 << 5)
// 2g range
#define KXTJ2_CTRL_REG1_GSEL_2G (0x00 << 3)
// 4g range
#define KXTJ2_CTRL_REG1_GSEL_4G (0x01 << 3)
// 8g range
#define KXTJ2_CTRL_REG1_GSEL_8G (0x02 << 3)
// 8g range with 14b resolution
#define KXTJ2_CTRL_REG1_GSEL_8G_14B (0x03 << 3)
// enables the Wake Up (motion detect) function.
#define KXTJ2_CTRL_REG1_WUFE (0x01 << 1)
// initiates software reset
#define KXTJ2_CTRL_REG2_SRST (0x01 << 7)
// initiates the digital communication self-test function.
#define KXTJ2_CTRL_REG2_DCST (0x01 << 4)
// 0.78Hz
#define KXTJ2_CTRL_REG2_OWUF_0P781 (0x00 << 0)
// 1.563Hz
#define KXTJ2_CTRL_REG2_OWUF_1P563 (0x01 << 0)
// 3.125Hz
#define KXTJ2_CTRL_REG2_OWUF_3P125 (0x02 << 0)
// 6.25Hz
#define KXTJ2_CTRL_REG2_OWUF_6P25 (0x03 << 0)
// 12.5Hz
#define KXTJ2_CTRL_REG2_OWUF_12P5 (0x04 << 0)
// 25Hz
#define KXTJ2_CTRL_REG2_OWUF_25 (0x05 << 0)
// 50Hz
#define KXTJ2_CTRL_REG2_OWUF_50 (0x06 << 0)
// 100Hz
#define KXTJ2_CTRL_REG2_OWUF_100 (0x07 << 0)
// enables/disables the physical interrupt pin
#define KXTJ2_INT_CTRL_REG1_IEN (0x01 << 5)
// sets the polarity of the physical interrupt pin
#define KXTJ2_INT_CTRL_REG1_IEA (0x01 << 4)
// sets the response of the physical interrupt pin
#define KXTJ2_INT_CTRL_REG1_IEL (0x01 << 3)
// selftest polarity
#define KXTJ2_INT_CTRL_REG1_STPOL (0x01 << 1)
// x-
#define KXTJ2_INT_CTRL_REG2_XNWU (0x01 << 5)
// x+
#define KXTJ2_INT_CTRL_REG2_XPWU (0x01 << 4)
// y-
#define KXTJ2_INT_CTRL_REG2_YNWU (0x01 << 3)
// y+
#define KXTJ2_INT_CTRL_REG2_YPWU (0x01 << 2)
// z-
#define KXTJ2_INT_CTRL_REG2_ZNWU (0x01 << 1)
// z+
#define KXTJ2_INT_CTRL_REG2_ZPWU (0x01 << 0)
// 12.5Hz
#define KXTJ2_DATA_CTRL_REG_OSA_12P5 (0x00 << 0)
// 25Hz
#define KXTJ2_DATA_CTRL_REG_OSA_25 (0x01 << 0)
// 50Hz
#define KXTJ2_DATA_CTRL_REG_OSA_50 (0x02 << 0)
// 100Hz
#define KXTJ2_DATA_CTRL_REG_OSA_100 (0x03 << 0)
// 200Hz
#define KXTJ2_DATA_CTRL_REG_OSA_200 (0x04 << 0)
// 400Hz
#define KXTJ2_DATA_CTRL_REG_OSA_400 (0x05 << 0)
// 800Hz
#define KXTJ2_DATA_CTRL_REG_OSA_800 (0x06 << 0)
// 1600Hz
#define KXTJ2_DATA_CTRL_REG_OSA_1600 (0x07 << 0)
// 0.78Hz
#define KXTJ2_DATA_CTRL_REG_OSA_0P781 (0x08 << 0)
// 1.563Hz
#define KXTJ2_DATA_CTRL_REG_OSA_1P563 (0x09 << 0)
// 3.125Hz
#define KXTJ2_DATA_CTRL_REG_OSA_3P125 (0x0A << 0)
// 6.25Hz
#define KXTJ2_DATA_CTRL_REG_OSA_6P25 (0x0B << 0)
// charge on
#define KXTJ2_SELF_TEST_TEST_ENABLE (0xCA << 0)
// charge off
#define KXTJ2_SELF_TEST_TEST_DISABLE (0x00 << 0)
 /*registers bit masks */

#define KXTJ2_DCST_RESP_COM_TEST_MASK 0xFF

#define KXTJ2_WHO_AM_I_WIA_MASK 0xFF
// selects the acceleration range of the accelerometer outputs
#define KXTJ2_CTRL_REG1_GSEL_MASK 0x18
// sets the Output Data Rate for the Wake Up function
#define KXTJ2_CTRL_REG2_OWUF_MASK 0x07
// sets the output data rate (ODR)
#define KXTJ2_DATA_CTRL_REG_OSA_MASK 0x0F
// When 0xCA is written to this register, the MEMS self-test function is enabled
#define KXTJ2_SELF_TEST_TEST_MASK 0xFF