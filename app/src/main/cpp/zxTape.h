//
// Created by Sergey on 26.12.2019.
//

#pragma once

#include "zxSound.h"

class zxTape {
    friend class zxALU;
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
                            posImpulse(0), lenImpulse(0), sizeBufImpulse(0), bufImpulse(nullptr), snd(_snd), isTrap(false) {
        for(int i = 0 ; i < 128; i++) blocks[i].data = nullptr;
    }

    ~zxTape() { reset(); }

    // управление импульсами
    void control(int ticks);

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

    // вернуть с разбором, содержиое блока
    const char* getBlockData(int index, uint16_t * data);

    // запись в порт
    void writePort(uint8_t value);

    uint8_t _MIC;
    uint8_t _BEEP;

    // индекс текущего блока
    int currentBlock;

    // актуальное количество блоков
    int countBlocks;

    uint8_t readPort();

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
    bool trapSave();

    // перехватчик LOAD ""
    bool trapLoad();

    // пересоздание буфера импульсов
    void updateImpulseBuffer(bool force);

    // перейти на следующий блок
    bool nextBlock();

    // добавление блока
    void addBlock(uint8_t *data, uint16_t size);

    // вычисление размера буфера импульсов(приблизительно)
    size_t calcSizeBufferImpulse(bool isSkip);

    // общий загрузчик
    bool load(uint8_t *ptr, bool unpacked);

    // общий сохранитель:))
    uint8_t *save(int root, uint8_t *buf, bool packed);

    // массив блоков
    TAPE_BLOCK blocks[128];

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
