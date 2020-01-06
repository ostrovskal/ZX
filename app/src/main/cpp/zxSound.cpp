//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxSound.h"

uint8_t* zxSound::_REGISTERS(nullptr);
uint8_t* zxSound::_CURRENT(nullptr);

static int sndTicks[3] = {SND_TICKS_PER_FRAME_1_CONST, SND_TICKS_PER_FRAME_2_CONST, SND_TICKS_PER_FRAME_4_CONST};

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

zxSound::zxSound() : nSamplers(0), updateStep(0), beepVal(0), beeperVolume(12), ayVolume(50), isAY(true), isBeeper(true), isInitSL(false) {
    _REGISTERS = &opts[AY_AFINE];
    _CURRENT = &opts[AY_REG];
    ssh_memzero(soundBuffer, sizeof(soundBuffer));
    ssh_memzero(beeperBuffer, sizeof(beeperBuffer));
}

void zxSound::updateProps(uint8_t period) {
    auto bits       = opts[ZX_PROP_SND_8BIT] != 0 ? 8 : 16;

    beeperVolume    = opts[ZX_PROP_SND_VOLUME_BP];
    ayVolume        = opts[ZX_PROP_SND_VOLUME_AY];

    freq            = frequencies[opts[ZX_PROP_SND_FREQUENCY]];
    stereoAY        = opts[ZX_PROP_SND_TYPE_AY];
    updateStep      = (uint32_t)(((double)SND_STEP * (double)freq * (double)8) / (double)(freq * bits * 2) * 2.0); // 1773400

    isBeeper        = opts[ZX_PROP_SND_BP];
    isAY            = opts[ZX_PROP_SND_AY];

    // delayCPU = 10 => 100%
    double delitel = TIMER_CONST * 100.0 / ((double)period / 10.0) * 100.0;
    double koef = delitel / (double)TIMER_CONST;

    for(int i = 0; i < 2; i++) {
        SND_TICKS_PER_FRAME[i] = (int)((double)sndTicks[i] * koef);
        if(SND_TICKS_PER_FRAME[i] > SND_TICKS_PER_FRAME_MAX) SND_TICKS_PER_FRAME[i] = SND_TICKS_PER_FRAME_MAX;
    }
}

void zxSound::reset() {
    nSamplers = 0;
    // аплитуда по трем каналам в 0
    opts[AY_AVOL] = opts[AY_BVOL] = opts[AY_CVOL] = 0;
}

void zxSound::write(uint8_t reg, uint8_t value) {
    _REGISTERS[reg] = value;
    if(nSamplers < SND_ARRAY_LEN) {
        SAMPLER[nSamplers].tcounter = *zxALU::_TICK;
        SAMPLER[nSamplers].reg = reg;
        SAMPLER[nSamplers].val = value;
        nSamplers++;
    }
}

void zxSound::applyRegister(uint8_t reg, uint8_t val) {
    if(reg <= AY_ESHAPE) _REGISTERS[reg] = val;
    switch(reg) {
        case AY_AFINE:
        case AY_ACOARSE:
            opts[AY_ACOARSE] &= 0x0f;
            channelA.genPeriod = (opts[AY_AFINE] + 256 * opts[AY_ACOARSE]) * updateStep / SND_STEP;
            if(!channelA.genPeriod) channelA.genPeriod = updateStep / SND_STEP;
            break;
        case AY_BFINE:
        case AY_BCOARSE:
            opts[AY_BCOARSE] &= 0x0f;
            channelB.genPeriod = (opts[AY_BFINE] + 256 * opts[AY_BCOARSE]) * updateStep / SND_STEP;
            if(!channelB.genPeriod) channelB.genPeriod = updateStep / SND_STEP;
            break;
        case AY_CFINE:
        case AY_CCOARSE:
            opts[AY_CCOARSE] &= 0x0f;
            channelC.genPeriod = (opts[AY_CFINE] + 256 * opts[AY_CCOARSE]) * updateStep / SND_STEP;
            if(!channelC.genPeriod) channelC.genPeriod = updateStep / SND_STEP;
            break;
        case AY_AVOL:
            opts[AY_AVOL] &= 0x1f;
            break;
        case AY_BVOL:
            opts[AY_BVOL] &= 0x1f;
            break;
        case AY_CVOL:
            opts[AY_CVOL] &= 0x1f;
            break;
        case AY_ENABLE:
            channelA.isChannel = (bool)(opts[AY_ENABLE] & 1);
            channelB.isChannel = (bool)(opts[AY_ENABLE] & 2);
            channelC.isChannel = (bool)(opts[AY_ENABLE] & 4);
            channelA.isNoise =	 (bool)(opts[AY_ENABLE] & 8);
            channelB.isNoise =	 (bool)(opts[AY_ENABLE] & 16);
            channelC.isNoise =	 (bool)(opts[AY_ENABLE] & 32);
            break;
        case AY_NOISEPER:
            opts[AY_NOISEPER] &= 0x1f;
            periodN = opts[AY_NOISEPER] * updateStep / SND_STEP;
            if(periodN == 0) periodN = updateStep / SND_STEP;
            break;
        case AY_EFINE:
        case AY_ECOARSE:
            periodE = ((opts[AY_EFINE] + 256 * opts[AY_ECOARSE])) * updateStep / SND_STEP;
            if(periodE == 0) periodE = updateStep / SND_STEP / 2;
            break;
        case AY_ESHAPE: {
            opts[AY_ESHAPE] &= 0x0f;
            auto pshape = &eshape[opts[AY_ESHAPE] * 4];
            cont = pshape[0]; att = pshape[1];
            if(opts[AY_ESHAPE] > 7) { alt = pshape[2]; hold = pshape[3]; }
            envelopeCounter = 0;
            envVolume = (uint8_t)(att ? 0 : 15);
            up = att;
            holded = 0;
            break;
        }
        case SND_BEEPER:
            if(isBeeper) beepVal = (uint16_t)(val ? volTable[beeperVolume] : 0);
            break;
    }
}

void zxSound::envelopeStep() {
    if(holded == 1) return;

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
                if(cont == 0) {
                    holded = 1;
                } else if(hold) {
                    if(alt) envVolume = 15;
                    holded = 1;
                } else {
                    if(alt) up = 1;
                    else envVolume = 15;
                }
            }
        }
    }
}

void zxSound::mix(uint16_t* buf, int num, int offs, int count, ...) {
    va_list ap;

    va_start(ap, count);
    for(int i = 0; i < count; i++) mixing_ch[i] = va_arg(ap, uint16_t*);
    for(int i = 0; i < SND_TICKS_PER_FRAME[num]; i++) {
        buf[i * 2 + offs] = 0;
        for(int j = 0; j < count; j++) buf[i * 2 + offs] += mixing_ch[j][i << num] / count;
    }
    va_end(ap);
}

void zxSound::AY_CHANNEL::gen(int i, int period, uint8_t volume, int ch_vol) {
    if(genCounter >= genPeriod) genCounter = 0;
    bool tone = genCounter < (genPeriod / 2);
    genCounter++;
    if(noiseCounter >= period) { noiseCounter = 0; rnd = rand(); }
    bool noise = (bool)(rnd & 1);
    noiseCounter++;
    auto tmp = (((tone | isChannel) & (noise | isNoise)) | !genPeriod);
    channelData[i] = (uint16_t)(tmp ? (_REGISTERS[ch_vol] & 0x10) ? volTable[volume] : volTable[_REGISTERS[ch_vol]] : 0);
}

void zxSound::update() {
    if(!isInitSL) initSL();

    if(!isAY) return;

    if(nSamplers > 0) {
        if(*zxALU::_TICK == 0) *zxALU::_TICK = 1;
        for(int i = 0; i < nSamplers; i++) SAMPLER[i].pos = (SND_TICKS_PER_FRAME[0] - 1) * SAMPLER[i].tcounter / *zxALU::_TICK;
    }
    // Генерим звук
    int j = 0;
    for(int i = 0; i < SND_TICKS_PER_FRAME[0]; i++) {
        if(nSamplers > 0) {
            while(i == SAMPLER[j].pos) {
                applyRegister(SAMPLER[j].reg, SAMPLER[j].val);
                SAMPLER[j].pos = SND_TICKS_PER_FRAME[0];
                j++;
                if(j >= SND_ARRAY_LEN) break;
            }
        }
        envelopeStep();
        channelA.gen(i, periodN, envVolume, AY_AVOL);
        channelB.gen(i, periodN, envVolume, AY_BVOL);
        channelC.gen(i, periodN, envVolume, AY_CVOL);
        beeperBuffer[i] = beepVal;
    }
    // Микшируем и записываем
    uint16_t* sndBuf= soundBuffer;
    uint16_t* bufA  = channelA.channelData;
    uint16_t* bufB  = channelB.channelData;
    uint16_t* bufC  = channelC.channelData;
    switch(stereoAY) {
        default:
            mix(sndBuf, freq, 0, 4, bufA, bufB, bufC, beeperBuffer);
            mix(sndBuf, freq, 1, 4, bufA, bufB, bufC, beeperBuffer);
            break;
        case AY_STEREO_ABC:
            mix(sndBuf, freq, 0, 3, bufA, bufB, beeperBuffer);
            mix(sndBuf, freq, 1, 3, bufC, bufB, beeperBuffer);
            break;
        case AY_STEREO_ACB:
            mix(sndBuf, freq, 0, 3, bufA, bufC, beeperBuffer);
            mix(sndBuf, freq, 1, 3, bufB, bufC, beeperBuffer);
            break;
    }
    nSamplers = 0;
    *zxALU::_TICK = 0;
    play();
}

void zxSound::play() {
/*
    char* memory = new char[len];
    memcpy(memory, buffer, len);

    WAVEHDR* curBuf(&bufs[nCurrent]);
    WAVEHDR* oldBuf((nCurrent < (NUM_BUFFERS - 1)) ? &bufs[nCurrent + 1] : &bufs[0]);
    ssh_memzero(curBuf, sizeof(WAVEHDR));
    curBuf->lpData = memory;
    curBuf->dwBufferLength = len;
    curBuf->dwFlags = 0;
    curBuf->dwLoops = 0;
    waveOutPrepareHeader(hsnd, curBuf, sizeof(WAVEHDR));
    waveOutWrite(hsnd, curBuf, sizeof(WAVEHDR));
    while(waveOutUnprepareHeader(hsnd, oldBuf, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) Sleep(15);
    SAFE_A_DELETE(oldBuf);
	nCurrent++;
	if(nCurrent >= NUM_BUFFERS) nCurrent = 0;

*/

/*
	SLObjectItf outputMixObj;
	const SLInterfaceID pOutputMixIDs[] = {};
	const SLboolean pOutputMixRequired[] = {};
//Аналогично slCreateEngine()
	result = (*engine)->CreateOutputMix(engine, &outputMixObj, 0, pOutputMixIDs, pOutputMixRequired);
	result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);

	struct WAVHeader{
		char                RIFF[4];
		unsigned long       ChunkSize;
		char                WAVE[4];
		char                fmt[4];
		unsigned long       Subchunk1Size;
		unsigned short      AudioFormat;
		unsigned short      NumOfChan;
		unsigned long       SamplesPerSec;
		unsigned long       bytesPerSec;
		unsigned short      blockAlign;
		unsigned short      bitsPerSample;
		char                Subchunk2ID[4];
		unsigned long       Subchunk2Size;
	};
	struct SoundBuffer{
		WAVHeader* header;
		char* buffer;
		int length;
	};
//Для чтения буфера PCM из файла используется AAssetManager:
	SoundBuffer* loadSoundFile(const char* filename){
		SoundBuffer* result = new SoundBuffer();
		AAsset* asset = AAssetManager_open(assetManager, filename, AASSET_MODE_UNKNOWN);
		off_t length = AAsset_getLength(asset);
		result->length = length - sizeof(WAVHeader);
		result->header = new WAVHeader();
		result->buffer = new char[result->length];
		AAsset_read(asset, result->header, sizeof(WAVHeader));
		AAsset_read(asset, result->buffer, result->length);
		AAsset_close(asset);
		return result;
	}

//Данные, которые необходимо передать в CreateAudioPlayer() для создания буферизованного плеера
	//SLDataLocator_AndroidSimpleBufferQueue locatorBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1}; Один буфер в очереди Информация, которую можно взять из заголовка wav
 	SLDataFormat_PCM formatPCM = {
			SL_DATAFORMAT_PCM,  1, SL_SAMPLINGRATE_44_1,
			SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
			SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audioSrc = {&locatorBufferQueue, &formatPCM};
	SLDataLocator_OutputMix locatorOutMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
	SLDataSink audioSnk = {&locatorOutMix, NULL};
	const SLInterfaceID pIDs[1] = {SL_IID_BUFFERQUEUE};
	const SLboolean pIDsRequired[1] = {SL_BOOLEAN_TRUE };
Создаем плеер
	result = (*engine)->CreateAudioPlayer(engine, &playerObj, &audioSrc, &audioSnk, 1, pIDs, pIDsRequired);
	result = (*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE);

	SLPlayItf player;
	result = (*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &player);
	SLBufferQueueItf bufferQueue;
	result = (*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &bufferQueue);
	result = (*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING);

	SoundBuffer* sound = loadSoundFile("mySound.wav");
	(*soundsBufferQueue)->Clear(bufferQueue); Очищаем очередь на случай, если там что-то было. Можно опустить, если хочется, чтобы очередь реально была очередью
	(*soundsBufferQueue)->Enqueue(bufferQueue, sound->buffer, sound->length);
Не забудьте почистить за собой SoundBuffer, когда он перестанет быть нужен
 */
}

bool zxSound::initSL() {
    isInitSL = true;
/*
	SLObjectItf engineObj;
	const SLInterfaceID pIDs[1] = {SL_IID_ENGINE};
	const SLboolean pIDsRequired[1]  = {SL_TRUE};
	SLresult result = slCreateEngine(
			&engineObj, Указатель на результирующий объект
			0, Количество элементов в массиве дополнительных опций
			NULL, Массив дополнительных опций, NULL, если они Вам не нужны
			1, Количество интерфесов, которые должен будет поддерживать создаваемый объект
			pIDs, Массив ID интерфейсов
			pIDsRequired Массив флагов, указывающих, необходим ли соответствующий интерфейс. Если указано SL_TRUE,
			а интерфейс не поддерживается, вызов завершится неудачей, с кодом возврата SL_RESULT_FEATURE_UNSUPPORTED
	);
Проверяем результат. Если вызов slCreateEngine завершился неуспехом – ничего не поделаешь
	if(result != SL_RESULT_SUCCESS){
		LOGE("Error after slCreateEngine");
		return;
	}
Вызов псевдометода. Первым аргументом всегда идет аналог this
	result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE); //Реализуем объект  в синхронном режиме
 В дальнейшем я буду опускать проверки результата, дабы не загромождать код
	if(result != SL_RESULT_SUCCESS){
		LOGE("Error after Realize engineObj");
		return;


		SLEngineItf engine;
		result = (*engineObj)->GetInterface(
				engineObj,  this
				SL_IID_ENGINE, ID интерфейса
				&engine Куда поместить результат
		);
*/
    return true;
}
