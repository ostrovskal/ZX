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
    zxSoundDev() : mix_l(0), mix_r(0), s1_l(0), s1_r(0), s2_l(0), s2_r(0) {
        setTimings(3500000, 44100);
    }

    virtual void frameStart(uint32_t tacts) {
        auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
        base_tick = tick - endtick;
    }

    virtual void frameEnd(uint32_t tacts) {
        auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
        flush(base_tick + endtick);
    }

    virtual void update(uint32_t tact, uint32_t l, uint32_t r);
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) { }
    virtual uint8_t ioRead(uint16_t port) { return 0xff; }

    void audioDataUse(uint32_t size) { dstpos = buffer; }
    void setTimings(uint32_t clock, uint32_t sample) { clock_rate = clock; sample_rate = sample; tick = base_tick = 0; dstpos = buffer; }
    void* audioData() { return buffer; }
    uint32_t audioDataReady() { return (dstpos - buffer) * sizeof(SNDSAMPLE); }
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

class zxBeeper: public zxSoundDev {
public:
    zxBeeper() : volSpk(12800), volMic(3200) { }
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) {
        auto spk = (short)(((v & 16) >> 4) * volSpk);
        auto mic = (short)(((v & 8)  >> 3) * volMic);
        auto mono = (uint32_t)(spk + mic);
        update(tact, mono, mono);
    }
    void setVolumes(int8_t v) {
        volSpk = (int)((2.5f * v) * 512);
        volMic = volSpk >> 2;
    }
    void reset() { memset(buffer, 0, sizeof(buffer)); }
protected:
    int volSpk, volMic;
};

class zxAy : public zxSoundDev {
public:
    zxAy();
    virtual ~zxAy() {}
    enum {
        AFINE, ACOARSE, BFINE, BCOARSE, CFINE, CCOARSE, NOISEPER, ENABLE, AVOL,
        BVOL, CVOL, EFINE, ECOARSE, ESHAPE
    };

    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) {
        if(port == 0xFFFD) select(v);
        else if(port == 0xBFFD) write((uint32_t)tact, v);
    }
    virtual void frameStart(uint32_t tacts) {
        t = tacts * chip_clock_rate / system_clock_rate;
        zxSoundDev::frameStart(t);
    }

    virtual void frameEnd(uint32_t tacts) {
        auto end_chip_tick = ((passed_clk_ticks + tacts) * chip_clock_rate) / system_clock_rate;
        flush((uint32_t)(end_chip_tick - passed_chip_ticks));
        zxSoundDev::frameEnd(t);
        passed_clk_ticks += tacts; passed_chip_ticks += t;
    }
    virtual uint8_t ioRead(uint16_t port) { if(opts[AY_REG] > 15) return 0xFF; return opts[AY_AFINE + opts[AY_REG]]; }

    void setChip(int type) { chiptype = type; }
    void setTimings(uint32_t system_clock_rate, uint32_t chip_clock_rate, uint32_t sample_rate);
    void setVolumes(uint32_t global_vol, int chip, int stereo);
    void reset(uint32_t timestamp = 0);

protected:
    void select(uint8_t nreg) { if(chiptype == 0) nreg &= 0x0F; opts[AY_REG] = nreg; }
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
    zxSoundMixer() : rdy(0), isEnable(true), isAyEnable(true), isBpEnable(true) {}
    void update(uint8_t * ext_buf);
    void use(uint32_t size, uint8_t * ext_buf = nullptr);
    void updateProps();
    void reset() { ay.reset(); bp.reset(); }
    void ioWrite(int dev, uint16_t port, uint8_t v, uint32_t tact) {
        if(isEnable) {
            switch (dev) {
                case 0: if(isAyEnable) ay.ioWrite(port, v, tact); break;
                case 1: if(isBpEnable) bp.ioWrite(port, v, tact); break;
            }
        }
    }
    uint8_t ioRead(int dev, uint16_t port) {
        if(isEnable) {
            switch (dev) {
                case 0: if(isAyEnable) return ay.ioRead(port); break;
                case 1: if(isBpEnable) return bp.ioRead(port); break;
            }
        }
        return 0xff;
    }
    void frameStart(uint32_t tacts) {
        ay.frameStart(tacts);
        bp.frameStart(tacts);
    }

    void frameEnd(uint32_t tacts) {
        ay.frameEnd(tacts);
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
    // 2 источник - звуковой процессор
    zxAy ay;
};
