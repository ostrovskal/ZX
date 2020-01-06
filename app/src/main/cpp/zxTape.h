//
// Created by Sergey on 26.12.2019.
//

#pragma once

#include "zxSound.h"

/*
Формат стандартного заголовочного блока Бейсика такой:
1 байт  - флаговый, для блока заголовка всегда равен 0 (для блока данных за ним равен 255)
1 байт  - тип Бейсик блока, 0 - бейсик программа, 1 - числовой массив, 2 - символьный массив, 3 - кодовый блок
10 байт - имя блока
2 байта - длина блока данных, следующего за заголовком (без флагового байта и байта контрольной суммы)
2 байта - Параметр 1, для Бейсик-программы - номер стартовой строки Бейсик-программы, заданный параметром LINE (или число >=32768,
            если стартовая строка не была задана.
            Для кодового блока - начальный адрес блока в памяти.
            Для массивов данных - 14й-байт хранит односимвольное имя массива
2 байта - Параметр 2. Для Бейсик-программы - хранит размер собственно Бейсик-програмы, без инициализированных переменных,
            хранящихся в памяти на момент записи Бейсик-программы.
            Для остальных блоков содержимое этого параметра не значимо, и я почти уверен, что это не два байта ПЗУ.
            Скорее всего, они просто не инициализируются при записи.
1 байт - контрольная сумма заголовочного блока.
*/

class zxTape {
public:
    struct TAPE_BLOCK {
        TAPE_BLOCK() : data(nullptr), size(0) { }
        TAPE_BLOCK(uint8_t* _data, uint16_t _size) : size(_size) {
                data = new uint8_t[_size]; memcpy(data, _data, size);
        }
        ~TAPE_BLOCK() { delete[] data; data = nullptr; }
        // данные
        uint8_t* data;
        // длина блока
        uint16_t size;

    };

    zxTape(zxSound* _snd) : _MIC(0), _BEEP(0), countBlocks(0), currentBlock(0), koef(0),
                                    posImpulse(0), lenImpulse(0), sizeBufImpulse(0), bufImpulse(nullptr), snd(_snd), isTrap(false) { }

    ~zxTape() { reset(); }

    // управление импульсами
    void control(int ticks);

    // перехватчик
    int trap(uint16_t PC);

    // загрузить WAV
    bool openWAV(const char *path);

    // сохранить WAV
    bool saveWAV(const char *path);

    // загрузить TAP
    bool openTAP(const char *path);

    // сохранить TAP
    bool saveTAP(const char *path);

    // загрузить состояние
    bool loadState(uint8_t *ptr);

    // сохранить состояние
    uint8_t *saveState(uint8_t *ptr);

    // обновить свойства
    void updateProps();

    // сброс
    void reset();

    // запись в порт
    void writePort(uint8_t value);

    uint8_t _MIC;
    uint8_t _BEEP;

protected:

    // проверить на наличие бита
    bool checkBit(uint8_t *src, int pos);

    // растянуть бит, в зависимости от несущей частоты
    void expandBit(uint8_t *dst, int pos, bool set);

    // создать буфер импульсов для определенного блока данных
    void makeImpulseBuffer(uint8_t *buf, int idx, int &len);

    // создать серию импульсов
    void makeImpulse(double count, uint8_t *buf, int &len);

    // перехватчик SAVE ""
    void trapSave();

    // перехватчик LOAD ""
    void trapLoad();

    // пересоздание буфера импульсов
    void updateImpulseBuffer(bool force);

    // вычисление размера буфера импульсов(приблизительно)
    size_t calcSizeBufferImpulse(bool isSkip);

    // общий загрузчик
    bool load(uint8_t *ptr, bool unpacked);

    // перейти на следующий блок
    bool nextBlock();

    // общий сохранитель:))
    uint8_t *save(int root, uint8_t *buf, bool packed);

    // добавление блока
    void addBlock(uint8_t *data, uint16_t size);

    // массив блоков
    TAPE_BLOCK blocks[128];

    // актуальное количество блоков
    int countBlocks;

    // индекс текущего блока
    int currentBlock;

    // количество миллисекунд для пропуска
    double skipMillis;

    // несущая частота
    double freq;

    int rewCounter;

    // позиция/длина импульсов
    int posImpulse, lenImpulse;

    // коэфицент чередования единичных/нулевых битов
    int koef;

    // признак перехвата load/save
    bool isTrap;

    // звук
    zxSound* snd;

    // временный буфер импульсов
    uint8_t *bufImpulse;

    // размер буфера импульсов
    int sizeBufImpulse;
};
