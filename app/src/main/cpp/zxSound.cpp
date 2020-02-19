//
// Created by Sergey on 04.02.2020.
//

#include "zxCommon.h"
#include "zxSound.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                       SOUND MIXER                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////

void zxSoundMixer::update(uint8_t * ext_buf) {
    auto ready_min = acpu->sizeData();
    auto ready_s = bp.sizeData();
    if(ready_min > ready_s) ready_min = ready_s;
    if(rdy + ready_min > 65536) { rdy = 0; ready_min = 0; }
    if(ready_min) {
        auto buf = ext_buf ? ext_buf : buffer;
        auto p = (int*)(buf + rdy);
        auto s0 = (int*)acpu->ptrData();
        auto s1 = (int*)bp.ptrData();
        for(int i = 0; i < ready_min / 4; i++) *p++ = (*s0++) + (*s1++);
        rdy += ready_min;
    }
    acpu->resetData();
    bp.resetData();
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
    acpu = (chipAy == 0 ? (zxDevSound*)&ay : (zxDevSound*)&ym);

    acpu->update(0);
    bp.update(opts[ZX_PROP_SND_VOLUME_BP]);
}

int zxSoundMixer::execute(uint8_t *buf) {
    update(buf);
    auto size = ready();
    use(size, buf);
    return size;
}
