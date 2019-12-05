//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include "zxStks.h"

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
    bool load(const char* name, int type);

    // запись
    bool save(const char* name, int type);

    // пресеты
    const char* presets(const char *name, int ops);

    // обновление свойств
    void updateProps();

    // сброс
    void signalRESET(bool resetTape);

    // обновление клавиатуры/джойстика
    int updateKeys(int key, int action);

    // диагностика
    void diag() { }

    // смена модели памяти
    bool changeModel(uint8_t _new, uint8_t  _old, bool isReset);

    // выполнение
    void execute();

    // чтение из порта
    uint8_t readPort(uint8_t A0A7, uint8_t A8A15);

    // поверхность рендеринга
    uint32_t* surface;

    // ПЗУ
    uint8_t* ROMS;

    // модель памяти
    uint8_t* _MODEL;

    // адреса текущих страниц
    static uint8_t *pageTRDOS, *pageROM, *pageRAM, *pageVRAM, *pageATTRIB;

    // страницы озу
    static uint8_t* PAGE_RAM[16];

    // статус
    static uint8_t* _STATE;

    // счетчик тактов
    static uint32_t* _TICK;

    // страницы ПЗУ
    uint8_t* PAGE_ROM[4];

protected:

    // загрузка состояния
    bool openState(const char *name);

    // загрузка файла в формате Z80
    bool openZ80(const char *name);

    // сохранение состояния
    bool saveState(const char* name);

    // сохранение файла в формате Z80
    bool saveZ80(const char *name);

    // установка актуальных страниц
    void setPages();

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

    // буфер ОЗУ
    uint8_t* RAMs;

    // параметры мерцания
    uint32_t blink, blinkMsk, blinkShift;

    // цвет и размер границы
    uint32_t sizeBorder, colorBorder;

    // габариты экрана
    uint32_t widthScreen, heightScreen;

    // процессор
    zxCPU* cpu;

    // звуковая карта
    //zxSND* snd;

    // лента
    //zxTAPE* tape;

    // имя программы
    char progName[32];
};
