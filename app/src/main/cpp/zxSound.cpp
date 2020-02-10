
#include <pthread.h>
#include "zxCommon.h"
#include "zxSound.h"

#define CLOCK_RESET(clock)  tickAY = (int)(65536. * clock / frequency)
#define GEN_STEREO(pos, val)                                        \
    if((pos) < 0) {                                                 \
        rstereobuf_l[rstereopos] += (val);                          \
        rstereobuf_r[(rstereopos - pos) % STEREO_BUF_SIZE] += (val);\
    } else {								                        \
        rstereobuf_l[(rstereopos + pos) % STEREO_BUF_SIZE] += (val);\
        rstereobuf_r[rstereopos] += (val); }

static const SLboolean pIDsRequired[1]  = { SL_BOOLEAN_TRUE };
static SLresult slres;
static bool isPlaying = false;

zxSound::zxSound() : isInit(false), isEnabled(false), tsmax(0), sound_stereo_ay(-1),
                    engineObj(nullptr), mixObj(nullptr), playerObj(nullptr), player(nullptr), bufferQueue(nullptr),
                    frequency(1), sndBuf(nullptr), sndPlayBuf(nullptr), psgap(250), beeperLastPos(0) {
    sound_stereo_ay_narrow = 0;
}

zxSound::~zxSound() {
    reset();
    SAFE_A_DELETE(sndBuf);
    SAFE_A_DELETE(sndPlayBuf);
    if(playerObj) {
        (*playerObj)->Destroy(playerObj);
        playerObj = nullptr;
    }
    if(mixObj) {
        (*mixObj)->Destroy(mixObj);
        mixObj = nullptr;
    }
    if(engineObj) {
        (*engineObj)->Destroy(engineObj);
        engineObj = nullptr;
    }
}

void zxSound::updateProps() {
    static int levels[16] = {
            0x0000, 0x0385, 0x053D, 0x0770,
            0x0AD7, 0x0FD5, 0x15B0, 0x230C,
            0x2B4C, 0x43C1, 0x5A4B, 0x732F,
            0x9204, 0xAFF1, 0xD921, 0xFFFF
    };

//    if(!isInit) initDriver();

    int f;

    reset();

    isEnabled       = opts[ZX_PROP_SND_LAUNCH] != 0;
    tsmax           = ULA->machine->cpuClock / 50;
    isAyEnabled     = isEnabled && opts[ZX_PROP_SND_AY];
    isBpEnabled     = isEnabled && opts[ZX_PROP_SND_BP];
    clockAY         = opts[ZX_PROP_TURBO_MODE] ? 1773400 * 2 : 1773400;

    auto modeAy     = opts[ZX_PROP_SND_CHANNEL_AY];
    auto vAy        = opts[ZX_PROP_SND_VOLUME_AY] * 256;
    auto vBp        = opts[ZX_PROP_SND_VOLUME_BP];
    auto freq       = frequencies[opts[ZX_PROP_SND_FREQUENCY]];
    auto change     = freq != frequency || modeAy != sound_stereo_ay;

    frequency = freq;
    sound_stereo_ay = modeAy;
    ampBeeper = (int)((2.5f * vBp) * 256);
    volBeeper = ampBeeper * 2;
    for (f = 0; f < 16; f++) toneLevels[f] = (uint32_t) ((levels[f] * vAy + 0x8000) / 0xffff);
    CLOCK_RESET(clockAY);

    if(change) {
        if(player) (*player)->SetPlayState(player, SL_PLAYSTATE_STOPPED);
        beeperOldVal = beeperOrigVal = 0;
        beeperOldPos = -1;
        beeperPos = 0;

        noiseTick = noisePeriod = 0;
        envIntTick = envTick = envPeriod = 0;
        toneSubCycles = envSubCycles = 0;

        for (f = 0; f < 3; f++) {
            toneTick[f] = toneHigh[f] = 0;
            tonePeriod[f] = 1;
        }

        SAFE_A_DELETE(sndBuf);
        SAFE_A_DELETE(sndPlayBuf);

        sndBufSize  = frequency / 50;
        sndBuf      = new short[sndBufSize * 2];
        sndPlayBuf  = new short[sndBufSize * 2];
        memset(sndBuf, 0, sndBufSize * sizeof(uint32_t));
        if(bufferQueue) (*bufferQueue)->Clear(bufferQueue);

        memset(pstereobuf, 0, sizeof(pstereobuf));
        memset(rstereobuf_l, 0, sizeof(rstereobuf_l));
        memset(rstereobuf_r, 0, sizeof(rstereobuf_r));
        for(f = 0 ; f < STEREO_BUF_SIZE; f++) pstereobuf[f] = ampBeeper;
        pstereopos = 0;
        pstereobufsiz = (frequency * psgap) / 22050;
        int pos = (sound_stereo_ay_narrow ? 3 : 6) * frequency / 8000;
        rstereopos = 0;
        rchan1pos = -pos;
        if(sound_stereo_ay == 1) {
            rchan2pos = 0; rchan3pos = pos;
        } else if(sound_stereo_ay == 2) {
            rchan2pos = pos; rchan3pos = 0;
        }
  //      makePlayer(true);
    }
}

short* zxSound::write_buf_pstereo(signed short *out, int c) {
//    *out++ = (short)c;
//    *out++ = (short)c;
    int bl = (c - pstereobuf[pstereopos]) / 2;
    int br = (c + pstereobuf[pstereopos]) / 2;
    if(bl < -ampBeeper) bl = -ampBeeper;
    if(br < -ampBeeper) br = -ampBeeper;
    if(bl > ampBeeper)  bl = ampBeeper;
    if(br > ampBeeper)  br = ampBeeper;
    *out++ = (short)bl; *out++ = (short)br;
    pstereobuf[pstereopos++] = c;
    if(pstereopos >= pstereobufsiz) pstereopos = 0;
    return out;
}

void zxSound::reset() {
    int f;

    countSamplers = 0;

    if(bufferQueue) (*bufferQueue)->Clear(bufferQueue);
    memset(sndPlayBuf, 0, sndBufSize * 2 * sizeof(short));

    isPlaying = false;

    for(f = 0; f < 16; f++) ayWrite((uint8_t)f, 0, 0);
    for(f = 0; f < 3; f++) toneHigh[f] = 0;

    toneSubCycles = envSubCycles = 0;
    beeperOldVal = beeperOrigVal = 0;

    CLOCK_RESET(clockAY);
}

void zxSound::ayWrite(uint8_t reg, uint8_t val, long tick) {
    if(isAyEnabled) {
        if (reg < 16) {
            if (countSamplers < AY_SAMPLERS) {
                samplers[countSamplers].tstates = tick % ULA->machine->tsTotal;
                samplers[countSamplers].reg = reg;
                samplers[countSamplers].val = val;
                countSamplers++;
            }
        }
    }
}

void zxSound::apply() {
    static int rng = 1;
    static int noise_toggle = 0;
    static int envFirst = 1, envRev = 0, envCounter = 15;
    int toneLevel[3], chans[3];
    int mixer, envshape;
    int f, g, level, count;
    signed short *ptr;
    struct AY_SAMPLER *change_ptr = samplers;
    int changes_left = countSamplers;
    int reg, r;
    int frameTime = (int)(tsmax * 50);
    uint32_t tone_count, noise_count;

    for(f = 0; f < countSamplers; f++) samplers[f].ofs = (uint16_t)((samplers[f].tstates * frequency) / frameTime);
    for(f = 0, ptr = sndBuf; f < sndBufSize; f++) {
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
}

void zxSound::beeperWrite(uint8_t on) {
    if(isBpEnabled && opts[ZX_PROP_EXECUTE]) {
        int f;

        auto tstates = zxULA::_TICK % ULA->machine->tsTotal;

        auto val = (on ? -ampBeeper : ampBeeper);
        if(val == beeperOrigVal) return;
        int newPos = (int) ((tstates * sndBufSize) / tsmax);
        int subPos = (int) ((((long long) tstates) * sndBufSize * volBeeper) / tsmax - volBeeper * newPos);

        if(newPos == beeperOldPos) {
            if(on) beeperLastPos += volBeeper - subPos; else beeperLastPos -= volBeeper - subPos;
        } else beeperLastPos = (on ? volBeeper - subPos : subPos);
        int subVal = ampBeeper - beeperLastPos;
        if(newPos >= 0) {
            auto ptr = sndBuf + beeperPos * 2;
            for(f = beeperPos; f < newPos && f < sndBufSize; f++) ptr = write_buf_pstereo(ptr, beeperOldVal);
            if(newPos < sndBufSize) {
                ptr = sndBuf + newPos * 2;
                write_buf_pstereo(ptr, subVal);
            }
        }
        beeperOldPos = newPos;
        beeperPos = newPos + 1;
        beeperOldVal = beeperOrigVal = val;
    }
}

int zxSound::update(uint8_t* buf) {
    static int silent_level = -1;
    signed short *ptr;
    int f, silent(1);

    if(isEnabled && opts[ZX_PROP_EXECUTE]) {
        int fulllen = sndBufSize * 2;
        ptr = sndBuf + beeperPos * 2;
        for (f = beeperPos; f < sndBufSize; f++) ptr = write_buf_pstereo(ptr, beeperOldVal);

        apply();

        ptr = sndBuf;
        auto chk = *ptr;
        for (f = 1; f < sndBufSize; f++) {
            if (*ptr++ != chk) { silent = 0; break; }
            ptr++;
        }
        if (chk != silent_level) silent = 0;
        silent_level = sndBuf[fulllen - 1];

        memcpy(buf, sndBuf, fulllen * sizeof(short));
/*

        if(!silent) {
            int delay = 1000000;
            while (isPlaying && delay-- > 0) {}
            isPlaying = true;
            memcpy(sndPlayBuf, sndBuf, fulllen * sizeof(short));
            (*bufferQueue)->Enqueue(bufferQueue, sndPlayBuf, (uint32_t) fulllen * sizeof(short));
        }
*/
        beeperOldPos = -1;
        beeperPos = 0;
        countSamplers = 0;
        return silent != 0 ? 0 : fulllen;
    }
    return 0;
}

void callback_ay8912(SLBufferQueueItf, void*) {
    //auto snd = (zxSound*)pThis;
    //memset(snd->sndPlayBuf, 0, snd->sndBufSize * 2 * sizeof(short));
    isPlaying = false;
}

void zxSound::initDriver() {
    isInit = true;

    // создаем движек
    SL_SUCCESS(slCreateEngine(&engineObj, 0, nullptr, 1, &SL_IID_ENGINE, pIDsRequired), "Error slCreateEngine (%X)");
    SL_SUCCESS((*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE), "Error Realize engineObj (%X)");
    SL_SUCCESS((*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine), "Error GetInterface SL_IID_ENGINE (%X)");
    // создаем миксер
    SL_SUCCESS((*engine)->CreateOutputMix(engine, &mixObj, 0, nullptr, nullptr), "Error CreateOutputMix (%X)");
    SL_SUCCESS((*mixObj)->Realize(mixObj, SL_BOOLEAN_FALSE), "Error Realize OutputMixObject (%X)");
}

void zxSound::makePlayer(bool stereo) {
    static uint32_t freqs[3] = { SL_SAMPLINGRATE_44_1, SL_SAMPLINGRATE_22_05, SL_SAMPLINGRATE_11_025 };

    if (engine) {
        if (playerObj) (*playerObj)->Destroy(playerObj);
        playerObj = nullptr;
        bufferQueue = nullptr;
        // Данные, которые необходимо передать в CreateAudioPlayer() для создания буферизованного плеера
        SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataLocator_OutputMix locatorOutMix = { SL_DATALOCATOR_OUTPUTMIX, mixObj };
        SLDataFormat_PCM formatPCMMono = { SL_DATAFORMAT_PCM, 1, freqs[opts[ZX_PROP_SND_FREQUENCY]], SL_PCMSAMPLEFORMAT_FIXED_16,
                                      SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
        SLDataFormat_PCM formatPCMStereo = { SL_DATAFORMAT_PCM, 2, freqs[opts[ZX_PROP_SND_FREQUENCY]],
                                       SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                       SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
        SLDataSource audioSrc = {&locatorBufferQueue, stereo ? &formatPCMStereo : &formatPCMMono};
        SLDataSink audioSnk = {&locatorOutMix, nullptr};

        // Создаем плеер
        SL_SUCCESS((*engine)->CreateAudioPlayer(engine, &playerObj, &audioSrc, &audioSnk, 1, &SL_IID_BUFFERQUEUE, pIDsRequired),
                   "Error CreateAudioPlayer (%02X)");
        SL_SUCCESS((*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE), "Error Realize PlayerObject (%X)");

        // создаем очередь
        SL_SUCCESS((*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &player), "Error GetInterface SL_IID_PLAY (%X)");
        SL_SUCCESS((*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &bufferQueue), "Error GetInterface SL_IID_BUFFERQUEUE (%X)");

        SL_SUCCESS((*player)->SetCallbackEventsMask(player, SL_PLAYEVENT_HEADATEND), "Error SetCallbackEventsMask (%X)");
        SL_SUCCESS((*bufferQueue)->RegisterCallback(bufferQueue, callback_ay8912, this), "Error RegisterCallback (%X)");

        SL_SUCCESS((*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING), "Error SetPlayState(%X)");
        isPlaying = false;
    }
}
