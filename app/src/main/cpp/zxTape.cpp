//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxTape.h"

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

uint8_t* zxTape::load(uint8_t* ptr, bool unpacked) {
    reset();

    while(true) {
        uint16_t size = *(uint16_t*)ptr; ptr += 2;
        if(size < 2) break;
        auto data = ptr;
        auto len = size;
        if(unpacked) {
            len	= *(uint16_t*)ptr; ptr += sizeof(uint16_t);
            if(!unpackBlock(ptr, TMP_BUF, TMP_BUF + size, len, true))
                return nullptr;
            data = TMP_BUF;
        }
/*
        if(data[1] == 0) {
            data[14] = 0;
            data[15] = 128;
        }
*/
        addBlock(data, size);
        ptr += len;
    }
    if(unpacked) {

    }
    return ptr;
}

uint8_t* zxTape::save(int root, uint8_t* buf, bool packed) {
    while(root < countBlocks) {
        auto blk = &blocks[root++];
        auto data = blk->data;
        auto size = blk->size;
        *(uint16_t*)buf = size; buf += sizeof(uint16_t);
        if(packed) {
            uint32_t nsz;
            packBlock(data, data + size, buf + 2, false, nsz);
            *(uint16_t*)buf = (uint16_t)nsz;
            buf += sizeof(uint16_t) + nsz;
        } else {
            ssh_memcpy(&buf, data, size);
        }
    }
    *(uint16_t*)buf = 0; buf += sizeof(uint16_t);
    if(packed) {
        // параметры импульсов, если, например, мы грузили через них

    }
    return buf;
}

uint8_t *zxTape::restoreState(uint8_t *ptr) {
    // сигнатура
    if(ptr[0] != 'S' || ptr[1] != 'E' || ptr[2] != 'R' || ptr[3] != 'G') {
        LOG_INFO("Сигнатура ленты некорректна!", 0);
        return ptr;
    }
    ptr += 4;
    return load(ptr, true);
}

uint8_t *zxTape::saveState(uint8_t *ptr) {
    // сигнатура
    *ptr++ = 'S'; *ptr++ = 'E'; *ptr++ = 'R'; *ptr++ = 'G';
    return save(currentBlock, ptr, true);
}

void zxTape::writePort(uint8_t value) {
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
            auto uint8_t = current->data[i];
            for (int j = 0; j < 8; j++) {
                if (checkBit(&uint8_t, j)) {
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

bool zxTape::trapSave() {
    if(!isTrap) return false;

    auto cpu = ULA->cpu;
    auto a = *cpu->_A;
    auto len = (uint16_t) (*cpu->_DE + 2);
    auto ix = *cpu->_IX;

    TMP_BUF[0] = a;
    for (int i = 0; i < len - 2; i++) {
        auto b = rm8((uint16_t) (ix + i));
        a ^= b;
        TMP_BUF[i + 1] = b;
    }
    TMP_BUF[len - 1] = a;
    addBlock(TMP_BUF, len);
    return true;
}

bool zxTape::trapLoad() {
    if (isTrap && currentBlock < countBlocks) {
        auto blk = &blocks[currentBlock];
        auto len = blk->size - 2;
        auto data = blk->data + 1;

        auto cpu = ULA->cpu;
        auto de = *cpu->_DE;
        auto ix = *cpu->_IX;
        if (de != len) LOG_INFO("В перехватчике LOAD отличаются блоки - block: %i (DE: %i != SIZE: %i)!", currentBlock, de, len);
        if (de < len) len = *cpu->_DE;
        for (int i = 0; i < len; i++) ::wm8(realPtr((uint16_t) (ix + i)), data[i]);
        LOG_DEBUG("trapLoad PC: %i load: %i type: %i addr: %i size: %i", zxULA::PC, *cpu->_F & 1, *cpu->_A, ix, len);
        *cpu->_AF = 0x00B3;
        *cpu->_BC = 0xB001;
        opts[_RH] = 0; opts[_RL] = data[len];
        if (nextBlock()) updateImpulseBuffer(false);
        ULA->pauseBetweenTapeBlocks = 50;
        modifySTATE(ZX_PAUSE, 0);
        return true;
    } else {
        if (posImpulse >= lenImpulse)
            updateImpulseBuffer(true);
    }
    return false;
}

void zxTape::control(int ticks) {
    rewCounter -= ticks;
    if(rewCounter < -2) {
        rewCounter = (int)(3500000.0 / freq);
        // для записи в магнитофон
        if(posImpulse < lenImpulse) {
            posImpulse++;
//            snd->beeperWrite((uint8_t) checkBit(bufImpulse, posImpulse));
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
const char* zxTape::getBlockData(int index, uint16_t *data) {
    auto name = (char*)&TMP_BUF[INDEX_TEMP];
    memset(name, 32, 10);
    memset(data, 255, 7 * 2);

    if(index < countBlocks && index >= 0) {
        auto addr = blocks[index].data;
        auto size = blocks[index].size - 2;
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
    }
    return name;
}

uint8_t zxTape::readPort() {
    return 0;
}

