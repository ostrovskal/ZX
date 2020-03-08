//
// Created by Sergey on 04.02.2020.
//

#pragma once

#include "zxDevs.h"

extern uint8_t* opts;

class zxSoundMixer {
public:
    zxSoundMixer();
    void update(uint8_t * ext_buf);
    void use(uint32_t size, uint8_t* ext_buf = nullptr);
    void update();
    void reset() { acpu->reset(); beeper->reset(); }
    void frameStart(uint32_t tacts) {
        acpu->frameStart(tacts);
        beeper->frameStart(tacts);
    }

    void frameEnd(uint32_t tacts) {
        acpu->frameEnd(tacts);
        beeper->frameEnd(tacts);
    }
    uint32_t ready() const { return rdy; }

    int execute(uint8_t *buf);

protected:
    // признак включенного звука
    bool isEnable;
    // признак включенного AY
    bool isAyEnable;
    // признак включенного бипера
    bool isBpEnable;
    // выходной буфер
    uint8_t	buffer[65536];
    // размер данных
    uint32_t rdy;
    // бипер
    zxDevSound* beeper;
    // текущий звуковой процессор
    zxDevSound* acpu;
};
