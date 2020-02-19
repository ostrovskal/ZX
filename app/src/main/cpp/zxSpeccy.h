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
#include "zxDevs.h"

struct ZX_MACHINE {
    struct ZX_TSTATE { int up, lp, rp, dp; };
    // стейты, в зависимости от размера границы
    ZX_TSTATE ts[4];
    // задержки стейтов для пикселей экрана
    int tsDelay[8];
    // всего стейтов на кадр
    uint32_t tsTotal;
    // первый цикл кадра
    uint32_t tsFirst;
    // частота звукового процессора
    uint32_t ayClock;
    // частота процессора
    uint32_t cpuClock;
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

class zxSpeccy {
    friend class zxFormats;
public:
    zxSpeccy();
    ~zxSpeccy();

    // запись в порт
    void writePort(uint8_t A0A7, uint8_t A8A15, uint8_t val);

    // загрузка
    bool load(const char* path, int type);

    // запись
    bool save(const char* path, int type);

    // обновление устройств
    void update(int filter);

    // сброс
    void reset();

    // смена модели памяти
    void changeModel(uint8_t _new);

    // выполнение
    int execute();

    // проверить на срабатывании ловушки
    bool checkBPs(uint16_t address, uint8_t flg);

    // быстрая установка ловушки в коде
    void quickBP(uint16_t address);

    // выполнение при трассировке
    void stepDebug();

    // выполнение в обычном режиме
    int stepCPU(bool allow_int);

    // быстрое сохранение
    void quickSave();

    // операции с диском
    int diskOperation(int num, int ops, const char* path);

    // установка/получение имени текущей программы
    const char *programName(const char *name);

    // чтение из порта
    uint8_t readPort(uint8_t A0A7, uint8_t A8A15);

    // быстрая проверка на ловушку
    BREAK_POINT* quickCheckBPs(uint16_t address, uint8_t flg);

    // модель памяти
    static uint8_t* _MODEL;

    // статус
    static uint8_t* _STATE;

    // адрес возврата
    static uint16_t* _CALL;

    // текущая машина
    static ZX_MACHINE* machine;

    // начало инструкции
    static uint16_t PC;

    // буфер ОЗУ
    uint8_t *RAMbs;

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

    // пауза между загрузкой блоков
    int pauseBetweenTapeBlocks;

    int getAddressCpuReg(const char *value);

    // нажата/отжата кнопка на клаве
    int updateKeys(int key, int act) { return devs[DEV_KEYB]->update(key | (act << 7)); }

protected:
    // восстановление состояния
    bool restoreState(const char *path);

    // сохранение состояния
    bool saveState(const char* path);

    // перехватчик
    void trap();

    // добавление устройства
    void addDev(zxDev *dev);

    // имя проги
    std::string name;

    // список доступных устройств
    zxDev* devs[DEV_COUNT];

    // список устройств на чтение/запись
    zxDev* access_devs[16];

    // карта устройств на чтение/запись
    uint8_t map_devs[65536 * 2];

    // цепочки адресов устройств(до 8-и), которые прошли тест на чтение/запись в порт
    zxDev* cache_devs[512][9];
};
