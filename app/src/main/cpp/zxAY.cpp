//
// Created by Sergey on 04.02.2020.
//

#include "zxCommon.h"
#include "zxAY.h"

// corresponds enum CHIP_TYPE
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

static uint32_t filter_diff[TICK_F * 2];
const double filter_sum_full     = 1.0, filter_sum_half = 0.5;
const uint32_t filter_sum_full_u = (uint32_t)(filter_sum_full * 0x10000);
const uint32_t filter_sum_half_u = (uint32_t)(filter_sum_half * 0x10000);

void zxSoundDev::update(uint32_t tact, uint32_t l, uint32_t r) {
    if(!((l ^ mix_l) | (r ^ mix_r))) return;
    auto endtick = (uint32_t)((tact * (uint64_t)sample_rate * TICK_F) / clock_rate);
    flush(base_tick + endtick);
    mix_l = l; mix_r = r;
}

void zxSoundDev::flush(uint32_t endtick) {
    uint32_t scale;
    auto tick1 = TICK_F - 1;
    if(!((endtick ^ tick) & ~tick1)) {
        //same discrete as before
        scale = filter_diff[(endtick & tick1) + TICK_F] - filter_diff[(tick & tick1) + TICK_F];
        s2_l  += mix_l * scale; s2_r  += mix_r * scale;
        scale = filter_diff[endtick & tick1] - filter_diff[tick & tick1];
        s1_l  += mix_l * scale; s1_r  += mix_r * scale;
        tick  = endtick;
    } else {
        scale = filter_sum_full_u - filter_diff[(tick & tick1) + TICK_F];
        uint32_t sample_value;
        sample_value =	((mix_l*scale + s2_l) >> 16) + ((mix_r * scale + s2_r) & 0xFFFF0000);
        dstpos->sample = sample_value;
        dstpos++;
        if(dstpos - buffer >= BUFFER_LEN) dstpos = buffer;
        scale = filter_sum_half_u - filter_diff[tick & tick1];
        s2_l = s1_l + mix_l * scale;
        s2_r = s1_r + mix_r * scale;
        tick = (tick | (TICK_F - 1)) + 1;
        if((endtick ^ tick) & ~tick1) {
            auto val_l = mix_l * filter_sum_half_u;
            auto val_r = mix_r * filter_sum_half_u;
            do {
                uint32_t sample_value1;
                sample_value1 =	((s2_l + val_l) >> 16) + ((s2_r + val_r) & 0xFFFF0000);
                dstpos->sample = sample_value1;
                dstpos++;
                if(dstpos - buffer >= BUFFER_LEN) dstpos = buffer;
                tick += TICK_F;
                s2_l = val_l;
                s2_r = val_r;
            } while ((endtick ^ tick) & ~tick1);
        }
        tick = endtick;
        scale = filter_diff[(endtick & tick1) + TICK_F] - filter_sum_half_u;
        s2_l += mix_l * scale; s2_r += mix_r * scale;
        scale = filter_diff[endtick & tick1];
        s1_l = mix_l * scale; s1_r = mix_r * scale;
    }
}

void zxSoundMixer::update(uint8_t * ext_buf) {
    auto ready_min = ay.audioDataReady();
    auto ready_s = bp.audioDataReady();
    if(ready_min > ready_s) ready_min = ready_s;
    if(rdy + ready_min > BUF_SIZE) { rdy = 0; ready_min = 0; }
    if(ready_min) {
        auto buf = ext_buf ? ext_buf : buffer;
        int* p = (int*)(buf + rdy);
        int* s0 = (int*)ay.audioData();
        int* s1 = (int*)bp.audioData();
        for(int i = ready_min / 4; --i >= 0;) {
            if(i > 8) {
                *p++ = (*s0++) + (*s1++); *p++ = (*s0++) + (*s1++);
                *p++ = (*s0++) + (*s1++); *p++ = (*s0++) + (*s1++);
                *p++ = (*s0++) + (*s1++); *p++ = (*s0++) + (*s1++);
                *p++ = (*s0++) + (*s1++); *p++ = (*s0++) + (*s1++);
                i -= 8;
            }
            *p++ = (*s0++) + (*s1++);
        }
        rdy += ready_min;
    }
    ay.audioDataUse(ay.audioDataReady());
    bp.audioDataUse(bp.audioDataReady());
}

void zxSoundMixer::use(uint32_t size, uint8_t * ext_buf) {
    if(size) {
        if(rdy > size) {
            auto buf = ext_buf ? ext_buf : buffer;
            memmove(buf, buf + size, rdy - size);
        }
        rdy -= size;
    }
}

zxAy::zxAy() : t(0), ta(0), tb(0), tc(0), tn(0), te(0), env(0), denv(0),
                bitA(0), bitB(0), bitC(0), bitN(0), ns(0),
                bit0(0), bit1(0), bit2(0), bit3(0), bit4(0), bit5(0),
                ea(0), eb(0), ec(0), va(0), vb(0), vc(0),
                fa(0), fb(0), fc(0), fn(0), fe(0), activereg(0) {
    setChip(CHIP_AY);
    setTimings(SNDR_DEFAULT_SYSTICK_RATE, SNDR_DEFAULT_AY_RATE, SNDR_DEFAULT_SAMPLE_RATE);
    setVolumes(0x7FFF, SNDR_VOL_AY, SNDR_PAN_ABC);
    reset();
}

void zxAy::flush(uint32_t chiptick) {
    // todo: noaction at (temp.sndblock || !conf.sound.ay)
    while (t < chiptick) {
        t++;
        if(++ta >= fa) ta = 0, bitA ^= -1;
        if(++tb >= fb) tb = 0, bitB ^= -1;
        if(++tc >= fc) tc = 0, bitC ^= -1;
        if(++tn >= fn) tn = 0, ns = (ns * 2 + 1) ^ (((ns >> 16 ) ^ (ns >> 13)) & 1), bitN = 0 - ((ns >> 16) & 1);
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
        if((mix_l ^ zxSoundDev::mix_l) | (mix_r ^ zxSoundDev::mix_r)) update(t, mix_l, mix_r);
    }
}

void zxAy::write(uint32_t timestamp, uint8_t val) {
    if(activereg >= 0x10) return;
    if((1 << activereg) & ((1 << 1) | (1 << 3) | (1 << 5) | (1 << 13))) val &= 0x0F;
    if((1 << activereg) & ((1 << 6) | (1 << 8) | (1 << 9) | (1 << 10))) val &= 0x1F;
    if(activereg != 13 && reg[activereg] == val) return;
    reg[activereg] = val;
    if(timestamp) flush((timestamp * mult_const) >> MULT_C_1);
    switch(activereg) {
        case 0: case 1: fa = r.fA; break;
        case 2: case 3: fb = r.fB; break;
        case 4: case 5: fc = r.fC; break;
        case 6: fn = (uint32_t)val * 2; break;
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
        case 11: case 12: fe = r.envT; break;
        case 13:
            te = 0;
            // attack/decay
            if(r.env & 4) env = 0, denv = 1; else env = 31, denv = -1;
            break;
    }
}

void zxAy::setTimings(uint32_t system, uint32_t chip, uint32_t sample) {
    chip /= 8;
    system_clock_rate = system;
    chip_clock_rate = chip;
    mult_const = (uint32_t)(((uint64_t)chip_clock_rate << MULT_C_1) / system_clock_rate);
    zxSoundDev::setTimings(chip, sample);
    passed_chip_ticks = passed_clk_ticks = 0;
    t = 0; ns = 0xFFFF;
}

void zxAy::setVolumes(uint32_t global_vol, const SNDCHIP_VOLTAB *voltab, const SNDCHIP_PANTAB *stereo) {
    for (int j = 0; j < 6; j++)
        for (int i = 0; i < 32; i++)
            vols[j][i] = (uint32_t)(((uint64_t)global_vol * voltab->v[i] * stereo->raw[j])/(65535*100*3));
}

void zxAy::reset(uint32_t timestamp) {
    for(int i = 0; i < 16; i++) reg[i] = 0;
    applyRegs(timestamp);
}

void zxAy::applyRegs(uint32_t timestamp) {
    uint8_t ar = activereg;
    for(uint8_t r = 0; r < 16; r++) {
        select(r);
        uint8_t p = reg[r];
        write(timestamp, (uint8_t)(p ^ 1));
        write(timestamp, p);
    }
    activereg = ar;
}
