//
// Created by Sergey on 19.02.2020.
//

#include "zxCommon.h"
#include "zxDevs.h"
#include "stkMnemonic.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                    КЛАВИАТУРА                                               //
/////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t semiRows[] = {
        /* 00 - 0 - 0 */ 0, 0x00, 0, 0,
        /* 01 - 1 - 5 */ 3, 0x01, 0, 0, 3, 0x02, 0, 0, 3, 0x04, 0, 0, 3, 0x08, 0, 0, 3, 0x10, 0, 0,
        /* 06 - 6 - 0 */ 4, 0x10, 0, 0, 4, 0x08, 0, 0, 4, 0x04, 0, 0, 4, 0x02, 0, 0, 4, 0x01, 0, 0,
        /* 11 - del - del */   4, 0x01, 0, 1,
        /* 12 - Q - T */ 2, 0x01, 0, 0, 2, 0x02, 0, 0, 2, 0x04, 0, 0, 2, 0x08, 0, 0, 2, 0x10, 0, 0,
        /* 17 - Y - P */ 5, 0x10, 0, 0, 5, 0x08, 0, 0, 5, 0x04, 0, 0, 5, 0x02, 0, 0, 5, 0x01, 0, 0,
        /* 22 - ent - ent */   6, 0x01, 0, 0,
        /* 23 - caps - caps */ 0, 0x01, 0, 0,
        /* 24 - A - G */ 1, 0x01, 0, 0, 1, 0x02, 0, 0, 1, 0x04, 0, 0, 1, 0x08, 0, 0, 1, 0x10, 0, 0,
        /* 28 - H - L */ 6, 0x10, 0, 0, 6, 0x08, 0, 0, 6, 0x04, 0, 0, 6, 0x02, 0, 0,
        /* 32 - sm-[E]*/ 7, 0x02, 0, 0, 0, 0x01, 7, 2,
        /* 34 - Z - V */ 0, 0x02, 0, 0, 0, 0x04, 0, 0, 0, 0x08, 0, 0, 7, 0x01, 0, 0, 0, 0x10, 0, 0,
        /* 39 - B - M */ 7, 0x10, 0, 0, 7, 0x08, 0, 0, 7, 0x04, 0, 0,
        /* 42 - RU/EN */    0x00, 0, 0, 0,
        /* 43 - cursor*/    4, 0x08, 0, 1, 4, 0x10, 0, 1, 3, 0x10, 0, 1, 4, 0x04, 0, 1,
        /* 47 - KEMPSTON */ 8, 0x02, 0, 0, 8, 0x01, 0, 0, 8, 0x08, 0, 0, 8, 0x04, 0, 0, 8, 0x10, 0, 0
};

void zxDevKeyboard::reset() {
    ssh_memset(&opts[ZX_PROP_VALUES_SEMI_ROW], 255, 8);
    opts[ZX_PROP_KEY_MODE] = 0;
    opts[ZX_PROP_KEY_CURSOR_MODE] = 255;
}

void zxDevKeyboard::read(uint16_t port, uint8_t* ret) {
    // 0,1,2,3,4 - клавиши полуряда, 6 - EAR, 5,7 - не используется
    port >>= 8;
    uint8_t res(0xBF);
    for(int i = 0; i < 8; i++) {
        if(!(port & numBits[i])) res &= opts[i + ZX_PROP_VALUES_SEMI_ROW];
    }
    *ret = res;
}

int zxDevKeyboard::update(int key) {
    // int key, int action
    if (key) {
        // клавиша была нажата/отпущена на экранной клавиатуре
        auto action = key >> 7;
        key &= 127;
        auto idx = key << 2;
        auto semiRow = ZX_PROP_VALUES_SEMI_ROW + semiRows[idx + 0];
        auto semiRowEx = ZX_PROP_VALUES_SEMI_ROW + semiRows[idx + 2];
        auto bit = semiRows[idx + 1];
        auto bitEx = semiRows[idx + 3];
        auto mode = opts[ZX_PROP_KEY_MODE];
        auto ret = (int)(*zx->_MODEL == MODEL_KOMPANION && (key == 43 && action));
        if(ret) {
            auto m = opts[RUS_LAT];
            opts[ZX_PROP_PORT_FEFC] = 255;
            m ^= 192; // признак режима
            opts[ZX_PROP_PORT_FEFC] &= ~(m & 128 ? 1 :  2);
            opts[RUS_LAT] = m;
        }
        if (!bitEx) {
            if (mode & ZX_CMD_KEY_MODE_CAPS) {
                bitEx = 1;
                semiRowEx = ZX_PROP_VALUES_SEMI_ROW;
            } else if (mode & ZX_CMD_KEY_MODE_SYMBOL) {
                bitEx = 2;
                semiRowEx = ZX_PROP_VALUES_SEMI_ROW + 7;
            }
        }
        if (!action) {
            if (key == 23) {
                mode &= ~ZX_CMD_KEY_MODE_SYMBOL;
                opts[ZX_PROP_VALUES_SEMI_ROW + 7] = 255;
                bitEx = 0;
                opts[ZX_PROP_KEY_MODE] = (uint8_t)(mode ^ ZX_CMD_KEY_MODE_CAPS);
            }
            if (key == 33) {
                mode &= ~ZX_CMD_KEY_MODE_CAPS;
                opts[ZX_PROP_VALUES_SEMI_ROW] = 255;
                bitEx = 0;
                opts[ZX_PROP_KEY_MODE] = (uint8_t)(mode ^ ZX_CMD_KEY_MODE_SYMBOL);
            }
            opts[semiRow] &= ~bit;
            opts[semiRowEx] &= ~bitEx;
        } else if (action == 1) {
            opts[semiRow] |= bit;
            opts[semiRowEx] |= bitEx;
        }
        return ret;
    } else {
        // опрос клавиатуры из ПЗУ
        if (opts[ZX_PROP_SHOW_KEY]) {
            static int delay = 0;
            delay++;
            if(!(delay & 7)) {
                // проверить режим клавиатуры
                uint8_t nmode = MODE_K;
                auto kmode = opts[ZX_PROP_KEY_MODE];
                auto omode = opts[ZX_PROP_KEY_CURSOR_MODE];
                if(*zx->_MODEL == MODEL_KOMPANION) {
/*
                    static uint8_t sys[256];
                    static char tmp[32768];
                    char* _tmp = tmp;
                    int idx = 0;
                    for(int i = 0 ; i < 255; i++) {
                        if(i < 120 || i > 122) {
                            int a = 23552 + i;
                            int n = rm8((uint16_t)a);
                            int o = sys[i];
                            if (n != o) {
                                ssh_strcpy(&_tmp, ssh_ntos(&a, RADIX_DEC));
                                *_tmp++ = ' ';
                                ssh_strcpy(&_tmp, ssh_ntos(&n, RADIX_DEC));
                                *_tmp++ = ' ';
                                idx++;
                            }
                        }
                    }
                    if(idx > 0) {
                        *_tmp = 0;
                        LOG_INFO("%s", tmp);
                        LOG_INFO("", 0);
                    }
                    memcpy(sys, realPtr(23552), 255);
*/
                    // проверить на текущий язык(по информации из знакогенератора)
                    auto n = ((rm8(23606) << 8) | rm8(23607));// en - 15424, ru - 14656
                    auto o = opts[RUS_LAT];
                    auto lng = (uint8_t)(((n == 14656) << 7) | 127);
                    if(lng != o) {
                        opts[ZX_PROP_KEY_CURSOR_MODE] = 255;
                        opts[RUS_LAT] = lng;
                    }
                }
                auto val0 = rm8(23617), val1 = rm8(23658), val2 = rm8(23611);
                switch (val0) {
                    case 0:
                        // если старый режим был [SE/CE], то сбрасывать CAPS/SYMBOL
                        if (val2 & 8) { nmode = (val1 & 8) ? MODE_C : MODE_L; }
                        else if (val1 & 16) nmode = MODE_K;
                        if (omode == MODE_CE) {
                            opts[ZX_PROP_VALUES_SEMI_ROW] |= 1;
                            kmode &= ~ZX_CMD_KEY_MODE_CAPS;
                            opts[ZX_PROP_KEY_MODE] = kmode;
                        }
                        else if (omode == MODE_SE) {
                            opts[ZX_PROP_VALUES_SEMI_ROW + 7] |= 2;
                            kmode &= ~ZX_CMD_KEY_MODE_SYMBOL;
                            opts[ZX_PROP_KEY_MODE] = kmode;
                        }
                        if ((nmode == MODE_L || nmode == MODE_C) && (kmode & ZX_CMD_KEY_MODE_CAPS)) nmode = MODE_CL;
                        else { if (kmode & ZX_CMD_KEY_MODE_SYMBOL) nmode = MODE_SK; else if (kmode & ZX_CMD_KEY_MODE_CAPS) nmode = MODE_CK; }
                        break;
                    case 1:
                        nmode = (kmode & ZX_CMD_KEY_MODE_SYMBOL) ? MODE_SE : ((kmode & ZX_CMD_KEY_MODE_CAPS) ? MODE_CE : MODE_E);
                        break;
                    case 2:
                        nmode = (kmode & ZX_CMD_KEY_MODE_CAPS) ? MODE_G : MODE_G1;
                        break;
                }
                if (opts[ZX_PROP_KEY_CURSOR_MODE] != nmode) {
                    opts[ZX_PROP_KEY_CURSOR_MODE] = nmode;
                    memcpy(&opts[ZX_PROP_VALUES_BUTTON], &buttons[nmode * 86], 86);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                     ДЖОЙСТИК                                                //
/////////////////////////////////////////////////////////////////////////////////////////////////
static void execJoyKeys(int i, bool pressed) {
    int k           = opts[ZX_PROP_JOY_KEYS + i] * 4;
    auto semiRow    = (ZX_PROP_VALUES_SEMI_ROW + semiRows[k + 0]);
    auto semiRowEx  = (ZX_PROP_VALUES_SEMI_ROW + semiRows[k + 2]);
    auto bit        = semiRows[k + 1];
    auto bitEx      = semiRows[k + 3];
    if (bitEx) opts[semiRowEx] ^= (-pressed ^ opts[semiRowEx]) & bitEx;
    if (semiRow == ZX_PROP_VALUES_KEMPSTON) pressed = !pressed;
    opts[semiRow] ^= (-pressed ^ opts[semiRow]) & bit;
}

zxDevJoy::zxDevJoy() {
    buttons = &opts[ZX_PROP_VALUES_KEMPSTON];
}

void zxDevJoy::reset() {
    oldButtons = 0;
    opts[ZX_PROP_JOY_ACTION_VALUE] = 0;
    opts[ZX_PROP_JOY_CROSS_VALUE] = 0;
}

bool zxDevJoy::checkRead(uint16_t port) const {
    if(port & 0x20) return false;
    port |= 0xfa00;
    return !(port == 0xfadf || port == 0xfbdf || port == 0xffdf);
}

int zxDevJoy::update(int) {
    if (opts[ZX_PROP_SHOW_JOY]) {
        auto buttons = (opts[ZX_PROP_JOY_ACTION_VALUE] << 4) | opts[ZX_PROP_JOY_CROSS_VALUE];
        if (buttons != oldButtons) {
            oldButtons = buttons;
            // 1. отжать
            for (int i = 0; i < 8; i++) {
                if(!(buttons & numBits[i])) execJoyKeys(i, true);
            }
            // 2. нажать
            for (int i = 0; i < 8; i++) {
                if(buttons & numBits[i]) execJoyKeys(i, false);
            }
        }
    }
    return 0;
}
