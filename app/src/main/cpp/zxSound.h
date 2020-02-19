//
// Created by Sergey on 04.02.2020.
//

#pragma once

#include "zxDevs.h"

extern uint8_t* opts;

class zxSoundMixer {
public:
    zxSoundMixer() : acpu(&ay), rdy(0), isEnable(true), isAyEnable(true), isBpEnable(true) {}
    void update(uint8_t * ext_buf);
    void use(uint32_t size, uint8_t * ext_buf = nullptr);
    void update();
    void reset() { acpu->reset(); bp.reset(); }
    void frameStart(uint32_t tacts) {
        acpu->frameStart(tacts);
        bp.frameStart(tacts);
    }

    void frameEnd(uint32_t tacts) {
        acpu->frameEnd(tacts);
        bp.frameEnd(tacts);
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
    // 1 источник - бипер
    zxDevBeeper bp;
    // 2 источник - звуковой процессор1
    zxDevAY ay;
    // 3 источник - звуковой процессор2
    zxDevYM ym;
    // текущий
    zxDevSound* acpu;
};
