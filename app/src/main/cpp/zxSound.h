//
// Created by Sergey on 26.12.2019.
//

#pragma once

#define TIMER_CONST			        19

#define SND_ARRAY_LEN				1000
#define SND_STEP					800
#define SND_BEEPER					255

#define SND_TICKS_PER_FRAME_1_CONST	882
#define SND_TICKS_PER_FRAME_2_CONST	441
#define SND_TICKS_PER_FRAME_4_CONST	220
#define SND_TICKS_PER_FRAME_MAX		10000

#define AY_STEREO_ACB               0
#define AY_STEREO_ABC               1

class zxSound {
public:
    struct AY_CHANNEL {
        uint16_t channelData[SND_TICKS_PER_FRAME_MAX * 2];
        int genPeriod, genCounter, noiseCounter;
        int rnd;
        bool isChannel;
        bool isNoise;
        void gen(int i, int period, uint8_t volume, int ch_vol);
    };

    struct AY_STEPS {
        uint32_t tcounter;
        uint8_t reg, val;
        int pos;
    };

    zxSound();

    static uint8_t* _REGISTERS;
    static uint8_t* _CURRENT;

    void updateProps(uint8_t period);

    void reset();

    void update();

    void write(uint8_t reg, uint8_t value);

protected:

    // инициализация OpenSL ES
    bool initSL();

    // проигрывание
    void play();

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
    void mix(uint16_t *buf, int num, int offs, int count, ...);

    //
    uint32_t updateStep;

    // громкость бипера/AY
    uint32_t beeperVolume, ayVolume;

    // признак активности AY/бипера
    bool isAY, isBeeper;

    // признак инициализации
    bool isInitSL;

    //
    uint32_t cont, att, alt, hold, up, holded;

    // 3 канала
    AY_CHANNEL channelA, channelB, channelC;

    // тип распределения каналов в стерео режиме
    int stereoAY;

    // частота дискредитации
    int freq;

    // результирующий буфер звука
    uint16_t soundBuffer[SND_TICKS_PER_FRAME_MAX * 2 * 2];

    // буфер бипера
    uint16_t beeperBuffer[SND_TICKS_PER_FRAME_MAX * 2];

    // текущее значение бипера
    uint16_t beepVal;

    //
    uint32_t periodN, periodE;

    //
    uint8_t envVolume;

    //
    uint32_t envelopeCounter;

    //
    uint16_t* mixing_ch[10];
};
