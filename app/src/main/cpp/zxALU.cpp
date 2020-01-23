//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxALU.h"
#include "stkMnemonic.h"
#include "zxDA.h"
#include "zxSound.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

// текущий счетчик инструкций
uint32_t zxALU::_TICK(0);

uint16_t* zxALU::_CALL(nullptr);
uint16_t zxALU::PC(0);

// страницы памяти
uint8_t* zxALU::memPAGES[4];

// STATE
uint8_t* zxALU::_STATE(nullptr);

ZX_MACHINE machines[] = {
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            {   { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 0, 10, 10, 10 },
            69888, 3500000, 8, "KOMPANION" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            {   { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 1, 10, 10, 10 },
            69888, 3500000, 8, "SINCLER 48K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            {   { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 2, 10, 10, 10 },
            69888, 3500000, 8, "SINCLER 48K 2006" },
        {   {   { 63 * 228, 8 + 24, 44 + 24, 56 * 228 }, { 47 * 228, 8 + 16, 44 + 16, 40 * 228 },
                { 31 * 228, 8 + 8, 44 + 8, 24 * 228 }, { 15 * 228, 8, 44, 8 * 228 } },
            {   { 0, 0, 0x7FFD }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 3, 4, 10, 10 },
            70908, 3546900, 8, "SINCLER 128K" },
        {   {   { 80 * 224, 8 + 24, 40 + 24, 48 * 224 }, { 64 * 224, 8 + 16, 40 + 16, 32 * 224 },
                { 48 * 224, 8 + 8, 40 + 8, 16 * 224 }, { 32 * 224, 8, 40, 0 * 224 } },
            {   { 0, 0, 0x7FFD }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 5, 6, 10, 10 },
            71680, 3575000, 8, "PENTAGON 128K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            {   { 0, 0, 0x7FFD }, { 0, 0, 0x1FFD }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 7, 8, 9, 10 },
            69888, 3546900, 16, "SCORPION 256K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            {   { 0, 0, 0x7FFD }, { 0, 0, 0x1FFD }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 },
                { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 }, { 0xFFFF, 0, 0 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            { 11, 12, 13, 14 },
            69888, 3546900, 8, "PROFI" }
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

zxALU::zxALU() : pauseBetweenTapeBlocks(0), joyOldButtons(0), deltaTSTATE(0), _FF(255), colorBorder(7),
                 blink(0), sizeBorder(0), machine(nullptr),
                 cpu(nullptr), snd(nullptr), tape(nullptr), disk(nullptr), gpu(nullptr), ROMs(nullptr) {
    RAMs = new uint8_t[ZX_TOTAL_RAM];

    // инициализация системы
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE]; *_STATE = 0;
    _KEMPSTON		= &opts[ZX_PROP_VALUES_KEMPSTON];
    _1FFD    		= &opts[PORT_1FFD];
    _7FFD    		= &opts[PORT_7FFD];
    _FE             = &opts[PORT_FE];

    zxALU::_RAM     = &opts[RAM];
    zxALU::_VID     = &opts[VID];
    zxALU::_ROM     = &opts[ROM];
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

bool zxALU::load(const char *path, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_TRD:
            ret = disk->openTRD(opts[ZX_PROP_ACTIVE_DISK], path);
            break;
        case ZX_CMD_IO_WAVE:
            ret = tape->openWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = tape->openTAP(path);
            if(ret && opts[ZX_PROP_TRAP_TAPE]) ret = openZ80("tapLoader.zx");
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

bool zxALU::openZ80(const char *path) {
    size_t sz;
    HEAD1_Z80* head1 = nullptr;
    HEAD2_Z80* head2 = nullptr;
    HEAD3_Z80* head3 = nullptr;
    uint8_t model = MODEL_48;
    int version;
    int pages = 3;

    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[262144], true, &sz);
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
        LOG_INFO("version:%i length:%i model:%i pages:%i mode:%i", version, length, model, pages, mode);
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
    changeModel(model, *_MODEL, isZ80);
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
    static uint8_t models[] = { 0, 0, 0, 4, 9, 10, 4 };
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

void zxALU::updateProps(int filter) {
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
        machine = &machines[_new];
        for (int i = 0; i < 4; i++) PAGE_ROM[i] = &ROMs[(machine->romPages[i] << 14)];
    }
    signalRESET(resetTape);
    return result;
}

void zxALU::signalRESET(bool resetTape) {
    LOG_INFO("RESET %i", *_MODEL);
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
    *cpu->_SP = 65534; *_FE = 0b11100111; *_VID = 5; *_RAM = 7;
    *_7FFD = 0b00000111;
    memPAGES[1] = &RAMs[5 << 14]; memPAGES[2] = &RAMs[2 << 14];
    // сброс состояния с сохранением статуса отладчика
    *_STATE &= ZX_DEBUG;
    // установка страниц
    setPages();
    // установка имени программы
    programName("BASIC");
    if(*_MODEL == MODEL_SCORPION) {
        memcpy(&RAMs[8 << 14], page8, 16384);
    }
    // NMI
//    cpu->signalNMI();
}

void zxALU::setPages() {
    memPAGES[0] = checkSTATE(ZX_TRDOS) ? &ROMs[ZX_ROM_TRDOS] : (*_ROM == 100 ? &RAMs[0] : PAGE_ROM[*_ROM]);
    memPAGES[3] = &RAMs[*_RAM << 14];
    pageVRAM    = &RAMs[*_VID << 14];
    pageATTRIB  = pageVRAM + 6144;
}

void zxALU::execute() {
    // отображение экрана, воспроизведение звука
    updateFrame();
    gpu->updateFrame();
    snd->update();
}

void zxALU::stepDebug() {
    PC = *cpu->_PC;
    // убрать отладчик - чтобы не сработала точка останова
    modifySTATE(0, ZX_DEBUG | ZX_HALT);
    // перехват системных процедур
    trap();
    _TICK += cpu->step();
    modifySTATE(ZX_DEBUG, 0);
}

int zxALU::step(bool interrupt) {
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
    // перехват системных процедур
    trap();
    // выполнение инструкции процессора
    return cpu->step();
}

void zxALU::updateCPU(int todo, bool interrupt) {
    int ticks;
    todo += deltaTSTATE;
    if (interrupt) {
        if (deltaTSTATE > 0) {
            ticks = step(false);
            todo -= ticks;
            _TICK += ticks;
        }
        ticks = step(true);
        todo -= ticks;
        _TICK += ticks;
    }
    while(todo > 3) {
        ticks = step(false);
        todo -= ticks;
        _TICK += ticks;
    }
    deltaTSTATE = todo;
}

void zxALU::updateFrame() {
    static uint32_t frames[2] = { 0, 0 };

    uint32_t line, i, tmp, c;

    if(pauseBetweenTapeBlocks > 0) {
        pauseBetweenTapeBlocks--;
        if(pauseBetweenTapeBlocks == 0)
            *_STATE &= ~ZX_PAUSE;
    }

    auto dest = gpu->frameBuffer;
    if(!dest) return;

    _TICK = 0;

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
            *dest++ = c;
            *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    // screen
    for(line = 0 ; line < 192; line++) {
        updateCPU(stateLP, false);
        for(i = 0 ; i < szBorder; i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c;
            *dest++ = c;
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
        *_STATE &= ~ZX_SCR;
        for(i = 0 ; i < szBorder; i++) {
            updateCPU(1, false);
            c = colours[colorBorder];
            *dest++ = c;
            *dest++ = c;
        }
        updateCPU(stateRP, false);
    }
    for(line = 0 ; line < sizeBorder; line++) {
        updateCPU(stateLP, false);
        for (i = 0; i < (sizeBorder + 128); i++) {
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
        // установить глобальную остановку системы
        opts[ZX_PROP_EXECUTE] = 0;
        // сообщение для выхода - обновить отладчик
        *_STATE |= ZX_BP;
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

void zxALU::trap() {
    if(PC < 16384) {
        // активность TR DOS
        if (!checkSTATE(ZX_TRDOS)) {
            if (PC >= 15616 && PC <= 15871) {
                if((*_MODEL >= MODEL_128 && *_ROM != 0) || *_MODEL < MODEL_128) {
                    *_STATE |= ZX_TRDOS;
                    setPages();
                }
            }
            bool success = false;
            if (PC == 1218) success = tape->trapSave();
            else if (PC == 1366 || PC == 1378) {
                //saveZ80("tapLoader.zx");
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

void zxALU::writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val) {
    auto port = (uint16_t)(A0A7 | (A8A15 << 8));
    if(checkSTATE(ZX_DEBUG)) { if(checkBPs(port, ZX_BP_WPORT)) return; }
    if(*_MODEL == MODEL_SCORPION && (port & 0xD027) == 0x1025) {
        // A0, A2, A5, A12 = 1; A1, A14, A15 = 0
        write1FFD(val);
        LOG_DEBUG("1FFD (%X%X(%i) ROM: %i RAM: %i VID: %i) PC: %i", A8A15, A0A7, val, *_ROM, *_RAM, *_VID, PC);
    } else if((port & 0xD027) == 0x5025) {
        write7FFD(val);
        LOG_DEBUG("7FFD (%X%X(%i) ROM: %i RAM: %i VID: %i) PC: %i", A8A15, A0A7, val, *_ROM, *_RAM, *_VID, PC);
    } else if(checkSTATE(ZX_TRDOS) && (A0A7 == 0x1F || A0A7 == 0x3F || A0A7 == 0x5F || A0A7 == 0x7F || A0A7 == 0xFF)) {
        disk->writePort(A0A7, val);
    } else if (port == 0xBFFD) {
        // BFFD
        // записываем значение в текущий регистр
        auto reg = opts[AY_REG];
        opts[reg + AY_AFINE] = val;
        snd->ayWrite(reg, val);
        //LOG_DEBUG("AY_VAL in:%i", val);
    } else if(port == 0xFFFD) {
        // FFFD
        // устанавливаем текущий регистр
        opts[AY_REG] = (uint8_t)(val & 15);
        //LOG_DEBUG("AY_REG %i", val);
    } else if (A0A7 == 0xFE) {
        // 0, 1, 2 - бордер, 3 MIC - при записи, 4 - бипер
        *_FE = val;
        colorBorder = val & 7U;
        tape->writePort(val);
    } else {
        LOG_DEBUG("UNKNOWN PORT (%02X%02X(%i) - PC: %i", A8A15, A0A7, val, PC);
    }
}

void zxALU::write1FFD(uint8_t val) {
    // 0 -> 1 - RAM0(!!), 0 - see bit 1 etc
    // 1 -> 1 - ROM 2, 0 - ROM from 0x7FFD
    // 4 -> 1 - RAM SCORPION, 0 - from 0x7FFD
    *_1FFD = val;
    if (val & 1) *_ROM = 100;
    else {
        if (val & 2) *_ROM = 2;
        else *_ROM = (uint8_t) ((*_7FFD & 16) >> 4);
    }
    *_RAM = (uint8_t) ((*_7FFD & 7) + (uint8_t) ((val & 16) >> 1));
    setPages();
}

void zxALU::write7FFD(uint8_t val) {
    // 1, 2, 4 - страница 0-7
    // 8 - экран 5/7
    // 16 - ПЗУ 0 - 128К 1 - 48К
    // 32 - блокировка
    if(*_MODEL >= MODEL_128) {
        if (*_7FFD & 32) return;
        *_7FFD = val;
        *_VID = (uint8_t) ((val & 8) ? 7 : 5);
        *_RAM = (uint8_t) (val & 7);
        if (*_MODEL == MODEL_SCORPION) {
            if (!(*_1FFD & 2)) *_ROM = (uint8_t) ((val & 16) >> 4);
            *_RAM += (uint8_t) ((*_1FFD & 16) >> 1);
        } else {
            *_ROM = (uint8_t) ((val & 16) >> 4);
        }
        setPages();
    }
}

uint8_t zxALU::readPort(uint8_t A0A7, uint8_t A8A15) {
    uint8_t ret = _FF;
    auto port = (uint16_t)(A0A7 | (A8A15 << 8));
    if(checkSTATE(ZX_DEBUG)) { if(checkBPs(port, ZX_BP_RPORT)) return ret; }
    if(checkSTATE(ZX_TRDOS) && (A0A7 == 0x1F || A0A7 == 0x3F || A0A7 == 0x5F || A0A7 == 0x7F || A0A7 == 0xFF))
        ret = disk->readPort(A0A7);
    else if(A0A7 == 0x1F) {
        ret = *_KEMPSTON;
    } else if(A0A7 == 0xFE) {
        // 0,1,2,3,4 - клавиши полуряда, 6 - EAR, 5,7 - не используется
        A0A7 = 31;
        for (int i = 0; i < 8; i++) {
            if (A8A15 & (1 << i)) continue;
            A0A7 &= opts[i + ZX_PROP_VALUES_SEMI_ROW];
        }
        ret = (A0A7 | tape->readPort());
    } else if((port & 0xD027) == 0x5025) {
        ret = *_7FFD;
        LOG_DEBUG("7FFD (%X%X) %i PC %i", A8A15, A0A7, ret, PC);
    } else if((port & 0xD027) == 0x1025) {
            ret = *_1FFD;
            LOG_DEBUG("1FFD (%X%X) %i PC %i", A8A15, A0A7, ret, PC);
    } else if(port == 0xFFFD) {
        // звук AY
        auto reg = opts[AY_REG];
        ret = opts[reg + AY_AFINE];
//            LOG_DEBUG("AY r:%i v:%i", reg, ret);
    } else if(A0A7 == 0xFF) {
        // FF
    } else {
            LOG_DEBUG("UNKNOWN PORT %X%X PC: %i", A8A15, A0A7, PC);
    }
    return ret;
}

#pragma clang diagnostic pop
