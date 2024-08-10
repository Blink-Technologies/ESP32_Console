
#include "spi_load.h"

static const char *TAG = "SPI_LOAD";

//Comment this line to disable debug messages
//#define DEBUG_INFO


volatile uint8_t receivedData;
uint8_t src_addr[32];
uint8_t dest_addr[32];
uint8_t Status_Register[4];

uint8_t READ_ID_CMD[4]={0xE0,0x00,0x00,0x00};
uint8_t REFRESH_CMD[3]={0x79,0x00,0x00};
uint8_t ENABLE_Config[3]={0xC6,0x00,0x00};
uint8_t ERASE_CMD[4]={0x0E,0x01,0x00,0x00};
uint8_t READ_STATUS_REG[4]={0x3C,0x00,0x00,0x00};
uint8_t LSC_BITSTREAM_BURST[4]={0x7A,0x00,0x00,0x00};
uint8_t DISABLE_CMD[3]={0x26,0x00,0x00};
uint8_t NO_OP_CMD[4]={0xFF,0xFF,0xFF,0xFF};

unsigned int numBytes =0;

spi_device_handle_t spi3;
FILE *file;

typedef enum{
    READ_ID,
    REFRESH,
    ENABLE_CFG,
    ERASE,
    READ_STATUS,
    LSC_BITSTREAM,
    DISABLE,
    NO_OP
}COMMAND_TYPE;

void transmitData(COMMAND_TYPE cmd)
{
    uint8_t tx_data[2] = { 0x0F, 0x03 };

    spi_transaction_t t = {
        .tx_buffer = tx_data,
        .length = 2 * 8
    };

    switch(cmd)
    {
        case READ_ID:
            t.tx_buffer = READ_ID_CMD;
            t.length = 4*8;
        break;
        case REFRESH:
            t.tx_buffer = REFRESH_CMD;
            t.length = 3*8;
        break;
        case ENABLE_CFG:
            t.tx_buffer = ENABLE_Config;
            t.length = 3*8;
        break;
        case ERASE:
            t.tx_buffer = ERASE_CMD;
            t.length = 4*8;
        break;
        case READ_STATUS:
            t.tx_buffer = READ_STATUS_REG;
            t.length = 4*8;
        break;
        case LSC_BITSTREAM:
            t.tx_buffer = LSC_BITSTREAM_BURST;
            t.length = 4*8;
        break;
        case DISABLE:
            t.tx_buffer = DISABLE_CMD;
            t.length = 3*8;
        break;
        case NO_OP:
            t.tx_buffer = NO_OP_CMD;
            t.length = 4*8;
        break;
    }

    //ESP_ERROR_CHECK(spi_device_acquire_bus(spi3, portMAX_DELAY));
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3, &t));
    //spi_device_release_bus(spi3);
}

int fpga_init()
{
    esp_err_t ret;


    spi_device_interface_config_t devcfg={
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 1000000,  // 100 kHz
        .mode = 0,                  //SPI mode 0
        .spics_io_num = -1,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi3);
    ESP_ERROR_CHECK(ret);

    if(ret!= ESP_OK)
    {
        ESP_LOGI("SPI-Load", "Some error in SPI Configuration");
        return -1;
    }


    file = fopen("/spiffs/fpga.bin" , "r");

    if(file == NULL)
    {


        printf("FPGA Bin File does not exist");
        return -1;
    }
    else
    {
        printf("FPGA Bin file found.. \n");
    }

    return 0;
}


///////////////FUNCTIONS///////////////
#ifdef DEBUG_INFO
void read_Status_Register(void)
{

    ESP_LOGI("Status Reg","Read_Status_Register_CMD to FPGA");

    transmitData(READ_STATUS);
    spi_transaction_t t = {
        .cmd = 0x300,
        .rx_buffer = Status_Register,
        .rxlength = 4 * 8
    };

    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3, &t));

    ESP_LOGI("STATUS REG:","%x %x %x %x",Status_Register[0],Status_Register[1],Status_Register[2],Status_Register[3]);
}
#endif

void bitstreamBurst(void)
{
    numBytes = 0;

    char one_byte[16] = {0};
    spi_transaction_t t = {
        .tx_buffer = one_byte,
        .length = 16 * 8
    };

    FILE *tmpf;
    tmpf = fopen("/spiffs/fpga.bin","a");
    long fsize = tmpf->_offset;
    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Size: %lu", fsize);
    #endif

    while (1)
    {
        char a = fgetc(file);
        numBytes = numBytes+1;
        t.tx_buffer = &a;
        t.length = 1 * 8;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi3, &t));

        if (numBytes >= fsize) break;

    }

    fclose(file);
    fclose(tmpf);
    //esp_vfs_spiffs_unregister(NULL);
}

void fpga_load()
{
    /////////FPGA CONFIG MODE////////
    ESP_LOGI(TAG,"CBV2:FPGA load start");
    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Sending REFRESH Comand");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    vTaskDelay(100/portTICK_PERIOD_MS);
    transmitData(REFRESH);
    gpio_set_level(PIN_NUM_CS_LOAD, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    while(gpio_get_level(PIN_NUM_INITN))
    {
        #ifdef DEBUG_INFO
        ESP_LOGI("Init FPGA","Waiting for INIT_N pin to go LOW!");
        #endif
        break;
    }
    while(!gpio_get_level(PIN_NUM_INITN))
    {
        #ifdef DEBUG_INFO
        ESP_LOGI("Init FPGA","Waiting for INIT_N pin to go HIGH!");
        #endif
    }
    #ifdef DEBUG_INFO
    ESP_LOGI("Config","FPGA in Config Mode");
    #endif
    vTaskDelay(20/portTICK_PERIOD_MS); //was100
    ////////////////////////////////////////

    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Reading Status Register");
    gpio_set_level(PIN_NUM_CS, 0);
    read_Status_Register();
    gpio_set_level(PIN_NUM_CS, 1);

    vTaskDelay(1/portTICK_PERIOD_MS);
    #endif

    //////////////READ FPGA ID////////////
    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Reading FPGA ID");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    #ifdef DEBUG_INFO
    ESP_LOGI("Read ID FPGA","Reading ID of FPGA");
    #endif
    transmitData(READ_ID);
    spi_transaction_t t = {
        .cmd = 0x300,
        .rx_buffer = dest_addr,
        .rxlength = 4 * 8
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi3, &t));

    #ifdef DEBUG_INFO
    ESP_LOGI("FPGA ID:","%x %x %x %x",dest_addr[0],dest_addr[1],dest_addr[2],dest_addr[3]);
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 1);

    vTaskDelay(1 / portTICK_PERIOD_MS);
    ///////////////////////////////////////////////

    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Sending EN CFG Comand");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    transmitData(ENABLE_CFG);
    gpio_set_level(PIN_NUM_CS_LOAD, 1);

    vTaskDelay(1 / portTICK_PERIOD_MS);

    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Sending ERASE Comand");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    transmitData(ERASE);
    gpio_set_level(PIN_NUM_CS_LOAD, 1);

    vTaskDelay(10 / portTICK_PERIOD_MS); //was1000

    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Sending command LCSC Bitstream");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    transmitData(LSC_BITSTREAM);

    /////////////Send Burst Stream from file/////////////
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    bitstreamBurst();
    gpio_set_level(PIN_NUM_CS_LOAD, 1);

    ESP_LOGI("Bytes Send : " , "%d", (unsigned int)numBytes);
    /////////////////////////////////////////////////////


    vTaskDelay(1/portTICK_PERIOD_MS);

    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Now sending ISC Disable");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    transmitData(DISABLE);
    gpio_set_level(PIN_NUM_CS_LOAD, 1);

    vTaskDelay(1/portTICK_PERIOD_MS);

    #ifdef DEBUG_INFO
    ESP_LOGI(TAG,"Now sending NO_OP");
    #endif
    gpio_set_level(PIN_NUM_CS_LOAD, 0);
    transmitData(NO_OP);
    gpio_set_level(PIN_NUM_CS_LOAD, 1);


    #ifdef DEBUG_INFO
    ESP_LOGI(TAG, "Configuration finished!");
    #endif


    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (gpio_get_level(PIN_NUM_DONE))
    {
            #ifdef DEBUG_INFO
            ESP_LOGI(TAG, "Oops! Done is High");
            #endif
    }
    else
    {
            #ifdef DEBUG_INFO
            ESP_LOGI(TAG, "Great! Done is Low");
            #endif
    }
    ESP_LOGI(TAG,"CBV2:FPGA load finished");

    
    esp_err_t ret;
    ret = spi_bus_remove_device(spi3);
    ESP_ERROR_CHECK(ret);

   if(ret!= ESP_OK)
   {
        printf("SPI_LOAD :SPI Device removed after FPGA configuration \n");
   }
}

void SPI_LOAD_FPGA()
{
    printf("Initializing FPGA IOs \n");
    if (fpga_init()==0)
    {
        printf("FPGA Init OK \n");
        fpga_load();
    }
    
    printf("FPGA LOAD PROCESS COMPLETED \n");
}