//
// Created by Sergey on 18.02.2020.
//

#include "zxCommon.h"
#include "zxDevs.h"
#include "zxFormats.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                       МЫШЬ                                                  //
/////////////////////////////////////////////////////////////////////////////////////////////////

bool zxDevMouse::checkRead(uint16_t port) const {
    if(port & 0x20) return false;
    port |= 0xfa00;
    return !(port != 0xfadf && port != 0xfbdf && port != 0xffdf);
}

void zxDevMouse::read(uint16_t port, uint8_t* ret) {
    switch(port | 0xFA00) {
        case 0xBFDF: *ret = opts[MOUSE_X]; break;
        case 0xFFDF: *ret = opts[MOUSE_Y]; break;
        case 0xFADF: *ret = opts[MOUSE_K]; opts[MOUSE_K] = 0xFF;	break;
    }
}

void zxDevMouse::reset() {
    opts[MOUSE_K] = 0xFF;
    opts[MOUSE_X] = 128;
    opts[MOUSE_Y] = 96;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//                               БАЗОВЫЕ ФУНКЦИИ ЗВУКА                                         //
/////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t filter_diff[TICK_F * 2];
constexpr double filter_sum_full     = 1.0, filter_sum_half = 0.5;
constexpr uint32_t filter_sum_full_u = (uint32_t)(filter_sum_full * 0x10000);
constexpr uint32_t filter_sum_half_u = (uint32_t)(filter_sum_half * 0x10000);

zxDevSound::zxDevSound() : mix_l(0), mix_r(0), s1_l(0), s1_r(0), s2_l(0), s2_r(0) {
    update(zxSpeccy::machine->cpuClock);
}

int zxDevSound::update(int _clock_rate) {
    clock_rate = (uint32_t)_clock_rate;
    tick = base_tick = 0; dstpos = buffer;
    sample_rate = frequencies[opts[ZX_PROP_SND_FREQUENCY]];
    return 0;
}

void zxDevSound::updateData(uint32_t tact, uint32_t l, uint32_t r) {
    if(!((l ^ mix_l) | (r ^ mix_r))) return;
    auto endtick = (uint32_t)((tact * (uint64_t)sample_rate * TICK_F) / clock_rate);
    flush(base_tick + endtick);
    mix_l = l; mix_r = r;
}

void zxDevSound::flush(uint32_t endtick) {
    uint32_t scale;
    auto tick1 = TICK_F - 1;
    if(!((endtick ^ tick) & ~tick1)) {
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

/////////////////////////////////////////////////////////////////////////////////////////////////
//                               ЗВУКОВОЙ СОПРОЦЕССОР AY                                       //
/////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t chipVol[] = {
        0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2, 0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
        0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258, 0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF, //  AY
        0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2, 0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
        0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E, 0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF  // YM
};

static uint32_t stereoMode[] = {
        58, 58,  58,58,   58,58 , // mono
        100,10,  66,66,   10,100, // ABC
        100,10,  10,100,  66,66 , // ACB
        66,66,   100,10,  10,100, // BAC
        10,100,  100,10,  66,66 , // BCA
        66,66,   10,100,  100,10, // CAB
        10,100,  66,66,   100,10  // CBA
};

zxDevAY::zxDevAY() : t(0), ta(0), tb(0), tc(0), tn(0), te(0), env(0), denv(0),
                     bitA(0), bitB(0), bitC(0), bitN(0), ns(0),
                     bit0(0), bit1(0), bit2(0), bit3(0), bit4(0), bit5(0),
                     ea(0), eb(0), ec(0), va(0), vb(0), vc(0),
                     fa(0), fb(0), fc(0), fn(0), fe(0) {
    opts[AY_REG] = 0;
    update();
    reset();
}

void zxDevAY::select(uint8_t v) {
    opts[AY_REG] = (uint8_t)(v & 0x0F);
}

bool zxDevAY::checkRead(uint16_t port) const {
    return (port & 0xC0FF) == 0xC0FD;
}

bool zxDevAY::checkWrite(uint16_t port) const {
    if(port & 2) return false;
    if((port & 0xC0FF) == 0xC0FD) return true;
    return (port & 0xC000) == 0x8000;
}

void zxDevAY::write(uint16_t port, uint8_t val) {
	if(!opts[ZX_PROP_SND_CHIP_AY]) {
		if((port & 0xC0FF) == 0xC0FD) select(val);
		if((port & 0xC000) == 0x8000) _write(zxDevUla::_ftick, val);
	}
}

void zxDevAY::read(uint16_t, uint8_t* ret) {
	if(!opts[ZX_PROP_SND_CHIP_AY]) {
		*ret = (opts[AY_REG] < 16 ? opts[AY_AFINE + opts[AY_REG]] : (uint8_t) 0xFF);
	}
}

void zxDevAY::flush(uint32_t chiptick) {
    while (t < chiptick) {
        t++;
        if(++ta >= fa) ta = 0, bitA ^= 0xFFFFFFFF;
        if(++tb >= fb) tb = 0, bitB ^= 0xFFFFFFFF;
        if(++tc >= fc) tc = 0, bitC ^= 0xFFFFFFFF;
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
        if((mix_l ^ zxDevSound::mix_l) | (mix_r ^ zxDevSound::mix_r)) updateData(t, mix_l, mix_r);
    }
}

void zxDevAY::_write(uint32_t timestamp, uint8_t val) {
	if(zx->snd && zx->snd->isAyEnable) {
        auto activereg = opts[AY_REG];
        if(activereg > 15) return;
        if((1 << activereg) & 0b0010000000101010) val &= 0x0F;
        if((1 << activereg) & 0b0000011101000000) val &= 0x1F;
        if(activereg != ESHAPE && opts[AY_AFINE + activereg] == val) return;
        opts[AY_AFINE + activereg] = val;
        if(timestamp) flush((timestamp * mult_const) >> MULT_C_1);
        switch(activereg) {
            case AFINE:
            case ACOARSE:
                fa = (opts[AY_AFINE] | (opts[AY_ACOARSE] << 8));
                break;
            case BFINE:
            case BCOARSE:
                fb = (opts[AY_BFINE] | (opts[AY_BCOARSE] << 8));
                break;
            case CFINE:
            case CCOARSE:
                fc = (opts[AY_CFINE] | (opts[AY_CCOARSE] << 8));
                break;
            case NOISEPER:
                fn = (uint32_t) (val * 2);
                break;
            case ENABLE:
                bit0 = (uint32_t) (0 - ((val & 1) != 0));
                bit1 = (uint32_t) (0 - ((val & 2) != 0));
                bit2 = (uint32_t) (0 - ((val & 4) != 0));
                bit3 = (uint32_t) (0 - ((val & 8) != 0));
                bit4 = (uint32_t) (0 - ((val & 16) != 0));
                bit5 = (uint32_t) (0 - ((val & 32) != 0));
                break;
            case AVOL:
                ea = (uint32_t) ((val & 0x10) ? -1 : 0);
                va = ((val & 0x0F) * 2 + 1) & ~ea;
                break;
            case BVOL:
                eb = (uint32_t) ((val & 0x10) ? -1 : 0);
                vb = ((val & 0x0F) * 2 + 1) & ~eb;
                break;
            case CVOL:
                ec = (uint32_t) ((val & 0x10) ? -1 : 0);
                vc = ((val & 0x0F) * 2 + 1) & ~ec;
                break;
            case EFINE:
            case ECOARSE:
                fe = (opts[AY_EFINE] | (opts[AY_ECOARSE] << 8));
                break;
            case ESHAPE:
                te = 0; /* attack/decay */ if(opts[AY_ESHAPE] & 4) env = 0, denv = 1; else env = 31, denv = -1;
                break;
        }
    }
}

int zxDevAY::update(int clock_rate) {
    clock_rate     = zxSpeccy::machine->cpuClock * (opts[ZX_PROP_TURBO_MODE] ? 2 : 1);
    auto clockAY   = zxSpeccy::machine->ayClock * (opts[ZX_PROP_TURBO_MODE] ? 2 : 1);
    auto chip      = opts[ZX_PROP_SND_CHIP_AY];
    auto chan      = opts[ZX_PROP_SND_CHANNEL_AY];
    auto vol       = (int)opts[ZX_PROP_SND_VOLUME_AY];

    auto voltab = &chipVol[chip * 32];
    auto stereo = &stereoMode[chan * 6];

    system_clock_rate = (uint32_t)clock_rate;
    chip_clock_rate = clockAY >> 3;
    mult_const = (uint32_t)(((uint64_t)chip_clock_rate << MULT_C_1) / system_clock_rate);
    zxDevSound::update(chip_clock_rate);
    passed_chip_ticks = passed_clk_ticks = 0;
    t = 0; ns = 0xFFFF;
    vol = vol * 2184 + 7;
    for (int j = 0; j < 6; j++)
        for (int i = 0; i < 32; i++)
            vols[j][i] = (uint32_t)(((uint64_t)vol * voltab[i] * stereo[j]) / (65535 * 100 * 3));
    return 0;
}

void zxDevAY::reset() {
    for(int i = 0; i < 16; i++) opts[AY_AFINE + i] = 0;
    applyRegs(0);
}

void zxDevAY::applyRegs(uint32_t timestamp) {
    for(uint8_t r = 0; r < 16; r++) {
        select(r);
        auto p = opts[AY_AFINE + r];
        _write(timestamp, (uint8_t)(p ^ 1));
        _write(timestamp, p);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//                               ЗВУКОВОЙ СОПРОЦЕССОР YM                                       //
/////////////////////////////////////////////////////////////////////////////////////////////////

#define CLOCK_RESET(clock)  tickAY = (int)(65536. * clock / frequency)
#define GEN_STEREO(pos, val)                                        \
    if((pos) < 0) {                                                 \
        rstereobuf_l[rstereopos] += (val);                          \
        rstereobuf_r[(rstereopos - pos) % STEREO_BUF_SIZE] += (val);\
    } else {								                        \
        rstereobuf_l[(rstereopos + pos) % STEREO_BUF_SIZE] += (val);\
        rstereobuf_r[rstereopos] += (val); }

bool zxDevYM::checkRead(uint16_t port) const {
    return (port & 0xC0FF) == 0xC0FD;
}

bool zxDevYM::checkWrite(uint16_t port) const {
    if(port & 2) return false;
    if((port & 0xC0FF) == 0xC0FD) return true;
    return (port & 0xC000) == 0x8000;
}

void zxDevYM::write(uint16_t port, uint8_t val) {
	if(opts[ZX_PROP_SND_CHIP_AY]) {
		if((port & 0xC0FF) == 0xC0FD) opts[AY_REG] = (uint8_t) (val & 0x0F);
		if((port & 0xC000) == 0x8000) _write(val, *zxDevUla::_TICK);
	}
}

void zxDevYM::read(uint16_t, uint8_t* ret) {
	if(opts[ZX_PROP_SND_CHIP_AY]) *ret = opts[AY_AFINE + opts[AY_REG]];
}

int zxDevYM::update(int) {
    static int levels[16] = {
            0x0000, 0x0385, 0x053D, 0x0770, 0x0AD7, 0x0FD5, 0x15B0, 0x230C,
            0x2B4C, 0x43C1, 0x5A4B, 0x732F, 0x9204, 0xAFF1, 0xD921, 0xFFFF
    };

    reset();

    auto turbo      = opts[ZX_PROP_TURBO_MODE] ? 2 : 1;
    tsmax           = zxSpeccy::machine->cpuClock * turbo;
    clockAY         = zxSpeccy::machine->ayClock * turbo;

    auto modeAy     = opts[ZX_PROP_SND_CHANNEL_AY];
    auto vAy        = opts[ZX_PROP_SND_VOLUME_AY] * 256;
    auto freq       = frequencies[opts[ZX_PROP_SND_FREQUENCY]];

    frequency = freq;
    sound_stereo_ay = modeAy;
    for (int f = 0; f < 16; f++) toneLevels[f] = (uint32_t) ((levels[f] * vAy + 0x8000) / 0xffff);
    CLOCK_RESET(clockAY);

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
    int pos = 3 * frequency / 8000;
    rstereopos = 0; rchan1pos = -pos;
    if(sound_stereo_ay == 1) {
        rchan2pos = 0; rchan3pos = pos;
    } else if(sound_stereo_ay == 2) {
        rchan2pos = pos; rchan3pos = 0;
    }
    return 0;
}

void zxDevYM::reset() {
    int f;

    for(f = 0; f < 16; f++) write((uint8_t)f, 0);
    for(f = 0; f < 3; f++) toneHigh[f] = 0;
    toneSubCycles = envSubCycles = 0;
    CLOCK_RESET(clockAY);
}

void zxDevYM::_write(uint8_t val, uint32_t tick) {
	if(zx->snd && zx->snd->isAyEnable) {
		auto reg = opts[AY_REG];
		if(reg < 16) {
			opts[reg + AY_AFINE] = val;
			if(reg < 14) {
				if(countSamplers < AY_SAMPLERS) {
					samplers[countSamplers].tstates = tick;
					samplers[countSamplers].reg = reg;
					samplers[countSamplers].val = val;
					countSamplers++;
				}
			}
		}
	}
}

void* zxDevYM::ptrData() {
    static int rng = 1;
    static int noise_toggle = 0;
    static int envFirst = 1, envRev = 0, envCounter = 15;
    int toneLevel[3], chans[3];
    int mixer, envshape;
    int f, g, level, count;
    signed short *ptr;
    auto change_ptr = &samplers[0];
    int changes_left = countSamplers;
    int reg, r;
    uint32_t tone_count, noise_count;
    memset(buffer, 0, sndBufSize * 2 * sizeof(short));
    if(!countSamplers) return buffer;
    for(f = 0; f < countSamplers; f++) samplers[f].ofs = (uint16_t)((samplers[f].tstates * frequency) / tsmax);
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
        toneSubCycles &= 524287;//(8 << 16) - 1;
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
            GEN_STEREO(rchan1pos, chans[0]);
            GEN_STEREO(rchan2pos, chans[1]);
            GEN_STEREO(rchan3pos, chans[2]);
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

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ДИНАМИК                                                //
/////////////////////////////////////////////////////////////////////////////////////////////////

void zxDevBeeper::write(uint16_t, uint8_t val) {
	if(zx->snd && zx->snd->isBpEnable) {
		auto spk = (short) (((val & 16) >> 4) * volSpk);
		auto mic = (short) (((val & 8) >> 3) * volMic);
		auto mono = (uint32_t) (spk + mic);
		updateData(zxDevUla::_ftick, mono, mono);
	}
}

int zxDevBeeper::update(int param) {
    param = opts[ZX_PROP_SND_VOLUME_BP];
    zxDevSound::update(zxSpeccy::machine->cpuClock * (opts[ZX_PROP_TURBO_MODE] ? 2 : 1));
    volSpk = (int)((2.5f * param) * 512);
    volMic = volSpk >> 2;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ДИСКОВОД                                               //
/////////////////////////////////////////////////////////////////////////////////////////////////

static char buf_str[65536];
// все дисководы
static zxFDD fdds[4];
// текущий сектор
static zxDisk::TRACK::SECTOR* found_sec(nullptr);

void zxDevBeta128::log_to_data(bool is_text, const char* title, int trk, int sec, int head) {
    auto s = &buf_str[0];
    auto data = _rwptr;
    auto len = rwptr - _rwptr;
    for(int i = 0 ; i < len; i++) {
        int dd = data[i];
        auto str = ssh_ntos(&dd, RADIX_HEX, nullptr);
        ssh_strcpy(&s, str);
        if(is_text) {
            auto vv = (uint8_t) ((dd < 32 || dd > 127) ? '.' : dd);
            *s++ = ',';
            *s++ = vv;
        }
        *s++ = ',';
    }
    *s++ = 0;
    zxFile::writeFile(title, buf_str, s - buf_str);
    LOG_INFO("%s t:%i s:%i h:%i -- %s", title, trk, sec, head, buf_str);
}

uint8_t* zxDevBeta128::state(uint8_t* ptr, bool restore) {
    if(restore) {
        time    = *zxDevUla::_TICK;
        ops     = *ptr++;
        nfdd    = *ptr++;
        fdd     = &fdds[nfdd];
        fdds[0].track(*ptr++); fdds[1].track(*ptr++);
        fdds[2].track(*ptr++); fdds[3].track(*ptr++);
        fdds[0].head = *ptr++; fdds[1].head = *ptr++;
        fdds[2].head = *ptr++; fdds[3].head = *ptr++;
        load();
        found_sec = (*ptr++ ? fdd->get_sec(fdd->track(), fdd->head, opts[TRDOS_SEC]) : nullptr);
        rwlen   = *(uint16_t*)(ptr +  0);
        crc     = *(uint16_t*)(ptr +  2);
        switch(ops) {
            case C_RTRK:
            case C_WTRK: _rwptr = fdd->get_trk()->content; break;
            case C_RSEC:
            case C_WSEC: _rwptr = found_sec->content; break;
            case C_RADR: _rwptr = found_sec->caption; break;
            default:     _rwptr = nullptr; break;
        }
        rwptr = (ops != 0 ? _rwptr + *(uint16_t*)(ptr + 4) : nullptr);
        start_crc = *(uint16_t*)(ptr + 6) ? rwptr + *(uint16_t*)(ptr + 8) : nullptr;
        next      = *(uint16_t*)(ptr + 10) | (*(uint16_t*)(ptr + 12) << 16);
        wait_drq  = *(uint16_t*)(ptr + 14) | (*(uint16_t*)(ptr + 16) << 16);
        direction = *(int8_t*)(ptr + 18);
        states     = *(ptr + 19);
        rqs       = *(ptr + 20);
        ptr       += 21;
        for(int i = 0 ; i < 4; i++) {
            uint32_t tmp = *(uint16_t*)(ptr + 0) | (*(uint16_t*)(ptr + 2) << 16);
            ptr += 4;
            fdds[i].engine(tmp);
        }
    } else {
        time = *zxDevUla::_TICK;
        *ptr++ = ops; *ptr++ = nfdd;
        *ptr++ = fdds[0].track(); *ptr++ = fdds[1].track();
        *ptr++ = fdds[2].track(); *ptr++ = fdds[3].track();
        *ptr++ = fdds[0].head; *ptr++ = fdds[1].head;
        *ptr++ = fdds[2].head; *ptr++ = fdds[3].head;
        *ptr++ = (uint8_t)(found_sec != nullptr);
        *(uint16_t*)(ptr +  0) = rwlen;
        *(uint16_t*)(ptr +  2) = crc;
        *(uint16_t*)(ptr +  4) = (uint16_t)(ops ? rwptr - _rwptr : 0);      // rwptr
        auto tmp = (uint32_t)(start_crc != nullptr);
        *(uint16_t*)(ptr +  6) = (uint16_t)(tmp);
        *(uint16_t*)(ptr +  8) = (uint16_t)(tmp ? start_crc - rwptr : 0);   // start_crc
        *(uint16_t*)(ptr + 10) = (uint16_t)(next & 0xFFFF);
        *(uint16_t*)(ptr + 12) = (uint16_t)(next >> 16);                    // next
        *(uint16_t*)(ptr + 14) = (uint16_t)(wait_drq & 0xFFFF);
        *(uint16_t*)(ptr + 16) = (uint16_t)(wait_drq >> 16);                // wait
        ptr += 18;
        *ptr++ = (uint8_t)direction;
        *ptr++ = states; *ptr++ = rqs;
        for(int i = 0 ; i < 4; i++) {
            tmp = fdds[i].engine();
            *(uint16_t*)(ptr + 0) = (uint16_t)(tmp & 0xFFFF);
            *(uint16_t*)(ptr + 2) = (uint16_t)(tmp >> 16);
            ptr += 4;
        }
    }
    return ptr;
}

int zxDevBeta128::update(int) {
    Z80FQ = zxSpeccy::machine->cpuClock;
    return 0;
}

zxDevBeta128::zxDevBeta128() : next(0), direction(0), rqs(R_NONE), states(S_IDLE), nfdd(0), wait_drq(0), ops(0),
                               rwptr(nullptr), rwlen(0), crc(0), start_crc(nullptr) {
    reset();
}

bool zxDevBeta128::open(uint8_t* ptr, size_t size, int type) {
    auto drive = opts[ZX_PROP_JNI_RETURN_VALUE];
    if(drive >= 0 && drive < 4) {
        if (drive == nfdd) {
            found_sec = nullptr;
            opts[TRDOS_STS] = ST_NOTRDY;
            rqs = R_INTRQ; states = S_IDLE;
        }
        return fdds[drive].open(ptr, size, type);
    }
    return false;
}

uint8_t* zxDevBeta128::save(int type) {
    auto drive = opts[ZX_PROP_JNI_RETURN_VALUE];
    if(drive >= 0 && drive < 4) {
        return fdds[drive].save(type);
    }
    return nullptr;
}

uint16_t zxDevBeta128::CRC(uint8_t v, uint16_t prev_crc) const {
    auto c = prev_crc ^ (v << 8);
    for(int i = 8; i; --i) { if((c <<= 1) & 0x10000) c ^= 0x1021; }
    return c;
}

uint16_t zxDevBeta128::CRC(uint8_t* src, int size) const {
    auto c = (uint16_t)0xcdb4;
    while(size--) c = CRC(*src++, c);
    return c;
}

void zxDevBeta128::exec() {
    time = *zxDevUla::_TICK;
    // Неактивные диски игнорируют бит HLT
    if(time > fdd->engine() && (opts[TRDOS_SYS] & CB_SYS_HLT)) fdd->engine(0);
    fdd->is_disk() ? opts[TRDOS_STS] &= ~ST_NOTRDY : opts[TRDOS_STS] |= ST_NOTRDY;
    // команды позиционирования
    if(!(opts[TRDOS_CMD] & 0x80)) {
        opts[TRDOS_STS] &= ~(ST_TRK00 | ST_INDEX | ST_HEADL);
        if(fdd->engine() && (opts[TRDOS_SYS] & CB_SYS_HLT)) opts[TRDOS_STS] |= ST_HEADL;
        if(!fdd->track()) opts[TRDOS_STS] |= ST_TRK00;
        // индексный импульс - чередуюется каждые 4 мс(если диск присутствует)
        if(fdd->is_disk() && fdd->engine() && (time % (Z80FQ / FDD_RPS) < (Z80FQ * 4 / 1000))) opts[TRDOS_STS] |= ST_INDEX;
    }
    while(true) {
        switch(states & 15) {
            // шина свободна. команда завершена
            case S_IDLE: opts[TRDOS_STS] &= ~ST_BUSY; rqs = R_INTRQ; return;
                // задержка при выполении команды
            case S_WAIT: if(time < next) return; states = _ST_NEXT; break;
                // подготовка к чтению/записи
            case S_PREPARE_CMD: cmdPrepareRW(); break;
                // подготовка операции чтения/записи
            case S_CMD_RW: cmdReadWrite(); break;
                // операция чтения(сектора, адреса, дорожки)
            case S_READ: cmdRead(); break;
                // нашли сектор - запускаем операцию
            case S_FIND_SEC: cmdPrepareSec(); break;
                // запись сектора
            case S_WRSEC: cmdWriteSector(); break;
                // запись дорожки
            case S_WRTRACK: cmdWriteTrack(); break;
                // операция записи
            case S_WRITE: cmdWrite(); break;
                // запись данных дорожки
            case S_WR_TRACK_DATA: cmdWriteTrackData(); break;
                // команды позиционирования
            case S_TYPE1_CMD: cmdType1(); break;
            case S_STEP: cmdStep(); break;
            case S_SEEKSTART: if(!(opts[TRDOS_CMD] & CB_SEEK_TRKUPD)) { opts[TRDOS_TRK] = 0xff; opts[TRDOS_DAT] = 0; }
            case S_SEEK: cmdSeek(); break;
            case S_VERIFY: cmdVerify(); break;
        }
    }
}

void zxDevBeta128::read_byte(){
    opts[TRDOS_DAT] = *rwptr++; rwlen--;
    crc = CRC(opts[TRDOS_DAT], crc);
    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
    next += fdd->ticks();
    _ST_SET(S_WAIT, S_READ);
}

bool zxDevBeta128::ready() {
    // Fdc слишком быстр в режиме без задержки, подождите, пока CPU обработает DRQ, но не больше 'wait_drq'
    if(!(rqs & R_DRQ)) return true;
    if(next > wait_drq) return true;
    next = time + fdd->ticks();
    _ST_SET(S_WAIT, states);
    return false;
}

void zxDevBeta128::cmdRead() {
    if(!ready()) return;
    if(rwlen) {
        if(rqs & R_DRQ) opts[TRDOS_STS] |= ST_LOST;
        read_byte();
    } else {
        LOG_DEBUG("S_READ FINISH", 0);
        ops = 0;
//        log_to_data(false, "read_sector", found_sec->trk(), found_sec->sec(), found_sec->head());
        if((opts[TRDOS_CMD] & 0xe0) == C_RSEC) {
            // если чтение сектора - проверяем на CRC
            if (crc != found_sec->crcContent()) {
                opts[TRDOS_STS] |= ST_CRCERR;
            }
            // если множественная загрузка секторов
            if (opts[TRDOS_CMD] & CB_MULTIPLE) {
                opts[TRDOS_SEC]++; states = S_CMD_RW;
                return;
            }
        } else if ((opts[TRDOS_CMD] & 0xf0) == C_RADR) {
            // проверяем на CRC
            if (CRC(found_sec->caption - 1, 5) != found_sec->crcCaption()) {
                opts[TRDOS_STS] |= ST_CRCERR;
            }
        }
        states = S_IDLE;
    }
}

void zxDevBeta128::cmdWrite() {
    if(!ready()) return;
    if(rqs & R_DRQ) {
        // потеря данных
        opts[TRDOS_STS] |= ST_LOST;
        opts[TRDOS_DAT] = 0;
    }
    // запись байта
    *rwptr++ = opts[TRDOS_DAT]; rwlen--;
    crc = CRC(opts[TRDOS_DAT], crc);
    if(rwlen) {
        next += fdd->ticks();
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WRITE);
    } else {
        // запись CRC
        *rwptr++ = (uint8_t)(crc >> 8); *rwptr++ = (uint8_t)(crc);
        // завершение операции
        LOG_DEBUG("S_WRITE FINISH", 0);
//        log_to_data(false, "write_sector", found_sec->trk(), found_sec->sec(), found_sec->head());
        found_sec = nullptr; ops = 0;
        // проверка на множественные сектора
        if(opts[TRDOS_CMD] & CB_MULTIPLE) { opts[TRDOS_SEC]++; states = S_CMD_RW; } else states = S_IDLE;
    }
}

void zxDevBeta128::cmdWriteTrackData() {
    if(!ready()) return;
    // потеря данных
    if(rqs & R_DRQ) {
        opts[TRDOS_STS] |= ST_LOST;
        opts[TRDOS_DAT] = 0;
    }
    auto d = opts[TRDOS_DAT]; auto v = d;
    if(d == 0xF5) { v = 0xA1; }
    else if(d == 0xF6) { v = 0xC2; }
    else if(d == 0xFB || d == 0xFE) { start_crc = rwptr; }
    else if(d == 0xF7) {
        // считаем КК
        crc = CRC(start_crc, (int)(rwptr - start_crc));
        *rwptr++ = (uint8_t)(crc >> 8); rwlen--; v = (uint8_t)crc;
        start_crc = nullptr;
    }
    *rwptr++ = v; rwlen--;
    if(rwlen) {
        next += fdd->ticks();
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WR_TRACK_DATA);
    } else {
        LOG_DEBUG("S_WR_TRACK_DATA FINISH", 0);
        log_to_data(false, "write_track", opts[TRDOS_TRK], opts[TRDOS_SEC], opts[TRDOS_HEAD]);
        rwptr = nullptr;
        fdd->get_trk()->update(); states = S_IDLE;
    }
}

void zxDevBeta128::cmdWriteSector() {
    if(rqs & R_DRQ) {
        opts[TRDOS_STS] |= ST_LOST;
        states = S_IDLE;
    } else {
        _rwptr = rwptr = found_sec->content; ops = C_WSEC;
        auto dat = (uint8_t) ((opts[TRDOS_CMD] & CB_WRITE_DEL) ? 0xF8 : 0xFB);
        rwptr[-1] = dat; rwlen = found_sec->len();
        crc = CRC(dat); states = S_WRITE;
        LOG_DEBUG("S_WSEC idx:%i len:%i trk:%i sec:%i head:%i marker:%X",
                  rwptr - fdd->get_trk()->content, rwlen, found_sec->trk(), found_sec->sec(), found_sec->head(), dat);
    }
}

void zxDevBeta128::get_index(int s_next) {
    auto t = fdd->get_trk();
    auto trlen = (uint32_t)(t->len * fdd->ticks());
    auto ticks = (uint32_t)(next % trlen);
    next += (trlen - ticks);
    _rwptr = rwptr = t->content; rwlen = t->len;
    _ST_SET(S_WAIT, s_next);
}

void zxDevBeta128::load() {
    fdd->seek(fdd->track(), opts[TRDOS_HEAD]);
}

void zxDevBeta128::cmdWriteTrack() {
    if(rqs & R_DRQ) {
        opts[TRDOS_STS] |= ST_LOST;
        states = S_IDLE;
    } else {
        start_crc = nullptr;
        get_index(S_WR_TRACK_DATA); ops = C_WTRK;
        wait_drq = next + 5 * Z80FQ / FDD_RPS;
        LOG_DEBUG("S_WRITE_TRACK idx:%i len:%i trk:%i sec:%i head:%i",
                  rwptr - fdd->get_trk()->content, rwlen, opts[TRDOS_TRK], opts[TRDOS_SEC], opts[TRDOS_HEAD]);
    }
}

void zxDevBeta128::cmdReadWrite() {
    load();
    auto cmd = opts[TRDOS_CMD];
    if(((cmd & 0xe0) == C_WSEC || (cmd & 0xf0) == C_WTRK) && fdd->is_protect()) {
        // проверка на защиту от записи в операциях записи
        opts[TRDOS_STS] |= ST_WRITEP;
        states = S_IDLE;
    } else if((cmd & 0xc0) == 0x80 || (cmd & 0xf8) == C_RADR) {
        // операции чтения/записи секторов или чтения адреса - поиск сектора
        wait_drq = next + 5 * Z80FQ / FDD_RPS;
        find_sec();
    } else if((cmd & 0xf8) == C_WTRK) {
        // запись дорожки(форматирование)
        next += 3 * fdd->ticks();
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WRTRACK);
    } else if((cmd & 0xf8) == C_RTRK) {
        // чтение дорожки
        LOG_DEBUG("S_RW_CMD - C_RTRK", 0);
        get_index(S_READ); ops = C_RTRK;
    } else {
        states = S_IDLE;
    }
}

void zxDevBeta128::cmdType1() {
//    LOG_INFO("S_TYPE1_CMD cmd:%i", opts[TRDOS_CMD]);
    opts[TRDOS_STS] = (opts[TRDOS_STS] | ST_BUSY) & ~(ST_DRQ | ST_CRCERR | ST_SEEKERR | ST_WRITEP);
    rqs = R_NONE;
    if(fdd->is_protect()) opts[TRDOS_STS] |= ST_WRITEP;
    fdd->engine(next + 2 * Z80FQ);
    // поиск/восстановление
    auto cmd = S_SEEKSTART;
    if(opts[TRDOS_CMD] & 0xE0) {
        // один шаг
        if(opts[TRDOS_CMD] & 0x40) direction = (uint8_t)((opts[TRDOS_CMD] & CB_SEEK_DIR) ? -1 : 1);
        cmd = S_STEP;
    }
    _ST_SET(S_WAIT, cmd);
    next += Z80FQ / 1000;
}

void zxDevBeta128::cmdPrepareSec() {
    load();
    auto cmd = opts[TRDOS_CMD];
    if ((cmd & 0xf0) == C_RADR) {
        // чтение адресного маркера
        //opts[TRDOS_SEC] = opts[TRDOS_TRK];
        _rwptr = rwptr = found_sec->caption; rwlen = 6; ops = C_RADR;
        auto marker = rwptr[-1];
        LOG_DEBUG("S_RARD idx:%i len:%i trk:%i sec:%i head:%i marker:%X",
                  rwptr - fdd->get_trk()->content, rwlen, found_sec->trk(), found_sec->sec(), found_sec->head(), marker);
        crc = CRC(marker); read_byte();
    } else if (cmd & 0x20) {
        // запись сектора
        next += fdd->ticks() * 9;
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WRSEC);
    } else {
        // чтение сектора
        _rwptr = rwptr = found_sec->content; rwlen = found_sec->len(); ops = C_RSEC;
        auto marker = rwptr[-1];
        if(marker == 0xF8) opts[TRDOS_STS] |= ST_RECORDT; else opts[TRDOS_STS] &= ~ST_RECORDT;
        LOG_DEBUG("S_RSEC idx:%i len:%i trk:%i sec:%i head:%i marker:%X",
                  rwptr - fdd->get_trk()->content, rwlen, found_sec->trk(), found_sec->sec(), found_sec->head(), marker);
        crc = CRC(marker); read_byte();
    }
}

void zxDevBeta128::cmdPrepareRW() {
    LOG_DEBUG("S_PREPARE_CMD cmd:%i trk:%i sec:%i head:%i 15ms:%i",
              opts[TRDOS_CMD], opts[TRDOS_TRK], opts[TRDOS_SEC], opts[TRDOS_HEAD], opts[TRDOS_CMD] & CB_DELAY);
    if(opts[TRDOS_CMD] & CB_DELAY) next += (Z80FQ * 15 / 1000);
    // сброс статуса
    opts[TRDOS_STS] = (opts[TRDOS_STS] | ST_BUSY) & ~(ST_DRQ | ST_LOST | ST_NOT_SEC | ST_RECORDT | ST_WRITEP);
    _ST_SET(S_WAIT, S_CMD_RW);
}

void zxDevBeta128::cmdStep() {
    //static const uint32_t steps[] = { 6, 12, 20, 30 };
    auto cmd = opts[TRDOS_CMD];
    if(fdd->track() == 0 && (cmd& 0xf0) == 0) {
        // RESTORE
        opts[TRDOS_TRK] = 0;
        states = S_VERIFY;
    } else {
        // позиционирование дорожки
        if(!(cmd & 0xe0) || (cmd & CB_SEEK_TRKUPD)) opts[TRDOS_TRK] += direction;
        // проверка на допустимые дорожки
        int cyl = fdd->track() + direction; if(cyl < 0) cyl = 0; if(cyl >= PHYS_CYL) cyl = PHYS_CYL - 1;
        fdd->track(cyl);
        next += Z80FQ / 1000;
        _ST_SET(S_WAIT, (cmd & 0xe0) ? S_VERIFY : S_SEEK);
    }
}

void zxDevBeta128::cmdSeek() {
    if(opts[TRDOS_DAT] == opts[TRDOS_TRK]) {
        states = S_VERIFY;
    } else {
        direction = (uint8_t) ((opts[TRDOS_DAT] < opts[TRDOS_TRK]) ? -1 : 1);
        states = S_STEP;
    }
}

void zxDevBeta128::cmdVerify() {
    // для проверки существования сектора или правильности КК
    if(opts[TRDOS_CMD] & CB_SEEK_VERIFY) find_sec();
    states = S_IDLE;
}

void zxDevBeta128::find_sec() {
    load();
    found_sec = nullptr;
    auto wait = 10 * Z80FQ / FDD_RPS;
    opts[TRDOS_STS] &= ~ST_CRCERR;
    if(fdd->engine() && fdd->is_disk()) {
        auto t = fdd->get_trk();
        for (int i = 0; i < t->total_sec; ++i) {
            auto sec = fdd->get_sec(i);
            if(sec->sec() == opts[TRDOS_SEC]) {
                found_sec = sec;
                wait = (uint32_t)((sec->caption - t->content) * fdd->ticks());
                break;
            }
        }
    }
    if(found_sec) {
        if(CRC(found_sec->caption - 1, 5) == found_sec->crcCaption()) {
            next += wait;
            _ST_SET(S_WAIT, S_FIND_SEC);
            return;
        }
        opts[TRDOS_STS] |= ST_CRCERR;
    } else opts[TRDOS_STS] |= ST_NOT_SEC;
    states = S_IDLE;
    found_sec = nullptr;
}

bool zxDevBeta128::checkRead(uint16_t port) const {
    if((port & 0x1F) != 0x1F) return false;
    auto p = (uint8_t)port;
    return p == 0x1F || p == 0x3F || p == 0x5F || p == 0x7F || p & 0x80;
}

bool zxDevBeta128::checkWrite(uint16_t port) const {
    if((port & 0x1F) != 0x1F) return false;
    auto p = (uint8_t)port;
    return p == 0x1F || p == 0x3F || p == 0x5F || p == 0x7F || p & 0x80;
}

void zxDevBeta128::read(uint16_t port, uint8_t* ret) {
    if(!checkSTATE(ZX_TRDOS)) return;
    exec();
    if(port == 0x1F) {
        rqs &= ~R_INTRQ;
        *ret = opts[TRDOS_STS];
    } else if(port == 0x3F) {
        *ret = opts[TRDOS_TRK];
    } else if(port == 0x5F) {
        *ret = opts[TRDOS_SEC];
    } else if(port == 0x7F) {
        *ret = opts[TRDOS_DAT];
        opts[TRDOS_STS] &= ~ST_DRQ;
        rqs &= ~R_DRQ;
    } else if(port & 0x80) {
        *ret = (uint8_t)(rqs | 0x3F);
    }
}

void zxDevBeta128::write(uint16_t port, uint8_t v) {
    exec();
    if(port == 0x1F) {
        // прерывание команды
        if((v & 0xf0) == C_INTERRUPT) {
            states = S_IDLE; rqs = R_INTRQ; opts[TRDOS_STS] &= ~ST_BUSY;
            return;
        }
        if(opts[TRDOS_STS] & ST_BUSY) return;
        opts[TRDOS_CMD] = v;
        next = *zxDevUla::_TICK;
        opts[TRDOS_STS] |= ST_BUSY; rqs = R_NONE;
        // команды чтения/записи
        if(opts[TRDOS_CMD] & 0x80) {
            // выйти, если нет диска
            if(opts[TRDOS_STS] & ST_NOTRDY) { states = S_IDLE; rqs = R_INTRQ; return; }
            // продолжить вращать диск
            if(fdd->engine()) fdd->engine(next + 2 * Z80FQ);
            states = S_PREPARE_CMD;
        } else {
            // для команд поиска/шага
            states = S_TYPE1_CMD;
        }
    } else if(port == 0x3F) {
        opts[TRDOS_TRK] = v;
    } else if(port == 0x5F) {
        opts[TRDOS_SEC] = v;
    } else if(port == 0x7F) {
        opts[TRDOS_DAT] = v;
        rqs &= ~R_DRQ;
        opts[TRDOS_STS] &= ~ST_DRQ;
    } else if(port & 0x80) {
        // сброс контроллера
        if(!(v & CB_RESET)) reset();
        opts[TRDOS_SYS] = v;
        nfdd = (uint8_t)(v & 3);
        fdd = &fdds[nfdd];
        opts[TRDOS_HEAD] = (uint8_t)((v & 0x10) == 0);
//        LOG_INFO(("WRITE SYSTEM: HEAD %i drive:%i fdd:%X"), opts[TRDOS_HEAD], nfdd, fdd);
    }
}

void zxDevBeta128::reset() {
    fdd = &fdds[nfdd];
    update();
    found_sec = nullptr;
    opts[TRDOS_SEC] = 1;
    opts[TRDOS_STS] = ST_NOTRDY;
    opts[TRDOS_HEAD] = opts[TRDOS_SYS] = opts[TRDOS_CMD] = opts[TRDOS_DAT] = opts[TRDOS_TRK] = 0;
    rqs = R_INTRQ; fdd->engine(0);
    _ST_SET(S_IDLE, S_IDLE);
}

// извлечь
void zxDevBeta128::eject(int num) {
    fdds[num].eject();
}

int zxDevBeta128::is_readonly(int num, int write) {
    if(write != -1) fdds[num].protect = write != 0;
    return fdds[num].is_protect();
}

int zxDevBeta128::read_sector(int num, int sec) {
    auto sector = fdds[num].get_sec(0, 0, sec);
    if(sector) memcpy(&opts[ZX_PROP_VALUES_SECTOR], sector->content, 256);
    else memset(&opts[ZX_PROP_VALUES_SECTOR], 0, 256);
    return sector != nullptr;
}

void zxDevBeta128::trap(uint16_t pc) {
    static uint32_t addr_func[] = {
            0x3D98, 0x3DCB, 0x3E63, 0x3F02, 0x3F06, 0x1E3D,
            0x1E4D, 0x28D8, 0x165C, 0x1664, 0x1CF0, 0x28FB,
            0x28F2, 0x01D3, 0x290F, 0x01D3, 0x01D3, 0x01D3,
            0x2926, 0x28E0, 0x28E3, 0x2739, 0x1FEB, 0x1FF6,
            0x0405 };
    for(int i = 0 ; i < 25; i++) {
        auto a = addr_func[i];
        if(a == pc) {
            LOG_INFO("cmd %i PC:%X %X%X%X%X%X%X%X%X", i, pc, rm8(23773), rm8(23774), rm8(23775), rm8(23776), rm8(23777), rm8(23778), rm8(23779), rm8(23780));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                      МАГНИТОФОН                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

void zxDevTape::closeTape() {
    for(int i = 0 ; i < countBlocks; i++) {
        SAFE_A_DELETE(blocks[i]->data);
    }
    countBlocks = 0;
    reset();
}

void zxDevTape::makeBlock(uint8_t type, uint8_t* data, uint16_t size, uint16_t pilot_t, uint16_t s1_t, uint16_t s2_t, uint16_t zero_t,
                          uint16_t one_t, uint16_t pilot_len, uint16_t pause, uint8_t last) {
    auto blk = new TAPE_BLOCK();
    blocks[countBlocks++] = blk;
    blk->type = type;
    blk->data = new uint8_t[size]; memcpy(blk->data, data, size);
    blk->data_size = size;
    blk->pilot_len = pilot_len; blk->pilot_t = pilot_t;
    blk->s1_t = s1_t; blk->s2_t = s2_t;
    blk->zero_t = zero_t; blk->one_t = one_t;
    blk->pause = pause; blk->last = last;
}

uint8_t zxDevTape::tapeBit() {
    uint32_t cur = *zxDevUla::_TICK;
    while(cur > edge_change) {
        if((int)(cur - edge_change) >= 0) write(0, tape_bit >> 3);
        tape_bit ^= 0x40;
        edge_change += getImpulse();
    }
    return tape_bit;
}

uint32_t zxDevTape::getImpulse() {
    auto blk = blocks[currentBlock];
    uint32_t impulse = 0, idx = 0;
    type_impulse++;
    auto len = (uint32_t)(blk->pilot_len - 1);
    if(type_impulse <= len) impulse = blk->pilot_t;
    else if(type_impulse == (len + 1)) impulse = blk->s1_t;
    else if(type_impulse == (len + 2)) impulse = blk->s2_t;
    else if(type_impulse < ((len + 3) + blk->data_size * 16)) {
        // данные
        // длина на 0-й бит(855) и на 1-й бит(1710)
        idx = type_impulse - (len + 3);
        impulse = (blk->data[idx / 16] & numBits[7 - ((idx % 16) >> 1)]) ? blk->one_t : blk->zero_t;
    } else {
        // next_block
        type_impulse = 0;
        currentBlock++;
        // pause
        impulse = (uint32_t)(blk->pause * 3500);
    }
    return impulse;
}

void zxDevTape::setCurrentBlock(int index) {
	if(index >= countBlocks) index = countBlocks - 1;
	else if(index < 0) index = 0;
	currentBlock = (uint8_t)index;
	stopTape();
}

bool zxDevTape::open(uint8_t* ptr, size_t size, int type) {
	switch(type) {
		case ZX_CMD_IO_TAP:
			return zxFormats::openTAP(this, ptr, size);
		case ZX_CMD_IO_TZX:
			return zxFormats::openTZX(this, ptr, size);
		case ZX_CMD_IO_CSW:
			return zxFormats::openCSW(this, ptr, size);
	}
	return false;
}

uint8_t* zxDevTape::state(uint8_t* ptr, bool restore) {
	if(restore) {
		if(ptr[0] != 'S' && ptr[1] != 'E' && ptr[2] != 'R' && ptr[3] != 'G') return ptr;
		ptr += 4;
		auto cblks = ptr[0];
		currentBlock = ptr[1]; tape_bit = ptr[2];
		type_impulse = ptr[3] | (ptr[4] << 8) | (ptr[5] << 16) | (ptr[6] << 24);
		edge_change = ptr[7] | (ptr[8] << 8) | (ptr[9] << 16) | (ptr[10] << 24);
		ptr += 11;
		for(int i = 0 ; i < cblks; i++) {
			auto pilot_t    = *(uint16_t*)(ptr + 0);
            auto s1_t       = *(uint16_t*)(ptr + 2);
            auto s2_t       = *(uint16_t*)(ptr + 4);
            auto zero_t     = *(uint16_t*)(ptr + 6);
            auto one_t      = *(uint16_t*)(ptr + 8);
            auto pilot_len  = *(uint16_t*)(ptr + 10);
            auto pause_t    = *(uint16_t*)(ptr + 12);
            auto size       = *(uint16_t*)(ptr + 14);
            auto last_t     = ptr[16];
            auto type       = ptr[17]; ptr += 18;
            auto data       = ptr; ptr += size;
			makeBlock(type, data, size, pilot_t, s1_t, s2_t, zero_t, one_t, pilot_len, pause_t, (uint8_t)last_t);
		}
	} else {
		ptr[0]  = 'S'; ptr[1] = 'E'; ptr[2] = 'R'; ptr[3] = 'G'; ptr += 4;
		ptr[0]  = countBlocks; ptr[1]  = currentBlock; ptr[2] = tape_bit;
        ptr[3]  = (uint8_t)(type_impulse & 0xFF);
        ptr[4]  = (uint8_t)((type_impulse >> 8) & 0xFF);
        ptr[5]  = (uint8_t)((type_impulse >> 16) & 0xFF);
        ptr[6]  = (uint8_t)((type_impulse >> 24) & 0xFF);
		ptr[7]  = (uint8_t)(edge_change & 0xFF);
		ptr[8]  = (uint8_t)((edge_change >> 8)  & 0xFF);
		ptr[9]  = (uint8_t)((edge_change >> 16) & 0xFF);
		ptr[10]  = (uint8_t)((edge_change >> 24) & 0xFF);
		ptr += 11;
		for(int i = 0 ; i < countBlocks; i++) {
			auto blk = blocks[i];
            *(uint16_t*)(ptr + 0)  = blk->pilot_t;
            *(uint16_t*)(ptr + 2)  = blk->s1_t;
            *(uint16_t*)(ptr + 4)  = blk->s2_t;
            *(uint16_t*)(ptr + 6)  = blk->zero_t;
            *(uint16_t*)(ptr + 8)  = blk->one_t;
            *(uint16_t*)(ptr + 10) = blk->pilot_len;
            *(uint16_t*)(ptr + 12) = blk->pause;
            *(uint16_t*)(ptr + 14) = blk->data_size;
            *(ptr + 16) = blk->last; *(ptr + 17) = blk->type;
			ptr += 18; memcpy(ptr, blk->data, blk->data_size);
			ptr += blk->data_size;
		}
	}
    return ptr;
}

int zxDevTape::trapSave() {
	if(opts[ZX_PROP_TRAP_TAPE]) {
		auto cpu = zx->cpu;
		auto a = *cpu->_A;
		auto len = (uint16_t) (*cpu->_DE + 2);
		auto ix = *cpu->_IX;

		TMP_BUF[0] = a;
		for(int i = 0; i < len - 2; i++) {
			auto b = rm8((uint16_t) (ix + i));
			a ^= b;
			TMP_BUF[i + 1] = b;
		}
		TMP_BUF[len - 1] = a;
//    addBlock(TMP_BUF, len);
		return 1;
	}
	return 0;
}

int zxDevTape::trapLoad() {
    if (opts[ZX_PROP_TRAP_TAPE] && currentBlock < countBlocks) {
        auto blk = blocks[currentBlock];
        auto len = blk->data_size - 2;
        auto data = blk->data + 1;

        auto cpu = zx->cpu;
        auto de = *cpu->_DE;
        auto ix = *cpu->_IX;
        if (de != len) LOG_INFO("В перехватчике LOAD отличаются блоки - block: %i (DE: %i != SIZE: %i)!", currentBlock, de, len);
        if (de < len) len = de;
        for (int i = 0; i < len; i++) ::wm8(realPtr((uint16_t) (ix + i)), data[i]);
        LOG_DEBUG("trapLoad PC: %i load: %i type: %i addr: %i size: %i", zxSpeccy::PC, *cpu->_F & 1, *cpu->_A, ix, len);
        *cpu->_AF = 0x00B3;
        *cpu->_BC = 0xB001;
        opts[_RH] = 0; opts[_RL] = data[len];
		currentBlock++;
        zx->pauseBetweenTapeBlocks = 50;
        modifySTATE(ZX_PAUSE, 0);
        return 1;
    }
    startTape();
    return 0;
}

/*
Формат стандартного заголовочного блока Бейсика такой:
1 байт  - флаговый, для блока заголовка всегда равен 0 (для блока данных за ним равен 255)
1 байт  - тип Бейсик блока, 0 - бейсик программа, 1 - числовой массив, 2 - символьный массив, 3 - кодовый блок
10 байт - имя блока
2 байта - длина блока данных, следующего за заголовком (без флагового байта и байта контрольной суммы)
2 байта - Параметр 1, для Бейсик-программы - номер стартовой строки Бейсик-программы, заданный параметром LINE (или число >=32768,
            если стартовая строка не была задана.
            Для кодового блока - начальный адрес блока в памяти.
            Для массивов данных - 14й-байт хранит односимвольное имя массива
2 байта - Параметр 2. Для Бейсик-программы - хранит размер собственно Бейсик-програмы, без инициализированных переменных,
            хранящихся в памяти на момент записи Бейсик-программы.
            Для остальных блоков содержимое этого параметра не значимо, и я почти уверен, что это не два байта ПЗУ.
            Скорее всего, они просто не инициализируются при записи.
1 байт - контрольная сумма заголовочного блока.
*/
// 0 - длина блока
// 1 - контрольная сумма
// 2 - флаг(0/255)
// 3 - тип блока(0-3)
// 4 - длина информационного блока
// 5 - 2(0) - стартовая строка/ 2(3) - адрес/ 2(2) - имя массива
// 6 - 2(0) - размер basic-проги
void zxDevTape::blockData(int index, uint16_t* data) {
    auto name = (char*)(data + 10);
	memset(name, 32, 10);
    memset(data, 255, 10 * 2);

    if(index < countBlocks && index >= 0) {
        auto addr = blocks[index]->data;
        auto size = blocks[index]->data_size - 2;
        data[0] = (uint16_t)size;
        data[1] = addr[size + 1];
        data[2] = addr[0];
        if(data[2] == 0) {
            data[3] = addr[1];
            data[4] = *(uint16_t *) (addr + 12);
            data[5] = *(uint16_t *) (addr + 14);
            data[6] = *(uint16_t *) (addr + 16);
            memcpy(name, &addr[2], 10);
            for(int i = 9 ; i > 0; i--) {
                auto ch = addr[2 + i];
                if(ch != ' ') break;
                name[i] = 0;
            }
        }
        data[7] = (uint16_t)(index < currentBlock);
    }
}
