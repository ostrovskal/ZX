//
// Created by Sergey on 04.02.2020.
//

#pragma once

extern uint8_t* opts;

constexpr uint32_t TICK_FF                      = 6;
constexpr uint32_t TICK_F                       = (1 << TICK_FF);
constexpr uint32_t MULT_C_1                     = 14;

struct CHANNEL {
    // левый канал
    uint16_t left;
    // правый канал
    uint16_t right;
};

union SNDSAMPLE {
    // левый/правый каналы в младшем/старшем слове
    uint32_t sample;
    // или правый/левый каналы раздельно
    CHANNEL ch;
};

class zxSoundDev {
public:
    enum {
        AFINE, ACOARSE, BFINE, BCOARSE, CFINE, CCOARSE, NOISEPER, ENABLE, AVOL,
        BVOL, CVOL, EFINE, ECOARSE, ESHAPE
    };

    zxSoundDev();
    virtual void frameStart(uint32_t tacts) {
        auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
        base_tick = tick - endtick;
    }
    virtual void frameEnd(uint32_t tacts) {
        auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
        flush(base_tick + endtick);
    }
    virtual void reset(uint32_t timestamp = 0) { memset(buffer, 0, sizeof(buffer)); }
    virtual void update(uint32_t tact, uint32_t l, uint32_t r);
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) { }
    virtual uint8_t ioRead(uint16_t port) { return 0xff; }
    virtual void updateProps(uint32_t clock_rate = 0);
    virtual void audioDataUse(uint32_t size) { dstpos = buffer; }
    virtual void* audioData() { return buffer; }
    virtual uint32_t audioDataReady() { return (dstpos - buffer) * sizeof(SNDSAMPLE); }
protected:
    uint32_t mix_l, mix_r;
    uint32_t clock_rate, sample_rate;
    SNDSAMPLE buffer[16384];
    SNDSAMPLE* dstpos;
private:
    void flush(uint32_t endtick);
    uint32_t tick, base_tick;
    uint32_t s1_l, s1_r;
    uint32_t s2_l, s2_r;
};

class zxCovox: public zxSoundDev {
    zxCovox() : volSpk(12800), volMic(3200) { }
    //virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) override;
    //virtual void updateProps(uint32_t clock_rate = 0) override;
protected:
    int volSpk, volMic;
};

class zxBeeper: public zxSoundDev {
public:
    zxBeeper() : volSpk(12800), volMic(3200) { }
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) override;
    virtual void updateProps(uint32_t clock_rate = 0) override;
protected:
    int volSpk, volMic;
};

#define AY_SAMPLERS     8000
#define AY_ENV_CONT	    8
#define AY_ENV_ATTACK	4
#define AY_ENV_ALT	    2
#define AY_ENV_HOLD	    1
#define STEREO_BUF_SIZE 1024

class zxYm: public zxSoundDev {
public:
    struct AY_SAMPLER {
        uint32_t tstates;
        uint16_t ofs;
        uint8_t reg, val;
    };

    zxYm() : tsmax(0), sound_stereo_ay(0), frequency(44100) { }

    virtual void frameStart(uint32_t tacts) override { }
    virtual void frameEnd(uint32_t tacts) override { }
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) override {
        if(port == 0xFFFD) { opts[AY_REG] = v; }
        else if(port == 0xBFFD) write(v, tact);
    }
    virtual void reset(uint32_t timestamp = 0) override;
    virtual void updateProps(uint32_t clock_rate = 0) override;
    virtual void audioDataUse(uint32_t size) override { countSamplers = 0; }
    virtual void* audioData() override;
    virtual uint8_t ioRead(uint16_t port) override { if(opts[AY_REG] > 15) return 0xFF; return opts[AY_AFINE + opts[AY_REG]]; }
    virtual uint32_t audioDataReady() override { return sndBufSize * 2 * sizeof(short); }
protected:
    void write(uint8_t val, uint32_t tact);
    // режим стерео для AM(ABC = 1, ACB = 0)
    int sound_stereo_ay;
    // циклов на кадр
    uint32_t tsmax;
    // частота звука
    int frequency;
    // массив сэмплов
    AY_SAMPLER samplers[AY_SAMPLERS];
    // текущее количество сэмплов
    int countSamplers;
    // размер звукового буфера
    int sndBufSize;
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
    uint32_t clockAY;
};

class zxAy : public zxSoundDev {
public:
    zxAy();
    virtual ~zxAy() {}
    virtual void frameStart(uint32_t tacts) override {
        t = tacts * chip_clock_rate / system_clock_rate;
        zxSoundDev::frameStart(t);
    }

    virtual void frameEnd(uint32_t tacts) override {
        auto end_chip_tick = ((passed_clk_ticks + tacts) * chip_clock_rate) / system_clock_rate;
        flush((uint32_t)(end_chip_tick - passed_chip_ticks));
        zxSoundDev::frameEnd(t);
        passed_clk_ticks += tacts; passed_chip_ticks += t;
    }
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) override {
        if(port == 0xFFFD) select(v);
        else if(port == 0xBFFD) write(tact, v);
    }
    virtual uint8_t ioRead(uint16_t port) override { if(opts[AY_REG] > 15) return 0xFF; return opts[AY_AFINE + opts[AY_REG]]; }
    virtual void updateProps(uint32_t clock_rate = 0) override;
    virtual void reset(uint32_t timestamp = 0) override;
    void setChip(int type) { chiptype = type; }
protected:
    void select(uint8_t v) { if(chiptype == 0) v &= 0x0F; opts[AY_REG] = v; }
    void write(uint32_t timestamp, uint8_t val);
private:
    void flush(uint32_t chiptick);
    void applyRegs(uint32_t timestamp = 0);

    int denv, chiptype;
    uint32_t t, ta, tb, tc, tn, te, env;
    uint32_t bitA, bitB, bitC, bitN, ns;
    uint32_t bit0, bit1, bit2, bit3, bit4, bit5;
    uint32_t ea, eb, ec, va, vb, vc;
    uint32_t fa, fb, fc, fn, fe;
    uint32_t mult_const;
    uint32_t chip_clock_rate, system_clock_rate;
    uint64_t passed_chip_ticks, passed_clk_ticks;
    uint32_t vols[6][32];
};

class zxSoundMixer {
public:
    zxSoundMixer() : acpu(&ay), rdy(0), isEnable(true), isAyEnable(true), isBpEnable(true) {}
    void update(uint8_t * ext_buf);
    void use(uint32_t size, uint8_t * ext_buf = nullptr);
    void updateProps();
    void reset() { ay.reset(); bp.reset(); }
    void ioWrite(int dev, uint16_t port, uint8_t v, uint32_t tact) {
        if(isEnable) {
            switch (dev) {
                case 0: if(isAyEnable) acpu->ioWrite(port, v, tact); break;
                case 1: if(isBpEnable) bp.ioWrite(port, v, tact); break;
            }
        }
    }
    uint8_t ioRead(int dev, uint16_t port) {
        if(isEnable) {
            switch (dev) {
                case 0: if(isAyEnable) return acpu->ioRead(port); break;
                case 1: if(isBpEnable) return bp.ioRead(port); break;
            }
        }
        return 0xff;
    }
    void frameStart(uint32_t tacts) {
        acpu->frameStart(tacts);
        bp.frameStart(tacts);
    }

    void frameEnd(uint32_t tacts) {
        acpu->frameEnd(tacts);
        bp.frameEnd(tacts);
    }
    uint32_t ready() const { return rdy; }
protected:
    // признак включенного звука
    bool isEnable;
    // признак включенного AY
    bool isAyEnable;
    // признак включенного бипера
    bool isBpEnable;
    // выходной буфер
    uint8_t	buffer[65536];
    // размер данных
    uint32_t rdy;
    // 1 источник - бипер
    zxBeeper bp;
    // 2 источник - звуковой процессор1
    zxAy ay;
    // 3 источник - звуковой процессор2
    zxYm ym;
    // текущий
    zxSoundDev* acpu;
};
