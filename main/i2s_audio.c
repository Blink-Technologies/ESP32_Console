
#include "i2s_audio.h"
#include <esp_log.h>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_STDIO
#include "minimp3.h"
#include <esp_task_wdt.h>


const int BUFFER_SIZE = 1024;
const int NUM_FRAMES_TO_SEND = 256;

static const char *TAG = "MP3";

int16_t *frames_buffer;

i2s_chan_handle_t i2s_tx_handle;
i2s_std_config_t tx_std_cfg;
i2s_chan_config_t tx_chan_cfg;

struct Params1
{
    int fileIndex;
    int Volume;
}SoundParams;

uint16_t process_sample(int16_t sample) { return sample; }


void play_mp3_task(void *args)
{

  printf("Inside MP3 Play task \n");

  int frame_index = 0;
  int frames_to_send = 0;

  struct Params1 FF = *(struct Params1*)args;
  int Vol = FF.Volume;
  int Index =  FF.fileIndex;

  float vol = FF.Volume/20.0;
  printf("Volume : %d %% \n", (int)(vol*100));
  printf("Index Passed : %d \n", Index);

  frames_buffer = (int16_t *)malloc(2 * sizeof(int16_t) * NUM_FRAMES_TO_SEND);

  // setup for the mp3 decoded
  short *pcm = (short *)malloc(sizeof(short) * MINIMP3_MAX_SAMPLES_PER_FRAME);
  uint8_t *input_buf = (uint8_t *)malloc(BUFFER_SIZE);
  if (!pcm)
  {
    ESP_LOGE("main", "Failed to allocate pcm memory");
  }
  if (!input_buf)
  {
    ESP_LOGE("main", "Failed to allocate input_buf memory");
  }
 
    // mp3 decoder state
    mp3dec_t mp3d = {};
    mp3dec_init(&mp3d);
    mp3dec_frame_info_t info = {};
    // keep track of how much data we have buffered, need to read and decoded
    int to_read = BUFFER_SIZE;
    int buffered = 0;
    int decoded = 0;
    bool is_output_started = false;


    // Setup for file name
    const char* basePath = "/sound/";
    const char* extension = ".mp3";
    char* fileIndex = malloc(3);
    itoa(FF.fileIndex, fileIndex, 10);
    char* fullPath;
    fullPath = malloc(strlen(basePath)+1+4+2); /* make space for the new string (should check the return value ...) */
    strcpy(fullPath, basePath); /* copy name into the new var */
    strcat(fullPath, fileIndex); /* add the extension */
    strcat(fullPath, extension); /* add the extension */
    
    printf("FullPath :%s \n", fullPath);

    FILE *fp = fopen(fullPath, "r");
    if (!fp)
    {
      ESP_LOGE("MP3", "Failed to open file");    
      
    }

    while (1)
    {

      //ESP_LOGE("MP3", "MP3 File Opened Suucessfully");
      // read in the data that is needed to top up the buffer
      size_t n = fread(input_buf + buffered, 1, to_read, fp);
      //ESP_LOGI("MP3", "N : %d \n", n);

      // feed the watchdog
      vTaskDelay(pdMS_TO_TICKS(1));
      //esp_task_wdt_reset();
      // ESP_LOGI("main", "Read %d bytes\n", n);
      buffered += n;
      if (buffered == 0)
      {
        // we've reached the end of the file and processed all the buffered data
        is_output_started = false;
        break;
      }
      // decode the next frame
      int samples = mp3dec_decode_frame(&mp3d, input_buf, buffered, pcm, &info);
      // we've processed this may bytes from teh buffered data
      buffered -= info.frame_bytes;
      // shift the remaining data to the front of the buffer
      memmove(input_buf, input_buf + info.frame_bytes, buffered);
      // we need to top up the buffer from the file
      to_read = info.frame_bytes;

      //ESP_LOGI("MP3", "Samples : %d \n", samples);

      if (samples > 0)
      {
        // if we haven't started the output yet we can do it now as we now know the sample rate and number of channels
        if (!is_output_started)
        {

         printf("MP3 INFO.HZ :  %d \n", info.hz);
         printf("MP3 INFO.BITRATE :  %d \n", info.bitrate_kbps);  
         printf("MP3 INFO.CHANNELS : %d \n", info.channels);  
         printf("MP3 INFO.FRAME_BYTES : %d \n", info.frame_bytes);  
         printf("MP3 INFO.FRAME_OFFSET : %d \n", info.frame_offset);  
         printf("MP3 INFO.LAYER : %d \n", info.layer);  
  

              i2s_std_clk_config_t clk_cfgg = {
                .sample_rate_hz = info.hz, \
                .clk_src = I2S_CLK_SRC_DEFAULT, \
                .mclk_multiple = I2S_MCLK_MULTIPLE_512, \
            };
            
            i2s_std_slot_config_t slot_cfgg = { \
                .data_bit_width = 16, \
                .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, \
                .slot_mode = 1, \
                .slot_mask = I2S_STD_SLOT_BOTH, \
                .ws_width = 16, \
                .ws_pol = false, \
                .bit_shift = false, \
                .left_align = true, \
                .big_endian = false, \
                .bit_order_lsb = false \
            };

            i2s_channel_reconfig_std_clock(i2s_tx_handle, &clk_cfgg);
            i2s_channel_reconfig_std_slot(i2s_tx_handle, &slot_cfgg);

            i2s_channel_enable(i2s_tx_handle);


             ESP_LOGI("MP3", "STD Clock and STD Slot Set");

          is_output_started = true;
        }
        // if we've decoded a frame of mono samples convert it to stereo by duplicating the left channel
        // we can do this in place as our samples buffer has enough space
        if (info.channels == 1)
        {
          /*
          for (int i = samples - 1; i >= 0; i--)
          {
            pcm[i * 2] = pcm[i];
            pcm[i * 2 - 1] = pcm[i];
          }
          */
        }

        //ESP_LOGI("MP3", "write the decoded samples to the I2S output");
        //output_write(pcm, samples);

        frame_index = 0;              // this will contain the prepared samples for sending to the I2S device
        while (frame_index < samples)
        {
          // fill up the frames buffer with the next NUM_FRAMES_TO_SEND frames
          frames_to_send = 0;
          for (int i = 0; i < NUM_FRAMES_TO_SEND && frame_index < samples; i++)
          {

            frames_buffer[i]  = process_sample(vol * (float)pcm[frame_index]);     
            //int left_sample = process_sample(pcm[frame_index * 2]);
            //int right_sample = process_sample(pcm[frame_index * 2 + 1]);
            //frames_buffer[i] = left_sample;
            //frames_buffer[i * 2 + 1] = right_sample;
            frames_to_send++;
            frame_index++;

            //ESP_LOGI("MP3", "Frames2Send : %d , FrameIndex : %d \n", frames_to_send, frame_index);

          }
          //ESP_LOGI("MP3", "write data to the i2s peripheral");
          size_t bytes_written = 0;
          size_t bytes2sent = frames_to_send * sizeof(int16_t);// * 2;
          //i2s_write(m_i2s_port, frames_buffer, frames_to_send * sizeof(int16_t) * 2, &bytes_written, portMAX_DELAY);
          i2s_channel_write(i2s_tx_handle, frames_buffer, bytes2sent , &bytes_written, portMAX_DELAY);
          //ESP_LOGI("MP3", "I2C WRITTEN");


          if (bytes_written != frames_to_send * sizeof(int16_t))
          {
            ESP_LOGE(TAG, "Did not write all bytes");
          }
  }
        // keep track of how many samples we've decoded
        decoded += samples;
      }
       //ESP_LOGI("MP3", "decoded %d samples\n", decoded);
    }
    ESP_LOGI("MP3", "Finished");
    free(frames_buffer);    
    free(fullPath);
    free(fileIndex);
    free(pcm);
    free(input_buf);
    fclose(fp);

    i2s_channel_disable(i2s_tx_handle);
    i2s_del_channel(i2s_tx_handle);
    //i2s_del_channel(i2s_tx_handle);
    ESP_LOGI("MP3", "Task Going to be Deleted");
    vTaskDelete(NULL);
}

void i2s_init_std_simplex(uint32_t ClockRate, uint8_t DataBits, uint8_t Channels)
{
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &i2s_tx_handle, NULL));


    i2s_std_clk_config_t clk_cfgg = {
        .sample_rate_hz = ClockRate, \
        .clk_src = I2S_CLK_SRC_DEFAULT, \
        .mclk_multiple = I2S_MCLK_MULTIPLE_256, \
    };


    i2s_std_slot_config_t slot_cfgg = { \
        .data_bit_width = DataBits, \
        .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, \
        .slot_mode = Channels, \
        .slot_mask = I2S_STD_SLOT_BOTH, \
        .ws_width = DataBits, \
        .ws_pol = false, \
        .bit_shift = false, \
        .left_align = true, \
        .big_endian = false, \
        .bit_order_lsb = false \
    };
   

      
        i2s_std_gpio_config_t gpio_cfgg = {
            .mclk = I2S_GPIO_UNUSED, 
            .bclk = I2S_BCLK,
            .ws   = I2S_LRC,
            .dout = I2S_DOUT,
            .din = I2S_GPIO_UNUSED, 
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        };
    
    tx_std_cfg.clk_cfg  = clk_cfgg;
    tx_std_cfg.slot_cfg = slot_cfgg;    
    tx_std_cfg.gpio_cfg = gpio_cfgg;
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_tx_handle, &tx_std_cfg));  
}

void i2s_write_spiffs(void *args)
{

    struct Params1 FF = *(struct Params1*)args;
    int Vol = FF.Volume;

    printf("I2S Task Created \n");
    printf("Index Passed : %d \n", FF.fileIndex);
     printf("Volume Passed : %d \n", Vol);


    const char* basePath = "/sound/";
    const char* extension = ".wav";
    char* fileIndex = malloc(3);
    itoa(FF.fileIndex, fileIndex, 10);
    char* fullPath;
    fullPath = malloc(strlen(basePath)+1+4+2); /* make space for the new string (should check the return value ...) */
    strcpy(fullPath, basePath); /* copy name into the new var */
    strcat(fullPath, fileIndex); /* add the extension */
    strcat(fullPath, extension); /* add the extension */
    
    printf("FullPath :%s \n", fullPath);
    

    uint8_t *buf = (uint8_t *)calloc(1, 2048);


    FILE *fh;
    fh = fopen(fullPath,"r");   

    if (fh == NULL)
    {
        printf("Failed to open wav file \n");
    }
    else
    {
        printf("Wave file found  ... \n");

        struct WaveHeader wave_header;  

        fread((uint8_t*)&wave_header , 1 , 44, fh);

        printf("Wave File Information \n");
        printf("Format : %s \n", wave_header.Format);
        printf("Channels : %d \n", wave_header.Channels);
        printf("BitsPerSample : %d \n", wave_header.BitsPerSample);
        printf("SampleRate : %d \n", (int)wave_header.SampleRate);

        
            i2s_std_clk_config_t clk_cfgg = {
                .sample_rate_hz = wave_header.SampleRate, \
                .clk_src = I2S_CLK_SRC_DEFAULT, \
                .mclk_multiple = I2S_MCLK_MULTIPLE_256, \
            };

            
            i2s_std_slot_config_t slot_cfgg = { \
                .data_bit_width = wave_header.BitsPerSample, \
                .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, \
                .slot_mode = wave_header.Channels, \
                .slot_mask = I2S_STD_SLOT_BOTH, \
                .ws_width = wave_header.BitsPerSample, \
                .ws_pol = false, \
                .bit_shift = false, \
                .left_align = true, \
                .big_endian = false, \
                .bit_order_lsb = false \
            };

            i2s_channel_reconfig_std_clock(i2s_tx_handle, &clk_cfgg);
            i2s_channel_reconfig_std_slot(i2s_tx_handle, &slot_cfgg);



        fseek(fh, 44, SEEK_SET);

        // create a writer buffer
        size_t bytes_read = 0;
        size_t bytes_written = 0;

        bytes_read = fread(buf, 1 , 2048, fh);

        i2s_channel_enable(i2s_tx_handle);

        while (bytes_read > 0)
        {
        
            i2s_channel_write(i2s_tx_handle, buf, bytes_read , &bytes_written, -1);
            bytes_read = fread(buf, 1, 2048, fh);
        }


            printf("Audio Completed \n");
    }

  printf("Task Deleted \n");

  free(fullPath);
  free(fileIndex);
  fclose(fh);
  free(buf);
  i2s_channel_disable(i2s_tx_handle);
  i2s_del_channel(i2s_tx_handle);
  vTaskDelete(NULL);

}


void Play_Wav(int index, int vol)
{
    SoundParams.fileIndex = index;
    SoundParams.Volume = vol;
    i2s_init_std_simplex(44100, 16, 1);

    xTaskCreate(i2s_write_spiffs, "i2s_write_spiffs", 4096, (void *)&SoundParams, 1, NULL);

}


void Play_MP3(int index, int vol)
{
    SoundParams.fileIndex = index;
    SoundParams.Volume = vol;
    i2s_init_std_simplex(44100, 16, 1);

    //xTaskCreatePinnedToCore(play_mp3_task, "play_mp3_task", 52768, (void *)&SoundParams, 1, NULL, 1);
    xTaskCreate(play_mp3_task, "play_mp3_task", 52768, (void *)&SoundParams, 1, NULL);
}


void init_i2s_audio()
{
    printf("I2S Code ... \n");

    gpio_set_direction(I2S_SD, GPIO_MODE_OUTPUT);

    gpio_set_level(I2S_SD, 1);

}