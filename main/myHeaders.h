
// Header File

#ifndef _MYHEADERS_H_
#define _MYHEADERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <inttypes.h>
#include "string.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <driver/spi_master.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <driver/gpio.h>
#include "esp_spiffs.h"
#include <esp_log.h>
#include <string.h>

typedef struct {
    uint8_t * buffer;
    int head;
    int tail;
    const int maxlen;
} circ_bbuf_t;

typedef struct
{
    uint8_t *data;
    int length;
}commands_t;



void Process_Commands();
int ExtractParameters(uint8_t*, uint8_t);
void Init_Param_Array();
void printh(char*);
uint8_t HexStr2Decimal(const char*);
uint32_t HexStr2Decimal32(const char*);
void Init_Console();
void cmd_buffer_push(uint8_t * cmd, uint8_t len);
void cmd_buffer_pop(uint8_t * cmd, uint8_t * len , uint8_t dir);




#define UART_ECHO_TASK_STACK_SIZE 2048
#define UART_BUFFER_SIZE 256
#define PARAM_SIZE 32
#define MAX_PARAMS 32
#define MAX_COMMAND_HISTORY_LEN 5

#define PIN_NUM_CS_USER     21
#define PIN_NUM_MISO        37
#define PIN_NUM_MOSI        35
#define PIN_NUM_SCK         36
#define PIN_NUM_CS_LOAD     42

#define PIN_NUM_INITN 1
#define PIN_NUM_DONE 3

#define JTAGEN_PIN  10
#define PROGRAMN_PIN  11
#define DONE_PIN  3
#define INITN_PIN  0

#define I2C_MASTER_SDA_IO 16
#define I2C_MASTER_SCL_IO 18

#define TIMEOUT_MS_I2C 1000
#define dev_i2c I2C_NUM_0


#define st_FindParameter 1
#define st_FillParameter 2

#define DUMMY_BYTE 0

extern uint8_t spi_tx_data[32];
extern uint8_t spi_rx_data[32];
extern uint8_t i2c_read_data_buff[128];
extern uint8_t i2c_write_data_buff[128];
extern uint8_t *PARAM_ARRAY[MAX_PARAMS];
extern uint8_t Num_Params;
extern uint8_t myState;

extern spi_device_handle_t dev_spi;
extern i2c_master_bus_handle_t i2c_bus_handle;
extern spi_bus_config_t spibuscfg;





#ifdef __cplusplus
}
#endif

#endif /* _MYHEADERS_H_ */