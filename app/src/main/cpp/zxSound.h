//
// Created by Sergey on 26.12.2019.
//

#pragma once

class zxSound {
public:
    zxSound();
    ~zxSound();

    static uint8_t* _REGISTERS;
    static uint8_t* _CURRENT;

    void updateProps(uint8_t period);

    void reset();

    void writeCurrent(uint8_t value);

    void update();
};
