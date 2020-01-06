//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxTape.h"

void zxTape::control(int ticks) {

}

int zxTape::trap(uint16_t PC) {
    return 0;
}

bool zxTape::openWAV(const char *path) {
    return false;
}

bool zxTape::openTAP(const char *path) {
    return false;
}

void zxTape::loadState(uint8_t *ptr) {

}

uint8_t *zxTape::saveState(uint8_t *ptr) {
    return nullptr;
}

bool zxTape::saveWAV(const char *path) {
    return false;
}

bool zxTape::saveTAP(const char *path) {
    return false;
}

void zxTape::updateProps() {

}

void zxTape::reset() {

}

void zxTape::writePort(uint8_t value) {

}
