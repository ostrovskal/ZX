//
// Created by Sergey on 21.01.2020.
//

#pragma once

#define AY_SAMPLERS     8000
#define AY_ENV_CONT	    8
#define AY_ENV_ATTACK	4
#define AY_ENV_ALT	    2
#define AY_ENV_HOLD	    1
#define STEREO_BUF_SIZE 1024

class zxSound {
    friend void callback_ay8912(SLBufferQueueItf pBufferQueue, void* pThis);
public:
    struct AY_SAMPLER {
        u_long   tstates;
        uint16_t ofs;
        uint8_t  reg, val;
    };

    zxSound();
    ~zxSound();

    enum {
        AFINE, ACOARSE, BFINE, BCOARSE, CFINE, CCOARSE, NOISEPER, ENABLE, AVOL,
        BVOL, CVOL, EFINE, ECOARSE, ESHAPE
    };

    int update();
    void ayWrite(uint8_t reg, uint8_t value, uint32_t tick);
    void reset();
    void beeperWrite(uint8_t on);
    void updateProps();
protected:
    void initDriver();
    void apply();
    void makePlayer(bool stereo);
    short* write_buf_pstereo(signed short *out, int c);

    // признак инициализации драйвер
    bool isInit;

    // признак активного звука
    bool isEnabled;

    // признак активного звукого процессора
    bool isAyEnabled;

    // признак активного бипера
    bool isBpEnabled;

    // режим стерео для AY(ABC=1, ACB =0)
    int sound_stereo_ay;
    int sound_stereo_ay_narrow;

    // циклов на кадр
    u_long tsmax;

    // амплитуда/громкость бипера
    int ampBeeper, volBeeper;

    // частота звука
    int frequency;

    // массив сэмплов
    AY_SAMPLER samplers[AY_SAMPLERS];

    // текущее количество сэмплов
    int countSamplers;


    // размер звукового буфера
    int sndBufSize;

    // буфер звука
    signed short *sndBuf;
    // воспроизводящий буфер звука
    signed short *sndPlayBuf;

    // параметры формирование сигнала бипера
    int beeperOldPos, beeperPos, beeperOldVal, beeperOrigVal;
    int beeperLastPos;

    // стерео буфер
    int pstereobuf[STEREO_BUF_SIZE];
    // позиция в буфере и размер стерео буфера
    int pstereobufsiz, pstereopos, psgap;

    // левый/правый буфер для стерео режима
    int rstereobuf_l[STEREO_BUF_SIZE], rstereobuf_r[STEREO_BUF_SIZE];
    int rstereopos, rchan1pos, rchan2pos, rchan3pos;

    // параметры 3-х канального AY
    uint32_t toneTick[3], toneHigh[3], noiseTick;
    uint32_t tonePeriod[3], noisePeriod, envPeriod;
    uint32_t envIntTick, envTick, tickAY;
    uint32_t toneSubCycles, envSubCycles;
    uint8_t  ayRegs[16];
    uint32_t toneLevels[16];

    // частота звукового процессора
    int clockAY;

    // движок
    SLObjectItf engineObj;

    // микшер
    SLObjectItf mixObj;

    // плеер
    SLObjectItf playerObj;

    // интерфейс движка
    SLEngineItf engine;

    // интерфейс плеера
    SLPlayItf player;

    // интерфейс буфера
    SLBufferQueueItf bufferQueue;
};