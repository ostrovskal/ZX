//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxALU.h"
#include "stkMnemonic.h"
#include "zxDA.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

// текущий счетчик инструкций
uint32_t* zxALU::_TICK(nullptr);

uint16_t* zxALU::_CALL(nullptr);

uint16_t zxALU::PC(0);

// актуальные страницы
uint8_t* zxALU::pageTRDOS(nullptr);
uint8_t* zxALU::pageROM(nullptr);
uint8_t* zxALU::pageRAM(nullptr);
uint8_t* zxALU::pageVRAM(nullptr);
uint8_t* zxALU::pageATTRIB(nullptr);

// страницы RAM
uint8_t* zxALU::PAGE_RAM[16];

// STATE
uint8_t* zxALU::_STATE(nullptr);

static uint32_t frames[2] = { 0, 0 };

static uint32_t banks[] = {8, 1, 8, 1, 8, 1, 8, 2, 8, 2, 16, 4, 16, 4};

static int pages_rom[8] = {
        ZX_ROM_KOMPANION,
        ZX_ROM_48,
        ZX_ROM_48N,
        ZX_ROM_128,
        ZX_ROM_PENTAGON,
        ZX_ROM_SCORPION,
        ZX_ROM_PROFI,
        ZX_ROM_TRDOS
};

static int modelParams[] = {
        8960, 8, 56, 7168, // 69888, 16192  48K
        8960, 8, 56, 7168, // 69888, 16192  48K
        8892, 8, 60, 7296, // 70908, 16256  128K
        12544,52,12, 5376, // 71680, 17984  Pentagon
        8960, 48,16, 7168, // 69888  16192  Scorpion
};

static uint8_t semiRows[] = {
        0, 0x00, 0, 0, 3, 0x01, 0, 0, 3, 0x02, 0, 0, 3, 0x04, 0, 0, 3, 0x08, 0, 0,
        3, 0x10, 0, 0, 4, 0x10, 0, 0, 4, 0x08, 0, 0, 4, 0x04, 0, 0, 4, 0x02, 0, 0,// 5
        4, 0x01, 0, 0, 4, 0x01, 0, 1, 2, 0x01, 0, 0, 2, 0x02, 0, 0, 2, 0x04, 0, 0,// 10
        2, 0x08, 0, 0, 2, 0x10, 0, 0, 5, 0x10, 0, 0, 5, 0x08, 0, 0, 5, 0x04, 0, 0,// 15
        5, 0x02, 0, 0, 5, 0x01, 0, 0, 6, 0x01, 0, 0, 0, 0x01, 0, 0, 1, 0x01, 0, 0,// 20
        1, 0x02, 0, 0, 1, 0x04, 0, 0, 1, 0x08, 0, 0, 1, 0x10, 0, 0, 6, 0x10, 0, 0,// 25
        6, 0x08, 0, 0, 6, 0x04, 0, 0, 6, 0x02, 0, 0, 7, 0x02, 0, 0, 0, 0x01, 7, 2,// 30
        0, 0x02, 0, 0, 0, 0x04, 0, 0, 0, 0x08, 0, 0, 7, 0x01, 0, 0, 0, 0x10, 0, 0,// 35
        7, 0x10, 0, 0, 7, 0x08, 0, 0, 7, 0x04, 0, 0,// 40
        4, 0x08, 0, 1, 4, 0x10, 0, 1, 3, 0x10, 0, 1, 4, 0x04, 0, 1,// 43
        8, 0x02, 0, 0, 8, 0x01, 0, 0, 8, 0x08, 0, 0, 8, 0x04, 0, 0, 8, 0x10, 0, 0
};

static void packPage(uint8_t** buffer, uint8_t* src, uint8_t page) {
    uint32_t size;
    uint8_t* buf = *buffer;
    *buffer = packBlock(src, src + 16384, &buf[3], false, size);
    *(uint16_t*)buf = (uint16_t)size;
    buf[2] = page;
}

zxALU::zxALU() : pauseBetweenTapeBlocks(0), joyOldButtons(0), periodGPU(0), _FF(255), colorBorder(7),
                 cpu(nullptr), snd(nullptr), tape(nullptr), disk(nullptr), gpu(nullptr), ROMS(nullptr) {

    blink = blinkMsk = blinkShift = 0;
    sizeBorder = 0;

    RAMs = new uint8_t[ZX_TOTAL_RAM];

    for (int i = 0; i < 16; i++) PAGE_RAM[i] = (RAMs + i * 16384);

    // инициализация системы
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE]; *_STATE = 0;
    _KEMPSTON		= &opts[ZX_PROP_VALUES_KEMPSTON];
    _1FFD    		= &opts[PORT_1F];
    _7FFD    		= &opts[PORT_FD];
    _FE             = &opts[PORT_FE];

    zxALU::_RAM     = &opts[RAM];
    zxALU::_VID     = &opts[VID];
    zxALU::_ROM     = &opts[ROM];
    zxALU::_TICK    = (uint32_t*)&opts[TICK0];
    zxALU::_CALL    = (uint16_t*)&opts[CALL0];

    cpu = new zxCPU();
    gpu = new zxGPU();

    snd = new zxSound();
    tape = new zxTape(snd);
    disk = new zxDisk();

    assembler = new zxAssembler();
    debugger = new zxDebugger();
}

zxALU::~zxALU() {
    SAFE_DELETE(assembler);
    SAFE_DELETE(debugger);

    SAFE_DELETE(disk);
    SAFE_DELETE(tape);
    SAFE_DELETE(snd);

    SAFE_DELETE(gpu);
    SAFE_DELETE(cpu);

    SAFE_A_DELETE(RAMs);
}

bool zxALU::load(int dsk, const char *path, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_TRD:
            ret = disk->openTRD(dsk, path);
            break;
        case ZX_CMD_IO_WAVE:
            ret = tape->openWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = tape->openTAP(path);
            if(ret) ret = openZ80("tapLoader.zx");
            break;
        case ZX_CMD_IO_STATE:
            ret = openState(path);
            break;
        case ZX_CMD_IO_Z80:
            ret = openZ80(path);
            break;
    }
    return ret;
}

bool zxALU::openState(const char *path) {
    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[262144], false);
    if(!ptr) return false;
    // меняем модель
    changeModel(ptr[MODEL], *_MODEL, true);
    // грузим регистры
    memcpy(opts, ptr, COUNT_REGS); ptr += COUNT_REGS;
    // грузим банки ОЗУ
    auto pages = banks[*_MODEL * 2];
    for (int i = 0; i < pages; i++) {
        size_t size = *(uint16_t*)ptr; ptr += sizeof(uint16_t);
        if (!unpackBlock(ptr, PAGE_RAM[i], PAGE_RAM[i] + 16384, size, true, false)) {
            LOG_DEBUG("Ошибка при распаковке страницы %i состояния!!!", i)
            signalRESET(true);
            return false;
        }
        ptr += size;
    }
    auto ptrName = (const char*)ptr;
    ptr += strlen(ptrName) + 1;
    // восстанавливаем состояние ленты
    if(!tape->loadState(ptr)) {
        LOG_DEBUG("Не удалось восстановить состояние ленты!!!", nullptr)
        signalRESET(true);
        return false;
    }
    // восстанавливаем состояние дисков
    if(!disk->loadState(ptr)) {
        LOG_DEBUG("Не удалось восстановить состояние дисков!!!", nullptr)
        signalRESET(true);
        return false;
    }
    // восстанавление страниц
    setPages();
    // загрузить имя сохраненной проги
    programName(ptrName);
    return true;
}

bool zxALU::saveState(const char* path) {
    uint32_t size;
    auto buf = TMP_BUF;
    // сохраняем регистры
    modifySTATE(0, ZX_PAUSE);
    ssh_memcpy(&buf, opts, COUNT_REGS);
    // количество банков и их содержимое
    auto pages =  banks[*_MODEL * 2];
    for(int i = 0; i < pages; i++) {
        packBlock(PAGE_RAM[i], PAGE_RAM[i] + 16384, &buf[2], false, size);
        *(uint16_t*)buf = (uint16_t)size; buf += size + sizeof(uint16_t);
    }
    // сохранить имя проги
    ssh_memcpy(&buf, name.c_str(), (size_t)(name.length() + 1));
    // сохраняем состояние ленты
    buf = tape->saveState(buf);
    // сохраняем состояние дисков
    buf = disk->saveState(buf);
    // записываем
    return zxFile::writeFile(path, TMP_BUF, buf - TMP_BUF, false);
}

bool zxALU::openZ80(const char *path) {
    size_t sz;
    HEAD1_Z80* head1 = nullptr;
    HEAD2_Z80* head2 = nullptr;
    HEAD3_Z80* head3 = nullptr;
    uint8_t model = MODEL_48;

    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[262144], true, &sz);
    if(!ptr) return false;

    int length = sizeof(HEAD1_Z80);
    head1 = (HEAD1_Z80*)ptr;
    auto PC = head1->PC;
    if(PC == 0) {
        head2 = (HEAD2_Z80*)ptr;
        bool isMod = (head2->emulateFlags & 128) != 0;
        length += head2->length + 2;
        if(length > 55) {
            head3 = (HEAD3_Z80*)ptr;
            head2 = &head3->head2;
        }
        switch(head2->hardMode) {
            case 0: case 1: if(!isMod) model = MODEL_48; break;
            case 3: case 4: case 5: case 6: if(!isMod) model = MODEL_128; break;
            case 9: model = MODEL_PENTAGON; break;
            case 10: model = MODEL_SCORPION; break;
            default: LOG_DEBUG("Неизвестное оборудование в %s!", path); return false;
        }
        head1 = &head2->head1;
        PC = head2->PC;
    }
    ptr += length;
    sz -= length;
    // формируем страницы во временном буфере
    if(head2) {
        while(sz > 0) {
            uint16_t szData = *(uint16_t*)ptr; ptr += 2; sz -= 2;
            uint32_t szSrc(szData == 65535 ? 16384 : szData);
            int numPage = *ptr++ - 3; sz--;
            bool isPage = false;
            switch(model) {
                case MODEL_48:
                case MODEL_KOMPANION:
                    isPage = true;
                    switch(numPage) {
                        case 1: numPage = 2; break;
                        case 2: numPage = 0; break;
                        case 5: numPage = 5; break;
                        default: isPage = false;
                    }
                    break;
                case MODEL_128:
                case MODEL_PENTAGON:
                    isPage = (numPage >= 0 && numPage <= 7);
                    break;
                case MODEL_SCORPION:
                    isPage = (numPage >= 0 && numPage <= 15);
                    break;
            }
            if(!isPage) {
                LOG_DEBUG("Неизвестная страница %i в (%s)!", numPage, path);
                return false;
            }
            auto page = &TMP_BUF[numPage * 16384];
            if(!unpackBlock(ptr, page, page + 16384, szSrc, szData != 65535, false)) {
                LOG_DEBUG("Ошибка при распаковке страницы %i в (%s)!", numPage, path);
                return false;
            }
            ptr += szSrc;
            sz -= szSrc;
        }
    } else {
        if(!unpackBlock(ptr, TMP_BUF, &TMP_BUF[49152], sz, (head1->STATE1 & 32) == 32, true)) {
            LOG_DEBUG("Ошибка при распаковке (%s)!", path);
            return false;
        }
        // перераспределяем буфер 0->5, 1->2, 2->0
        memcpy(&TMP_BUF[5 * 16384], &TMP_BUF[0 * 16384], 16384);
        memcpy(&TMP_BUF[0 * 16384], &TMP_BUF[2 * 16384], 16384);
        memcpy(&TMP_BUF[2 * 16384], &TMP_BUF[1 * 16384], 16384);
    }
    // меняем модель памяти и иинициализируем регистры
    auto isZ80 = strcasecmp(path + strlen(path) - 4, ".z80") == 0;
    changeModel(model, *_MODEL, isZ80);
    *cpu->_BC = head1->BC; *cpu->_DE = head1->DE; *cpu->_HL = head1->HL; *cpu->_AF = head1->AF;
    *cpu->_SP = head1->SP; *cpu->_IX = head1->IX; *cpu->_IY = head1->IY;
    *cpu->_I = head1->I; *cpu->_R = head1->R; *cpu->_IM = (uint8_t)(head1->STATE2 & 3);
    *cpu->_IFF1 = head1->IFF1; *cpu->_IFF2 = head1->IFF2;
    *cpu->_R |= (head1->STATE1 & 1) << 7;
    *_FE = (uint8_t)(224 | ((head1->STATE1 & 14) >> 1));
    memcpy(&opts[RC_], &head1->BC_, 8);
    *cpu->_PC = PC;
    if(head2) {
        writePort(0xfd, 0x7f, head2->hardState);
        memcpy(snd->_REGISTERS, head2->sndRegs, 16);
        writePort(0xfd, 0xff, head2->sndChipRegNumber);
    }
    if(head3 && length == 87) writePort(0xfd, 0x1f, head3->port1FFD);
    // копируем буфер
    memcpy(RAMs, TMP_BUF, 262144);
    return true;
}

bool zxALU::save(const char *path, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_WAVE:
            ret = tape->saveWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = tape->saveTAP(path);
            break;
        case ZX_CMD_IO_STATE:
            ret = saveState(path);
            break;
        case ZX_CMD_IO_Z80:
            ret = saveZ80(path);
            break;
    }
    return ret;
}

bool zxALU::saveZ80(const char *path) {
    static uint8_t models[] = { 0, 0, 0, 3, 9, 10, 3 };
    static HEAD3_Z80 head;

    auto head2 = &head.head2;
    auto head1 = &head2->head1;
    memset(&head, 0, sizeof(HEAD3_Z80));

    auto buf = TMP_BUF;
    // основные
    head1->BC = *cpu->_BC; head1->DE = *cpu->_DE; head1->HL = *cpu->_HL; head1->AF = *cpu->_AF;
    head1->SP = *cpu->_SP; head1->IX = *cpu->_IX; head1->IY = *cpu->_IY; head1->PC = 0;
    head1->STATE2 = *cpu->_IM; head1->IFF1 = *cpu->_IFF1; head1->IFF2 = *cpu->_IFF2;
    head1->I = *cpu->_I; head1->R = (uint8_t)(*cpu->_R & 127);
    head1->STATE1 = (uint8_t)((*cpu->_R & 128) >> 7) | (uint8_t)((*_FE & 7) << 1);
    memcpy(&head1->BC_, &opts[RC_], 8);
    // для режима 128К
    head2->PC = *cpu->_PC;
    head2->hardMode = models[*_MODEL];
    head2->length = 55;
    head.port1FFD = *_1FFD;
    head2->hardState = *_7FFD;
    head2->sndChipRegNumber = *snd->_CURRENT;
    memcpy(head2->sndRegs, snd->_REGISTERS, 16);
    ssh_memzero(head2->sndRegs, 16);
    // формируем буфер из содержимого страниц
    ssh_memcpy(&buf, &head, sizeof(HEAD3_Z80));
    // страницы, в зависимости от режима
    if(*_MODEL < MODEL_128) {
        packPage(&buf, PAGE_RAM[5], 8);
        packPage(&buf, PAGE_RAM[2], 4);
        packPage(&buf, PAGE_RAM[0], 5);
    } else {
        auto pages = banks[*_MODEL * 2];
        for(int i = 0; i < pages; i++) {
            packPage(&buf, PAGE_RAM[i], (uint8_t)(i + 3));
        }
    }
    return zxFile::writeFile(path, TMP_BUF, buf - TMP_BUF, true);
}

void zxALU::updateProps(int filter) {
    pageTRDOS = &ROMS[ZX_ROM_TRDOS];
    // скорость мерцания
    blinkShift = opts[ZX_PROP_BLINK_SPEED];
    blinkMsk = (1U << (blinkShift + 1)) - 1;
    // граница
    sizeBorder = (uint32_t)opts[ZX_PROP_BORDER_SIZE] * 8;

    auto params = &modelParams[*_MODEL * 4];
    auto turbo = opts[ZX_PROP_TURBO_MODE] ? 2 : 1;
    auto periodCPU = opts[ZX_PROP_CPU_SPEED];
    periodCPU *= turbo;

    stateUP = params[0] * periodCPU / 10;
    stateLP = params[1] * periodCPU / 10;
    stateRP = params[2] * periodCPU / 10;
    stateDP = params[3] * periodCPU / 10;

    gpu->updateProps(sizeBorder, filter);
    snd->updateProps(periodCPU);
    tape->updateProps();
    disk->updateProps();
}

static void execJoyKeys(int i, bool pressed) {
    int k           = opts[ZX_PROP_JOY_KEYS + i] * 4;
    auto semiRow    = (ZX_PROP_VALUES_SEMI_ROW + semiRows[k + 0]);
    auto semiRowEx  = (ZX_PROP_VALUES_SEMI_ROW + semiRows[k + 2]);
    auto bit        = semiRows[k + 1];
    auto bitEx      = semiRows[k + 3];
    if (bitEx) opts[semiRowEx] ^= (-pressed ^ opts[semiRowEx]) & bitEx;
    if (semiRow == ZX_PROP_VALUES_KEMPSTON) pressed = !pressed;
    opts[semiRow] ^= (-pressed ^ opts[semiRow]) & bit;
}

int zxALU::updateKeys(int key, int action) {
    if (key) {
        // клавиша была нажата/отпущена на экранной клавматуре
        auto idx = key * 4;
        auto semiRow = ZX_PROP_VALUES_SEMI_ROW + semiRows[idx + 0];
        auto semiRowEx = ZX_PROP_VALUES_SEMI_ROW + semiRows[idx + 2];
        auto bit = semiRows[idx + 1];
        auto bitEx = semiRows[idx + 3];
        auto mode = opts[ZX_PROP_KEY_MODE];
        if (!bitEx) {
            if (mode & ZX_CMD_KEY_MODE_CAPS) {
                bitEx = 1;
                semiRowEx = ZX_PROP_VALUES_SEMI_ROW;
            } else if (mode & ZX_CMD_KEY_MODE_SYMBOL) {
                bitEx = 2;
                semiRowEx = ZX_PROP_VALUES_SEMI_ROW + 7;
            }
        }
        if (action == 0) {
            if (key == 23) {
                mode &= ~ZX_CMD_KEY_MODE_SYMBOL;
                opts[ZX_PROP_VALUES_SEMI_ROW + 7] = 255;
                bitEx = 0;
                opts[ZX_PROP_KEY_MODE] = (uint8_t)(mode ^ ZX_CMD_KEY_MODE_CAPS);
            }
            if (key == 33) {
                mode &= ~ZX_CMD_KEY_MODE_CAPS;
                opts[ZX_PROP_VALUES_SEMI_ROW] = 255;
                bitEx = 0;
                opts[ZX_PROP_KEY_MODE] = (uint8_t)(mode ^ ZX_CMD_KEY_MODE_SYMBOL);
            }
            opts[semiRow] &= ~bit;
            opts[semiRowEx] &= ~bitEx;
        } else if (action == 1) {
            opts[semiRow] |= bit;
            opts[semiRowEx] |= bitEx;
        }
    } else {
        // опрос клавиатуры из ПЗУ
        if (opts[ZX_PROP_SHOW_KEY]) {
            static int delay = 0;
            delay++;
            if(!(delay & 7)) {
                // проверить режим клавиатуры
                uint8_t nmode = MODE_K;
                auto kmode = opts[ZX_PROP_KEY_MODE];
                auto omode = opts[ZX_PROP_KEY_CURSOR_MODE];
                auto val0 = rm8(23617), val1 = rm8(23658), val2 = rm8(23611);
                switch (val0) {
                    case 0:
                        // если старый режим был [SE/CE], то сбрасывать CAPS/SYMBOL
                        if (val2 & 8) { nmode = (val1 & 8) ? MODE_C : MODE_L; }
                        else if (val1 & 16) nmode = MODE_K;
                        if (omode == MODE_CE) {
                            opts[ZX_PROP_VALUES_SEMI_ROW] |= 1;
                            kmode &= ~ZX_CMD_KEY_MODE_CAPS;
                            opts[ZX_PROP_KEY_MODE] = kmode;
                        }
                        else if (omode == MODE_SE) {
                            opts[ZX_PROP_VALUES_SEMI_ROW + 7] |= 2;
                            kmode &= ~ZX_CMD_KEY_MODE_SYMBOL;
                            opts[ZX_PROP_KEY_MODE] = kmode;
                        }
                        if ((nmode == MODE_L || nmode == MODE_C) && (kmode & ZX_CMD_KEY_MODE_CAPS)) nmode = MODE_CL;
                        else { if (kmode & ZX_CMD_KEY_MODE_SYMBOL) nmode = MODE_SK; else if (kmode & ZX_CMD_KEY_MODE_CAPS) nmode = MODE_CK; }
                        break;
                    case 1:
                        nmode = (kmode & ZX_CMD_KEY_MODE_SYMBOL) ? MODE_SE : ((kmode & ZX_CMD_KEY_MODE_CAPS) ? MODE_CE : MODE_E);
                        break;
                    case 2:
                        nmode = (kmode & ZX_CMD_KEY_MODE_CAPS) ? MODE_G : MODE_G1;
                        break;
                }
                if (opts[ZX_PROP_KEY_CURSOR_MODE] != nmode) {
                    opts[ZX_PROP_KEY_CURSOR_MODE] = nmode;
                    memcpy(&opts[ZX_PROP_VALUES_BUTTON], &buttons[nmode * 84], 84);
                    return 1;
                }
            }
        }
        // joystick
        if (opts[ZX_PROP_SHOW_JOY]) {
            auto buttons = (opts[ZX_PROP_JOY_ACTION_VALUE] << 4) | opts[ZX_PROP_JOY_CROSS_VALUE];
            if (buttons != joyOldButtons) {
                joyOldButtons = buttons;
                // 1. отжать
                for (int i = 0; i < 8; i++) {
                    if(!(buttons & numBits[i])) execJoyKeys(i, true);
                }
                // 2. нажать
                for (int i = 0; i < 8; i++) {
                    if(buttons & numBits[i]) execJoyKeys(i, false);
                }
            }
        }
    }
    return 0;
}

bool zxALU::changeModel(uint8_t _new, uint8_t _old, bool resetTape) {
    auto result = _new != _old;
    if(result) {
        *_MODEL = _new;
        opts[ZX_PROP_MODEL_TYPE] = _new;
        auto rom = &ROMS[pages_rom[_new]];
        auto pagesROM =  banks[_new * 2 + 1];
        ssh_memzero(PAGE_ROM, sizeof(PAGE_ROM));
        for (int i = 0; i < pagesROM; i++) PAGE_ROM[i] = (rom + i * 16384);
    }
    uint8_t src[] = {
            LD_A_N, 1,
            LD_BC_NN, 255, 255,
            PUSH_BC,
            POP_AF,
            CCF,
            INC_A,
            LD_DE_NN, 3, 4,
            LD_HL_NN, 5, 6,
            IN_A_N, 2,
            PREF_ED, IN_A_BC,
            PREF_ED, IN_B_BC,
            PREF_ED, IN_C_BC,
            PREF_ED, IN_D_BC,
            PREF_ED, IN_E_BC,
            PREF_ED, IN_H_BC,
            PREF_ED, IN_L_BC,
            PREF_DD, LD_HL_NN, 0, 0,
            PREF_DD, LD_H_PHL, 0,
            PREF_DD, LD_L_PHL, 0,
            JR_N, 2,
            PREF_DD, EXX,
            PREF_DD, EX_PSP_HL,
            PREF_DD, EX_PSP_HL,
            PREF_DD, EXX,
            PREF_DD, INC_H,
            PREF_DD, INC_L,
            PREF_DD, ADD_A_H,
            PREF_DD, ADD_A_L,
            PREF_DD, ADD_A_D,
            PREF_DD, ADD_A_E,
            PREF_DD, LD_D_H,
            PREF_DD, LD_E_L,
            PREF_DD, DEC_HL,
            PREF_DD, LD_C_L,
            PREF_DD, LD_B_H,
            PREF_DD, LD_L_H,
            LD_HL_NN, 0, 64,
            LD_BC_NN, 0, 27,
            PUSH_HL,
            PUSH_BC,
            INC_PHL,
            INC_HL,
            DEC_BC,
            LD_A_C,
            OR_B,
            JR_NZ, 249,
            POP_BC,
            POP_HL,
            JP_NN, 6, 0
                     };
    auto dst = &PAGE_ROM[*_ROM][0];
    //memcpy(dst, src, sizeof(src));
    signalRESET(resetTape);
    return result;
}

void zxALU::signalRESET(bool resetTape) {
    // сброс устройств
    if(resetTape) tape->reset();
    snd->reset();
    disk->reset();
    // очищаем ОЗУ
    ssh_memzero(RAMs, ZX_TOTAL_RAM);
    // очищаем регистры/порты
    ssh_memzero(opts, STATE);
    // сбрасываем клавиатуру
    ssh_memset(&opts[ZX_PROP_VALUES_SEMI_ROW], 255, 8);
    opts[ZX_PROP_KEY_MODE] = 0; opts[ZX_PROP_KEY_CURSOR_MODE] = 255;
    // сбрасываем джойстики
    opts[ZX_PROP_JOY_ACTION_VALUE] = 0; opts[ZX_PROP_JOY_CROSS_VALUE] = 0;
    joyOldButtons = 0;
    // инициализаруем переменные
    *cpu->_SP = 65534; *_FE = 0b11100111; *_VID = 5;
    // сброс состояния с сохранением статуса отладчика
    modifySTATE(0, ~ZX_DEBUG);
    setPages();
    programName("BASIC");
}

void zxALU::writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val) {
    if(*_STATE & ZX_DEBUG) { if(checkBPs((A0A7 | (A8A15 << 8)), ZX_BP_WPORT)) return; }
    if(*_STATE & ZX_TRDOS) {
        disk->writePort(A0A7, val);
    } else {
        switch(A0A7) {
            case 0xFE:
                // 0, 1, 2 - бордер, 3 MIC - при записи, 4 - бипер
                *_FE = val;
                colorBorder = val & 7U;
                tape->writePort(val);
                break;
            case 0x1F:
                *_KEMPSTON = val;
                break;
            default:
                switch(A8A15) {
                    case 0x1F:
                        if(*_MODEL == MODEL_SCORPION) {
                            // 1FFD
                            // 0 -> 1 - RAM0(!!), 0 - see bit 1 etc
                            // 1 -> 1 - ROM 2, 0 - ROM from 0x7FFD
                            // 4 -> 1 - RAM SCORPION, 0 - from 0x7FFD
                            // A0, A2, A5, A12 = 1; A1, A14, A15 = 0
                            *_1FFD = val;
                            if (val & 1) *_ROM = 100;
                            else {
                                if (val & 2) *_ROM = 2;
                                else *_ROM = (uint8_t) ((*_7FFD & 16) >> 4);
                            }
                            *_RAM = (uint8_t) ((*_7FFD & 7) + (uint8_t) ((val & 16) >> 1));
                            LOG_DEBUG("(%X%X(%i) ROM: %i RAM: %i VID: %i)", A8A15, A0A7, val, *_ROM, *_RAM, *_VID);
                            setPages();
                        }
                        break;
                    case 0x7F:
                        // 7FFD
                        // 1, 2, 4 - страница 0-7
                        // 8 - экран 5/7
                        // 16 - ПЗУ 0 - 128К 1 - 48К
                        // 32 - блокировка
                        // A0, A2, A5, A12, A14 = 1; A1, A15 = 0
                        if (*_MODEL < MODEL_128) return;
                        if(*_7FFD & 32) return;
                        *_7FFD = val;
                        *_VID = (uint8_t)((val & 8) ? 7 : 5);
                        *_RAM = (uint8_t)(val & 7);
                        if(*_MODEL == MODEL_SCORPION) {
                            if(!(*_1FFD & 2)) *_ROM = (uint8_t)((val & 16) >> 4);
                            *_RAM += (uint8_t)((*_1FFD & 16) >> 1);
                            LOG_DEBUG("(%X%X(%i) ROM: %i RAM: %i VID: %i)", A8A15, A0A7, val, *_ROM, *_RAM, *_VID);
                        } else {
                            *_ROM = (uint8_t)((val & 16) >> 4);
                        }
                        setPages();
                        break;
                    case 0xBF:
                        // записываем значение в текущий регистр
                        snd->write(*snd->_CURRENT, val);
                        //LOG_DEBUG("AY VAL (%X%X(%i)", A8A15, A0A7, val);
                        break;
                    case 0xFF:
                        // устанавливаем текущий регистр
                        *snd->_CURRENT = (uint8_t)(val & 15);
                        //LOG_DEBUG("AY REG (%X%X(%i)", A8A15, A0A7, val);
                        break;
                }
        }
    }
}

uint8_t zxALU::readPort(uint8_t A0A7, uint8_t A8A15) {
    if(*_STATE & ZX_DEBUG) { if(checkBPs((A0A7 | (A8A15 << 8)), ZX_BP_RPORT)) return _FF; }
    if(*_STATE & ZX_TRDOS) return disk->readPort(A0A7);
    switch(A0A7) {
        // звук AY
        case 0xFD:
            if(A8A15 != 0xff) break;
            return snd->_REGISTERS[*snd->_CURRENT];
        // джойстик
        case 0x1F:
            return *_KEMPSTON;
        case 0xFC:
            return _FF;
        // клавиатура | MIC
        case 0xFE:
            // 0,1,2,3,4 - клавиши полуряда, 6 - EAR, 5,7 - не используется
            A0A7 = 31;
            for(int i = 0; i < 8; i++) {
                if(A8A15 & (1 << i)) continue;
                A0A7 &= opts[i + ZX_PROP_VALUES_SEMI_ROW];
            }
            return A0A7 | tape->readPort();
    }
    return _FF;
}

void zxALU::setPages() {
    pageROM = *_STATE & ZX_TRDOS ? pageTRDOS : (*_ROM == 100 ? PAGE_RAM[0] : PAGE_ROM[*_ROM]);
    pageRAM = PAGE_RAM[*_RAM];
    pageVRAM = PAGE_RAM[*_VID];
    pageATTRIB = pageVRAM + 6144;
}

void zxALU::execute() {
    // отображение экрана, воспроизведение звука
    updateFrame();
    //snd->update();
    gpu->updateFrame();
}

void zxALU::stepDebug() {
    // убрать отладчик - чтобы не сработала точка останова
    modifySTATE(0, ZX_DEBUG | ZX_HALT);
    // перехват системных процедур
    auto ticks = trap();
    ticks += cpu->step();
    *_TICK += ticks;
    modifySTATE(ZX_DEBUG, 0);
}

int zxALU::step(bool allow_int) {
    if(!opts[ZX_PROP_EXECUTE]) return 0;
    // проверить на точку останова для кода
    PC = *cpu->_PC;
    if(*_STATE & ZX_DEBUG) { if(checkBPs(PC, ZX_BP_EXEC)) return 0; }
    // установить возможность прерывания
    modifySTATE(allow_int * ZX_INT, 0);
    // перехват системных процедур
    auto ticks = trap();
    // выполнение операции
    ticks += cpu->step();
    // проверка на прерывания
    if(*_STATE & ZX_NMI) ticks += cpu->signalNMI();
    else if(*_STATE & ZX_INT) ticks += cpu->signalINT();
    *_TICK += ticks;
    return ticks;
}

void zxALU::updateCPU(int todo, bool interrupt) {
    if(*_STATE & (ZX_BP | ZX_PAUSE)) return;
    if(!opts[ZX_PROP_EXECUTE]) return;
    int ticks;
    todo += periodGPU;
    if (interrupt) {
        if (periodGPU > 0) {
            ticks = step(false);
            todo -= ticks;
            tape->control(ticks);
        }
        ticks = step(true);
        todo -= ticks;
        tape->control(ticks);
    }
    while (todo > 3) {
        ticks = step(false);
        // если сработала точка останова - возвращает 0
        if(!ticks) break;
        todo -= ticks;
        tape->control(ticks);
    }
    periodGPU = todo;
}

void zxALU::updateFrame() {
    uint32_t line, i, tmp, c;

    if(pauseBetweenTapeBlocks > 0) {
        pauseBetweenTapeBlocks--;
        if(pauseBetweenTapeBlocks == 0)
            modifySTATE(0, ZX_PAUSE);
    }

    auto dest = gpu->frameBuffer;
    if(!dest) return;

    auto isBlink = ((blink & blinkMsk) >> blinkShift) == 0;
    auto szBorder = sizeBorder >> 1;
    auto colours = (uint32_t*)&opts[ZX_PROP_COLORS];

//    _FF = 0xff;
    colorBorder = (uint8_t)(*_FE & 7);

    updateCPU(stateUP, true);
    // верхняя часть бордера
    for (line = 0; line < sizeBorder; line++) {
        updateCPU(stateLP, false);
        for (i = 0; i < (128 + (szBorder << 1)); i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c;
            *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    // экран по центру и бордер слева и справа
    for (line = 0; line < 192; line++) {
        updateCPU(stateLP, false);
        for (i = 0; i < szBorder; i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c;
            *dest++ = c;
        }
        auto rb = (((line & 192) << 5) + ((line & 7) << 8) + ((line & 56) << 2));
        for (int ri = 0; ri < 32; ri++) {
            _FF = pageATTRIB[ri + ((line >> 3) << 5)];
            frames[0] = colours[(_FF & 120) >> 3];
            frames[1] = colours[(_FF & 7) | ((_FF & 64) >> 3)];
            if ((_FF & 128) && isBlink) {
                tmp = frames[0];
                frames[0] = frames[1];
                frames[1] = tmp;
            }
            auto v = pageVRAM[rb];
            *dest++ = frames[(v >> 7) & 1];
            *dest++ = frames[(v >> 6) & 1];
            updateCPU(1, false);
            *dest++ = frames[(v >> 5) & 1];
            *dest++ = frames[(v >> 4) & 1];
            updateCPU(1, false);
            *dest++ = frames[(v >> 3) & 1];
            *dest++ = frames[(v >> 2) & 1];
            updateCPU(1, false);
            *dest++ = frames[(v >> 1) & 1];
            *dest++ = frames[v & 1];
            updateCPU(1, false);
            rb++;
        }
        for (i = 0; i < szBorder; i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c;
            *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    // нижняя часть бордера
    for (line = 0; line < sizeBorder; line++) {
        updateCPU(stateLP, false);
        for (i = 0; i < (128 + (szBorder << 1)); i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c;
            *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    updateCPU(stateDP, false);
    blink++;
}

void zxALU::tracer(int start) {
    ftracer.close();
    opts[ZX_PROP_LAUNCH_TRACCER] = (uint8_t)start;
    if(start != 0) {
        auto nm = name + ".log";
        ftracer.open(zxFile::makePath(nm.c_str(), false).c_str(), zxFile::create_write);
    }
}

void zxALU::quickBP(uint16_t address) {
    BREAK_POINT* bpe = nullptr;
    for(int i = 0; i < 8; i++) {
        auto bp = &bps[i];
        auto flag = bp->flg;
        if(flag == ZX_BP_NONE) {
            if(bpe == nullptr) bpe = bp;
        } else if(address >= bp->address1 && address <= bp->address2 && flag == ZX_BP_EXEC) {
            bpe = nullptr;
            bp->flg = ZX_BP_NONE;
            break;
        }
    }
    if(bpe != nullptr) {
        bpe->address1 = address; bpe->address2 = address;
        bpe->val = 0; bpe->msk = 0; bpe->ops = 0; bpe->flg = ZX_BP_EXEC;
    }
}

BREAK_POINT* zxALU::quickCheckBPs(uint16_t address, uint8_t flg) {
    BREAK_POINT* bp;
    for(int i = 0 ; i < 8 ; i++) {
        bp = &bps[i];
        if (bp->flg == flg) {
            if (address >= bp->address1 && address <= bp->address2)
                return bp;
        }
    }
    return nullptr;
}

bool zxALU::checkBPs(uint16_t address, uint8_t flg) {
    auto bp = quickCheckBPs(address, flg);
    if(!bp) return false;
    bool res = true;
    if(bp->flg == ZX_BP_WMEM) {
        auto v1 = rm8(address) & bp->msk;
        auto v2 = bp->val;
        res = false;
        switch(bp->ops) {
            case ZX_BP_OPS_EQ:  res = v1 == v2; break;
            case ZX_BP_OPS_NQ:  res = v1 != v2; break;
            case ZX_BP_OPS_GT:  res = v1 >  v2; break;
            case ZX_BP_OPS_LS:  res = v1 <  v2; break;
            case ZX_BP_OPS_GTE: res = v1 >= v2; break;
            case ZX_BP_OPS_LSE: res = v1 <= v2; break;
        }
    }
    if(res) {
        LOG_DEBUG("addr1: %i addr2: %i flag1: %i flag2: %i", bp->address1, bp->address2, bp->flg, flg);
        // установить глобальную остановку системы
        opts[ZX_PROP_EXECUTE] = 0;
        // сообщение для выхода - обновить отладчик
        modifySTATE(ZX_BP, 0);
        // вернуть на начало инструкции (в случае доступа к портам или памяти)
        *cpu->_PC = PC;
    }
    return res;
}

const char *zxALU::programName(const char *nm) {
    if(nm && strlen(nm) > 0) {
        static char tmp_nm[64];
        // оставить только имя
        auto s = strrchr(nm, '/');
        auto e = strrchr(nm, '.');
        if(!e) e = (char*)(nm + strlen(nm));
        if(s) s++; else s = (char*)nm;
        memcpy(tmp_nm, s, e - s);
        tmp_nm[e - s] = 0;
        name = tmp_nm;
    }
    return name.c_str();
}

int zxALU::trap() {
    if(PC < 16384) {
        // активность TR DOS
        if(PC == 15616 || PC == 15619) {
            LOG_INFO("Перехват TRDOS addr: %i", PC);
            if(pageROM != PAGE_ROM[0]) {
                *_STATE |= ZX_TRDOS;
                setPages();
            }
            return 4;
        }
        bool success = false;
        if (PC == 1218) success = tape->trapSave();
        else if (PC == 1366 || PC == 1388) success = tape->trapLoad();
        if(success) {
            auto psp = cpu->_SP;
            *cpu->_PC = rm16(*psp);
            *psp += 2;
            return 4;
        }
    } else {
        if(*_STATE & ZX_TRDOS) {
            *_STATE &= ~ZX_TRDOS;
            setPages();
        }
    }
    return 0;
}

#pragma clang diagnostic pop
