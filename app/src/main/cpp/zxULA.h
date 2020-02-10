//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include "zxStks.h"
#include "zxAssembler.h"
#include "zxDebugger.h"
#include "zxTape.h"
#include "zxSound.h"
#include "zxGPU.h"
#include "zxFDD.h"
#include "zxSound.h"

struct ZX_MACHINE {
    struct ZX_TSTATE { int up, lp, rp, dp; };
    // стейты, в зависимости от размера границы
    ZX_TSTATE ts[4];
    // задержки стейтов для пикселей экрана
    int tsDelay[8];
    // всего стейтов на кадр
    long tsTotal;
    // частота звукового процессора
    u_long ayClock;
    // частота процессора
    u_long cpuClock;
    // всего страниц ОЗУ
    int ramPages;
    // страница ПЗУ при старте
    uint8_t startRom;
    // индекс страницы ПЗУ для загрузки
    uint8_t indexRom;
    // всего страниц ПЗУ
    uint8_t totalRom;
    // индекс страницы TRDOS
    uint8_t indexTRDOS;
    // имя
    const char* name;
};

struct BREAK_POINT {
    // начальный адрес
    uint16_t address1;
    // конечный адрес
    uint16_t address2;
    // маска
    uint8_t msk;
    // значение
    uint8_t val;
    // операция со значением
    uint8_t ops;
    // флаги(тип: 0 - исполнение, 1 - запись в память, 2 - чтение из порта, 3 - запись в порт)
    uint8_t flg;
};

class zxULA {
    friend class zxFormats;
public:
    zxULA();
    ~zxULA();

    // Обновление кадра
    void updateFrame();

    // обновление процессора
    void updateCPU(int todo, bool interrupt);

    // запись в порт
    void writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val);

    // загрузка
    bool load(const char* path, int type);

    // запись
    bool save(const char* path, int type);

    // обновление свойств
    void updateProps(int filter);

    // сброс
    void signalRESET(bool resetTape);

    // обновление клавиатуры/джойстика
    int updateKeys(int key, int action);

    // смена модели памяти
    void changeModel(uint8_t _new, bool isReset);

    // выполнение
    void execute();

    // установка актуальных страниц
    void setPages();

    // проверить на срабатывании точки останова
    bool checkBPs(uint16_t address, uint8_t flg);

    // быстрая установка в коде
    void quickBP(uint16_t address);

    // выполнение при трассировке
    void stepDebug();

    // быстрое сохранение
    void quickSave();

    // операции с диском
    int diskOperation(int num, int ops, const char* path);

    // установка/получение имени текущей программы
    const char *programName(const char *name);

    // чтение из порта
    uint8_t readPort(uint8_t A0A7, uint8_t A8A15);

    // быстрая проверка на точку останова
    BREAK_POINT* quickCheckBPs(uint16_t address, uint8_t flg);

    // модель памяти
    uint8_t* _MODEL;

    // видео страницы
    uint8_t *pageVRAM, *pageATTRIB;

    // активные страницы памяти
    static uint8_t *memPAGES[4];

    // статус
    static uint8_t* _STATE;

    // счетчик тактов
    static long _TICK;

    // счетчик тактов в кадре
    static uint32_t frameTick;

    // адрес возврата
    static uint16_t* _CALL;

    // текущая машина
    ZX_MACHINE* machine;

    // страницы ПЗУ
    uint8_t* PAGE_ROM[4];

    // буфер ОЗУ
    uint8_t* RAMs;

    // буфер ПЗУ - начальный/конечный
    uint8_t* ROMb, *ROMe;

    // буфер ПЗУ бета диска
    uint8_t *ROMtr;

    // процессор
    zxCPU* cpu;

    // видеокарта
    zxGPU* gpu;

    // звуковая карта
    zxSoundMixer* snd;

    // лента
    zxTape* tape;

    // ассемблер
    zxAssembler* assembler;

    // отладчик
    zxDebugger* debugger;

    // начало инструкции
    static uint16_t PC;

    // пауза между загрузкой блоков
    int pauseBetweenTapeBlocks;

    int getAddressCpuReg(const char *value);

    jint UpdateSound(uint8_t *buf);

protected:

    // загрузка состояния
    bool restoreState(const char *path);

    // сохранение состояния
    bool saveState(const char* path);

    // исполнение инструкции процессора
    int step(bool allow_int);

    // перехватчик
    void trap();

    void write1FFD(uint8_t val);

    void write7FFD(uint8_t val);

    // содержимое портов
    uint8_t *_KEMPSTON, *_1FFD, *_7FFD, *_FE;

    // страницы
    uint8_t *_RAM, *_VID, *_ROM;

    // текущий атрибут (порт #FF)
    uint8_t _FF;

    // TSTATES
    int stateUP, stateLP, stateRP, stateDP;

    // остаток TSTATES
    int deltaTSTATE;

    // старое значение кнопок джойстика
    uint8_t joyOldButtons;

    // параметры мерцания
    uint32_t blink;

    // цвет и размер границы
    uint32_t sizeBorder, colorBorder;

    // дисковод
    zxVG93* disk;

    // имя проги
    std::string name;
};
