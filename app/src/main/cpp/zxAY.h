//
// Created by Sergey on 04.02.2020.
//

#pragma once

union SNDSAMPLE {
    // left/right channels in low/high WORDs
    uint32_t sample;
    // or left/right separately
    struct { uint16_t left, right; } ch;
};

#pragma pack(push, 1)
struct AYREGS
{
    uint16_t fA, fB, fC;
    uint8_t noise, mix;
    uint8_t vA, vB, vC;
    uint16_t envT;
    uint8_t env;
    uint8_t portA, portB;
};
#pragma pack(pop)

// output volumes (#0000-#FFFF) for given envelope state or R8-R10 value
// AY chip has only 16 different volume values, so v[0]=v[1], v[2]=v[3], ...
struct SNDCHIP_VOLTAB {
    uint32_t v[32];
};

// generator's channel panning, % (0-100)
struct SNDCHIP_PANTAB {
    uint32_t raw[6];
    // structured as 'struct { uint32_t left, right; } chan[3]';
};

// used as parameters to SNDCHIP::set_volumes(),
// if application don't want to override defaults
extern const SNDCHIP_VOLTAB* SNDR_VOL_AY;
extern const SNDCHIP_VOLTAB* SNDR_VOL_YM;
extern const SNDCHIP_PANTAB* SNDR_PAN_MONO;
extern const SNDCHIP_PANTAB* SNDR_PAN_ABC;
extern const SNDCHIP_PANTAB* SNDR_PAN_ACB;

const uint32_t SNDR_DEFAULT_SYSTICK_RATE  = 71680 * 50; // ZX-Spectrum Z80 clock
const uint32_t SNDR_DEFAULT_SAMPLE_RATE   = 44100;
const uint32_t SNDR_DEFAULT_AY_RATE       = 1774400; // original ZX-Spectrum soundchip clock fq

class zxSoundDev {
public:
    zxSoundDev();
    void SetTimings(uint32_t clock_rate, uint32_t sample_rate);
    void FrameStart(uint32_t tacts);
    void FrameEnd(uint32_t tacts);
    void Update(uint32_t tact, uint32_t l, uint32_t r);
    void Update(uint8_t * ext_buf = nullptr);
    uint32_t Ready() const { return ready; }
    const void*	Ptr() const { return buffer; }
    void Use(uint32_t size, uint8_t * ext_buf = nullptr);
    int UpdateSound(uint8_t* buf);

    enum { BUFFER_LEN = 16384 };

    void* AudioData();
    uint32_t AudioDataReady();
    void AudioDataUse(uint32_t size);
protected:
    uint32_t mix_l, mix_r;
    SNDSAMPLE* dstpos;
    uint32_t clock_rate, sample_rate;
    SNDSAMPLE buffer[BUFFER_LEN];
private:
    enum { BUF_SIZE = 65536 };
    uint8_t	buffer1[BUF_SIZE];
    uint32_t ready;
    uint32_t tick, base_tick;
    uint32_t s1_l, s1_r;
    uint32_t s2_l, s2_r;
    void Flush(uint32_t endtick);
};

class zxAy : public zxSoundDev {
    typedef zxSoundDev eInherited;
public:
    zxAy();
    virtual ~zxAy() {}
    uint8_t IoRead(uint16_t port, int tact);
    void IoWrite(uint16_t port, uint8_t v, int tact);
    virtual void beeperWrite(uint16_t port, uint8_t v, int tact);

    enum CHIP_TYPE { CHIP_AY, CHIP_YM, CHIP_MAX };
    static const char* GetChipName(CHIP_TYPE i);

    void SetChip(CHIP_TYPE type) { chiptype = type; }
    void SetTimings(uint32_t system_clock_rate, uint32_t chip_clock_rate, uint32_t sample_rate);
    void SetVolumes(uint32_t global_vol, const SNDCHIP_VOLTAB *voltab, const SNDCHIP_PANTAB *stereo);
    void SetRegs(const uint8_t _reg[16]) { memcpy(reg, _reg, sizeof(reg)); ApplyRegs(0); }
    void Select(uint8_t nreg);

    virtual void Reset() { _Reset(); }
    virtual void FrameStart(uint32_t tacts);
    virtual void FrameEnd(uint32_t tacts);
protected:
    void Write(uint32_t timestamp, uint8_t val);
    uint8_t Read();
private:
    uint32_t t, ta, tb, tc, tn, te, env;
    int denv;
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
        struct AYREGS r;
    };

    uint32_t chip_clock_rate, system_clock_rate;
    uint64_t passed_chip_ticks, passed_clk_ticks;

    // call with default parameter, when context outside start_frame/end_frame block
    void _Reset(uint32_t timestamp = 0);
    void Flush(uint32_t chiptick);
    void ApplyRegs(uint32_t timestamp = 0);
};
