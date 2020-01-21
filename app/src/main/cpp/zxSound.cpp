
#include "zxCommon.h"
#include "zxSound.h"

#define CLOCK_RESET(clock)  ay_tick_incr = (int)(65536. * clock / sound_freq)
#define AY_GET_SUBVAL(chan)	(level * 2 * ay_tone_tick[chan] / tone_count)

static int process = 0;
static const SLboolean pIDsRequired[1]  = { SL_BOOLEAN_TRUE };
static SLresult slres;

#define AY_DO_TONE(var,chan) \
    is_low = 0;                                         \
    if(is_on) {                                         \
        (var) = 0;                                      \
        if(level) {                                     \
            if(ay_tone_high[chan])                      \
                (var) = (level);                        \
            else								        \
                (var) = -(level), is_low = 1;           \
        }                                               \
    }                                                   \
    ay_tone_tick[chan] += tone_count;                   \
    count = 0;                                          \
    while(ay_tone_tick[chan] >= ay_tone_period[chan]) { \
        count++;								        \
        ay_tone_tick[chan] -= ay_tone_period[chan];     \
        ay_tone_high[chan] =! ay_tone_high[chan];       \
        if(is_on && count == 1 && level && ay_tone_tick[chan] < tone_count)	{ \
            if(is_low)							        \
                (var) += AY_GET_SUBVAL(chan);           \
            else                                        \
                (var) -= AY_GET_SUBVAL(chan);           \
        }                                               \
    }                                                   \
    if(is_on && count > 1) (var) =- (level)

zxSound::zxSound() : isInit(false), isEnabled(false), tsmax(FRAME_STATES_48),
                    sound_freq(0), sound_buf(nullptr), psgap(250), beeper_last_subpos(0) {
}

void zxSound::updateProps(uint8_t period) {
    static int levels[16] = {
            0x0000, 0x0385, 0x053D, 0x0770,
            0x0AD7, 0x0FD5, 0x15B0, 0x230C,
            0x2B4C, 0x43C1, 0x5A4B, 0x732F,
            0x9204, 0xAFF1, 0xD921, 0xFFFF
    };

    int f;

    if(!isInit) initDriver();

    isEnabled       = opts[ZX_PROP_SND_LAUNCH] != 0;
    tsmax           = (*ALU->_MODEL >= MODEL_128) ? FRAME_STATES_128 : FRAME_STATES_48;
    isAyEnabled     = opts[ZX_PROP_SND_AY];
    isBpEnabled     = opts[ZX_PROP_SND_BP];

    //auto vAy        = opts[ZX_PROP_SND_VOLUME_AY] * 256;
    auto vBp        = opts[ZX_PROP_SND_VOLUME_BP];
    auto freq       = frequencies[opts[ZX_PROP_SND_FREQUENCY]];

    ampBeeper = (int)((2.5f * vBp) * 256);
    volBeeper = ampBeeper * 2;

    if(freq != sound_freq) {
        sound_freq = freq;
        for (f = 0; f < 16; f++) ay_tone_levels[f] = (uint32_t) ((levels[f] * AMPL_AY_TONE + 0x8000) / 0xffff);
        ay_noise_tick = ay_noise_period = 0;
        ay_env_internal_tick = ay_env_tick = ay_env_period = 0;
        ay_tone_subcycles = ay_env_subcycles = 0;
        for (f = 0; f < 3; f++) {
            ay_tone_tick[f] = ay_tone_high[f] = 0;
            ay_tone_period[f] = 1;
        }
        CLOCK_RESET(AY_CLOCK);
        ay_change_count = 0;

        SAFE_A_DELETE(sound_buf);

        sound_framesiz = sound_freq / 50;
        sound_buf = new short[sound_framesiz];

        sound_oldval = sound_oldval_orig = 0;
        sound_oldpos = -1;
        sound_fillpos = 0;

        makePlayer();
    }
}

void zxSound::ay_overlay() {
    static int rng = 1;
    static int noise_toggle = 0;
    static int env_first = 1, env_rev = 0, env_counter = 15;
    int tone_level[3];
    int mixer, envshape;
    int f, g, level, count;
    signed short *ptr;
    struct ay_change_tag *change_ptr = ay_change;
    int changes_left = ay_change_count;
    int reg, r;
    int is_low, is_on;
    int chan1, chan2, chan3;
    int frametime = (int)(tsmax * 50);
    uint32_t tone_count, noise_count;

    for(f = 0; f < ay_change_count; f++) ay_change[f].ofs = (uint16_t)((ay_change[f].tstates * sound_freq) / frametime);
    for(f = 0, ptr = sound_buf; f < sound_framesiz; f++) {
        while(changes_left && f >= change_ptr->ofs) {
            sound_ay_registers[reg = change_ptr->reg] = change_ptr->val;
            change_ptr++; changes_left--;
            switch(reg) {
                case 0: case 1: case 2: case 3: case 4: case 5:
                    r = reg >> 1;
                    ay_tone_period[r] = (uint32_t)((sound_ay_registers[reg & ~1] | (sound_ay_registers[reg | 1] & 15) << 8));
                    if(!ay_tone_period[r]) ay_tone_period[r]++;
                    if(ay_tone_tick[r] >= ay_tone_period[r] * 2) ay_tone_tick[r] %= ay_tone_period[r] * 2;
                    break;
                case 6:
                    ay_noise_tick = 0;
                    ay_noise_period = (uint32_t)(sound_ay_registers[reg] & 31);
                    break;
                case 11: case 12:
                    ay_env_period = sound_ay_registers[11] | (sound_ay_registers[12] << 8);
                    break;
                case 13:
                    ay_env_internal_tick = ay_env_tick = ay_env_subcycles = 0;
                    env_first = 1;
                    env_rev = 0;
                    env_counter = (sound_ay_registers[13] & AY_ENV_ATTACK) ? 0 : 15;
                    break;
            }
        }
        for(g = 0 ; g < 3; g++) tone_level[g] = ay_tone_levels[sound_ay_registers[8 + g] & 15];
        envshape = sound_ay_registers[13];
        level = ay_tone_levels[env_counter];
        for(g = 0; g < 3; g++)
            if(sound_ay_registers[8 + g] & 16) tone_level[g] = level;
        ay_env_subcycles += ay_tick_incr;
        noise_count = 0;
        while(ay_env_subcycles >= (16 << 16)) {
            ay_env_subcycles -= (16 << 16);
            noise_count++;
            ay_env_tick++;
            while(ay_env_tick>=ay_env_period) {
                ay_env_tick -= ay_env_period;
                if(env_first || ((envshape & AY_ENV_CONT) && !(envshape & AY_ENV_HOLD))) {
                    if(env_rev)
                        env_counter -= (envshape & AY_ENV_ATTACK) ? 1 : -1;
                    else
                        env_counter += (envshape & AY_ENV_ATTACK) ? 1 : -1;
                    if(env_counter < 0) env_counter = 0;
                    if(env_counter > 15) env_counter = 15;
                }
                ay_env_internal_tick++;
                while(ay_env_internal_tick >= 16) {
                    ay_env_internal_tick -= 16;
                    if(!(envshape & AY_ENV_CONT)) env_counter = 0;
                    else {
                        if(envshape & AY_ENV_HOLD) {
                            if(env_first && (envshape & AY_ENV_ALT)) env_counter = (env_counter ? 0 : 15);
                        }
                        else {
                            if(envshape & AY_ENV_ALT) env_rev = !env_rev;
                            else
                                env_counter = (envshape & AY_ENV_ATTACK) ? 0 : 15;
                        }
                    }
                    env_first = 0;
                }
                if(!ay_env_period) break;
            }
        }
        chan1 = tone_level[0];
        chan2 = tone_level[1];
        chan3 = tone_level[2];
        mixer = sound_ay_registers[7];
        ay_tone_subcycles += ay_tick_incr;
        tone_count = ay_tone_subcycles >> (3 + 16);
        ay_tone_subcycles &= (8 << 16) - 1;
        level = chan1; is_on = !(mixer & 1);
        AY_DO_TONE(chan1, 0);
        if((mixer & 0x08) == 0 && noise_toggle) chan1 = 0;
        level = chan2; is_on = !(mixer & 2);
        AY_DO_TONE(chan2, 1);
        if((mixer & 0x10) == 0 && noise_toggle) chan2 = 0;
        level = chan3; is_on = !(mixer & 4);
        AY_DO_TONE(chan3, 2);
        if((mixer & 0x20) == 0 && noise_toggle) chan3 = 0;

        // моно режим
        (*ptr++) += chan1 + chan2 + chan3;

        ay_noise_tick += noise_count;
        while(ay_noise_tick >= ay_noise_period) {
            ay_noise_tick -= ay_noise_period;
            if((rng & 1) ^ ((rng & 2) ? 1 : 0)) noise_toggle = !noise_toggle;
            rng |= ((rng & 1) ^ ((rng & 4) ? 1 : 0)) ? 0x20000 : 0;
            rng >>= 1;
            if(!ay_noise_period) break;
        }
    }
}

void zxSound::ayWrite(uint8_t reg, uint8_t val) {
    if(!isEnabled) return;
    if(reg >= 15) return;
    if(ay_change_count < AY_CHANGE_MAX) {
        ay_change[ay_change_count].tstates = zxALU::_TICK;
        ay_change[ay_change_count].reg = reg;
        ay_change[ay_change_count].val = val;
        ay_change_count++;
    }
}

void zxSound::reset() {
    int f;

    ay_change_count = 0;

    for(f = 0; f < 16; f++) ayWrite((uint8_t)f, 0);
    for(f = 0; f < 3; f++) ay_tone_high[f] = 0;

    ay_tone_subcycles = ay_env_subcycles = 0;
    sound_oldval = sound_oldval_orig = 0;
    CLOCK_RESET(AY_CLOCK);
}

int zxSound::update() {
    static int silent_level = -1;
    signed short *ptr;

    int f, silent, chk;
    int fulllen = sound_framesiz;
    ptr = sound_buf + sound_fillpos;
    for(f = sound_fillpos; f < sound_framesiz; f++) *ptr++ = (short)sound_oldval;

    ay_overlay();

    silent = 1;
    ptr = sound_buf;
    chk = *ptr++;
    for(f = 1; f < fulllen; f++) {
        if(*ptr++ != chk) { silent = 0; break; }
    }
    if(chk != silent_level) silent = 0;
    silent_level = sound_buf[fulllen - 1];

    if(!silent) {
        if(fulllen < sound_framesiz) {
            LOG_INFO("fulllen %i sound_framesiz %i", fulllen, sound_framesiz);
        }
        (*bufferQueue)->Enqueue(bufferQueue, sound_buf, (uint32_t) fulllen * 2);
    }
    sound_oldpos = -1;
    sound_fillpos = 0;
    ay_change_count = 0;

    return !silent;
}

void zxSound::beeperWrite(uint8_t on) {
    int f;

    if(!isEnabled) return;
    if(!isBpEnabled) return;

    auto tstates = zxALU::_TICK;

    auto val = (on ? -ampBeeper : ampBeeper);
    if(val == sound_oldval_orig) return;

    int newpos = (tstates * sound_framesiz) / tsmax;
    int subpos = (int)((((long long)tstates) * sound_framesiz * volBeeper) / tsmax - volBeeper * newpos);

    if(newpos == sound_oldpos) {
        if(on)
            beeper_last_subpos += volBeeper - subpos;
        else
            beeper_last_subpos -= volBeeper - subpos;
    } else beeper_last_subpos = (on ? volBeeper - subpos : subpos);
    int subval = ampBeeper - beeper_last_subpos;
    if(newpos >= 0) {
        auto ptr = (sound_buf + sound_fillpos);
        for(f = sound_fillpos; f < newpos && f < sound_framesiz; f++) *ptr++ = (short)sound_oldval;
        if(newpos < sound_framesiz) {
            ptr = sound_buf + newpos;
            *ptr = (short)subval;
        }
    }
    sound_oldpos = newpos;
    sound_fillpos = newpos + 1;
    sound_oldval = sound_oldval_orig = val;
}

/*
void callback_ay8912(SLBufferQueueItf pBufferQueue, void* pThis) {
      //LOG_INFO("%i",process);
    auto snd = (zxSound*)pThis;
    //memset(snd->sound_buf, 0, (size_t)(snd->sound_framesiz * 2));
    //(*pBufferQueue)->Clear(pBufferQueue);
    process--;
    //auto p = snd->player;
}

*/
void zxSound::initDriver() {
    isInit = true;

    // создаем движек
    SL_SUCCESS(slCreateEngine(&engineObj, 0, nullptr, 1, &SL_IID_ENGINE, pIDsRequired), "Error slCreateEngine (%X)");
    SL_SUCCESS((*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE), "Error Realize engineObj (%X)");
    SL_SUCCESS((*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine), "Error GetInterface SL_IID_ENGINE (%X)");
    // создаем миксер
    SL_SUCCESS((*engine)->CreateOutputMix(engine, &mixObj, 0, nullptr, nullptr), "Error CreateOutputMix (%X)");
    SL_SUCCESS((*mixObj)->Realize(mixObj, SL_BOOLEAN_FALSE), "Error Realize OutputMixObject (%X)");

    makePlayer();

}

void zxSound::makePlayer() {
    static uint32_t freqs[3] = {SL_SAMPLINGRATE_44_1, SL_SAMPLINGRATE_22_05, SL_SAMPLINGRATE_11_025};

    if (engine) {
        if (playerObj) {
            (*playerObj)->Destroy(playerObj);
            playerObj = nullptr;
        }
        // Данные, которые необходимо передать в CreateAudioPlayer() для создания буферизованного плеера
        SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
        SLDataLocator_OutputMix locatorOutMix = {SL_DATALOCATOR_OUTPUTMIX, mixObj};
        SLDataFormat_PCM formatPCM = {SL_DATAFORMAT_PCM, 1, freqs[opts[ZX_PROP_SND_FREQUENCY]], SL_PCMSAMPLEFORMAT_FIXED_16,
                                      SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};

        SLDataSource audioSrc = {&locatorBufferQueue, &formatPCM};
        SLDataSink audioSnk = {&locatorOutMix, nullptr};

        // Создаем плеер
        SL_SUCCESS((*engine)->CreateAudioPlayer(engine, &playerObj, &audioSrc, &audioSnk, 1, &SL_IID_BUFFERQUEUE, pIDsRequired),
                   "Error CreateAudioPlayer (%X)");
        SL_SUCCESS((*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE), "Error Realize PlayerObject (%X)");

        // создаем очередь
        SL_SUCCESS((*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &player), "Error GetInterface SL_IID_PLAY (%X)");
        SL_SUCCESS((*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &bufferQueue), "Error GetInterface SL_IID_BUFFERQUEUE (%X)");

        //SL_SUCCESS((*player)->SetCallbackEventsMask(player, SL_PLAYEVENT_HEADATEND), "Error SetCallbackEventsMask (%X)");
        //SL_SUCCESS((*bufferQueue)->RegisterCallback(bufferQueue, callback_ay8912, this), "Error RegisterCallback (%X)");

        SL_SUCCESS((*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING), "Error SetPlayState (%X)");
    }
}
