//
// Created by Sergey on 04.02.2020.
//

#include "zxCommon.h"
#include "zxSound.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                       SOUND MIXER                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////

zxSoundMixer::zxSoundMixer() : rdy(0), isEnable(true), isAyEnable(true), isBpEnable(true) {
	acpu   = (zxDevSound*)zx->devs[DEV_AY];
    beeper = (zxDevSound*)zx->devs[DEV_BEEPER];
    tape   = (zxDevSound*)zx->devs[DEV_TAPE];
}

void zxSoundMixer::update(uint8_t * ext_buf) {
    auto ready_a = acpu->sizeData();
    auto ready_b = beeper->sizeData();
    auto ready_t = tape->sizeData();
    if(ready_a > ready_b) ready_a = ready_b;
    if(ready_a > ready_t) ready_a = ready_t;
    if(rdy + ready_a > 65536) { rdy = 0; ready_a = 0; }
//    LOG_INFO("a:%i b:%i t:%i", ready_a, ready_b, ready_t);
    if(ready_a) {
        auto buf = ext_buf ? ext_buf : buffer;
        auto p = (int*)(buf + rdy);
        auto s0 = (int*)acpu->ptrData();
        auto s1 = (int*)beeper->ptrData();
        auto s2 = (int*)tape->ptrData();
        for(int i = 0; i < ready_a / 4; i++) *p++ = (*s0++) + (*s1++) + (*s2++);
        rdy += ready_a;
    }
    acpu->resetData();
    beeper->resetData();
    tape->resetData();
}

void zxSoundMixer::use(uint32_t size, uint8_t * ext_buf) {
    if(size) {
        if(rdy > size) {
            auto buf = ext_buf ? ext_buf : buffer;
            memmove(buf, buf + size, rdy - size);
        }
        rdy -= size;
    }
}

void zxSoundMixer::update() {
    reset();

    isEnable       = opts[ZX_PROP_SND_LAUNCH] != 0;
    isAyEnable     = isEnable && opts[ZX_PROP_SND_AY];
    isBpEnable     = isEnable && opts[ZX_PROP_SND_BP];

    auto chipAy    = opts[ZX_PROP_SND_CHIP_AY];
    acpu = (chipAy == 0 ? (zxDevSound*)zx->devs[DEV_AY] : (zxDevSound*)zx->devs[DEV_YM]);
}

int zxSoundMixer::execute(uint8_t *buf) {
    if(isEnable) {
        update(buf);
        auto size = ready();
        use(size, buf);
        return size;
    }
    return 0;
}
