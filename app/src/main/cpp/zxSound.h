//
// Created by Sergey on 21.01.2020.
//

#pragma once

#define AMPL_AY_TONE	(28 * 256)
#define AY_SAMPLERS     8000
#define AY_ENV_CONT	    8
#define AY_ENV_ATTACK	4
#define AY_ENV_ALT	    2
#define AY_ENV_HOLD	    1

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
    void ayWrite(uint8_t reg, uint8_t value);
    void reset();
    void beeperWrite(uint8_t on);
    void updateProps();
protected:
    void initDriver();
    void apply();
    void makePlayer();

    // признак инициализации драйвер
    bool isInit;

    // признак активного звука
    bool isEnabled;

    // признак активного звукого процессора
    bool isAyEnabled;

    // признак активного бипера
    bool isBpEnabled;

    // циклов на кадр
    u_long tsmax;

    // амплитуда/громкость бипера
    int ampBeeper, volBeeper;

    // амплитуда/громкость звукового процессора
    int ampAy, volAy;

    // частота звука
    int frequency;

    // массив сэмплов
    AY_SAMPLER samplers[AY_SAMPLERS];

    // текущее количество сэмплов
    int countSamplers;

    int psgap;

    // размер звукового буфера
    int sndBufSize;

    // буфер звука
    signed short *sndBuf;

    // параметры формирование сигнала бипера
    int beeperOldPos, beeperPos, beeperOldVal, beeperOrigVal;
    int beeperLastPos;

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