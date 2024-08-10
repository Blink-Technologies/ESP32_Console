
#include "i2c_send.h"

uint32_t I2C_FREQ = 10000;

void i2c_init() 
{
    esp_err_t ret;


    i2c_master_bus_config_t i2c_mst_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle);

    printf("I2C Bus : %d \n", ret);
}

void I2C_Probe()
{
    esp_err_t ret;

    for (int i=0; i<127; i++)
    {
        ret = i2c_master_probe(i2c_bus_handle, i, TIMEOUT_MS_I2C/portTICK_PERIOD_MS);

        if (ret == ESP_OK)
        {
            printf("I2C Slave Found on Address : 0x%x \n", i);
        }

        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

void I2C_Transaction_WriteRead(uint8_t slave_addr, uint8_t writeSize, uint8_t readSize)
{
    esp_err_t ret;

    i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = slave_addr,
    .scl_speed_hz = I2C_FREQ,
    };

    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle);
    i2c_master_transmit_receive(dev_handle, i2c_write_data_buff, writeSize, i2c_read_data_buff, readSize, -1);    
    i2c_master_bus_rm_device(dev_handle);
}

void I2C_Transaction_Read(uint8_t slave_addr, uint8_t readSize)
{

    esp_err_t ret;

    i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = slave_addr,
    .scl_speed_hz = I2C_FREQ,
    };

    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle);
    i2c_master_receive(dev_handle, i2c_read_data_buff, readSize, -1);
    i2c_master_bus_rm_device(dev_handle);

}

void I2C_Transaction_Write(uint8_t slave_addr, uint8_t writeSize)
{
    esp_err_t ret;

    i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = slave_addr,
    .scl_speed_hz = I2C_FREQ,
    };

    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle);
    i2c_master_transmit(dev_handle, i2c_write_data_buff, writeSize, -1);
    i2c_master_bus_rm_device(dev_handle);

}

void I2C_SEND()
{
    uint8_t cmd_type =0; 
    uint8_t slave_addr =0;
    uint8_t readSize =0; 
    uint8_t writeSize =0;
    uint8_t addr_write=0;
    uint8_t data =0; 
 

    
        if (Num_Params == 1)
        {
            printf("I2C_SEND parameters missing .... \n");
            return;
        }

        cmd_type = HexStr2Decimal((const char*) PARAM_ARRAY[1]);

        switch (cmd_type)
        {
            case 0 : //Probe
                         I2C_Probe();
                         printf("I2C_SEND : Probe \n");  
            break;
            case 1 : //Read
                         
                        slave_addr =  HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        readSize =  HexStr2Decimal((const char*) PARAM_ARRAY[3]);

                         I2C_Transaction_Read(slave_addr, readSize);
                         printf("I2C_SEND : Read -> ");
                         for (int i=0; i<readSize; i++) printf("0x%x ", i2c_read_data_buff[i]);
                         printf("\n");  
            break;
            case 2 : //Write
                        slave_addr =  HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        i2c_write_data_buff[0] =  HexStr2Decimal((const char*) PARAM_ARRAY[3]);
                        //i2c_write_data_buff[1] =  HexStr2Decimal((const char*) PARAM_ARRAY[4]);
                        I2C_Transaction_Write(slave_addr,1);
                        printf("I2C_SEND : Write \n");  
            break;
            case 3 : //Write and Read
                        slave_addr =  HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        data =  HexStr2Decimal((const char*) PARAM_ARRAY[3]);
                        readSize =  HexStr2Decimal((const char*) PARAM_ARRAY[4]);   

                        i2c_write_data_buff[0] = data;

                        I2C_Transaction_WriteRead(slave_addr,1,readSize);

                        printf("I2C_SEND : WriteRead -> ");
                        for (int i=0; i<readSize; i++) printf("0x%x ", i2c_read_data_buff[i]);
                        printf("\n");  
            break;
            case 4 : //Write Command Word
                        slave_addr =  HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        i2c_write_data_buff[0] =  0;
                        i2c_write_data_buff[1] =  HexStr2Decimal((const char*) PARAM_ARRAY[3]);
                        I2C_Transaction_Write(slave_addr,2);
                        printf("I2C_SEND : Write Command Word\n");  
            break;			
			case 5 : //Write Data Word
                        slave_addr =  HexStr2Decimal((const char*) PARAM_ARRAY[2]);
                        i2c_write_data_buff[0] =  0x40;
                        i2c_write_data_buff[1] =  HexStr2Decimal((const char*) PARAM_ARRAY[3]);
                        I2C_Transaction_Write(slave_addr,2);
                        printf("I2C_SEND : Write Data Word\n");  
            break;			
			case 6 : //Write Multiple Data Words
                        slave_addr =  HexStr2Decimal((const char*) PARAM_ARRAY[2]);
						writeSize =  HexStr2Decimal((const char*) PARAM_ARRAY[3]);
                        i2c_write_data_buff[0] =  0x40;
						for (int i=0; i<writeSize; i++)
						{
							i2c_write_data_buff[i+1] = HexStr2Decimal((const char*) PARAM_ARRAY[4+i]);
						}
						
                        I2C_Transaction_Write(slave_addr,writeSize+1);
                        printf("I2C_SEND : Write Burst Data Word\n");  
            break;			

        }
}