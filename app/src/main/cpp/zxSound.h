//
// Created by Sergey on 21.01.2020.
//

#pragma once

#define FRAME_STATES_48		    (3500000 /50)
#define FRAME_STATES_128	    (3546900 / 50)
#define AY_CLOCK		        1773400
#define AMPL_AY_TONE		    (28 * 256)
#define AY_CHANGE_MAX		    8000
#define AY_ENV_CONT	            8
#define AY_ENV_ATTACK	        4
#define AY_ENV_ALT	            2
#define AY_ENV_HOLD	            1

class zxSound {
    friend void callback_ay8912(SLBufferQueueItf pBufferQueue, void*);
public:
    struct ay_change_tag {
        u_long   tstates;
        uint16_t ofs;
        uint8_t  reg,val;
    };

    zxSound();
    virtual ~zxSound() { }

    enum {
        AFINE, ACOARSE, BFINE, BCOARSE, CFINE, CCOARSE, NOISEPER, ENABLE, AVOL,
        BVOL, CVOL, EFINE, ECOARSE, ESHAPE, PORTA, PORTB, BEEPER
    };

    int update();
    void ayWrite(uint8_t reg, uint8_t value);
    void reset();
    void beeperWrite(uint8_t on);
    void updateProps(uint8_t period);
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

    // частота звука
    int sound_freq;

    // массив сэмплов
    ay_change_tag ay_change[AY_CHANGE_MAX];

    // текущее количество сэмплов
    int ay_change_count;

    int psgap;

    int sound_framesiz;
    uint32_t ay_tone_levels[16];

    signed short *sound_buf;

    int sound_oldpos, sound_fillpos, sound_oldval, sound_oldval_orig;

    uint32_t ay_tone_tick[3], ay_tone_high[3], ay_noise_tick;
    uint32_t ay_tone_subcycles, ay_env_subcycles;
    uint32_t ay_env_internal_tick,ay_env_tick;
    uint32_t ay_tick_incr;
    uint32_t ay_tone_period[3], ay_noise_period, ay_env_period;
    int beeper_last_subpos;
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