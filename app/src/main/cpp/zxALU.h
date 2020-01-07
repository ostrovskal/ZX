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
    bool load(int disk, const char* path, int type);

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

    // запуск/отключение трассера
    void tracer(int start);

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

    // ПЗУ
    uint8_t* ROMS;

    // модель памяти
    uint8_t* _MODEL;

    // адреса текущих страниц
    static uint8_t *pageROM, *pageRAM, *pageVRAM, *pageATTRIB;

    // страницы озу
    static uint8_t* PAGE_RAM[16];

    // статус
    static uint8_t* _STATE;

    // счетчик тактов
    static uint32_t* _TICK;

    // адрес возврата
    static uint16_t* _CALL;

    // страницы ПЗУ
    uint8_t* PAGE_ROM[4];

    // буфер ОЗУ
    uint8_t* RAMs;

    // файл трассировщика
    zxFile ftracer;

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

    // содержимое портов
    uint8_t *_KEMPSTON, *_1FFD, *_7FFD, *_FE;

    // страницы
    uint8_t *_RAM, *_VID, *_ROM;

    // текущий атрибут (порт #FF)
    uint8_t _FF;

    // параметры обновления ЦПУ
    int stateUP, stateLP, stateRP, stateDP;

    // текущее приращение ГПУ
    int periodGPU;

    // старое значение кнопок джойстика
    uint8_t joyOldButtons;

    // параметры мерцания
    uint32_t blink, blinkMsk, blinkShift;

    // цвет и размер границы
    uint32_t sizeBorder, colorBorder;

    // звуковая карта
    zxSound* snd;

    // дисковод
    zxDisk* disk;

    // имя проги
    std::string name;
};
