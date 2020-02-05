//
// Created by Sergey on 03.02.2020.
//

#include "zxCommon.h"
#include "zxFDD.h"

// порядок следования секторов в дорожке
const int trdos_interleave = 1;

static const char* boot_sign = "boot    B";
static u_long Z80FQ = 3500000;

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

void zxFDD::seek(int _trk, int _head) {
    if(disk) {
        trk = (uint8_t)_trk;
        head = (uint8_t)_head;
        ts = Z80FQ / (get_trk()->len * FDD_RPS);
    }
}

void zxDisk::TRACK::write(int pos, uint8_t v, bool marker) {
    if(content) {
        content[pos] = v;
        if(marker) caption[pos / 8] |= 1 << (pos & 7); else caption[pos / 8] &= ~(1 << (pos & 7));
    }
}

zxDisk::zxDisk(int _trks, int _heads){
    trks = _trks; heads = _heads;
    uint16_t data_len = MAX_TRK_LEN;
    int track_len = data_len + data_len / 8 + ((data_len & 7) != 0); // 7032
    int size = _trks * _heads * track_len;
    raw = new uint8_t[size];
    memset(raw, 0, (size_t)size);
    for(int i = 0; i < _trks; ++i) {
        for(int j = 0; j < _heads; ++j) {
            auto t = &tracks[i][j];
            t->len = data_len;
            t->content = raw + track_len * (i * _heads + j);
            t->caption = t->content + data_len;
        }
    }
}

void zxDisk::TRACK::update() {
    uint8_t* src = content;
    int l = len - 8;
    total_sec = 0;
    int i = 0;
    while(i < l) {
        for(; i < len; ++i) {
            // поиск индекса маркера данных
            if(src[i] == 0xa1 && src[i + 1] == 0xfe && marker(i)) {
                sectors[total_sec].caption = src + i + 2;
                sectors[total_sec].content = nullptr;
                i += 8;
                break;
            }
        }
        // длина маркера данных 30-SD, 43-DD
        int end = Min(len, i + 43);
        for(; i < end; ++i) {
            // поиск маркера данных
            if(src[i] == 0xa1 && marker(i) && !marker(i + 1)) {
                if((i < len && src[i + 1] == 0xf8) || src[i + 1] == 0xfb) sectors[total_sec].content = src + i + 2;
                break;
            }
        }
        if(total_sec++ >= MAX_SEC) {
            // слишком много секторов
            LOG_INFO("слишком много секторов %i", total_sec);
            break;
        }
    }
    total_sec--;
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
        auto s = get_sec(0, 0, (uint8_t)i);
        if(!s) continue;
        auto ptr = s->content;
        const char* b = boot_sign;
        for(; (ptr = (uint8_t*)memchr(ptr, *b, s->len() - (ptr - s->content))) != nullptr; ++b) {
            if(!*b) return true;
        }
    }
    return false;
}

// Рассогласование данных по боевым функциям АРМ
static uint16_t sec_dataW(zxDisk::TRACK::SECTOR* s, size_t offset) { return wordLE(s->content + offset); }

static void sec_dataW(zxDisk::TRACK::SECTOR* s, size_t offset, uint16_t d) {
    s->content[offset] = (uint8_t)(d & 0xff);
    s->content[offset + 1] = (uint8_t)(d >> 8);
}

uint16_t zxFDD::CRC(uint8_t * src, int size) const {
    uint32_t crc(0xcdb4);
    while(size--) {
        crc ^= (*src++) << 8;
        for(int i = 8; i; --i) { if((crc *= 2) & 0x10000) crc ^= 0x1021; }
    }
    return (uint16_t)crc;
}

bool zxFDD::write_sec(int trk, int head, int sec, const uint8_t* data) {
    auto s = get_sec(trk, head, sec);
    if(!s || !s->content) return false;
    memcpy(s->content, data, (size_t)s->len());
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
            auto t = get_trk(); auto l = t->len; auto d = t->content;
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
                sec->caption = d + pos;
                write(pos++, (uint8_t)trk); write(pos++, (uint8_t)head); write(pos++, lv[trdos_interleave][n]); write(pos++, 1);
                auto crc = CRC(d + pos - 5, 5);
                write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
                // gap2 sync am
                write_blk(pos, 0x4e, 22); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfb);
                sec->content = d + pos;
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
    s->content[0xe2] = 1;	 // первая свободная дорожка
    s->content[0xe3] = 0x16; // 80 дорожек, двойная плотность
    s->content[0xe7] = 0x10; // признак trdos
    sec_dataW(s, 0xe5, 2544);// свободных секторов
    update_crc(s);
}

void zxFDD::update_crc(zxDisk::TRACK::SECTOR *s) const {
    auto len = s->len();
    sec_dataW(s, (size_t)len, swap_byte_order(CRC(s->content - 1, len + 1)));
}

bool zxFDD::add_file(const uint8_t *hdr, const uint8_t *data) {
    auto s = get_sec(0, 0, 9);
    if(!s) return false;
    int len = hdr[13];
    int pos = s->content[228] * 16;
    auto* dir = get_sec(0, 0, 1 + pos / 256);
    if(!dir) return false;
    // диск заполнен
    if(sec_dataW(s, 229) < len) return false;
    memcpy(dir->content + (pos & 0xff), hdr, 14);
    sec_dataW(dir, (size_t)((pos & 0xff) + 14), sec_dataW(s, 0xe1));
    update_crc(dir);

    pos = s->content[225] + 16 * s->content[0xe2];
    s->content[225] = (uint8_t)((pos + len) & 0x0f);
    s->content[226] = (uint8_t)((pos + len) >> 4);
    s->content[228]++;
    sec_dataW(s, 229, (uint16_t)(sec_dataW(s, 0xe5) - len));
    update_crc(s);

    // перейти на следующую дорожку
    for(int i = 0; i < len; ++i, ++pos) {
        int t = pos / 32;
        int h = (pos / 16) & 1;
        if(!write_sec(t, h, (pos & 0x0f) + 1, data + i * 256)) return false;
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
    _rwlen = 6250;
    track(1);
    log_to_data(false, "open_trd", get_trk()->content, 0, 0, 0);
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
            auto t = get_trk(); auto l = t->len; auto d = t->content;
            int id_len = l / 8 + ((l & 7) != 0);
            memset(d, 0, (size_t)(l + id_len));

            int pos = 0;
            // gap4a sync iam
            write_blk(pos, 0x4e, 80); write_blk(pos, 0, 12); write_blk(pos, 0xc2, 3, true);
            write(pos++, 0xfc);

            auto t0 = dat + Dword(trks);
            auto ns = trks[6];
            t->total_sec = ns;
            trks += 7;
            for(int n = 0; n < ns; ++n) {
                // gap1 sync am
                write_blk(pos, 0x4e, 40); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfe);
                auto sec = get_sec(n);
                sec->caption = d + pos;
                write(pos++, trks[0]); write(pos++, trks[1]); write(pos++, trks[2]); write(pos++, trks[3]);
                auto crc = CRC(d + pos - 5, 5);
                write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
                if(trks[4] & 0x40) sec->content = nullptr;
                else {
                    auto data1 = (t0 + wordLE(trks + 5));
                    if(data1 + 128 > buf + data_size) return false;
                    // gap2 sync am
                    write_blk(pos, 0x4e, 22); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3, true); write(pos++, 0xfb);
                    sec->content = d + pos;
                    auto len = sec->len();
                    memcpy(sec->content, data1, (size_t)len);
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

zxVG93::zxVG93() : next(0), tshift(0), head(0), direction(0), rqs(R_NONE),
                   system(0), end_waiting_am(0), found_sec(nullptr), rwptr(0), rwlen(0), crc(0), start_crc(-1) {
    _ST_SET(S_IDLE, S_IDLE);
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
                rqs = R_INTRQ; state = S_IDLE;
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

void zxVG93::exec(int tact) {
    int cmd;
    auto time = ALU->_TICK + tact;
    // Неактивные диски игнорируют бит HLT
    if(time > fdd->engine() && (system & CB_SYS_HLT)) fdd->engine(0);
    fdd->is_disk() ? opts[TRDOS_STS] &= ~ST_NOTRDY : opts[TRDOS_STS] |= ST_NOTRDY;
    // команды позиционирования
    if(!(opts[TRDOS_CMD] & 0x80)) {
        opts[TRDOS_STS] &= ~(ST_TRK00 | ST_INDEX);
        if(fdd->engine() && (system & CB_SYS_HLT)) opts[TRDOS_STS] |= ST_HEADL;
        if(!fdd->track()) opts[TRDOS_STS] |= ST_TRK00;
        // индексный импульс - чередуюется каждые 4 мс(если диск присутствует)
        if(fdd->is_disk() && fdd->engine() && ((time + tshift) % (Z80FQ / FDD_RPS) < (Z80FQ * 4 / 1000))) opts[TRDOS_STS] |= ST_INDEX;
    }
    while(true) {
        switch(state & 15) {
            // шина свободна. команда завершена
            case S_IDLE: opts[TRDOS_STS] &= ~ST_BUSY; rqs = R_INTRQ; return;
            // задержка при выполении команды
            case S_WAIT: if(time < next) return; _ST_NEXT; break;
            // подготовка команда чтения/записи
            case S_PREPARE_CMD:
                LOG_INFO("S_PREPARE_CMD cmd:%i 15ms:%i", opts[TRDOS_CMD], opts[TRDOS_CMD] & CB_DELAY);
                if(!wd93_nodelay && (opts[TRDOS_CMD] & CB_DELAY)) next += (Z80FQ * 15 / 1000);
                // сброс статуса
                opts[TRDOS_STS] = (opts[TRDOS_STS] | ST_BUSY) & ~(ST_DRQ | ST_LOST | ST_NOT_SEC | ST_RECORDT | ST_WRITEP);
                _ST_SET(S_WAIT, S_CMD_RW);
                break;
            // подготовка операции чтения/записи
            case S_CMD_RW:
                // проверка на защиту от записи в операциях записи
                cmd = opts[TRDOS_CMD];
                LOG_INFO("S_RW_CMD cmd:%i", cmd);
                if(((cmd & 0xe0) == C_WSEC || (cmd & 0xf0) == C_WTRK) && fdd->is_protect()) { opts[TRDOS_STS] |= ST_WRITEP; state = S_IDLE; break; }
                // операции чтения/записи секторов или чтения адреса - поиск следующего адресного маркера
                // нахрена???
                if((cmd & 0xc0) == 0x80 || (cmd & 0xf8) == C_RADR) {
                    end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
                    find_sec();
                    break;
                }
                // запись дорожки(форматирование)
                if((cmd & 0xf8) == C_WTRK) {
                    next += 3 * fdd->ticks();
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    _ST_SET(S_WAIT, S_WRTRACK);
                    break;
                }
                // чтение дорожки
                if((cmd & 0xf8) == C_RTRK) {
                    load();
                    rwptr = 0; rwlen = fdd->get_trk()->len; _rwlen = rwlen;
                    get_index(S_READ);
                    break;
                }
                state = S_IDLE;
                break;
            // нет диска - ждем
            case S_FIND_SEC: {
                cmd = opts[TRDOS_CMD];
                LOG_INFO("S_FIND_SEC cmd:%i trk: %i sec:%i end_waiting_am:%i", cmd, opts[TRDOS_TRK], opts[TRDOS_SEC], end_waiting_am);
                if(!fdd->is_disk()) { end_waiting_am = next + 5 * Z80FQ / FDD_RPS; find_sec(); break; }
                // время ожидания истекло. сектор не наден
                if(next >= end_waiting_am || !found_sec) {LOG_INFO("S_FIND_SEC - TIME OUT", 0); opts[TRDOS_STS] |= ST_NOT_SEC; state = S_IDLE; break; }
                opts[TRDOS_STS] &= ~ST_CRCERR;
                load();
                // проверка на CRC после поиска сектора
                if(!(cmd & 0x80)) {
                    if(found_sec->trk() != opts[TRDOS_TRK]) { find_sec(); break; }
                    if(CRC(found_sec->caption - 1, 5) != found_sec->crcId()) {
                        LOG_INFO("S_FIND_SEC - SEEK CHECK CRC", 0);
                        opts[TRDOS_STS] |= ST_CRCERR; find_sec();
                    } else state = S_IDLE;
                    break;
                }
                // чтение адресного маркера
                if((cmd & 0xf0) == C_RADR) {
                    LOG_INFO("S_FIND_SEC - RADR", 0);
                    rwptr = (uint16_t) (found_sec->caption - fdd->get_trk()->content); rwlen = 6; _rwlen = rwlen;
                    read_first_byte(); break;
                }
                // запись/чтение сектора
                if (found_sec->trk() != opts[TRDOS_TRK] || found_sec->sec() != opts[TRDOS_SEC] ||
                    ((cmd & CB_SIDE_CMP) && (((cmd >> CB_SIDE_SHIFT) ^ found_sec->head()) & 1))) { LOG_INFO("S_FIND_SEC - RADR", 0);find_sec(); break; }
                // проверка на CRC
                if(CRC(found_sec->caption - 1, 5) != found_sec->crcId()) { LOG_INFO("S_FIND_SEC - SEC CHECK CRC", 0); opts[TRDOS_STS] |= ST_CRCERR; find_sec(); break; }
                // запись сектора
                if (cmd & 0x20) {
                    LOG_INFO("S_FIND_SEC - WSEC", 0);
                    next += fdd->ticks() * 9;
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    _ST_SET(S_WAIT, S_WRSEC);
                    break;
                }
                LOG_INFO("S_FIND_SEC - RSEC", 0);
                // чтение сектора
                if(!found_sec->content) { find_sec(); break; }
                found_sec->content[-1] == 0xf8 ? opts[TRDOS_STS] |= ST_RECORDT : opts[TRDOS_STS] &= ~ST_RECORDT;
                rwptr = (uint16_t) (found_sec->content - fdd->get_trk()->content); rwlen = found_sec->len();
                _rwlen = rwlen;
                read_first_byte();
                break;
            }
            // операция чтения(сектора, адреса, дорожки)
            case S_READ:
                if(!ready()) break;
                load();
                if(rwlen) {
                    if(rqs & R_DRQ) opts[TRDOS_STS] |= ST_LOST;
                    opts[TRDOS_DAT] = fdd->get_trk()->content[rwptr++]; rwlen--;
                    crc = CRC(opts[TRDOS_DAT], crc);
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    if(!wd93_nodelay) next += fdd->ticks();
                    _ST_SET(S_WAIT, S_READ);
                } else {
                    LOG_INFO("S_READ FINISH idx:%i len:%i", rwptr, _rwlen);
                    log_to_data(true, "read sector", found_sec->content, opts[TRDOS_TRK], opts[TRDOS_SEC], head);
                    if((opts[TRDOS_CMD] & 0xe0) == C_RSEC) {
                        // если чтение сектора - проверяем на CRC
                        if (crc != found_sec->crcData()) opts[TRDOS_STS] |= ST_CRCERR;
                        // если множественная загрузка секторов
                        if (opts[TRDOS_CMD] & CB_MULTIPLE) { opts[TRDOS_SEC]++; state = S_CMD_RW; break; }
                    } else if ((opts[TRDOS_CMD] & 0xf0) == C_RADR) {
                        // проверяем на CRC
                        if (CRC(found_sec->caption - 1, 5) != found_sec->crcId()) opts[TRDOS_STS] |= ST_CRCERR;
                    }
                    state = S_IDLE;
                }
                break;
            // запись сектора
            case S_WRSEC:
                LOG_INFO("S_WRSEC del:%i", opts[TRDOS_CMD] & CB_WRITE_DEL);
                load();
                // потеря данных
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; state = S_IDLE; break; }
                rwptr = (uint16_t)((found_sec->caption - fdd->get_trk()->content) + 28);
                for(rwlen = 0; rwlen < 12; rwlen++) fdd->write(rwptr++, 0);
                for(rwlen = 0; rwlen < 3; rwlen++) fdd->write(rwptr++, 0xa1, true);
                fdd->write(rwptr++, (uint8_t)((opts[TRDOS_CMD] & CB_WRITE_DEL) ? 0xf8 : 0xfb));
                crc = CRC(fdd->get_trk()->content[rwptr - 1]); rwlen = found_sec->len();
                _rwlen = rwlen;
                state = S_WRITE;
                break;
            // операция записи
            case S_WRITE:
                if(!ready()) break;
                // потеря данных
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; opts[TRDOS_DAT] = 0; }
                // запись байта
                fdd->write(rwptr++, opts[TRDOS_DAT]); rwlen--; crc = CRC(opts[TRDOS_DAT], crc);
                if(rwptr == fdd->get_trk()->len) rwptr = 0;
                if(rwlen) {
                    if(!wd93_nodelay) next += fdd->ticks();
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    _ST_SET(S_WAIT, S_WRITE);
                } else {
                    // завершение операции
                    LOG_INFO("S_WRITE FINISH idx:%i len:%i", rwptr, _rwlen);
                    log_to_data(false, "write sector", found_sec->content, opts[TRDOS_TRK], opts[TRDOS_SEC], head);
                    // запись CRC
                    fdd->write(rwptr++, (uint8_t)(crc >> 8)); fdd->write(rwptr++, (uint8_t)crc); fdd->write(rwptr, 0xff);
                    // проверка на множественные сектора
                    if(opts[TRDOS_CMD] & CB_MULTIPLE) { opts[TRDOS_SEC]++; state = S_CMD_RW; break; }
                    state = S_IDLE;
                }
                break;
            // запись дорожки
            case S_WRTRACK:
                LOG_INFO("S_WRTRACK", 0);
                // потеря данных
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; state = S_IDLE; break; }
                end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
                start_crc = -1; get_index(S_WR_TRACK_DATA);
                break;
            // запись данных дорожки
            case S_WR_TRACK_DATA: {
                if(!ready()) break;
                // потеря данных
                if(rqs & R_DRQ) { opts[TRDOS_STS] |= ST_LOST; opts[TRDOS_DAT] = 0; }
                load();
                if(!fdd->get_trk()->content) { state = S_IDLE; break; }
                bool marker = false;
                auto d = opts[TRDOS_DAT]; auto v = d;
                if(d == 0xf5) { v = 0xa1; marker = true; start_crc = rwptr + 1; }
                if(d == 0xf6) { v = 0xc2; marker = true; }
                if(d == 0xf7) { start_crc = -1; fdd->write(rwptr++, (uint8_t)(crc >> 8)); rwlen--; v = (uint8_t)crc; }
                if(start_crc >= 0 && rwptr >= start_crc) crc = rwptr == start_crc ? CRC(v) : CRC(v, crc);
                fdd->write(rwptr++, v, marker);
                rwlen--;
                if(rwlen) {
                    if(!wd93_nodelay) next += fdd->ticks();
                    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
                    _ST_SET(S_WAIT, S_WR_TRACK_DATA);
                } else {
                    LOG_INFO("S_WR_TRACK_DATA FINISH idx:%i len:%i", rwptr, _rwlen);
                    log_to_data(false, "write track", fdd->get_trk()->content, opts[TRDOS_TRK], opts[TRDOS_SEC], head);
                    fdd->get_trk()->update(); state = S_IDLE;
                }
                break;
            }
            // команды позиционирования
            case S_TYPE1_CMD:
                LOG_INFO("S_TYPE1_CMD cmd:%i", opts[TRDOS_CMD]);
                opts[TRDOS_STS] = (opts[TRDOS_STS]|ST_BUSY) & ~(ST_DRQ | ST_CRCERR | ST_SEEKERR | ST_WRITEP);
                rqs = R_NONE;
                if(fdd->is_protect()) opts[TRDOS_STS] |= ST_WRITEP;
                fdd->engine(next + 2 * Z80FQ);
                if(!wd93_nodelay) next += 1 * Z80FQ / 1000;
                // поиск/восстановление
                cmd = S_SEEKSTART;
                if(opts[TRDOS_CMD] & 0xE0) {
                    // один шаг
                    if(opts[TRDOS_CMD] & 0x40) direction = (uint8_t)((opts[TRDOS_CMD] & CB_SEEK_DIR) ? -1 : 1); cmd = S_STEP;
                }
                _ST_SET(S_WAIT, cmd);
                break;
            case S_STEP: {
                LOG_INFO("S_STEP trk:%i", opts[TRDOS_TRK]);
                static const uint32_t steps[] = { 6, 12, 20, 30 };
                cmd = opts[TRDOS_CMD];
                // выборка 0 дорожки только в команде RESTORE
                if(!fdd->track() && !(cmd& 0xf0)) { opts[TRDOS_TRK] = 0; state = S_VERIFY; break; }
                // позиционирование дорожки
                if(!(cmd & 0xe0) || (cmd & CB_SEEK_TRKUPD)) opts[TRDOS_TRK] += direction;
                // проверка на допустимые дорожки
                int cyl = fdd->track() + direction; if(cyl < 0) cyl = 0; if(cyl >= PHYS_CYL) cyl = PHYS_CYL;
                fdd->track(cyl);
                if(!wd93_nodelay) next += steps[0] * Z80FQ / 1000;
                _ST_SET(S_WAIT, (cmd & 0xe0) ? S_VERIFY : S_SEEK);
                break;
            }
            case S_SEEKSTART:
                if(!(opts[TRDOS_CMD] & CB_SEEK_TRKUPD)) { opts[TRDOS_TRK] = 0xff; opts[TRDOS_DAT] = 0; }
                LOG_INFO("S_SEEKSTART trk:%i", opts[TRDOS_TRK]);
            case S_SEEK:
                LOG_INFO("S_SEEK trk:%i dat:%i", opts[TRDOS_TRK], opts[TRDOS_DAT]);
                if(opts[TRDOS_DAT] == opts[TRDOS_TRK]) { state = S_VERIFY; break; }
                direction = (uint8_t)((opts[TRDOS_DAT] < opts[TRDOS_TRK]) ? -1 : 1);
                state = S_STEP;
                break;
            case S_VERIFY:
                LOG_INFO("S_VERIFY trk:%i sec:%i", opts[TRDOS_TRK], opts[TRDOS_SEC]);
                if(!(opts[TRDOS_CMD] & CB_SEEK_VERIFY)) { state = S_IDLE; break; }
                end_waiting_am = next + 6 * Z80FQ / FDD_RPS;
                load(); find_sec();
                break;
        }
    }
}

void zxVG93::read_first_byte(){
    auto t = fdd->get_trk();
    crc = CRC(t->content[rwptr - 1]);
    opts[TRDOS_DAT] = t->content[rwptr++]; rwlen--;
    crc = CRC(opts[TRDOS_DAT], crc);
    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
    next += fdd->ticks();
    _ST_SET(S_WAIT, S_READ);
}

void zxVG93::find_sec() {
    if(wd93_nodelay && fdd->track() != opts[TRDOS_TRK]) fdd->track(opts[TRDOS_TRK]);
    load();
    found_sec = nullptr;
    auto wait = 10 * Z80FQ / FDD_RPS;
    if(fdd->engine() && fdd->is_disk()) {
        auto div = (fdd->get_trk()->len * fdd->ticks());
        auto idx = ((next + tshift) % div) / fdd->ticks();
        auto t = fdd->get_trk();
        wait = 0xffffffff;
        for(int i = 0; i < t->total_sec; ++i) {
            auto sec = fdd->get_sec(i);
            auto pos = sec->caption - t->content;
            auto dist = (pos > idx) ? pos - idx : t->len + pos - idx;
            if(dist < wait) { wait = dist; found_sec = sec; }
        }
        wait = found_sec ? wait * fdd->ticks() : 10 * Z80FQ / FDD_RPS;
        if(wd93_nodelay && found_sec) {
            // Отрегулировать тяговую передачу, которая находится прямо под головкой
            auto pos = found_sec->caption - t->content + 2;
            tshift = (uint32_t )(((pos * fdd->ticks()) - (next % div) + div) % div);
            wait = 100; // Задержка = 0 вызывает бесконечный поиск fdc, при отсутствии совпадающего идентификатора на дорожке
        }
    }
    // нет импульса индекса - ждать 10 оборотов, и перепроверить, если диск вставлен
    next += wait;
    if(fdd->is_disk() && next > end_waiting_am) {
        //LOG_INFO("FIND_SEC FIND2 wait:%i sec:%X", wait, found_sec);
        next = end_waiting_am; found_sec = nullptr;
    }
    _ST_SET(S_WAIT, S_FIND_SEC);
}

bool zxVG93::ready() {
    // Fdc слишком быстр в режиме без задержки, подождите, пока CPU обработает DRQ, но не больше 'end_waiting_am'
    if(!wd93_nodelay || !(rqs & R_DRQ)) return true;
    if(next > end_waiting_am) return true;
    next += fdd->ticks();
    _ST_SET(S_WAIT, S_WAIT);
    return false;
}

void zxVG93::get_index(int s_next) {
    auto t = fdd->get_trk();
    auto trlen = (uint32_t)(t->len * fdd->ticks());
    auto ticks = (uint32_t)((next + tshift) % trlen);
    if(!wd93_nodelay) next += (trlen - ticks);
    rwptr = 0; rwlen = t->len; _rwlen = rwlen;
    _ST_SET(S_WAIT, s_next);
}

uint8_t zxVG93::vg93_read(uint8_t port, int tact) {
    exec(tact);
    uint8_t ret(0xff);
    switch(port) {
        case 0x1F: rqs &= ~R_INTRQ; ret = opts[TRDOS_STS]; break;
        case 0x3F: ret = opts[TRDOS_TRK]; break;
        case 0x5F: ret = opts[TRDOS_SEC]; break;
        case 0x7F: ret = opts[TRDOS_DAT]; opts[TRDOS_STS] &= ~ST_DRQ; rqs &= ~R_DRQ; break;
        case 0xFF: ret = (uint8_t)(rqs | 0x3F); break;
    }
    return ret;
}

void zxVG93::vg93_write(uint8_t port, uint8_t v, int tact) {
    exec(tact);
    switch(port) {
        case 0x1F:
            // прерывание команды
            if((v & 0xf0) == C_INTERRUPT) {
                LOG_INFO("C_INTERRUPT", 0);
                state = S_IDLE; rqs = R_INTRQ; opts[TRDOS_STS] &= ~ST_BUSY; return;
            }
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
                state = S_PREPARE_CMD;
                return;
            }
            // для команд поиска/шага
            state = S_TYPE1_CMD;
            break;
        case 0x3F: opts[TRDOS_TRK] = v; break;
        case 0x5F: opts[TRDOS_SEC] = v; break;
        case 0x7F: opts[TRDOS_DAT] = v; rqs &= ~R_DRQ; opts[TRDOS_STS] &= ~ST_DRQ; break;
        case 0xFF: system = v; fdd = &fdds[v & 3]; head = (uint8_t)(1 & ~(v >> 4));
                   // сброс контроллера
                   if(!(v & CB_RESET)) { opts[TRDOS_STS] = ST_NOTRDY; rqs = R_INTRQ; fdd->engine(0); state = S_IDLE; }
    }
}

int zxVG93::save(const char *path, int num, int type) {
    return 0;
}

void zxVG93::updateProps() {
    //Z80FQ = ALU->machine->cpuClock;
}

