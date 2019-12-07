//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxALU.h"
#include "zxMnemonic.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

// текущий счетчик инструкций
uint32_t* zxALU::_TICK(nullptr);

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

static uint32_t banks[] = {8, 1, 8, 1, 8, 2, 8, 2, 16, 4};

static int pages_rom[5] = {
        ZX_ROM_K48K,
        ZX_ROM_48K,
        ZX_ROM_128K,
        ZX_ROM_PENTAGON,
        ZX_ROM_SCORPION
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
        3, 0x10, 0, 0, 4, 0x10, 0, 0, 4, 0x08, 0, 0, 4, 0x04, 0, 0, 4, 0x02, 0, 0,
        4, 0x01, 0, 0, 4, 0x01, 0, 1, 2, 0x01, 0, 0, 2, 0x02, 0, 0, 2, 0x04, 0, 0,
        2, 0x08, 0, 0, 2, 0x10, 0, 0, 5, 0x10, 0, 0, 5, 0x08, 0, 0, 5, 0x04, 0, 0,
        5, 0x02, 0, 0, 5, 0x01, 0, 0, 6, 0x01, 0, 0, 0, 0x01, 0, 0, 1, 0x01, 0, 0,
        1, 0x02, 0, 0, 1, 0x04, 0, 0, 1, 0x08, 0, 0, 1, 0x10, 0, 0, 6, 0x10, 0, 0,
        6, 0x08, 0, 0, 6, 0x04, 0, 0, 6, 0x02, 0, 0, 7, 0x02, 0, 0, 0, 0x01, 7, 2,
        0, 0x02, 0, 0, 0, 0x04, 0, 0, 0, 0x08, 0, 0, 7, 0x01, 0, 0, 0, 0x10, 0, 0,
        7, 0x10, 0, 0, 7, 0x08, 0, 0, 7, 0x04, 0, 0,
        4, 0x08, 0, 1, 4, 0x10, 0, 1, 3, 0x10, 0, 1, 4, 0x04, 0, 1,
        8, 0x02, 0, 0, 8, 0x01, 0, 0, 8, 0x08, 0, 0, 8, 0x04, 0, 0, 8, 0x10, 0, 0
};

static void packPage(uint8_t** buffer, uint8_t* src, uint8_t page) {
    uint32_t size;
    uint8_t* buf = *buffer;
    *buffer = packBlock(src, src + 16384, &buf[3], false, size);
    *(uint16_t*)buf = (uint16_t)size;
    buf[2] = page;
}

zxALU::zxALU() : joyOldButtons(0), periodGPU(0), _FF(255), colorBorder(7), surface(nullptr), cpu(nullptr) {
    blink = blinkMsk = blinkShift = 0;
    sizeBorder = 0;
    pageTRDOS = &ROMS[ZX_ROM_TRDOS];

    RAMs = new uint8_t[ZX_TOTAL_RAM];

    for (int i = 0; i < 16; i++) PAGE_RAM[i] = (RAMs + i * 16384);

    // инициализация системы
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE];
    _KEMPSTON		= &opts[ZX_PROP_VALUES_KEMPSTON];
    _1FFD    		= &opts[PORT_1F];
    _7FFD    		= &opts[PORT_FD];
    _FE             = &opts[PORT_FE];

    //zxALU::_TRDOS   = &opts[TRDOS0];
    zxALU::_RAM     = &opts[RAM];
    zxALU::_VID     = &opts[VID];
    zxALU::_ROM     = &opts[ROM];
    zxALU::_TICK    = (uint32_t*)&opts[TICK0];

    cpu = new zxCPU();
}

zxALU::~zxALU() {
    SAFE_A_DELETE(RAMs);
//    SAFE_DELETE(tape);
//    SAFE_DELETE(snd);
    SAFE_DELETE(cpu);
}

bool zxALU::load(const char *name, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_TRD:
            // ret = disk->open(name);
            break;
        case ZX_CMD_IO_SCREEN:
            // ret = openScreen(name);
            break;
        case ZX_CMD_IO_WAVE:
            // ret = tape->openWAV(name);
            break;
        case ZX_CMD_IO_TAPE:
            // ret = tape->openTAP(name);
            // if(ret) ret = openZ80("tapLoader.zx");
            break;
        case ZX_CMD_IO_STATE:
            ret = openState(name);
            break;
        case ZX_CMD_IO_Z80:
            ret = openZ80(name);
            break;
    }
    return ret;
}

bool zxALU::openState(const char *name) {
    auto ptr = (uint8_t*)zxFile::readFile(name, &TMP_BUF[262144], false);
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
            debug("Ошибка при распаковке страницы %i состояния эмулятора!!!", i);
            signalRESET(true);
            return false;
        }
        ptr += size;
    }
    // восстанавливаем состояние ленты
    // tape->loadState(ptr);
    // восстанавливаем страницы
    setPages();
    // грузим параметры джойстика
    presets((const char*)ptr, ZX_CMD_PRESETS_SET);
    return true;
}

bool zxALU::saveState(const char* name) {
    uint32_t size;
    auto buf = TMP_BUF;
    // сохраняем регистры
    ssh_memcpy(&buf, opts, COUNT_REGS);
    // количество банков и их содержимое
    auto pages =  banks[*_MODEL * 2];
    for(int i = 0; i < pages; i++) {
        packBlock(PAGE_RAM[i], PAGE_RAM[i] + 16384, &buf[2], false, size);
        *(uint16_t*)buf = (uint16_t)size; buf += size + sizeof(uint16_t);
    }
    // сохраняем состояние ленты
    // buf = tape->saveState(buf);
    // сохраняем имя программы
    ssh_memcpy(&buf, progName, sizeof(progName));
    return zxFile::writeFile(name, TMP_BUF, buf - TMP_BUF, false);
}

bool zxALU::openZ80(const char *name) {
    size_t sz;
    HEAD1_Z80* head1 = nullptr;
    HEAD2_Z80* head2 = nullptr;
    HEAD3_Z80* head3 = nullptr;
    uint8_t model = MODEL_48K;

    auto ptr = (uint8_t*)zxFile::readFile(name, &TMP_BUF[262144], true, &sz);
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
            case 0: case 1: if(!isMod) model = MODEL_48K; break;
            case 3: case 4: case 5: case 6: if(!isMod) model = MODEL_128K; break;
            case 9: model = MODEL_PENTAGON; break;
            case 10: model = MODEL_SCORPION; break;
            default: debug("Неизвестное оборудование в %s!", name); return false;
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
                case MODEL_48K:
                case MODEL_48KK:
                    isPage = true;
                    switch(numPage) {
                        case 1: numPage = 2; break;
                        case 2: numPage = 0; break;
                        case 5: numPage = 5; break;
                        default: isPage = false;
                    }
                    break;
                case MODEL_128K:
                case MODEL_PENTAGON:
                    isPage = (numPage >= 0 && numPage <= 7);
                    break;
                case MODEL_SCORPION:
                    isPage = (numPage >= 0 && numPage <= 15);
                    break;
            }
            if(!isPage) {
                debug("Неизвестная страница %i в openZ80(%s)!", numPage, name);
                return false;
            }
            auto page = &TMP_BUF[numPage * 16384];
            if(!unpackBlock(ptr, page, page + 16384, szSrc, szData != 65535, false)) {
                debug("Ошибка при распаковке страницы %i в openZ80(%s)!", numPage, name);
                return false;
            }
            ptr += szSrc;
            sz -= szSrc;
        }
    } else {
        if(!unpackBlock(ptr, TMP_BUF, &TMP_BUF[49152], sz, (head1->STATE1 & 32) == 32, true)) {
            debug("Ошибка при распаковке openZ80(%s)!", name);
            return false;
        }
        // перераспределяем буфер 0->5, 1->2, 2->0
        memcpy(&TMP_BUF[5 * 16384], &TMP_BUF[0 * 16384], 16384);
        memcpy(&TMP_BUF[0 * 16384], &TMP_BUF[2 * 16384], 16384);
        memcpy(&TMP_BUF[2 * 16384], &TMP_BUF[1 * 16384], 16384);
    }
    // меняем модель памяти и иинициализируем регистры
    auto isZ80 = strcasecmp(name + strlen(name) - 4, ".z80") == 0;
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
        //ssh_memcpy(zxSND::_AY, head2->sndRegs, 16);
        writePort(0xfd, 0xff, head2->sndChipRegNumber);
    }
    if(head3 && length == 87) writePort(0xfd, 0x1f, head3->port1FFD);
    // копируем буфер
    memcpy(RAMs, TMP_BUF, 262144);
    // загружаем параметры джойстика
    if(isZ80) presets(name, ZX_CMD_PRESETS_SET);
    return true;
}

bool zxALU::save(const char *name, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_WAVE:
            // ret = tape->saveWAV(name);
            break;
        case ZX_CMD_IO_TAPE:
            // ret = tape->saveTAP(name);
            break;
        case ZX_CMD_IO_SCREEN:
            // ret = saveScreen(name);
            break;
        case ZX_CMD_IO_STATE:
            ret = saveState(name);
            break;
        case ZX_CMD_IO_Z80:
            ret = saveZ80(name);
            break;
    }
    return ret;
}

bool zxALU::saveZ80(const char *name) {
    static uint8_t models[] = { 0, 0, 3, 9, 10 };
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
    head2->sndChipRegNumber = 0;//*snd->_AY_REG;
//    memcpy(head2->sndRegs, zxSND::_AY, 16);
    ssh_memzero(head2->sndRegs, 16);
    // формируем буфер из содержимого страниц
    ssh_memcpy(&buf, &head, sizeof(HEAD3_Z80));
    // страницы, в зависимости от режима
    if(*_MODEL < MODEL_128K) {
        packPage(&buf, PAGE_RAM[5], 8);
        packPage(&buf, PAGE_RAM[2], 4);
        packPage(&buf, PAGE_RAM[0], 5);
    } else {
        auto pages = banks[*_MODEL * 2];
        for(int i = 0; i < pages; i++) {
            packPage(&buf, PAGE_RAM[i], (uint8_t)(i + 3));
        }
    }
    return zxFile::writeFile(name, TMP_BUF, buf - TMP_BUF, true);
}

static bool presetsOps(PRESET* ptr, int ops, const char* name, char** l) {
    auto lst = *l;
    auto isFind = strcasecmp(ptr->name, name) == 0;
    switch(ops) {
        // Получить список пресетов
        case ZX_CMD_PRESETS_LIST:
            if(*(lst - 1)) *lst++ = ',';
            *l = ssh_strcpy(&lst, ptr->name);
            break;
            // Загрузить параметры джойстика
        case ZX_CMD_PRESETS_LOAD:
        case ZX_CMD_PRESETS_SET:
            if(isFind) {
                opts[ZX_PROP_JOY_TYPE] = ptr->joyType;
                memcpy(&opts[ZX_PROP_JOY_KEYS], &ptr->joyL, 8);
                return true;
            }
            break;
            // Сохранить параметры джойстика
        case ZX_CMD_PRESETS_SAVE:
            return isFind;
    }
    return false;
}

const char* zxALU::presets(const char *name, int ops) {
    if(ops == ZX_CMD_PRESETS_NAME) return progName;
    auto tmp = &TMP_BUF[524288 - 80000];
    ssh_memzero(tmp, 80000);
    zxFile::readFile("joy_presets", tmp, false, nullptr);
    char* buf = (char*)(tmp + 40000);
    auto lst = buf;
    auto ptr = (PRESET*)tmp;
    // в имени убрать расширение
    if(ops == ZX_CMD_PRESETS_SET) {
        if(name) {
            auto n = strstr(name, ".");
            if(n) {
                memcpy(&buf[100], name, n - name);
                name = &buf[100];
            }
            auto lenName = strlen(name);
            ssh_memzero(&progName, sizeof(progName));
            memcpy(&progName, name, lenName > 30 ? 30 : lenName);
        }
    }
    // ищем имя в БД
    while(ptr->name[0]) {
        if(presetsOps(ptr, ops, name, &lst)) break;
        ptr += sizeof(PRESET);
    }
    // Сохранить/Добавить параметры джойстика
    if(ops == ZX_CMD_PRESETS_SAVE) {
        ssh_memzero(&ptr->name, 31);
        memcpy(&ptr->name, progName, sizeof(progName));
        ptr->joyType = opts[ZX_PROP_JOY_TYPE];
        memcpy(&ptr->joyL, &opts[ZX_PROP_JOY_KEYS], 8);
        zxFile::writeFile("joy_presets", tmp, 40000, false);
    }
    return buf;
}

void zxALU::updateProps() {
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

//    snd->updateProps(periodCPU);
//    tape->updateProps();
}

int zxALU::updateKeys(int key, int action) {
    if (key) {
        // клавиша была нажата/отпущена на экранной клавматурк
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
            // проверить режим клавиатуры
            uint8_t nmode = MODE_K;
            auto kmode = opts[ZX_PROP_KEY_MODE];
            auto val0 = rm8(23617), val1 = rm8(23658), val2 = rm8(23611);
//            info("%i %i %i", val0, val1, val2);
            switch (val0) {
                case 0:
                    if (val2 & 8) { nmode = (val1 & 8) ? MODE_C : MODE_L; }
                    else if (val1 & 16) nmode = MODE_K;
                    if ((nmode == MODE_L || nmode == MODE_C) && (kmode & ZX_CMD_KEY_MODE_CAPS)) nmode = MODE_CL;
                    else { if (kmode & ZX_CMD_KEY_MODE_SYMBOL) nmode = MODE_E1; else if (kmode & ZX_CMD_KEY_MODE_CAPS) nmode = MODE_CK; }
                    break;
                case 1:
                    nmode = (kmode & ZX_CMD_KEY_MODE_SYMBOL) ? MODE_E2 : MODE_E;
                    break;
                case 2:
                    nmode = (kmode & ZX_CMD_KEY_MODE_CAPS) ? MODE_G : MODE_G1;
                    break;
            }
            if (opts[ZX_PROP_KEY_CURSOR_MODE] != nmode) {
                opts[ZX_PROP_KEY_CURSOR_MODE] = nmode;
                return 1;
            }
        }
        // joystick
        if (opts[ZX_PROP_SHOW_JOY]) {
            auto buttons = (opts[ZX_PROP_JOY_ACTION_VALUE] << 4) | opts[ZX_PROP_JOY_CROSS_VALUE];
            if (buttons != joyOldButtons) {
                uint8_t o;
                joyOldButtons = buttons;
                for (int i = 0; i < 8; i++) {
                    int pressed     = buttons & (1 << i);
                    int k           = opts[ZX_PROP_JOY_KEYS + i] * 4;
                    auto semiRow    = (ZX_PROP_VALUES_SEMI_ROW + semiRows[k + 0]);
                    auto semiRowEx  = (ZX_PROP_VALUES_SEMI_ROW + semiRows[k + 2]);
                    auto bit        = semiRows[k + 1];
                    auto bitEx      = semiRows[k + 3];
                    if (bitEx) {
                        o = opts[semiRowEx];
                        opts[semiRowEx] = pressed ? (o & ~bitEx) : (o | bitEx);
                    }
                    if (semiRow == ZX_PROP_VALUES_KEMPSTON) pressed = !pressed;
                    o = opts[semiRow];
                    opts[semiRow] = pressed ? (o & ~bit) : (o | bit);
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
    signalRESET(resetTape);
    return result;
}

void zxALU::signalRESET(bool resetTape) {
    // сброс устройств
//    if(resetTape) tape->reset();
//    snd->reset();
//    disk->reset();
    // очищаем ОЗУ
    ssh_memzero(RAMs, ZX_TOTAL_RAM);
    // очищаем регистры/порты
    ssh_memzero(opts, MODEL);
    // сбрасываем клавиатуру
    ssh_memset(&opts[ZX_PROP_VALUES_SEMI_ROW], 255, 8);
    opts[ZX_PROP_KEY_MODE] = 0; opts[ZX_PROP_KEY_CURSOR_MODE] = MODE_K;
    // сбрасываем джойстики
    opts[ZX_PROP_JOY_ACTION_VALUE] = 0; opts[ZX_PROP_JOY_CROSS_VALUE] = 0;
    joyOldButtons = 0;
    // инициализаруем переменные
    *cpu->_SP = 65534; *_FE = 0b11100111; *_VID = 5;
    // устанавливаем программу по умолчанию
    presets("BASIC", ZX_CMD_PRESETS_SET);
    setPages();
}

void zxALU::writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val) {
    if(*_STATE & ZX_TRDOS) {
/*
        switch(A0A7) {
            case 0x1F: TRDOS_CMD = val; break;
            case 0x3F: TRDOS_TRK = val; break;
            case 0x5F: TRDOS_SEC = val; break;
            case 0x7F: TRDOS_DAT = val; */
/* disk->write(val) *//*
;break;
            case 0xFF: TRDOS_SYS = val; break;
        }
*/
        // TR-DOS
    } else if((A0A7 & 3) < 2) {
        if(A8A15 == 0x1F) {
            if(*_MODEL == MODEL_SCORPION) {
                // 1FFD
                // 0 -> 1 - RAM0(!!), 0 - see bit 1 etc
                // 1 -> 1 - ROM 2, 0 - ROM from 0x7FFD
                // 4 -> 1 - RAM SCORPION, 0 - from 0x7FFD
                // A0, A2, A5, A12 = 1; A1, A14, A15 = 0
                debug("writePort %X-%X=%i", A8A15, A0A7, val);
                *_1FFD = val;
                if (val & 1) *_ROM = 100;
                else {
                    if (val & 2) *_ROM = 2;
                    else *_ROM = (uint8_t) ((*_7FFD & 16) >> 4);
                }
                *_RAM = (uint8_t) ((*_7FFD & 7) + (uint8_t) ((val & 16) >> 1));
                setPages();
            }
        } else if(A8A15 < 0x80) {
            // 7FFD
            // 1, 2, 4 - страница 0-7
            // 8 - экран 5/7
            // 16 - ПЗУ 0 - 128К 1 - 48К
            // 32 - блокировка
            // A0, A2, A5, A12, A14 = 1; A1, A15 = 0
            if (*_MODEL < MODEL_128K) return;
            if(*_7FFD & 32) return;
            *_7FFD = val;
            *_VID = (uint8_t)((val & 8) ? 7 : 5);
            *_RAM = (uint8_t)(val & 7);
            if(*_MODEL == MODEL_SCORPION) {
                debug("writePort %X-%X=%i", A8A15, A0A7, val);
                if(!(*_1FFD & 2)) *_ROM = (uint8_t)((val & 16) >> 4);
                *_RAM += (uint8_t)((*_1FFD & 16) >> 1);
            } else {
                *_ROM = (uint8_t)((val & 16) >> 4);
            }
            setPages();
        } else if(A8A15 < 0xC0) {
            // BFFD
//            snd->writeAY(*zxSND::_AY_REG, val);
        } else if(A8A15 >= 0xC0) {
            // FFFD
//            *zxSND::_AY_REG = (uint8_t)(val & 15);
        }
    } else switch(A0A7) {
            case 0xFE:
                // 0, 1, 2 - бордер, 3 MIC - при записи, 4 - бипер
                *_FE = val;
                colorBorder = val & 7U;
//                tape->writePort(val);// 00011000
                break;
            case 0x1F:
                *_KEMPSTON = val;
                break;
        }
}

uint8_t zxALU::readPort(uint8_t A0A7, uint8_t A8A15) {
    switch(A0A7) {
        // звук AY
//        case 0xFD: if(A8A15 == 0xff) return snd->ayRegs[*zxSND::_AY_REG]; break;
        // джойстик
        case 0x1F: return *_KEMPSTON;
        // клавиатура | MIC
//        case 0xFC:// TODO
        case 0xFE:
            A0A7 = 255;
            for(int i = 0; i < 8; i++) {
                if(A8A15 & (1 << i)) continue;
                A0A7 &= opts[i + ZX_PROP_VALUES_SEMI_ROW];
            }
            return A0A7/* | (tape->_FE8 << 3) */;
    }
    return _FF;
}

void zxALU::setPages() {
    pageROM = (*_STATE & ZX_TRDOS) ? pageTRDOS : (*_ROM == 100 ? PAGE_RAM[0] : PAGE_ROM[*_ROM]);
    pageRAM = PAGE_RAM[*_RAM];
    pageVRAM = PAGE_RAM[*_VID];
    pageATTRIB = pageVRAM + 6144;
}

void zxALU::execute() {
    // опрос отображение экрана, воспроизведение звука
    updateFrame();
    if(opts[ZX_PROP_SND_LAUNCH]) {
//        snd->execute();
//        snd->play();
    }
}

int zxALU::step(bool allow_int) {
    int ticks = 0;
    // установить возможность прерывания
    modifySTATE(allow_int * ZX_INT, 0);
    // перехват системных процедур
//    auto ticks = tape->trap(*cpu->_PC);
    // выполнение операции
    ticks += cpu->step(0, 0);
    // проверка на прерывания
    if(*_STATE & ZX_NMI) ticks += cpu->signalNMI();
    else if(*_STATE & ZX_INT) ticks += cpu->signalINT();
    *_TICK += ticks;
    return ticks;
}

void zxALU::updateCPU(int todo, bool interrupt) {
    if(opts[ZX_PROP_EXECUTE]) {
        int ticks;
        todo += periodGPU;
        if (interrupt) {
            if (periodGPU > 0) {
                ticks = step(false);
                todo -= ticks;
//                tape->control(ticks);
            }
            ticks = step(true);
//            ticks = step(false);
            todo -= ticks;
//            tape->control(ticks);
        }
        while (todo > 3) {
            ticks = step(false);
            // если сработала точка останова - возвращает 0
            if(!ticks) { todo &= 3; break; }
            todo -= ticks;
//            tape->control(ticks);
        }
        periodGPU = todo;
    }
}

void zxALU::updateFrame() {
    uint32_t line, i, tmp, c;

    auto dest = surface;
    if(!dest) return;

    auto isBlink = ((blink & blinkMsk) >> blinkShift) == 0;
    auto szBorder = sizeBorder >> 1;
    auto colours = (uint32_t*)&opts[ZX_PROP_COLORS];

    _FF = 0xff;
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

#pragma clang diagnostic pop