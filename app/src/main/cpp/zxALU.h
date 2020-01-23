//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include "zxStks.h"
#include "zxAssembler.h"
#include "zxDebugger.h"
#include "zxTape.h"
#include "zxSound.h"
#include "zxDisk.h"
#include "zxGPU.h"

struct ZX_MACHINE {
    struct ZX_TSTATE { int up, lp, rp, dp; };
    struct ZX_PORT { int msk, val, num; };
    // стейты, в зависимости от размера границы
    ZX_TSTATE ts[4];
    // несколько портов
    ZX_PORT ports[8];
    // задержки стейтов для пикселей экрана
    int tsDelay[8];
    // номера страниц ПЗУ
    int romPages[4];
    // всего стейтов на кадр
    int tsTotal;
    // частота процессора
    u_long cpuClock;
    // всего страниц ОЗУ
    int ramPages;
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

class zxALU {
public:
    zxALU();
    ~zxALU();

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
    bool changeModel(uint8_t _new, uint8_t  _old, bool isReset);

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
    static uint32_t _TICK;

    // адрес возврата
    static uint16_t* _CALL;

    // текущая машина
    ZX_MACHINE* machine;

    // страницы ПЗУ
    uint8_t* PAGE_ROM[4];

    // буфер ОЗУ
    uint8_t* RAMs;

    // буфер ПЗУ
    uint8_t* ROMs;

    // 8 страница скорпиона
    uint8_t *page8;

    // процессор
    zxCPU* cpu;

    // видеокарта
    zxGPU* gpu;

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

protected:

    // загрузка состояния
    bool openState(const char *path);

    // загрузка файла в формате Z80
    bool openZ80(const char *path);

    // сохранение состояния
    bool saveState(const char* path);

    // сохранение файла в формате Z80
    bool saveZ80(const char *path);

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

    // остаток TSTATE
    int deltaTSTATE;

    // старое значение кнопок джойстика
    uint8_t joyOldButtons;

    // параметры мерцания
    uint32_t blink;

    // цвет и размер границы
    uint32_t sizeBorder, colorBorder;

    // звуковая карта
    zxSound* snd;

    // дисковод
    zxDisk* disk;

    // имя проги
    std::string name;
};
