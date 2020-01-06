//
// Created by Sergey on 26.12.2019.
//

#include "zxCommon.h"
#include "zxSound.h"

uint8_t* zxSound::_REGISTERS(nullptr);
uint8_t* zxSound::_CURRENT(nullptr);

zxSound::zxSound() {
    _REGISTERS = &opts[AY_AFINE];
    _CURRENT = &opts[AY_REG];
}

zxSound::~zxSound() {

}

void zxSound::updateProps(uint8_t period) {

}

void zxSound::reset() {

}

void zxSound::writeCurrent(uint8_t value) {

}

void zxSound::update() {

}
