//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxTape.h"

struct WAV {
    uint32_t fileID;		// RIFF
    uint32_t fileSize;		// len + 8 + 12 + 16
    uint32_t waveID;		// WAVE
    uint32_t chunkID;		// fmt_
    uint32_t wFormatTag;	// 16
    uint16_t nChannels;		// 1/2
    uint16_t nChannels1;	// 1/2
    uint32_t nSamplesPerSec;// 11025/22050/44100
    uint32_t nAvgBytesPerSec;// 11025/22050/44100
    uint16_t nBlockAlign;	// 1
    uint16_t wBitsPerSample;// 8/16
    uint32_t nData;			// data
    uint32_t nDataSize;		// len
};

inline bool zxTape::checkBit(uint8_t* src, int pos) {
    return (src[pos >> 3] & numBits[7 - (pos % 8)]) != 0;
}

inline void zxTape::expandBit(uint8_t* dst, int pos, bool set) {
    int tmp = pos >> 3;
    dst[tmp] ^= (-set ^ dst[tmp]) & numBits[7 - (pos % 8)];
}

void zxTape::addBlock(uint8_t *data, uint16_t size) {
    for(uint32_t i = 0; i < 128; i++) {
        if(!blocks[i].data) {
            new(&blocks[i]) TAPE_BLOCK(data, size);
            if(countBlocks < 128) {
                countBlocks++;
            } else {
                info("Превышен лимит TAP блоков!");
            }
            break;
        }
    }
}

void zxTape::reset() {
    for(int i = 0 ; i < countBlocks; i++) {
        SAFE_DELETE(blocks[i].data);
    }
    countBlocks = currentBlock = 0;
    posImpulse = lenImpulse = 0;

    SAFE_A_DELETE(bufImpulse);
    sizeBufImpulse = 0;
}

bool zxTape::load(uint8_t* ptr, bool unpacked) {
    reset();

    while(true) {
        uint16_t size = *(uint16_t*)ptr; ptr += sizeof(uint16_t);
        if(size < 2) break;
        auto data = ptr;
        auto len = size;
        if(unpacked) {
            len	= *(uint16_t*)ptr; ptr += sizeof(uint16_t);
            if(!unpackBlock(ptr, TMP_BUF, TMP_BUF + size, len, true, false))
                return false;
            data = TMP_BUF;
        }
        info("load TAP len: %i", len);
        addBlock(data, size);
        ptr += len;
    }
    return true;
}

uint8_t* zxTape::save(int root, uint8_t* buf, bool packed) {
    while(root < countBlocks) {
        auto blk = &blocks[root++];
        auto data = blk->data;
        auto size = blk->size;
        *(uint16_t*)buf = size; buf += sizeof(uint16_t);
        if(packed) {
            uint32_t nsz;
            data = packBlock(data, data + size, buf + 2, false, nsz);
            *(uint16_t*)buf = (uint16_t)nsz; buf += sizeof(uint16_t);
            size = (uint16_t)nsz;
        }
        ssh_memcpy(&buf, data, size);
    }
    *(uint16_t*)buf = 0; buf += sizeof(uint16_t);
    return buf;
}

bool zxTape::loadState(uint8_t *ptr) {
    if(*(uint32_t*)ptr != 'epaT') return false;
    ptr += sizeof(uint32_t);
    return load(ptr, true);
}

uint8_t *zxTape::saveState(uint8_t *ptr) {
    *(uint32_t*)ptr = 'Tape'; ptr += sizeof(uint32_t);
    return save(currentBlock, ptr, true);
}

bool zxTape::openTAP(const char *path) {
    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[262144], true);
    if(!ptr) return false;
    return load(ptr, false);
}

bool zxTape::saveTAP(const char *path) {
    auto buf = save(0, TMP_BUF, false);
    return zxFile::writeFile(path, TMP_BUF, buf - TMP_BUF, true);
}

bool zxTape::openWAV(const char *path) {

    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[262144], true);
    if(!ptr) return false;
    ptr += 14; // skip

    WAV wav;

    wav.wFormatTag		= *(uint16_t*)ptr; ptr += sizeof(uint16_t);
    // Only PCM files supported!
    if(wav.wFormatTag != 1)
        return false;

    wav.nChannels 		= *(uint16_t*)ptr; ptr += sizeof(uint16_t);
    // Only MONO and STEREO files supported!
    if(wav.nChannels != 2 && wav.nChannels != 1)
        return false;

    wav.nSamplesPerSec 	= *(uint32_t*)ptr; ptr += sizeof(uint32_t) + 6;
    // Supported SamplesPerSec are : 11025, 22050, 44100!
    if(wav.nSamplesPerSec != 11025 && wav.nSamplesPerSec != 44100 && wav.nSamplesPerSec != 22050)
        return false;

    wav.wBitsPerSample	= *(uint16_t*)ptr; ptr += sizeof(uint16_t) + 4;
    // Only 8 and 16 bit files supported!
    if(wav.wBitsPerSample != 8 && wav.wBitsPerSample != 16)
        return false;

    reset();

    uint8_t posBit = 0;
    uint32_t counter = 0;

    auto len32 = *(uint32_t*)ptr; ptr += sizeof(uint32_t);
    auto true_len = len32;

    if(wav.nSamplesPerSec == 11025) true_len *= 2;
    if(wav.wBitsPerSample == 16) { true_len /= 2; len32 /= 2; }
    if(wav.nChannels == 2) { true_len /= 2; len32 /= 2; }

    true_len /= 8;
    true_len++;

    auto data = new uint8_t[true_len];

    for(uint32_t i = 0; i < len32; i++) {
        uint16_t value;
        bool bit;
        if(wav.nChannels == 1) {
            if(wav.wBitsPerSample == 16) {
                value = *(uint16_t*)ptr;
                ptr += sizeof(uint16_t);
            } else {
                value = *ptr++;
            }
        } else {
            if(wav.wBitsPerSample == 16) {
                auto tmp1 = (*(uint16_t*)ptr) / 2; ptr += sizeof(uint16_t);
                auto tmp2 = (*(uint16_t*)ptr) / 2; ptr += sizeof(uint16_t);
                value = (uint16_t)(tmp1 + tmp2);
            }
            else {
                auto tmp1 = (*ptr++) / 2;
                auto tmp2 = (*ptr++) / 2;
                value = (uint8_t)(tmp1 + tmp2);
            }
        }
        if(wav.wBitsPerSample == 8) bit = value > 127; else bit = value < 32768;
        if(wav.nSamplesPerSec == 11025) {
            expandBit(data, counter++, bit);
            posBit = (uint8_t)((posBit + 1) & 7);
        }
        expandBit(data, counter++, bit);
        posBit = (uint8_t)((posBit + 1) & 7);
    }
    return true;
}

bool zxTape::saveWAV(const char *path) {
    // расчитать размер блока данных
    auto size = calcSizeBufferImpulse(true);
    auto buf = new uint8_t[size];
    zxFile f;
    if(f.open(zxFile::makePath(path, true).c_str(), zxFile::create_write)) {
        WAV wav{'FFIR', 0, 'EVAW', ' tmf', 16, 1, 1, (uint32_t)freq, (uint32_t)freq, 1, 8, 'atad', 0};
        f.write(&wav, sizeof(WAV));
        int len = 0, root = 0, tmp;
        size = 0;
        while(root < countBlocks) {
            len = 0;
            makeImpulseBuffer(buf, root, len);
            for (int i = 0; i < len; i++) {
                auto bt = (uint8_t)(checkBit(buf, i) ? 250 : 5);
                // TODO: сделать накопление хотя бы по 8 байт
                f.write(&bt, 1);
            }
            root++;
            size += len;
        }
        tmp = (size + 8 + 12 + 16);
        f.set_pos(4, 0); f.write(&tmp, 4);
        f.set_pos(sizeof(WAV) - 4, 0); f.write(&size, 4);
        delete[] buf;
    }
    return true;
}

void zxTape::writePort(uint8_t value) {
    auto mic    = (uint8_t)(value & 8);
    auto beep   = (uint8_t)(value & 16);
    //out = (uint8_t)(_MIC ? 250: 5);
    if (mic != _MIC) {
        _MIC = mic;
        snd->write(15, mic);
    }
    if (beep != _BEEP) {
        _BEEP = beep;
        snd->write(15, beep);
    }
}

void zxTape::updateProps() {
    freq = frequencies[opts[ZX_PROP_SND_FREQUENCY]];
    isTrap = opts[ZX_PROP_TRAP_TAPE];
}

void zxTape::updateImpulseBuffer(bool force) {
    if(currentBlock < countBlocks) {
        lenImpulse = posImpulse = 0;
        if(!force) {
            skipMillis = 1000;
        } else {
            snd->reset();
            // определить размер буфера и пересоздать его
            auto size = calcSizeBufferImpulse(false);
            if(size > sizeBufImpulse) {
                bufImpulse = new uint8_t[size];
                sizeBufImpulse = size;
            }
            koef = 0;
            makeImpulse(44100.0 * skipMillis / 1000.0, bufImpulse, lenImpulse);
            skipMillis = 0;
            makeImpulseBuffer(bufImpulse, currentBlock, lenImpulse);
        }
    }
}

size_t zxTape::calcSizeBufferImpulse(bool isSkip) {
    double divider = freq / 3500000.0;
    auto s1 = 4032 * (divider * 2168.0 * 2.0);
    auto s2 = divider * 667.0 + divider * 735.0;
    auto s3 = isSkip ? 0.0 : (44100.0 * skipMillis / 1000.0);
    auto s5 = 65536 * (divider * 1710.0 * 2.0);
    return (size_t)(((size_t)(s1 + s2 + s3 + freq) >> 3) + s5);
}

void zxTape::makeImpulse(double count, uint8_t* buf, int& len) {
    for(int j = 0; j < count; j++) expandBit(buf, len++, (koef & 1) == 1);
    koef++;
}

void zxTape::makeImpulseBuffer(uint8_t* buf, int idx, int& len) {
    int i;

    if(idx < countBlocks) {
        koef = 0;
        // делитель частоты
        double divider = freq / 3500000.0;
        // пилот-тон
        for (i = 0; i < 4032; i++) {
            makeImpulse(divider * 2168.0, buf, len);
            makeImpulse(divider * 2168.0, buf, len);
        }
        // настройщик
        makeImpulse(divider * 667.0, buf, len);
        makeImpulse(divider * 735.0, buf, len);
        // текущий блок
        auto current = &blocks[idx];
        // преобразуем биты блока в имульсы, в зависимости от делителя частоты
        for (i = 0; i < current->size; i++) {
            auto byte = current->data[i];
            for (int j = 0; j < 8; j++) {
                if (checkBit(&byte, j)) {
                    // единичный бит
                    makeImpulse(divider * 1710.0, buf, len);
                    makeImpulse(divider * 1710.0, buf, len);
                } else {
                    // нулевой бит
                    makeImpulse(divider * 855.0, buf, len);
                    makeImpulse(divider * 855.0, buf, len);
                }
            }
        }
        // пропуск
        makeImpulse(freq, buf, len);
    }
}

void zxTape::trapSave() {
    auto cpu    = ALU->cpu;
    auto a 	    = *cpu->_A;
    auto len    = (uint16_t)(*cpu->_DE + 2);
    auto ix     = *cpu->_IX;

    TMP_BUF[0]  = a;
    for(int i = 0; i < len - 2; i++) {
        auto b = rm8((uint16_t)(ix + i));
        a ^= b;
        TMP_BUF[i + 1] = b;
    }
    TMP_BUF[len - 1] = a;
    addBlock(TMP_BUF, len);
}

void zxTape::trapLoad() {
    auto cpu    = ALU->cpu;
    auto blk	= &blocks[currentBlock];
    auto len 	= blk->size - 2;
    auto data 	= blk->data + 1;
    auto ix 	= *cpu->_IX;
    info("trapLoad block: %i size: %i", currentBlock, len);
    for(int i = 0; i < len; i++) ::wm8(realPtr((uint16_t)(ix + i)), data[i]);
    *cpu->_DE -= len; *cpu->_IX += len;
    *cpu->_AF = 0x00B3;
    *cpu->_BC = 0x01B0;
    opts[_RH] = 0;
    opts[_RL] = data[len];
    if(nextBlock()) updateImpulseBuffer(false);
}

void zxTape::control(int ticks) {
    rewCounter -= ticks;
    if(rewCounter < -2) {
        rewCounter = (int)(3500000.0 / freq);
        // для записи в магнитофон
        if(posImpulse < lenImpulse) {
            posImpulse++;
            snd->write(15, (uint8_t) checkBit(bufImpulse, posImpulse));
            if (posImpulse >= lenImpulse) {
                if (nextBlock()) updateImpulseBuffer(false);
            }
        }
    }
}

bool zxTape::nextBlock() {
    currentBlock++;
    if(currentBlock >= countBlocks) reset();
    return currentBlock < countBlocks;
}

int zxTape::trap(uint16_t pc) {
    if(pc < 16384) {
        // активность TR DOS
        if(pc == 15616 || pc == 15619) {
            info("Перехват TRDOS");
            ALU->save("trdLoader.zx", ZX_CMD_IO_Z80);
            //(pageROM != PAGE_ROM[0] && *_MODEL > MODEL_128K) * ZX_TR_DOS
            //modifySTATE(, ZX_TRDOS);
            return 4;
        }
        if (pc == 1218) {
            if(isTrap) {
                // перехват SAVE ""
                trapSave();
                auto psp = ALU->cpu->_SP;
                *ALU->cpu->_PC = rm16(*psp);
                *psp += 2;
                return 4;
            }
        } else if ((pc == 1366 || pc == 1388) && currentBlock < countBlocks) {
            if(isTrap) {
                // перехват LOAD ""
                auto size = blocks[currentBlock].size - 2;
                auto de = *ALU->cpu->_DE;
                if (de != size) {
                    info("В перехватчике load "" недопустимый размер block: %i (DE: %i - BLOCK_SIZE: %i)!", currentBlock, de, size);
                } else {
                    trapLoad();
                    // переносим на конец процедуры загрузки
                    *ALU->cpu->_PC = 0x5e2;
                }
            } else {
                if (posImpulse >= lenImpulse)
                    updateImpulseBuffer(true);
            }
        }
    }
    return 0;
}

