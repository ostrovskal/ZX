//
// Created by Sergey on 04.02.2020.
//

#include "zxCommon.h"
#include "zxSound.h"

struct SNDCHIP_VOLTAB {
    uint32_t v[32];
};

struct SNDCHIP_PANTAB {
    uint32_t raw[6];
};

static SNDCHIP_VOLTAB chipVol[] = {
        {   0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2, 0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
            0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258, 0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF }, //  AY
        {   0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2, 0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
            0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E, 0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF }  // YM
};

static SNDCHIP_PANTAB stereoMode[] = {
        { 58, 58,  58,58,   58,58  }, // mono
        { 100,10,  66,66,   10,100 }, // ABC
        { 100,10,  10,100,  66,66  }, // ACB
        { 66,66,   100,10,  10,100 }, // BAC
        { 10,100,  100,10,  66,66  }, // BCA
        { 66,66,   10,100,  100,10 }, // CAB
        { 10,100,  66,66,   100,10 }  // CBA
};

static uint32_t filter_diff[TICK_F * 2];
constexpr double filter_sum_full     = 1.0, filter_sum_half = 0.5;
constexpr uint32_t filter_sum_full_u = (uint32_t)(filter_sum_full * 0x10000);
constexpr uint32_t filter_sum_half_u = (uint32_t)(filter_sum_half * 0x10000);

zxSoundDev::zxSoundDev() : mix_l(0), mix_r(0), s1_l(0), s1_r(0), s2_l(0), s2_r(0) {
    updateProps(ULA->machine->cpuClock);
}

void zxSoundDev::updateProps(uint32_t _clock_rate) {
    clock_rate = _clock_rate;
    tick = base_tick = 0; dstpos = buffer;
    sample_rate = frequencies[opts[ZX_PROP_SND_FREQUENCY]];
}

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
        dstpos->sample = ((mix_l * scale + s2_l) >> 16) + ((mix_r * scale + s2_r) & 0xFFFF0000);
        dstpos++;
        if(dstpos - buffer >= 16384) dstpos = buffer;
        scale = filter_sum_half_u - filter_diff[tick & tick1];
        s2_l = s1_l + mix_l * scale;
        s2_r = s1_r + mix_r * scale;
        tick = (tick | tick1) + 1;
        if((endtick ^ tick) & ~tick1) {
            auto val_l = mix_l * filter_sum_half_u;
            auto val_r = mix_r * filter_sum_half_u;
            do {
                dstpos->sample = ((s2_l + val_l) >> 16) + ((s2_r + val_r) & 0xFFFF0000);
                dstpos++;
                if(dstpos - buffer >= 16384) dstpos = buffer;
                tick += TICK_F; s2_l = val_l; s2_r = val_r;
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
    auto ready_min = acpu->audioDataReady();
    auto ready_s = bp.audioDataReady();
    if(ready_min > ready_s) ready_min = ready_s;
    if(rdy + ready_min > 65536) { rdy = 0; ready_min = 0; }
    if(ready_min) {
        auto buf = ext_buf ? ext_buf : buffer;
        auto p = (int*)(buf + rdy);
        auto s0 = (int*)acpu->audioData();
        auto s1 = (int*)bp.audioData();
        for(int i = 0; i < ready_min / 4; i++) *p++ = (*s0++) + (*s1++);
        rdy += ready_min;
    }
    acpu->audioDataUse(acpu->audioDataReady());
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

void zxBeeper::ioWrite(uint16_t port, uint8_t v, uint32_t tact) {
    auto spk = (short)(((v & 16) >> 4) * volSpk);
    auto mic = (short)(((v & 8)  >> 3) * volMic);
    auto mono = (uint32_t)(spk + mic);
    update(tact, mono, mono);
}

void zxBeeper::updateProps(uint32_t v) {
    clock_rate = ULA->machine->cpuClock * (opts[ZX_PROP_TURBO_MODE] ? 2 : 1);
    volSpk = (int)((2.5f * v) * 512);
    volMic = volSpk >> 2;
}

void zxSoundMixer::updateProps() {
    reset();

    isEnable       = opts[ZX_PROP_SND_LAUNCH] != 0;
    isAyEnable     = isEnable && opts[ZX_PROP_SND_AY];
    isBpEnable     = isEnable && opts[ZX_PROP_SND_BP];

    auto chipAy    = opts[ZX_PROP_SND_CHIP_AY];
    acpu = (chipAy == 0 ? (zxSoundDev*)&ay : (zxSoundDev*)&ym);

    acpu->updateProps();
    bp.updateProps(opts[ZX_PROP_SND_VOLUME_BP]);
}

zxAy::zxAy() : t(0), ta(0), tb(0), tc(0), tn(0), te(0), env(0), denv(0),
                bitA(0), bitB(0), bitC(0), bitN(0), ns(0),
                bit0(0), bit1(0), bit2(0), bit3(0), bit4(0), bit5(0),
                ea(0), eb(0), ec(0), va(0), vb(0), vc(0),
                fa(0), fb(0), fc(0), fn(0), fe(0) {
    opts[AY_REG] = 0;
    setChip(0);
    updateProps();
    reset();
}

void zxAy::flush(uint32_t chiptick) {
    while (t < chiptick) {
        t++;
        if(++ta >= fa) ta = 0, bitA ^= -1;
        if(++tb >= fb) tb = 0, bitB ^= -1;
        if(++tc >= fc) tc = 0, bitC ^= -1;
        if(++tn >= fn) tn = 0, ns = (ns * 2 + 1) ^ (((ns >> 16 ) ^ (ns >> 13)) & 1), bitN = 0 - ((ns >> 16) & 1);
        if(++te >= fe) {
            te = 0, env += denv;
            if(env & ~31) {
                auto mask = (uint32_t)(1 << opts[AY_ESHAPE]);
                if(mask & 0b1000001011111110) env = (uint32_t)(denv = 0);
                else if(mask & 0b0001000100000000) env &= 31;
                else if(mask & 0b0100010000000000) denv = -denv, env += denv;
                else env = 31, denv = 0;
            }
        }
        uint32_t en, mix_l, mix_r;
        en = ((ea & env) | va) & ((bitA | bit0) & (bitN | bit3));
        mix_l  = vols[0][en]; mix_r  = vols[1][en];
        en = ((eb & env) | vb) & ((bitB | bit1) & (bitN | bit4));
        mix_l += vols[2][en]; mix_r += vols[3][en];
        en = ((ec & env) | vc) & ((bitC | bit2) & (bitN | bit5));
        mix_l += vols[4][en]; mix_r += vols[5][en];
        if((mix_l ^ zxSoundDev::mix_l) | (mix_r ^ zxSoundDev::mix_r)) update(t, mix_l, mix_r);
    }
}

void zxAy::write(uint32_t timestamp, uint8_t val) {
    auto activereg = opts[AY_REG];
    if(activereg > 15) return;
    if((1 << activereg) & 0b0010000000101010) val &= 0x0F;
    if((1 << activereg) & 0b0000011101000000) val &= 0x1F;
//    if(activereg != ESHAPE && opts[AY_AFINE + activereg] == val) return;
    opts[AY_AFINE + activereg] = val;
    if(timestamp) flush((timestamp * mult_const) >> MULT_C_1);
    switch(activereg) {
        case AFINE: case ACOARSE: fa = (opts[AY_AFINE] | (opts[AY_ACOARSE] << 8)); break;
        case BFINE: case BCOARSE: fb = (opts[AY_BFINE] | (opts[AY_BCOARSE] << 8)); break;
        case CFINE: case CCOARSE: fc = (opts[AY_CFINE] | (opts[AY_CCOARSE] << 8)); break;
        case NOISEPER: fn = (uint32_t)(val * 2); break;
        case ENABLE:
            bit0 = (uint32_t)(0 - ((val &  1) != 0));
            bit1 = (uint32_t)(0 - ((val &  2) != 0));
            bit2 = (uint32_t)(0 - ((val &  4) != 0));
            bit3 = (uint32_t)(0 - ((val &  8) != 0));
            bit4 = (uint32_t)(0 - ((val & 16) != 0));
            bit5 = (uint32_t)(0 - ((val & 32) != 0));
            break;
        case AVOL:
            ea = (uint32_t)((val & 0x10) ? -1 : 0);
            va = ((val & 0x0F) * 2 + 1) & ~ea;
            break;
        case BVOL:
            eb = (uint32_t)((val & 0x10)? -1 : 0);
            vb = ((val & 0x0F) * 2 + 1) & ~eb;
            break;
        case CVOL:
            ec = (uint32_t)((val & 0x10)? -1 : 0);
            vc = ((val & 0x0F) * 2 + 1) & ~ec;
            break;
        case EFINE: case ECOARSE: fe = (opts[AY_EFINE] | (opts[AY_ECOARSE] << 8)); break;
        case ESHAPE: te = 0; /* attack/decay */ if(opts[AY_ESHAPE] & 4) env = 0, denv = 1; else env = 31, denv = -1; break;
    }
}

void zxAy::updateProps(uint32_t clock_rate) {
    clock_rate     = ULA->machine->cpuClock * (opts[ZX_PROP_TURBO_MODE] ? 2 : 1);
    auto clockAY   = ULA->machine->ayClock * (opts[ZX_PROP_TURBO_MODE] ? 2 : 1);
    auto chip      = opts[ZX_PROP_SND_CHIP_AY];
    auto chan      = opts[ZX_PROP_SND_CHANNEL_AY];
    auto vol       = (int)opts[ZX_PROP_SND_VOLUME_AY];

    auto voltab = &chipVol[chip];
    auto stereo = &stereoMode[chan];

    system_clock_rate = clock_rate;
    chip_clock_rate = clockAY >> 3;
    mult_const = (uint32_t)(((uint64_t)chip_clock_rate << MULT_C_1) / system_clock_rate);
    zxSoundDev::updateProps(chip_clock_rate);
    passed_chip_ticks = passed_clk_ticks = 0;
    t = 0; ns = 0xFFFF;
    vol = vol * 2184 + 7;
    for (int j = 0; j < 6; j++)
        for (int i = 0; i < 32; i++)
            vols[j][i] = (uint32_t)(((uint64_t)vol * voltab->v[i] * stereo->raw[j]) / (65535 * 100 * 3));
}

void zxAy::reset(uint32_t timestamp) {
    for(int i = 0; i < 16; i++) opts[AY_AFINE + i] = 0;
    applyRegs(timestamp);
}

void zxAy::applyRegs(uint32_t timestamp) {
    for(uint8_t r = 0; r < 16; r++) {
        select(r);
        auto p = opts[AY_AFINE + r];
        write(timestamp, (uint8_t)(p ^ 1));
        write(timestamp, p);
    }
}

void zxYm::updateProps(uint32_t) {
    static int levels[16] = {
            0x0000, 0x0385, 0x053D, 0x0770, 0x0AD7, 0x0FD5, 0x15B0, 0x230C,
            0x2B4C, 0x43C1, 0x5A4B, 0x732F, 0x9204, 0xAFF1, 0xD921, 0xFFFF
    };

    reset();

    auto turbo      = opts[ZX_PROP_TURBO_MODE] ? 2 : 1;
    tsmax           = ULA->machine->cpuClock * turbo / 50;
    clockAY         = ULA->machine->ayClock * turbo;

    auto modeAy     = opts[ZX_PROP_SND_CHANNEL_AY];
    auto vAy        = opts[ZX_PROP_SND_VOLUME_AY] * 256;
    auto freq       = frequencies[opts[ZX_PROP_SND_FREQUENCY]];

    frequency = freq;
    sound_stereo_ay = modeAy;
    for (int f = 0; f < 16; f++) toneLevels[f] = (uint32_t) ((levels[f] * vAy + 0x8000) / 0xffff);
    tickAY = (uint32_t)(65536. * clockAY / frequency);

    noiseTick = noisePeriod = 0;
    envIntTick = envTick = envPeriod = 0;
    toneSubCycles = envSubCycles = 0;

    for (int f = 0; f < 3; f++) {
        toneTick[f] = toneHigh[f] = 0;
        tonePeriod[f] = 1;
    }

    sndBufSize  = frequency / 50;
    memset(rstereobuf_l, 0, sizeof(rstereobuf_l));
    memset(rstereobuf_r, 0, sizeof(rstereobuf_r));
    int pos = 6 * frequency / 8000;
    rstereopos = 0; rchan1pos = -pos;
    if(sound_stereo_ay == 1) {
        rchan2pos = 0; rchan3pos = pos;
    } else if(sound_stereo_ay == 2) {
        rchan2pos = pos; rchan3pos = 0;
    }
}

void zxYm::reset(uint32_t timestamp) {
    int f;

    memset(buffer, 0, sndBufSize * 2 * sizeof(short));
    countSamplers = 0;
    rng = 1; noise_toggle = 0; envFirst = 1; envRev = 0; envCounter = 15;

    for(f = 0; f < 16; f++) ioWrite((uint8_t)f, 0, timestamp);
    for(f = 0; f < 3; f++) toneHigh[f] = 0;
    toneSubCycles = envSubCycles = 0;
    tickAY = (uint32_t)(65536. * clockAY / frequency);
}

void zxYm::write(uint8_t val, uint32_t tick) {
    auto reg = opts[AY_REG];
    if (reg < 16) {
        opts[reg + AY_AFINE] = val;
        if(reg < 14) {
            if (countSamplers < AY_SAMPLERS) {
                samplers[countSamplers].tstates = tick;
                samplers[countSamplers].reg = reg;
                samplers[countSamplers].val = val;
                countSamplers++;
            }
        }
    }
}

void* zxYm::audioData() {
    int toneLevel[3], chans[3];
    int mixer, envshape;
    int f, g, level, count;
    signed short *ptr;
    struct AY_SAMPLER *change_ptr = samplers;
    int changes_left = countSamplers;
    int reg, r;
    int frameTime = (int)(tsmax * 50);
    uint32_t tone_count, noise_count;
    memset(buffer, 0, sndBufSize * 2 * sizeof(short));
    if(!countSamplers) return buffer;
    for(f = 0; f < countSamplers; f++) samplers[f].ofs = (uint16_t)((samplers[f].tstates * frequency) / frameTime);
    for(f = 0, ptr = (short*)buffer; f < sndBufSize; f++) {
        while(changes_left && f >= change_ptr->ofs) {
            ayRegs[reg = change_ptr->reg] = change_ptr->val;
            change_ptr++; changes_left--;
            switch(reg) {
                case AFINE: case ACOARSE: case BFINE: case BCOARSE: case CFINE: case CCOARSE:
                    r = reg >> 1;
                    tonePeriod[r] = (uint32_t)((ayRegs[reg & ~1] | (ayRegs[reg | 1] & 15) << 8));
                    if(!tonePeriod[r]) tonePeriod[r]++;
                    if(toneTick[r] >= tonePeriod[r] * 2) toneTick[r] %= tonePeriod[r] * 2;
                    break;
                case NOISEPER:
                    noiseTick = 0;
                    noisePeriod = (uint32_t)(ayRegs[reg] & 31);
                    break;
                case EFINE: case ECOARSE:
                    envPeriod = ayRegs[EFINE] | (ayRegs[ECOARSE] << 8);
                    break;
                case ESHAPE:
                    envIntTick = envTick = envSubCycles = 0;
                    envFirst = 1;
                    envRev = 0;
                    envCounter = (ayRegs[13] & AY_ENV_ATTACK) ? 0 : 15;
                    break;
            }
        }
        for(g = 0 ; g < 3; g++) toneLevel[g] = toneLevels[ayRegs[AVOL + g] & 15];
        envshape = ayRegs[ESHAPE];
        level = toneLevels[envCounter];
        for(g = 0; g < 3; g++) { if(ayRegs[AVOL + g] & 16) toneLevel[g] = level; }
        envSubCycles += tickAY;
        noise_count = 0;
        while(envSubCycles >= (16 << 16)) {
            envSubCycles -= (16 << 16);
            noise_count++;
            envTick++;
            while(envTick>=envPeriod) {
                envTick -= envPeriod;
                if(envFirst || ((envshape & AY_ENV_CONT) && !(envshape & AY_ENV_HOLD))) {
                    auto t = (envshape & AY_ENV_ATTACK) ? 1 : -1;
                    if(envRev) envCounter -= t; else envCounter += t;
                    if(envCounter < 0) envCounter = 0;
                    if(envCounter > 15) envCounter = 15;
                }
                envIntTick++;
                while(envIntTick >= 16) {
                    envIntTick -= 16;
                    if(!(envshape & AY_ENV_CONT)) envCounter = 0;
                    else {
                        if(envshape & AY_ENV_HOLD) {
                            if(envFirst && (envshape & AY_ENV_ALT)) envCounter = (envCounter ? 0 : 15);
                        }
                        else {
                            if(envshape & AY_ENV_ALT) envRev = !envRev;
                            else envCounter = (envshape & AY_ENV_ATTACK) ? 0 : 15;
                        }
                    }
                    envFirst = 0;
                }
                if(!envPeriod) break;
            }
        }
        mixer = ayRegs[ENABLE];
        toneSubCycles += tickAY;
        tone_count = toneSubCycles >> 19;
        toneSubCycles &= (8 << 16) - 1;
        int channelSum = 0;
        for(int chan = 0; chan < 3; chan++) {
            auto channel = toneLevel[chan];
            level = channel;
            auto is_on = !(mixer & (1 << chan));
            auto is_low = false;
            if(is_on) {
                channel = 0;
                if(level) { if(toneHigh[chan]) channel = level; else channel = -level, is_low = 1; }
            }
            toneTick[chan] += tone_count;
            count = 0;
            while(toneTick[chan] >= tonePeriod[chan]) {
                count++;
                toneTick[chan] -= tonePeriod[chan];
                toneHigh[chan] = (uint32_t)!toneHigh[chan];
                if(is_on && count == 1 && level && toneTick[chan] < tone_count)	{
                    auto t = (level * 2 * toneTick[chan] / tone_count);
                    if(is_low) channel += t; else channel -= t;
                }
            }
            if(is_on && count > 1) channel = -level;
            if((mixer & (8 << chan)) == 0 && noise_toggle) channel = 0;
            chans[chan] = channel;
            channelSum += channel;
        }
        if(sound_stereo_ay == 0) {
            (*ptr++) += channelSum;
            (*ptr++) += channelSum;
        } else {
            getStereo(rchan1pos, chans[0]);
            getStereo(rchan2pos, chans[1]);
            getStereo(rchan3pos, chans[2]);
            (*ptr++) += rstereobuf_l[rstereopos];
            (*ptr++) += rstereobuf_r[rstereopos];
            rstereobuf_l[rstereopos] = rstereobuf_r[rstereopos] = 0;
            rstereopos++;
            if(rstereopos >= STEREO_BUF_SIZE) rstereopos = 0;
        }
        noiseTick += noise_count;
        while(noiseTick >= noisePeriod) {
            noiseTick -= noisePeriod;
            if((rng & 1) ^ ((rng & 2) ? 1 : 0)) noise_toggle = !noise_toggle;
            rng |= ((rng & 1) ^ ((rng & 4) ? 1 : 0)) ? 0x20000 : 0;
            rng >>= 1;
            if(!noisePeriod) break;
        }
    }
    return buffer;
}

void zxYm::getStereo(int pos, int chan) {
    if(pos < 0) {
        rstereobuf_l[rstereopos] += chan;
        rstereobuf_r[(rstereopos - pos) % STEREO_BUF_SIZE] += chan;
    } else {
        rstereobuf_l[(rstereopos + pos) % STEREO_BUF_SIZE] += chan;
        rstereobuf_r[rstereopos] += chan;
    }
}
