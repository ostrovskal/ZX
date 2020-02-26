//
// Created by Sergey on 19.02.2020.
//

#pragma once

class zxMagnitophon : public zxDevTape {
public:
    zxMagnitophon();
    virtual ~zxMagnitophon() { closeTape(); }
    // проверка на порт для чтения
    virtual bool    checkRead(uint16_t port) const override { return !(port & 1); }
    // чтение из порта
    virtual void    read(uint16_t port, uint8_t* ret) override { *ret |= tapeBit() & 0x40; }
    // сброс устройства
    virtual void    reset() override { zxDevSound::reset(); resetTape(); }
    // обновление
    virtual int     update(int param = 0) override { return 0; }
    // тип устройства
    virtual int     type() const override { return DEV_TAPE; }
    // доступ к портам
    virtual int     access() const override { return ACCESS_READ; }


    bool open(uint8_t* ptr, size_t size, int type);

    void startTape();
    void stopTape();
    void resetTape();
    void closeTape();
    bool started() const { return tape.play_pointer != nullptr; }
    bool inserted() const { return tape_image != nullptr; }
    uint8_t tapeBit();
protected:
    uint32_t findPulse(uint32_t t);
    void findTapeIndex();
    void findTapeSizes();
    void reserve(uint32_t datasize);
    void makeBlock( uint8_t* data, uint32_t size, uint32_t pilot_t, uint32_t s1_t, uint32_t s2_t, uint32_t zero_t, uint32_t one_t,
                    uint32_t pilot_len, uint32_t pause, uint8_t last = 8);
    // описение текущего блока(заголовок, блок кода и т.д.)
    void desc(const uint8_t* data, uint32_t size, char* dst);
    // выделить память под структуру описания текущего блока
    void allocInfocell();
    void namedCell(const void *nm, uint32_t sz = 0);
    void createAppendableBlock();
    void parseHardware(uint8_t* ptr);
protected:
    struct TAPE_BLOCK {
        // тип блока
        uint8_t type;
        // тип импульса(пилот, после пилота, данные, последний байт, пауза)
        uint32_t type_impulse;
        // данные
        uint8_t* data;
        // длина данных
        uint32_t data_size;
        // импульсы
        uint8_t* impulse;
        // длина импульсов
        uint32_t impulse_size;
        // длины импульсов
        uint32_t pilot_t, s1_t, s2_t;
        uint32_t zero_t, one_t, pilot_len;
        uint32_t pause, last;
    };
    struct TAPE_STATE {
        // таксты для следующих импульсов
        uint64_t edge_change;
        // указатель на текущий блок импульсов
        uint8_t* play_pointer;  // или nullptr, если лента остановлена
        // указатель на конец ленты
        uint8_t* end_of_tape;   // место, где лента была остановлена
        //
        uint32_t index;         // текущий блок ленты
        uint8_t tape_bit;
    };
    struct TAPEINFO {
        char desc[280];
        uint32_t pos, t_size;
    };

    TAPE_STATE tape;
    TAPEINFO* tapeinfo;

    uint32_t tape_pulse[256];
    uint32_t max_pulses;
    uint32_t tape_err;

    uint8_t* tape_image;
    uint32_t tape_imagesize;

    uint32_t tape_infosize;
    uint32_t appendable;
};
