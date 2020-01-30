//
// Created by Sergey on 24.01.2020.
//

#include <math.h>
#include "zxCommon.h"
#include "zxBetaDisk.h"

struct CRC {
    CRC(): value(0xffff) { }
    uint16_t value;
    void add(uint8_t v8) {
        value ^= (v8 << 8);
        for(int i = 0; i < 8; i++) value = (uint16_t)((value & 0x8000) ? ((value << 1) ^ 0x1021) : (value << 1));
    }
};

static uint8_t dataSec[1024];
static uint8_t dataGap[32];

static TrackData* track_data;
static SectorData* sec_data;

static CRC crc;
static bool crc_just_stored;
static bool sector_address_zone;
static bool sector_data_zone;
static bool multiple, check_side;
static bool wait_15ms;

static int data_mark;
static int sec_length;
static int index, length_byte;
static int max_bytes;

static uint8_t byte_counter;
static uint8_t cyl_byte;
static uint8_t head_byte;
static uint8_t sec_byte;
static uint8_t expected_side;
static uint8_t* rw;
static uint8_t* tmp;

void zxBetaDisk::schedule_data(bool read, uint8_t* data, size_t length, bool action) {
    auto _st = &_state; read_state = write_state = nullptr;
    _st->action = action; rw = data; length_byte = length; index = 0;
    if(read) read_state = _st; else write_state = _st;
}

void zxBetaDisk::read_next_byte() {
    if(index < length_byte) {
        r_data = rw[index++]; drq = 1;
    } else {
        if(multiple) { r_sector++; read_sectors_end(); }
        else { read_state = nullptr; intrq = 1; drq = 0; set_busy(false); }
    }
}

void zxBetaDisk::write_next_byte() {
    if(write_state->action) {
        if(byte_counter == 0) {
            if(!track_data) { write_state = nullptr; write_fault = intrq = 1; drq = 0; set_busy(false); return; }
            track_data->clear();
        }
        if(mfm) {
            if(!crc_just_stored) {
                // нормальная дешифровка
                switch(r_data) {
                    case 0xf5: *rw++ = 0xa1; crc = CRC(); crc_just_stored = false; break;
                    case 0xf6: *rw++ = 0xc2; crc_just_stored = false; break;
                    case 0xf7: *rw++ = (uint8_t)((crc.value >> 8) & 0xff); *rw++ = (uint8_t)((crc.value >> 0) & 0xff); crc_just_stored = true; break;
                    default: *rw++ = r_data; crc_just_stored = false; break;
                }
            } else { *rw++ = r_data; crc_just_stored = false; } // баг дешифровки после записи контрольной суммы

        } else {
            if(!crc_just_stored) {
                switch(r_data) {
                    case 0xf7: *rw++ = (uint8_t)((crc.value >> 8) & 0xff); *rw++ = (uint8_t)((crc.value >> 0) & 0xff); crc_just_stored = true; break;
                    case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfe: *rw++ = r_data; crc = CRC(); crc_just_stored = false; break;
                    default: *rw++ = r_data; crc_just_stored = false; break;
                }
            } else { *rw++ = r_data; crc_just_stored = false; }
        }
        if(!crc_just_stored) {
            switch(r_data) {
                case 0xfe:
                    if(!sector_address_zone && !sector_data_zone) { sector_address_zone = true; sector_data_zone = false; index = -1; } break;
                case 0xf8: case 0xfb:
                    if(sector_address_zone) {
                        sector_address_zone = false;
                        sector_data_zone = true;
                        index = -1;
                        sec_data = new SectorData(cyl_byte, head_byte, sec_byte, (uint8_t)length_byte, r_data == 0xf8);
                        sec_length = 128 << length_byte;
                        track_data->add_sector(sec_data);
                    }
                    break;
            }
        }
        auto len = (rw - TMP_BUF);
        for(int i = 0; i < len; i++) {
            if(sector_address_zone) {
                switch(index) {
                    case 0: cyl_byte    = rw[i]; break;
                    case 1: head_byte   = rw[i]; break;
                    case 2: sec_byte    = rw[i]; break;
                    case 3: length_byte = rw[i]; break;
                }
            }
            if(sector_data_zone && index >= 0) {
                if(index < sec_length) sec_data->set_data_byte(index, rw[i]);
                else { sector_address_zone = false; sector_data_zone = false; }
            }
            crc.add(rw[i]);
            index++;
            byte_counter++;
            if(byte_counter < max_bytes) drq = 1;
            else { SAFE_DELETE(write_state); intrq = 1; drq = 0; set_busy(false); return; }
        }
        drq = 1;
    } else {
        *rw++ = r_data; index++;
        if(index < length_byte) drq = 1;
        else {
            rw -= length_byte;
            auto length = 128 << sec_data->get_length_byte();
            for(int i = 0; i < length_byte; i++) sec_data->set_data_byte(i, rw[i]);
            LOG_INFO("store_sec_data length_byte: %i length_sec:%i", length_byte, length);
            if(multiple) { r_sector++; write_sectors_end(); }
            else { SAFE_DELETE(write_state); drq = 0; intrq = 1; set_busy(false); }
        }
    }
}

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

// Производит поиск сектора по адресу:
// - текущая дорожка
// - текущая головка
// - регистр r_sector
// Возвращает найденный сектор либо null, если диск не
// вставлен либо на текущей дорожке нет сектора с данным
// адресом.
SectorData* zxBetaDisk::get_sector_data() {
    if(!ready()) return nullptr;

    auto image = get_current_image();
    auto track_data = image->get_track(get_current_track(), head);
    SectorData* sec_data = nullptr;
    if(track_data) {
        auto sec_count = track_data->get_sec_count();
        for(int sec_index = 0; sec_index < sec_count; sec_index++) {
            auto _sd = track_data->get_sector(sec_index);
            auto track_match = (_sd->get_cyl_byte() == r_track);
            auto head_match = (!check_side || (_sd->get_head_byte() == expected_side));
            auto sec_match = (_sd->get_sec_byte() == r_sector);
            if(track_match && head_match && sec_match) { sec_data = _sd; break; }
        }
    }
    return sec_data;
}

void zxBetaDisk::hardware_reset_controller() {
    int_on_ready = int_on_unready = int_on_index_pointer = false;
    intrq = drq = busy = hld = 0;
    seek_error = sec_error = dirc = 0;
    r_sector = 1; r_track = 0; r_command = 0x03; r_data = 0;
    read_state = write_state = nullptr;
    process_command(0x03);
}

// Бит      | Восст. и позиц.  |    Запись сектора  | Чтение сектора    |   Чтение адреса   |   Запись дорожки  |   Чтение дорожки  |
//  7       |               Г   о   т   о   в   н   о   с   т   ь       д   и   с   к   о   в   о   д   а                           |
//  6       |        З А Щ И Т А   З А П И С И      |                   0                   |   ЗАЩИТА ЗАПИСИ   |         0         |
//  5       |     ЗАГРУЗКА     |       ОШИБКА       |    ПОВТОРЯЕТ      |                   |      ОШИБКА       |                   |
//          | МАГНИТНОЙ ГОЛОВКИ|       ЗАПИСИ       |  ЗНАЧЕНИЕ БИТА    |         0         |      ЗАПИСИ       |         0         |
//  4       |  ОШИБКА ПОИСКА   |          C  Е  К  Т  О  Р   Н  Е   Н  А  Й  Д  Е  Н        |                   0                   |
//  3       |           О   Ш   И   Б   К   А           C       R       C                   |                   0                   |
//  2       |    ДОРОЖКА 0     |     П       О       Т       Е       Р       Я           Д       А       Н       Н       Ы       Х  |
//  1       |ИНДЕКСНЫЙ ИМПУЛЬС |       З       А       П       Р       О       С       Д       А       Н       Н       Ы       Х    |
//  0       |                И   д   е   т           в   ы   п   о   л   н   е   н   и   е       к   о   м   а   н   д   ы          |
uint8_t zxBetaDisk::get_status() {
    // 0 - занято
    // 7 - готовность дисковода
    auto rdy = (uint8_t)ready();
    auto is_write_protected = rdy && get_current_image()->is_write_protected();
    int status = 0;
    const char* nm = "";
    // after Restore, Seek, Step commands and also Interrupt command if it actually did not unterrupt anything.
    if((last_command & 0x80) == 0 || (last_command & 0xf0) == 0xd0) {
        status =(to_bit(is_write_protected) << 6) | ((get_hld() & hlt) << 5) | (seek_error << 4) |
                (to_bit(get_current_track() == 0) << 2) | (to_bit(rdy ? index_pointer() : 0) << 1);
        nm = "seek/restore";
    }
    // after Read Address command
    if((last_command & 0xf0) == 0xc0) {
        status = (sec_error << 4) | (drq << 1);
        nm = "read address";
    }
    // after Read Sector command
    if((last_command & 0xe0) == 0x80) {
        status = (record_type << 6) | (sec_error << 4) | (drq << 1);
        nm = "read sector";
    }
    // after Read Track command
    if((last_command & 0xf0) == 0xe0) {
        status = (drq << 1);
        nm = "read track";
    }
    // after Write Sector command
    if((last_command & 0xe0) == 0xa0) {
        status = (to_bit(is_write_protected) << 6) | (write_fault << 5) |
                (sec_error << 4) | (drq << 1);
        nm = "write sector";
    }
    // after Write Track command
    if((last_command & 0xf0 ) == 0xf0) {
        status = (to_bit(is_write_protected) << 6) | (write_fault << 5) | (drq << 1);
        nm = "format";
    }
    intrq = 0;
    status |= (((!rdy) << 7) | busy);
    LOG_INFO("%s = %i", nm, status);
    return (uint8_t)status;
}

// Field		FM					MFM
// =====================================================
// GAP I		0xff * 16 (min)		0x4e * 32 (min)
// GAP II		0xff * 11 (exact)	0x4e * 22 (exact)
// GAP III		0xff * 10 (min)		0x4e * 24 (min)
// GAP IV		0xff * 16 (min)		0x4e * 32 (min)
// SYNC			0x00 * 6  (exact)	0x00 * 12 (exact)
size_t zxBetaDisk::build_GAP(int gap_num) {
    static size_t cGAP[] = { 0, 16, 11, 12, 16 };
    size_t count = cGAP[gap_num], v8 = 0xff;
    if(mfm) { v8 = 0x4e; count *= 2; }
    memset(dataGap, v8, count);
    return count;
}

size_t zxBetaDisk::build_SYNC() {
    size_t count = mfm ? 12 : 6;
    memset(dataGap, 0, count);
    return count;
}

zxBetaDisk::zxBetaDisk() {
    drive = head = 0;
    index_pointer_interval = 200;
    index_pointer_length = 10;
    //track_rw_timeout = index_pointer_interval * 2;
    //rw_timeout = 0.15;
    hld_extratime = index_pointer_interval * 15;
    intrq = drq = hlt = busy = hld = 0;
    idle_since = currentTimeMillis();
    seek_error = sec_error = write_fault = 0;
    record_type = 0;
    dirc = 1;
    last_index_pointer_time = 0;
    r_sector = 1;
    r_track = r_data = 0;
    r_command = 0x03;
    last_command = 0x00;
    addr_sec_index = 0x00;
    read_state = write_state = nullptr;
    mfm = int_on_ready = int_on_unready = int_on_index_pointer = false;
}

void zxBetaDisk::insert(int drive_code, zxDiskImage* image) {
    if(drive_code >= 0 && drive_code < 4) {
        // сначала извлекаем текущий диск, если он есть
        if(disks[drive_code].image) eject(drive_code);
        // вставляем новый
        disks[drive_code].image = image;
        // генерируем событие, если необходимо
        on_drive_ready(drive_code);
    }
}

void zxBetaDisk::eject(int drive_code) {
    if(drive_code >= 0 && drive_code < 4) {
        delete disks[drive_code].image;
        disks[drive_code].image = nullptr;
        on_drive_unready(drive_code);
    }
}

void zxBetaDisk::process_command(uint8_t cmd) {
    r_command = cmd;
    LOG_INFO("%i PC %i", cmd, zxALU::PC);
    // проверка не команду прерывания
    if((r_command & 0xf0) == 0xd0) {
        // прерывание команды
        LOG_INFO("cmd interrupt", 0);
        if(is_busy()) { read_state = write_state = nullptr; } else { seek_error = 0; last_command = r_command; }
        set_busy(false);
        int_on_ready = (r_command & 0x01) != 0;
        int_on_unready = (r_command & 0x02) != 0;
        int_on_index_pointer = (r_command & 0x04) != 0;
        if(r_command & 0x08) intrq = 1;
        return;
    }
    // другие команды не принимаются, если контроллер занят
    if(is_busy()) return;
    last_command = r_command;
    int_on_ready = int_on_unready = int_on_index_pointer = false;
    // 0000 HVXX - восстановление
    //          переход на нулевую дорожку
    // позиционирование:
    // I=1 дорожка меняет свое значение
    // H - определяет положение магнитной головки дисковода во время выполнения команды
    // V - в случае установки магнитной головки в рабочее положение (h=l) этот бит используется для задания режима проверки положения головки. Если
    //     бит установлен, то в конце операции содержимое регистра дорожки сравнивается с действительным номером, считанным с дискеты.
    // 010I HVXX - на один шаг вперед
    // 011I HVXX - на один шаг назад
    // 001I HVXX - на один шаг в том же направлении
    // 0001 HVXX - позиционирование магнитной головки на заданную дорожку. Номер в регистр данных.
    if(!(r_command & 0x80)) {
        auto image = get_current_image();
        // обработка первой группы команд: восстановление, поиск, шаг, шаг назад, шаг вперед
        auto H = (uint8_t)((r_command & 0x08) >> 3);
        auto V = (uint8_t)((r_command & 0x04) >> 2);
        // мы работаем на максимальной скорости
        //auto rate = (r_command & 0x03);
        set_busy(true);
        drq = intrq = seek_error = 0;
        hld = H;
        auto cmd_restore = (r_command & 0x70) == 0;
        auto cmd_seek = (r_command & 0x70) == 0x10;
        if(cmd_restore) {
            dirc = r_track = r_data = 0; addr_sec_index = 0;
            set_current_track(0);
            if(V) {
                // проверяем соответствие идентификаторов на диске и в регистре дорожки
                seek_error = 1;
                if(ready()) {
                    hld = 1;
                    auto track_data = image->get_track(get_current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) seek_error = (uint8_t)(sec_data->get_cyl_byte() != r_track);
                }
            }
            LOG_INFO("cmd restore error: %i", seek_error);
        } else if(cmd_seek) {
            // в регистре данных искомый номер дорожки
            if(r_track != r_data) dirc = to_bit(r_track < r_data);
            r_track = r_data;
            set_current_track(r_track);
            addr_sec_index = 0;
            if(V) {
                seek_error = 1;
                // проверяем соответствие идентификаторов на диске и в регистре дорожки
                if(ready()) {
                    hld = 1;
                    auto track_data = image->get_track(get_current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) seek_error = (uint8_t)(sec_data->get_cyl_byte() != r_track);
                }
            }
            LOG_INFO("cmd seek trk:%i seek_error:%i", r_track, seek_error);
        } else {
            // 010I HVXX - на один шаг вперед
            // 011I HVXX - на один шаг назад
            // 001I HVXX - на один шаг в том же направлении
            switch(r_command & 0x60) {
                case 0x40: dirc = 1; break;
                case 0x60: dirc = 0; break;
            }
            auto phys_track = get_current_track();
            phys_track = (dirc ? (phys_track + 1) : (phys_track - 1));
            if(phys_track < 0) phys_track = 0;
            if(phys_track > 255) phys_track = 255;
            r_track = (uint8_t)phys_track;
            if((r_command & 0x10)) {
                set_current_track(r_track);
            }
            addr_sec_index = 0;
            if(V) {
                // проверяем, существует ли данная дорожка
                seek_error = 1;
                if(ready()) {
                    hld = 1;
                    auto track_data = image->get_track(get_current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) seek_error = (uint8_t)(sec_data->get_cyl_byte() != r_track);
                }
            }
            LOG_INFO("cmd step trk:%i dir:%i error:%i", r_track, dirc, seek_error);
        }
        intrq = 1;
        set_busy(false);
        return;
    }
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

    // 1110 0E00 - Чтение дорожки
    //              Эта команда считывает с дорожки всю имеющуюся на ней информацию, включая поля пробелов, заголовков и служебные байты.
    // 1100 0E00 - Чтение адреса
    //              По этой команде с дискеты считывается первый встреченный заголовок сектора. Из поля заголовка передаются 6 байтов —
    //              4 байта информационные (номер дорожки, номер стороны, номер сектора и длина сектора) и 2 байта контрольного кода.
    //              При выполнении этой Команды содержимое регистра дорожки пересылается в регистр сектора
    switch(r_command & 0xe0) {
        case 0x80: read_sectors_begin(); return;
        case 0xa0: write_sectors_begin(); return;
    }
    switch(r_command & 0xf0) {
        case 0xc0: read_address(); return;
        case 0xe0: read_track(); return;
//        case 0xf0: write_track(); return;
    }
}

void zxBetaDisk::read_sectors_begin() {
    LOG_INFO("", 0);
    multiple        = to_bool(r_command & 0x10);
    expected_side   = to_bit( r_command & 0x08);
    wait_15ms       = to_bool(r_command & 0x04);
    check_side      = to_bool(r_command & 0x02);
    data_mark       = false;//to_bool(r_command & 0x01);
    drq = intrq = sec_error = 0;
    auto rdy = ready(); set_busy(rdy);
    if(rdy) { hld = 1; read_sectors_end(); } else intrq = 1;
}

void zxBetaDisk::read_sectors_end() {
    LOG_INFO("", 0);
    sec_data = get_sector_data();
    if(sec_data) {
        auto length = (size_t)(128 << sec_data->get_length_byte());
        for(int i = 0; i < length; i++ ) dataSec[i] = sec_data->get_data_byte(i);
        record_type = (uint8_t)sec_data->is_deleted(-1);
        schedule_data(true, dataSec, length, false);
        read_next_byte();
    } else { read_state = nullptr; sec_error = intrq = 1; drq = 0; set_busy(false); }
}

void zxBetaDisk::write_sectors_begin() {
    LOG_INFO("", 0);
    multiple        = to_bool(r_command & 0x10);
    expected_side   = to_bit( r_command & 0x08);
    wait_15ms       = to_bool(r_command & 0x04);
    check_side      = to_bool(r_command & 0x02);
    data_mark       = to_bool(r_command & 0x01);
    drq = intrq = sec_error = write_fault = 0;
    auto rdy = ready() && get_current_image()->is_write_protected();
    set_busy(rdy);
    if(rdy) { hld = 1; write_sectors_end(); } else intrq = 1;
}

void zxBetaDisk::write_sectors_end() {
    LOG_INFO("", 0);
    sec_data = get_sector_data();
    if(sec_data) {
        sec_data->is_deleted(data_mark);
        schedule_data(false, nullptr, (size_t)(128 << sec_data->get_length_byte()), false);
        drq = 1;
    } else { write_state = nullptr; sec_error = intrq = 1; drq = 0; set_busy(false); }
}

void zxBetaDisk::read_address() {
    LOG_INFO("", 0);
    wait_15ms = to_bool(r_command & 0x04);
    multiple  = false;

    drq = intrq = sec_error = 0; set_busy(false);
    if(ready()) {
        auto image = get_current_image();
        track_data = image->get_track(get_current_track(), head);
        sec_data = track_data ? track_data->get_sector(addr_sec_index) : nullptr;
        if(sec_data) {
            hld = 1;
            set_busy(true);
            crc = CRC();
            dataSec[0] = sec_data->get_cyl_byte();
            dataSec[1] = sec_data->get_head_byte();
            dataSec[2] = sec_data->get_sec_byte();
            dataSec[3] = sec_data->get_length_byte();
            if(mfm) crc.add(0xa1);
            crc.add((uint8_t)(sec_data->is_deleted(-1) ? 0xf8 : 0xfb));
            crc.add(dataSec[0]); crc.add(dataSec[1]); crc.add(dataSec[2]); crc.add(dataSec[3]);
            dataSec[4] = (uint8_t)((crc.value >> 0) & 0xff); dataSec[5] = (uint8_t)((crc.value >> 8) & 0xff);
            addr_sec_index++;
            if(addr_sec_index == track_data->get_sec_count()) addr_sec_index = 0;
            schedule_data(true, dataSec, 6, false);
            read_next_byte();
        } else {
            addr_sec_index = 0; // это так, на всякий случай :)
            sec_error = intrq = 1; drq = 0;
            read_state = nullptr;
        }
    } else intrq = 1;
}

void zxBetaDisk::read_track() {
    LOG_INFO("", 0);
    multiple = false;
    set_busy(true);
    drq = intrq = seek_error = 0; read_state = nullptr;
    if(!ready()) { intrq = 1; set_busy(false); return; }
    hld = 1;
    auto image = get_current_image();
    track_data = image->get_track(get_current_track(), head);
    auto sec_count = track_data->get_sec_count();

    rw = &TMP_BUF[524288]; tmp = rw;

    // GAP IVa
    ssh_memcpy(&tmp, &dataGap, build_GAP(4));
    // SYNC
    ssh_memcpy(&tmp, &dataGap, build_SYNC());
    // Index Mark
    *tmp++ = 0xfc;
    // GAP I
    ssh_memcpy(&tmp, &dataGap, build_GAP(1));
    // запись каждого сектора
    for(int sec_index = 0; sec_index < sec_count; sec_index++) {
        sec_data = track_data->get_sector(sec_index);
        ssh_memcpy(&tmp, &dataGap, build_SYNC()); // SYNC
        if(mfm) { tmp = ssh_memset(tmp, 0xa1, 3); } // if MFM then 0xa1 * 3 (exact)
        *tmp++ = 0xfe; // IDAM
        // ID
        *tmp++ = sec_data->get_cyl_byte();
        *tmp++ = sec_data->get_head_byte();
        *tmp++ = sec_data->get_sec_byte();
        *tmp++ = sec_data->get_length_byte();
        // CRC
        auto crc1 = CRC();
        crc1.add(sec_data->get_cyl_byte());
        crc1.add(sec_data->get_head_byte());
        crc1.add(sec_data->get_sec_byte());
        crc1.add(sec_data->get_length_byte());
        *tmp++ = (uint8_t)((crc1.value >> 8) & 0xff);
        *tmp++ = (uint8_t)((crc1.value >> 0) & 0xff);
        ssh_memcpy(&tmp, &dataGap, build_GAP(2)); // GAP II
        ssh_memcpy(&tmp, &dataGap, build_SYNC()); // SYNC
        if(mfm) { tmp = ssh_memset(tmp, 0xa1, 3); } // if MFM then 0xa1 * 3 (exact)
        *tmp++ = 0xfb; // DAM
        // Data
        auto crc2 = CRC();
        sec_length = 128 << sec_data->get_length_byte();
        for(int i = 0; i < sec_length; i++) {
            *tmp++ = sec_data->get_data_byte(i);
            *tmp++ = sec_data->get_data_byte(i);
        }
        // CRC
        *tmp++ = (uint8_t)((crc2.value >> 8) & 0xff);
        *tmp++ = (uint8_t)((crc2.value >> 0) & 0xff);
        ssh_memcpy(&tmp, &dataGap, build_GAP(3)); // GAP III
    }
    // GAP IV b
    ssh_memcpy(&tmp, &dataGap, build_GAP(4));
    // дополняем до приблизительной типичной длины дорожки
    auto required_length = (tmp - rw) - (mfm ? 6250 : 3125);
    tmp = ssh_memset(tmp, mfm ? 0x4e : 0xff, required_length);
    schedule_data(true, rw, tmp - rw, false);
    read_next_byte();
}

void zxBetaDisk::write_track() {
    LOG_INFO("", 0);
    drq = intrq = seek_error = write_fault = 0; write_state = nullptr;
    auto image = get_current_image();
    auto rdy = ready() && image->is_write_protected();
    set_busy(rdy);
    if(rdy) {
        hld = 1;
        track_data = image->get_track(get_current_track(), head);
        byte_counter = 0; max_bytes = mfm ? 6250 : 3125;
        crc = CRC();
        crc_just_stored = sector_address_zone = sector_data_zone = false;
        index = -1; sec_length = length_byte = 0;
        cyl_byte = head_byte = sec_byte = 0;
        schedule_data(false, &TMP_BUF[524288], 0, true);
    } else intrq = 1;
}

zxDiskImage* zxDiskImage::openTRD(const char* path) {
    // UNDONE: имя диска из служебного сектора можно использовать как комментарий
    size_t length;
    auto trd_data = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length);
    if(!trd_data || !length) {
        LOG_DEBUG("Wrong TRD data", 0);
        return nullptr;
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
    auto image = new zxDiskImage(head_count, cyl_count, false, "");
    int data_index = 0;
    for(int cyl_index = 0; cyl_index < cyl_count; cyl_index++) {
        auto cyl = image->add_cyl();
        for(int head_index = 0; head_index < head_count; head_index++) {
            for(int sec_index = 0; sec_index < 16; sec_index++) {
                auto cyl_byte   = (uint8_t)cyl_index;
                auto head_byte  = (uint8_t)head_index;
                auto sec_byte   = (uint8_t)(sec_index + 1);
                auto sec        = new SectorData(cyl_byte, head_byte, sec_byte, SEC_LENGTH_0x0100, false);
                for(int sec_data_index = 0; sec_data_index < 256; sec_data_index++) {
                    sec->set_data_byte(sec_data_index, trd_data[data_index]);
                    data_index++;
                }
                cyl[head_index].add_sector(sec);
            }
        }
    }
    return image;
}

zxDiskImage* zxDiskImage::openFDI(const char *path){
    size_t length;
    auto fdi_data = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length);
    if(!fdi_data || length < 14 || fdi_data[0] != 'F' || fdi_data[1] != 'D' || fdi_data[2] != 'I') {
        LOG_DEBUG("Wrong FDI data", 0);
        return nullptr;
    }
    bool write_protect      = fdi_data[3] != 0;
    int cyl_count           = ((fdi_data[5]  << 8) | fdi_data[4]);
    int head_count          = ((fdi_data[7]  << 8) | fdi_data[6]);
    int comment_offset      = ((fdi_data[9]  << 8) | fdi_data[8]);
    int data_offset         = ((fdi_data[11] << 8) | fdi_data[10]);
    int extra_header_length = ((fdi_data[13] << 8) | fdi_data[12]);
    int extra_header_offset = 14;
    int track_header_offset = (extra_header_offset + extra_header_length);
    auto comment            = (char*)&TMP_BUF[700000]; auto tmp = comment;
    while(comment_offset < length && fdi_data[comment_offset] != 0) *tmp++ = fdi_data[comment_offset++];
    *tmp = '\0';
    auto image = new zxDiskImage(head_count, cyl_count, write_protect, comment);
    for(int cyl_index = 0; cyl_index < cyl_count; cyl_index++) {
        auto cyl = image->add_cyl();
        for(int head_index = 0; head_index < head_count; head_index++) {
            int track_data_offset = (
                    data_offset +   (fdi_data[track_header_offset + 3] << 24) +
                    (fdi_data[track_header_offset + 2] << 16) +
                    (fdi_data[track_header_offset + 1] <<  8) +
                    (fdi_data[track_header_offset + 0] <<  0));
            int sec_count = fdi_data[track_header_offset + 6];
            for(int sec_index = 0; sec_index < sec_count; sec_index++) {
                int sec_header_offset   = track_header_offset + 7 + (sec_index * 7);
                auto cyl_byte           = fdi_data[sec_header_offset + 0];
                auto head_byte          = fdi_data[sec_header_offset + 1];
                auto sec_byte           = fdi_data[sec_header_offset + 2];
                auto length_byte        = fdi_data[sec_header_offset + 3];
                auto flags              = fdi_data[sec_header_offset + 4];
                int sec_data_offset     = (track_data_offset + (fdi_data[sec_header_offset + 6] << 8) + fdi_data[sec_header_offset + 5]);
                int sec_length          = 128 << length_byte;
                auto sec                = new SectorData(cyl_byte, head_byte, sec_byte, length_byte, (flags & 0x80) != 0);
                for(int sec_data_index = 0; sec_data_index < sec_length; sec_data_index++) {
                    sec->set_data_byte(sec_data_index, fdi_data[sec_data_offset]);
                    sec_data_offset++;
                }
                cyl[head_index].add_sector(sec);
            }
            // передвигаем смещение на следующий заголовок дорожки
            track_header_offset += (7 + (7 * sec_count));
        }
    }
    return image;
}

#define get_next_track(track) \
    if(cur_cyl_index < 0 || cur_head == 1) { cur_cyl = image->add_cyl(); cur_cyl_index++; cur_head = 0; } else cur_head = 1; \
    track = &cur_cyl[cur_head]; \
    for(int sec_byte = 1; sec_byte <= 16; sec_byte++) track->add_sector(new SectorData(cur_cyl_index, cur_head, sec_byte, SEC_LENGTH_0x0100, false));

zxDiskImage* zxDiskImage::openSCL(const char *path) {
    // UNDONE: не проверяется контрольная сумма файла
    size_t length;
    auto scl_data = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length);
    if(!scl_data || length < 9 || scl_data[0] != 'S' || scl_data[1] != 'I' || scl_data[2] != 'N' ||
       scl_data[3] != 'C' || scl_data[4] != 'L' || scl_data[5] != 'A' || scl_data[6] != 'I' || scl_data[7] != 'R' ) {
        LOG_DEBUG("Wrong SCL data", 0);
        return nullptr;
    }
    auto image = new zxDiskImage(2, 256, false, "");
    TrackData* cur_cyl = nullptr;
    TrackData* catalog_track = nullptr;
    TrackData* data_track = nullptr;
    int cur_cyl_index = -1;
    int cur_head = 0;

    get_next_track(catalog_track);
    uint8_t catalog_sec_index = 0;
    int catalog_sec_data_index = 0;
    get_next_track(data_track);
    auto data_track_index = (uint8_t)(cur_cyl_index * 2 + cur_head);
    uint8_t data_sec_index = 0;
    // количество файлов
    auto file_count = scl_data[8];
    int file_header_offset = 9;
    int file_data_offset = (file_header_offset + ( file_count * 14));
    for(int file_index = 0; file_index < file_count; file_index++) {
        // получаем длину файла в секторах
        int file_sec_count = scl_data[file_header_offset + 13];
        // копируем заголовок в каталог
        auto catalog_sec = catalog_track->get_sector(catalog_sec_index);
        for(int i = 0; i < 14; i++) {
            catalog_sec->set_data_byte(catalog_sec_data_index, scl_data[ file_header_offset ]);
            catalog_sec_data_index++;
            file_header_offset++;
        }
        // кроме того, добавляем в каталог номер дорожки и номер сектора, где будет храниться файл
        catalog_sec->set_data_byte(catalog_sec_data_index, data_sec_index);
        catalog_sec_data_index++;
        catalog_sec->set_data_byte(catalog_sec_data_index, data_track_index);
        catalog_sec_data_index++;
        // проверяем, не нужно ли переключиться на следующий сектор в каталоге
        if(catalog_sec_data_index >= 0x0100) {
            catalog_sec_index++;
            catalog_sec_data_index = 0;
            // TR-DOS каталог занимает максимум 8 секторов (в 9-м идет служебная информация)
            if(catalog_sec_index >= 8) {
                LOG_DEBUG("Too many files %s", path);
                delete image;
                return nullptr;
            }
        }
        // копируем данные файла
        for(int file_sec_index = 0; file_sec_index < file_sec_count; file_sec_index++ ) {
            auto data_sec = data_track->get_sector(data_sec_index);
            for(int i = 0; i < 256; i++ ) {
                data_sec->set_data_byte(i, scl_data[ file_data_offset ]);
                file_data_offset++;
            }
            // переключаемся на следующий сектор / дорожку
            data_sec_index++;
            if(data_sec_index == 16) {
                get_next_track(data_track);
                data_track_index = (uint8_t)(cur_cyl_index * 2 + cur_head);
                data_sec_index = 0;
            }
        }
    }
    // заполняем служебную информацию
    auto first_free_track = data_track_index;
    auto first_free_sec = data_sec_index;
    int free_sec_count = (160 - first_free_track) * 16 - data_sec_index;
    auto serv_sector = catalog_track->get_sector(8);
    serv_sector->set_data_byte(0x00, 0x00);                             // признак конца каталога
    serv_sector->set_data_byte(0xe1, first_free_sec);                   // первый свободный сектор
    serv_sector->set_data_byte(0xe2, first_free_track);                 // первая свободная дорожка
    serv_sector->set_data_byte(0xe3, 0x16);                             // дискета с 80-ю цилиндрами и двумя сторонами
    serv_sector->set_data_byte(0xe4, file_count);                       // количество всех (вместе с удаленными) файлов
    serv_sector->set_data_byte(0xe5, (uint8_t)(free_sec_count & 0xff)); // количество свободных секторов - младший байт
    serv_sector->set_data_byte(0xe6, (uint8_t)(free_sec_count >> 8));   // --//-- - старший байт
    serv_sector->set_data_byte(0xe7, 0x10);                             // код TR-DOS'а
    serv_sector->set_data_byte(0xf4, 0x00);                             // количество удаленных файлов
    // имя диска
    for(int i = 0xf5; i <= 0xff; i++) serv_sector->set_data_byte(i, 0x20);
    // дополняем диск до 160 дорожек
    while(data_track_index < 159) {
        get_next_track(data_track);
        data_track_index = (uint8_t)(cur_cyl_index * 2 + cur_head);
    }
    return image;
}

/*
DiskImage.createCustomImage = function ( cyl_count, head_count, trdos_format ) {
    var image = new DiskImage(head_count, false, '', '');
    for ( var cyl_index = 0; cyl_index < cyl_count; cyl_index++ ) {
        image.add_cyl();
    }

    if ( trdos_format ) {
        for ( var cyl_index = 0; cyl_index < cyl_count; cyl_index++ ) {
            for ( var head_index = 0; head_index < head_count; head_index++ ) {

                var track = image.get_track(cyl_index, head_index);
                for ( var sec_byte = 1; sec_byte <= 16; sec_byte++ ) {
                    var sector = new SectorData(cyl_index, head_index, sec_byte, SectorData.SEC_LENGTH_0x0100, false);
                    track.add_sector(sector);
                }
            }
        }

        var cat_track = image.get_track(0, 0);
        if ( cat_track ) {
            var serv_sector = cat_track.get_sector(8);

            serv_sector.set_data_byte(0x00, 0x00); // признак конца каталога
            serv_sector.set_data_byte(0xe1, 0); // первый свободный сектор
            serv_sector.set_data_byte(0xe2, 1); // первая свободная дорожка

            if ( cyl_count < 80 ) {
                serv_sector.set_data_byte(0xe3, head_count == 2 ? 0x17 : 0x19); // дискета с 40 цилиндрами
            }
            else {
                serv_sector.set_data_byte(0xe3, head_count == 2 ? 0x16 : 0x18); // дискета с 80 цилиндрами
            }

            serv_sector.set_data_byte(0xe4, 0); // количество всех (вместе с удаленными) файлов

            var free_sec_count = ( cyl_count * head_count - 1 ) * 16; // одна дорожка уходит под каталог
            serv_sector.set_data_byte(0xe5, free_sec_count & 0xff ); // количество свободных секторов - младший байт
            serv_sector.set_data_byte(0xe6, free_sec_count >> 8 ); // --//-- - старший байт

            serv_sector.set_data_byte(0xe7, 0x10); // код TR-DOS'а
            serv_sector.set_data_byte(0xf4, 0); // количество удаленных файлов
            for ( var i = 0xf5; i <= 0xff; i++ ) {
                serv_sector.set_data_byte(i, 0x20); // имя диска
            }
        }
    }

    return image;
}
*/

bool zxDiskImage::getCompatibleFormats(zxDiskImage* image, int fmt) {

    if(fmt == ZX_CMD_IO_FDI) return true;

    auto trdos_compatible = true;
    auto cyl_count = image->get_cyl_count();
    auto head_count = image->get_head_count();
    for(int cyl_index = 0; trdos_compatible && cyl_index < cyl_count; cyl_index++) {
        for(int head_index = 0; trdos_compatible && head_index < head_count; head_index++) {
            auto track      = image->get_track(cyl_index, head_index);
            auto sec_count  = track->get_sec_count();
            if(sec_count == 16) {
                for(int sec_index = 0; trdos_compatible && sec_index < 16; sec_index++) {
                    auto sector = track->get_sector(sec_index);
                    if(sector->get_length_byte() != SEC_LENGTH_0x0100) trdos_compatible = false;
                }
            } else trdos_compatible = false;
        }
    }
    if(trdos_compatible) {
        if(fmt == ZX_CMD_IO_SCL) return true;
        if(cyl_count == 40 || cyl_count == 80) {
            return fmt == ZX_CMD_IO_TRD;
        }
    }
    return false;
}

uint8_t* zxDiskImage::saveToTRD(zxDiskImage* image, uint32_t* length) {
    if(!zxDiskImage::getCompatibleFormats(image, ZX_CMD_IO_TRD)) {
        return nullptr;
    }
    auto trd_data = TMP_BUF; auto trd = trd_data;

    auto cyl_count = image->get_cyl_count();
    auto head_count = image->get_head_count();
    for(int cyl_index = 0; cyl_index < cyl_count; cyl_index++) {
        for(int head_index = 0; head_index < head_count; head_index++) {
            auto track = image->get_track(cyl_index, head_index);
            for(int sec_index = 0; sec_index < 16; sec_index++) {
                auto sector = track->get_sector(sec_index);
                for(int i = 0; i < 0x0100; i++ ) trd_data[i] = sector->get_data_byte(i);
            }
        }
    }
    *length = trd - trd_data;
    return trd_data;
}

uint8_t* zxDiskImage::saveToFDI(zxDiskImage* image, uint32_t* length) {
    if(!zxDiskImage::getCompatibleFormats(image, ZX_CMD_IO_TRD)) {
        return nullptr;
    }

    auto fdi_data = TMP_BUF;

    auto cyl_count = image->get_cyl_count();
    auto head_count = image->get_head_count();
    // записиываем FDI-заголовок
    fdi_data[0]  = 'F'; fdi_data[1]  = 'D'; fdi_data[2]  = 'I';
    fdi_data[3]  = (uint8_t)(image->is_write_protected() ? 1 : 0);
    fdi_data[4]  = (uint8_t)((cyl_count  >> 0) & 0xff);
    fdi_data[5]  = (uint8_t)((cyl_count  >> 8) & 0xff);
    fdi_data[6]  = (uint8_t)((head_count >> 0) & 0xff);
    fdi_data[7]  = (uint8_t)((head_count >> 8) & 0xff);
    fdi_data[8]  = 0; // смещение описания (L)
    fdi_data[9]  = 0; // смещение описания (H)
    fdi_data[10] = 0; // смещение данных (L)
    fdi_data[11] = 0; // смещение данных (H)
    fdi_data[12] = 0; // длина расширения заголовка (L)
    fdi_data[13] = 0; // длина расширения заголовка (H)
    auto fdi = &fdi_data[14];
    // записываем заголовки секторов
    int track_data_offset = 0x00000000;
    for(int cyl_index = 0; cyl_index < cyl_count; cyl_index++ ) {
        for(int head_index = 0; head_index < head_count; head_index++ ) {
            *fdi++ = (uint8_t)((track_data_offset >>  0) & 0xff);
            *fdi++ = (uint8_t)((track_data_offset >>  8) & 0xff);
            *fdi++ = (uint8_t)((track_data_offset >> 16) & 0xff);
            *fdi++ = (uint8_t)((track_data_offset >> 24) & 0xff);
            *fdi++ = 0; *fdi++ = 0;
            auto track  = image->get_track(cyl_index, head_index);
            auto sec_count = (uint8_t)track->get_sec_count();
            *fdi++ = sec_count;
            int sector_data_offset = 0x0000;
            for(int sec_index = 0; sec_index < sec_count; sec_index++ ) {
                auto sector = track->get_sector(sec_index);
                *fdi++ = (uint8_t)sector->get_cyl_byte();
                *fdi++ = sector->get_head_byte();
                *fdi++ = sector->get_sec_byte();
                *fdi++ = sector->get_length_byte();
                *fdi++ = sector->get_flags();
                *fdi++ = (uint8_t)((sector_data_offset >> 0) & 0xff);
                *fdi++ = (uint8_t)((sector_data_offset >> 8) & 0xff);
                auto sector_length = (0x0080 << sector->get_length_byte());
                sector_data_offset += sector_length;
            }
            track_data_offset += sector_data_offset;
        }
    }
    // сохраняем смещение описания
    fdi_data[8] = (uint8_t)(((fdi - fdi_data) >> 0) & 0xff);
    fdi_data[9] = (uint8_t)(((fdi - fdi_data) >> 8) & 0xff);
    for(int i = 0 ; i < image->description.length(); i++) *fdi++ = (uint8_t)image->description[i];
    *fdi++ = '\0';
    // сохраняем смещение данных
    fdi_data[10] = (uint8_t)(((fdi - fdi_data) >> 0) & 0xff);
    fdi_data[11] = (uint8_t)(((fdi - fdi_data) >> 8) & 0xff);
    // записываем данные секторов
    for(int cyl_index = 0; cyl_index < cyl_count; cyl_index++) {
        for(int head_index = 0; head_index < head_count; head_index++) {
            auto track = image->get_track(cyl_index, head_index);
            auto sec_count = track->get_sec_count();
            for(int sec_index = 0; sec_index < sec_count; sec_index++) {
                auto sector = track->get_sector(sec_index);
                auto sector_length = (0x0080 << sector->get_length_byte());
                for(int i = 0; i < sector_length; i++) *fdi++ = sector->get_data_byte(i);
            }
        }
    }
    *length = fdi - fdi_data;
    return fdi_data;
}

uint8_t* zxDiskImage::saveToSCL(zxDiskImage* image, uint32_t* length) {
    if(!zxDiskImage::getCompatibleFormats(image, ZX_CMD_IO_TRD)) {
        return nullptr;
    }

    auto scl_data = TMP_BUF;
    auto tmp = scl_data;

    // записываем SCL-заголовок
    *tmp++ = 'S'; *tmp++ = 'I'; *tmp++ = 'N'; *tmp++ = 'C'; *tmp++ = 'L'; *tmp++ = 'A'; *tmp++ = 'I'; *tmp++ = 'R';

    auto cat_track = image->get_track(0, 0);
    if(cat_track) {
        // считаем количество неудаленных файлов
        uint8_t file_count = 0;
        int sec_index = 0;
        int sec_offset = 0;
        while(sec_index < 8) {
            auto sector = cat_track->get_sector(sec_index);
            // проверяем первый байт имени файла
            auto filename_byte = sector->get_data_byte(sec_offset);
            if(filename_byte == 0x00) break; // конец каталога
            if(filename_byte != 0x01) file_count++; // если файл не удален, то учитываем его
            // переходим к следующему файлу
            sec_offset += 16;
            if(sec_offset == 256) { sec_index++; sec_offset = 0; }
        }
        // записываем количество файлов
        *tmp++ = file_count;
        // сохраняем по 14 байт из заголовка для всех неудаленных файлов
        sec_index = 0;
        sec_offset = 0;
        while(sec_index < 8) {
            auto sector = cat_track->get_sector(sec_index);
            // проверяем первый байт имени файла
            auto filename_byte = sector->get_data_byte(sec_offset);
            if(filename_byte == 0x00) break; // конец каталога
            if(filename_byte != 0x01) {
                // если файл не удален, то копируем 14 байт его заголовка
                for(int i = 0; i < 14; i++) *tmp++ = sector->get_data_byte(sec_offset + i);
            }
            // переходим к следующему файлу
            sec_offset += 16;
            if(sec_offset == 256) { sec_index++; sec_offset = 0; }
        }
        // сохраняем данные всех неудаленных файлов
        sec_index = 0;
        sec_offset = 0;
        while(sec_index < 8) {
            auto sector = cat_track->get_sector(sec_index);
            // проверяем первый байт имени файла
            auto filename_byte = sector->get_data_byte(sec_offset);
            if(filename_byte == 0x00) break; // конец каталога
            if(filename_byte != 0x01) {
                // если файл не удален, то копируем его данные
                auto data_sec_count = sector->get_data_byte(sec_offset + 13);
                auto data_sec_index = sector->get_data_byte(sec_offset + 14);
                auto data_trk_index = sector->get_data_byte(sec_offset + 15);
                while(data_sec_count) {
                    auto data_cyl_index = (int)floorf(data_trk_index / image->get_head_count());
                    auto data_head_index = data_trk_index - ( data_cyl_index * image->get_head_count());
                    auto data_track = image->get_track(data_cyl_index, data_head_index);
                    auto data_sector = data_track->get_sector(data_sec_index);
                    for(int i = 0; i < 0x0100; i++) *tmp++ = data_sector->get_data_byte(i);
                    data_sec_index++;
                    if(data_sec_index == 16) { data_trk_index++; data_sec_index = 0; }
                    data_sec_count--;
                }
            }
            // переходим к следующему файлу
            sec_offset += 16;
            if(sec_offset == 256) { sec_index++; sec_offset = 0; }
        }
    } else {
        // записываем количество файлов
        *tmp++ = 0;
    }
    // считаем и записываем контрольную сумму
    int sum_hi_word = 0x0000;
    int sum_lo_word = 0x0000;
    auto len = tmp - scl_data;
    for(int i = 0; i < len; i++ ) {
        sum_lo_word += scl_data[i];
        if(sum_lo_word > 0xffff) {
            sum_lo_word &= 0xffff;
            sum_hi_word = (sum_hi_word + 1) & 0xffff;
        }
    }
    *tmp++ = (uint8_t)((sum_lo_word >> 0) & 0xff);
    *tmp++ = (uint8_t)((sum_lo_word >> 8) & 0xff);
    *tmp++ = (uint8_t)((sum_hi_word >> 0) & 0xff);
    *tmp   = (uint8_t)((sum_hi_word >> 8) & 0xff);
    *length = tmp - scl_data;
    return scl_data;
}

void zxBetaDisk::reset() {

}

bool zxBetaDisk::open(int active, const char *path, int type) {
    zxDiskImage* disk(nullptr);
    switch(type) {
        case ZX_CMD_IO_TRD:
            disk = zxDiskImage::openTRD(path);
            break;
        case ZX_CMD_IO_SCL:
            disk = zxDiskImage::openSCL(path);
            break;
        case ZX_CMD_IO_FDI:
            disk = zxDiskImage::openFDI(path);
            break;
    }
    if(disk) insert(active, disk);
    return disk != nullptr;
}

bool zxBetaDisk::save(uint8_t active, const char *path, int type) {
    uint8_t* data(nullptr);
    uint32_t length(0);
    bool result = false;
    auto disk = disks[active].image;

    switch(type) {
        case ZX_CMD_IO_TRD:
            data = zxDiskImage::saveToTRD(disk, &length);
            break;
        case ZX_CMD_IO_SCL:
            data = zxDiskImage::saveToSCL(disk, &length);
            break;
        case ZX_CMD_IO_FDI:
            data = zxDiskImage::saveToFDI(disk, &length);
            break;
    }
    if(data) {
        result = zxFile::writeFile(path, data, length, true);
        delete data;
    }
    return result;
}

void zxBetaDisk::vg93_write(uint8_t address, uint8_t data) {
    switch(address) {
        case 0x1f: process_command(data); break;
        case 0x3f: r_track = data; break;
        case 0x5f: r_sector = data; break;
        case 0x7f: r_data = data; if(write_state) write_next_byte(); break;
        case 0xff:
            // 0,1 - номер диска
            // 2 - апаратный сброс
            // 3 - блокировка согнала HLT(должна быть 1)
            // 4 - номер головки 0=первая
            // 6 - 1=MFM, 0=FM
            drive = data & 0x03;
            hlt = to_bit(data & 0x08);
            head = to_bit(!(data & 0x10));
            mfm = to_bool(!(data & 0x40));
            if(!(data & 0x04)) hardware_reset_controller();
            LOG_INFO("drv:%i hlt:%i head:%i mfm:%i reset:%i", drive, hlt, head, mfm, (data & 0x04));
            break;
    }
}

uint8_t zxBetaDisk::vg93_read(uint8_t address) {
    uint8_t result = 0xff;
    switch(address) {
        case 0x1f: result = get_status(); break;
        case 0x3f: result = r_track; break;
        case 0x5f: result = r_sector; break;
        case 0x7f: result = r_data; if(read_state) read_next_byte(); break;
        case 0xff:
            // 6 - drq запрос данных
            // 7 - intrq окончание команды
            if(intrq) address |= 0x80; else address &= 0x7f;
            if(drq) address |= 0x40; else address &= 0xbf;
            result = address;
            LOG_INFO("a:%X r:%i", address, result);
            break;
    }
    return result;
}
