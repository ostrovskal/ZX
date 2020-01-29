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
static bool multiple, action, check, mark;

static int sec_length;
static int index;
static int max_bytes;

static uint8_t byte_counter;
static uint8_t cyl_byte;
static uint8_t head_byte;
static uint8_t sec_byte;
static uint8_t length_byte;
static uint8_t side;
static uint8_t* rw;

void zxBetaDisk::schedule_data(bool read, SectorData* sec, uint8_t* data, size_t length, bool _multiple, uint8_t expected_side, bool check_side, bool action, bool _mark) {
    auto st = new STATE();
    st->data = new uint8_t[length]; st->length = length; st->index = 0; st->action = action;
    sec_data = sec; side = expected_side;
    multiple = _multiple; check = check_side; mark = _mark;
    if(read) {
        delete read_state;
        memcpy(st->data, data, length);
        read_state = st;
    } else {
        delete write_state;
        memset(st->data, 0, length);
        write_state = st;
    }
}

void zxBetaDisk::read_next_byte() {
    auto st = read_state;
    if(st->index < st->length) { r_data = st->data[st->index++]; drq = 1; } else {
        if(multiple) { r_sector++; read_sectors_end(multiple, side, check); }
        else { read_state = nullptr; intrq = 1; drq = 0; set_busy(false); }
    }
}

void zxBetaDisk::write_next_byte() {
    auto st = write_state;
    if(st->action) {
        if(byte_counter == 0) {
            if(!track_data) {
                write_state = nullptr;
                write_fault = 1; drq = 0; intrq = 1; set_busy(false); return;
            }
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
                        sec_data = new SectorData(cyl_byte, head_byte, sec_byte, length_byte, r_data == 0xf8);
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
            else { write_state = nullptr; intrq = 1; drq = 0; set_busy(false); return; }
        }
        drq = 1;
    } else {
        *st->data++ = r_data; st->index++;
        if(st->index < st->length) drq = 1;
        else {
            store_sector_data(sec_data, st->data);
            if(multiple) { r_sector++; write_sectors_end(multiple, side, check, mark); }
            else { write_state = nullptr; drq = 0; intrq = 1; set_busy(false); }
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
SectorData* zxBetaDisk::get_sector_data(uint8_t expected_side, bool check_side) {
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

uint8_t* zxBetaDisk::extract_sector_address(SectorData* sec_data) {
    static uint8_t data[6];

    auto crc = CRC();

    data[0] = sec_data->get_cyl_byte();
    data[1] = sec_data->get_head_byte();
    data[2] = sec_data->get_sec_byte();
    data[3] = sec_data->get_length_byte();

    if(mfm) crc.add(0xa1);
    crc.add((uint8_t)(sec_data->is_deleted(-1) ? 0xf8 : 0xfb));// TODO: == -1?
    crc.add(data[0]); crc.add(data[1]); crc.add(data[2]); crc.add(data[3]);

    data[4] = (uint8_t)((crc.value >> 8) & 0xff);
    data[5] = (uint8_t)((crc.value >> 0) & 0xff);

    return data;
}

size_t zxBetaDisk::extract_sector_data(SectorData* sec_data) {
    auto length = (size_t)(128 << sec_data->get_length_byte());
    for(int i = 0; i < length; i++ ) dataSec[i] = sec_data->get_data_byte(i);
    return length;
}

void zxBetaDisk::store_sector_data(SectorData* sec_data, uint8_t* data) {
    auto length = 128 << sec_data->get_length_byte();
    for(int i = 0; i < length; i++) sec_data->set_data_byte(i, data[i]);
}

void zxBetaDisk::hardware_reset_controller() {
    int_on_ready = false;
    int_on_unready = false;
    //int_on_index_pointer = false;
    intrq = 0; drq = 0; busy = 0; hld = 0;
    seek_error = 0; crc_error = 0; dirc = 0;
    r_sector = 1; r_track = 0; r_command = 0x03; r_data = 0;
    read_state = nullptr; write_state = nullptr;
    process_command(0x03);
}

uint8_t zxBetaDisk::get_status() {
    auto rdy = ready();
    auto is_write_protected = rdy && get_current_image()->is_write_protected();
    int status = 0;
    // after Restore, Seek, Step commands and also Interrupt command if it actually did not unterrupt anything.
    if((last_command & 0x80) == 0 || (last_command & 0xf0) == 0xd0) {
        status =(to_bit(!rdy) << 7) | (to_bit(is_write_protected) << 6) |
                ((get_hld() & hlt) << 5) | (seek_error << 4) |
                (crc_error << 3) | (to_bit(get_current_track() == 0) << 2) |
                (to_bit(rdy ? index_pointer() : 0) << 1) | busy;
    }
    // after Read Address command
    if((last_command & 0xf0) == 0xc0) {
        status = (to_bit(!rdy) << 7) | ( rnf_error << 4) | (crc_error << 3) | (lost_data_error << 2) | (drq << 1) | busy;
    }
    // after Read Sector command
    if((last_command & 0xe0) == 0x80) {
        status = (to_bit(!rdy) << 7) | (record_type << 6) | (rnf_error << 4) | (crc_error << 3) | (lost_data_error << 2) | (drq << 1) | busy;
    }
    // after Read Track command
    if((last_command & 0xf0) == 0xe0) {
        status = (to_bit(!rdy) << 7) | (lost_data_error << 2) | (drq << 1) | busy;
    }
    // after Write Sector command
    if((last_command & 0xe0) == 0xa0) {
        status = (to_bit(!rdy) << 7) | (to_bit(is_write_protected) << 6) | (write_fault << 5) |
                (rnf_error << 4) | (crc_error << 3) | (lost_data_error << 2) | (drq << 1) | busy;
    }
    // after Write Track command
    if((last_command & 0xf0 ) == 0xf0) {
        status = (to_bit(!rdy) << 7) | (to_bit(is_write_protected) << 6) | (write_fault << 5) | (lost_data_error << 2) | (drq << 1) | busy;
    }
    intrq = 0;
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
    drive = 0;
    head = 0;
    mfm = 0;
    index_pointer_interval = 200;
    index_pointer_length = 10;
    //track_rw_timeout = index_pointer_interval * 2;
    //rw_timeout = 0.15;
    hld_extratime = index_pointer_interval * 15;
    intrq = 0;
    drq = 0;
    hlt = 0;
    busy = 0;
    hld = 0;
    idle_since = getTimeMillis();
    seek_error = 0;
    crc_error = 0;
    rnf_error = 0;
    lost_data_error = 0;
    write_fault = 0;
    record_type = 0;
    dirc = 1;
    last_index_pointer_time = 0;
    r_sector = 1;
    r_track = 0;
    r_command = 0x03;
    r_data = 0;
    last_command = 0x00;
    addr_sec_index = 0x00;
    read_state = nullptr;
    write_state = nullptr;
    int_on_ready = false;
    int_on_unready = false;
    //int_on_index_pointer = false;
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
    // проверка не команду прерывания
    if((r_command & 0xf0) == 0xd0) {
        // прерывание команды
        if(is_busy()) {
            set_busy(false);
            read_state = nullptr; write_state = nullptr;
        }
        else {
            set_busy(false);
            seek_error = 0; crc_error = 0; last_command = r_command;
        }
        int_on_ready = (r_command & 0x01) != 0;
        int_on_unready = (r_command & 0x02) != 0;
        //int_on_index_pointer = (r_command & 0x04) != 0;
        if(r_command & 0x08) intrq = 1;
        return;
    }
    // другие команды не принимаются, если контроллер занят
    if(is_busy()) return;
    last_command = r_command;
    int_on_ready = false;
    int_on_unready = false;
    //int_on_index_pointer = false;
    if(!(r_command & 0x80)) {
        // обработка первой группы команд: восстановление, поиск, шаг, шаг назад, шаг вперед
        auto f_headload = to_bool(r_command & 0x08);
        auto f_verify = to_bool(r_command & 0x04);
        // мы работаем на максимальной скорости
        // var rate = command & 0x03;
        set_busy(true);
        drq = 0; intrq = 0; seek_error = 0; crc_error = 0;
        hld = to_bit(f_headload);
        auto cmd_restore = !(r_command & 0x70);
        auto cmd_seek = (r_command & 0x70) == 0x10;
        if(cmd_restore) {
            dirc = 0; set_current_track(0); r_track = 0; r_data = 0; addr_sec_index = 0;
            if(f_verify) {
                // проверяем соответствие идентификаторов на диске и в регистре дорожки
                if(ready()) {
                    hld = 1;
                    auto image = get_current_image();
                    auto track_data = image->get_track(get_current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) seek_error = (uint8_t)(sec_data->get_cyl_byte() != r_track); else seek_error = 1;
                } else seek_error = 1;
            }
        }
        else if(cmd_seek) {
            // в регистре данных искомый номер дорожки
            if(r_track != r_data) dirc = to_bit(r_track < r_data);
            auto diff = r_data - r_track;
            auto phys_track = get_current_track();
            phys_track += diff;
            if(phys_track < 0) phys_track = 0;
            if(phys_track > 255) phys_track = 255;
            set_current_track(phys_track);
            r_track += diff;
            if(r_track < 0) r_track = 0;
            if(r_track > 255) r_track = 255;
            if(phys_track == 0) r_track = 0;
            addr_sec_index = 0;
            if(f_verify) {
                // проверяем соответствие идентификаторов на диске и в регистре дорожки
                if(ready()) {
                    hld = 1;
                    auto image = get_current_image();
                    auto track_data = image->get_track(get_current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) seek_error = (uint8_t)(sec_data->get_cyl_byte() != r_track); else seek_error = 1;
                } else seek_error = 1;
            }
        }
        else {
            switch ( r_command & 0x60 ) {
                case 0x40: dirc = 1; break;
                case 0x60: dirc = 0; break;
            }
            auto phys_track = get_current_track();
            phys_track = (dirc ? (phys_track + 1) : (phys_track - 1));
            if(phys_track < 0) phys_track = 0;
            if(phys_track > 255) phys_track = 255;
            auto f_update = to_bool(r_command & 0x10);
            if(f_update) {
                auto trk = (dirc ? r_track + 1 : r_track - 1);
                if(trk < 0) trk = 0;
                if(trk > 255) trk = 255;
                r_track = (uint8_t)trk;
            }
            if(phys_track == 0) r_track = 0;
            addr_sec_index = 0;
            if(f_verify) {
                // проверяем, существует ли данная дорожка
                if(ready()) {
                    hld = 1;
                    auto image = get_current_image();
                    auto track_data = image->get_track(get_current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) seek_error = (uint8_t)(sec_data->get_cyl_byte() != r_track); else seek_error = 1;
                } else seek_error = 1;
            }
        }
        intrq = 1;
        set_busy(false);
        return;
    }
    switch(r_command & 0xe0) {
        case 0x80: read_sectors_begin(); return;
        case 0xa0: write_sectors_begin(); return;
    }
    switch(r_command & 0xf0) {
        case 0xc0: read_address(); return;
        case 0xe0: read_track(); return;
        case 0xf0: write_track(); return;
    }
}

void zxBetaDisk::read_sectors_begin() {
    auto multiple       = to_bool(r_command & 0x10);
    auto expected_side  = to_bit(r_command & 0x08);
    //auto wait_15ms      = to_bool(r_command & 0x04);
    auto check_side     = to_bool(r_command & 0x02);
    //auto data_mark      = to_bool(r_command & 0x01);
    set_busy(true);
    drq = 0; intrq = 0; crc_error = 0; rnf_error = 0; lost_data_error = 0;
    if(!ready()) { intrq = 1; set_busy(false); return; }
    hld = 1;
    read_sectors_end(multiple, expected_side, check_side);
}

void zxBetaDisk::read_sectors_end(bool multiple, uint8_t expected_side, bool check_side) {
    auto sec_data = get_sector_data(expected_side, check_side);
    if(sec_data) {
        auto length = extract_sector_data(sec_data);
        // TODO: rnf_error = 1??
        schedule_data(true, nullptr, dataSec, length, multiple, expected_side, check_side, false, false);
        read_next_byte();
    }
    else {
        read_state = nullptr; rnf_error = 1;
        intrq = 1; drq = 0; set_busy(false);
    }
}

void zxBetaDisk::write_sectors_begin() {
    auto multiple               = to_bool(r_command & 0x10);
    auto expected_side          = to_bit(r_command & 0x08);
    //auto wait_15ms              = to_bool(r_command & 0x04);
    auto check_side             = to_bool(r_command & 0x02);
    auto deleted_address_mark   = to_bool(r_command & 0x01);
    set_busy(true);
    drq = 0; intrq = 0; crc_error = 0;
    rnf_error = 0; lost_data_error = 0; write_fault = 0;
    if(!ready()) { intrq = 1; set_busy(false); return; }
    hld = 1;
    if(get_current_image()->is_write_protected()) { intrq = 1; set_busy(false); return; }
    write_sectors_end(multiple, expected_side, check_side, deleted_address_mark);
}

void zxBetaDisk::write_sectors_end(bool multiple, uint8_t expected_side, bool check_side, bool deleted_address_mark) {
    auto sec_data = get_sector_data(expected_side, check_side);
    if(sec_data) {
        sec_data->is_deleted(deleted_address_mark);
        auto length = (size_t)(128 << sec_data->get_length_byte());
        schedule_data(false, sec_data, nullptr, length, multiple, expected_side, check_side, false, deleted_address_mark);
        drq = 1;
    } else { write_state = nullptr; rnf_error = 1; intrq = 1; drq = 0; set_busy(false); }
}

void zxBetaDisk::read_address() {
    //auto wait_15ms = to_bool(r_command & 0x04);

    set_busy(true);
    drq = 0; intrq = 0; crc_error = 0; rnf_error = 0; lost_data_error = 0;

    if(!ready()) { intrq = 1; set_busy(false); return; }
    hld = 1;
    auto image = get_current_image();
    auto track_data = image->get_track(get_current_track(), head);
    auto sec_data = track_data ? track_data->get_sector(addr_sec_index) : nullptr;
    if(!sec_data) {
        addr_sec_index = 0; // это так, на всякий случай :)
        rnf_error = 1; intrq = 1; drq = 0; read_state = nullptr; set_busy(false);
        return;
    }
    addr_sec_index++;
    if(addr_sec_index == track_data->get_sec_count()) addr_sec_index = 0;
    schedule_data(true, nullptr, extract_sector_address(sec_data), 6, false, 0, false, false, false);
    read_next_byte();
}

void zxBetaDisk::read_track() {
    set_busy(true);
    drq = 0; intrq = 0; crc_error = 0; seek_error = 0; lost_data_error = 0;
    if(!ready()) { intrq = 1; set_busy(false); return; }
    hld = 1;
    auto image = get_current_image();
    auto track_data = image->get_track(get_current_track(), head);
    auto sec_count = track_data->get_sec_count();

    auto data = TMP_BUF;

    // GAP IVa
    ssh_memcpy(&data, &dataGap, build_GAP(4));
    // SYNC
    ssh_memcpy(&data, &dataGap, build_SYNC());
    // Index Mark
    *data++ = 0xfc;
    // GAP I
    ssh_memcpy(&data, &dataGap, build_GAP(1));
    // запись каждого сектора
    for(int sec_index = 0; sec_index < sec_count; sec_index++) {
        auto sec_data = track_data->get_sector(sec_index);
        // SYNC
        ssh_memcpy(&data, &dataGap, build_SYNC());
        // if MFM then 0xa1 * 3 (exact)
        if(mfm) { data = ssh_memset(data, 0xa1, 3); }
        // IDAM
        *data++ = 0xfe;
        // ID
        *data++ = sec_data->get_cyl_byte();
        *data++ = sec_data->get_head_byte();
        *data++ = sec_data->get_sec_byte();
        *data++ = sec_data->get_length_byte();
        // CRC
        auto crc1 = CRC();
        crc1.add(sec_data->get_cyl_byte());
        crc1.add(sec_data->get_head_byte());
        crc1.add(sec_data->get_sec_byte());
        crc1.add(sec_data->get_length_byte());
        *data++ = (uint8_t)((crc1.value >> 8) & 0xff);
        *data++ = (uint8_t)(crc1.value & 0xff);
        // GAP II
        ssh_memcpy(&data, &dataGap, build_GAP(2));
        // SYNC
        ssh_memcpy(&data, &dataGap, build_SYNC());
        // if MFM then 0xa1 * 3 (exact)
        if(mfm) { data = ssh_memset(data, 0xa1, 3); }
        // DAM
        *data++ = 0xfb;
        // Data
        auto crc2 = CRC();
        auto sec_data_length = 128 << sec_data->get_length_byte();
        for(int i = 0; i < sec_data_length; i++) {
            *data++ = sec_data->get_data_byte(i);
            *data++ = sec_data->get_data_byte(i);
        }
        // CRC
        *data++ = (uint8_t)((crc2.value >> 8) & 0xff);
        *data++ = (uint8_t)(crc2.value & 0xff);
        // GAP III
        ssh_memcpy(&data, &dataGap, build_GAP(3));
    }
    // GAP IV b
    ssh_memcpy(&data, &dataGap, build_GAP(4));
    // дополняем до приблизительной типичной длины дорожки
    auto required_length = (data - TMP_BUF) - (mfm ? 6250 : 3125);
    data = ssh_memset(data, mfm ? 0x4e : 0xff, required_length);
    schedule_data(true, nullptr, data, data - TMP_BUF, false, 0, false, false, false);
    read_next_byte();
}

void zxBetaDisk::write_track() {
    set_busy(true);
    drq = 0; intrq = 0; crc_error = 0; seek_error = 0;
    lost_data_error = 0; write_fault = 0;
    if(!ready()) { intrq = 1; set_busy(false); return; }
    hld = 1;
    auto image = get_current_image();
    if(image->is_write_protected()) { intrq = 1; set_busy(false); return; }
    track_data = image->get_track(get_current_track(), head);
    byte_counter = 0; max_bytes = ( mfm ? 6250 : 3125 );
    crc = CRC();
    crc_just_stored = sector_address_zone = sector_data_zone = false;
    index = -1; sec_length = 0;
    cyl_byte = head_byte = sec_byte = length_byte = 0;
    schedule_data(false, nullptr, nullptr, 0, false, 0, false, true, false);
}

void zxBetaDisk::write_sysreg(uint8_t data) {
    drive = data & 0x03;
    hlt = to_bit(data & 0x08 );
    head = to_bit(!( data & 0x10));
    mfm = to_bit(!( data & 0x40));
    if(!(data & 0x04)) hardware_reset_controller();
}

uint8_t zxBetaDisk::read_sysreg(uint8_t data) {
    if(intrq) data |= 0x80; else data &= 0x7f;
    if(drq) data |= 0x40; else data &= 0xbf;
    return data;
}

void zxBetaDisk::vg93_write(uint8_t address, uint8_t data) {
    switch(address) {
        case 0x1f: process_command(data); break;
        case 0x3f: r_track = data; break;
        case 0x5f: r_sector = data; break;
        case 0x7f: r_data = data; if(write_state) write_next_byte(); break;
        case 0xff: write_sysreg(data);
    }
}

uint8_t zxBetaDisk::vg93_read(uint8_t address) {
    uint8_t result = 0xff;
    switch(address) {
        case 0x1f: result = get_status(); break;
        case 0x3f: result = r_track; break;
        case 0x5f: result = r_sector; break;
        case 0x7f: result = r_data; if(read_state) read_next_byte(); break;
        case 0xff: result = read_sysreg(address);
    }
    return result;
}

zxDiskImage* zxDiskImage::openTRD(const char* path) {
    // UNDONE: имя диска из служебного сектора можно использовать как комментарий
    uint32_t length;
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
    auto image = new zxDiskImage(path, head_count, cyl_count, false, "");
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
    uint32_t length;
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
    auto comment            = (char*)TMP_BUF; auto tmp = comment;
    while(comment_offset < length && fdi_data[comment_offset] != 0) *tmp++ = fdi_data[comment_offset++];
    auto image = new zxDiskImage(path, head_count, cyl_count, write_protect, comment);
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
                int sec_length          = (0x0080 << length_byte);
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
    uint32_t length;
    auto scl_data = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length);
    if(!scl_data || length < 9 || scl_data[0] != 'S' || scl_data[1] != 'I' || scl_data[2] != 'N' ||
       scl_data[3] != 'C' || scl_data[4] != 'L' || scl_data[5] != 'A' || scl_data[6] != 'I' || scl_data[7] != 'R' ) {
        LOG_DEBUG("Wrong SCL data", 0);
        return nullptr;
    }
    auto image = new zxDiskImage(path, 2, 256, false, "");// TODO: 256 цилиндров - может больше???
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
    fdi_data[8]  = 0; // смещение текста (L)
    fdi_data[9]  = 0; // смещение текста (H)
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
    // сохраняем смещение текста
    fdi_data[8] = (uint8_t)(((fdi - fdi_data) >> 0) & 0xff);
    fdi_data[9] = (uint8_t)(((fdi - fdi_data) >> 8) & 0xff);
    *fdi++ = '\0';// TODO:string_to_bytes()
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
    uint32_t length;
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
