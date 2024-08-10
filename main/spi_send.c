
#include "spi_send.h"

void spi_init(int clk_speed) 
{
    esp_err_t ret;


    printf("SPI Clock Speed Recieved : %d Hz \n", (int)clk_speed);
    
    if (dev_spi != NULL)
    {
        //First Remove Device
        ret = spi_bus_remove_device(dev_spi);
        ESP_ERROR_CHECK(ret);
        if(ret == ESP_OK)
        {
            printf("Previous SPI Device removed \n");
        }
        else
        {
            printf("Error in removing previous SPI Device \n");
        }
    }


    spi_device_interface_config_t devcfg={
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = clk_speed,  // 100 Khz
        .mode = 0,                  //SPI mode 0
        .spics_io_num = PIN_NUM_CS_USER,     
        .queue_size = 1,
        .flags = NULL,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &dev_spi);
    ESP_ERROR_CHECK(ret);

    if(ret != ESP_OK)
    {
        printf("Error Initializing SPI-DEVICE \n");
    }
    else {
        printf("SPI-DEV Initialized Successfully \n");

    }

    printf("SPI Initialized Successfully \n");



    // Getting Actual Frequency
    /*
    int freq_khz;
    ret = spi_device_get_actual_freq(&dev_spi, &freq_khz);

    if (ret == ESP_OK)
    {
        printf("Actual Freq of SPI is %d Khz \n", freq_khz);
    }
    else
    {
        printf("Error getting actual spi frequency \n");
    }
    */
} 


void SPI_Transaction_Transmit(uint8_t spi_length)
{
    //gpio_set_level(SPI_CS, 0);
    //vTaskDelay(1 / portTICK_PERIOD_MS);

    spi_transaction_t t = {
        .tx_buffer = spi_tx_data,
        .rx_buffer = spi_rx_data,
        .length = spi_length * 8
    };

    ESP_ERROR_CHECK(spi_device_polling_transmit(dev_spi, &t));

   
    //vTaskDelay(1 / portTICK_PERIOD_MS);
    //gpio_set_level(SPI_CS, 1);

}

void SPI_SEND()
{
        uint8_t cmd_type =0; 
        uint8_t spi_addr =0;
        uint8_t spi_read =0;
        uint8_t spi_write =0;
        uint8_t spi_length =0;

        if (Num_Params == 1)
        {
            printf("SPI_SEND parameters missing .... \n");
            return;
        }

        cmd_type = HexStr2Decimal((const char*) PARAM_ARRAY[1]);
        printf("Command Type : %d \n", cmd_type);

        switch (cmd_type)
        {
            case 0x06 : //Enable
                        spi_tx_data[0] = 0x06;
                        spi_tx_data[1] = 0;
                        spi_tx_data[2] = 0;
                        spi_tx_data[3] = 0;
                        spi_length = 1;

                        SPI_Transaction_Transmit(spi_length);

                        printf("SPI_SEND : Enable \n");   
            break;
            case 0x04 : //Disable
                        spi_tx_data[0] = 0x04;
                        spi_tx_data[1] = 0;
                        spi_tx_data[2] = 0;
                        spi_tx_data[3] = 0;
                        spi_length = 1;

                        SPI_Transaction_Transmit(spi_length);

                        printf("SPI_SEND : Disable \n");   
            break;
            case 0x01 : //Write GPO
                        if (Num_Params != 4)
                        {
                            printf("SPI_SEND : Write GPO -> Invalid Paramaters \n");  
                        }

                        spi_addr = HexStr2Decimal((const char*) PARAM_ARRAY[2]); 
                        spi_write = HexStr2Decimal((const char*) PARAM_ARRAY[3]); 

                        spi_tx_data[0] = 0x01;
                        spi_tx_data[1] = spi_addr;
                        spi_tx_data[2] = spi_write;
                        spi_tx_data[3] = 0;
                        spi_length = 3;

                        SPI_Transaction_Transmit(spi_length);
                        
                        printf("SPI_SEND : Write GPO -> %d -> %d \n", spi_addr, spi_write);   
            break;
            
            case 0x03 : //LATCH GPI
                        if (Num_Params != 3)
                        {
                            printf("SPI_SEND : LATCH GPI -> Invalid Paramaters \n");  
                        }
                        else
                        {
                            spi_write = HexStr2Decimal((const char*) PARAM_ARRAY[2]);         

                            spi_tx_data[0] = 0x03;
                            spi_tx_data[1] = spi_write;
                            spi_tx_data[2] = 0;
                            spi_tx_data[3] = 0;
                            spi_length = 2;

                             SPI_Transaction_Transmit(spi_length);
                            printf("SPI_SEND : LATCH GPI -> %d \n", spi_write);   
                        }
                      
            break;

            case 0x05 : //READ GPI
                        if (Num_Params != 3)
                        {
                            printf("SPI_SEND : READ GPI -> Invalid Paramaters \n");  
                        }
                        else
                        {
                            spi_addr = HexStr2Decimal((const char*) PARAM_ARRAY[2]);

                            spi_tx_data[0] = 0x05;
                            spi_tx_data[1] = spi_addr;
                            spi_tx_data[2] = DUMMY_BYTE;
                            spi_tx_data[3] = 0;
                            spi_length = 4;

                            SPI_Transaction_Transmit(spi_length);         
                            printf("SPI_SEND : READ GPI -> %x \n", spi_rx_data[3]);   
                        }
                        
            break;

            case 0x02 : //Write Memory
                        if (Num_Params < 5)
                        {
                            printf("SPI_SEND : Write Memory-> Invalid Paramaters \n");
                            return;  
                        }
                        
                        spi_addr = HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        spi_length = HexStr2Decimal((const char*) PARAM_ARRAY[3]);

                        if (spi_length > 16)  
                        {
                            printf("SPI_SEND : Write Memory : Invalid Burst Length \n");
                            return;
                        }
    
                        if (Num_Params < (4+spi_length))
                        {
                            printf("SPI_SEND : Write Memory : Invalid Burst Paramaters \n");
                            return;
                        }


                        spi_tx_data[0] = 0x02;
                        spi_tx_data[1] = spi_addr;

                        for (int i=0; i<spi_length; i++)
                        {
                            spi_tx_data[2+i] = HexStr2Decimal((const char*) PARAM_ARRAY[4+i]);
                              
                        }

                        spi_length = spi_length + 2; 
                        SPI_Transaction_Transmit(spi_length);               
                        printf("SPI_SEND : Write Memory \n");   
                        
                        
            break;

            case 0x0B : //READ Memory
                        if (Num_Params != 4)
                        {
                            printf("SPI_SEND : READ Memory -> Invalid Paramaters \n");
                            return;  
                        }
                       
                        spi_addr = HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        spi_length = HexStr2Decimal((const char*) PARAM_ARRAY[3]);

                        if (spi_length > 16)
                        {
                            printf("SPI_SEND : READ Memory -> Invalid Read Length \n");
                            return;  
                        }

                        spi_tx_data[0] = 0x0B;
                        spi_tx_data[1] = spi_addr;
                        spi_tx_data[2] = DUMMY_BYTE;

                        for (int i=0; i<spi_length; i++)
                        {
                            spi_tx_data[3+i] = 0;
                        }    
                       
                        spi_length = 3 + spi_length;
                        SPI_Transaction_Transmit(spi_length);     
                        printf("SPI_SEND : Read Memory-> ");
                        for (int i=0; i<spi_length; i++) printf("0x%x ", spi_rx_data[3+i]);
                        printf("\n");  
                        
            break;
           
            case 0x66 : //IRQ_EN_WRITE
                        if (Num_Params != 3)
                        {
                            printf("SPI_SEND : IRQ_EN_WRITE -> Invalid Paramaters \n");  
                        }
                        else
                        {
                            spi_write = HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                            spi_tx_data[0] = 0x66;
                            spi_tx_data[1] = spi_write;
                            spi_tx_data[2] = 0;
                            spi_tx_data[3] = 0;
                            spi_length = 2;

                            SPI_Transaction_Transmit(spi_length);
                            printf("SPI_SEND : IRQ_EN_WRITE \n");   
                        }
                        
            break;

            case 0x6A : //IRQ_EN_READ
                            spi_tx_data[0] = 0x6A;
                            spi_tx_data[1] = DUMMY_BYTE;
                            spi_tx_data[2] = 0;
                            spi_tx_data[3] = 0;
                            spi_length = 3;
                            SPI_Transaction_Transmit(spi_length);     
                            printf("SPI_SEND : IRQ_Enable_Read -> %x \n", spi_rx_data[2]);  
                        
            break;
           
            case 0x65 : //IRQ_STATUS
                            spi_tx_data[0] = 0x65;
                            spi_tx_data[1] = DUMMY_BYTE;
                            spi_tx_data[2] = 0;
                            spi_tx_data[3] = 0;
                            spi_length = 3;
                            SPI_Transaction_Transmit(spi_length);       
                            printf("SPI_SEND : IRQ_STATUS -> %x \n", spi_rx_data[2]);           
            break;
           
            case 0x61 : //IRQ_CLEAR
                        if (Num_Params != 3)
                        {
                            printf("SPI_SEND : IRQ_CLEAR -> Invalid Paramaters \n");  
                        }
                        else
                        {
                            spi_write = HexStr2Decimal((const char*) PARAM_ARRAY[2]);   
                            spi_tx_data[0] = 0x61;
                            spi_tx_data[1] = spi_write;
                            spi_tx_data[2] = 0;
                            spi_tx_data[3] = 0;
                            spi_length = 2;

                            SPI_Transaction_Transmit(spi_length);

                            printf("SPI_SEND : IRQ_CLEAR \n");   
                        }
                        
            break;
            
            case 0x9F : //REVISION_ID
                        spi_tx_data[0] = 0x9F;
                        spi_tx_data[1] = DUMMY_BYTE;
                        spi_tx_data[2] = 0;
                        spi_tx_data[3] = 0;
                        spi_length = 3;

                        SPI_Transaction_Transmit(spi_length); 

                        printf("SPI_SEND : Revision ID -> %x \n", spi_rx_data[2]);   
                        
            break;  

            default :
                       printf("SPI_SEND : Inavlid Command \n");   
            break;
        }


}
