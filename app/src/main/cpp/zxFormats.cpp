//
// Created by Sergey on 10.02.2020.
//

#include "zxCommon.h"
#include "zxFormats.h"
#include "zxStks.h"

static void packPage(uint8_t** buffer, uint8_t* src, uint8_t page) {
    uint32_t size;
    uint8_t* buf = *buffer;
    *buffer = packBlock(src, src + 16384, &buf[3], false, size);
    *(uint16_t*)buf = (uint16_t)size;
    buf[2] = page;
}

bool zxFormats::openZ80(const char *path) {
    size_t sz;
    HEAD1_Z80* head1 = nullptr;
    HEAD2_Z80* head2 = nullptr;
    HEAD3_Z80* head3 = nullptr;
    uint8_t model = MODEL_48;
    int version;
    int pages = 3;

    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[INDEX_OPEN], true, &sz);
    if(!ptr) { LOG_INFO("File %s not found!", path); return false; }
    int length = sizeof(HEAD1_Z80);
    head1 = (HEAD1_Z80*)ptr;
    auto isCompressed = (head1->STATE1 & 0x20) == 0x20;
    auto PC = head1->PC;
    if(PC == 0) {
        // версия 2 или 3
        head2 = (HEAD2_Z80*)ptr;
        length += head2->length + 2;
        auto mode = head2->hardMode;
        switch(length) {
            case 55:
                version = 2;
                model = (uint8_t)(mode < 3 ? MODEL_48 : MODEL_128);
                break;
            case 86: case 87:
                version = 3;
                model = (uint8_t)(mode < 4 ? MODEL_48 : MODEL_128);
                break;
            default: LOG_INFO("Недопустимый формат файла %s!", path); return false;
        }
        if(model == MODEL_48) {
            pages = (head2->emulateFlags & 0x80) ? 1 : 3;
        } else if(model == MODEL_128) {
            if(mode == 9) {
                model = MODEL_PENTAGON;
                pages = 32;
            } else if(mode == 10) {
                model = MODEL_SCORPION;
                pages = 16;
            } else pages = 8;
        } else {
            LOG_INFO("Неизвестное оборудование %i в (%s)", mode, path);
            return false;
        }
        LOG_DEBUG("version:%i length:%i model:%i pages:%i mode:%i model:%i", version, length, model, pages, mode, model);
        if(version >= 3) {
            head3 = (HEAD3_Z80*)ptr;
            head2 = &head3->head2;
        }
        head1 = &head2->head1;
        PC = head2->PC;
    }
    ptr += length;
    sz -= length;
    // формируем страницы во временном буфере
    if(head2) {
        for(int i = 0 ; i < pages; i++) {
            auto sizeData = *(uint16_t*)ptr; ptr += 2;
            auto numPage = *ptr++ - 3;
            isCompressed = sizeData != 0xFFFF;
            auto sizePage = (uint32_t)(isCompressed ? sizeData : 0x4000);
            auto isValidPage = false;
            switch(model) {
                case MODEL_48:
                    isValidPage = true;
                    switch(numPage) {
                        // 4->2 5->0 8->5
                        case 1: numPage = 2; break;
                        case 2: numPage = 0; break;
                        case 5: numPage = 5; break;
                        default: isValidPage = false; break;
                    }
                    break;
                case MODEL_128:
                case MODEL_PENTAGON:
                case MODEL_SCORPION:
                    isValidPage = numPage == i;
                    break;
            }
            if(!isValidPage) {
                LOG_DEBUG("Неизвестная страница %i-%i в (%s)!", numPage, i, path);
                return false;
            }
            auto page = &TMP_BUF[numPage << 14];
            if(!unpackBlock(ptr, page, page + 16384, sizePage, isCompressed)) {
                LOG_DEBUG("Ошибка при распаковке страницы %i в %s!", numPage, path);
                return false;
            }
            ptr += sizePage;
            sz -= (sizePage + 3);
        }
    } else {
        if(!unpackBlock(ptr, TMP_BUF, &TMP_BUF[49152], sz, isCompressed)) {
            LOG_DEBUG("Ошибка при распаковке %s!", path);
            return false;
        }
        // перераспределяем буфер 0->5, 1->2, 2->0
        memcpy(&TMP_BUF[5 << 14], &TMP_BUF[0 << 14], 16384);
        memcpy(&TMP_BUF[0 << 14], &TMP_BUF[2 << 14], 16384);
        memcpy(&TMP_BUF[2 << 14], &TMP_BUF[1 << 14], 16384);
    }
    // меняем модель памяти и иинициализируем регистры
    auto cpu = zx->cpu;
    zx->changeModel(model);
    *cpu->_BC = head1->BC; *cpu->_DE = head1->DE; *cpu->_HL = head1->HL;
    *cpu->_A = head1->A; *cpu->_F = head1->F; opts[RA_] = head1->A_; opts[RF_] = head1->F_;
    *cpu->_SP = head1->SP; *cpu->_IX = head1->IX; *cpu->_IY = head1->IY;
    *cpu->_I = head1->I; *cpu->_IM = (uint8_t)(head1->STATE2 & 3);
    *cpu->_IFF1 = head1->IFF1; *cpu->_IFF2 = head1->IFF2;
    if(head1->STATE1 == 255) head1->STATE1 = 1;
    *cpu->_R |= (head1->STATE1 << 7) | (head1->R & 0x7F);
    zx->writePort(0xfe, 0, (uint8_t)(224 | ((head1->STATE1 & 14) >> 1)));
    memcpy(&opts[RC_], &head1->BC_, 6);
    *cpu->_PC = PC;
    LOG_INFO("Z80 Start PC: %i", PC);
    if(head2) {
        zx->writePort(0xfd, 0x7f, head2->hardState);
        memcpy(&opts[AY_AFINE], head2->sndRegs, 16);
        zx->writePort(0xfd, 0xff, head2->sndChipRegNumber);
    }
    if(head3 && length == 87) zx->writePort(0xfd, 0x1f, head3->port1FFD);
    // копируем буфер
    memcpy(zx->RAMbs, TMP_BUF, ZX_TOTAL_RAM);
    return true;
}

bool zxFormats::saveZ80(const char *path) {
    static uint8_t models[] = { 0, 0, 0, 4, 4, 4, 9, 10 };
    static HEAD3_Z80 head;

    auto head2 = &head.head2;
    auto head1 = &head2->head1;
    memset(&head, 0, sizeof(HEAD3_Z80));

    auto buf = TMP_BUF;
    auto cpu = zx->cpu;
    auto ram = zx->RAMbs;
    // основные
    head1->BC = *cpu->_BC; head1->DE = *cpu->_DE; head1->HL = *cpu->_HL;
    head1->A = *cpu->_A; head1->F = *cpu->_F; head1->A_ = opts[RA_]; head1->F_ = opts[RF_];
    head1->SP = *cpu->_SP; head1->IX = *cpu->_IX; head1->IY = *cpu->_IY; head1->PC = 0;
    head1->STATE2 = *cpu->_IM; head1->IFF1 = *cpu->_IFF1; head1->IFF2 = *cpu->_IFF2;
    head1->I = *cpu->_I; head1->R = (uint8_t)(*cpu->_R & 127);
    head1->STATE1 = (uint8_t)((*cpu->_R & 128) >> 7) | (uint8_t)((opts[PORT_FE] & 7) << 1);
    memcpy(&head1->BC_, &opts[RC_], 6);
    // для режима 128К
    head2->PC = *cpu->_PC;
    head2->hardMode = models[*zx->_MODEL];
    head2->length = 55;
    head.port1FFD = opts[PORT_1FFD];
    head2->hardState = opts[PORT_7FFD];
    head2->sndChipRegNumber = opts[AY_REG];
    memcpy(head2->sndRegs, &opts[AY_AFINE], 16);
    ssh_memzero(head2->sndRegs, 16);
    // формируем буфер из содержимого страниц
    ssh_memcpy(&buf, &head, sizeof(HEAD3_Z80));
    // страницы, в зависимости от режима
    if(head2->hardMode < MODEL_128) {
        // 4->2 5->7 8->5
        packPage(&buf, &ram[5 << 14], 8);
        packPage(&buf, &ram[2 << 14], 4);
        packPage(&buf, &ram[7 << 14], 5);
    } else {
        auto count = zxSpeccy::machine->ramPages;
        LOG_INFO("saveZ80 pages:%i model:%i", count, head2->hardMode);
        for(int i = 0; i < count; i++) {
            packPage(&buf, &ram[i << 14], (uint8_t)(i + 3));
        }
    }
    return zxFile::writeFile(path, TMP_BUF, buf - TMP_BUF, true);
}

bool zxFormats::openWAV(const char *path) {
    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[INDEX_OPEN], true);
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

    auto tape = zx->tape;

    tape->reset();

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
            tape->expandBit(data, counter++, bit);
            posBit = (uint8_t)((posBit + 1) & 7);
        }
        tape->expandBit(data, counter++, bit);
        posBit = (uint8_t)((posBit + 1) & 7);
    }
    return true;
}

bool zxFormats::saveWAV(const char *path) {
    auto tape = zx->tape;
    // расчитать размер блока данных
    auto size = tape->calcSizeBufferImpulse(true);
    auto buf = new uint8_t[size];
    zxFile f;
    if(f.open(zxFile::makePath(path, true).c_str(), zxFile::create_write)) {
        WAV wav{'FFIR', 0, 'EVAW', ' tmf', 16, 1, 1, (uint32_t)tape->freq, (uint32_t)tape->freq, 1, 8, 'atad', 0};
        f.write(&wav, sizeof(WAV));
        int len = 0, root = 0, tmp;
        size = 0;
        while(root < tape->countBlocks) {
            len = 0;
            tape->makeImpulseBuffer(buf, root, len);
            for (int i = 0; i < len; i++) {
                auto bt = (uint8_t)(tape->checkBit(buf, i) ? 250 : 5);
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

bool zxFormats::openTAP(const char *path) {
    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[INDEX_OPEN], true);
    if(!ptr) return false;
    return zx->tape->load(ptr, false) != nullptr;
}

bool zxFormats::saveTAP(const char *path) {
    auto buf = zx->tape->save(0, TMP_BUF, false);
    return zxFile::writeFile(path, TMP_BUF, buf - TMP_BUF, true);
}

bool zxFormats::openSNA(const char *path) {
    return false;
}

bool zxFormats::saveSNA(const char *path) {
    return false;
}

bool zxFormats::openTZX(const char *path) {
    return false;
}

bool zxFormats::saveTZX(const char *path) {
    return false;
}

bool zxFormats::openRZX(const char *path) {
    return false;
}

bool zxFormats::saveRZX(const char *path) {
    return false;
}

bool zxFormats::openTRD(const char *path) {
    return false;
}

bool zxFormats::saveTRD(const char *path) {
    return false;
}

bool zxFormats::openSCL(const char *path) {
    return false;
}

bool zxFormats::saveSCL(const char *path) {
    return false;
}

bool zxFormats::openFDI(const char *path) {
    return false;
}

bool zxFormats::saveFDI(const char *path) {
    return false;
}

bool zxFormats::openUDI(const char *path) {
    return false;
}

bool zxFormats::saveUDI(const char *path) {
    return false;
}

bool zxFormats::openTD0(const char *path) {
    return false;
}

bool zxFormats::saveTD0(const char *path) {
    return false;
}
/*

int FDD::read_hob()
{
    if (!rawdata) emptydisk();
    snbuf[13] = snbuf[14];
    int r = addfile(snbuf, snbuf+0x11);
    return r;
}
*/
