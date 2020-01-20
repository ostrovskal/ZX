//
// Created by Sergey on 26.12.2019.
//

#pragma once

#define TIMER_CONST			        19

#define SND_ARRAY_LEN				512
#define SND_STEP					800

#define AY_STEREO_ACB               0
#define AY_STEREO_ABC               1

class zxSound {
    friend void callback_sound(SLBufferQueueItf pBufferQueue, void *pContext);
public:
    struct AY_CHANNEL {
        AY_CHANNEL() : data(nullptr), period(0), counter(0), noise(0), rnd(0) { }
        ~AY_CHANNEL() { delete[] data; data = nullptr; }
        uint16_t* data;
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
    void initSL();

    // применение параметров регистров на текущем шаге
    void applyRegister(uint8_t reg, uint8_t val);

    void envelopeStep();

    // количество выборок(sampler-ов)
    int nSamplers;

    // размер буфера
    uint32_t nLength;

    // значения выборок звука
    AY_STEPS SAMPLER[SND_ARRAY_LEN];

    // микширование AY/бипера
    void mix(uint16_t *buf, int count, ...);

    //
    uint32_t updateStep;

    // громкость бипера/AY
    uint32_t beeperVolume, ayVolume;

    // признак активности AY/бипера
    bool isAY, isBeeper;

    // признак инициализации
    bool isInitSL;

    // параметры формы
    uint8_t cont, att, alt, hold, up, holded;

    // 3 канала
    AY_CHANNEL channelA, channelB, channelC;

    // стерео режим
    int stereoMode;

    // буфер звука
    uint16_t* buffer;

    // буфер бипера
    uint16_t* beeperBuffer;

    // текущее значение бипера
    uint16_t beepVal;

    // период огибающей
    uint32_t periodN, periodE;

    // базовая громкость
    uint8_t envVolume;

    //
    uint32_t envelopeCounter;

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

    void makePlayer();
};
