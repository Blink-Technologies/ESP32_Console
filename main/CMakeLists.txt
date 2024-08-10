idf_component_register(SRCS "i2s_audio.c" "spi_send.c" "i2c_send.c" "spi_load.c" "esp_prog_main.c"
                       INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
spiffs_create_partition_image(sound ../spiffs_sound FLASH_IN_PROJECT)
