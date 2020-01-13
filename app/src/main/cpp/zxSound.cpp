//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxSound.h"

#define SND_TICKS_PER_FRAME_44100_CONST	882
#define SND_TICKS_PER_FRAME_22050_CONST	441
#define SND_TICKS_PER_FRAME_11025_CONST	220

static int sndTicks[3] = {SND_TICKS_PER_FRAME_44100_CONST, SND_TICKS_PER_FRAME_22050_CONST, SND_TICKS_PER_FRAME_11025_CONST};
uint8_t zxSound::regs[17];

static uint16_t volTable[] = {
        0x0000 / 2, 0x0344 / 2, 0x04BC / 2, 0x06ED / 2,
        0x0A3B / 2, 0x0F23 / 2, 0x1515 / 2, 0x2277 / 2,
        0x2898 / 2, 0x4142 / 2, 0x5B2B / 2, 0x726C / 2,
        0x9069 / 2, 0xB555 / 2, 0xD79B / 2, 0xFFFF / 2
};

static uint8_t eshape[] = {
        0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255,
        0, 1, 255, 255, 0, 1, 255, 255, 0, 1, 255, 255, 0, 1, 255, 255,
        1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 
        1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1
};

zxSound::zxSound() : nSamplers(0), updateStep(0), beepVal(0), beeperVolume(12), ayVolume(50),
                     engineObj(nullptr), mixObj(nullptr), playerObj(nullptr),
                     engine(nullptr), player(nullptr), bufferQueue(nullptr),
                    isAY(true), isBeeper(true), isInitSL(false) {
    cont = att = alt = hold = up = holded = 0;
    ssh_memzero(soundBuffer, sizeof(soundBuffer));
    ssh_memzero(beeperBuffer, sizeof(beeperBuffer));
    ssh_memzero(regs, sizeof(regs));
}

zxSound::~zxSound() {

}

void zxSound::updateProps(uint8_t period) {
    auto bits       = opts[ZX_PROP_SND_8BIT] != 0 ? 8 : 16;

    beeperVolume    = opts[ZX_PROP_SND_VOLUME_BP];
    ayVolume        = opts[ZX_PROP_SND_VOLUME_AY];

    freq            = frequencies[opts[ZX_PROP_SND_FREQUENCY]];
    stereoAY        = opts[ZX_PROP_SND_TYPE_AY];
//    updateStep      = (uint32_t)(((double)SND_STEP * (double)freq * (double)8) / (double)(freq * bits * 2) * 2.0); // 1773400
    updateStep      = (unsigned int)(((double)SND_STEP * (double)44100 * (double)8) / (double)1773400 * (double)2);

    isBeeper        = opts[ZX_PROP_SND_BP];
    isAY            = opts[ZX_PROP_SND_AY];

    // ACB = 0, ABC = 1, MONO = 2

    //divider = TIMER_CONST * 100 / emulator_speed;// 19
    double divider = 19.0;
    double koef = divider / (double)TIMER_CONST;

    for(int i = 0; i < 3; i++) {
        SND_TICKS_PER_FRAME[i] = (int)((double)sndTicks[i] * koef);
        if(SND_TICKS_PER_FRAME[i] > SND_TICKS_PER_FRAME_MAX) SND_TICKS_PER_FRAME[i] = SND_TICKS_PER_FRAME_MAX;
    }

    LOG_INFO("bvol: %i ayvol: %i freq: %i stereoAy: %i step: %i isb: %i iay: %i bits: %i ticks: %i",
            beeperVolume, ayVolume, freq, stereoAY, updateStep, isBeeper, isAY, bits,  SND_TICKS_PER_FRAME[0]);
}

void zxSound::reset() {
    nSamplers = 0;
    // аплитуда по трем каналам в 0
    regs[AVOL] = regs[BVOL] = regs[CVOL] = 0;
}

void zxSound::write(uint8_t reg, uint8_t value) {
    LOG_INFO("r: %i v: %i", reg, value);
    opts[reg] = value;
    reg -= AY_AFINE;
    if(nSamplers < SND_ARRAY_LEN) {
        SAMPLER[nSamplers].tcounter = *zxALU::_TICK;
        SAMPLER[nSamplers].reg = reg;
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
            channelA.period = (regs[AFINE] + 256 * regs[ACOARSE]) * updateStep / SND_STEP;
            if(!channelA.period) channelA.period = updateStep / SND_STEP;
            break;
        case BFINE:
        case BCOARSE:
            regs[BCOARSE] &= 0x0f;
            channelB.period = (regs[BFINE] + 256 * regs[BCOARSE]) * updateStep / SND_STEP;
            if(!channelB.period) channelB.period = updateStep / SND_STEP;
            break;
        case CFINE:
        case CCOARSE:
            regs[CCOARSE] &= 0x0f;
            channelC.period = (regs[CFINE] + 256 * regs[CCOARSE]) * updateStep / SND_STEP;
            if(!channelC.period) channelC.period = updateStep / SND_STEP;
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
            periodN = regs[NOISEPER] * updateStep / SND_STEP;
            if(periodN == 0) periodN = updateStep / SND_STEP;
            break;
        case EFINE:
        case ECOARSE:
            periodE = ((regs[EFINE] + 256 * regs[ECOARSE])) * updateStep / SND_STEP;
            if(periodE == 0) periodE = updateStep / SND_STEP / 2;
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

void zxSound::update() {
    if(!isInitSL) initSL();

    //if(!isAY) return;

    // заполняем позицию
    auto tcounter = *zxALU::_TICK;
    auto tick = SND_TICKS_PER_FRAME[0];
    if(nSamplers > 0) {
        if (tcounter == 0) tcounter = 1;
        auto t = tick - 1;
        for (int i = 0; i < nSamplers; i++) SAMPLER[i].pos = t * SAMPLER[i].tcounter / tcounter;
    }
    // Генерим звук
    int j = 0;
    for(int i = 0; i < tick; i++) {
        if(nSamplers > 0) {
            while (i == SAMPLER[j].pos) {
                applyRegister(SAMPLER[j].reg, SAMPLER[j].val);
                SAMPLER[j].pos = tick;
                j++;
                if (j >= SND_ARRAY_LEN) break;
            }
        }
        envelopeStep();
        channelA.gen(i, periodN, envVolume, AVOL);
        channelB.gen(i, periodN, envVolume, BVOL);
        channelC.gen(i, periodN, envVolume, CVOL);
        beeperBuffer[i] = beepVal;
    }
    // Микшируем и проигрываем
    uint16_t* sndBuf= soundBuffer;
    ssh_memzero(soundBuffer, sizeof(soundBuffer));

    uint16_t* bufA  = channelA.data;
    uint16_t* bufB  = channelB.data;
    uint16_t* bufC  = channelC.data;
    switch(stereoAY) {
        default:
            mix(sndBuf, 4, bufA, bufB, bufC, beeperBuffer);
            break;
        case AY_STEREO_ABC:
            mix(sndBuf, 3, bufA, bufC, beeperBuffer);
            break;
        case AY_STEREO_ACB:
            mix(sndBuf, 3, bufA, bufB, beeperBuffer);
            break;
    }

    nSamplers = 0;
    *zxALU::_TICK = 0;

    if(bufferQueue) {
        //(*bufferQueue)->Clear(bufferQueue); //Очищаем очередь на случай, если там что-то было. Можно опустить, если хочется, чтобы очередь реально была очередью
        (*bufferQueue)->Enqueue(bufferQueue, soundBuffer, SND_TICKS_PER_FRAME[0]);
    }
}

// выходной буфер, количество буферов для смещивания
void zxSound::mix(uint16_t* buf, int count, ...) {
    static uint16_t* mixing_ch[4];
    va_list ap;

    va_start(ap, count);
    for(int i = 0; i < count; i++) mixing_ch[i] = va_arg(ap, uint16_t*);
    for(int i = 0; i < SND_TICKS_PER_FRAME[0]; i++) {
        buf[i] = 0;
        for(int j = 0; j < count; j++) buf[i] += mixing_ch[j][i] / count;
    }
    va_end(ap);
}

bool zxSound::initSL() {
    isInitSL = true;

    LOG_INFO("start", nullptr);
	const SLboolean pIDsRequired[1]  = { SL_BOOLEAN_TRUE };

    // создаем движек
    SL_SUCCESS(slCreateEngine(&engineObj, 0, nullptr, 1, &SL_IID_ENGINE, pIDsRequired), "Error slCreateEngine");
    SL_SUCCESS((*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE), "Error Realize engineObj");
    SL_SUCCESS((*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine), "Error GetInterface SL_IID_ENGINE");
    // создаем миксер
    SL_SUCCESS((*engine)->CreateOutputMix(engine, &mixObj, 0, nullptr, nullptr), "Error CreateOutputMix");
    SL_SUCCESS((*mixObj)->Realize(mixObj, SL_BOOLEAN_FALSE), "Error Realize OutputMixObject");

    // Данные, которые необходимо передать в CreateAudioPlayer() для создания буферизованного плеера
    SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
    SLDataLocator_OutputMix locatorOutMix = { SL_DATALOCATOR_OUTPUTMIX, mixObj };

    SLDataFormat_PCM formatPCM = {
            SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN
    };

    static SLDataSource audioSrc = {&locatorBufferQueue, &formatPCM};
    static SLDataSink audioSnk = {&locatorOutMix, nullptr};

    // Создаем плеер
    SL_SUCCESS((*engine)->CreateAudioPlayer(engine, &playerObj, &audioSrc, &audioSnk, 1, &SL_IID_BUFFERQUEUE, pIDsRequired), "Error CreateAudioPlayer");
    SL_SUCCESS((*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE), "Error Ralize PlayerObject");

    // создаем очередь
    SL_SUCCESS((*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &player), "Error GetInterface SL_IID_PLAY");
    SL_SUCCESS((*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &bufferQueue), "Error GetInterface SL_IID_BUFFERQUEUE");
    SL_SUCCESS((*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING), "Error SetPlayState");
/*
    // (*obj)->Destroy(obj)
SL_IID_VOLUME для управления громкостью
SL_IID_MUTESOLO для управления каналами (только для многоканального звука, это указывается в поле numChannels структуры SLDataFormat_PCM).
SL_IID_EFFECTSEND для наложения эффектов(по спецификации – только эффект реверберации) *
 */
    LOG_INFO("finish", nullptr);
    return true;
}
