//
// Created by Sergey on 19.02.2020.
//

#include "zxCommon.h"
#include "zxDevs.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                    КОНТРОЛЛЕР ULA                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////

static int atrTab[192];
static uint8_t colTab[512];

uint32_t* zxDevUla::_TICK(nullptr);
uint32_t zxDevUla::_ftick(0);

zxDevUla::zxDevUla(): attr(0xFF), deltaTSTATE(0), colorBorder(7), sizeBorder(0), VIDEO(nullptr), ATTR(nullptr)  {
    _FE     = &opts[PORT_FE];
    _VID    = &opts[VID];
    _TICK   = (uint32_t*)&opts[TICK0];

    // вычисление таблицы адреса строк экрана
    for(int line = 0; line < 192; line++) atrTab[line] = (((line & 192) << 5) + ((line & 7) << 8) + ((line & 56) << 2));
    // таблица для мгновенного доступа к атрибутам
    for(int a = 0; a < 256; a++) {
        auto ink	= (uint8_t)((a >> 0) & 7);
        auto paper	= (uint8_t)((a >> 3) & 7);
        auto bright	= (uint8_t)((a >> 6) & 1);
        auto flash	= (uint8_t)((a >> 7) & 1);
        if(ink) ink |= bright << 3;
        if(paper) paper |= bright << 3;
        auto c1 = (uint8_t)((paper << 4) | ink);
        if(flash) { auto t = ink; ink = paper; paper = t; }
        auto c2 = (uint8_t)((paper << 4) | ink);
        colTab[a] = c1; colTab[a + 256] = c2;
    }
}

void zxDevUla::write(uint16_t port, uint8_t val) {
    if(!(port & 1)) {
        *_FE = val;
        colorBorder = (uint8_t)(val & 7);
    } else if(!(port & 0x8002)) {
	    auto vid = (uint8_t) ((val & 8) ? 7 : 5);
	    if(vid != *_VID) {
            *_VID = vid;
            update();
	    }
    }
}

void zxDevUla::reset() {
    attr = 0xFF;
    *_FE = 0b11100111;
    *_VID = 5;
    colorBorder = 7;
    update();
}

int zxDevUla::update(int param) {
    if(param) {
        static uint32_t frames[2] = { 0, 0 };

        uint32_t line, i, c;

        if(zx->pauseBetweenTapeBlocks > 0) {
            zx->pauseBetweenTapeBlocks--;
            if(zx->pauseBetweenTapeBlocks == 0)
                *zxSpeccy::_STATE &= ~ZX_PAUSE;
        }

        auto dest = zxGPU::frameBuffer;
        if(!dest) return 0;

        auto isBlink = (blink & 16) >> 4;
        auto szBorder = sizeBorder >> 1;
        auto colours = (uint32_t*)&opts[ZX_PROP_COLORS];

        attr = 0xFF;
        _ftick = 0;
        colorBorder = (uint8_t)(*_FE & 7);

        updateCPU(stateUP, true);
        for(line = 0 ; line < sizeBorder; line++) {
            updateCPU(stateLP, false);
            for (i = 0; i < (128 + sizeBorder); i++) {
                updateCPU(1, false);
                c = colours[colorBorder];
                *dest++ = c; *dest++ = c;
            }
            updateCPU(stateRP, false);
        }
        // screen
        for(line = 0 ; line < 192; line++) {
            updateCPU(stateLP, false);
            for(i = 0 ; i < szBorder; i++) {
                updateCPU(1, false);
                c = colours[colorBorder];
                *dest++ = c; *dest++ = c;
            }
            //*zxSpeccy::_STATE |= ZX_SCR;
            auto rb = atrTab[line];
            for (int ri = 0; ri < 32; ri++) {
                attr = ATTR[ri + ((line & 248) << 2)];
                auto idx = colTab[(((attr & 128) && isBlink) << 8) + attr];
                frames[1] = colours[idx & 15];
                frames[0] = colours[idx >> 4];
                auto v = VIDEO[rb];
                for(int b = 7 ; b >= 0; b--) {
                    *dest++ = frames[(v >> b) & 1];
                    if(!(b & 1)) updateCPU(1, false);
                }
                rb++;
            }
            //attr = 0xFF;
            //*zxSpeccy::_STATE &= ~ZX_SCR;
            for(i = 0 ; i < szBorder; i++) {
                updateCPU(1, false);
                c = colours[colorBorder];
                *dest++ = c; *dest++ = c;
            }
            updateCPU(stateRP, false);
        }
        for(line = 0 ; line < sizeBorder; line++) {
            updateCPU(stateLP, false);
            for (i = 0; i < (sizeBorder + 128); i++) {
                updateCPU(1, false);
                c = colours[colorBorder];
                *dest++ = c; *dest++ = c;
            }
            updateCPU(stateRP, false);
        }
        updateCPU(stateDP, false);
        blink++;
        //LOG_INFO("ts: %i", _TICK % ts);
    } else {
        auto periodCPU = opts[ZX_PROP_CPU_SPEED];

        sizeBorder  = opts[ZX_PROP_BORDER_SIZE];
        stateUP     = zxSpeccy::machine->ts[sizeBorder].up * periodCPU / 10;
        stateLP     = zxSpeccy::machine->ts[sizeBorder].lp * periodCPU / 10;
        stateRP     = zxSpeccy::machine->ts[sizeBorder].rp * periodCPU / 10;
        stateDP     = zxSpeccy::machine->ts[sizeBorder].dp * periodCPU / 10;
	    sizeBorder  <<= 4;

	    VIDEO       = &zx->RAMbs[*_VID << 14];
	    ATTR        = VIDEO + 6144;
    }
    return 0;
}

void zxDevUla::updateCPU(int todo, bool interrupt) {
    int ticks;
    todo += deltaTSTATE;
    if (interrupt) {
        ticks = zx->stepCPU(true);
        todo -= ticks;
        *_TICK += ticks;
        _ftick += ticks;
    }
    while(todo > 0) {
        ticks = zx->stepCPU(false);
        todo -= ticks;
        *_TICK += ticks;
        _ftick += ticks;
    }
    deltaTSTATE = todo;
}
