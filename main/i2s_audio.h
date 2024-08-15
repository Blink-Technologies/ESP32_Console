#ifndef _I2S_AUDIO_H_
#define _I2S_AUDIO_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "myHeaders.h"
#include <driver/i2s_std.h>

#define I2S_LRC 21
#define I2S_BCLK 22
#define I2S_DOUT 23
#define I2S_SD 20

#define I2S_BUFF_SIZE 1024
#define I2S_CLOCK_FREQ 48000

#define HEADER_SIZE 44

    struct WaveHeader
    {
        char ID1[4];
        uint32_t Size1;
        char Format[4];
        char ID2[4];
        uint32_t Size2;
        uint16_t Format2;
        uint16_t Channels;
        uint32_t SampleRate;
        uint32_t ByteRate;
        uint16_t BlockAlign;
        uint16_t BitsPerSample;
        char ID3[4];
        uint32_t DataSize;
    };

    typedef unsigned char byte;

    void init_i2s_audio();
    void Play_Wav(int index, int vol);
    void Play_MP3(int index, int vol);

#ifdef __cplusplus
}
#endif

#endif