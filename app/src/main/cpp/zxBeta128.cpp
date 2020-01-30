//
// Created by Sergey on 30.01.2020.
//

#include "zxCommon.h"
#include "zxBeta128.h"

void zxBeta128::DISK::setPos(uint8_t trk, uint8_t head, uint8_t sec) {
    auto addr = (size_t)((trk * nhead + head) * nsec + sec) * (128 << nsize);
    file.set_pos(addr, zxFile::begin);
    LOG_INFO("addr: %i trk: %i head: %i sec: % i", addr, trk, head, sec);
}

void zxBeta128::reset_controller() {
    req = hlt = head = mfm = 0;
    ndsk = sec_mult = delay = formatCounter = 0;
    sdir = 0;
    regs[R_TRK] = regs[R_DAT] = regs[R_STS] = regs[R_CMD] = 0;
    regs[R_SEC] = 1;
    cmd = state = nextState = -1;
    time = currentTimeMillis();
}

// 0,1 - номер диска
// 2 - апаратный сброс
// 3 - блокировка согнала HLT(должна быть 1)
// 4 - номер головки 0=первая
// 6 - 1=MFM, 0=FM
void zxBeta128::write_sys(uint8_t val) {
    if(!(val & 0x04)) reset_controller();
    hlt     = (uint8_t) ((val & 0x08) >> 3);
    head    = (uint8_t)!((val & 0x10) >> 4);
    mfm     = (uint8_t)!((val & 0x40) >> 6);
    regs[R_CMD] = val;
    if((val & 3) != ndsk) {
        if(ndsk >= 0 && ndsk < 4) curDisk()->file.close();
        // смена диска
        ndsk = val & 3;
        auto dsk = curDisk();
        dsk->file.open(dsk->path.c_str(), dsk->write ? zxFile::open_read_write : zxFile::open_read);
    }
    LOG_INFO("drv:%i hlt:%i head:%i mfm:%i reset:%i", ndsk, hlt, head, mfm, (val & 0x04));
}

void zxBeta128::cmd_exec(uint8_t val) {
    // проверить на команду interrupt
    if((val & 0xF0) == 0xD0) {
        // прерывание команды
        state = C_STATUS;
        cmd = 1;
        regs[R_STS] &= ~stsBusy;
        if(val & 0x08) req = reqINTRQ;
        return;
    }
    // если шина занята - команды игнорируются
    if(regs[R_STS] & stsBusy) return;

    regs[R_STS] = (uint8_t)(stsBusy | ((!ready()) << 7));
    req = 0;
    switch(val & 0xF0) {
        case 0x00:
            // RESTORE
            state = C_DELAY;
            nextState = C_STATUS;
            cmd = 1; sdir = 0;
            regs[R_TRK] = 0;
            curDisk()->track = 0;
            delay = STEP_TIME;
            break;
        case 0x10:
            // SEEK
            state = C_DELAY;
            nextState = C_STATUS;
            cmd = 1;
            sdir = (uint8_t)(regs[R_TRK] > regs[R_DAT]);
            delay = abs(regs[R_TRK] - regs[R_DAT]) * STEP_TIME;
            regs[R_TRK] = regs[R_DAT];
            curDisk()->track = regs[R_TRK];
            break;
        case 0x20: case 0x30:
            state = C_STEP;
            break;
        case 0x40: case 0x50:
            state = C_STEP;
            sdir = 0;
            break;
        case 0x60: case 0x70:
            state = C_STEP;
            sdir = 1;
            break;
        case 0x80: case 0x90:
            state = C_DELAY;
            nextState = C_READ_SECTOR;
            cmd = 2; index = 0;
            sec_mult = regs[R_CMD] & 0x10;
            delay = (regs[R_CMD] & 0x04) ? HEAD_TIME : 1;
            curDisk()->setPos(regs[R_TRK], head, regs[R_SEC]);
            break;
        case 0xA0: case 0xB0:
            state = C_DELAY;
            nextState = C_WRITE_SECTOR;
            sec_mult = regs[R_CMD] & 0x10;
            delay = (regs[R_CMD] & 0x04) ? HEAD_TIME : 1;
            cmd = 2; index = 0;
            curDisk()->setPos(regs[R_TRK], head, regs[R_SEC]);
            //delay = LOST_DATA_TIMEOUT;
            req |= reqDRQ;
            regs[R_STS] |= stsDRQ;
            if(!curDisk()->write) regs[R_STS] |= stsWriteProtect;
            break;
        case 0xC0:
            state = C_READ_ADDRESS;
            cmd = 3;
            break;
        case 0xE0:
            state = C_IDLE;
            cmd = 3;
            //state = C_READ_TRACK;
            curDisk()->setPos(regs[R_TRK], head, regs[R_SEC]);
            break;
        case 0xF0:
            state = C_WRITE_TRACK;
            cmd = 3; formatCounter = 0;
            delay = LOST_DATA_TIMEOUT;
            req |= reqDRQ;
            regs[R_STS] |= stsDRQ;
            if(!curDisk()->write) regs[R_STS] |= stsWriteProtect;
            break;
    }
    time = currentTimeMillis();
}

// ----------------------------------------------------------------------------------------------------------------------------------
// Бит      | Восст. и позиц.  |    Запись сектора  | Чтение сектора    |   Чтение адреса   |   Запись дорожки  |   Чтение дорожки  |
// ----------------------------------------------------------------------------------------------------------------------------------
//  7       |               Г   о   т   о   в   н   о   с   т   ь       д   и   с   к   о   в   о   д   а                           |
// ----------------------------------------------------------------------------------------------------------------------------------
//  6       |        З А Щ И Т А   З А П И С И      |                   0                   |   ЗАЩИТА ЗАПИСИ   |         0         |
// ----------------------------------------------------------------------------------------------------------------------------------
//  5       |     ЗАГРУЗКА     |       ОШИБКА       |    ПОВТОРЯЕТ      |                   |      ОШИБКА       |                   |
//          | МАГНИТНОЙ ГОЛОВКИ|       ЗАПИСИ       |  ЗНАЧЕНИЕ БИТА    |         0         |      ЗАПИСИ       |         0         |
// ----------------------------------------------------------------------------------------------------------------------------------
//  4       |  ОШИБКА ПОИСКА   |          C  Е  К  Т  О  Р   Н  Е   Н  А  Й  Д  Е  Н        |                   0                   |
// ----------------------------------------------------------------------------------------------------------------------------------
//  3       |           О   Ш   И   Б   К   А           C       R       C                   |                   0                   |
// ----------------------------------------------------------------------------------------------------------------------------------
//  2       |    ДОРОЖКА 0     |     П       О       Т       Е       Р       Я           Д       А       Н       Н       Ы       Х  |
// ----------------------------------------------------------------------------------------------------------------------------------
//  1       |ИНДЕКСНЫЙ ИМПУЛЬС |       З       А       П       Р       О       С       Д       А       Н       Н       Ы       Х    |
// ----------------------------------------------------------------------------------------------------------------------------------
//  0       |                И   д   е   т           в   ы   п   о   л   н   е   н   и   е       к   о   м   а   н   д   ы          |
// ----------------------------------------------------------------------------------------------------------------------------------
void zxBeta128::exec() {
    switch(state) {
        case C_IDLE:
            regs[R_STS] &= ~stsBusy;
            req &= ~reqDRQ; req |= reqINTRQ;
            break;
        case C_STATUS:
            if(curDisk()->track == 0) regs[R_STS] |= stsTrack0;
            if(regs[R_CMD] & 0x08) regs[R_STS] |= stsLoadHead;
            regs[R_STS] |= stsImpulse;
            state = C_IDLE;
            break;
        case C_DELAY:
            if((currentTimeMillis() - time) >= delay) state = nextState;
            break;
        case C_STEP:
            if(sdir == 0 && regs[R_TRK] < 80) regs[R_TRK]++;
            if(sdir == 1 && regs[R_TRK] > 0) regs[R_TRK]--;
            delay = STEP_TIME;
            nextState = C_STATUS;
            state = C_DELAY;
            cmd = 1;
            break;
        case C_READ_ADDRESS:
            if(index >= 6) { state = C_IDLE; break; }
            if(req & reqDRQ) {
                if((currentTimeMillis() - time) >= delay) regs[R_STS] |= stsLostData;
                break;
            }
            switch(index) {
                // track
                case 0: regs[R_DAT] = regs[R_TRK]; break;
                // side always 0
                case 1: regs[R_DAT] = 0; break;
                // sector
                case 2: regs[R_DAT] = regs[R_SEC]; break;
                // sector size
                case 3: regs[R_DAT] = curDisk()->nsize; break;
                // checksum
                case 4: case 5: regs[R_DAT] = 0; break;
            }
            index++; req |= reqDRQ; regs[R_STS] |= stsDRQ;
            delay = LOST_DATA_TIMEOUT;
            time = currentTimeMillis();
            break;
        case C_READ_SECTOR:
            if(index >= (sec_mult ? (curDisk()->nsec - regs[R_SEC] + 1) : 1) * (128 << curDisk()->nsize)) { state = C_IDLE; break; }
            if(req & reqDRQ) {
                if((currentTimeMillis() - time) < delay) break;
                regs[R_STS] |= stsLostData;
            }
            curDisk()->file.read(&regs[R_DAT], 1);
            index++; req |= reqDRQ; regs[R_STS] |= stsDRQ;
            delay = LOST_DATA_TIMEOUT;
            time = currentTimeMillis();
            break;
        case C_READ_TRACK:
            break;
        case C_WRITE_SECTOR:
            if(index >= (sec_mult ? (curDisk()->nsec - regs[R_SEC] + 1) : 1) * (128 << curDisk()->nsize)) { state = C_IDLE; break; }
            if(req & reqDRQ) {
                if((currentTimeMillis() - time) < delay) break;
                regs[R_STS] |= stsLostData;
            }
            curDisk()->file.write(&regs[R_DAT], 1);
            index++; req |= reqDRQ; regs[R_STS] |= stsDRQ;
            delay = LOST_DATA_TIMEOUT;
            time = currentTimeMillis();
            break;
        case C_WRITE_TRACK:
            if(req & reqDRQ) {
                if((currentTimeMillis() - time) < delay) break;
                regs[R_STS] |= stsLostData;
                state = C_IDLE;
            }
            delay = LOST_DATA_TIMEOUT;
            time = currentTimeMillis();
            if(regs[R_DAT] == 0x4E) formatCounter++; else formatCounter = 0;
            if(formatCounter > 400) { state = C_IDLE; break; }
            index++;
            req |= reqDRQ; regs[R_STS] |= stsDRQ;
            break;
    }
}

bool zxBeta128::openTRD(int active, const char *path) {
    if(active >= 0 && active < 4) {
        size_t length;
        auto trd_data = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length);
        if(!trd_data || !length) {
            LOG_DEBUG("Wrong TRD data", 0);
            return false;
        }
        uint8_t cyl_count = 0;
        uint8_t head_count = 0;
        // определяем конфигурацию диска из служебной информации
        switch(trd_data[0x08e3]) {
            case 0x16: cyl_count = 80; head_count = 2; break;
            case 0x17: cyl_count = 40; head_count = 2; break;
            case 0x18: cyl_count = 80; head_count = 1; break;
            case 0x19: cyl_count = 40; head_count = 1; break;
            // если служебная информация некорректная, то пытаемся определить конфигурацию исходя из размера файла
            default:
                switch(length) {
                    case 655360: cyl_count = 80; head_count = 2; break;
                    case 327680: cyl_count = 80; head_count = 1; break;
                    case 163840: cyl_count = 40; head_count = 1; break;
                    // если и размер нестандартный, то ошибка
                    default: LOG_DEBUG("Unknown TRD disk type", 0); return nullptr;
                }
                break;
        }
        auto dsk = &disks[active];
        dsk->file.close();
        dsk->nsize = 1; dsk->nsec = 16; dsk->nhead = head_count; dsk->ntrk = cyl_count;
        dsk->write = 1; dsk->track = 0; dsk->path = path;
        dsk->file.open(path, zxFile::open_read_write);
        return true;
    }
    return false;
}

uint8_t* zxBeta128::loadState(uint8_t *ptr) {
    return ptr;
}

uint8_t *zxBeta128::saveState(uint8_t *ptr) {
    return ptr;
}

uint8_t zxBeta128::vg_read(uint8_t addr) {
    uint8_t ret(0);
    exec();
    switch(addr) {
        case 0x1F: req &= ~reqINTRQ; ret = regs[R_STS]; break;
        case 0x3F: ret = regs[R_TRK]; break;
        case 0x5F: ret = regs[R_SEC]; break;
        case 0x7F: regs[R_STS] &= ~stsDRQ; req &= ~reqDRQ; ret = regs[R_DAT]; break;
        case 0xFF: ret = req;         break;
    }
    return ret;
}

void zxBeta128::vg_write(uint8_t addr, uint8_t val) {
    switch(addr) {
        case 0x1F: regs[R_CMD] = val; cmd_exec(val); break;
        case 0x3F: regs[R_TRK] = val; break;
        case 0x5F: regs[R_SEC] = val; break;
        case 0x7F: regs[R_STS] &= ~stsDRQ; req &= ~reqDRQ; regs[R_DAT] = val; break;
        case 0xFF: write_sys(val);    break;
    }
}
