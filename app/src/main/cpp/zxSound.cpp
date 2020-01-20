//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxSound.h"

#define SND_TICKS_PER_FRAME_44100_CONST	882
#define SND_TICKS_PER_FRAME_22050_CONST	441
#define SND_TICKS_PER_FRAME_11025_CONST	220

static int sndTicks[3] = {SND_TICKS_PER_FRAME_44100_CONST, SND_TICKS_PER_FRAME_22050_CONST, SND_TICKS_PER_FRAME_11025_CONST};
static const SLboolean pIDsRequired[1]  = { SL_BOOLEAN_TRUE };
static SLresult slres;

static uint8_t regs[17];
static uint16_t volTable[16];

static uint8_t eshape[] = {
        0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255,
        0, 1, 255, 255, 0, 1, 255, 255, 0, 1, 255, 255, 0, 1, 255, 255,
        1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 
        1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1
};

zxSound::zxSound() : nSamplers(0), updateStep(0), beepVal(0), nLength(0), beeperVolume(12), ayVolume(50),
                     engineObj(nullptr), mixObj(nullptr), playerObj(nullptr),
                     engine(nullptr), player(nullptr), bufferQueue(nullptr), buffer(nullptr),
                     beeperBuffer(nullptr), isAY(true), isBeeper(true), isInitSL(false) {
    cont = att = alt = hold = up = holded = 0;
    ssh_memzero(regs, sizeof(regs));
}

zxSound::~zxSound() {
    SAFE_A_DELETE(buffer);
    SAFE_A_DELETE(beeperBuffer);
    SAFE_A_DELETE(channelA.data);
    SAFE_A_DELETE(channelB.data);
    SAFE_A_DELETE(channelC.data);

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

void zxSound::updateProps(uint8_t period) {
    beeperVolume    = opts[ZX_PROP_SND_VOLUME_BP];
    ayVolume        = opts[ZX_PROP_SND_VOLUME_AY];

    stereoMode      = opts[ZX_PROP_SND_TYPE_AY];
    updateStep      = 1;//(uint32_t)(((double)SND_STEP * (double)freq * (double)16) / (double)1773400 * (double)2);

    isBeeper        = opts[ZX_PROP_SND_BP];
    isAY            = opts[ZX_PROP_SND_AY];

    double koef     = 1.0;//TIMER_CONST / (period / 10.0);

    auto len        = (uint32_t)(sndTicks[opts[ZX_PROP_SND_FREQUENCY]] * koef);
    if(len != nLength) {
        nLength = len;
        delete [] buffer;
        delete [] beeperBuffer;
        delete [] channelA.data;
        delete [] channelB.data;
        delete [] channelC.data;

        buffer = new uint16_t[nLength];
        beeperBuffer = new uint16_t[nLength];
        channelA.data= new uint16_t[nLength];
        channelB.data= new uint16_t[nLength];
        channelC.data= new uint16_t[nLength];

        makePlayer();
    }
    ssh_memzero(buffer, nLength * 2);
    ssh_memzero(beeperBuffer, nLength * 2);
    ssh_memzero(channelA.data, nLength * 2);
    ssh_memzero(channelB.data, nLength * 2);
    ssh_memzero(channelC.data, nLength * 2);
}

void zxSound::reset() {
    nSamplers = 0;
    // аплитуда по трем каналам в 0
    regs[AVOL] = regs[BVOL] = regs[CVOL] = 0;
    // очистить очередь
    if(bufferQueue) (*bufferQueue)->Clear(bufferQueue);
}

void zxSound::write(uint8_t reg, uint8_t value) {
    if(reg < AY_AFINE && reg > AY_BEEPER) {
        LOG_INFO("AY_REG = %i invalid!",  reg);
        return;
    }
    opts[reg] = value;
    if(nSamplers < SND_ARRAY_LEN) {
        SAMPLER[nSamplers].tcounter = *zxALU::_TICK;
        SAMPLER[nSamplers].reg = reg - AY_AFINE;
        SAMPLER[nSamplers].val = value;
        nSamplers++;
    }
}

void zxSound::applyRegister(uint8_t reg, uint8_t val) {
    if(reg <= ESHAPE) regs[reg] = val;
    switch(reg) {
        case AFINE:
        case ACOARSE:
            regs[ACOARSE] &= 0x0f;
            channelA.period = (regs[AFINE] + 256 * regs[ACOARSE]) * updateStep;
            if(!channelA.period) channelA.period = updateStep;
            break;
        case BFINE:
        case BCOARSE:
            regs[BCOARSE] &= 0x0f;
            channelB.period = (regs[BFINE] + 256 * regs[BCOARSE]) * updateStep;
            if(!channelB.period) channelB.period = updateStep;
            break;
        case CFINE:
        case CCOARSE:
            regs[CCOARSE] &= 0x0f;
            channelC.period = (regs[CFINE] + 256 * regs[CCOARSE]) * updateStep;
            if(!channelC.period) channelC.period = updateStep;
            break;
        case AVOL:
            regs[AVOL] &= 0x1f;
            break;
        case BVOL:
            regs[BVOL] &= 0x1f;
            break;
        case CVOL:
            regs[CVOL] &= 0x1f;
            break;
        case ENABLE:
            channelA.isEnable = (bool)(regs[ENABLE] & 1);
            channelB.isEnable = (bool)(regs[ENABLE] & 2);
            channelC.isEnable = (bool)(regs[ENABLE] & 4);
            channelA.isNoise  =	(bool)(regs[ENABLE] & 8);
            channelB.isNoise  =	(bool)(regs[ENABLE] & 16);
            channelC.isNoise  =	(bool)(regs[ENABLE] & 32);
            break;
        case NOISEPER:
            regs[NOISEPER] &= 0x1f;
            periodN = regs[NOISEPER] * updateStep;
            if(!periodN) periodN = updateStep;
            break;
        case EFINE:
        case ECOARSE:
            periodE = ((regs[EFINE] + 256 * regs[ECOARSE])) * updateStep;
            if(!periodE) periodE = updateStep / 2;
            break;
        case ESHAPE: {
            regs[ESHAPE] &= 0x0f;
            auto pshape = &eshape[regs[ESHAPE] * 4];
            cont = pshape[0]; att = pshape[1];
            if(regs[ESHAPE] > 7) { alt = pshape[2]; hold = pshape[3]; }
            envelopeCounter = 0;
            envVolume = (uint8_t)(att ? 0 : 15);
            up = att;
            holded = 0;
            break;
        }
        case BEEPER:
            beepVal = (uint16_t)(val ? volTable[beeperVolume] : 0);
            break;
    }
}

void zxSound::envelopeStep() {
    if(holded) return;

    if((alt ? periodE : (periodE * 2)) > envelopeCounter)
        envelopeCounter++;
    else {
        envelopeCounter = 0;
        if(up) {
            if(envVolume < 15) envVolume++;
            else {
                if(cont == 0) { holded = 1; envVolume = 0; }
                else if(hold) { if(alt) envVolume = 0; holded = 1; }
                else { if(alt) up = 0; else envVolume = 0; }
            }
        } else {
            if(envVolume > 0) envVolume--;
            else {
                if(cont == 0) { holded = 1; }
                else if(hold) { if(alt) envVolume = 15; holded = 1; }
                else { if(alt) up = 1; else envVolume = 15; }
            }
        }
    }
}

void zxSound::AY_CHANNEL::gen(int i, int periodN, uint8_t volume, int ch_vol) {
    if(counter >= period) counter = 0;
    bool tone = counter < (period / 2);
    counter++;
    if(noise >= periodN) { noise = 0; rnd = rand(); }
    bool noice = (bool)(rnd & 1);
    noise++;
    auto tmp = (((tone | isEnable) & (noice | isNoise)) | !period);
    data[i] = tmp ? ((regs[ch_vol] & 0x10) ? volTable[volume] : volTable[regs[ch_vol]]) : (uint16_t)0;
}

static int process = 0;

void zxSound::update() {
    if(!isInitSL) initSL();
    if(bufferQueue) {
        if (!opts[ZX_PROP_SND_LAUNCH]) (*bufferQueue)->Clear(bufferQueue);
        else {
            // заполняем позицию
            auto tcounter = *zxALU::_TICK;
            if (nSamplers > 0) {
                if (tcounter == 0) tcounter = 1;
                auto len = nLength - 1;
                for (int i = 0; i < nSamplers; i++) {
                    SAMPLER[i].pos = (len * SAMPLER[i].tcounter) / tcounter;
                }
            }
            // Генерим звук
            int j = 0;
            beepVal = 0;
            for (int i = 0; i < nLength; i++) {
                if (nSamplers > 0) {
                    while (i == SAMPLER[j].pos) {
                        applyRegister(SAMPLER[j].reg, SAMPLER[j].val);
                        SAMPLER[j].pos = nLength;
                        j++;
                        if (j >= SND_ARRAY_LEN) break;
                    }
                }
                envelopeStep();
                if (isAY) {
                    channelA.gen(i, periodN, envVolume, AVOL);
                    channelB.gen(i, periodN, envVolume, BVOL);
                    channelC.gen(i, periodN, envVolume, CVOL);
                }
                if (isBeeper) {
                    beeperBuffer[i] = beepVal;
                }
            }
            // Микшируем и проигрываем
            uint16_t *bufA = channelA.data;
            uint16_t *bufB = channelB.data;
            uint16_t *bufC = channelC.data;
            switch (stereoMode) {
                default:
                    mix(buffer, 4, bufA, bufB, bufC, beeperBuffer);
                    break;
                case AY_STEREO_ABC:
                    mix(buffer, 3, bufA, bufC, beeperBuffer);
                    break;
                case AY_STEREO_ACB:
                    mix(buffer, 3, bufA, bufB, beeperBuffer);
                    break;
            }
            nSamplers = 0;
            process = 1;
            (*bufferQueue)->Enqueue(bufferQueue, buffer, nLength);
            while(process != 0) { }
        }
    }
}

void callback_sound(SLBufferQueueItf pBufferQueue, void*) {
    //(*pBufferQueue)->Clear(pBufferQueue);
    process = 0;
}

// выходной буфер, количество буферов для смещивания
void zxSound::mix(uint16_t* buf, int count, ...) {
    static uint16_t* mixing_ch[4];
    va_list ap;

    va_start(ap, count);
    for (int i = 0; i < count; i++) mixing_ch[i] = va_arg(ap, uint16_t*);
    va_end(ap);
    for(int i = 0; i < nLength; i++) {
        float val = 0;
        for(int j = 0; j < count; j++) val += mixing_ch[j][i];
        buf[i] = (uint16_t)(val / count);
    }
}

void zxSound::initSL() {
    isInitSL = true;

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
    static uint32_t freqs[3] = { SL_SAMPLINGRATE_44_1, SL_SAMPLINGRATE_22_05, SL_SAMPLINGRATE_11_025 };

    static uint16_t vols[] = {
            0x0000, 0x0344, 0x04BC, 0x06ED, 0x0A3B, 0x0F23, 0x1515, 0x2277,
            0x2898, 0x4142, 0x5B2B, 0x726C, 0x9069, 0xB555, 0xD79B, 0xFFFF
    };

    if(engine) {
        if (playerObj) {
            (*playerObj)->Destroy(playerObj);
            playerObj = nullptr;
        }
        // Данные, которые необходимо передать в CreateAudioPlayer() для создания буферизованного плеера
        SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
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

        SL_SUCCESS((*player)->SetCallbackEventsMask(player, SL_PLAYEVENT_HEADATEND), "Error SetCallbackEventsMask (%X)");
        SL_SUCCESS((*bufferQueue)->RegisterCallback(bufferQueue, callback_sound, this), "Error RegisterCallback (%X)");

        SL_SUCCESS((*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING), "Error SetPlayState (%X)");

        for (int i = 0; i < 16; i++) volTable[i] = vols[i] >> 1;
    }
}
