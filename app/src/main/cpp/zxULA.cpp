//
// Created by Сергей on 21.11.2019.
//

#include <ctime>
#include "zxCommon.h"
#include "zxULA.h"
#include "stkMnemonic.h"
#include "zxDA.h"
#include "zxSound.h"
#include "zxFormats.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

static uint8_t mem1FFD[] = { 0, 1, 2, 3,  4, 5, 6, 7,  4, 5, 6, 3,  4, 7, 6, 3,  0, 5, 2, 0 };
static int atrTab[192];
static uint8_t colTab[512];

uint32_t zxULA::frameTick(0);
uint16_t zxULA::PC(0);

// страницы памяти
uint8_t* zxULA::memPAGES[4];

// возврат из процедуры
uint16_t* zxULA::_CALL(nullptr);
// текущий счетчик инструкций
uint32_t* zxULA::_TICK(nullptr);
// STATE
uint8_t* zxULA::_STATE(nullptr);
//
ZX_MACHINE* zxULA::machine(nullptr);

// 0 KOMPANION, 1 48, 2 2006, 3 128_0, 4 128_1, 5 pentagon_0, 6 pentagon_1, 7 scorp_0, 8 scorp_1, 9 scorp_2, 10 scorp_3,
// 11 plus2_0, 12 plus2_1, 13 plus3_0, 14 plus3_1, 15 plus3_2, 16 plus3_3, 17 trdos_0, 18 trdos128_0
ZX_MACHINE machines[] = {
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 14336, 1773400, 3500000, 8, 1, 0, 1, 17, "KOMPANION" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 14336, 1773400, 3500000, 8, 1, 1, 1, 17, "SINCLER 48K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 14336, 1773400, 3500000, 8, 1, 2, 1, 17, "SINCLER 48K 2006" },
        {   {   { 63 * 228, 8 + 24, 44 + 24, 56 * 228 }, { 47 * 228, 8 + 16, 44 + 16, 40 * 228 },
                { 31 * 228, 8 + 8, 44 + 8, 24 * 228 }, { 15 * 228, 8, 44, 8 * 228 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            70908, 14336, 1773400, 3546900, 8, 0, 3, 2, 17, "SINCLER 128K" },
        {   {   { 63 * 228, 8 + 24, 44 + 24, 56 * 228 }, { 47 * 228, 8 + 16, 44 + 16, 40 * 228 },
                { 31 * 228, 8 + 8, 44 + 8, 24 * 228 }, { 15 * 228, 8, 44, 8 * 228 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            70908, 14336, 1773400, 3546900, 8, 0, 11, 2, 17, "SINCLER PLUS2" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 14336, 1773400, 3546900, 8, 2, 13, 4, 17, "SINCLAIR PLUS3" },
        {   {   { 80 * 224, 8 + 24, 40 + 24, 48 * 224 }, { 64 * 224, 8 + 16, 40 + 16, 32 * 224 },
                { 48 * 224, 8 + 8, 40 + 8, 16 * 224 }, { 32 * 224, 8, 40, 0 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            71680, 14336, 1792000, 3575000, 32, 0, 5, 2, 17, "PENTAGON 512K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 14336, 1750000, 3500000, 16, 0, 7, 3, 10, "SCORPION 256K" }
};

static uint8_t semiRows[] = {
        /* 00 - 0 - 0 */ 0, 0x00, 0, 0,
        /* 01 - 1 - 5 */ 3, 0x01, 0, 0, 3, 0x02, 0, 0, 3, 0x04, 0, 0, 3, 0x08, 0, 0, 3, 0x10, 0, 0,
        /* 06 - 6 - 0 */ 4, 0x10, 0, 0, 4, 0x08, 0, 0, 4, 0x04, 0, 0, 4, 0x02, 0, 0, 4, 0x01, 0, 0,
        /* 11 - del - del */   4, 0x01, 0, 1,
        /* 12 - Q - T */ 2, 0x01, 0, 0, 2, 0x02, 0, 0, 2, 0x04, 0, 0, 2, 0x08, 0, 0, 2, 0x10, 0, 0,
        /* 17 - Y - P */ 5, 0x10, 0, 0, 5, 0x08, 0, 0, 5, 0x04, 0, 0, 5, 0x02, 0, 0, 5, 0x01, 0, 0,
        /* 22 - ent - ent */   6, 0x01, 0, 0,
        /* 23 - caps - caps */ 0, 0x01, 0, 0,
        /* 24 - A - G */ 1, 0x01, 0, 0, 1, 0x02, 0, 0, 1, 0x04, 0, 0, 1, 0x08, 0, 0, 1, 0x10, 0, 0,
        /* 28 - H - L */ 6, 0x10, 0, 0, 6, 0x08, 0, 0, 6, 0x04, 0, 0, 6, 0x02, 0, 0,
        /* 32 - sm-[E]*/ 7, 0x02, 0, 0, 0, 0x01, 7, 2,
        /* 34 - Z - V */ 0, 0x02, 0, 0, 0, 0x04, 0, 0, 0, 0x08, 0, 0, 7, 0x01, 0, 0, 0, 0x10, 0, 0,
        /* 39 - B - M */ 7, 0x10, 0, 0, 7, 0x08, 0, 0, 7, 0x04, 0, 0,
        /* 42 - RU/EN */       0, 0, 0, 0,
        /* 43 - cursor*/    4, 0x08, 0, 1, 4, 0x10, 0, 1, 3, 0x10, 0, 1, 4, 0x04, 0, 1,
        /* 47 - KEMPSTON */ 8, 0x02, 0, 0, 8, 0x01, 0, 0, 8, 0x08, 0, 0, 8, 0x04, 0, 0, 8, 0x10, 0, 0
};

zxULA::zxULA() : pauseBetweenTapeBlocks(0), joyOldButtons(0), deltaTSTATE(0), _FF(255), colorBorder(7),
                 blink(0), sizeBorder(0), ROMb(nullptr), ROMtr(nullptr),
                 cpu(nullptr), snd(nullptr), tape(nullptr), disk(nullptr), gpu(nullptr) {
    RAMs = new uint8_t[ZX_TOTAL_RAM];
    ROMtr = new uint8_t[1 << 14];

    // вычисление таблицы адреса строк экрана
    for(int line = 0; line < 192; line++) atrTab[line] = (((line & 192) << 5) + ((line & 7) << 8) + ((line & 56) << 2));
    // таблица для мгновенного доступа к атрибутам
    for(int a = 0; a < 256; a++) {
        auto ink	= (uint8_t)((a >> 0) & 7);
        auto paper	= (uint8_t)((a >> 3) & 7);
        auto bright	= (uint8_t)((a >> 6) & 1);
        auto flash	= (uint8_t)((a >> 7) & 1);
        if(ink) ink |= bright << 3;
        if(paper) paper |= bright << 3;
        auto c1 = (uint8_t)((paper << 4) | ink);
        if(flash) { auto t = ink; ink = paper; paper = t; }
        auto c2 = (uint8_t)((paper << 4) | ink);
        colTab[a] = c1; colTab[a + 256] = c2;
    }

    // инициализация системы
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE]; *_STATE = 0;
    _KEMPSTON		= &opts[ZX_PROP_VALUES_KEMPSTON];
    _1FFD    		= &opts[PORT_1FFD];
    _7FFD    		= &opts[PORT_7FFD];
    _FE             = &opts[PORT_FE];

    _RAM            = &opts[RAM];
    _VID            = &opts[VID];
    _ROM            = &opts[ROM];
    _CALL           = (uint16_t*)&opts[CALL0];
    _TICK     		= (uint32_t*)&opts[TICK0];

    cpu = new zxCPU();
    gpu = new zxGPU();

    // машина по умолчанию(до загрузки состояния)
    machine = &machines[MODEL_48];

    // создаем устройства
    snd = new zxSoundMixer();
    tape = new zxTape(snd);
    disk = new zxVG93();
    assembler = new zxAssembler();
    debugger = new zxDebugger();
}

zxULA::~zxULA() {
    SAFE_DELETE(assembler);
    SAFE_DELETE(debugger);

    SAFE_DELETE(disk);
    SAFE_DELETE(tape);
    SAFE_DELETE(snd);

    SAFE_DELETE(gpu);
    SAFE_DELETE(cpu);

    SAFE_A_DELETE(RAMs);
    SAFE_A_DELETE(ROMtr);
    SAFE_A_DELETE(ROMb);
}

bool zxULA::load(const char *path, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_WAVE:
            ret = zxFormats::openWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = zxFormats::openTAP(path);
            if(ret && opts[ZX_PROP_TRAP_TAPE]) {
                ret = zxFormats::openZ80(*_MODEL >= MODEL_128 ? "tapLoad128.zx" : "tapLoad48.zx");
            }
            break;
        case ZX_CMD_IO_STATE:
            ret = restoreState(path);
            break;
        case ZX_CMD_IO_Z80:
            ret = zxFormats::openZ80(path);
            break;
    }
    return ret;
}

bool zxULA::restoreState(const char *path) {
    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[INDEX_OPEN], false);
    if(!ptr) return false;
    // меняем модель
    changeModel(ptr[MODEL], true);
    // грузим регистры
    memcpy(opts, ptr, COUNT_REGS); ptr += COUNT_REGS;
    // грузим банки ОЗУ
    for (int i = 0; i < machine->ramPages; i++) {
        size_t size = *(uint16_t*)ptr; ptr += sizeof(uint16_t);
        if (!unpackBlock(ptr, &RAMs[i << 14], &RAMs[i << 14] + 16384, size, true)) {
            LOG_DEBUG("Ошибка при распаковке страницы %i состояния!!!", i)
            signalRESET(true);
            return false;
        }
        ptr += size;
    }
    auto ptrName = (const char*)ptr;
    ptr += strlen(ptrName) + 1;
    // восстанавливаем состояние ленты
    if(!(ptr = tape->restoreState(ptr))) {
        LOG_DEBUG("Не удалось восстановить состояние ленты!!!", nullptr)
        signalRESET(true);
        return false;
    }
    // восстанавливаем состояние дисков
    if(!disk->restoreState(ptr)) {
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

bool zxULA::saveState(const char* path) {
    uint32_t size;
    auto buf = TMP_BUF;
    // сохраняем регистры
    modifySTATE(0, ZX_PAUSE);
    ssh_memcpy(&buf, opts, COUNT_REGS);
    // количество банков и их содержимое
    for(int i = 0; i < machine->ramPages; i++) {
        packBlock(&RAMs[i << 14], &RAMs[i << 14] + 16384, &buf[2], false, size);
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

bool zxULA::save(const char *path, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_WAVE:
            ret = zxFormats::saveWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = zxFormats::saveTAP(path);
            break;
        case ZX_CMD_IO_STATE:
            ret = saveState(path);
            break;
        case ZX_CMD_IO_Z80:
            ret = zxFormats::saveZ80(path);
            break;
    }
    return ret;
}

void zxULA::updateProps(int filter) {
    // граница
    sizeBorder = (uint32_t)opts[ZX_PROP_BORDER_SIZE];
    auto periodCPU = opts[ZX_PROP_CPU_SPEED];

    stateUP = machine->ts[sizeBorder].up * periodCPU / 10;
    stateLP = machine->ts[sizeBorder].lp * periodCPU / 10;
    stateRP = machine->ts[sizeBorder].rp * periodCPU / 10;
    stateDP = machine->ts[sizeBorder].dp * periodCPU / 10;

    sizeBorder *= 16;

    gpu->updateProps(sizeBorder, filter);
    snd->updateProps();
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

int zxULA::updateKeys(int key, int action) {
    if (key) {
        // клавиша была нажата/отпущена на экранной клавматуре
        auto idx = key * 4;
        auto semiRow = ZX_PROP_VALUES_SEMI_ROW + semiRows[idx + 0];
        auto semiRowEx = ZX_PROP_VALUES_SEMI_ROW + semiRows[idx + 2];
        auto bit = semiRows[idx + 1];
        auto bitEx = semiRows[idx + 3];
        auto mode = opts[ZX_PROP_KEY_MODE];
        auto ret = *_MODEL == MODEL_KOMPANION && (key == 43 && action);
        if(ret) {
            auto m = opts[RUS_LAT];
            opts[ZX_PROP_PORT_FEFC] = 255;
            m ^= 192; // признак режима
            opts[ZX_PROP_PORT_FEFC] &= ~(m & 128 ? 1 :  2);
            opts[RUS_LAT] = m;
        }
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
        return ret;
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
                if(*_MODEL == MODEL_KOMPANION) {
/*
                    static uint8_t sys[256];
                    static char tmp[32768];
                    char* _tmp = tmp;
                    int idx = 0;
                    for(int i = 0 ; i < 255; i++) {
                        if(i < 120 || i > 122) {
                            int a = 23552 + i;
                            int n = rm8((uint16_t)a);
                            int o = sys[i];
                            if (n != o) {
                                ssh_strcpy(&_tmp, ssh_ntos(&a, RADIX_DEC));
                                *_tmp++ = ' ';
                                ssh_strcpy(&_tmp, ssh_ntos(&n, RADIX_DEC));
                                *_tmp++ = ' ';
                                idx++;
                            }
                        }
                    }
                    if(idx > 0) {
                        *_tmp = 0;
                        LOG_INFO("%s", tmp);
                        LOG_INFO("", 0);
                    }
                    memcpy(sys, realPtr(23552), 255);
*/
                    // проверить на текущий язык(по информации из знакогенератора)
                    auto n = ((rm8(23606) << 8) | rm8(23607));// en - 15424, ru - 14656
                    auto o = opts[RUS_LAT];
                    auto lng = (uint8_t)(((n == 14656) << 7) | 127);
                    if(lng != o) {
                        opts[ZX_PROP_KEY_CURSOR_MODE] = 255;
                        opts[RUS_LAT] = lng;
                    }
                }
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
                    memcpy(&opts[ZX_PROP_VALUES_BUTTON], &buttons[nmode * 86], 86);
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

void zxULA::changeModel(uint8_t _new, bool reset) {
    static zxFile file;

    machine         = &machines[_new];
    auto totalRom   = machine->totalRom << 14;
    auto rom        = new uint8_t[totalRom];
    if(file.open((FOLDER_FILES + "rom.zx").c_str(), zxFile::open_read)) {
        // trdos rom
        file.set_pos(machine->indexTRDOS << 14, zxFile::begin);
        file.read(ROMtr, 1 << 14);
        // base rom
        file.set_pos(machine->indexRom << 14, zxFile::begin);
        file.read(rom, totalRom);
        file.close();
        *_MODEL = _new;
        opts[ZX_PROP_MODEL_TYPE] = _new;
        delete[] ROMb; ROMb = rom; ROMe = ROMb + totalRom;
        totalRom >>= 14;
        for(int i = 0 ; i < 4; i++) PAGE_ROM[i] = rom +  ((i >= totalRom ? totalRom - 1 : i) << 14);
    } else {
        LOG_INFO("Не удалось загрузить ПЗУ для машины - %s!", machines[_new].name);
        delete[] rom;
    }
    signalRESET(reset);
}

void zxULA::signalRESET(bool reset) {
    // сброс устройств
    if(reset) tape->reset();
    //if(reset) disk->reset();
    snd->reset();
    // очищаем ОЗУ
    ssh_memzero(RAMs, ZX_TOTAL_RAM);
    // очищаем регистры/порты
    ssh_memzero(opts, STATE);
    // сбрасываем клавиатуру
    ssh_memset(&opts[ZX_PROP_VALUES_SEMI_ROW], 255, 8);
    opts[ZX_PROP_KEY_MODE] = 0; opts[ZX_PROP_KEY_CURSOR_MODE] = 255;
    // сброс мыши
    opts[MOUSE_K] = 0xFF; opts[MOUSE_X] = 128; opts[MOUSE_Y] = 96;
    // сбрасываем джойстики
    opts[ZX_PROP_JOY_ACTION_VALUE] = 0; opts[ZX_PROP_JOY_CROSS_VALUE] = 0;
    joyOldButtons = 0;
    // инициализаруем переменные
    *cpu->_SP = 65534; *_FE = 0b11100111; *_VID = 5; *_RAM = 0; *_ROM = machine->startRom;
    memPAGES[1] = &RAMs[5 << 14]; memPAGES[2] = &RAMs[2 << 14];
    // сброс состояния с сохранением статуса отладчика
    *_STATE &= ZX_DEBUG;
    // установка страниц
    setPages();
    // установка имени программы
    programName("BASIC");
}

void zxULA::setPages() {
    if(checkSTATE(ZX_TRDOS)) {
        memPAGES[0] = ROMtr;
    } else {
        auto rom = *_ROM;
        if(rom < 100) {
            memPAGES[0] = PAGE_ROM[rom];
        } else {
            memPAGES[0] = &RAMs[mem1FFD[rom - 100]];
        }
    }
    memPAGES[3] = &RAMs[*_RAM << 14];
    pageVRAM    = &RAMs[*_VID << 14];
    pageATTRIB  = pageVRAM + 6144;
}

void zxULA::execute() {
    // формирование/отображение экрана
    updateFrame();
    gpu->updateFrame();
}

void zxULA::stepDebug() {
    PC = *cpu->_PC;
    // убрать отладчик - чтобы не сработала точка останова
    modifySTATE(0, ZX_DEBUG | ZX_HALT);
    // перехват системных процедур
    auto ticks = cpu->step();
    *_TICK += ticks;
    frameTick += ticks;
    trap();
    modifySTATE(ZX_DEBUG, 0);
}

int zxULA::step(bool interrupt) {
    // если отменено выполнение - эмулируем NOP
    if(!opts[ZX_PROP_EXECUTE]) return 4;
    // если пауза - аналогично
    if(checkSTATE(ZX_PAUSE)) return 4;
    // проверить на точку останова для кода
    PC = *cpu->_PC;
    if(checkSTATE(ZX_DEBUG)) { if(checkBPs(PC, ZX_BP_EXEC)) return 4; }
    // проверяем на возможность прерывания
    int tick = 0;
    if(interrupt) {
        if((tick = cpu->signalINT()))
            return tick;
    }
    // выполнение инструкции процессора
    tick = cpu->step();
    // перехват системных процедур
    trap();
    return tick;
}

void zxULA::updateCPU(int todo, bool interrupt) {
    int ticks;
    todo += deltaTSTATE;
    if (interrupt) {
        ticks = step(true);
        todo -= ticks;
        *_TICK += ticks;
        frameTick += ticks;
    }
    while(todo > 0) {
        ticks = step(false);
        todo -= ticks;
        *_TICK += ticks;
        frameTick += ticks;
    }
    deltaTSTATE = todo;
}

void zxULA::updateFrame() {
    static uint32_t frames[2] = { 0, 0 };

    uint32_t line, i, c;

    if(pauseBetweenTapeBlocks > 0) {
        pauseBetweenTapeBlocks--;
        if(pauseBetweenTapeBlocks == 0)
            *_STATE &= ~ZX_PAUSE;
    }

    auto dest = gpu->frameBuffer;
    if(!dest) return;

    frameTick = *_TICK % machine->tsTotal;
    snd->frameStart(frameTick);

    auto isBlink = (blink & 16) >> 4;
    auto szBorder = sizeBorder >> 1;
    auto colours = (uint32_t*)&opts[ZX_PROP_COLORS];

    _FF = 0xff;
    colorBorder = (uint8_t)(*_FE & 7);

    updateCPU(stateUP, true);
    for(line = 0 ; line < sizeBorder; line++) {
        updateCPU(stateLP, false);
        for (i = 0; i < (128 + sizeBorder); i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c; *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    // screen
    for(line = 0 ; line < 192; line++) {
        updateCPU(stateLP, false);
        for(i = 0 ; i < szBorder; i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c; *dest++ = c;
        }
        *_STATE |= ZX_SCR;
        auto rb = atrTab[line];
        for (int ri = 0; ri < 32; ri++) {
            _FF = pageATTRIB[ri + ((line & 248) << 2)];
            auto idx = colTab[(((_FF & 128) && isBlink) << 8) + _FF];
            frames[1] = colours[idx & 15];
            frames[0] = colours[idx >> 4];
            auto v = pageVRAM[rb];
            for(int b = 7 ; b >= 0; b--) {
                *dest++ = frames[(v >> b) & 1];
                if(!(b & 1)) updateCPU(1, false);
            }
            rb++;
        }
        *_STATE &= ~ZX_SCR;
        for(i = 0 ; i < szBorder; i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c; *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    for(line = 0 ; line < sizeBorder; line++) {
        updateCPU(stateLP, false);
        for (i = 0; i < (sizeBorder + 128); i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c; *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    updateCPU(stateDP, false);
    blink++;
    //LOG_INFO("ts: %i", _TICK % ts);
    snd->frameEnd(frameTick);
}

void zxULA::quickBP(uint16_t address) {
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

BREAK_POINT* zxULA::quickCheckBPs(uint16_t address, uint8_t flg) {
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

bool zxULA::checkBPs(uint16_t address, uint8_t flg) {
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
        // установить глобальную остановку системы
        opts[ZX_PROP_EXECUTE] = 0;
        // сообщение для выхода - обновить отладчик
        *_STATE |= ZX_BP;
        // вернуть на начало инструкции (в случае доступа к портам или памяти)
        *cpu->_PC = PC;
    }
    return res;
}

const char *zxULA::programName(const char *nm) {
    if(nm && strlen(nm) > 0) {
        auto tmp_nm = (char*)&TMP_BUF[INDEX_TEMP];
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

int zxULA::diskOperation(int num, int ops, const char* path) {
    int ret(0);
    switch(ops & 7) {
        case ZX_DISK_OPS_GET_READONLY:  ret = disk->is_readonly(num & 3); break;
        case ZX_DISK_OPS_EJECT:         disk->eject(num); break;
        case ZX_DISK_OPS_OPEN:          ret = (int)disk->open(path, num, parseExtension(path)); break;
        case ZX_DISK_OPS_SAVE:          ret = disk->save(path, num, parseExtension(path)); break;
        case ZX_DISK_OPS_SET_READONLY:  ret = disk->is_readonly(num & 3, (num & 128)); break;
        case ZX_DISK_OPS_TRDOS:         ret = zxFormats::openZ80(*_MODEL >= MODEL_128 ? "trdosLoad128.zx" : "trdosLoad48.zx"); break;
        case ZX_DISK_OPS_RSECTOR:       ret = disk->read_sector(num & 3, (num >> 3) + 1); break;
    }
    return ret;
}

void zxULA::quickSave() {
    static zxFile file;
    int a = 0;auto buf = (char*)&TMP_BUF[INDEX_TEMP];
    while(true) {
        sprintf(buf, "%sSAVERS/%s_%02i.z80", FOLDER_FILES.c_str(), name.c_str(), a);
        if(!file.open(buf, zxFile::open_read)) break;
        file.close();
        a++;
    }
    zxFormats::saveZ80(strstr(buf, "SAVERS/"));
}

void zxULA::trap() {
    auto pc = *cpu->_PC;
    if(pc < 16384) {
        // активность TR DOS
        if (!checkSTATE(ZX_TRDOS)) {
            if (pc >= 15616 && pc <= 15871) {
//                if(PC == 15616) zxFormats::saveZ80(*_MODEL >= MODEL_128 ? "trdosLoad128.zx" : "trdosLoad48.zx");
                if(*_ROM == 1) {
                    *_STATE |= ZX_TRDOS;
                    setPages();
                }
            }
            bool success = false;
            if (pc == 1218) success = tape->trapSave();
            else if (pc == 1366 || pc == 1378) {
//                zxFormats::saveZ80(*_MODEL >= MODEL_128 ? "tapLoad128.zx" : "tapLoad48.zx");
                success = tape->trapLoad();
            }
            if (success) {
                auto psp = cpu->_SP;
                auto addr = rm16(*psp);
                *cpu->_PC = addr;
                *psp += 2;
            }
        } else {
            disk->trap(pc);
        }
    } else {
        if (*_STATE & ZX_TRDOS) {
            *_STATE &= ~ZX_TRDOS;
            setPages();
        }
    }
}

static bool isAyport(uint16_t port) {
    if(port&2) return false;
    if((port & 0xC0FF) == 0xC0FD) return true;
    return (port & 0xC000) == 0x8000;
}

void zxULA::writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val) {
    uint16_t p = (A8A15 << 8) | A0A7;
    if(isAyport(p)) {
        snd->ioWrite(0, p, val, frameTick);
        return;
    }
    switch(A0A7) {
        case 0xFD:
            if((*_MODEL == MODEL_PLUS3 || *_MODEL == MODEL_SCORPION) && A8A15 == 0x1F) {
                // 0,2,5,12=1; 1,14,15=0
                write1FFD(val);
            } else write7FFD(val);
            break;
        case 0xFE:
            // 0, 1, 2 - бордер, 3 MIC - при записи, 4 - бипер
            *_FE = val;
            colorBorder = val & 7U;
            snd->ioWrite(1, 0xFE, val, frameTick);
            tape->writePort((uint8_t)(val & 24));
            break;
        case 0x1F: case 0x3F: case 0x5F: case 0x7F: case 0xFF:
            if(checkSTATE(ZX_TRDOS)) {
                disk->vg93_write(A0A7, val);
            } else {

            }
            break;
        default:
//            LOG_DEBUG("WRITE UNKNOWN PORT (%02X%02X(%i) - PC: %i", A8A15, A0A7, val, PC);
            break;
   }
    if(checkSTATE(ZX_DEBUG)) { if(checkBPs(p, ZX_BP_WPORT)) return; }
}

void zxULA::write1FFD(uint8_t val) {
    // 0   -> 1 - RAM0(!!), 0 - see bit 1 etc
    // 1   -> 1 - ROM 2, 0 - ROM from 0x7FFD
    // 4   -> 1 - RAM SCORPION/KAY 256K, 0 - from 0x7FFD
    // 6.7 -> SCORPION/KAY 1024K
    *_1FFD = val;
    uint8_t* ram(&mem1FFD[16]);
    switch (*_MODEL) {
        case MODEL_SCORPION:
            if (val & 1) *_ROM = ram[0];
            else {
                if (val & 2) *_ROM = 2;
                else *_ROM = (uint8_t) ((*_7FFD & 16) >> 4);
            }
            *_RAM = (uint8_t) ((*_7FFD & 7) + (uint8_t) ((val & 16) >> 1));
            break;
        case MODEL_PLUS3:
            if(val & 1) {
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
                auto rom = (val & 3) << 2;
                ram = &mem1FFD[rom];
                memPAGES[0] = &RAMs[ram[0] << 14];
                *_ROM = (uint8_t)(rom + 100);
                *_RAM = ram[3];
                memPAGES[1] = &RAMs[ram[1] << 14];
                memPAGES[2] = &RAMs[ram[2] << 14];
            } else {
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
                *_ROM = (uint8_t)(((val & 4) >> 1) | ((*_7FFD & 16) >> 4));
                //*_RAM = (uint8_t)(*_7FFD & 7);
                //auto motor = (uint8_t)((val & 8) != 0);
                //auto strob = (uint8_t)((val & 16) != 0);
            }
            LOG_INFO("plus3 val: %i ROM:%i RAM:%i PC:%X", val, *_ROM, *_RAM, PC);
            break;
    }
    setPages();
}

void zxULA::write7FFD(uint8_t val) {
    // 0, 1, 2 - страница 0-7
    // 3 - экран 5/7
    // 4 - ПЗУ 0 - 128К 1 - 48К
    // 5 - блокировка
    // 6 - pentagon 256K, KAY 1024K
    // 7 - pentagon 512K
    if(*_MODEL >= MODEL_128) {
        if (*_7FFD & 32) return;
        *_7FFD = val;
        *_VID = (uint8_t) ((val & 8) ? 7 : 5);
        *_RAM = (uint8_t) (val & 7);
        switch(*_MODEL) {
            case  MODEL_SCORPION:
                if (!(*_1FFD & 2)) *_ROM = (uint8_t) ((val & 16) >> 4);
                *_RAM += (uint8_t) ((*_1FFD & 16) >> 1);
                break;
            case MODEL_PENTAGON:
                *_RAM += (uint8_t) ((val & 192) >> 3);
            default:
                *_ROM = (uint8_t) ((val & 16) >> 4);
                break;
        }
        if(val & 32) {
            LOG_INFO("block 7ffd %i PC:%i", val, PC);
        }
        setPages();
    }
}

uint8_t zxULA::readPort(uint8_t A0A7, uint8_t A8A15) {
    uint16_t p = (A8A15 << 8) | A0A7;
    if(isAyport(p)) return snd->ioRead(0, p);
    uint8_t ret = _FF;
    switch(A0A7) {
        case 0x1F:
            if(*_MODEL == MODEL_SCORPION && *_ROM == 2) break;
        case 0x3F: case 0x5F: case 0x7F: case 0xFF:
            if(checkSTATE(ZX_TRDOS)) ret = disk->vg93_read(A0A7); else ret = *_KEMPSTON;
            break;
        // COVOX
        case 0xBF:
            ret = 0;
            break;
        // Kempston Mouse
        case 0xDF:
            if(A8A15 == 0xFA) {
                // key 254 -> left 253 -> right
                ret = opts[MOUSE_K];
                opts[MOUSE_K] = 255;
            }
            else if(A8A15 == 0xFB) ret = opts[MOUSE_X];
            else if(A8A15 == 0xFF) ret = opts[MOUSE_Y];
            else {
                // SPECDRUM
            }
            break;
        case 0xFC:
            if(A8A15 == 0xFE && *_MODEL == MODEL_KOMPANION) {
                auto k = opts[ZX_PROP_PORT_FEFC];
                if(!(opts[RUS_LAT] & 64)) {
                    ret = k;
                    opts[ZX_PROP_PORT_FEFC] = 255;
                    opts[RUS_LAT] |= 127;
                }
            }
            break;
        case 0xFE:
            // 0,1,2,3,4 - клавиши полуряда, 6 - EAR, 5,7 - не используется
            A0A7 = 31;
            for (int i = 0; i < 8; i++) {
                if (A8A15 & (1 << i)) continue;
                A0A7 &= opts[i + ZX_PROP_VALUES_SEMI_ROW];
            }
            ret = (A0A7 | tape->readPort());
            break;
        default:
            //LOG_DEBUG("READ UNKNOWN PORT %X%X PC: %i", A8A15, A0A7, PC);
            break;
    }
    if(checkSTATE(ZX_DEBUG)) { if(checkBPs((uint16_t)(A0A7 | (A8A15 << 8)), ZX_BP_RPORT)) return ret; }
    return ret;
}

int zxULA::getAddressCpuReg(const char *value) {
    static uint8_t regs[] = { 'I', 'X', RXL, 'I', 'Y', RYL, 'H', 'L', RL, 'D', 'E', RE, 'B', 'C', RC, 'S', 'P', RSPL, 'P', 'C', RPCL};
    int ret, i;
    for(i = 0 ; i < 7; i++) {
        auto pos = i * 3;
        if(value[0] == regs[pos] && value[1] == regs[pos + 1])
            break;
    }
    if(i > 6) {
        ret = *(int*)ssh_ston(value, RADIX_DEC);
    } else {
        ret = *(uint16_t*)&opts[regs[i * 3 + 2]];
    }
    return ret;
}

jint zxULA::UpdateSound(uint8_t *buf) {
    snd->update(buf);
    auto size = snd->ready();
    snd->use(size, buf);
    return size;
}

#pragma clang diagnostic pop
