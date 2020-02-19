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

uint32_t Z80FQ;

zxDisk::zxDisk(int _trks, int _heads) {
    trks = _trks; heads = _heads;
    uint16_t data_len = MAX_TRK_LEN;
    int size = _trks * _heads * data_len;
    raw = new uint8_t[size];
    memset(raw, 0, (size_t)size);
    memset(tracks, 0, sizeof(tracks));
    for(int i = 0; i < _trks; ++i) {
        for(int j = 0; j < _heads; ++j) {
            auto t = &tracks[i][j];
            t->len = data_len;
            t->content = raw + data_len * (i * _heads + j);
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

bool zxFDD::read_trd(const void *data, size_t data_size) {
    make_trd();
    if(data_size > 655360) data_size = 655360;
    for(size_t i = 0; i < data_size; i += 256) {
        write_sec(i >> 13, (i >> 12) & 1, ((i >> 8) & 0x0f) + 1, (const uint8_t *)data + i);
    }
    return true;
}

bool zxFDD::read_fdi(const void *data, size_t data_size) {
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
