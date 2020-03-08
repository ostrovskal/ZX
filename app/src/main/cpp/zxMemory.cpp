//
// Created by Sergey on 19.02.2020.
//

#include "zxCommon.h"
#include "zxDevs.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ПАМЯТЬ                                                 //
/////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t* zxDevMem::memPAGES[4];
uint8_t *zxDevMem::ROMb(nullptr);
uint8_t *zxDevMem::ROMe(nullptr);

zxDevMem::zxDevMem(): ROMtr(nullptr) {
    _1FFD   = &opts[PORT_1FFD];
    _7FFD   = &opts[PORT_7FFD];
    _RAM    = &opts[RAM];
    _ROM    = &opts[ROM];

    ROMtr = new uint8_t[1 << 14];

    reset();
}

zxDevMem::~zxDevMem() {
    SAFE_A_DELETE(ROMb);
    SAFE_A_DELETE(ROMtr);
}

void zxDevMem::write(uint16_t port, uint8_t val) {
    if(mode48K()) return;
    auto model = *zxSpeccy::_MODEL;
    // 0,2,5,12=1; 1,14,15=0
    if(model == MODEL_SCORPION && port == 0x1FFD) {
        // 0   -> 1 - RAM0(!!), 0 - see bit 1 etc
        // 1   -> 1 - ROM 2, 0 - ROM from 0x7FFD
        // 4   -> 1 - RAM SCORPION/KAY 256K, 0 - from 0x7FFD
        // 6.7 -> SCORPION/KAY 1024K
        *_1FFD = val;
        if (val & 1) *_ROM = 100;
        else {
            if (val & 2) *_ROM = 2;
            else *_ROM = (uint8_t) ((*_7FFD & 16) >> 4);
        }
        *_RAM = (uint8_t) ((*_7FFD & 7) + (uint8_t) ((val & 16) >> 1));
    } else {
        // 0, 1, 2 - страница 0-7
        // 3 - экран 5/7
        // 4 - ПЗУ 0 - 128К 1 - 48К
        // 5 - блокировка
        // 6 - pentagon 256K, KAY 1024K
        // 7 - pentagon 512K
        *_7FFD = val;
        *_RAM = (uint8_t) (val & 7);
        switch (model) {
            case MODEL_SCORPION:
                if (!(*_1FFD & 2)) *_ROM = (uint8_t) ((val & 16) >> 4);
                *_RAM += (uint8_t) ((*_1FFD & 16) >> 1);
                break;
            case MODEL_PENTAGON:
                *_RAM += (uint8_t) ((val & 192) >> 3);
            default:
                *_ROM = (uint8_t) ((val & 16) >> 4);
                break;
        }
        if (val & 32) {
            LOG_INFO("block 7ffd %i PC:%i", val, zxSpeccy::PC);
        }
    }
    update();
}

void zxDevMem::reset() {
    *_RAM = 0;
    *_ROM = zxSpeccy::machine->startRom;
    memPAGES[1] = &zx->RAMbs[5 << 14];
    memPAGES[2] = &zx->RAMbs[2 << 14];
    if(*zxSpeccy::_MODEL < MODEL_128) *_7FFD = 32;
    update();
}

int zxDevMem::update(int param) {
    if(param) {
        zxFile file;
        auto totalRom   = zxSpeccy::machine->totalRom << 14;
        auto rom        = new uint8_t[totalRom];
        if(file.open((FOLDER_FILES + "rom.ezx").c_str(), zxFile::open_read)) {
            // trdos rom
            file.set_pos(zxSpeccy::machine->indexTRDOS << 14, zxFile::begin);
            file.read(ROMtr, 1 << 14);
            // base rom
            file.set_pos(zxSpeccy::machine->indexRom << 14, zxFile::begin);
            file.read(rom, totalRom);
            file.close();
            delete[] ROMb; ROMb = rom; ROMe = ROMb + totalRom;
            totalRom >>= 14;
            for(int i = 0 ; i < 4; i++) romPAGES[i] = rom +  ((i >= totalRom ? totalRom - 1 : i) << 14);
        } else {
            LOG_INFO("Не удалось загрузить ПЗУ для машины - %s!", zxSpeccy::machine->name);
            delete[] rom;
            return 0;
        }
        return 1;
    }
    if (checkSTATE(ZX_TRDOS)) {
        memPAGES[0] = ROMtr;
    } else {
        auto rom = *_ROM;
        memPAGES[0] = (rom != 100 ? romPAGES[rom] : zx->RAMbs);
    }
    memPAGES[3] = &zx->RAMbs[*_RAM << 14];
    return 0;
}

uint8_t *zxDevMem::state(uint8_t *ptr, bool restore) {
    uint32_t size;
    auto ram = zx->RAMbs;
    if(restore) {
        // грузим банки ОЗУ
        for (int i = 0; i < zxSpeccy::machine->ramPages; i++) {
            size = *(uint16_t *)ptr;
            ptr += sizeof(uint16_t);
            if (!unpackBlock(ptr, &ram[i << 14], &ram[i << 14] + 16384, size, true)) {
                LOG_DEBUG("Ошибка при распаковке страницы %i состояния!!!", i)
                return nullptr;
            }
            ptr += size;
        }
    } else {
        // количество банков и их содержимое
        for(int i = 0; i < zxSpeccy::machine->ramPages; i++) {
            packBlock(&ram[i << 14], &ram[i << 14] + 16384, &ptr[2], false, size);
            *(uint16_t*)ptr = (uint16_t)size; ptr += size + sizeof(uint16_t);
        }
    }
    return ptr;
}



