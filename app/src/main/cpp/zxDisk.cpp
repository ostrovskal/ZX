//
// Created by Sergey on 06.01.2020.
//

#include "zxCommon.h"
#include "zxDisk.h"

// SYS
// чтение:
// 6 - DRQ      - сигнал, отражающий запрос данных микроконтроллером
// 7 - INTRQ    - сигнал окончания выполнения команды
// запись:
// 0, 1         - выбор дисковода A,B,C,D
// 2            - аппаратный сброс
// 3            - этот разряд блокирует сигнал HLT микроконтроллера, для нормальной работы в нем должна быть записана единица
// 4            - выбор магнитной головки. Содержимое этого разряда напрямую транслируется в дисковод.
//                  // 0 - соответствует первой магнитной головке или нижней стороне дискеты
//                  // 1 — второй магнитной головке или верхней стороне
// 5(6?)        - выбор плотности записи. Сброс разряда заставляет микроконтроллер работать по методу частотной модуляции (FM),
                    // установка — по методу модифицированной частотной модуляции (MFM)


// временный буфер диска
static uint8_t tmpBufDisk[16384];

zxDisk::DISK& zxDisk::currentDisk() {
    return disks[opts[TRDOS_OUT] & 3];
}

void zxDisk::seekDelay(uint8_t dst_track) {
    auto sub = dst_track - currentDisk().track;
    dir = sub > 0;
    cmd_done = currentTimeMillis() + 20 * abs(sub);
    opts[TRDOS_IN] = (uint8_t)(isDiskNoReady() ? 0x40 : 0x80);
/*
    if(isDiskNoReady()) vg_portFF_in = 0x80;
    else vg_portFF_in = 0x80;
*/
    currentDisk().track = dst_track;
    opts[TRDOS_TRK] = dst_track;
}

void zxDisk::setFlagsSeeks() {
    vg_rs.bit.b0 = 0;
    vg_rs.bit.b2 = (uint8_t)(currentDisk().track == 0);
    vg_rs.bit.b3 = 0;
    vg_rs.bit.b4 = 0;
    vg_rs.bit.b6 = currentDisk().ro;
    vg_rs.bit.b7 = (uint8_t)isDiskNoReady();
}

void zxDisk::reset() {
    for(int i = 0; i < 4; i++) {
        auto disk = &disks[i];
        disk->track = 0;
        disk->ro = 0;
        memset(disk->filename, 0, 256);
    }
    bufferData = nullptr;
    posWrite = posRead = 0;
    lenWrite = lenRead = 0;
    dir = spin = side = 0;
    cmd_done = 0;
}

void zxDisk::writeData(uint8_t val) {
    if(lenWrite == 0) return;
    if(!file.open(currentDisk().filename, zxFile::open_read_write)) return;

    if(file.set_pos(posWrite, SEEK_SET) != posWrite) {
        lenWrite = 0;
        file.close();
        return;
    }
    file.write(&val, 1);
    file.close();
    lenWrite--;
    posWrite++;
    if(lenWrite == 0) {
        opts[TRDOS_IN] = 0x80;
        vg_rs.byte = 0;
        vg_rs.bit.b0 = 0; // Busy
        vg_rs.bit.b1 = 0; // DRQ copy
    }
    else opts[TRDOS_IN] = 0x40;
}

void zxDisk::writePort(uint8_t A0A7, uint8_t val) {
    LOG_INFO("port: %02X val: %i", A0A7, val);
    switch(A0A7) {
        case PORT_CMD: writeCommand(val); break;
        case PORT_TRK: opts[TRDOS_TRK] = val; break;
        case PORT_SEC: opts[TRDOS_SEC] = val; break;
        case PORT_DAT: opts[TRDOS_DAT] = val; writeData(val); break;
        case PORT_SYS: opts[TRDOS_OUT] = val; break;
    }
}

uint8_t zxDisk::readPort(uint8_t A0A7) {

    uint8_t ret = 0xff;

    if(spin) vg_rs.bit.b1 = (uint8_t)(isDiskNoReady() ? 0 : (currentTimeMillis() % 200) < 20);

    switch(A0A7) {
        case PORT_CMD:
            ret = (uint8_t)(isDiskNoReady() ? 0x80 : vg_rs.byte);
            break;
        case PORT_TRK:
            ret = opts[TRDOS_TRK];
            break;
        case PORT_SEC:
            ret = opts[TRDOS_SEC];
            break;
        case PORT_DAT:
            ret = readData();
            break;
        case PORT_SYS: {
//            auto psys = opts[TRDOS_IN];
//            ret = (currentTimeMillis() < cmd_done) ? (uint8_t)(psys & ~0x40) : psys;
            break;
        }
    }
    //LOG_INFO("port: %X ret: %i", A0A7, ret);
    return ret;
}

uint8_t zxDisk::readData() {
    if(posRead < lenRead) {
        opts[TRDOS_DAT] = bufferData[posRead];
        if(((posRead & 0x00ff) == 0) && posRead != 0) opts[TRDOS_SEC]++;
        if (opts[TRDOS_SEC] > 16) opts[TRDOS_SEC] = 1;
        posRead++;
        if (posRead == lenRead) {
            opts[TRDOS_IN] = 0x80;
            vg_rs.byte = 0;
        } else {
            opts[TRDOS_IN] = 0x40;
            vg_rs.bit.b0 = 1; // Busy
            vg_rs.bit.b1 = 1; // DRQ copy
        }
    }
    return opts[TRDOS_DAT];
}

void zxDisk::writeCommand(uint8_t val) {
    side = (opts[TRDOS_OUT] >> 4) ^ 1;
    // interrupt
    if ((val & 0xF0) == 0xD0) {
        opts[TRDOS_IN] = 0x80;
        setFlagsSeeks();
        return;
    }
    if((val & 0xF0) == 0x00) {
        seekDelay(0);
        vg_rs.bit.b5 = (uint8_t)((val & 8) >> 3);
        setFlagsSeeks();
        if (val & 8) spin = 1;
        return;
    }
    // seek track
    if((val & 0xF0) == 0x10) {
        seekDelay(opts[TRDOS_DAT]);
        vg_rs.bit.b5 = (uint8_t)((val & 8) >> 3);
        setFlagsSeeks();
        if (val & 8) spin = 1;
        return;
    }
    // fwd
    if((val & 0xE0) == 0x40) {
        dir = 1;
        // step
        val = 0x20;
    }
    // back
    if((val & 0xE0) == 0x60) {
        dir = 0;
        // step
        val = 0x20;
    }
    // step
    if((val & 0xE0) == 0x20) {
        if (dir)
            seekDelay(++currentDisk().track);
        else
            seekDelay(--currentDisk().track);
        vg_rs.bit.b5 = 1;
        setFlagsSeeks();
        spin = 1;
        return;
    }
    // io_readsec
    if((val & 0xE0) == 0x80) {
        vg_rs.byte = 0x81;
        opts[TRDOS_IN] = 0x40;
        spin = 0;
        if(isDiskNoReady()) {
            vg_rs.byte = 0x90;
            opts[TRDOS_IN] = 0x80;
            return;
        }
        if((opts[TRDOS_SEC] == 0) || (opts[TRDOS_SEC] > 16)) {
            // sector not found
            vg_rs.byte |= 0x10;
            opts[TRDOS_IN] = 0x80;
            return;
        }
        if(!file.open(currentDisk().filename, zxFile::open_read)) {
            vg_rs.byte = 0x90;
            opts[TRDOS_IN] = 0x80;
            return;
        }
        auto pointer = (currentDisk().track * 2 + side) * 256 * 16 + (opts[TRDOS_SEC] - 1) * 256;
        if(file.set_pos(pointer, SEEK_SET) != pointer) {
            file.close();
            vg_rs.byte |= 0x10; // sector not found
            opts[TRDOS_IN] = 0x80;
            return;
        }
        if(val & 0x10) {
            auto sector = opts[TRDOS_SEC];
            file.read(tmpBufDisk, (size_t)(256 * (17 - sector)));
            file.set_pos(-256 * ((17 - sector) + (sector - 1)), SEEK_CUR);
            file.read(tmpBufDisk, (size_t)(256 * (sector - 1)));
            file.close();
            bufferData = tmpBufDisk;
            lenRead = 256 * 16;
            posRead = 0;
            // vg_portFF_in=0x80;
            // *** !!! ***
            // todo : Eto proverit' !!!
        }
        else {
            if(file.read(tmpBufDisk, 256)) {
                file.close();
                bufferData = tmpBufDisk;
                lenRead = 256;
                posRead = 0;
                cmd_done = currentTimeMillis() + 30;
            }
            else {
                file.close();
                vg_rs.byte |= 0x10; // sector not found
                opts[TRDOS_IN] = 0x80;
            }
        }
        return;
    }
    // io_read adr
    if((val & 0xFB) == 0xC0) {
        static uint8_t six_bytes[6];
        vg_rs.byte = 0x81;
        opts[TRDOS_IN] = 0x40;
        six_bytes[0] = currentDisk().track;
        six_bytes[1] = 0;
        six_bytes[2] = opts[TRDOS_SEC];
        six_bytes[3] = 1;
        six_bytes[4] = 0; // todo : crc !!!
        six_bytes[5] = 0; // todo : crc !!!
        bufferData = six_bytes;
        lenRead = 6;
        posRead = 0;
        cmd_done = currentTimeMillis() + 30;
        spin = 0;
        return;
    }
    // io_writesec
    if((val & 0xE0) == 0xA0) {
        vg_rs.byte = 0x81;
        opts[TRDOS_IN] = 0x40;
        spin = 0;
        if(currentDisk().ro) {
            vg_rs.byte = 0x60;
            opts[TRDOS_IN] = 0x80;
            return;
        }
        if(isDiskNoReady()) {
            vg_rs.byte = 0x90;
            opts[TRDOS_IN] = 0x80;
            return;
        }
        if((opts[TRDOS_SEC] == 0) || (opts[TRDOS_SEC] > 16)) {
            vg_rs.byte |= 0x10; // sector not found
            opts[TRDOS_IN] = 0x80;
            return;
        }
        posWrite = (currentDisk().track * 2 + side) * 256 * 16 + (opts[TRDOS_SEC] - 1) * 256;
        lenWrite = 256;
        spin = 0;
        return;
    }
    LOG_INFO("unknown command %X", val);
}

uint8_t *zxDisk::saveState(uint8_t *ptr) {
    return ptr;
}

bool zxDisk::loadState(uint8_t *ptr) {
    return true;
}

bool zxDisk::openTRD(int num, const char *path) {
    if(num >= 0 && num < 4) {
        auto disk = &disks[num];
        disk->track = 0;
        disk->ro = 0;
        auto len = strlen(path);
        memset(disk->filename, 0, 256);
        memcpy(disk->filename, path, len > 254 ? 254 : len);
        return true;
    }
    return false;
}

void zxDisk::updateProps() {

}
