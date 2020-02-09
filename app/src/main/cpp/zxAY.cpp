//
// Created by Sergey on 04.02.2020.
//

#include "zxCommon.h"
#include "zxAY.h"

// corresponds enum CHIP_TYPE
const char * const ay_chips[] = { "AY-3-8910", "YM2149F" };
const SNDCHIP_VOLTAB SNDR_VOL_AY_S =
        { { 0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2, 0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
                  0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258, 0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF } };

const SNDCHIP_VOLTAB SNDR_VOL_YM_S =
        { { 0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2, 0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
                  0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E, 0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF } };

static const SNDCHIP_PANTAB SNDR_PAN_MONO_S =  { { 58, 58,  58,58,   58,58  } };
static const SNDCHIP_PANTAB SNDR_PAN_ABC_S  =  { { 100,10,  66,66,   10,100 } };
static const SNDCHIP_PANTAB SNDR_PAN_ACB_S  =  { { 100,10,  10,100,  66,66  } };

const SNDCHIP_VOLTAB* SNDR_VOL_AY           = &SNDR_VOL_AY_S;
const SNDCHIP_VOLTAB* SNDR_VOL_YM           = &SNDR_VOL_YM_S;
const SNDCHIP_PANTAB* SNDR_PAN_MONO         = &SNDR_PAN_MONO_S;
const SNDCHIP_PANTAB* SNDR_PAN_ABC          = &SNDR_PAN_ABC_S;
const SNDCHIP_PANTAB* SNDR_PAN_ACB          = &SNDR_PAN_ACB_S;
// oversampling ratio: 2^6 = 64
const uint32_t TICK_FF                      = 6;
const uint32_t TICK_F                       = (1 << TICK_FF);
const uint32_t MULT_C_1                     = 14;

static uint32_t filter_diff[TICK_F * 2];
const double filter_sum_full     = 1.0, filter_sum_half = 0.5;
const uint32_t filter_sum_full_u = (uint32_t)(filter_sum_full * 0x10000);
const uint32_t filter_sum_half_u = (uint32_t)(filter_sum_half * 0x10000);

const double filter_coeff[TICK_F * 2] = {
        // filter designed with Matlab's DSP toolbox
        0.000797243121022152, 0.000815206499600866, 0.000844792477531490, 0.000886460636664257,
        0.000940630171246217, 0.001007677515787512, 0.001087934129054332, 0.001181684445143001,
        0.001289164001921830, 0.001410557756409498, 0.001545998595893740, 0.001695566052785407,
        0.001859285230354019, 0.002037125945605404, 0.002229002094643918, 0.002434771244914945,
        0.002654234457752337, 0.002887136343664226, 0.003133165351783907, 0.003391954293894633,
        0.003663081102412781, 0.003946069820687711, 0.004240391822953223, 0.004545467260249598,
        0.004860666727631453, 0.005185313146989532, 0.005518683858848785, 0.005860012915564928,
        0.006208493567431684, 0.006563280932335042, 0.006923494838753613, 0.007288222831108771,
        0.007656523325719262, 0.008027428904915214, 0.008399949736219575, 0.008773077102914008,
        0.009145787031773989, 0.009517044003286715, 0.009885804729257883, 0.010251021982371376,
        0.010611648461991030, 0.010966640680287394, 0.011314962852635887, 0.011655590776166550,
        0.011987515680350414, 0.012309748033583185, 0.012621321289873522, 0.012921295559959939,
        0.013208761191466523, 0.013482842243062109, 0.013742699838008606, 0.013987535382970279,
        0.014216593638504731, 0.014429165628265581, 0.014624591374614174, 0.014802262449059521,
        0.014961624326719471, 0.015102178534818147, 0.015223484586101132, 0.015325161688957322,
        0.015406890226980602, 0.015468413001680802, 0.015509536233058410, 0.015530130313785910,
        0.015530130313785910, 0.015509536233058410, 0.015468413001680802, 0.015406890226980602,
        0.015325161688957322, 0.015223484586101132, 0.015102178534818147, 0.014961624326719471,
        0.014802262449059521, 0.014624591374614174, 0.014429165628265581, 0.014216593638504731,
        0.013987535382970279, 0.013742699838008606, 0.013482842243062109, 0.013208761191466523,
        0.012921295559959939, 0.012621321289873522, 0.012309748033583185, 0.011987515680350414,
        0.011655590776166550, 0.011314962852635887, 0.010966640680287394, 0.010611648461991030,
        0.010251021982371376, 0.009885804729257883, 0.009517044003286715, 0.009145787031773989,
        0.008773077102914008, 0.008399949736219575, 0.008027428904915214, 0.007656523325719262,
        0.007288222831108771, 0.006923494838753613, 0.006563280932335042, 0.006208493567431684,
        0.005860012915564928, 0.005518683858848785, 0.005185313146989532, 0.004860666727631453,
        0.004545467260249598, 0.004240391822953223, 0.003946069820687711, 0.003663081102412781,
        0.003391954293894633, 0.003133165351783907, 0.002887136343664226, 0.002654234457752337,
        0.002434771244914945, 0.002229002094643918, 0.002037125945605404, 0.001859285230354019,
        0.001695566052785407, 0.001545998595893740, 0.001410557756409498, 0.001289164001921830,
        0.001181684445143001, 0.001087934129054332, 0.001007677515787512, 0.000940630171246217,
        0.000886460636664257, 0.000844792477531490, 0.000815206499600866, 0.000797243121022152
};

static struct zxFilterDiffInit {
    zxFilterDiffInit() {
        double sum = 0;
        for(int i = 0; i < (int)TICK_F * 2; i++) {
            filter_diff[i] = (uint32_t)(sum * 0x10000);
            sum += filter_coeff[i];
        }
    }
} fdi;

void zxAy::beeperWrite(uint16_t port, uint8_t v, int tact) {
    auto spk = (short)(v & 16) << 9;// 8192
    auto mic = (short)(v & 8) << 7; // 1024
    auto mono = spk + mic;
    Update(tact, mono, mono);
}

const char* zxAy::GetChipName(CHIP_TYPE i) { return ay_chips[i]; }

zxAy::zxAy() : t(0), ta(0), tb(0), tc(0), tn(0), te(0), env(0), denv(0)
        ,bitA(0), bitB(0), bitC(0), bitN(0), ns(0)
        ,bit0(0), bit1(0), bit2(0), bit3(0), bit4(0), bit5(0)
        ,ea(0), eb(0), ec(0), va(0), vb(0), vc(0)
        ,fa(0), fb(0), fc(0), fn(0), fe(0), activereg(0) {
    SetChip(CHIP_AY);
    SetTimings(SNDR_DEFAULT_SYSTICK_RATE, SNDR_DEFAULT_AY_RATE, SNDR_DEFAULT_SAMPLE_RATE);
    SetVolumes(0x7FFF, SNDR_VOL_AY, SNDR_PAN_ABC);
    _Reset();
}

uint8_t zxAy::IoRead(uint16_t port, int tact) {
    return Read();
}

void zxAy::IoWrite(uint16_t port, uint8_t v, int tact) {
    if((port & 0xC0FF) == 0xC0FD) Select(v);
    if((port & 0xC000) == 0x8000) Write((uint32_t)tact, v);
}

void zxAy::FrameStart(uint32_t tacts) {
    t = tacts * chip_clock_rate / system_clock_rate;
    eInherited::FrameStart(t);
}

void zxAy::FrameEnd(uint32_t tacts) {
    //adjusting 't' with whole history will fix accumulation of rounding errors
    uint64_t end_chip_tick = ((passed_clk_ticks + tacts) * chip_clock_rate) / system_clock_rate;
    Flush((uint32_t)(end_chip_tick - passed_chip_ticks));
    eInherited::FrameEnd(t);
    passed_clk_ticks += tacts;
    passed_chip_ticks += t;
}

void zxAy::Flush(uint32_t chiptick) {
    // todo: noaction at (temp.sndblock || !conf.sound.ay)
    while (t < chiptick) {
        t++;
        if(++ta >= fa) ta = 0, bitA ^= -1;
        if(++tb >= fb) tb = 0, bitB ^= -1;
        if(++tc >= fc) tc = 0, bitC ^= -1;
        if(++tn >= fn) tn = 0, ns = (ns * 2 + 1) ^ (((ns >> 16 )^ (ns >> 13)) & 1), bitN = 0 - ((ns >> 16) & 1);
        if(++te >= fe) {
            te = 0, env += denv;
            if(env & ~31) {
                uint32_t mask = (uint32_t)(1 << r.env);
                if(mask & ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 15)))
                    env = (uint32_t)(denv = 0);
                else if(mask & ((1 << 8) | (1 << 12))) env &= 31;
                else if(mask & ((1 << 10) | (1 << 14))) denv = -denv, env = env + denv;
                else env = 31, denv = 0; //11, 13
            }
        }
        uint32_t en, mix_l, mix_r;
        en = ((ea & env) | va) & ((bitA | bit0) & (bitN | bit3));
        mix_l  = vols[0][en]; mix_r  = vols[1][en];
        en = ((eb & env) | vb) & ((bitB | bit1) & (bitN | bit4));
        mix_l += vols[2][en]; mix_r += vols[3][en];
        en = ((ec & env) | vc) & ((bitC | bit2) & (bitN | bit5));
        mix_l += vols[4][en]; mix_r += vols[5][en];
        // similar check inside update()
        if((mix_l ^ eInherited::mix_l) | (mix_r ^ eInherited::mix_r)) Update(t, mix_l, mix_r);
    }
}

void zxAy::Select(uint8_t nreg) {
    if(chiptype == CHIP_AY) nreg &= 0x0F;
    activereg = nreg;
}

void zxAy::Write(uint32_t timestamp, uint8_t val) {
    if(activereg >= 0x10) return;
    if((1 << activereg) & ((1 << 1) | (1 << 3) | (1 << 5) | (1 << 13))) val &= 0x0F;
    if((1 << activereg) & ((1 << 6) | (1 << 8) | (1 << 9) | (1 << 10))) val &= 0x1F;
    if(activereg != 13 && reg[activereg] == val) return;
    reg[activereg] = val;
    // cputick * ( (chip_clock_rate/8) / system_clock_rate );
    if(timestamp) Flush((timestamp * mult_const) >> MULT_C_1);
    switch(activereg) {
        case 0: case 1:
            fa = SwapWord(r.fA);
            break;
        case 2: case 3:
            fb = SwapWord(r.fB);
            break;
        case 4: case 5:
            fc = SwapWord(r.fC);
            break;
        case 6:
            fn = (uint32_t)val * 2;
            break;
        case 7:
            bit0 = (uint32_t)(0 - ((val >> 0) & 1));
            bit1 = (uint32_t)(0 - ((val >> 1) & 1));
            bit2 = (uint32_t)(0 - ((val >> 2) & 1));
            bit3 = (uint32_t)(0 - ((val >> 3) & 1));
            bit4 = (uint32_t)(0 - ((val >> 4) & 1));
            bit5 = (uint32_t)(0 - ((val >> 5) & 1));
            break;
        case 8:
            ea = (uint32_t)((val & 0x10)? -1 : 0);
            va = ((val & 0x0F) * 2 + 1) & ~ea;
            break;
        case 9:
            eb = (uint32_t)((val & 0x10)? -1 : 0);
            vb = ((val & 0x0F) * 2 + 1) & ~eb;
            break;
        case 10:
            ec = (uint32_t)((val & 0x10)? -1 : 0);
            vc = ((val & 0x0F) * 2 + 1) & ~ec;
            break;
        case 11:
        case 12:
            fe = SwapWord(r.envT);
            break;
        case 13:
            te = 0;
            // attack
            if(r.env & 4) env = 0, denv = 1;
            // decay
            else env = 31, denv = -1;
            break;
    }
}

uint8_t zxAy::Read() {
    if(activereg >= 0x10) return 0xFF;
    return reg[activereg & 0x0F];
}

void zxAy::SetTimings(uint32_t _system_clock_rate, uint32_t _chip_clock_rate, uint32_t _sample_rate) {
    _chip_clock_rate /= 8;
    system_clock_rate = _system_clock_rate;
    chip_clock_rate = _chip_clock_rate;
    mult_const = (uint32_t)(((uint64_t)chip_clock_rate << MULT_C_1) / system_clock_rate);
    eInherited::SetTimings(_chip_clock_rate, _sample_rate);
    passed_chip_ticks = passed_clk_ticks = 0;
    t = 0; ns = 0xFFFF;
}

void zxAy::SetVolumes(uint32_t global_vol, const SNDCHIP_VOLTAB *voltab, const SNDCHIP_PANTAB *stereo) {
    for (int j = 0; j < 6; j++)
        for (int i = 0; i < 32; i++)
            vols[j][i] = (uint32_t)(((uint64_t)global_vol * voltab->v[i] * stereo->raw[j])/(65535*100*3));
}

void zxAy::_Reset(uint32_t timestamp) {
    for(int i = 0; i < 16; i++) reg[i] = 0;
    ApplyRegs(timestamp);
}

void zxAy::ApplyRegs(uint32_t timestamp) {
    uint8_t ar = activereg;
    for(uint8_t r = 0; r < 16; r++) {
        Select(r);
        uint8_t p = reg[r];
        /* clr cached values */
        Write(timestamp, (uint8_t)(p ^ 1));
        Write(timestamp, p);
    }
    activereg = ar;
}

zxSoundDev::zxSoundDev() : mix_l(0), mix_r(0), s1_l(0), s1_r(0), s2_l(0), s2_r(0), ready(0) {
    SetTimings(SNDR_DEFAULT_SYSTICK_RATE, SNDR_DEFAULT_SAMPLE_RATE);
}

void zxSoundDev::FrameStart(uint32_t tacts) {
    //prev frame rest
    auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
    base_tick = tick - endtick;
}

void zxSoundDev::Update(uint32_t tact, uint32_t l, uint32_t r) {
    if(!((l ^ mix_l) | (r ^ mix_r))) return;
    auto endtick = (uint32_t)((tact * (uint64_t)sample_rate * TICK_F) / clock_rate);
    Flush(base_tick + endtick);
    mix_l = l; mix_r = r;
}

void zxSoundDev::FrameEnd(uint32_t tacts) {
    auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
    Flush(base_tick + endtick);
}

void* zxSoundDev::AudioData() {
    return buffer;
}

uint32_t zxSoundDev::AudioDataReady() {
    return (dstpos - buffer) * sizeof(SNDSAMPLE);
}

void zxSoundDev::AudioDataUse(uint32_t size) {
    assert(size == AudioDataReady());
    dstpos = buffer;
}

void zxSoundDev::SetTimings(uint32_t _clock_rate, uint32_t _sample_rate) {
    clock_rate = _clock_rate;
    sample_rate = _sample_rate;

    tick = base_tick = 0;
    dstpos = buffer;
}

void zxSoundDev::Flush(uint32_t endtick) {
    uint32_t scale;
    if(!((endtick ^ tick) & ~(TICK_F-1))) {
        //same discrete as before
        scale = filter_diff[(endtick & (TICK_F - 1)) + TICK_F] - filter_diff[(tick & (TICK_F - 1)) + TICK_F];
        s2_l  += mix_l * scale;
        s2_r  += mix_r * scale;
        scale = filter_diff[endtick & (TICK_F-1)] - filter_diff[tick & (TICK_F-1)];
        s1_l  += mix_l * scale;
        s1_r  += mix_r * scale;

        tick  = endtick;
    }
    else {
        scale = filter_sum_full_u - filter_diff[(tick & (TICK_F-1)) + TICK_F];

        uint32_t sample_value;
        sample_value =	((mix_l*scale + s2_l) >> 16) + ((mix_r*scale + s2_r) & 0xFFFF0000);
        dstpos->sample = sample_value;
        dstpos++;
        if(dstpos - buffer >= BUFFER_LEN) dstpos = buffer;
        scale = filter_sum_half_u - filter_diff[tick & (TICK_F - 1)];
        s2_l = s1_l + mix_l * scale;
        s2_r = s1_r + mix_r * scale;
        tick = (tick | (TICK_F - 1))+1;
        if((endtick ^ tick) & ~(TICK_F-1)) {
            // assume filter_coeff is symmetric
            uint32_t val_l = mix_l * filter_sum_half_u;
            uint32_t val_r = mix_r * filter_sum_half_u;
            do {
                uint32_t sample_value1;
                // save s2+val
                sample_value1 =	((s2_l + val_l) >> 16) + ((s2_r + val_r) & 0xFFFF0000);
                dstpos->sample = sample_value1;
                dstpos++;
                if(dstpos - buffer >= BUFFER_LEN) dstpos = buffer;
                tick += TICK_F;
                s2_l = val_l;
                // s2=s1, s1=0;
                s2_r = val_r;
            } while ((endtick ^ tick) & ~(TICK_F - 1));
        }
        tick = endtick;
        scale = filter_diff[(endtick & (TICK_F-1)) + TICK_F] - filter_sum_half_u;
        s2_l += mix_l * scale;
        s2_r += mix_r * scale;
        scale = filter_diff[endtick & (TICK_F-1)];
        s1_l = mix_l * scale;
        s1_r = mix_r * scale;
    }
}

void zxSoundDev::Update(uint8_t * ext_buf) {
/*    using namespace xPlatform;
    int x = Handler()->AudioSources();
    if(!x) return;
    assert(x == 3); // for some optimizations
    auto ready_min = Handler()->AudioDataReady(0);
    for(int s = 1; s < x; ++s) {
        auto ready_s = Handler()->AudioDataReady(s);
        if(ready_min > ready_s) ready_min = ready_s;
    }
    if(ready + ready_min > BUF_SIZE) {
        ready = 0;
        ready_min = 0;
    }
    if(ready_min) {
        auto buf = ext_buf ? ext_buf : buffer1;
        auto p = (int*)(buf + ready);
        auto s0 = (int*)Handler()->AudioData(0);
        auto s1 = (int*)Handler()->AudioData(1);
        auto s2 = (int*)Handler()->AudioData(2);
        for(int i = ready_min/4; --i >= 0;) {
            if(i > 8) {
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                *p++ = (*s0++) + (*s1++) + (*s2++);
                i -= 8;
            }
            *p++ = (*s0++) + (*s1++) + (*s2++);
        }
        ready += ready_min;
    }
    for(int s = 0; s < x; ++s) {
        Handler()->AudioDataUse(s, Handler()->AudioDataReady(s));
    }*/
}
//=============================================================================
//	eSoundMixer::Use
//-----------------------------------------------------------------------------
void zxSoundDev::Use(uint32_t size, uint8_t * ext_buf) {
    if(size) {
        if(ready > size) {
            auto buf = ext_buf ? ext_buf : buffer1;
            memmove(buf, buf + size, ready - size);
        }
        ready -= size;
    }
}

int zxSoundDev::UpdateSound(uint8_t* buf) {
    Update(buf);
    auto size = Ready();
    Use(size, buf);
    return size;
}