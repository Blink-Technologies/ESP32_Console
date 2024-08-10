#include "myHeaders.h"
#include "spi_load.h"
#include "spi_send.h"
#include "i2c_send.h"
#include "help.h"
#include "i2s_audio.h"
#include <ff.h>
#include <dirent.h>

spi_device_handle_t dev_spi;
i2c_master_bus_handle_t i2c_bus_handle;

uint8_t spi_tx_data[32] = {0};
uint8_t spi_rx_data[32] = {0};
uint8_t i2c_read_data_buff[128] = {0};
uint8_t i2c_write_data_buff[128] = {0};
uint8_t *PARAM_ARRAY[MAX_PARAMS];
uint8_t Num_Params =0;
uint8_t myState=0;

uint8_t SPI_LOAD_TRIGGER =0;

circ_bbuf_t cmd_buffer;

char* COMMAND_SPI = "spi_send";
char* COMMAND_I2C = "i2c_send";
char* COMMAND_GPIO = "gpio_send";
char* COMMAND_HELP = "help";
char* COMMAND_LOAD = "spi_load";
char* COMMAND_SPI_INIT = "spi_init";
char* COMMAND_I2C_INIT = "i2c_init";
char* COMMAND_CLEAR = "clear";
char* COMMAND_WAV = "wav_play";
char* COMMAND_MP3 = "mp3_play";

uint8_t data[UART_BUFFER_SIZE] = {0};
uint8_t command[UART_BUFFER_SIZE] = {0};
uint8_t command_temp[UART_BUFFER_SIZE] = {0};
uint8_t *command_history[MAX_COMMAND_HISTORY_LEN]; 
uint8_t  command_history_len[MAX_COMMAND_HISTORY_LEN] = {0}; 
int cmd_push_pointer =0;
int cmd_pop_pointer =0;
int flag_prev_cmd =0;

spi_bus_config_t spibuscfg;

void Init_Console()
{
/* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 10, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)); 

    // Configure a temporary buffer for the incoming data
    
    for (int i=0; i<MAX_COMMAND_HISTORY_LEN; i++)
    {
        command_history[i] = calloc(UART_BUFFER_SIZE, sizeof(uint8_t));
    }

}

void echo_task(void *parameters)
{
    
    Init_Console();

    
    uint8_t s_char =0;
    uint8_t len_command =0;
    uint8_t flag_command =0;
    

    while (1) 
    {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, UART_BUFFER_SIZE, 20 / portTICK_PERIOD_MS);
        //uart_write_bytes(UART_NUM_0, (const char *) data, len);

        // First Check for Control Characters
        if (len == 3)
        {
                //UP =  1B 5B 41
                //DOWN = 1B 5B 42

            if (data[0]==0x1B && data[1]==0x5B && data[2]==0x41)
            {
                //UP Key
                printf("\n");
                cmd_buffer_pop(command, &len_command, 0);
                uart_write_bytes(UART_NUM_0, (const char *) command, len_command);
                
                flag_prev_cmd =1;

            } 
        }

        else 
        {
            uart_write_bytes(UART_NUM_0, (const char *) data, len);

        flag_command =0;

        for (int i=0; i<len; i++)
        {
            s_char  = data[i]; 

            if (s_char > 0 && s_char < 0x80)
            {
            
                if (s_char == 0x0D)
                {
                    flag_command =1;
                    uart_flush(UART_NUM_0);
                    break;
                }
                else if (s_char == 0x7F)
                {
                    // This is backspace
                    if (len_command>0)
                    {
                        len_command--;
                        command[len_command] =0;
                    }

                    flag_prev_cmd =0;
                }
                else
                {
                    command[len_command] = s_char;
                    len_command++;
                    flag_prev_cmd =0;
                }

                flag_command =0;
            }
        }
    }


    if (flag_command==1 && len_command > 0)
    {
        if (flag_prev_cmd ==0) 
        {           
            cmd_buffer_push(command, len_command);
        }
        printf("%s \n", (char*)command);
        printf("Length Command : %d \n", len_command);

        ExtractParameters(command, len_command);
        Process_Commands();

        for (int i=0; i<UART_BUFFER_SIZE; i++) command[i] =0;

        flag_command =0;
        len_command =0;
    }

    }
}

void cmd_buffer_push(uint8_t * cmd, uint8_t len)
{
    //printf("Push : ");
    //for (int i=0; i<len; i++)
    //printf("%x ", cmd[i]);
    //printf("Push Ended \n");

    for (int i=0; i<UART_BUFFER_SIZE; i++)
    {
        command_history[cmd_push_pointer][i] =0;
    }

    for (int i=0; i<UART_BUFFER_SIZE; i++)
    {
        command_history[cmd_push_pointer][i] = cmd[i];
    }
       command_history_len[cmd_push_pointer] = len;

    cmd_push_pointer++;
    if (cmd_push_pointer ==  MAX_COMMAND_HISTORY_LEN) cmd_push_pointer =0;
    
    cmd_pop_pointer = cmd_push_pointer;

}

void cmd_buffer_pop(uint8_t *cmd,  uint8_t *len, uint8_t dir)
{

    for (int i=0; i<UART_BUFFER_SIZE; i++) cmd[i] =0;


    int pp = cmd_pop_pointer-1;
    if (pp<0) pp = MAX_COMMAND_HISTORY_LEN-1;

    for (int i=0; i<UART_BUFFER_SIZE; i++)
    {
        cmd[i] = command_history[pp][i];
    }

    *len = command_history_len[pp];

    // Next
    if (dir)
    {
        cmd_pop_pointer++;
        if (cmd_pop_pointer > MAX_COMMAND_HISTORY_LEN) cmd_pop_pointer =1;
    }
    else
    {
        cmd_pop_pointer--;
        if (cmd_pop_pointer < 1) cmd_pop_pointer = MAX_COMMAND_HISTORY_LEN;
    }

}

int ExtractParameters(uint8_t* command_word, uint8_t command_length)
{
    if (command_length == 0xFF) 
    {
        printh("Error : Command Length is greater than or equal to UART Buffer Size \r\n");
        return -1;
    }

    if (command_length < 1) 
    {
        printh("Error : Command Length < 1 \r\n");
        return -3;
    }
    
    int param_index =0;
    int jj=0;
    uint8_t cc =0;
    uint8_t ll = command_length;

    Init_Param_Array();
    myState = st_FindParameter;

    for (int i=0; i<ll; i++)
    {
        cc = command_word[i];

        switch (myState)
        {
            case st_FindParameter:
                                    if (cc == 0x20)
                                    {
                                        myState = st_FindParameter;
                                    }
                                    else
                                    {
                                        param_index++;
                                        PARAM_ARRAY[param_index-1][0] = cc;
                                        myState = st_FillParameter;
                                    }
            break;

            case st_FillParameter : 
                                   if (cc == 0x20)
                                    {
                                        jj=0;
                                        myState = st_FindParameter;

                                    }
                                    else
                                    {
                                        jj++;
                                        PARAM_ARRAY[param_index-1][jj] = cc;
                                        myState = st_FillParameter;
                                    }
            break;
        }
    }

    Num_Params = param_index;
    printf("Number of Params Recieved : %d \r\n", Num_Params);
    
    for (int i=0; i<Num_Params; i++)
    {
        printf("Param # %d :%s\r\n", i+1, PARAM_ARRAY[i]);
    }
    return 0;

    // Find the first Space 0x20


}

void Process_Commands()
{
    const char* cmd = (const char*)PARAM_ARRAY[0];

   
    if (strcmp(cmd,COMMAND_SPI)==0)
    {
        printf("SPI Command Recieved \n");
        SPI_SEND();
        printf("------------------------------------- \n");
    }
    else if (strcmp(cmd,COMMAND_I2C)==0)
    {
        printf("I2C Command Recieved \n");
        I2C_SEND();
        printf("------------------------------------- \n");

    }
    else if (strcmp(cmd,COMMAND_GPIO)==0)
    {
        printf("GPIO Command Recieved \n");
        printf("------------------------------------- \n");

    }
    else if (strcmp(cmd,COMMAND_HELP)==0)
    {
        printf("Help Command Recieved \n");
        printf("%s" , help_str);
        printf("------------------------------------- \n");
    }
    else if (strcmp(cmd,COMMAND_LOAD)==0)
    {
        printf("SPI Load Command Recieved \n");
        SPI_LOAD_TRIGGER =1;
        printf("------------------------------------- \n");

    }
    else if (strcmp(cmd, COMMAND_SPI_INIT) ==0)
    {
        printf("SPI INIT Command Recieved \n");
        if (Num_Params < 2)
        {
            printf("Initializing SPI with default clk i.e 1Mhz");
            spi_init(100000);
        }
        else
        {
            int spiclk = HexStr2Decimal32((const char*)PARAM_ARRAY[1]);
            spi_init(spiclk);
        }
        
        printf("------------------------------------- \n");
    }

    else if (strcmp(cmd, COMMAND_I2C_INIT) ==0)
    {
        printf("I2C INIT Command Recieved \n");
        if (Num_Params < 2)
        {
            printf("Invalid Paramaters");
            I2C_FREQ = 10000;
        }
        else
        {
            uint32_t i2c_f = HexStr2Decimal32((const char*)PARAM_ARRAY[1]);
            if (i2c_f <= 100000)
            {
                 I2C_FREQ = i2c_f;
                 printf("I2C Clock is set \n");
            }
            else
            {
                printf("Invalid I2C Clock Paramater \n");
            }
           
        }
        
        printf("------------------------------------- \n");
    }

    else if (strcmp(cmd, COMMAND_CLEAR) ==0)
    {
        printf("Clear Command Recieved \n");
        printf("\033c");
        printf("\n");

    }

    else if (strcmp(cmd, COMMAND_WAV) ==0)
    {
        printf("Play Wav Command Recieved \n");
        if (Num_Params < 3) 
        {
            printf("Invalid Paramaters \n");
        }
        else
        {
            int index = HexStr2Decimal((const char*)PARAM_ARRAY[1]);
            int vol = HexStr2Decimal((const char*)PARAM_ARRAY[2]);
            Play_Wav(index, vol);
        }
        
        printf("------------------------------------- \n");
    }

    else if (strcmp(cmd, COMMAND_MP3) ==0)
    {
        printf("Play MP3 Command Recieved \n");

        if (Num_Params < 3) 
        {
            printf("Invalid Paramaters \n");
        }
        else
        {
            int index = HexStr2Decimal((const char*)PARAM_ARRAY[1]);
            int vol = HexStr2Decimal((const char*)PARAM_ARRAY[2]);
            Play_MP3(index, vol);
        }
        
        printf("------------------------------------- \n");
    }

    else
    {
        printf("Invalid Command \n");
        printf("------------------------------------- \n");

    }

}

uint8_t HexStr2Decimal(const char* hexStr)
{
    uint8_t number = (int)strtol(hexStr, NULL, 0);
    return number;
}

uint32_t HexStr2Decimal32(const char* hexStr)
{
    uint32_t number = (uint32_t)strtol(hexStr, NULL, 0);
    return number;
}

void printh(char* str)
{
    uart_write_bytes(UART_NUM_0, (const char *) str, strlen((const char *)str));
}

void Init_Param_Array()
{
    Num_Params =0;
    for (int i=0; i<MAX_PARAMS; i++)
    {
        for (int j=0; j<PARAM_SIZE; j++)
        {
            PARAM_ARRAY[i][j] = 0;
        }
    }
}

void SPI_BUS_INIT()
{
    esp_err_t ret;

    spibuscfg.miso_io_num= PIN_NUM_MISO;
    spibuscfg.mosi_io_num= PIN_NUM_MOSI;
    spibuscfg.sclk_io_num= PIN_NUM_SCK;
    spibuscfg.quadwp_io_num= -1;
    spibuscfg.quadhd_io_num= -1;
    spibuscfg.max_transfer_sz= 32;

    ret = spi_bus_initialize(SPI2_HOST, &spibuscfg, SPI_DMA_CH_AUTO);

    ESP_ERROR_CHECK(ret);

    if (ret == ESP_OK)
    {
        printh("SPI Bus Initialized Success \n");
    }
    else
    {
        printh("SPI Bus Initialization Error \n");
    }
}

void SPIFFS_Init()
{
    esp_err_t ret;

     esp_vfs_spiffs_conf_t config1 = {
        .base_path = "/spiffs" ,
        .partition_label = "storage",
        .max_files = 10,
        .format_if_mount_failed = true,
    };
    ret = esp_vfs_spiffs_register(&config1);

    esp_vfs_spiffs_conf_t config2 = {
        .base_path = "/sound" ,
        .partition_label = "sound",
        .max_files = 20,
        .format_if_mount_failed = true,
    };
    ret = esp_vfs_spiffs_register(&config2);

     ESP_ERROR_CHECK(ret);

    if (ret == ESP_OK)
    {
        printh("SPIFFS Initialized Successfully \n");
    }
    else
    {
        printh("SPIFFS Initialization Error \n");
    }


    DIR* dir = opendir("/sound");
    if (dir == NULL) {
        return;
    }

    while (true) {
    struct dirent* de = readdir(dir);
    if (!de) {
        break;
    }
    
    printf("Found file: %s\n", de->d_name);
}

closedir(dir);
}

void GPIO_Init()
{

     //FPGA Related IOs    

    /* 
    gpio_reset_pin(PIN_NUM_INITN);
    gpio_reset_pin(PIN_NUM_DONE);
    gpio_reset_pin(PIN_NUM_CS_LOAD);

    gpio_set_direction(PIN_NUM_INITN, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_NUM_DONE, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_NUM_CS_LOAD,GPIO_MODE_OUTPUT);

    gpio_set_pull_mode(PIN_NUM_INITN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_DONE, GPIO_PULLUP_ONLY);

    gpio_set_level(PIN_NUM_CS_LOAD, 1);
    */
}

void app_main(void)
{

     for (int i=0; i< MAX_PARAMS; i++)
    {
        PARAM_ARRAY[i] = (uint8_t*) malloc(PARAM_SIZE);
    }

    //printf("Configuring All Peripherals .....\n");

    GPIO_Init();
    SPIFFS_Init();
    //SPI_BUS_INIT();
    //spi_init(100000);
    //i2c_init(); 
    init_i2s_audio();

    printf("Console is Ready.....\n");
    xTaskCreate(echo_task, "UART_ECHO_TASK",UART_ECHO_TASK_STACK_SIZE, NULL, 5, NULL);


    while(1)
    {
        if (SPI_LOAD_TRIGGER == 1)
        {
            SPI_LOAD_TRIGGER =0;
            SPI_LOAD_FPGA();
        }

        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}
