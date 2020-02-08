//
// Created by Sergey on 03.02.2020.
//

#include "zxCommon.h"
#include "zxFDD.h"

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
//  3       |                                     ОШИБКА CRC                                |                   0                   |
// ----------------------------------------------------------------------------------------------------------------------------------
//  2       |    ДОРОЖКА 0     |                                            ПОТЕРЯ  ДАННЫХ                                          |
// ----------------------------------------------------------------------------------------------------------------------------------
//  1       |ИНДЕКСНЫЙ ИМПУЛЬС |                                            ЗАПРОС  ДАННЫХ                                          |
// ----------------------------------------------------------------------------------------------------------------------------------
//  0       |                И   д   е   т           в   ы   п   о   л   н   е   н   и   е       к   о   м   а   н   д   ы          |
// ----------------------------------------------------------------------------------------------------------------------------------
// КОЛИЧЕСТВО   /  КОД     /  НАЗНАЧЕНИЕ
//   ~40/80     /  FF/4E   /   Последний пробел
//  6/12        /    0     /
//  3           /   F6     /  Поле С2     (MFM)
//  1           /   FC     /  Индексная метка
//  26/50       /  FF/4E   /  Первый пробел
//  6/12        /   0      /
//  3           /   F5     /   Попе А1      (MFM)
//  1           /   FE     /   Адресная метка заголовка сектора
//  1           /   xx     /   Номер дорожки
//  1           /   xx     /   Номер стороны дискеты
//  1           /   xx     /   Номер сектора
//  1           /   xx     /   Длина сектора
//  1           /   F7     /   Формирование двух байтов CRC заголовка
//  11/22       /   FF/4E  /   Второй пробел
//  6/12        /   0      /
//  3           /   F5     /   Попе А1      (MFM)
//  1           /   FB     /   Адресная метка данных
//  xx          /   xx     /   Данные
//  1           /   F7     /   Формирование двух байтов CRC данных
//  27/54       /   FF/4E  /   Третий пробел
//  247/598     /   FF/4E  /   Четвертый пробел

/*
    По расчетам из спецификации к ВГ-93 на дорожку
    приблизительно помещается 10364 байт данных в
    режиме MFM и 5156 байт в режиме FM.

    Однако в TR-DOS'е с режимом MFM типичной длиной
    дорожки считается 6250 байт (6208...6464 байт).
    Эта цифра и взята за основу, т.к. при первом
    варианте случаются ошибки. Для режима FM взято
    соответственно число 3125.
*/

// Field		FM					MFM
// =====================================================
// GAP I		0xff * 16 (min)		0x4e * 32 (min)
// GAP II		0xff * 11 (exact)	0x4e * 22 (exact)
// GAP III		0xff * 12 (min)		0x4e * 24 (min)
// GAP IV		0xff * 16 (min)		0x4e * 32 (min)
// SYNC			0x00 * 6  (exact)	0x00 * 12 (exact)

// 0000 HVXX - восстановление
//     переход на нулевую дорожку
// позиционирование:
// I=1 дорожка меняет свое значение
// H - определяет положение магнитной головки дисковода во время выполнения команды
// V - в случае установки магнитной головки в рабочее положение (h=l) этот бит используется для задания режима проверки положения головки. Если
//     бит установлен, то в конце операции содержимое регистра дорожки сравнивается с действительным номером, считанным с дискеты.
// 0000 HVXX - восстановление
// 010I HVXX - на один шаг вперед
// 011I HVXX - на один шаг назад
// 001I HVXX - на один шаг в том же направлении
// 0001 HVXX - позиционирование магнитной головки на заданную дорожку. Номер в регистр данных.

// 100M SEC0 - Чтение секторов.
// M -  бит задает количество секторов, участвующих в операции. Если он сброшен, то обрабатывается один сектор,
//      если установлен — обрабатываются последовательно все сектора на текущей дорожке, начиная с того, который указан в регистре сектора
// S -  значение этого бита определяет номер стороны дискеты (0 — нижняя сторона, 1 — верхняя). Микросхема не имеет аппаратных сигналов для
//      выбора магнитных головок на дисководе (это делает системный регистр Beta Disk-интерфейса), но номер стороны содержится в заголовке сектора
// E -  этот бит используется для задания задержки при установке магнитной головки в рабочее положение. Если бит сброшен,
//      задержка не производится, в противном случае между выдачей сигнала на установку головки в рабочее состояние и началом операции делается задержка в 15 мс
// C -  значение этого бита определяет, делать или не делать проверку стороны дискеты при операции. Если бит сброшен, то проверки не производится
// 101M SECA - Запись секторов
// A -  указывает на один из двух возможных форматов сектора. В дальнейшем при считывании этот формат будет индицироваться в 5 бите системного регистра.
//      Обычно этот бит обнуляют, при этом в поле заголовка сектора формируется специальный байт #FB, в противном случае — байт #F8.
// 1111 0E00 - Запись (форматирование) дорожки FM/MFM
// 1110 0E00 - Чтение дорожки
//              Эта команда считывает с дорожки всю имеющуюся на ней информацию, включая поля пробелов, заголовков и служебные байты.
// 1100 0E00 - Чтение адреса
//              По этой команде с дискеты считывается первый встреченный заголовок сектора. Из поля заголовка передаются 6 байтов —
//              4 байта информационные (номер дорожки, номер стороны, номер сектора и длина сектора) и 2 байта контрольного кода.
//              При выполнении этой Команды содержимое регистра дорожки пересылается в регистр сектора

#define Min(o, p)	(o < p ? o : p)

uint64_t Z80FQ = 3500000;

uint8_t* zxVG93::loadState(uint8_t* ptr) {
    return ptr;
}

uint8_t* zxVG93::saveState(uint8_t* ptr) {
    /*
    // начальная КК
    int8_t start_crc;
    // головка
    uint8_t head;
    // напраление
    int8_t direction;
    // позиция при чтении/записи
    int16_t rwptr;
    // длина буфера чтения/записи
    int16_t rwlen;
    // следующее время
    uint32_t next;
    // время ожидания сектора
    uint32_t end_waiting_am;
    // текущее состояние
    uint8_t	state;
    // состояние порта 0xFF
    uint8_t	rqs;
    // системный регистр
    uint8_t	system;
    // текущий КК
    uint16_t crc;
     *
     */

    /*
    uint32_t motor;
    uint8_t  trk;
    uint8_t  head;
    u_long   ts;
     *
     */
    return ptr;
}

int zxVG93::save(const char *path, int num, int type) {
    return 0;
}

void zxVG93::updateProps() {
    Z80FQ = ULA->machine->cpuClock;
}

/*
static const char* boot_sign = "boot    B";

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
*/

zxDisk::zxDisk(int _trks, int _heads){
    trks = _trks; heads = _heads;
    uint16_t data_len = MAX_TRK_LEN;
    int size = _trks * _heads * data_len;
    raw = new uint8_t[size];
    memset(raw, 0, (size_t)size);
    for(int i = 0; i < _trks; ++i) {
        for(int j = 0; j < _heads; ++j) {
            auto t = &tracks[i][j];
            t->len = data_len;
            t->content = raw + data_len * (i * _heads + j);
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
            if(src[i] == 0xa1 && src[i + 1] == 0xfe) {
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
            if(src[i] == 0xa1 && (src[i + 1] == 0xfb || src[i + 1] == 0xf8)) {
                sectors[total_sec].content = src + i + 2;
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

/*
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
*/

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
            write_blk(pos, 0x4e, 80); write_blk(pos, 0, 12); write_blk(pos, 0xc2, 3); write(pos++, 0xfc);
            t->total_sec = max_trd_sectors;
            for(int n = 0; n < max_trd_sectors; ++n) {
                // gap1 sync am
                write_blk(pos, 0x4e, 40); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3); write(pos++, 0xfe);
                auto sec = get_sec(n);
                sec->caption = d + pos;
                write(pos++, (uint8_t)trk); write(pos++, (uint8_t)head); write(pos++, lv[1][n]); write(pos++, 1);
                auto crc = CRC(d + pos - 5, 5);
                write(pos++, (uint8_t)(crc >> 8)); write(pos++, (uint8_t)crc);
                // gap2 sync am
                write_blk(pos, 0x4e, 22); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3); write(pos++, 0xfb);
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
    if(s) fillDiskConfig(s);
}

void zxFDD::fillDiskConfig(zxDisk::TRACK::SECTOR *s) {
    auto sec = s->content;
    sec[226] = 1;	 // первая свободная дорожка
    sec[227] = 0x16; // тип диска
    sec[228] = 0x0;  // количество файлов на диске(включая удаленные)
    sec[229] = 240;
    sec[230] = 9;    // количество секторов 2544
    sec[231] = 16;   // количество секторов на дорожке
    memset(&sec[234], 32, 9);
    sec[244] = 0;    // количество удаленных файлов
    //sec[245]       - метка диска(8 байт)
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
    return true;
}

bool zxFDD::read_fdi(const void *data, size_t data_size){
    delete disk;
    auto buf = (const uint8_t*)data;
    disk = new zxDisk(buf[4], buf[6]);

    auto offsSecs = *(uint16_t*)(buf + 0x0A);
    auto offsTrks = *(uint16_t*)(buf + 0x0C);
    auto trks = buf + 0x0E + offsTrks;
    auto dat = buf + offsSecs;

    for(int i = 0; i < disk->trks; ++i) {
        for(int j = 0; j < disk->heads; ++j) {
            seek(i, j);
            auto t = get_trk(); auto l = t->len; auto d = t->content;
            int id_len = l / 8 + ((l & 7) != 0);
            memset(d, 0, (size_t)(l + id_len));

            int pos = 0;
            // gap4a sync iam
            write_blk(pos, 0x4e, 80); write_blk(pos, 0, 12); write_blk(pos, 0xc2, 3);
            write(pos++, 0xfc);

            auto t0 = dat + Dword(trks);
            auto ns = trks[6];
            t->total_sec = ns;
            trks += 7;
            for(int n = 0; n < ns; ++n) {
                // gap1 sync am
                write_blk(pos, 0x4e, 40); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3); write(pos++, 0xfe);
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
                    write_blk(pos, 0x4e, 22); write_blk(pos, 0, 12); write_blk(pos, 0xa1, 3); write(pos++, 0xfb);
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

zxVG93::zxVG93() : next(0), head(0), direction(0), rqs(R_NONE),
                   system(0), end_waiting_am(0), found_sec(nullptr), rwptr(0), rwlen(0), crc(0), start_crc(-1) {
    _ST_SET(S_IDLE, S_IDLE);
    Z80FQ = 3500000;
    fdd = fdds;
    opts[TRDOS_SEC] = 1;
    opts[TRDOS_CMD] = opts[TRDOS_DAT] = opts[TRDOS_TRK] = opts[TRDOS_STS] = 0;
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
    auto c = (uint16_t)0xcdb4;
    while(size--) c = CRC(*src++, c);
    return c;
}

void zxVG93::exec(int tact) {
    auto time = ULA->_TICK + tact;
    // Неактивные диски игнорируют бит HLT
    if(time > fdd->engine() && (system & CB_SYS_HLT)) fdd->engine(0);
    fdd->is_disk() ? opts[TRDOS_STS] &= ~ST_NOTRDY : opts[TRDOS_STS] |= ST_NOTRDY;
    // команды позиционирования
    if(!(opts[TRDOS_CMD] & 0x80)) {
        opts[TRDOS_STS] &= ~(ST_TRK00 | ST_INDEX | ST_HEADL);
        if(fdd->engine() && (system & CB_SYS_HLT)) opts[TRDOS_STS] |= ST_HEADL;
        if(!fdd->track()) opts[TRDOS_STS] |= ST_TRK00;
        // индексный импульс - чередуюется каждые 4 мс(если диск присутствует)
        if(fdd->is_disk() && fdd->engine() && (time % (Z80FQ / FDD_RPS) < (Z80FQ * 4 / 1000))) opts[TRDOS_STS] |= ST_INDEX;
    }
    while(true) {
        switch(state & 15) {
            // шина свободна. команда завершена
            case S_IDLE: opts[TRDOS_STS] &= ~ST_BUSY; rqs = R_INTRQ; return;
            // задержка при выполении команды
            case S_WAIT: if(time < next) return; _ST_NEXT; break;
            // подготовка к чтению/записи
            case S_PREPARE_CMD: cmdPrepareRW(); break;
            // подготовка операции чтения/записи
            case S_CMD_RW: cmdReadWrite(); break;
            // операция чтения(сектора, адреса, дорожки)
            case S_READ: cmdRead(); break;
            // нет диска - ждем
            case S_FIND_SEC: cmdFindSec(); break;
            // запись сектора
            case S_WRSEC: cmdWriteSector(); break;
            // запись дорожки
            case S_WRTRACK: cmdWriteTrack(); break;
            // операция записи
            case S_WRITE: cmdWrite(); break;
            // запись данных дорожки
            case S_WR_TRACK_DATA: cmdWriteTrackData(); break;
            // команды позиционирования
            case S_TYPE1_CMD: cmdType1(); break;
            case S_STEP: cmdStep(); break;
            case S_SEEKSTART: if(!(opts[TRDOS_CMD] & CB_SEEK_TRKUPD)) { opts[TRDOS_TRK] = 0xff; opts[TRDOS_DAT] = 0; }
            case S_SEEK: cmdSeek(); break;
            case S_VERIFY: cmdVerify(); break;
        }
    }
}

void zxVG93::read_byte(){
    auto t = fdd->get_trk();
    opts[TRDOS_DAT] = t->content[rwptr++]; rwlen--;
    crc = CRC(opts[TRDOS_DAT], crc);
    rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
    next += fdd->ticks();
    _ST_SET(S_WAIT, S_READ);
}

bool zxVG93::ready() {
    // Fdc слишком быстр в режиме без задержки, подождите, пока CPU обработает DRQ, но не больше 'end_waiting_am'
    if(!(rqs & R_DRQ)) return true;
    if(next > end_waiting_am) return true;
    next += fdd->ticks();
    _ST_SET(S_WAIT, S_WAIT);
    return false;
}

void zxVG93::cmdRead() {
    if(!ready()) return;
    if(rwlen) {
        if(rqs & R_DRQ) opts[TRDOS_STS] |= ST_LOST;
        read_byte();
    } else {
        LOG_DEBUG("S_READ FINISH idx:%i", rwptr);
//        log_to_data(false, "read sector", fdd->get_trk()->content + rwptr - _rwlen, found_sec->trk(), found_sec->sec(), found_sec->head());
        if((opts[TRDOS_CMD] & 0xe0) == C_RSEC) {
            // если чтение сектора - проверяем на CRC
            if (crc != found_sec->crcData()) {
                opts[TRDOS_STS] |= ST_CRCERR;
            }
            // если множественная загрузка секторов
            if (opts[TRDOS_CMD] & CB_MULTIPLE) {
                opts[TRDOS_SEC]++; state = S_CMD_RW;
                return;
            }
        } else if ((opts[TRDOS_CMD] & 0xf0) == C_RADR) {
            // проверяем на CRC
            if (CRC(found_sec->caption - 1, 5) != found_sec->crcId()) {
                opts[TRDOS_STS] |= ST_CRCERR;
            }
        }
        state = S_IDLE;
    }
}

void zxVG93::cmdWrite() {
    if(!ready()) return;
    if(rqs & R_DRQ) {
        // потеря данных
        opts[TRDOS_STS] |= ST_LOST;
        opts[TRDOS_DAT] = 0;
    }
    // запись байта
    fdd->write(rwptr++, opts[TRDOS_DAT]); rwlen--;
    crc = CRC(opts[TRDOS_DAT], crc);
    if(rwlen) {
        next += fdd->ticks();
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WRITE);
    } else {
        // запись CRC
        fdd->write(rwptr++, (uint8_t)(crc >> 8)); fdd->write(rwptr++, (uint8_t)crc);
        // завершение операции
        LOG_DEBUG("S_WRITE FINISH idx:%i", rwptr);
        //log_to_data(false, "write sector", found_sec->content, found_sec->trk(), found_sec->sec(), head);
        // проверка на множественные сектора
        auto mult = opts[TRDOS_CMD] & CB_MULTIPLE;
        if(mult) opts[TRDOS_SEC]++;
        state =  mult ? S_CMD_RW : S_IDLE;
    }
}

void zxVG93::cmdWriteTrackData() {
    if(!ready()) return;
    // потеря данных
    if(rqs & R_DRQ) {
        opts[TRDOS_STS] |= ST_LOST;
        opts[TRDOS_DAT] = 0;
    }
    auto d = opts[TRDOS_DAT]; auto v = d;
    if(d == 0xF5) { v = 0xA1; }
    else if(d == 0xF6) { v = 0xC2; }
    else if(d == 0xFB || d == 0xFE) { start_crc = (int8_t)rwptr; }
    else if(d == 0xF7) {
        // считаем КК
        crc = CRC(fdd->get_trk()->content + start_crc, rwptr - start_crc);
        fdd->write(rwptr++, (uint8_t)(crc >> 8)); rwlen--; v = (uint8_t)crc;
        start_crc = -1;
    }
    fdd->write(rwptr++, v); rwlen--;
    if(rwlen) {
        next += fdd->ticks();
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WR_TRACK_DATA);
    } else {
        LOG_DEBUG("S_WR_TRACK_DATA FINISH idx:%i", rwptr);
        //log_to_data(false, "write track", fdd->get_trk()->content, opts[TRDOS_TRK], opts[TRDOS_SEC], head);
        fdd->get_trk()->update(); state = S_IDLE;
        if(fdd->track() == 0) {
            LOG_DEBUG("SET DATA SECTOR 9", 0);
            fdd->fillDiskConfig(fdd->get_sec(9));
        }
    }
}

void zxVG93::cmdWriteSector() {
    if(rqs & R_DRQ) {
        opts[TRDOS_STS] |= ST_LOST;
        state = S_IDLE;
    } else {
        rwptr = (uint16_t) ((found_sec->caption - fdd->get_trk()->content) + 6 + 22);
        for(rwlen = 0; rwlen < 12; rwlen++) fdd->write(rwptr++, 0);
        for(rwlen = 0; rwlen < 3; rwlen++) fdd->write(rwptr++, 0xa1);
        auto dat = (uint8_t) ((opts[TRDOS_CMD] & CB_WRITE_DEL) ? 0xF8 : 0xFB);
        fdd->write(rwptr++, dat); rwlen = found_sec->len();
        crc = CRC(dat); state = S_WRITE;
        LOG_DEBUG("S_WRITE idx:%i len:%i", rwptr, rwlen);
    }
}

void zxVG93::get_index(int s_next) {
    auto t = fdd->get_trk();
    auto trlen = (uint32_t)(t->len * fdd->ticks());
    auto ticks = (uint32_t)(next % trlen);
    next += (trlen - ticks);
    rwptr = 0; rwlen = t->len;
    _ST_SET(S_WAIT, s_next);
}

void zxVG93::cmdWriteTrack() {
    if(rqs & R_DRQ) {
        opts[TRDOS_STS] |= ST_LOST;
        state = S_IDLE;
    } else {
        start_crc = -1;
        get_index(S_WR_TRACK_DATA);
        int i;
        for(i = 0 ; i < 80; i++) fdd->write(rwptr++, 0x4e);
        for(i = 0 ; i < 12; i++) fdd->write(rwptr++, 0);
        for(i = 0 ; i < 3;  i++) fdd->write(rwptr++, 0xc2);
        fdd->write(rwptr++, 0xfc);
        //for(i = 0 ; i < 40; i++) fdd->write(rwptr++, 0x4e);
        end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
        rwlen -= rwptr;
        LOG_DEBUG("S_WRITE_TRACK idx:%i len:%i", rwptr, rwlen);
    }
}

void zxVG93::cmdReadWrite() {
    load();
    auto cmd = opts[TRDOS_CMD];
    if(((cmd & 0xe0) == C_WSEC || (cmd & 0xf0) == C_WTRK) && fdd->is_protect()) {
        // проверка на защиту от записи в операциях записи
        opts[TRDOS_STS] |= ST_WRITEP;
        state = S_IDLE;
    } else if((cmd & 0xc0) == 0x80 || (cmd & 0xf8) == C_RADR) {
        // операции чтения/записи секторов или чтения адреса - поиск сектора
        end_waiting_am = next + 5 * Z80FQ / FDD_RPS;
        find_sec();
    } else if((cmd & 0xf8) == C_WTRK) {
        // запись дорожки(форматирование)
        next += 3 * fdd->ticks();
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WRTRACK);
    } else if((cmd & 0xf8) == C_RTRK) {
        // чтение дорожки
        LOG_DEBUG("S_RW_CMD - C_RTRK", 0);
        rwptr = 0; rwlen = fdd->get_trk()->len;
        get_index(S_READ);
    } else {
        state = S_IDLE;
    }
}

void zxVG93::cmdType1() {
//    LOG_INFO("S_TYPE1_CMD cmd:%i", opts[TRDOS_CMD]);
    opts[TRDOS_STS] = (opts[TRDOS_STS]|ST_BUSY) & ~(ST_DRQ | ST_CRCERR | ST_SEEKERR | ST_WRITEP);
    rqs = R_NONE;
    if(fdd->is_protect()) opts[TRDOS_STS] |= ST_WRITEP;
    fdd->engine(next + 2 * Z80FQ);
    next += 1 * Z80FQ / 1000;
    // поиск/восстановление
    auto cmd = S_SEEKSTART;
    if(opts[TRDOS_CMD] & 0xE0) {
        // один шаг
        if(opts[TRDOS_CMD] & 0x40) direction = (uint8_t)((opts[TRDOS_CMD] & CB_SEEK_DIR) ? -1 : 1);
        cmd = S_STEP;
    }
    _ST_SET(S_WAIT, cmd);
}

void zxVG93::cmdFindSec() {
    load();
    auto cmd = opts[TRDOS_CMD];
    if ((cmd & 0xf0) == C_RADR) {
        // чтение адресного маркера
        opts[TRDOS_SEC] = opts[TRDOS_TRK];
        rwptr = (uint16_t) (found_sec->caption - fdd->get_trk()->content); rwlen = 6;
        crc = CRC(found_sec->caption[-1]);
        LOG_DEBUG("S_READ idx:%i len:%i", rwptr, rwlen);
        read_byte();
    } else if (cmd & 0x20) {
        // запись сектора
        next += fdd->ticks() * 9;
        rqs = R_DRQ; opts[TRDOS_STS] |= ST_DRQ;
        _ST_SET(S_WAIT, S_WRSEC);
    } else {
        // чтение сектора
        cmd = found_sec->content[-1];
        cmd == 0xf8 ? opts[TRDOS_STS] |= ST_RECORDT : opts[TRDOS_STS] &= ~ST_RECORDT;
        rwptr = (uint16_t) (found_sec->content - fdd->get_trk()->content); rwlen = found_sec->len();
        LOG_DEBUG("S_READ idx:%i len:%i", rwptr, rwlen);
        crc = CRC(cmd); read_byte();
    }
}

void zxVG93::cmdPrepareRW() {
    LOG_DEBUG("S_PREPARE_CMD cmd:%i trk:%i sec:%i head:%i 15ms:%i", opts[TRDOS_CMD], opts[TRDOS_TRK], opts[TRDOS_SEC], head, opts[TRDOS_CMD] & CB_DELAY);
    if(opts[TRDOS_CMD] & CB_DELAY) next += (Z80FQ * 15 / 1000);
    // сброс статуса
    opts[TRDOS_STS] = (opts[TRDOS_STS] | ST_BUSY) & ~(ST_DRQ | ST_LOST | ST_NOT_SEC | ST_RECORDT | ST_WRITEP);
    _ST_SET(S_WAIT, S_CMD_RW);
}

void zxVG93::cmdStep() {
    //static const uint32_t steps[] = { 6, 12, 20, 30 };
    auto cmd = opts[TRDOS_CMD];
    if(fdd->track() == 0 && (cmd& 0xf0) == 0) {
        // RESTORE
        opts[TRDOS_TRK] = 0;
        state = S_VERIFY;
    } else {
        // позиционирование дорожки
        if(!(cmd & 0xe0) || (cmd & CB_SEEK_TRKUPD)) opts[TRDOS_TRK] += direction;
        // проверка на допустимые дорожки
        int cyl = fdd->track() + direction; if(cyl < 0) cyl = 0; if(cyl >= PHYS_CYL) cyl = PHYS_CYL;
        fdd->track(cyl);
        next += Z80FQ / 1000;
        _ST_SET(S_WAIT, (cmd & 0xe0) ? S_VERIFY : S_SEEK);
    }
}

void zxVG93::cmdSeek() {
    if(opts[TRDOS_DAT] == opts[TRDOS_TRK]) {
        state = S_VERIFY;
    } else {
        direction = (uint8_t) ((opts[TRDOS_DAT] < opts[TRDOS_TRK]) ? -1 : 1);
        state = S_STEP;
    }
}

void zxVG93::cmdVerify() {
    find_sec();
    if(!(opts[TRDOS_CMD] & CB_SEEK_VERIFY)) {
        state = S_IDLE;
    } else {
        end_waiting_am = next + 6 * Z80FQ / FDD_RPS;
    }
}

void zxVG93::find_sec() {
    load();
    found_sec = nullptr;
    auto wait = 10 * Z80FQ / FDD_RPS;
    opts[TRDOS_STS] &= ~ST_CRCERR;
    if(fdd->engine() && fdd->is_disk()) {
        auto t = fdd->get_trk();
        for (int i = 0; i < t->total_sec; ++i) {
            auto sec = fdd->get_sec(i);
            auto secMatch = sec->sec() == opts[TRDOS_SEC];
            auto trkMatch = sec->trk() == opts[TRDOS_TRK];
            auto headMatch = sec->head() == head;
            if (secMatch && trkMatch && headMatch) {
                found_sec = sec;
                wait = sec->caption - t->content;
                break;
            }
        }
    }
    if(found_sec) {
        if(CRC(found_sec->caption - 1, 5) == found_sec->crcId()) {
            next += wait;
            _ST_SET(S_WAIT, S_FIND_SEC);
            return;
        }
        opts[TRDOS_STS] |= ST_CRCERR;
        found_sec = nullptr;
    } else opts[TRDOS_STS] |= ST_NOT_SEC;
    state = S_IDLE;
}

uint8_t zxVG93::vg93_read(uint8_t port, int tact) {
    exec(tact);
    uint8_t ret(0xff);
    switch(port) {
        case 0x1F: rqs &= ~R_INTRQ; ret = opts[TRDOS_STS]; break;
        case 0x3F: ret = opts[TRDOS_TRK]; break;
        case 0x5F: ret = opts[TRDOS_SEC]; break;
        case 0x7F: ret = opts[TRDOS_DAT]; opts[TRDOS_STS] &= ~ST_DRQ; rqs &= ~R_DRQ; break;
        case 0xFF: ret = rqs;             break;
    }
    return ret;
}

void zxVG93::vg93_write(uint8_t port, uint8_t v, int tact) {
    exec(tact);
    switch(port) {
        case 0x1F:
            // прерывание команды
            if((v & 0xf0) == C_INTERRUPT) {
                state = S_IDLE; rqs = R_INTRQ; opts[TRDOS_STS] &= ~ST_BUSY;
                return;
            }
            if(opts[TRDOS_STS] & ST_BUSY) return;
            opts[TRDOS_CMD] = v;
            next = ULA->_TICK + tact;
            opts[TRDOS_STS] |= ST_BUSY; rqs = R_NONE;
            // команды чтения/записи
            if(opts[TRDOS_CMD] & 0x80) {
                // выйти, если нет диска
                if(opts[TRDOS_STS] & ST_NOTRDY) { state = S_IDLE; rqs = R_INTRQ; return; }
                // продолжить вращать диск
                if(fdd->engine()) fdd->engine(next + 2 * Z80FQ);
                state = S_PREPARE_CMD;
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
            head = (uint8_t)((v & 0x10) == 0);
//            LOG_INFO(("WRITE SYSTEM: HEAD %i MFM:%i HLT: %i"), head, (uint8_t)((v & 16) != 0), (uint8_t)((v & 8) != 0));
            // сброс контроллера
            if(!(v & CB_RESET)) { opts[TRDOS_STS] = ST_NOTRDY; rqs = R_INTRQ; fdd->engine(0); state = S_IDLE; }
    }
}

int zxVG93::read_sector(int num, int sec) {
    auto sector = fdds[num].get_sec(sec);
    if(sector) memcpy(&opts[ZX_PROP_VALUES_SECTOR], sector->content, 256);
    else memset(&opts[ZX_PROP_VALUES_SECTOR], 0, 256);
    return sector != nullptr;
}

int zxVG93::count_files(int num, int is_del) {
    auto sector = fdds[num].get_sec(9);
    return sector ? sector->content[is_del ? 244 : 228] : 0;
}
