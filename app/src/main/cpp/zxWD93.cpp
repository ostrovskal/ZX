//
// Created by Sergey on 10.02.2020.
//

#include "zxCommon.h"
#include "zxWD93.h"

#define Min(o, p)	(o < p ? o : p)

static int _rwlen;

static char buf_str[65536];
void log_to_data(bool is_text, const char* title, uint8_t* data, int trk, int sec, int head) {
    auto s = &buf_str[0];
    for(int i = 0 ; i < _rwlen; i++) {
        int dd = data[i];
        auto str = ssh_ntos(&dd, RADIX_HEX, nullptr);
        ssh_strcpy(&s, str);
        if(is_text) {
            auto vv = (uint8_t) ((dd < 32 || dd > 127) ? '.' : dd);
            *s++ = ',';
            *s++ = vv;
        }
        *s++ = ',';
    }
    *s++ = 0;
    zxFile::writeFile(title, buf_str, s - buf_str);
    LOG_INFO("%s t:%i s:%i h:%i -- %s", title, trk, sec, head, buf_str);

}

uint16_t CRC(uint8_t * src, int size) {
    uint32_t crc(0xcdb4);
    while(size--) {
        crc ^= (*src++) << 8;
        for(int i = 8; i; --i) { if((crc *= 2) & 0x10000) crc ^= 0x1021; }
    }
    return (uint16_t)crc;
}

int FDD::index() {
    return 0;//this - comp.wd.fdd;
}

void FDD::free() {
    delete [] rawdata; rawdata = 0;
    memset(this, 0, sizeof(FDD));
    //conf.trdos_wp[index()] = 0;
    //comp.wd.trkcache.clear();
    t.clear();
}

void FDD::newdisk(int cyls, int sides) {
    free();
    FDD::cyls = cyls; FDD::sides = sides;
    int len = MAX_TRACK_LEN;
    auto len2 = len + (len / 8) + ((len & 7) ? 1 : 0);
    rawsize = cyls * sides * len2;
    rawdata = new uint8_t[rawsize];
    for (int i = 0; i < cyls; i++)
        for (int j = 0; j < sides; j++) {
            trklen[i][j] = len;
            trkd[i][j] = rawdata + len2 * (i * sides + j);
            trki[i][j] = trkd[i][j] + len;
        }
}

void FDD::format_trd() {
    static const unsigned char lv[3][16] =
            { { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 },
              { 1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16 },
              { 1,12,7,2,13,8,3,14,9,4,15,10,5,16,11,6 } };

    newdisk(MAX_CYLS, 2);
    for (int c = 0; c < cyls; c++) {
        for (int side = 0; side < 2; side++) {
            t.seek(this, c, side, JUST_SEEK); t.s = 16;
            for(int sn = 0; sn < 16; sn++) {
                auto s = lv[0][sn];
                t.hdr[sn].n = s, t.hdr[sn].l = 1;
                t.hdr[sn].c = (uint8_t)c, t.hdr[sn].s = 0;
                t.hdr[sn].c1 = t.hdr[sn].c2 = 0;
                t.hdr[sn].data = (uint8_t*)1;
            }
            t.format();
        }
    }
}

void FDD::emptydisk() {
    format_trd();
    t.seek(this, 0, 0, LOAD_SECTORS);
    auto s8 = t.get_sector(9);
    if(!s8) return;
    s8->data[0xE2] = 1;                 // first free track
    s8->data[0xE3] = 0x16;              // 80T,DS
    *(short*)(s8->data+0xE5) = 2544;    // free sec
    s8->data[0xE7] = 0x10;              // trdos flag
    t.write_sector(9, s8->data);        // update sector CRC
}

int FDD::addfile(unsigned char *hdr, unsigned char *data) {
    t.seek(this, 0, 0, LOAD_SECTORS);
    auto s8 = t.get_sector(9);
    if (!s8) return 0;
    auto len = hdr[13];
    auto pos = s8->data[0xE4] * 0x10;
    auto dir = t.get_sector(1 + pos / 0x100);
    if (!dir) return 0;
    if (*(uint16_t *)(s8->data + 0xE5) < len) return 0;
    memcpy(dir->data + (pos & 0xFF), hdr, 14);
    *(short*)(dir->data + (pos & 0xFF) + 14) = *(short*)(s8->data+0xE1);
    t.write_sector(1+pos/0x100,dir->data);
    pos = s8->data[0xE1] + 16 * s8->data[0xE2];
    s8->data[0xE1] = (uint8_t)((pos + len) & 0x0F), s8->data[0xE2] = (uint8_t)((pos+len) >> 4);
    s8->data[0xE4]++;
    *(uint16_t *)(s8->data + 0xE5) -= len;
    t.write_sector(9,s8->data);
    for (int i = 0; i < len; i++, pos++) {
        t.seek(this, pos/32, (pos / 16) & 1, LOAD_SECTORS);
        if (!t.trkd) return 0;
        if (!t.write_sector((pos&0x0F) + 1, data + i * 0x100)) return 0;
    }
    return 1;
}

void FDD::addboot() {
    t.seek(this, 0, 0, LOAD_SECTORS);
    for (int s = 0; s < 8; s++) {
        auto sc = t.get_sector(s+1);
        if (!sc) return;
        for (int p = 0; p < 0x100; p += 0x10)
            if(!memcmp(sc->data + p, "boot    B", 9)) return;
    }
/*
    FILE *f = fopen(conf.appendboot, "rb");
    if (!f) return;
    fread(snbuf, 1, sizeof snbuf, f);
    fclose(f);
    snbuf[13] = snbuf[14]; // copy length
    addfile(snbuf, snbuf+0x11);
*/
}

void TRKCACHE::seek(FDD *d, int cyl, int side, SEEK_MODE fs) {
    if(!(((long)d - (long)drive) | (sf - fs) | (cyl - TRKCACHE::cyl) | (side - TRKCACHE::side))) return;
    drive = d; sf = fs; s = 0;
    TRKCACHE::cyl = cyl; TRKCACHE::side = side;
    if (cyl >= d->cyls || !d->rawdata) { trkd = 0; return; }
    trkd = d->trkd[cyl][side];
    trki = d->trki[cyl][side];
    trklen = d->trklen[cyl][side];
    if (!trklen) { trkd = 0; return; }
    ts_byte = Z80FQ / (trklen * FDD_RPS);
    if (fs == JUST_SEEK) return;
    for (int i = 0; i < trklen - 8; i++) {
        if (trkd[i] != 0xA1 || trkd[i + 1] != 0xFE || !test_i(i)) continue;
        if (s == MAX_SEC) { LOG_INFO("too many sectors", s); return; }
        SECHDR *h = &hdr[s++];
        h->id = trkd + i + 2; *(int*)h = *(int*)h->id;
        h->crc = *(uint16_t*)(trkd + i + 6);
        h->c1 = (uint8_t)(CRC(trkd + i + 1, 5) == h->crc);
        h->data = 0; h->datlen = 0;
        if (h->l > 5) continue;
        auto end = Min(trklen - 8, i + 8 + 43);
        for (int j = i + 8; j < end; j++) {
            if (trkd[j] != 0xA1 || !test_i(j) || test_i(j + 1)) continue;
            if (trkd[j + 1] == 0xF8 || trkd[j + 1] == 0xFB) {
                h->datlen = 128 << h->l;
                h->data = trkd+j + 2;
                h->c2 = (uint8_t)(CRC(h->data-1, h->datlen+1) == *(uint16_t*)(h->data + h->datlen));
            }
            break;
        }
    }
}

void TRKCACHE::format() {
    memset(trkd, 0, (size_t)trklen);
    memset(trki, 0, (size_t)(trklen / 8 + ((trklen & 7) ? 1 : 0)));
    auto *dst = trkd;
    int i;
    for (i = 0; i < 80; i++) *dst++ = 0x4E;
    for (i = 0; i < 12; i++) *dst++ = 0;
    for (i = 0; i < 3; i++) write((int)(dst++ - trkd), 0xC2, 1);
    *dst++ = 0xFC;
    for (int is = 0; is < s; is++) {
        for (i = 0; i < 50; i++) *dst++ = 0x4E;
        for (i = 0; i < 12; i++) *dst++ = 0;
        for (i = 0; i < 3; i++) write((int)(dst++ - trkd), 0xA1, 1);
        *dst++ = 0xFE;
        SECHDR *sechdr = hdr + is;
        *(int *)dst = *(int *)sechdr; dst += 4;
        auto crc = CRC(dst - 5, 5);
        if(sechdr->c1==1) crc = sechdr->crc;
        if(sechdr->c1==2) crc ^= 0xFFFF;
        *(int *)dst = crc; dst += 2;
        if (sechdr->data) {
            for (i = 0; i < 22; i++) *dst++ = 0x4E;
            for (i = 0; i < 12; i++) *dst++ = 0;
            for (i = 0; i < 3; i++) write((int)(dst++ - trkd), 0xA1, 1);
            *dst++ = 0xFB;
            if (sechdr->l > 5) { LOG_INFO("strange sector", 0); return; }
            auto len = 128 << sechdr->l;
            if (sechdr->data != (uint8_t*)1) memcpy(dst, sechdr->data, (size_t)len); else memset(dst, 0, (size_t)len);
            crc = CRC(dst - 1, len + 1);
            if (sechdr->c2 == 1) crc = sechdr->crcd;
            if (sechdr->c2 == 2) crc ^= 0xFFFF;
            *(int *)(dst + len) = crc; dst += len+2;
        }
    }
    if (dst > trklen + trkd) { LOG_INFO("track too long", 0); return; }
    while (dst < trkd + trklen) *dst++ = 0x4E;
}

int TRKCACHE::write_sector(int sec, uint8_t *data) {
    auto h = get_sector(sec);
    if (!h || !h->data) return 0;
    auto sz = h->datlen;
    memcpy(h->data, data, (size_t)sz);
    *(uint16_t*)(h->data + sz) = CRC(h->data - 1, sz + 1);
    return sz;
}

SECHDR *TRKCACHE::get_sector(int sec) {
    int i;
    for (i = 0; i < s; i++) if (hdr[i].n == sec) break;
    if (i == s) return 0;
    if (hdr[i].l != 1 || hdr[i].c != cyl) return 0;
    return &hdr[i];
}

void WD1793::cmdVerify() {
    LOG_INFO("", 0);
    if(!(cmd & CMD_SEEK_VERIFY)) state = S_IDLE;
    else {
        end_waiting_am = next + 6 * Z80FQ / FDD_RPS;
        load(); find_marker();
    }
}

void WD1793::cmdSeek() {
    LOG_INFO("", 0);
    if(data == track) state = S_VERIFY;
    else {
        stepdirection = (int8_t)((data < track) ? -1 : 1);
        state = S_STEP;
    }
}

void WD1793::cmdStep() {
    LOG_INFO("", 0);
//    static const uint8_t steps[] = { 6,12,20,30 };
    static const uint8_t steps[] = { 6, 6, 6, 6 };
//    trdos_seek = ROMLED_TIME;
    if(!seldrive->track && !(cmd & 0xF0)) { track = 0; state = S_VERIFY; return; }
    if(!(cmd & 0xE0) || (cmd & CMD_SEEK_TRKUPD)) track += stepdirection;
    seldrive->track += stepdirection;
    if(seldrive->track == (uint8_t)-1) seldrive->track = 0;
    if(seldrive->track >= MAX_PHYS_CYL) seldrive->track = MAX_PHYS_CYL;
    trkcache.clear();
    next += steps[cmd & CMD_SEEK_RATE] * Z80FQ / 1000;
    state2 = (cmd & 0xE0)? S_VERIFY : S_SEEK;
    state = S_WAIT;
}

void WD1793::cmdType1() {
    LOG_INFO("", 0);
    status = (status | WDS_BUSY) & ~(WDS_DRQ | WDS_CRCERR | WDS_SEEKERR | WDS_WRITEP);
    rqs = 0;
    //if (conf.trdos_wp[drive]) status |= WDS_WRITEP;
    seldrive->motor = next + 2*Z80FQ;
    state2 = S_SEEKSTART;
    if (cmd & 0xE0) {
        // single step
        if (cmd & 0x40) stepdirection = (int8_t)((cmd & CMD_SEEK_DIR) ? -1 : 1);
        state2 = S_STEP;
    }
    next += 1 * Z80FQ / 1000;
    state = S_WAIT;
}

void WD1793::cmdWriteTrackData() {
    if(notready()) return;
//    trdos_format = ROMLED_TIME;
    if (rqs & DRQ) status |= WDS_LOST, data = 0;
    trkcache.seek(seldrive, seldrive->track, side, JUST_SEEK);
    trkcache.sf = JUST_SEEK; // invalidate sectors
    uint8_t marker = 0, byte = data;
    uint16_t crc(0);
    if(data == 0xF5) byte = 0xA1, marker = 1, start_crc = rwptr + 1;
    if(data == 0xF6) byte = 0xC2, marker = 1;
    if(data == 0xF7) crc = CRC(trkcache.trkd + start_crc, rwptr - start_crc), byte = (uint8_t)(crc & 0xFF);
    trkcache.write(rwptr++, byte, marker); rwlen--;
    if(data == 0xF7) trkcache.write(rwptr++, (uint8_t)(crc >> 8), 0), rwlen--;
    if(rwlen > 0) {
        next += trkcache.ts_byte;
        state2 = S_WR_TRACK_DATA; state = S_WAIT;
        rqs = DRQ; status |= WDS_DRQ;
    } else {
        LOG_INFO("FINISH %i", seldrive->track);
        log_to_data(false, "format", trkcache.trkd, seldrive->track, 0, 0);
        state = S_IDLE;
    }
}

void WD1793::cmdWriteTrack() {
    LOG_INFO("", 0);
    if(rqs & DRQ) {
        status |= WDS_LOST; state = S_IDLE;
    } else {
        seldrive->optype |= 2;
        state2 = S_WR_TRACK_DATA;
        getindex();
        end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
        _rwlen = rwlen;
        LOG_INFO("START %i idx: %i len: %i", seldrive->track, rwptr, rwlen);
    }
}

void WD1793::cmdWrite() {
    if(notready()) return;
    LOG_INFO("", 0);
    if(rqs & DRQ) status |= WDS_LOST, data = 0;
    //trdos_save = ROMLED_TIME;
    trkcache.write(rwptr++, data, 0); rwlen--;
    if(rwptr == trkcache.trklen) rwptr = 0;
    trkcache.sf = JUST_SEEK; // invalidate sectors
    if(rwlen) {
        next += trkcache.ts_byte;
        state = S_WAIT; state2 = S_WRITE;
        rqs = DRQ; status |= WDS_DRQ;
    } else {
        auto len = (128 << trkcache.hdr[foundid].l) + 1;
        uint8_t sc[2056];
        if (rwptr < len) memcpy(sc, trkcache.trkd + trkcache.trklen - rwptr, (size_t)rwptr), memcpy(sc + rwptr, trkcache.trkd, (size_t)(len - rwptr));
        else memcpy(sc, trkcache.trkd + rwptr - len, (size_t)len);
        auto crc = CRC(sc, len);
        trkcache.write(rwptr++, (uint8_t)crc, 0);
        trkcache.write(rwptr++, (uint8_t)(crc >> 8), 0);
        trkcache.write(rwptr, 0xFF, 0);
        if(cmd & CMD_MULTIPLE) sector++, state = S_CMD_RW; else state = S_IDLE;
    }
}

void WD1793::cmdWriteSec() {
    LOG_INFO("", 0);
    load();
    if(rqs & DRQ) {
        status |= WDS_LOST; state = S_IDLE;
    } else {
        seldrive->optype |= 1;
        rwptr = (int)(trkcache.hdr[foundid].id + 6 + 11 + 11 - trkcache.trkd);
        for(rwlen = 0; rwlen < 12; rwlen++) trkcache.write(rwptr++, 0, 0);
        for(rwlen = 0; rwlen < 3; rwlen++) trkcache.write(rwptr++, 0xA1, 1);
        trkcache.write(rwptr++, (uint8_t)((cmd & CMD_WRITE_DEL) ? 0xF8 : 0xFB), 0);
        rwlen = 128 << trkcache.hdr[foundid].l;
        state = S_WRITE;
    }
}

void WD1793::cmdRead() {
    if(notready()) return;
    LOG_INFO("", 0);
    load();
    if (rwlen) {
//        trdos_load = ROMLED_TIME;
        if (rqs & DRQ) status |= WDS_LOST;
        data = trkcache.trkd[rwptr++]; rwlen--;
        rqs = DRQ; status |= WDS_DRQ;
        next += trkcache.ts_byte;
        state = S_WAIT;
        state2 = S_READ;
    } else {
        if ((cmd & 0xE0) == 0x80) {
            // read sector
            if(!trkcache.hdr[foundid].c2) status |= WDS_CRCERR;
            if(cmd & CMD_MULTIPLE) { sector++, state = S_CMD_RW; return; }
        }
        if ((cmd & 0xF0) == 0xC0) {
            // read address
            if (!trkcache.hdr[foundid].c1) status |= WDS_CRCERR;
        }
        state = S_IDLE;
    }
}

void WD1793::cmdFindSec() {
    LOG_INFO("", 0);
    if(!seldrive->rawdata) {
        end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
        find_marker();
        return;
    }
    if(next >= end_waiting_am || foundid == -1) { status |= WDS_NOTFOUND; state = S_IDLE; return; }
    status &= ~WDS_CRCERR;
    load();
    if (!(cmd & 0x80)) {
        // verify after seek
        if(trkcache.hdr[foundid].c != track) { find_marker(); return; }
        if(!trkcache.hdr[foundid].c1) { status |= WDS_CRCERR; find_marker(); return; }
        state = S_IDLE;
        return;
    }
    if ((cmd & 0xF0) == 0xC0) {
        rwlen = 6;
        // next byte
        rwptr = (int)(trkcache.hdr[foundid].id - trkcache.trkd);
        data = trkcache.trkd[rwptr++]; rwlen--;
        rqs = DRQ; status |= WDS_DRQ;
        next += trkcache.ts_byte;
        state = S_WAIT; state2 = S_READ;
        return;
    }
    if(trkcache.hdr[foundid].c != track || trkcache.hdr[foundid].n != sector) { find_marker(); return; }
    if((cmd & CMD_SIDE_CMP_FLAG) && (((cmd >> CMD_SIDE_SHIFT) ^ trkcache.hdr[foundid].s) & 1)) { find_marker(); return; }
    if(!trkcache.hdr[foundid].c1) { status |= WDS_CRCERR; find_marker(); return; }
    if (cmd & 0x20) {
        // write sector(s)
        rqs = DRQ; status |= WDS_DRQ;
        next += trkcache.ts_byte*9;
        state = S_WAIT; state2 = S_WRSEC;
        return;;
    }
    // read sector(s)
    if(!trkcache.hdr[foundid].data) { find_marker(); return; }
    if(trkcache.hdr[foundid].data[-1] == 0xF8) status |= WDS_RECORDT; else status &= ~WDS_RECORDT;
    // next byte
    rwptr = (int)(trkcache.hdr[foundid].data - trkcache.trkd);
    rwlen = 128 << trkcache.hdr[foundid].l;
    data = trkcache.trkd[rwptr++]; rwlen--;
    rqs = DRQ; status |= WDS_DRQ;
    next += trkcache.ts_byte;
    state = S_WAIT; state2 = S_READ;
}

void WD1793::cmdPrepare() {
    LOG_INFO("", 0);
    if((cmd & CMD_DELAY)) next += (Z80FQ * 15 / 1000);
    status = (status | WDS_BUSY) & ~(WDS_DRQ | WDS_LOST | WDS_NOTFOUND | WDS_RECORDT | WDS_WRITEP);
    state2 = S_CMD_RW; state = S_WAIT;
}

void WD1793::cmdReadWrite() {
    LOG_INFO("", 0);
/*
    if(((cmd & 0xE0) == 0xA0 || (cmd & 0xF0) == 0xF0) && conf.trdos_wp[drive]) {
        status |= WDS_WRITEP;
        state = S_IDLE;
        return;
    }
*/
    if ((cmd & 0xC0) == 0x80 || (cmd & 0xF8) == 0xC0) {
        end_waiting_am = next + (5 * Z80FQ / FDD_RPS);
        find_marker();
        return;
    }
    if((cmd & 0xF8) == 0xF0) {
        rqs = DRQ; status |= WDS_DRQ;
        next += 3 * trkcache.ts_byte;
        state2 = S_WRTRACK; state = S_WAIT;
        return;
    }
    if ((cmd & 0xF8) == 0xE0) {
        load(); rwptr = 0; rwlen = trkcache.trklen;
        state2 = S_READ; getindex();
        return;
    }
    state = S_IDLE;
}

void WD1793::process() {
    time = (uint64_t)zxULA::_TICK;
    // inactive drives disregard HLT bit
    if (time > seldrive->motor && (system & 0x08)) seldrive->motor = 0;
    if (seldrive->rawdata) status &= ~WDS_NOTRDY; else status |= WDS_NOTRDY;
    if (!(cmd & 0x80)) {
        // seek / step commands
        status &= ~(WDS_TRK00 | WDS_INDEX);
        if(seldrive->motor && (system & 0x08)) status |= WDS_HEADL;
        if(!seldrive->track) status |= WDS_TRK00;
        // todo: test spinning
        if(seldrive->rawdata && seldrive->motor && ((time + tshift) % (Z80FQ / FDD_RPS) < (Z80FQ * 4 / 1000)))
            // index every turn, len=4ms (if disk present)
            status |= WDS_INDEX;
    }
    while(true) {
        switch (state) {
            case S_IDLE: status &= ~WDS_BUSY; rqs = INTRQ; return;
            case S_WAIT: if (time < next) return;
            state = state2; break;
            case S_PREPARE_CMD: cmdPrepare(); break;
            case S_CMD_RW: cmdReadWrite(); break;
            case S_FIND_SEC: cmdFindSec(); break;
            case S_READ: cmdRead(); break;
            case S_WRSEC: cmdWriteSec(); break;
            case S_WRITE: cmdWrite(); break;
            case S_WRTRACK: cmdWriteTrack(); break;
            case S_WR_TRACK_DATA: cmdWriteTrackData(); break;
            case S_TYPE1_CMD: cmdType1(); break;
            case S_STEP: cmdStep(); break;
            case S_SEEKSTART: if (!(cmd & 0x10)) track = 0xFF, data = 0;
            case S_SEEK: cmdSeek(); break;
            case S_VERIFY: cmdVerify(); break;
//            case S_RESET: cmdReset(); break;
//            default: //errexit("WD1793 in wrong state");
        }
    }
}

void WD1793::find_marker() {
    load();
    foundid = -1; int wait = 10 * Z80FQ / FDD_RPS;
    if (seldrive->motor && seldrive->rawdata) {
        auto div = trkcache.trklen * trkcache.ts_byte;
        auto i = (uint32_t)((next + tshift) % div) / trkcache.ts_byte;
        wait = -1;
        for(int is = 0; is < trkcache.s; is++) {
            auto pos  = trkcache.hdr[is].id - trkcache.trkd;
            auto dist = (pos > i)? pos-i : trkcache.trklen+pos-i;
            if (dist < wait) wait = (int)dist, foundid = is;
        }
        if (foundid != -1) wait *= trkcache.ts_byte; else wait = 10 * Z80FQ / FDD_RPS;
    }
    next += wait;
    if(seldrive->rawdata && next > end_waiting_am) next = end_waiting_am, foundid = -1;
    state = S_WAIT; state2 = S_FIND_SEC;
}

char WD1793::notready() {
    if (!(rqs & DRQ)) return 0;
    if (next > end_waiting_am) return 0;
    state2 = state; state = S_WAIT;
    next += trkcache.ts_byte;
    return 1;
}

void WD1793::getindex() {
    auto trlen = trkcache.trklen*trkcache.ts_byte;
    auto ticks = (uint32_t)((next+tshift) % trlen);
    next += (trlen - ticks);
    rwptr = 0; rwlen = trkcache.trklen; state = S_WAIT;
}

void WD1793::load() {
    trkcache.seek(seldrive, seldrive->track, side, LOAD_SECTORS);
}

uint8_t WD1793::vg93_read(uint8_t port, int tact) {
    process();
    if(port &  0x80) return (uint8_t)(rqs | 0x3F);
    if(port == 0x1F) { rqs &= ~INTRQ; return status; }
    if(port == 0x3F) return track;
    if(port == 0x5F) return sector;
    if(port == 0x7F) { status &= ~WDS_DRQ; rqs &= ~DRQ; return data; }
    return 0xFF;
}

void WD1793::vg93_write(uint8_t port, uint8_t val, int tact) {
    process();
    if(port == 0x1F) {
        if ((val & 0xF0) == 0xD0) {
            state = S_IDLE; rqs = INTRQ;
            status &= ~WDS_BUSY;
            return;
        }
        if (status & WDS_BUSY) return;
        cmd = val; next = (uint64_t)zxULA::_TICK;
        status |= WDS_BUSY; rqs = 0;
        if (cmd & 0x80) {
            if (status & WDS_NOTRDY) {
                state = S_IDLE; rqs = INTRQ;
                return;
            }
            if (seldrive->motor) seldrive->motor = next + 2 * Z80FQ;
            state = S_PREPARE_CMD;
            return;
        }
        state = S_TYPE1_CMD;
        return;
    }
    if (port == 0x3F) { track = val; return; }
    if (port == 0x5F) { sector = val; return; }
    if (port == 0x7F) { data = val, rqs &= ~DRQ, status &= ~WDS_DRQ; return; }
    if (port & 0x80) {
        system = val;
        drive = val & 3, side = 1 & ~(val >> 4);
        seldrive = fdd + drive;
        trkcache.clear();
        if(!(val & 0x04)) {
            status = WDS_NOTRDY;
            rqs = INTRQ;
            seldrive->motor = 0;
            state = S_IDLE;
        }
    }
}
/*

void WD1793::trdos_traps() {
    unsigned pc = (cpu.pc & 0xFFFF);
    if (pc < 0x3E01) return;

    if (pc == 0x3E01 && bankr[0][0x3E01] == 0x0D) { cpu.a = cpu.c = 1; return; } // no delays
    if (pc == 0x3FEC && bankr[0][0x3FED] == 0xA2 &&
        (state == S_READ || (state2 == S_READ && state == S_WAIT))) {
        trdos_load = ROMLED_TIME;
        if (rqs & DRQ) {
            z80dbg::wm(cpu.hl, data); // move byte from controller
            cpu.hl++, cpu.b--;
            rqs &= ~DRQ; status &= ~WDS_DRQ;
        }
        while (rwlen) { // move others
            z80dbg::wm(cpu.hl, trkcache.trkd[rwptr++]); rwlen--;
            cpu.hl++; cpu.b--;
        }
        cpu.pc += 2; // skip INI
        return;
    }
    if (pc == 0x3FD1 && bankr[0][0x3FD2] == 0xA3 &&
        (rqs & DRQ) && (rwlen>1) && (state == S_WRITE || (state2 == S_WRITE && state == S_WAIT))) {
        trdos_save = ROMLED_TIME;
        while (rwlen > 1) {
            trkcache.write(rwptr++, z80dbg::rm(cpu.hl), 0); rwlen--;
            cpu.hl++; cpu.b--;
        }
        cpu.pc += 2; // skip OUTI
        return;
    }
}
*/
