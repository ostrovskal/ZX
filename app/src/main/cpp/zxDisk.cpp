//
// Created by Sergey on 06.01.2020.
//

#include "zxCommon.h"
#include "zxDisk.h"

bool zxDisk::openTRD(const char *path) {
    return false;
}

void zxDisk::updateProps() {

}

void zxDisk::reset() {

}

void zxDisk::write(uint8_t value) {

}

void zxDisk::writePort(uint8_t A0A7, uint8_t val) {
    info("disk writePort cmd: %i val: %i", A0A7, val);
    switch(A0A7) {
        case 0x1F: opts[TRDOS_CMD] = val; break;
        case 0x3F: opts[TRDOS_TRK] = val; break;
        case 0x5F: opts[TRDOS_SEC] = val; break;
        case 0x7F: opts[TRDOS_DAT] = val; write(val); break;
        case 0xFF: opts[TRDOS_SYS] = val; break;
    }
}
