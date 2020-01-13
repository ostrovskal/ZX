//
// Created by Sergey on 26.12.2019.
//

#pragma once

#define TIMER_CONST			        19

#define SND_ARRAY_LEN				256
#define SND_STEP					800

#define SND_TICKS_PER_FRAME_MAX		10000

#define AY_STEREO_ACB               0
#define AY_STEREO_ABC               1

class zxSound {
public:
    struct AY_CHANNEL {
        AY_CHANNEL() { memset(data, 0, sizeof(data)); period = counter = noise = rnd = 0; }
        uint16_t data[SND_TICKS_PER_FRAME_MAX];
        int period, counter, noise, rnd;
        bool isEnable;
        bool isNoise;
        void gen(int i, int periodN, uint8_t volume, int reg);
    };

    struct AY_STEPS {
        uint32_t tcounter;
        uint8_t reg, val;
        int pos;
    };

    enum {
        AFINE, ACOARSE, BFINE, BCOARSE, CFINE, CCOARSE, NOISEPER, ENABLE, AVOL,
        BVOL, CVOL, EFINE, ECOARSE, ESHAPE, PORTA, PORTB, BEEPER
    };
    zxSound();
    ~zxSound();

    void updateProps(uint8_t period);

    void reset();

    void update();

    void write(uint8_t reg, uint8_t value);

protected:

    // инициализация OpenSL ES
    bool initSL();

    // применение параметров регистров на текущем шаге
    void applyRegister(uint8_t reg, uint8_t val);

    void envelopeStep();

    // количество выборок(sampler-ов)
    int nSamplers;

    // значения выборок звука
    AY_STEPS SAMPLER[SND_ARRAY_LEN];

    //
    int SND_TICKS_PER_FRAME[3];

    // микширование AY/бипера
    void mix(uint16_t *buf, int count, ...);

    // копия буфера регистров(в процессе преобразования)
    static uint8_t regs[17];

    //
    uint32_t updateStep;

    // громкость бипера/AY
    uint32_t beeperVolume, ayVolume;

    // признак активности AY/бипера
    bool isAY, isBeeper;

    // признак инициализации
    bool isInitSL;

    //
    uint8_t cont, att, alt, hold, up, holded;

    // 3 канала
    AY_CHANNEL channelA, channelB, channelC;

    // тип распределения каналов в стерео режиме
    int stereoAY;

    // частота дискредитации
    int freq;

    // результирующий буфер звука
    uint16_t soundBuffer[SND_TICKS_PER_FRAME_MAX];

    // буфер бипера
    uint16_t beeperBuffer[SND_TICKS_PER_FRAME_MAX];

    // текущее значение бипера
    uint16_t beepVal;

    //
    uint32_t periodN, periodE;

    //
    uint8_t envVolume;

    //
    uint32_t envelopeCounter;

    // движок
    SLObjectItf engineObj;

    // миксер
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
