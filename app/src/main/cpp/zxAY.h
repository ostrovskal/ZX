//
// Created by Sergey on 04.02.2020.
//

#pragma once

enum CHIP_TYPE { CHIP_AY, CHIP_YM, CHIP_MAX };

constexpr uint32_t TICK_FF                      = 6;
constexpr uint32_t TICK_F                       = (1 << TICK_FF);
constexpr uint32_t MULT_C_1                     = 14;
constexpr uint32_t SNDR_DEFAULT_SYSTICK_RATE    = 69888 * 50; // ZX-Spectrum Z80 clock
constexpr uint32_t SNDR_DEFAULT_SAMPLE_RATE     = 44100;
constexpr uint32_t SNDR_DEFAULT_AY_RATE         = 1773400;//1774400; // original ZX-Spectrum soundchip clock fq

struct CHANNEL {
    uint16_t left;
    uint16_t right;
};

union SNDSAMPLE {
    // left/right channels in low/high WORDs
    uint32_t sample;
    // or left/right separately
    CHANNEL ch;
};

#pragma pack(push, 1)
struct AYREGS {
    uint16_t fA, fB, fC;
    uint8_t noise, mix;
    uint8_t vA, vB, vC;
    uint16_t envT;
    uint8_t env;
    uint8_t portA, portB;
};
#pragma pack(pop)

struct SNDCHIP_VOLTAB {
    uint32_t v[32];
};

struct SNDCHIP_PANTAB {
    uint32_t raw[6];
};

class zxSoundDev {
public:
    enum { BUFFER_LEN = 16384 };
    zxSoundDev() : mix_l(0), mix_r(0), s1_l(0), s1_r(0), s2_l(0), s2_r(0) {
        setTimings(SNDR_DEFAULT_SYSTICK_RATE, SNDR_DEFAULT_SAMPLE_RATE);
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
    uint32_t audioDataReady() {
        auto sub = dstpos - buffer;
        return sub * sizeof(SNDSAMPLE);
    }
protected:
    uint32_t mix_l, mix_r;
    uint32_t clock_rate, sample_rate;
    SNDSAMPLE buffer[BUFFER_LEN];
    SNDSAMPLE* dstpos;
private:
    void flush(uint32_t endtick);
    uint32_t tick, base_tick;
    uint32_t s1_l, s1_r;
    uint32_t s2_l, s2_r;
};

class zxBeeper: public zxSoundDev {
public:
    virtual void ioWrite(uint16_t port, uint8_t v, uint32_t tact) {
        auto spk = (short)(v & 16) << 9;// 8192
        auto mic = (short)(v & 8) << 7; // 1024
        auto mono = spk + mic;
        update(tact, (uint32_t)mono, (uint32_t)mono);
    }
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
    virtual uint8_t ioRead(uint16_t port) { return read(); }

    void setChip(CHIP_TYPE type) { chiptype = type; }
    void setTimings(uint32_t system_clock_rate, uint32_t chip_clock_rate, uint32_t sample_rate);
    void setVolumes(uint32_t global_vol, const SNDCHIP_VOLTAB *voltab, const SNDCHIP_PANTAB *stereo);
    //void setRegs(const uint8_t _reg[16]) { memcpy(reg, _reg, sizeof(reg)); applyRegs(0); }
    void reset(uint32_t timestamp = 0);

protected:
    void select(uint8_t nreg) { if(chiptype == CHIP_AY) nreg &= 0x0F; activereg = nreg; }
    void write(uint32_t timestamp, uint8_t val);
    uint8_t read() { if(activereg >= 0x10) return 0xFF; return reg[activereg & 0x0F]; }
private:
    int denv;
    uint32_t t, ta, tb, tc, tn, te, env;
    uint32_t bitA, bitB, bitC, bitN, ns;
    uint32_t bit0, bit1, bit2, bit3, bit4, bit5;
    uint32_t ea, eb, ec, va, vb, vc;
    uint32_t fa, fb, fc, fn, fe;
    uint32_t mult_const;
    uint8_t activereg;
    uint32_t vols[6][32];
    CHIP_TYPE chiptype;

    union {
        uint8_t reg[16];
        AYREGS r;
    };

    uint32_t chip_clock_rate, system_clock_rate;
    uint64_t passed_chip_ticks, passed_clk_ticks;

    void flush(uint32_t chiptick);
    void applyRegs(uint32_t timestamp = 0);
};

class zxSoundMixer {
public:
    enum { BUF_SIZE = 65536 };
    zxSoundMixer() : rdy(0) {}
    void update(uint8_t * ext_buf);
    void use(uint32_t size, uint8_t * ext_buf = nullptr);
    void reset() {
        ay.reset();
    }
    void updateProps() {

    }
    void ioWrite(int dev, uint16_t port, uint8_t v, long tact) {
        switch(dev) {
            case 0: ay.ioWrite(port, v, (uint32_t)tact); break;
            case 1: bp.ioWrite(port, v, (uint32_t)tact); break;
        }
    }
    uint8_t ioRead(int dev, uint16_t port) {
        switch (dev) {
            case 0: return ay.ioRead(port);
            case 1: return bp.ioRead(port);
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
    // выходной буфер
    uint8_t	buffer[BUF_SIZE];
    // размер данных
    uint32_t rdy;
    // 1 источник - бипер
    zxBeeper bp;
    // 2 источник - звуковой процессор
    zxAy ay;
};
