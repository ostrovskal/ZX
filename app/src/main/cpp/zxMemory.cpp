//
// Created by Sergey on 19.02.2020.
//

#include "zxCommon.h"
#include "zxDevs.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                      ПАМЯТЬ                                                 //
/////////////////////////////////////////////////////////////////////////////////////////////////

// normal +3SOS
// -----------------
// |D2#1FFD|D4#7FFD|
// |-------|-------|
// |   0   |   0   | +3 Editor
// |---------------|
// |   0   |   1   | +3 Syntax
// |-------|-------|
// |   1   |   0   | +3 DOS
// |-------|-------|
// |   1   |   1   | +3 SOS
// -----------------
// special +3DOS
// -------------------------------------
// | D2| D1| CPU0 | CPU1 | CPU2 | CPU3 |
// -------------------------------------
// | 0 | 0 |  0   |  1   |  2   |  3   |
// -------------------------------------
// | 0 | 1 |  4   |  5   |  6   |  7   |
// -------------------------------------
// | 1 | 0 |  4   |  5   |  6   |  3   |
// -------------------------------------
// | 1 | 1 |  4   |  7   |  6   |  3   |
// -------------------------------------

static uint8_t mem1FFD[] = { 0, 1, 2, 3,  4, 5, 6, 7,  4, 5, 6, 3,  4, 7, 6, 3,  0, 5, 2, 0 };

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
    if((model == MODEL_SCORPION || model == MODEL_PLUS3) && ((port & 0b0001000000100101) == 0b0001000000100101)) {
        // 0   -> 1 - RAM0(!!), 0 - see bit 1 etc
        // 1   -> 1 - ROM 2, 0 - ROM from 0x7FFD
        // 4   -> 1 - RAM SCORPION/KAY 256K, 0 - from 0x7FFD
        // 6.7 -> SCORPION/KAY 1024K
        *_1FFD = val;
        uint8_t* ram(&mem1FFD[16]);
        switch (model) {
            case MODEL_SCORPION:
                if (val & 1) *_ROM = ram[0];
                else {
                    if (val & 2) *_ROM = 2;
                    else *_ROM = (uint8_t) ((*_7FFD & 16) >> 4);
                }
                *_RAM = (uint8_t) ((*_7FFD & 7) + (uint8_t) ((val & 16) >> 1));
                break;
            case MODEL_PLUS3:
                if (val & 1) {
                    auto rom = (val & 3) << 2;
                    ram = &mem1FFD[rom];
                    memPAGES[0] = &zx->RAMbs[ram[0] << 14];
                    *_ROM = (uint8_t) (rom + 100);
                    *_RAM = ram[3];
                    memPAGES[1] = &zx->RAMbs[ram[1] << 14];
                    memPAGES[2] = &zx->RAMbs[ram[2] << 14];
                } else {
                    *_ROM = (uint8_t) (((val & 4) >> 1) | ((*_7FFD & 16) >> 4));
                    //*_RAM = (uint8_t)(*_7FFD & 7);
                    //auto motor = (uint8_t)((val & 8) != 0);
                    //auto strob = (uint8_t)((val & 16) != 0);
                }
                LOG_INFO("plus3 val: %i ROM:%i RAM:%i PC:%X", val, *_ROM, *_RAM, zxSpeccy::PC);
                break;
        }
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
        if(file.open((FOLDER_FILES + "rom.zx").c_str(), zxFile::open_read)) {
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
        if (rom < 100) {
            memPAGES[0] = romPAGES[rom];
        } else {
            memPAGES[0] = &zx->RAMbs[mem1FFD[rom - 100]];
        }
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



