#ifndef _I2C_SEND_H_
#define _I2C_SEND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "myHeaders.h"

extern uint32_t I2C_FREQ;

void i2c_init();
void I2C_Probe();
void I2C_Transaction_Write(uint8_t slave_addr, uint8_t writeSize);
void I2C_Transaction_Read(uint8_t slave_addr, uint8_t readSize);
void I2C_Transaction_WriteRead(uint8_t slave_addr, uint8_t writeSize, uint8_t readSize);
void I2C_SEND();



#ifdef __cplusplus
}
#endif

#endif /* _I2C_SEND_H_ */
