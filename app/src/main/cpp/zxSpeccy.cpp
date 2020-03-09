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
            69888, 14336, 1773400, 3500000, 8, 1, 0, 1, 9, "SINCLER 48K" },
        {   {   { 63 * 228, 8 + 24, 44 + 24, 56 * 228 }, { 47 * 228, 8 + 16, 44 + 16, 40 * 228 },
                { 31 * 228, 8 + 8, 44 + 8, 24 * 228 }, { 15 * 228, 8, 44, 8 * 228 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            70908, 14336, 1773400, 3546900, 8, 0, 1, 2, 9, "SINCLER 128K" },
        {   {   { 80 * 224, 8 + 24, 40 + 24, 48 * 224 }, { 64 * 224, 8 + 16, 40 + 16, 32 * 224 },
                { 48 * 224, 8 + 8, 40 + 8, 16 * 224 }, { 32 * 224, 8, 40, 0 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            71680, 14336, 1792000, 3575000, 32, 0, 3, 2, 9, "PENTAGON 512K" },
        {   {   { 64 * 224, 8 + 24, 40 + 24, 56 * 224 }, { 48 * 224, 8 + 16, 40 + 16, 40 * 224 },
                { 32 * 224, 8 + 8, 40 + 8, 24 * 224 }, { 16 * 224, 8, 40, 8 * 224 } },
            { 6, 5, 4, 3, 2, 1, 0, 0 },
            69888, 14336, 1750000, 3500000, 16, 0, 5, 3, 8, "SCORPION 256K" }
};

zxSpeccy::zxSpeccy() : pauseBetweenTapeBlocks(0), cpu(nullptr), snd(nullptr), gpu(nullptr) {

    // инициализация системы
    zx              = this;
    RAMbs           = new uint8_t[ZX_TOTAL_RAM];
    _MODEL     		= &opts[MODEL];
    _STATE     		= &opts[STATE]; *_STATE = 0;
    _CALL           = (uint16_t*)&opts[CALL0];

    // машина по умолчанию(до загрузки состояния)
    machine = &machines[MODEL_48];

    memset(devs, 0, sizeof(devs));
    memset(access_devs, 0, sizeof(access_devs));
    memset(map_devs, 0, sizeof(map_devs));
    memset(cache_devs, 0, sizeof(cache_devs));

    addDev(new zxDevMem());         // w
    addDev(new zxDevUla());         // rw
    addDev(new zxDevKeyboard());    // r
    addDev(new zxDevJoy());         // r
    addDev(new zxDevMouse());       // r
    addDev(new zxDevTape());        // r
    addDev(new zxDevBeeper());      // w
    addDev(new zxDevCovox());       // w
    addDev(new zxDevAY());          // rw
    addDev(new zxDevYM());          // rw
    addDev(new zxDevBeta128());     // rw

	// создаем устройства
	cpu         = new zxCPU();
	gpu         = new zxGPU();
	snd         = new zxSoundMixer();
	assembler   = new zxAssembler();
	debugger    = new zxDebugger();

	for(int port = 0; port < 65536; port++) {
        uint8_t rdevs(0), wdevs(0);
        for(int d = 0; d < 8; ++d) {
            auto dev = access_devs[d + 0];
            if(dev && dev->checkRead((uint16_t)port)) rdevs |= numBits[d];
            dev = access_devs[d + 8];
            if(dev && dev->checkWrite((uint16_t)port)) wdevs |= numBits[d];
        }
        map_devs[port] = rdevs;
        map_devs[port + 65536] = wdevs;
    }
    for(int i = 0; i < 256; ++i) {
        zxDev** dev = &cache_devs[i * 10];
        for(int d = 0; d < 8; ++d) {
            if(i & numBits[d]) {
                *(dev + 2560) = access_devs[d + 8];
                *dev++ = access_devs[d];
            }
        }
    }
}

void zxSpeccy::addDev(zxDev *dev) {
    devs[dev->type()] = dev;
    if(dev->access() & ACCESS_READ) {
        for(int i = 0 ; i < 8; i++) {
            if(!access_devs[i]) { access_devs[i] = dev; break; }
        }
    }
    if(dev->access() & ACCESS_WRITE) {
        for(int i = 8 ; i < 16; i++) {
            if(!access_devs[i]) { access_devs[i] = dev; break; }
        }
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

bool zxSpeccy::load(const char *path) {
    size_t size;
    uint8_t* ptr;
    auto type = parseExtension(path);
    if(!(ptr = (uint8_t*)zxFile::readFile(path, TMP_BUF, type != ZX_CMD_IO_STATE, &size))) return false;
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_STATE:
            ret = restoreState(ptr);
            break;
        case ZX_CMD_IO_TAP:
        case ZX_CMD_IO_CSW:
        case ZX_CMD_IO_TZX:
            ret = devs[DEV_TAPE]->open(ptr, size, type);
            if(ret && (opts[PORT_7FFD] & 32)) {
                if((ptr = (uint8_t*)zxFile::readFile("tapLoad48.ezx", TMP_BUF, true, &size))) {
                    ret = zxFormats::openSnapshot(ptr, size, ZX_CMD_IO_EZX);
                }
            }
            break;
        case ZX_CMD_IO_EZX:
        case ZX_CMD_IO_Z80:
        case ZX_CMD_IO_SNA:
            ret = zxFormats::openSnapshot(ptr, size, type);
            break;
        case ZX_CMD_IO_TRD:
        case ZX_CMD_IO_SCL:
        case ZX_CMD_IO_FDI:
        case ZX_CMD_IO_UDI:
        case ZX_CMD_IO_TD0:
            ret = devs[DEV_DISK]->open(ptr, size, type);
            break;
    }
    return ret;
}

bool zxSpeccy::save(const char *path) {
    auto type = parseExtension(path);
    uint8_t* ret(nullptr);
    switch(type) {
        case ZX_CMD_IO_STATE:
            ret = saveState();
            break;
        case ZX_CMD_IO_TAP:
        case ZX_CMD_IO_CSW:
        case ZX_CMD_IO_TZX:
            ret = devs[DEV_TAPE]->save(type);
            break;
        case ZX_CMD_IO_TRD:
        case ZX_CMD_IO_SCL:
        case ZX_CMD_IO_FDI:
        case ZX_CMD_IO_UDI:
        case ZX_CMD_IO_TD0:
            ret = devs[DEV_DISK]->save(type);
            break;
        case ZX_CMD_IO_EZX:
        case ZX_CMD_IO_Z80:
        case ZX_CMD_IO_SNA:
            ret = zxFormats::saveSnapshot(type);
            break;
    }
    if(ret) return zxFile::writeFile(path, TMP_BUF, ret - TMP_BUF, type != ZX_CMD_IO_STATE);
    return false;
}

bool zxSpeccy::restoreState(uint8_t* ptr) {
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

uint8_t* zxSpeccy::saveState() {
    auto ptr = TMP_BUF;
    modifySTATE(0, ZX_PAUSE);
    // сохраняем базовые параметры
    ssh_memcpy(&ptr, opts, COUNT_REGS);
    // сохранить имя проги
    ssh_memcpy(&ptr, name.c_str(), (size_t)(name.length() + 1));
    // сохранение состояния устройств
    for(int i = 0 ; i < DEV_COUNT; i++) ptr = devs[i]->state(ptr, false);
    return ptr;
}

void zxSpeccy::update(int filter) {
	snd->update();
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
    // очищаем регистры/порты
    ssh_memzero(opts, STATE);
    // очищаем ОЗУ
    ssh_memzero(RAMbs, ZX_TOTAL_RAM);
    // сброс устройств
    for(int i = 0 ; i < DEV_COUNT; i++) devs[i]->reset();
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
    snd->frameEnd(zxDevUla::_ftick);
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
                    if (opts[PORT_7FFD] & 32) name = "trdosLoad48.ezx";
                    else if (*_MODEL < MODEL_PENTAGON) name = "trdosLoad128.ezx";
                    else break;
                    save(name);
                }
*/
                if(opts[ROM] == 1) *_STATE |= ZX_TRDOS;
            }
            bool success = false;
            if(pc == 0x5E2) ((zxDevTape*)devs[DEV_TAPE])->stopTape();
            else if (pc == 0x4C2) success = tapeOperations(ZX_TAPE_OPS_TRAP_SAVE, 0) != 0;
            else if (/*pc == 0x556 || */pc == 0x562) {
//                zxFormats::saveZ80(*_MODEL >= MODEL_128 ? "tapLoad128.ezx" : "tapLoad48.ezx");
                success = tapeOperations(ZX_TAPE_OPS_TRAP_LOAD, 0) != 0;
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
	zxDev** dl = &cache_devs[map_devs[port + 65536] * 10 + 2560];
	while(*dl) (*dl++)->write(port, val);
    if(checkSTATE(ZX_DEBUG)) checkBPs(port, ZX_BP_WPORT);
}

uint8_t zxSpeccy::readPort(uint8_t A0A7, uint8_t A8A15) {
    uint16_t port = (A8A15 << 8) | A0A7;
    uint8_t ret(0xFF);
	zxDev** dl = &cache_devs[map_devs[port] * 10];
	while(*dl) (*dl++)->read(port, &ret);
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
        case ZX_DISK_OPS_OPEN:          ret = load(path); break;
        case ZX_DISK_OPS_SAVE:          opts[ZX_PROP_JNI_RETURN_VALUE] = (uint8_t)num; ret = save(path); break;
        case ZX_DISK_OPS_SET_READONLY:  ret = disk->is_readonly(num & 3, (num & 128)); break;
        case ZX_DISK_OPS_TRDOS: {
            const char* name(nullptr);
            if (opts[PORT_7FFD] & 32) name = "trdosLoad48.ezx";
            else if (*_MODEL < MODEL_PENTAGON) name = "trdosLoad128.ezx";
            else break;
            ret = load(name);
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
    save(strstr(buf, "SAVERS/"));
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

int zxSpeccy::tapeOperations(int ops, int index) {
    int ret(0);
    auto tape = (zxDevTape*)devs[DEV_TAPE];
    switch(ops) {
        case ZX_TAPE_OPS_COUNT:     ret = tape->countBlocks; break;
        case ZX_TAPE_OPS_RESET:     tape->setCurrentBlock(index); break;
        case ZX_TAPE_OPS_BLOCKC:    tape->blockData(index, (uint16_t*)&opts[ZX_PROP_VALUES_TAPE]); break;
        case ZX_TAPE_OPS_BLOCKP:    tape->blockData(index, (uint16_t*)&opts[ZX_PROP_VALUES_TAPE + 128]); break;
        case ZX_TAPE_OPS_TRAP_LOAD: ret = tape->trapLoad(); break;
        case ZX_TAPE_OPS_TRAP_SAVE: tape->trapSave(); break;
    }
    return ret;
}

#pragma clang diagnostic pop
