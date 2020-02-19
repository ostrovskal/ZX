//
// Created by Сергей on 21.11.2019.
//

#include <ctime>
#include "zxCommon.h"
#include "zxSpeccy.h"
#include "zxDA.h"
#include "zxSound.h"
#include "zxFormats.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

uint16_t zxSpeccy::PC(0);

// текущая машина
uint8_t* zxSpeccy::_MODEL(nullptr);
// возврат из процедуры
uint16_t* zxSpeccy::_CALL(nullptr);
// STATE
uint8_t* zxSpeccy::_STATE(nullptr);
// параметры текущей машины
ZX_MACHINE* zxSpeccy::machine(nullptr);

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

zxSpeccy::zxSpeccy() : pauseBetweenTapeBlocks(0), cpu(nullptr), snd(nullptr), gpu(nullptr) {
    // инициализация системы
    RAMbs           = new uint8_t[ZX_TOTAL_RAM];
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE]; *_STATE = 0;
    _CALL           = (uint16_t*)&opts[CALL0];

    // машина по умолчанию(до загрузки состояния)
    machine = &machines[MODEL_48];

    // создаем устройства
    cpu         = new zxCPU();
    gpu         = new zxGPU();
    snd         = new zxSoundMixer();
    assembler   = new zxAssembler();
    debugger    = new zxDebugger();

    memset(devs, 0, sizeof(devs));
    memset(access_devs, 0, sizeof(access_devs));
    memset(map_devs, 0, sizeof(map_devs));
    memset(cache_devs, 0, sizeof(cache_devs));

    addDev(new zxDevMem());         // w
    addDev(new zxDevUla());         // rw
    addDev(new zxDevKeyboard());    // r
    addDev(new zxDevJoy());         // r
    addDev(new zxDevMouse());       // r
    addDev(new zxDevBeeper());      // w
    addDev(new zxDevCovox());       // w
    addDev(new zxDevAY());          // rw
    addDev(new zxDevYM());          // rw
    addDev(new zxDevBeta128());     // rw
    addDev(new zxDevTape());        // r

    for(uint16_t port = 0; port <= 65535; ++port) {
        uint8_t rdevs(0), wdevs(0);
        for(int d = 0; d < 8; ++d) {
            auto dev = access_devs[d + 0];
            if(dev && dev->checkRead(port)) rdevs |= numBits[d];
            dev = access_devs[d + 8];
            if(dev && dev->checkWrite(port)) wdevs |= numBits[d];
        }
        map_devs[port] = rdevs;
        map_devs[port + 65536] = wdevs;
    }
    for(int i = 0; i < 256; ++i) {
        zxDev** dev = cache_devs[i];
        for(int d = 0; d < 8; ++d) {
            if(i & numBits[d]) {
                *(dev + 256) = access_devs[d + 8];
                *dev++ = access_devs[d];
            }
        }
    }
}

void zxSpeccy::addDev(zxDev *dev) {
    devs[dev->type()] = dev;
    if(dev->access() & ACCESS_READ) {
        auto d = &access_devs[-1];
        while(*++d) { }
        *d = dev;
    }
    if(dev->access() & ACCESS_WRITE) {
        auto d = &access_devs[7];
        while(*++d) { } *d = dev;
    }
}

zxSpeccy::~zxSpeccy() {
    for(int i = 0 ; i < DEV_COUNT; i++) SAFE_DELETE(devs[i]);

    SAFE_DELETE(assembler);
    SAFE_DELETE(debugger);

    SAFE_DELETE(snd);

    SAFE_DELETE(gpu);
    SAFE_DELETE(cpu);

    SAFE_A_DELETE(RAMbs);
}

bool zxSpeccy::load(const char *path, int type) {
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_WAVE:
            ret = zxFormats::openWAV(path);
            break;
        case ZX_CMD_IO_TAPE:
            ret = zxFormats::openTAP(path);
            if(ret && opts[ZX_PROP_TRAP_TAPE] && (opts[PORT_7FFD] & 32)) {
                ret = zxFormats::openZ80("tapLoad48.zx");
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

bool zxSpeccy::restoreState(const char *path) {
    auto ptr = (uint8_t*)zxFile::readFile(path, &TMP_BUF[INDEX_OPEN], false);
    if(!ptr) return false;
    // меняем модель
    changeModel(ptr[MODEL]);
    // грузим базовые параметры
    memcpy(opts, ptr, COUNT_REGS); ptr += COUNT_REGS;
    auto ptrName = (const char*)ptr;
    ptr += strlen(ptrName) + 1;
    // восстановление состояния устройств
    for(int i = 0 ; i < DEV_COUNT; i++) {
        auto ret = devs[i]->state(ptr, true);
        if(!ret) { reset(); return false; }
        ptr = ret;
    }
    // загрузить имя сохраненной проги
    programName(ptrName);
    return true;
}

bool zxSpeccy::saveState(const char* path) {
    auto ptr = TMP_BUF;
    modifySTATE(0, ZX_PAUSE);
    // сохраняем базовые параметры
    ssh_memcpy(&ptr, opts, COUNT_REGS);
    // сохранить имя проги
    ssh_memcpy(&ptr, name.c_str(), (size_t)(name.length() + 1));
    // сохранение состояния устройств
    for(int i = 0 ; i < DEV_COUNT; i++) ptr = devs[i]->state(ptr, false);
    // записываем
    return zxFile::writeFile(path, TMP_BUF, ptr - TMP_BUF, false);
}

bool zxSpeccy::save(const char *path, int type) {
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

void zxSpeccy::update(int filter) {
    gpu->updateProps(opts[ZX_PROP_BORDER_SIZE] << 4, filter);
    for(int i = 0 ; i < DEV_COUNT; i++) devs[i]->update();
}

void zxSpeccy::changeModel(uint8_t _new) {
    auto m = machine;
    machine = &machines[_new];
    if(devs[DEV_MEM]->update(1)) {
        *_MODEL = _new;
        opts[ZX_PROP_MODEL_TYPE] = _new;
    } else machine = m;
    reset();
}

void zxSpeccy::reset() {
    // сброс устройств
    snd->reset();
    for(int i = 0 ; i < DEV_COUNT; i++) devs[i]->reset();
    // очищаем регистры/порты
    ssh_memzero(opts, STATE);
    // очищаем ОЗУ
    ssh_memzero(RAMbs, ZX_TOTAL_RAM);
    // инициализаруем переменные
    *cpu->_SP = 65534;
    // сброс состояния с сохранением статуса отладчика
    *_STATE &= ZX_DEBUG;
    // установка имени программы
    programName("BASIC");
}

int zxSpeccy::execute() {
    // формирование/отображение экрана
    snd->frameStart(0);
    devs[DEV_ULA]->update(1);
    snd->frameEnd(*zxDevUla::_TICK % machine->tsTotal);
    gpu->updateFrame();
    return devs[DEV_KEYB]->update(0);
}

void zxSpeccy::stepDebug() {
    PC = *cpu->_PC;
    // убрать отладчик - чтобы не сработала точка останова
    modifySTATE(0, ZX_DEBUG | ZX_HALT);
    // перехват системных процедур
    *zxDevUla::_TICK += cpu->step();
    trap();
    modifySTATE(ZX_DEBUG, 0);
}

int zxSpeccy::stepCPU(bool interrupt) {
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

void zxSpeccy::trap() {
    auto pc = *cpu->_PC;
    auto state = *_STATE;
    if(pc < 16384) {
        // активность TRDOS
        if (!checkSTATE(ZX_TRDOS)) {
            if (pc >= 15616 && pc <= 15871) {
/*
                if(PC == 15616) {
                    const char* name(nullptr);
                    if(*_MODEL < MODEL_128) name = "trdosLoad48.zx";
                    else if(*_MODEL < MODEL_PENTAGON) name = "trdosLoad128.zx";
                    else return;
                    zxFormats::saveZ80(name);
                }
*/
                if(opts[ROM] == 1) *_STATE |= ZX_TRDOS;
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
            ((zxDevBeta128*)devs[DEV_DISK])->trap(pc);
        }
    } else *_STATE &= ~ZX_TRDOS;
    if(state != *_STATE) devs[DEV_MEM]->update();
}

void zxSpeccy::writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val) {
    uint16_t port = (A8A15 << 8) | A0A7;
    zxDev** dl = cache_devs[map_devs[port + 65536] + 256];
    while(*dl) (*dl++)->write(port, val);
    if(checkSTATE(ZX_DEBUG)) checkBPs(port, ZX_BP_WPORT);
}

uint8_t zxSpeccy::readPort(uint8_t A0A7, uint8_t A8A15) {
    uint16_t port = (A8A15 << 8) | A0A7;
    uint8_t ret(0xFF);
    if(A0A7 == 0xFC) {
        if(A8A15 == 0xFE && *_MODEL == MODEL_KOMPANION) {
            auto k = opts[ZX_PROP_PORT_FEFC];
            if(!(opts[RUS_LAT] & 64)) {
                ret = k;
                opts[ZX_PROP_PORT_FEFC] = 255;
                opts[RUS_LAT] |= 127;
            }
        }
    } else {
        zxDev** dl = cache_devs[map_devs[port]];
        while(*dl) (*dl++)->read(port, &ret);
    }
    if(checkSTATE(ZX_DEBUG)) checkBPs(port, ZX_BP_RPORT);
    return ret;
}

void zxSpeccy::quickBP(uint16_t address) {
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

BREAK_POINT* zxSpeccy::quickCheckBPs(uint16_t address, uint8_t flg) {
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

bool zxSpeccy::checkBPs(uint16_t address, uint8_t flg) {
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

const char *zxSpeccy::programName(const char *nm) {
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

int zxSpeccy::diskOperation(int num, int ops, const char* path) {
    int ret(0);
    auto disk = (zxDevBeta128*)devs[DEV_DISK];
    switch(ops & 7) {
        case ZX_DISK_OPS_GET_READONLY:  ret = disk->is_readonly(num & 3); break;
        case ZX_DISK_OPS_EJECT:         disk->eject(num); break;
        case ZX_DISK_OPS_OPEN:          ret = (int)disk->open(path, num, parseExtension(path)); break;
        case ZX_DISK_OPS_SAVE:          ret = disk->save(path, num, parseExtension(path)); break;
        case ZX_DISK_OPS_SET_READONLY:  ret = disk->is_readonly(num & 3, (num & 128)); break;
        case ZX_DISK_OPS_TRDOS: {
            const char* name(nullptr);
            if (opts[PORT_7FFD] & 32) name = "trdosLoad48.zx";
            else if (*_MODEL < MODEL_PENTAGON) name = "trdosLoad128.zx";
            else break;
            ret = zxFormats::openZ80(name);
            break;
        }
        case ZX_DISK_OPS_RSECTOR:       ret = disk->read_sector(num & 3, (num >> 3) + 1); break;
    }
    return ret;
}

void zxSpeccy::quickSave() {
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

int zxSpeccy::getAddressCpuReg(const char *value) {
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
