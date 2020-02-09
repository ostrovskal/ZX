//
// Created by Сергей on 21.11.2019.
//

#include <ctime>
#include "zxCommon.h"
#include "zxULA.h"
#include "stkMnemonic.h"
#include "zxDA.h"
#include "zxSound.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

static uint8_t mem1FFD[] = { 0, 1, 2, 3,  4, 5, 6, 7,  4, 5, 6, 3,  4, 7, 6, 3,  0, 5, 2, 0 };

// текущий счетчик инструкций
long zxULA::_TICK(0);

uint16_t* zxULA::_CALL(nullptr);
uint16_t zxULA::PC(0);

// страницы памяти
uint8_t* zxULA::memPAGES[4];

// STATE
uint8_t* zxULA::_STATE(nullptr);

// 0 KOMPANION, 1 48, 2 2006, 3 128_0, 4 128_1, 5 pentagon_0, 6 pentagon_1, 7 scorp_0, 8 scorp_1, 9 scorp_2, 10 scorp_3,
// 11 plus2_0, 12 plus2_1, 13 plus3_0, 14 plus3_1, 15 plus3_2, 16 plus3_3, 17 trdos_0, 18 trdos128_0
ZX_MACHINE machines[] = {
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 3500000, 8, 1, 0, 1, 17, "KOMPANION" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 3500000, 8, 1, 1, 1, 17, "SINCLER 48K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 3500000, 8, 1, 2, 1, 17, "SINCLER 48K 2006" },
        {   {   { 63 * 228, 8 + 24, 44 + 24, 56 * 228 }, { 47 * 228, 8 + 16, 44 + 16, 40 * 228 },
                { 31 * 228, 8 + 8, 44 + 8, 24 * 228 }, { 15 * 228, 8, 44, 8 * 228 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            70908, 3546900, 8, 0, 3, 2, 17, "SINCLER 128K" },
        {   {   { 63 * 228, 8 + 24, 44 + 24, 56 * 228 }, { 47 * 228, 8 + 16, 44 + 16, 40 * 228 },
                { 31 * 228, 8 + 8, 44 + 8, 24 * 228 }, { 15 * 228, 8, 44, 8 * 228 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            70908, 3546900, 8, 0, 11, 2, 17, "SINCLER PLUS2" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 3546900, 8, 2, 13, 4, 17, "SINCLAIR PLUS3" },
        {   {   { 80 * 224, 8 + 24, 40 + 24, 48 * 224 }, { 64 * 224, 8 + 16, 40 + 16, 32 * 224 },
                { 48 * 224, 8 + 8, 40 + 8, 16 * 224 }, { 32 * 224, 8, 40, 0 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            71680, 3575000, 16, 0, 5, 2, 17, "PENTAGON 256K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 3546900, 16, 0, 7, 3, 10, "SCORPION 256K" }
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
        4, 0x08, 0, 1, 4, 0x10, 0, 1, 3, 0x10, 0, 1, 0, 0, 0, 0,
        4, 0x04, 0, 1,// 44
        8, 0x02, 0, 0, 8, 0x01, 0, 0, 8, 0x08, 0, 0, 8, 0x04, 0, 0, 8, 0x10, 0, 0
};

static void packPage(uint8_t** buffer, uint8_t* src, uint8_t page) {
    uint32_t size;
    uint8_t* buf = *buffer;
    *buffer = packBlock(src, src + 16384, &buf[3], false, size);
    *(uint16_t*)buf = (uint16_t)size;
    buf[2] = page;
}

zxULA::zxULA() : pauseBetweenTapeBlocks(0), joyOldButtons(0), deltaTSTATE(0), _FF(255), colorBorder(7),
                 blink(0), sizeBorder(0), machine(nullptr), ROMb(nullptr), ROMtr(nullptr),
                 cpu(nullptr), snd(nullptr), tape(nullptr), disk(nullptr), gpu(nullptr) {
    RAMs = new uint8_t[ZX_TOTAL_RAM];
    ROMtr = new uint8_t[1 << 14];

    // инициализация системы
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE]; *_STATE = 0;
    _KEMPSTON		= &opts[ZX_PROP_VALUES_KEMPSTON];
    _1FFD    		= &opts[PORT_1FFD];
    _7FFD    		= &opts[PORT_7FFD];
    _FE             = &opts[PORT_FE];

    zxULA::_RAM     = &opts[RAM];
    zxULA::_VID     = &opts[VID];
    zxULA::_ROM     = &opts[ROM];
    zxULA::_CALL    = (uint16_t*)&opts[CALL0];

    cpu = new zxCPU();
    gpu = new zxGPU();

    snd = new zxSound();
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
            ret = tape->openWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = tape->openTAP(path);
            if(ret && opts[ZX_PROP_TRAP_TAPE]) {
                ret = openZ80(*_MODEL >= MODEL_128 ? "tapLoad128.zx" : "tapLoad48.zx");
            }
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

bool zxULA::openState(const char *path) {
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
    if(!(ptr = tape->loadState(ptr))) {
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

bool zxULA::openZ80(const char *path) {
    size_t sz;
    HEAD1_Z80* head1 = nullptr;
    HEAD2_Z80* head2 = nullptr;
    HEAD3_Z80* head3 = nullptr;
    uint8_t model = MODEL_48;
    int version;
    int pages = 3;

    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[INDEX_OPEN], true, &sz);
    if(!ptr) return false;

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
            default: LOG_DEBUG("Недопустимый формат файла %s!", path); return false;
        }
        if(model == MODEL_48) {
            pages = (head2->emulateFlags & 0x80) ? 1 : 3;
        } else if(model == MODEL_128) {
            pages = 8;
        } else if(mode == 9) {
            model = MODEL_PENTAGON;
            pages = 8;
        } else if(mode == 10) {
            model = MODEL_SCORPION;
            pages = 16;
        } else {
            LOG_INFO("Неизвестное оборудование %i в (%s)", mode, path);
            return false;
        }
        LOG_DEBUG("version:%i length:%i model:%i pages:%i mode:%i", version, length, model, pages, mode);
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
                        // 4->2 5->7 8->5
                        case 1: numPage = 2; break;
                        case 2: numPage = 7; break;
                        case 5: numPage = 5; break;
                        default: isValidPage = false;
                    }
                    break;
                case MODEL_128:
                case MODEL_PENTAGON:
                case MODEL_SCORPION:
                    isValidPage = numPage == i;
                    break;
            }
            if(!isValidPage) {
                LOG_DEBUG("Неизвестная страница %i в (%s)!", numPage, path);
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
        // перераспределяем буфер 0->5, 1->2, 2->7
        memcpy(&TMP_BUF[5 << 14], &TMP_BUF[0 << 14], 16384);
        memcpy(&TMP_BUF[7 << 14], &TMP_BUF[2 << 14], 16384);
        memcpy(&TMP_BUF[2 << 14], &TMP_BUF[1 << 14], 16384);
    }
    // меняем модель памяти и иинициализируем регистры
    auto isZ80 = strcasecmp(path + strlen(path) - 4, ".z80") == 0;
    changeModel(model, isZ80);
    *cpu->_BC = head1->BC; *cpu->_DE = head1->DE; *cpu->_HL = head1->HL;
    *cpu->_A = head1->A; *cpu->_F = head1->F; opts[RA_] = head1->A_; opts[RF_] = head1->F_;
    *cpu->_SP = head1->SP; *cpu->_IX = head1->IX; *cpu->_IY = head1->IY;
    *cpu->_I = head1->I; *cpu->_IM = (uint8_t)(head1->STATE2 & 3);
    *cpu->_IFF1 = head1->IFF1; *cpu->_IFF2 = head1->IFF2;
    if(head1->STATE1 == 255) head1->STATE1 = 1;
    *cpu->_R |= (head1->STATE1 << 7) | (head1->R & 0x7F);
    writePort(0xfe, 0, (uint8_t)(224 | ((head1->STATE1 & 14) >> 1)));
    memcpy(&opts[RC_], &head1->BC_, 6);
    *cpu->_PC = PC;
    LOG_INFO("Z80 Start PC: %i", PC);
    if(head2) {
        writePort(0xfd, 0x7f, head2->hardState);
        memcpy(&opts[AY_AFINE], head2->sndRegs, 16);
        writePort(0xfd, 0xff, head2->sndChipRegNumber);
    }
    if(head3 && length == 87) writePort(0xfd, 0x1f, head3->port1FFD);
    // копируем буфер
    memcpy(RAMs, TMP_BUF, 256 * 1024);
    return true;
}

bool zxULA::save(const char *path, int type) {
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

bool zxULA::saveZ80(const char *path) {
    static uint8_t models[] = { 0, 0, 0, 4, 4, 4, 9, 10 };
    static HEAD3_Z80 head;

    auto head2 = &head.head2;
    auto head1 = &head2->head1;
    memset(&head, 0, sizeof(HEAD3_Z80));

    auto buf = TMP_BUF;
    // основные
    head1->BC = *cpu->_BC; head1->DE = *cpu->_DE; head1->HL = *cpu->_HL;
    head1->A = *cpu->_A; head1->F = *cpu->_F; head1->A_ = opts[RA_]; head1->F_ = opts[RF_];
    head1->SP = *cpu->_SP; head1->IX = *cpu->_IX; head1->IY = *cpu->_IY; head1->PC = 0;
    head1->STATE2 = *cpu->_IM; head1->IFF1 = *cpu->_IFF1; head1->IFF2 = *cpu->_IFF2;
    head1->I = *cpu->_I; head1->R = (uint8_t)(*cpu->_R & 127);
    head1->STATE1 = (uint8_t)((*cpu->_R & 128) >> 7) | (uint8_t)((*_FE & 7) << 1);
    memcpy(&head1->BC_, &opts[RC_], 6);
    // для режима 128К
    head2->PC = *cpu->_PC;
    head2->hardMode = models[*_MODEL];
    head2->length = 55;
    head.port1FFD = *_1FFD;
    head2->hardState = *_7FFD;
    head2->sndChipRegNumber = opts[AY_REG];
    memcpy(head2->sndRegs, &opts[AY_AFINE], 16);
    ssh_memzero(head2->sndRegs, 16);
    // формируем буфер из содержимого страниц
    ssh_memcpy(&buf, &head, sizeof(HEAD3_Z80));
    // страницы, в зависимости от режима
    if(*_MODEL < MODEL_128) {
        // 4->2 5->7 8->5
        packPage(&buf, &RAMs[5 << 14], 8);
        packPage(&buf, &RAMs[2 << 14], 4);
        packPage(&buf, &RAMs[7 << 14], 5);
    } else {
        for(int i = 0; i < machine->ramPages; i++) {
            packPage(&buf, &RAMs[i << 14], (uint8_t)(i + 3));
        }
    }
    return zxFile::writeFile(path, TMP_BUF, buf - TMP_BUF, true);
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
                // 23728 - #57|#4F
                //auto lng = rm8(23728);
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
    if(reset) disk->reset();
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
    *cpu->_SP = 65534; *_FE = 0b11100111; *_VID = 5; *_RAM = 7; *_ROM = machine->startRom;
    //*_7FFD = *_RAM | (*_ROM << 4);
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
    // отображение экрана, воспроизведение звука
    updateFrame();
    gpu->updateFrame();
    //snd->update();
}

void zxULA::stepDebug() {
    PC = *cpu->_PC;
    // убрать отладчик - чтобы не сработала точка останова
    modifySTATE(0, ZX_DEBUG | ZX_HALT);
    // перехват системных процедур
    _TICK += cpu->step();
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
        _TICK += ticks;
    }
    while(todo > 0) {
        ticks = step(false);
        todo -= ticks;
        _TICK += ticks;
    }
    deltaTSTATE = todo;
}

void zxULA::updateFrame() {
    static uint32_t frames[2] = { 0, 0 };

    uint32_t line, i, tmp, c;

    if(pauseBetweenTapeBlocks > 0) {
        pauseBetweenTapeBlocks--;
        if(pauseBetweenTapeBlocks == 0)
            *_STATE &= ~ZX_PAUSE;
    }

    auto dest = gpu->frameBuffer;
    if(!dest) return;

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
            for(int b = 7 ; b >= 0; b--) {
                auto v = pageVRAM[rb];
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
    //LOG_INFO("ts: %i", _TICK % machine->tsTotal);
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
        case ZX_DISK_OPS_TRDOS:         ret = openZ80(*_MODEL >= MODEL_128 ? "trdosLoad128.zx" : "trdosLoad48.zx"); break;
        case ZX_DISK_OPS_RSECTOR:       ret = disk->read_sector(num & 3, (num >> 3) + 1); break;
        case ZX_DISK_COUNT_FILES:       ret = disk->count_files(num & 3, num >> 3); break;
    }
    return ret;
}

void zxULA::quickSave() {
    static zxFile file;
    int a = 0;
    auto buf = (char*)&TMP_BUF[INDEX_TEMP];
    while(true) {
        sprintf(buf, "%sSAVERS/%s_%02i.z80", FOLDER_FILES.c_str(), name.c_str(), a);
        if(!file.open(buf, zxFile::open_read)) break;
        file.close();
        a++;
    }
    saveZ80(strstr(buf, "SAVERS/"));
}

void zxULA::trap() {
    auto pc = *cpu->_PC;
    if(pc < 16384) {
        // активность TR DOS
        if (!checkSTATE(ZX_TRDOS)) {
            if (pc >= 15616 && pc <= 15871) {
//                if(PC == 15616) saveZ80(*_MODEL >= MODEL_128 ? "trdosLoad128.zx" : "trdosLoad48.zx");
                if(*_ROM == 1) {
                    *_STATE |= ZX_TRDOS;
                    setPages();
                }
            }
            bool success = false;
            if (pc == 1218) success = tape->trapSave();
            else if (pc == 1366 || pc == 1378) {
//                saveZ80(*_MODEL >= MODEL_128 ? "tapLoad128.zx" : "tapLoad48.zx");
                success = tape->trapLoad();
            }
            if (success) {
                auto psp = cpu->_SP;
                auto addr = rm16(*psp);
                *cpu->_PC = addr;
                *psp += 2;
            }
        }
    } else {
        if (*_STATE & ZX_TRDOS) {
            *_STATE &= ~ZX_TRDOS;
            setPages();
        }
    }
}

void zxULA::writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val) {
    switch(A0A7) {
        case 0xFD:
            if(*_MODEL >= MODEL_PLUS3 && A8A15 == 0x1F) {
                write1FFD(val);
            } else if(A8A15 == 0xFF) {
                // устанавливаем текущий регистр
                opts[AY_REG] = (uint8_t)(val & 15);
            } else if(A8A15 == 0xBF) {
                // записываем значение в текущий регистр
                auto reg = opts[AY_REG];
                opts[reg + AY_AFINE] = val;
                snd->ayWrite(reg, val, _TICK);
            } else {
//                LOG_INFO("7FFD A0A7:%i A8A15:%i val:%i", A0A7, A8A15, val);
                write7FFD(val);
            }
            break;
        case 0xFE:
            // 0, 1, 2 - бордер, 3 MIC - при записи, 4 - бипер
            *_FE = val;
            colorBorder = val & 7U;
            snd->beeperWrite((uint8_t)(val & 24));
            tape->writePort((uint8_t)(val & 24));
            break;
        case 0x1F: case 0x3F: case 0x5F: case 0x7F: case 0xFF:
            if(checkSTATE(ZX_TRDOS)) {
                disk->vg93_write(A0A7, val, 0);
            } else {

            }
            break;
        default:
            LOG_DEBUG("WRITE UNKNOWN PORT (%02X%02X(%i) - PC: %i", A8A15, A0A7, val, PC);
            break;
   }
    if(checkSTATE(ZX_DEBUG)) { if(checkBPs((uint16_t)(A0A7 | (A8A15 << 8)), ZX_BP_WPORT)) return; }
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
        setPages();
    }
}

uint8_t zxULA::readPort(uint8_t A0A7, uint8_t A8A15) {
    uint8_t ret = _FF;
    switch(A0A7) {
        case 0x1F:
            if(!checkSTATE(ZX_TRDOS)) { ret = (*_MODEL == MODEL_SCORPION && *_ROM == 2) ? _FF: *_KEMPSTON; break; }
        case 0x3F: case 0x5F: case 0x7F: case 0xFF:
            if(checkSTATE(ZX_TRDOS)) ret = disk->vg93_read(A0A7, 0);
            break;
        case 0xBF:
            // COVOX
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
        case 0xFD:
            if(A8A15 == 0xFF) ret = opts[opts[AY_REG] + AY_AFINE];
            else if(A8A15 == 0xBF) {
                /*
                6.4 RD #BFFD(%10xxx1x1xxxxxx01) - порт статуса системных вызовов.
                RESET: невозможен.

                D0 #BFFD NMI Magic: 0-on, 1-off - индицирует вызов от кнопки Magic.
                D1 #BFFD NMI Key: 0-on, 1-off - индицирует вызов от кнопки NMI на клавиатуре.
                D2 #BFFD NMI Instruction eZ80 or non documented command: 0-on, 1-off - индицирует вызов при считывании КОПа
                              расширенной инструции CPU eZ80 в стандартном режиме, или недокументированной команды.
                D3 #BFFD NMI I/O Adr eZ80: 0-on, 1-off - индицирует вызов при работе с CPU eZ80, возникающий при обращении к портам в диапазоне #80-#FF.
                D4 #BFFD NMI Error: 0-on, 1-off - индицирует вызов при ошибке контрольной суммы ОЗУ.
                D5 #BFFD DMMC ON/OFF: 0-on, 1-off - индицирует включение режима ДММЦ.
                D6 #BFFD LineNumberV8: 0-high, 1-low - индицирует отображение нижней или верхней половины экранной области в режиме удвоенного вертикального разрешения.
                D7 #BFFD Screen=1, Border=0 - индицирует прохождение луча по экранной области.            }
                */
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

#pragma clang diagnostic pop
