#ifndef _SPI_SEND_H_
#define _SPI_SEND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "myHeaders.h"

void SPI_Transaction_Transmit(uint8_t spi_length);
void SPI_SEND();
void spi_init(int clk_speed); 

#ifdef __cplusplus
}
#endif

#endif /* _SPI_SEND_H_ */