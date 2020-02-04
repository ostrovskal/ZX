//
// Created by Sergey on 03.02.2020.
//

#include "zxCommon.h"
#include "zxFDD.h"

const int trdos_interleave = 1;
static const char* boot_sign = "boot    B";
static u_long Z80FQ = 3500000;

#define Min(o, p)	(o < p ? o : p)

void zxFDD::seek(int _trk, int _head) {
    if(disk) {
        trk = _trk;
        head = _head;
        ts = Z80FQ / (get_trk()->len * FDD_RPS);
    }
}

void zxDisk::TRACK::write(int pos, uint8_t v, bool marker) {
    if(data) {
        data[pos] = v;
        if(marker) id[pos / 8] |= 1 << (pos & 7); else id[pos / 8] &= ~(1 << (pos & 7));
    }
}

void zxDisk::TRACK::update() {
    uint8_t* src = data;
    int l = len - 8;
    total_sec = 0;
    int i = 0;
    while(i < l) {
        for(; i < len; ++i) {
            // find index data marker
            if(src[i] == 0xa1 && src[i+1] == 0xfe && marker(i)) {
                sectors[total_sec].id = src + i + 2;
                sectors[total_sec].data = nullptr;
                i += 8;
                break;
            }
        }
        // opts[TRDOS_DAT] marker margin 30-SD, 43-DD
        int end = Min(len, i + 43);
        for(; i < end; ++i) {
            // find opts[TRDOS_DAT] marker
            if(src[i] == 0xa1 && marker(i) && !marker(i + 1)) {
                if((i < len && src[i + 1] == 0xf8) || src[i + 1] == 0xfb) sectors[total_sec].data = src + i + 2;
                break;
            }
        }
        //if(total_sec++ >= MAX_SEC) {} // слишком много секторов
    }
}

zxDisk::zxDisk(int _trks, int _heads){
    const int max_track_len = 6250;

    trks = _trks; heads = _heads;
    int data_len = max_track_len;
    int track_len = data_len + data_len / 8 + ((data_len & 7) != 0); // 7032
    int size = _trks * _heads * track_len;
    raw = new uint8_t[size];
    memset(raw, 0, (size_t)size);
    for(int i = 0; i < _trks; ++i) {
        for(int j = 0; j < _heads; ++j) {
            auto t = &tracks[i][j];
            t->len = data_len;
            t->data = raw + track_len * (i * _heads + j);
            t->id = t->data + data_len;
        }
    }
}

bool zxFDD::open(const void* data, size_t data_size, int type) {
    engine(0);
    bool ret = false;
    switch(type) {
        case ZX_CMD_IO_SCL: ret = read_scl(data, data_size); break;
        case ZX_CMD_IO_FDI: ret = read_fdi(data, data_size); break;
        case ZX_CMD_IO_TRD: ret = read_trd(data, data_size); break;
    }
    if(!ret) {
        // удалить образ
        SAFE_DELETE(disk);
    }
    return ret;
}

bool zxFDD::is_boot() {
    for(int i = 0; i < 9; ++i) {
        auto s = get_sec(0, 0, i);
        if(!s) continue;
        auto ptr = s->data;
        const char* b = boot_sign;
        for(; (ptr = (uint8_t*)memchr(ptr, *b, s->len() - (ptr - s->data))) != nullptr; ++b) {
            if(!*b) return true;
        }
    }
    return false;
}

// Рассогласование данных по боевым функциям АРМ
static uint16_t sec_dataW(zxDisk::TRACK::SECTOR* s, size_t offset) { return wordLE(s->data + offset); }

static void sec_dataW(zxDisk::TRACK::SECTOR* s, size_t offset, uint16_t d) {
    s->data[offset] = (uint8_t)(d & 0xff);
    s->data[offset + 1] = (uint8_t)(d >> 8);
}

uint16_t zxFDD::CRC(uint8_t * src, int size) const {
    uint32_t crc(0xcdb4);
    while(size--) {
        crc ^= (*src++) << 8;
        // x^12+x^5+1
        for(int i = 8; i; --i) { if((crc *= 2) & 0x10000) crc ^= 0x1021; }
    }
    return (uint16_t)crc;
}

bool zxFDD::write_sec(int trk, int head, int sec, const uint8_t* data) {
    auto s = get_sec(trk, head, sec);
    if(!s || !s->data) return false;
    memcpy(s->data, data, (size_t)s->len());
    update_crc(s);
    return true;
}

zxDisk::TRACK::SECTOR* zxFDD::get_sec(int trk, int head, int sec) {
    seek(trk, head);
    for(int i = 0; i < get_trk()->total_sec; ++i) {
        auto s = get_sec(i);
        auto sc = s->sec();
        auto l = s->len();
        if(sc == sec && l == 256) return s;
    }
    return nullptr;
}

void zxFDD::make_trd() {
    const int max_trd_sectors = 16;
    static const uint8_t lv[3][max_trd_sectors] = {
            { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, 
            { 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15, 8, 16 }, 
            { 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5, 16, 11, 6 }
    };

    SAFE_DELETE(disk);
    disk = new zxDisk(MAX_TRK, MAX_HEAD);
    for(int i = 0; i < disk->trks; ++i) {
        for(int j = 0; j < disk->heads; ++j) {
            seek(i, j);
            auto t = get_trk(); auto l = t->len; auto d = t->data;
            int pos = 0;
            int id_len = l / 8 + ((l & 7) != 0);
            memset(d, 0, (size_t)(l + id_len));
            // gap4a sync iam
            write_blk(pos, 0x4e, 80); write_blk(pos, 0, 12); write_blk(pos, 0xc2, 3, true); write(pos++, 0xfc);
            t->total_sec = max_trd_sectors;
            for(int n = 0; n < max_trd_sectors; ++n) {
                // gap1 sync am
                write_blk(pos, 0x4e, 40); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfe);
                auto sec = get_sec(n);
                sec->id = d + pos;
                write(pos++, (uint8_t)trk); write(pos++, (uint8_t)head); write(pos++, lv[trdos_interleave][n]); write(pos++, 1);
                auto crc = CRC(d + pos - 5, 5);
                write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
                // gap2 sync am
                write_blk(pos, 0x4e, 22); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfb);
                sec->data = d + pos;
                int len = sec->len();
                crc = CRC(d + pos - 1, len + 1);
                pos += len;
                write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
            }
            // дорожек слишком много
            if(pos > t->len) { assert(0); }
            write_blk(pos, 0x4e, l - pos - 1); //gap3
        }
    }
    auto s = get_sec(0, 0, 9);
    if(!s) return;
    s->data[0xe2] = 1;					// first free track
    s->data[0xe3] = 0x16;				// 80T,DS
    sec_dataW(s, 0xe5, 2544);			// free sec
    s->data[0xe7] = 0x10;				// trdos flag
    update_crc(s);
}

void zxFDD::update_crc(zxDisk::TRACK::SECTOR *s) const {
    auto len = s->len();
    sec_dataW(s, (size_t)len, swap_byte_order(CRC(s->data - 1, len + 1)));
}

bool zxFDD::add_file(const uint8_t *hdr, const uint8_t *data) {
    auto s = get_sec(0, 0, 9);
    if(!s) return false;
    int len = hdr[13];
    int pos = s->data[0xe4] * 0x10;
    auto* dir = get_sec(0, 0, 1 + pos / 0x100);
    if(!dir) return false;
    // disk full
    if(sec_dataW(s, 0xe5) < len) return false;
    memcpy(dir->data + (pos & 0xff), hdr, 14);
    sec_dataW(dir, (size_t)((pos & 0xff) + 14), sec_dataW(s, 0xe1));
    update_crc(dir);

    pos = s->data[0xe1] + 16 * s->data[0xe2];
    s->data[0xe1] = (uint8_t)((pos + len) & 0x0f);
    s->data[0xe2] = (uint8_t)((pos + len) >> 4);
    s->data[0xe4]++;
    sec_dataW(s, 0xe5, (uint16_t)(sec_dataW(s, 0xe5) - len));
    update_crc(s);

    // goto next track. s8 become invalid
    for(int i = 0; i < len; ++i, ++pos) {
        int t = pos / 32;
        int h = (pos / 16) & 1;
        if(!write_sec(t, h, (pos & 0x0f) + 1, data + i * 0x100)) return false;
    }
    return true;
}

bool zxFDD::read_scl(const void* data, size_t data_size) {
    if(data_size < 9) return false;
    auto buf = (const uint8_t*)data;
    if(memcmp(data, "SINCLAIR", 8) || int(data_size) < (9 + 270 * buf[8])) return false;

    make_trd();
    int size = 0;
    for(int i = 0; i < buf[8]; ++i) size += buf[14 * i + 22];
    if(size > 2544) {
        auto s = get_sec(0, 0, 9);
        // free sec
        sec_dataW(s, 0xe5, (uint16_t)size);
        update_crc(s);
    }
    auto d = buf + 9 + 14 * buf[8];
    for(int i = 0; i < buf[8]; ++i) {
        if(!add_file(buf + 9 + 14 * i, d)) return false;
        d += buf[14 * i + 22] * 256;
    }
    return true;
}

bool zxFDD::read_trd(const void *data, size_t data_size){
    make_trd();
    enum { TRD_SIZE = 655360 };
    if(data_size > TRD_SIZE) data_size = TRD_SIZE;
    for(size_t i = 0; i < data_size; i += 0x100) {
        write_sec(i >> 13, (i >> 12) & 1, ((i >> 8) & 0x0f) + 1, (const uint8_t *)data + i);
    }
    return true;
}

bool zxFDD::read_fdi(const void *data, size_t data_size){
    delete disk;
    auto buf = (const uint8_t*)data;
    disk = new zxDisk(buf[4], buf[6]);

    //auto offsDesc = *(uint16_t*)(buf + 0x08);
    auto offsSecs = *(uint16_t*)(buf + 0x0A);
    auto offsTrks = *(uint16_t*)(buf + 0x0C);
    auto trks = buf + 0x0E + offsTrks;//wordLE(buf + 0x0C);
    auto dat = buf + offsSecs;//wordLE(buf + 0x0A);

    for(int i = 0; i < disk->trks; ++i) {
        for(int j = 0; j < disk->heads; ++j) {
            seek(i, j);
            auto t = get_trk(); auto l = t->len; auto d = t->data;
            int id_len = l / 8 + ((l & 7) != 0);
            memset(d, 0, (size_t)(l + id_len));

            int pos = 0;
            // gap4a sync iam
            write_blk(pos, 0x4e, 80); write_blk(pos, 0, 12); write_blk(pos, 0xc2, 3, true);
            write(pos++, 0xfc);

            auto t0 = dat + Dword(trks);
            int ns = trks[6];
            t->total_sec = ns;
            trks += 7;
            for(int n = 0; n < ns; ++n) {
                // gap1 sync am
                write_blk(pos, 0x4e, 40); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfe);
                auto sec = get_sec(n);
                sec->id = d + pos;
                write(pos++, trks[0]); write(pos++, trks[1]); write(pos++, trks[2]); write(pos++, trks[3]);
                auto crc = CRC(d + pos - 5, 5);
                write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
                if(trks[4] & 0x40) sec->data = nullptr;
                else {
                    auto data1 = (t0 + wordLE(trks + 5));
                    if(data1 + 128 > buf + data_size) return false;
                    // gap2 sync am
                    write_blk(pos, 0x4e, 22); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfb);
                    sec->data = d + pos;
                    auto len = sec->len();
                    memcpy(sec->data, data1, (size_t)len);
                    crc = CRC(d + pos - 1, len + 1);
                    if(!(trks[4] & (1 << (trks[3] & 3)))) crc ^= 0xffff;
                    pos += len;
                    write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
                }
                trk += 7;
            }
            if(pos > get_trk()->len) { assert(0); }
            write_blk(pos, 0x4e, get_trk()->len - pos - 1); //gap3
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  VG93                                                                 //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const uint16_t  crc_initial = 0xcdb4;

zxVG93::zxVG93() : next(0), tshift(0), state(S_IDLE), state_next(S_IDLE),
                   head(0), direction(0), rqs(R_NONE),
                   system(0), end_waiting_am(0), found_sec(nullptr), rwptr(0), rwlen(0), crc(0), start_crc(-1) {
    Z80FQ = 3500000;
    wd93_nodelay = false;
    fdd = fdds;
    opts[TRDOS_CMD] = opts[TRDOS_DAT] = opts[TRDOS_TRK] = opts[TRDOS_SEC] = opts[TRDOS_STS] = 0;
}

bool zxVG93::open(const char* path, int drive, int type) {
    if(drive >= 0 && drive < 4) {
        size_t length; uint8_t* ptr;
        if((ptr = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length))) {
            int current_fdd;
            for(current_fdd = 4; --current_fdd >= 0;) {
                if(fdd == &fdds[current_fdd]) break;
            }
            if (drive == current_fdd) {
                found_sec = nullptr;
                opts[TRDOS_STS] = ST_NOTRDY;
                rqs = R_INTRQ;
                state = S_IDLE;
            }
            return fdds[drive].open(ptr, length, type);
        }
    }
    return false;
}

uint16_t zxVG93::CRC(uint8_t v, uint16_t prev_crc) const {
    auto c = prev_crc ^ (v << 8);
    for(int i = 8; i; --i) { if((c <<= 1) & 0x10000) c ^= 0x1021; }
    return c;
}

uint16_t zxVG93::CRC(uint8_t* src, int size) const {
    auto c = crc_initial;
    while(size--) c = CRC(*src++, c);
    return c;
}

void zxVG93::exec(int tact){
    auto time = ALU->_TICK + tact;
    // Неактивные диски игнорируют бит HLT
    if(time > fdd->engine() && (system & 0x08)) fdd->engine(0);
    fdd->is_disk() ? opts[TRDOS_STS] &= ~ST_NOTRDY : opts[TRDOS_STS] |= ST_NOTRDY;
    //seek/step commands
    if(!(opts[TRDOS_CMD] & 0x80)) {
        opts[TRDOS_STS] &= ~(ST_TRK00 | ST_INDEX);
        if(fdd->engine() && (system & CB_SYS_HLT)) opts[TRDOS_STS] |= ST_HEADL;
        if(!fdd->track()) opts[TRDOS_STS] |= ST_TRK00;
        // index every turn, len=4ms (if disk present)
        if(fdd->is_disk() && fdd->engine() && ((time + tshift) % (Z80FQ / FDD_RPS) < (Z80FQ * 4 / 1000))) opts[TRDOS_STS] |= ST_INDEX;
    }
    for(;;) {
        switch (state) {
            // шина свободна. команда завершена
            case S_IDLE: opts[TRDOS_STS] &= ~ST_BUSY; rqs = R_INTRQ; return;
            case S_WAIT: if(time < next) return; state = state_next; break;
            case S_DELAY_BEFORE_CMD:
                // задержка 15мс
                if(!wd93_nodelay && (opts[TRDOS_CMD] & CB_DELAY)) next += (Z80FQ * 15 / 1000);
                opts[TRDOS_STS] = (opts[TRDOS_STS] | ST_BUSY) & ~(ST_DRQ | ST_LOST | ST_NOT_SEC | ST_RECORDT | ST_WRITEP);
                state = S_WAIT; state_next = S_CMD_RW;
                break;
            case S_CMD_RW:
                if(((opts[TRDOS_CMD] & 0xe0) == 0xa0 || (opts[TRDOS_CMD] & 0xf0) == 0xf0) && fdd->is_protect()) { opts[TRDOS_STS] |= ST_WRITEP; state = S_IDLE; break; }
                //read/write sectors or read am - find next AM
                if((opts[TRDOS_CMD] & 0xc0) == 0x80 || (opts[TRDOS_CMD] & 0xf8) == 0xc0) {
                    end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
                    find_marker();
                    break;
                }
                // запись дорожки(форматирование)
                if((opts[TRDOS_CMD] & 0xf8) == 0xf0) {
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    next += 3 * fdd->ticks();
                    state = S_WAIT; state_next = S_WRTRACK;
                    break;
                }
                // чтение дорожки
                if((opts[TRDOS_CMD] & 0xf8) == 0xe0) {
                    load();
                    rwptr = 0;
                    rwlen = fdd->get_trk()->len;
                    state_next = S_READ;
                    get_index();
                    break;
                }
                state = S_IDLE;
                break;
            case S_FOUND_NEXT_ID: {
                //no disk - wait again
                if (!fdd->is_disk()) {
                    end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
                    find_marker();
                    break;
                }
                if (next >= end_waiting_am || !found_sec) {
                    opts[TRDOS_STS] |= ST_NOT_SEC;
                    state = S_IDLE;
                    break;
                }
                opts[TRDOS_STS] &= ~ST_CRCERR;
                load();
                // проверка после поиска
                if (!(opts[TRDOS_CMD] & 0x80)) {
                    if (found_sec->trk() != opts[TRDOS_TRK]) { find_marker(); break; }
                    if (CRC(found_sec->id - 1, 5) != found_sec->crcId()) { opts[TRDOS_STS] |= ST_CRCERR; find_marker(); break; }
                    state = S_IDLE;
                    break;
                }
                // read AM
                if ((opts[TRDOS_CMD] & 0xf0) == C_RADR) {
                    rwptr = (int) (found_sec->id - fdd->get_trk()->data);
                    rwlen = 6;
                    read_first_byte();
                    break;
                }
                // else R/W sector(s)
                if (found_sec->trk() != opts[TRDOS_TRK] || found_sec->sec() != opts[TRDOS_SEC] ||
                    ((opts[TRDOS_CMD] & CB_SIDE_CMP) && (((opts[TRDOS_CMD] >> CB_SIDE_SHIFT) ^ found_sec->head()) & 1))) { find_marker(); break; }
                if (CRC(found_sec->id - 1, 5) != found_sec->crcId()) { opts[TRDOS_STS] |= ST_CRCERR; find_marker(); break; }
                // запись сектора
                if (opts[TRDOS_CMD] & 0x20) {
                    rqs = R_DRQ;
                    opts[TRDOS_STS] |= ST_DRQ;
                    next += fdd->ticks() * 9;
                    state = S_WAIT;
                    state_next = S_WRSEC;
                    break;
                }
                // чтение сектора
                if (!found_sec->data) { find_marker(); break; }
                found_sec->data[-1] == 0xf8 ? opts[TRDOS_STS] |= ST_RECORDT : opts[TRDOS_STS] &= ~ST_RECORDT;
                rwptr = (int) (found_sec->data - fdd->get_trk()->data);
                rwlen = found_sec->len();
                read_first_byte();
                break;
            }
            case S_READ:
                if(!ready()) break;
                load();
                if(rwlen) {
                    if(rqs & R_DRQ) opts[TRDOS_STS] |= ST_LOST;
                    opts[TRDOS_DAT] = fdd->get_trk()->data[rwptr++];
                    crc = CRC(opts[TRDOS_DAT], crc);
                    rwlen--;
                    rqs = R_DRQ;
                    opts[TRDOS_STS] |= ST_DRQ;
                    if(!wd93_nodelay) next += fdd->ticks();
                    state = S_WAIT; state_next = S_READ;
                }
                else {
                    // чтение сектора
                    if((opts[TRDOS_CMD] & 0xe0) == 0x80) {
                        if(crc != found_sec->crcData()) opts[TRDOS_STS] |= ST_CRCERR;
                        if(opts[TRDOS_CMD] & CB_MULTIPLE) { opts[TRDOS_SEC]++; state = S_CMD_RW; break; }
                    }
                    // чтение адреса
                    if((opts[TRDOS_CMD] & 0xf0) == 0xc0) {
                        if(CRC(found_sec->id - 1, 5) != found_sec->crcId()) opts[TRDOS_STS] |= ST_CRCERR;
                    }
                    state = S_IDLE;
                }
                break;
            case S_WRSEC:
                load();
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; state = S_IDLE; break; }
                rwptr = (int)(found_sec->id + 6 + 22 - fdd->get_trk()->data);
                for(rwlen = 0; rwlen < 12; rwlen++) fdd->write(rwptr++, 0);
                for(rwlen = 0; rwlen < 3; rwlen++) fdd->write(rwptr++, 0xa1, true);
                fdd->write(rwptr++, (uint8_t)((opts[TRDOS_CMD] & CB_WRITE_DEL) ? 0xf8 : 0xfb));
                crc = CRC(fdd->get_trk()->data[rwptr - 1]);
                rwlen = found_sec->len();
                state = S_WRITE;
                break;
            case S_WRITE:
                if(!ready()) break;
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; opts[TRDOS_DAT] = 0; }
                fdd->write(rwptr++, opts[TRDOS_DAT]);
                crc = CRC(opts[TRDOS_DAT], crc);
                rwlen--;
                if(rwptr == fdd->get_trk()->len) rwptr = 0;
                if(rwlen) {
                    if(!wd93_nodelay) next += fdd->ticks();
                    state = S_WAIT; state_next = S_WRITE;
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                }
                else {
                    fdd->write(rwptr++, (uint8_t)(crc >> 8)); fdd->write(rwptr++, (uint8_t)crc);
                    fdd->write(rwptr, 0xff);
                    if(opts[TRDOS_CMD] & CB_MULTIPLE) { opts[TRDOS_SEC]++; state = S_CMD_RW; break; }
                    state = S_IDLE;
                }
                break;
            case S_WRTRACK:
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; state = S_IDLE; break; }
                state_next = S_WR_TRACK_DATA;
                start_crc = -1;
                get_index();
                end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
                break;
            case S_WR_TRACK_DATA: {
                if(!ready()) break;
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; opts[TRDOS_DAT] = 0; }
                load();
                if(!fdd->get_trk()->data) { state = S_IDLE; break; }
                bool marker = false;
                auto v = opts[TRDOS_DAT];
                if(opts[TRDOS_DAT] == 0xf5) { v = 0xa1; marker = true; start_crc = rwptr + 1; }
                if(opts[TRDOS_DAT] == 0xf6) { v = 0xc2; marker = true; }
                if(opts[TRDOS_DAT] == 0xf7) { start_crc = -1; fdd->write(rwptr++, (uint8_t)(crc >> 8)); rwlen--; v = (uint8_t)crc; }
                if(start_crc >= 0 && rwptr >= start_crc) crc = rwptr == start_crc ? CRC(v) : CRC(v, crc);
                fdd->write(rwptr++, v, marker);
                rwlen--;
                if(rwlen > 0) {
                    if(!wd93_nodelay) next += fdd->ticks();
                    state = S_WAIT; state_next = S_WR_TRACK_DATA;
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    break;
                }
                fdd->get_trk()->update();
                state = S_IDLE;
                break;
            }
            case S_TYPE1_CMD:
                opts[TRDOS_STS] = (opts[TRDOS_STS]|ST_BUSY) & ~(ST_DRQ | ST_CRCERR | ST_SEEKERR | ST_WRITEP);
                rqs = R_NONE;
                if(fdd->is_protect()) opts[TRDOS_STS] |= ST_WRITEP;
                fdd->engine(next + 2 * Z80FQ);
                // для поиска/восстановления
                state_next = S_SEEKSTART;
                // один шаг
                if(opts[TRDOS_CMD] & 0xE0) { if(opts[TRDOS_CMD] & 0x40) direction = (opts[TRDOS_CMD] & CB_SEEK_DIR) ? -1 : 1; state_next = S_STEP; }
                if(!wd93_nodelay) next += 1 * Z80FQ / 1000;
                state = S_WAIT;
                break;
            case S_STEP: {
                static const uint32_t steps[] = { 6, 12, 20, 30 };
                // выборка 0 дорожки только в команде RESTORE
                if(!fdd->track() && !(opts[TRDOS_CMD] & 0xf0)) { opts[TRDOS_TRK] = 0; state = S_VERIFY; break; }
                if(!(opts[TRDOS_CMD] & 0xe0) || (opts[TRDOS_CMD] & CB_SEEK_TRKUPD)) opts[TRDOS_TRK] += direction;
                int cyl = fdd->track() + direction;
                if(cyl < 0) cyl = 0;
                if(cyl >= PHYS_CYL) cyl = PHYS_CYL;
                fdd->track(cyl);
                if(!wd93_nodelay) next += /*steps[opts[TRDOS_CMD] & CB_SEEK_RATE]*/ 1 * Z80FQ / 1000;
                state = S_WAIT; state_next = (opts[TRDOS_CMD] & 0xe0) ? S_VERIFY : S_SEEK;
                break;
            }
            case S_SEEKSTART:
                if(!(opts[TRDOS_CMD] & CB_SEEK_TRKUPD)) { opts[TRDOS_TRK] = 0xff; opts[TRDOS_DAT] = 0; }
            case S_SEEK:
                if(opts[TRDOS_DAT] == opts[TRDOS_TRK]) { state = S_VERIFY; break; }
                direction = (opts[TRDOS_DAT] < opts[TRDOS_TRK]) ? -1 : 1;
                state = S_STEP;
                break;
            case S_VERIFY:
                if(!(opts[TRDOS_CMD] & CB_SEEK_VERIFY)) { state = S_IDLE; break; }
                end_waiting_am = next + 6 * Z80FQ / FDD_RPS;
                load();
                find_marker();
                break;
            case S_RESET:
                if(!fdd->track()) state = S_IDLE;
                else fdd->track(fdd->track() - 1);
                next += 6 * Z80FQ / 1000;
                break;
        }
    }
}

void zxVG93::read_first_byte(){
    auto t = fdd->get_trk();
    crc = CRC(t->data[rwptr - 1]);
    opts[TRDOS_DAT] = t->data[rwptr++];
    crc = CRC(opts[TRDOS_DAT], crc);
    rwlen--;
    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
    next += fdd->ticks();
    state = S_WAIT; state_next = S_READ;
}

void zxVG93::find_marker(){
    if(wd93_nodelay && fdd->track() != opts[TRDOS_TRK]) fdd->track(opts[TRDOS_TRK]);
    load();
    found_sec = nullptr;
    uint32_t wait = 10 * Z80FQ / FDD_RPS;
    if(fdd->engine() && fdd->is_disk()) {
        auto div = (uint32_t)(fdd->get_trk()->len * fdd->ticks());
        auto idx = (uint32_t)((next + tshift) % div) / fdd->ticks();
        auto t = fdd->get_trk();
        wait = (uint32_t)-1;
        for(int i = 0; i < t->total_sec; ++i) {
            auto pos = fdd->get_sec(i)->id - t->data;
            auto dist = (pos > idx) ? pos - idx : t->len + pos - idx;
            if(dist < wait) { wait = dist; found_sec = fdd->get_sec(i); }
        }
        wait = found_sec ? wait * fdd->ticks() : 10 * Z80FQ / FDD_RPS;
        if(wd93_nodelay && found_sec) {
            // Отрегулировать тяговую передачу, которая находится прямо под головкой
            auto pos = found_sec->id - t->data + 2;
            tshift = (uint32_t )(((pos * fdd->ticks()) - (next % div) + div) % div);
            wait = 100; // Задержка = 0 вызывает бесконечный поиск fdc, при отсутствии совпадающего идентификатора на дорожке
        }
    } // Иначе нет импульсов индекса - бесконечно ждать, но теперь ждать 10 оборотов, и перепроверить, если диск вставлен
    next += wait;
    if(fdd->is_disk() && next > end_waiting_am) { next = end_waiting_am; found_sec = nullptr; }
    state = S_WAIT; state_next = S_FOUND_NEXT_ID;
}

bool zxVG93::ready() {
    // Fdc слишком быстр в режиме без задержки, подождите, пока CPU обработает DRQ, но не больше 'end_waiting_am'
    if(!wd93_nodelay || !(rqs & R_DRQ)) return true;
    if(next > end_waiting_am) return true;
    state = S_WAIT; state_next = state;
    next += fdd->ticks();
    return false;
}

void zxVG93::get_index() {
    auto t = fdd->get_trk();
    auto trlen = (uint32_t)(t->len * fdd->ticks());
    auto ticks = (uint32_t)((next + tshift) % trlen);
    if(!wd93_nodelay) next += (trlen - ticks);
    rwptr = 0;
    rwlen = t->len;
    state = S_WAIT;
}

uint8_t zxVG93::vg93_read(uint8_t port, int tact) {
    exec(tact);
    uint8_t ret(0xff);
    switch(port) {
        case 0x1F: rqs &= ~R_INTRQ; ret = opts[TRDOS_STS]; break;
        case 0x3F: ret = opts[TRDOS_TRK]; break;
        case 0x5F: ret = opts[TRDOS_SEC]; break;
        case 0x7F: ret = opts[TRDOS_DAT]; opts[TRDOS_STS] &= ~ST_DRQ; rqs &= ~R_DRQ; break;
        case 0xFF: ret = rqs/* | 0x3F)*/; break;
    }
    //LOG_INFO("port:%02X ret: %i", port, ret);
    return ret;
}

void zxVG93::vg93_write(uint8_t port, uint8_t v, int tact) {
    exec(tact);
    switch(port) {
        case 0x1F:
            // прерывание команды
            if((v & 0xf0) == 0xd0) { state = S_IDLE; rqs = R_INTRQ; opts[TRDOS_STS] &= ~ST_BUSY; return; }
            if(opts[TRDOS_STS] & ST_BUSY) return;
            opts[TRDOS_CMD] = v;
            next = ALU->_TICK + tact;
            opts[TRDOS_STS] |= ST_BUSY; rqs = R_NONE;
            // команды чтения/записи
            if(opts[TRDOS_CMD] & 0x80) {
                // выйти, если нет диска
                if(opts[TRDOS_STS] & ST_NOTRDY) { state = S_IDLE; rqs = R_INTRQ; return; }
                // продолжить вращать диск
                if(fdd->engine() || wd93_nodelay) fdd->engine(next + 2 * Z80FQ);
                state = S_DELAY_BEFORE_CMD;
                return;
            }
            // для команд поиска/шага
            state = S_TYPE1_CMD;
            break;
        case 0x3F: opts[TRDOS_TRK] = v; break;
        case 0x5F: opts[TRDOS_SEC] = v; break;
        case 0x7F: opts[TRDOS_DAT] = v; rqs &= ~R_DRQ; opts[TRDOS_STS] &= ~ST_DRQ; break;
        case 0xFF: system = v;
                   fdd = &fdds[v & 3];
                   head = 1 & ~(v >> 4);
                   // сброс контроллера
                   if(!(v & 0x04)) { opts[TRDOS_STS] = ST_NOTRDY; rqs = R_INTRQ; fdd->engine(0); state = S_IDLE; }
    }
}

int zxVG93::save(const char *path, int num, int type) {
    return 0;
}

void zxVG93::updateProps() {
    Z80FQ = ALU->machine->cpuClock;
}
