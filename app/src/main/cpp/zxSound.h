//
// Created by Sergey on 21.01.2020.
//

#pragma once

#define AMPL_AY_TONE		    (28 * 256)
#define AY_CHANGE_MAX		    8000
#define AY_ENV_CONT	            8
#define AY_ENV_ATTACK	        4
#define AY_ENV_ALT	            2
#define AY_ENV_HOLD	            1

class zxSound {
public:
    struct AY_SAMPLER {
        u_long   tstates;
        uint16_t ofs;
        uint8_t  reg, val;
    };

    zxSound();
    virtual ~zxSound() { }

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
    void ay_overlay();
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

    // частота звукового процессора
    int ay_clock;

    // частота звука
    int sound_freq;

    // массив сэмплов
    AY_SAMPLER samplers[AY_CHANGE_MAX];

    // текущее количество сэмплов
    int countSamplers;

    int psgap;

    // размер звукового буфера
    int sndBufSize;

    // буфер звука
    signed short *sndBuf;

    uint32_t ay_tone_levels[16];

    // параметры формирование сигнала бипера
    int beeperOldPos, beeperPos, beeperOldVal, beeperOrigVal;
    int beeperLastPos;

    uint32_t ay_tone_tick[3], ay_tone_high[3], ay_noise_tick;
    uint32_t ay_tone_subcycles, ay_env_subcycles;
    uint32_t ay_env_internal_tick,ay_env_tick;
    uint32_t ay_tick_incr;
    uint32_t ay_tone_period[3], ay_noise_period, ay_env_period;
    uint8_t sound_ay_registers[16];

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