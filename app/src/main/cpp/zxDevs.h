//
// Created by Sergey on 18.02.2020.
//

#pragma once

#include "zxCommon.h"

enum DEV_ACCESS { ACCESS_NONE = 0, ACCESS_WRITE, ACCESS_READ };
enum DEVS {
    DEV_MEM, DEV_ULA, DEV_KEYB, DEV_JOY, DEV_MOUSE, DEV_BEEPER, DEV_AY, DEV_YM, DEV_COVOX, DEV_DISK, DEV_TAPE, DEV_COUNT
};

class zxDev {
public:
    zxDev() {}
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const { return false; }
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const { return false; }
    // открыть
    virtual bool    open(uint8_t* ptr, size_t size, int type) { return false; }
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) {  }
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) {  }
    // восстановление/сохранение состояния
    virtual uint8_t* state(uint8_t* ptr, bool restore) { return ptr; }
    // сохранить
    virtual uint8_t* save(int type) { return nullptr; }
    // сброс устройства
    virtual void    reset() { }
    // обновление
    virtual int     update(int param = 0) { return 0; }
    // тип устройства
    virtual int     type() const { return 0; }
    // доступ к портам
    virtual int     access() const { return ACCESS_NONE; }
};

class zxDevUla : public zxDev {
public:
    zxDevUla();
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override { return (port & 0xFF) == 0xFF; }
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override { return !(port & 1) || !(port & 0x8002); }
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override;
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override { *ret = attr; }
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int param = 0) override;
    // тип устройства
    virtual int     type() const override { return DEV_ULA; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ | ACCESS_WRITE; }
	// количество тактов
	static uint32_t* _TICK;
	// количество тактов в кадре
	static uint32_t _ftick;
protected:
    void updateCPU(int todo, bool interrupt);
    uint8_t attr;
    uint8_t* _FE, *_VID;
    uint8_t* VIDEO, *ATTR;
    int sizeBorder, colorBorder;
    int deltaTSTATE, stateUP, stateDP, stateLP, stateRP;
    // параметры мерцания
    int blink;
};

class zxDevMem : public zxDev {
public:
    zxDevMem();
    virtual ~zxDevMem();
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override { return !(port & 0x8002); }
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override;
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int param = 0) override;
    // тип устройства
    virtual int     type() const override { return DEV_MEM; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_WRITE; }
    // восстановление/сохранение состояния
    virtual uint8_t* state(uint8_t* ptr, bool restore) override;

    // страницы памяти
    static uint8_t* memPAGES[4];
    // буфер ПЗУ - начальный/конечный
    static uint8_t *ROMb, *ROMe;
protected:
    // признак блокировки порта 7FFD
    bool mode48K() const { return (*_7FFD & 32) != 0; }
    uint8_t *_7FFD, *_1FFD, *_RAM, *_ROM, *ROMtr;
    uint8_t* romPAGES[4];
};

class zxDevJoy : public zxDev {
public:
    zxDevJoy();
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override;
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override { *ret = *kempston; }
    // тип устройства
    virtual int     type() const override { return DEV_JOY; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ; }
protected:
    uint8_t* kempston;
};

class zxDevMouse : public zxDev {
public:
    zxDevMouse() { reset(); }
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override;
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override;
    // тип устройства
    virtual int     type() const override { return DEV_MOUSE; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ; }
    // сброс устройства
    virtual void    reset() override;
};

class zxDevKeyboard : public zxDev {
public:
    zxDevKeyboard() { reset(); }
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override { return !(port & 1); }
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override;
    // тип устройства
    virtual int     type() const override { return DEV_KEYB; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ; }
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int key) override;
protected:
	// старое значение кнопок джойстика
	uint8_t joyButtons;
};

class zxFDD;
class zxDevBeta128: public zxDev {
public:
    enum FDD_REQ { R_NONE, R_DRQ = 0x40, R_INTRQ = 0x80 };
    enum { C_WTRK = 0xF0, C_RTRK = 0xE0, C_RSEC = 0x80, C_WSEC = 0xA0, C_RADR = 0xC0, C_INTERRUPT = 0xD0 };
    enum FDD_STATUS {
        ST_BUSY	= 0x01, ST_INDEX = 0x02, ST_DRQ	= 0x02, ST_TRK00 = 0x04,
        ST_LOST	= 0x04, ST_CRCERR = 0x08, ST_NOT_SEC = 0x10, ST_SEEKERR	= 0x10,
        ST_RECORDT = 0x20, ST_HEADL = 0x20, /*ST_WRFAULT = 0x20, */ST_WRITEP = 0x40, ST_NOTRDY = 0x80
    };
    enum FDD_STATE {
        S_IDLE, S_WAIT, S_PREPARE_CMD, S_CMD_RW, S_FIND_SEC,
        S_READ, S_WRSEC, S_WRITE, S_WRTRACK, S_WR_TRACK_DATA, S_TYPE1_CMD,
        S_STEP, S_SEEKSTART, S_SEEK, S_VERIFY
    };
    enum FDD_CMD {
        CB_SEEK_VERIFY = 0x04, CB_SEEK_TRKUPD = 0x10, CB_SEEK_DIR = 0x20, CB_SYS_HLT = 0x08, CB_WRITE_DEL = 0x01,
        CB_DELAY = 0x04, CB_MULTIPLE = 0x10, CB_RESET = 0x04
    };
    zxDevBeta128();
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override;
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override;
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override;
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override;
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int param = 0) override;
    // тип устройства
    virtual int     type() const override { return DEV_DISK; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ | ACCESS_WRITE; }
    // восстановление/сохранение состояния
    virtual uint8_t* state(uint8_t* ptr, bool restore) override;
    // монтировать образ
    virtual bool    open(uint8_t* ptr, size_t size, int type) override;
    // сохранение диска
    virtual uint8_t* save(int type) override;
    // перехват команд
    void trap(uint16_t pc);
    // извлечь
    void eject(int num);
    // установка/чтение режима защиты записи
    int is_readonly(int num, int write = -1);
    // прочитать содержимое сектора
    int read_sector(int num, int sec);
protected:
    // выполнение
    void exec();
    // чтение первого байта
    void read_byte();
    // имем сектор на дорожке
    void find_sec();
    // признак готовности
    bool ready();
    // загрузка трека
    void load();
    // получение импульса
    void get_index(int s_next);
    // вычисление КК
    uint16_t CRC(uint8_t * src, int size) const;
    // вычисление КК
    uint16_t CRC(uint8_t v, uint16_t prev = 0xcdb4) const;
private:
    // процедуры обработки команд
    void cmdReadWrite();
    void cmdWriteTrackData();
    void cmdType1();
    void cmdStep();
    void cmdWrite();
    void cmdPrepareRW();
    void cmdPrepareSec();
    void cmdRead();
    void cmdWriteSector();
    void cmdWriteTrack();
    void cmdSeek();
    void cmdVerify();
    void log_to_data(bool is_text, const char* title, int trk, int sec, int head);
    // текущая операция чтения/записи
    uint8_t ops;
    // текущий дисковод
    uint8_t nfdd;
    // следующее время/время ожидания сектора
    uint32_t time, next, wait_drq;
    // напраление
    int8_t direction;
    // начальный адрес вычисления КК
    uint8_t* start_crc;
    // адрес чтения/записи
    uint8_t* _rwptr;
    uint8_t* rwptr;
    // длина буфера чтения/записи
    uint16_t rwlen;
    // текущее состояние
    uint8_t	states;
    // состояние порта 0xFF
    uint8_t	rqs;
    // текущий КК
    uint16_t crc;
    // текущий дисковод
    zxFDD* fdd;
};

constexpr uint32_t TICK_FF                      = 6;
constexpr uint32_t TICK_F                       = (1 << TICK_FF);
constexpr uint32_t MULT_C_1                     = 14;

class zxDevSound : public zxDev {
    friend class zxSoundMixer;
public:
    enum {
        AFINE, ACOARSE, BFINE, BCOARSE, CFINE, CCOARSE, NOISEPER, ENABLE, AVOL,
        BVOL, CVOL, EFINE, ECOARSE, ESHAPE
    };
    struct CHANNEL {
        // левый канал
        uint16_t left;
        // правый канал
        uint16_t right;
    };

    union SNDSAMPLE {
        // левый/правый каналы в младшем/старшем слове
        uint32_t sample;
        // или правый/левый каналы раздельно
        CHANNEL ch;
    };

    zxDevSound();
    virtual void frameStart(uint32_t tacts) {
        auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
        base_tick = tick - endtick;
    }
    virtual void frameEnd(uint32_t tacts) {
        auto endtick = (uint32_t)((tacts * (uint64_t)sample_rate * TICK_F) / clock_rate);
        flush(base_tick + endtick);
    }
    // сброс устройства
    virtual void    reset() override {  }
    // обновление
    virtual int     update(int param = 0) override;
protected:
    // сброс буфера
    virtual void    resetData() { dstpos = buffer; }
    // адрес звуковых данных
    virtual void*   ptrData() { return buffer; }
    // размер готовых данных
    virtual int     sizeData() { return (int)(dstpos - buffer) * sizeof(SNDSAMPLE); }

    void            updateData(uint32_t tact, uint32_t l, uint32_t r);
    uint32_t mix_l, mix_r;
    uint32_t clock_rate, sample_rate;
    SNDSAMPLE buffer[16384];
    SNDSAMPLE* dstpos;
private:
    void flush(uint32_t endtick);
    uint32_t tick, base_tick;
    uint32_t s1_l, s1_r;
    uint32_t s2_l, s2_r;
};

class zxDevBeeper: public zxDevSound {
public:
    zxDevBeeper() : volSpk(12800), volMic(3200) { }
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override { return !(port & 1); }
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override;
    // обновление
    virtual int     update(int param = 0) override;
    // тип устройства
    virtual int     type() const override { return DEV_BEEPER; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_WRITE; }
protected:
    int volSpk, volMic;
};

class zxDevCovox: public zxDevSound {
public:
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override { return false; }
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override { return false; }
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override {  }
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override { }
    // сброс устройства
    virtual void    reset() override { }
    // обновление
    virtual int     update(int param = 0) override { return 0; }
    // тип устройства
    virtual int     type() const override { return DEV_COVOX; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_WRITE; }
};

class zxDevAY: public zxDevSound {
public:
    zxDevAY();
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override;
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override;
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override;
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override;
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int param = 0) override;
    // тип устройства
    virtual int     type() const override { return DEV_AY; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ | ACCESS_WRITE; }

    virtual void    frameStart(uint32_t tacts) override {
        t = tacts * chip_clock_rate / system_clock_rate;
        zxDevSound::frameStart(t);
    }

    virtual void    frameEnd(uint32_t tacts) override {
        auto end_chip_tick = ((passed_clk_ticks + tacts) * chip_clock_rate) / system_clock_rate;
        flush((uint32_t)(end_chip_tick - passed_chip_ticks));
        zxDevSound::frameEnd(t);
        passed_clk_ticks += tacts; passed_chip_ticks += t;
    }
protected:
    void select(uint8_t v);
    void _write(uint32_t timestamp, uint8_t val);
private:
    void flush(uint32_t chiptick);
    void applyRegs(uint32_t timestamp = 0);

    int denv;
    uint32_t t, ta, tb, tc, tn, te, env;
    uint32_t bitA, bitB, bitC, bitN, ns;
    uint32_t bit0, bit1, bit2, bit3, bit4, bit5;
    uint32_t ea, eb, ec, va, vb, vc;
    uint32_t fa, fb, fc, fn, fe;
    uint32_t mult_const;
    uint32_t chip_clock_rate, system_clock_rate;
    uint64_t passed_chip_ticks, passed_clk_ticks;
    uint32_t vols[6][32];
};

class zxDevYM: public zxDevSound {
public:
    enum { AY_SAMPLERS = 8000, AY_ENV_CONT = 8, AY_ENV_ATTACK = 4, AY_ENV_ALT = 2, AY_ENV_HOLD = 1, STEREO_BUF_SIZE = 1024 };
    struct AY_SAMPLER {
        uint32_t tstates;
        uint16_t ofs;
        uint8_t reg, val;
    };
    zxDevYM() : tsmax(0), sound_stereo_ay(0), frequency(44100) { reset(); }
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override;
    // проверка на порт на запись
    virtual bool    checkWrite(uint16_t port) const override;
    // запись в порт
    virtual void    write(uint16_t port, uint8_t val) override;
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override;
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int param = 0) override;
    // тип устройства
    virtual int     type() const override { return DEV_YM; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ | ACCESS_WRITE; }

    virtual void    frameStart(uint32_t tacts) override { }
    virtual void    frameEnd(uint32_t tacts) override { }
protected:
    virtual void    resetData() override { countSamplers = 0; }
    virtual void*   ptrData() override;
    virtual int     sizeData() override { return sndBufSize * 2 * sizeof(short); }
    void            _write(uint8_t val, uint32_t tact);
    // режим стерео для AM(ABC = 1, ACB = 0)
    int sound_stereo_ay;
    // циклов на кадр
    uint32_t tsmax;
    // частота звука
    int frequency;
    // массив сэмплов
    AY_SAMPLER samplers[AY_SAMPLERS];
    // текущее количество сэмплов
    int countSamplers;
    // размер звукового буфера
    int sndBufSize;
    // левый/правый буфер для стерео режима
    int rstereobuf_l[STEREO_BUF_SIZE], rstereobuf_r[STEREO_BUF_SIZE];
    int rstereopos, rchan1pos, rchan2pos, rchan3pos;
    // параметры 3-х канального AY
    uint32_t toneTick[3], toneHigh[3], noiseTick;
    uint32_t tonePeriod[3], noisePeriod, envPeriod;
    uint32_t envIntTick, envTick, tickAY;
    uint32_t toneSubCycles, envSubCycles;
    uint8_t  ayRegs[16];
    uint32_t toneLevels[16];
    // частота звукового процессора
    uint32_t clockAY;
};

class zxDevTape : public zxDevSound {
    friend class zxSpeccy;
public:
    struct TAPE_BLOCK {
        // тип блока
        uint8_t type;
        // тип импульса(пилот, после пилота, данные, последний байт, пауза)
        uint32_t type_impulse;
        // данные
        uint8_t* data;
        // длина данных
        uint32_t data_size;
        // длины импульсов
        uint32_t pilot_t, s1_t, s2_t;
        uint32_t zero_t, one_t, pilot_len;
        uint32_t pause, last;
    };
    zxDevTape();
    virtual ~zxDevTape() { closeTape(); }
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override { return !(port & 1); }
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override { *ret |= tapeBit(); }
    // сброс устройства
    virtual void    reset() override;
    // обновление
    virtual int     update(int param = 0) override { return 0; }
    // тип устройства
    virtual int     type() const override { return DEV_TAPE; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ; }
    // восстановление/сохранение состояния
    virtual uint8_t* state(uint8_t* ptr, bool restore) override;
    // открыть
    virtual bool open(uint8_t* ptr, size_t size, int type) override;

    void blockData(int index, uint16_t *data);
    void trapSave();
    void trapLoad();
    void startTape();
    void stopTape();
    void closeTape();
    void makeBlock( uint8_t* data, uint32_t size, uint32_t pilot_t, uint32_t s1_t, uint32_t s2_t, uint32_t zero_t, uint32_t one_t,
                    uint32_t pilot_len, uint32_t pause, uint8_t last = 8);
protected:
    uint8_t tapeBit();
    uint32_t getImpulse();
    // массив блоков
    TAPE_BLOCK* blocks[128];
    // всего блоков
    int countBlocks;
    // текущий блок
    int currentBlock;
    // такты для следующих импульсов
    uint64_t edge_change;
    // значение текущего бита(64/0)
    uint8_t tape_bit;
};

