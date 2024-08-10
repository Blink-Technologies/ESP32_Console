#ifndef _HELP_H_
#define _HELP_H_

#ifdef __cplusplus
extern "C" {
#endif


const char* help_str = "\n ********Welcome to ESP32-C6 Help Menu*********** \n \
-------------------------------------------------------------------------- \n \
Please note that all paramters must be passed in HEX Format \n \
---------------------------------------------------------------------------\n \
--> SPI_LOAD : Used to load bit file into the fpga \n \
               Usage : spi_load \n \
-------------------------------------------------------------------------- \n \
--> SPI_SEND : Used for SPI Communication \n \
    Enable :            spi_send 0x06 \n \
    Disable :           spi_send 0x04 \n \
    Write GPO :         spi_send 0x01 AddressByte DataByte \n \
    Latch GPI :         spi_send 0x03 DataByte \n \
    Read GPI :          spi_send 0x05 AddressByte \n \
    Write Memory :      spi_send 0x02 AddressByte Bytes2Write DataByte-1 DataByte-n .. \n \
    Read Memory :       spi_send 0x0B AddressByte Bytes2Read \n \
    IRQ Enable Write :  spi_send 0x66 DataByte \n \
    IRQ Enable Read :   spi_send 0x6A \n \
    IRQ Status :        spi_send 0x65 \n \
    IRQ Clear :         spi_send 0x65 DataByte \n \
    Revision ID :       spi_send 0x9F \n \
    Init SPI :          spi_init Clock \n \
------------------------------------------------------------------------------ \n \
--> I2C_SEND : Used for I2C Communication \n \
    Probe :             i2c_send 0x00 \n \
    Read Bytes :        i2c_send 0x01 SlaveAddress Bytes2Read \n \
    Write Byte :        i2c_send 0x02 SlaveAddress DataByte \n \
    Write then Read :   i2c_send 0x03 DataByte Bytes2Read \n \
    Init I2C :          i2c_init Clock \n \
-------------------------------------------------------------------------------- \n \
--> Console Special Commands \n \
    Clear Screen :      clear \n \
    Get Help :          help \n \
    Console Special Keys: \n \
    BackSpace Key : 0x7F            : Erases Character \n \
    Up Key : 0x7F : 0x1B 0x5B 0x41  : Previous Command \n \
--------------------------------------------------------------------------------- \n \
";

#ifdef __cplusplus
}
#endif

#endif