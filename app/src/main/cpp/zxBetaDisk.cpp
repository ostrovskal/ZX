//
// Created by Sergey on 24.01.2020.
//

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

#include <math.h>
#include "zxCommon.h"
#include "zxBetaDisk.h"

// текущая дорожка
static zxDiskTrack* track_data;
// текущий сектор
static zxDiskSector* sec_data;

size_t zxBetaDisk::buildGAP(int num) {
    static size_t GAP[] = { 0, 0, 0, 16, 0xFF, 0x4E, 11, 0xFF, 0x4E, 12, 0xFF, 0x4E, 16, 0xFF, 0x4E, 6, 0, 0 };
    auto gap    = &GAP[num * 3];
    auto count  = (size_t)gap[0] * (mfm + 1);
    auto fill   = gap[mfm + 1];
    memset(TMP_BUF, fill, count);
    return count;
}

void zxBetaDisk::crc_add(uint8_t v8) {
    crc ^= (v8 << 8);
    for(int i = 0; i < 8; i++) crc = (uint16_t)((crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1));
}

void zxBetaDisk::set_busy(int value) {
    busy = to_bit(value);
    if(value) hld = get_hld(); else cmd_time = currentTimeMillis();
}

bool zxBetaDisk::index_pointer() {
    static u_long time = 0;
    auto is = ((currentTimeMillis() - time) < 10);
    if(int_on_index_pointer) intrq = sINTRQ;
    time = currentTimeMillis();
    return is;
}

uint8_t zxBetaDisk::get_hld() {
    if(busy) return hld;
    return (uint8_t)(hld && ((currentTimeMillis() - cmd_time) < HLD_EXTRA_TIME));
}

void zxBetaDisk::read_next_byte() {
    if(state_index < state_length) {
        uint8_t bt(0);
        switch(lcmd) {
            case VG_RADDRESS:
                switch(state_index) {
                    case 0: bt = sec_data->ntrk; break;
                    case 1: bt = sec_data->nhead; break;
                    case 2: bt = sec_data->nsec; break;
                    case 3: bt = sec_data->slen; break;
                    case 4: bt = crc_hi(); break;
                    case 5: bt = crc_lo(); break;
                }
                if(state_index < 4) crc_add(bt);
                break;
            case VG_RTRACK: break;
            default: bt = sec_data->get(get_file(), state_index);
        }
        opts[TRDOS_DAT] = bt; drq = sDRQ;
        state_index++;
    } else {
        // проверить на потерю данных
        if((currentTimeMillis() - cmd_time) >= lost_timeout) elost = sELOST;
        if(state_mult) { opts[TRDOS_SEC]++; read_sectors(); }
        else { states = ST_NONE; intrq = sINTRQ; drq = 0; set_busy(false); }
    }
}

void zxBetaDisk::write_next_byte() {
    switch(lcmd) {
        case VG_WTRACK: format(); break;
        default: sec_data->put(get_file(), state_index, opts[TRDOS_DAT]); break;
    }
    state_index++;
    if(state_index < state_length) drq = sDRQ;
    else {
        // проверить на потерю данных
        if((currentTimeMillis() - cmd_time) >= lost_timeout) elost = sELOST;
        if(state_mult) { opts[TRDOS_SEC]++; write_sectors(); }
        else { states = ST_NONE; drq = 0; intrq = sINTRQ; set_busy(false); }
    }
}

zxDiskSector* zxBetaDisk::find_sector() {
    if(!ready()) return nullptr;
    auto track_data = image->get_track(current_track(), head);
    zxDiskSector* sec_data = nullptr;
    if(track_data) {
        auto sec_count = track_data->get_sec_count();
        for(int sec_index = 0; sec_index < sec_count; sec_index++) {
            auto _sd = track_data->get_sector(sec_index);
            auto track_match = (_sd->ntrk == opts[TRDOS_TRK]);
            auto head_match = (!state_check || (_sd->nhead == state_head));
            auto sec_match = (_sd->nsec == opts[TRDOS_SEC]);
            if(track_match && head_match && sec_match) { sec_data = _sd; break; }
        }
    }
    return sec_data;
}

zxBetaDisk::zxBetaDisk() {
    drive = head = trec = addr_sec_index = 0; lost_timeout = 0;
    lcmd = VG_NONE;
    cmd_time = currentTimeMillis();
    reset_controller();
}

void zxBetaDisk::reset_controller() {
    int_on_ready = int_on_unready = int_on_index_pointer = false;
    intrq = drq = busy = hld = 0;
    dirc = eseek = esector = ewrite = elost = trec = 0;
    opts[TRDOS_SEC] = 1; opts[TRDOS_TRK] = 0; opts[TRDOS_CMD] = 0x03; opts[TRDOS_DAT] = 0;
    states = ST_NONE;
    process_command(0x03);
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
//  3       |                                     ОШИБКА CRC                                |                   0                   |
// ----------------------------------------------------------------------------------------------------------------------------------
//  2       |    ДОРОЖКА 0     |                                            ПОТЕРЯ  ДАННЫХ                                          |
// ----------------------------------------------------------------------------------------------------------------------------------
//  1       |ИНДЕКСНЫЙ ИМПУЛЬС |                                            ЗАПРОС  ДАННЫХ                                          |
// ----------------------------------------------------------------------------------------------------------------------------------
//  0       |                И   д   е   т           в   ы   п   о   л   н   е   н   и   е       к   о   м   а   н   д   ы          |
// ----------------------------------------------------------------------------------------------------------------------------------
uint8_t zxBetaDisk::get_status() {
    // 0 - занято
    // 7 - готовность дисковода
    auto rdy    = (uint8_t)ready();
    auto pwrite = ((uint8_t)(rdy && image->is_protected()) << 6);
    auto reqd   = drq >> 5;
    int status  = 0;
    switch(lcmd) {
        case VG_INTERRUPT:
        case VG_RESTORE: case VG_SEEK:
        case VG_STEP: case VG_STEP0: case VG_STEP1:
            status = pwrite | ((get_hld() & hlt) << 5) | eseek | ((uint8_t)(current_track() == 0) << 2) | ((uint8_t)(rdy && index_pointer()) << 1);
            break;
        case VG_RADDRESS:   status = esector | elost | reqd; break;
        case VG_RSECTOR:    status = trec | esector | elost | state_mark | reqd; break;
        case VG_RTRACK:     status = elost | reqd; break;
        case VG_WSECTOR:    status = pwrite | ewrite | esector | elost | reqd; break;
        case VG_WTRACK:     status = pwrite | ewrite | elost | reqd; break;
    }
    intrq = 0;
    status |= (((!rdy) << 7) | busy);
    return (uint8_t)status;
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

int zxBetaDisk::is_readonly(int num, int write) {
    auto img = disks[num].image;
    if(img && write != -1) img->write = (uint8_t)write;
    return (img ? img->is_protected() : 0);
}

void zxBetaDisk::process_command(uint8_t cmd) {
    bool rdy;
    opts[TRDOS_CMD] = cmd;
//    LOG_INFO("cmd:%i PC:%i", cmd, zxALU::PC);
    // проверка не команду прерывания
    if((cmd & 0xf0) == 0xd0) {
        // прерывание команды
        if(busy) states = ST_NONE; else { eseek = 0; lcmd = VG_INTERRUPT; }
        set_busy(false);
        int_on_ready        = (cmd & 0x01) != 0;
        int_on_unready      = (cmd & 0x02) != 0;
        int_on_index_pointer= (cmd & 0x04) != 0;
        if(cmd & 0x08) intrq = sINTRQ;
        LOG_INFO("INTERRUPT cmd:%i", cmd & 15);
        return;
    }
    // другие команды не принимаются, если контроллер занят
    if(busy) return;
    int_on_ready = int_on_unready = int_on_index_pointer = false;
    set_busy(true);
    drq = intrq = eseek = esector = ewrite = elost = 0;
    switch(cmd & 0xF0) {
        case 0x00:
            lcmd = VG_RESTORE;
            hld = (uint8_t)((cmd & 0x08) >> 3);
            dirc = opts[TRDOS_TRK] = opts[TRDOS_DAT] = 0;
            addr_sec_index = 0;
            set_current_track(0);
            intrq = sINTRQ;
            set_busy(false);
            break;
        case 0x10:
            lcmd = VG_SEEK;
            hld = to_bit(cmd & 0x08);
            // в регистре данных искомый номер дорожки
            dirc = to_bit(opts[TRDOS_TRK] < opts[TRDOS_DAT]);
            opts[TRDOS_TRK] = opts[TRDOS_DAT];
            set_current_track(opts[TRDOS_TRK]);
            addr_sec_index = 0;
            if(cmd & 0x04) {
                eseek = sESEEK;
                // проверяем соответствие идентификаторов на диске и в регистре дорожки
                if (ready()) {
                    hld = 1;
                    auto track_data = image->get_track(current_track(), head);
                    auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
                    if(sec_data) eseek = (uint8_t)((sec_data->ntrk != opts[TRDOS_TRK]) << 4);
                }
            }
            intrq = sINTRQ;
            set_busy(false);
            break;
        case 0x40: case 0x50:
            step(VG_STEP1);
            break;
        case 0x60: case 0x70:
            step(VG_STEP0);
            break;
        case 0x20: case 0x30:
            step(VG_STEP);
            break;
        case 0x80: case 0x90:
            lcmd = VG_RSECTOR;
            state_mult  = to_bit(opts[TRDOS_CMD] & 0x10);
            state_head  = to_bit(opts[TRDOS_CMD] & 0x08);
            state_check = to_bit(opts[TRDOS_CMD] & 0x02);
            state_mark  = (to_bit(opts[TRDOS_CMD] & 0x01) << 5);
            rdy = ready(); set_busy(rdy);
            if(rdy) { hld = 1; read_sectors(); } else intrq = sINTRQ;
            LOG_INFO("RSECTOR mult:%i expect:%i check:%i mark:%i", state_mult, state_head, state_check, state_mark);
            break;
        case 0xa0: case 0xb0:
            lcmd = VG_WSECTOR;
            state_mult  = to_bit(opts[TRDOS_CMD] & 0x10);
            state_head  = to_bit( opts[TRDOS_CMD] & 0x08);
            state_check = to_bit(opts[TRDOS_CMD] & 0x02);
            state_mark  = to_bit(opts[TRDOS_CMD] & 0x01);
            rdy = ready() && !image->is_protected();
            set_busy(rdy);
            if(rdy) { hld = 1; write_sectors(); } else intrq = sINTRQ;
            LOG_INFO("WSECTOR mult:%i expect:%i check:%i mark:%i", state_mult, state_head, state_check, state_mark);
            break;
        case 0xc0:
            lcmd = VG_RADDRESS;
            state_mult = 0; states = ST_NONE;
            set_busy(false);
            if(ready()) {
                track_data = image->get_track(current_track(), head);
                sec_data = track_data ? track_data->get_sector(addr_sec_index) : nullptr;
                if(sec_data) {
                    addr_sec_index++;
                    if(addr_sec_index >= track_data->get_sec_count()) addr_sec_index = 0;
                    hld = 1; set_busy(true); crc_init();
                    if(mfm) crc_add(0xA1);
                    set_states(ST_READ, 6); read_next_byte();
                } else {
                    addr_sec_index = 0;
                    esector = sESECTOR; intrq = sINTRQ; drq = 0;
                }
            } else intrq = sINTRQ;
            break;
        case 0xe0:
            lcmd = VG_RTRACK;
            read_track();
            break;
        case 0xf0: {
            lcmd = VG_WTRACK;
            rdy = ready() && image->is_protected();
            set_busy(rdy);
            intrq = sINTRQ; states = ST_NONE;
            LOG_INFO("FORMAT", 0);
            break;
        }
    }
}

void zxBetaDisk::step(int cmd) {
    lcmd = (uint8_t)cmd;
    if(cmd == VG_STEP1) dirc = 1;
    else if(cmd == VG_STEP0) dirc = 0;
    auto phys_track = current_track();
    phys_track = (dirc ? (phys_track + 1) : (phys_track - 1));
    if(phys_track < 0) phys_track = 0;
    if(phys_track > 255) phys_track = 255;
    opts[TRDOS_TRK] = (uint8_t)phys_track;
    hld = to_bit(opts[TRDOS_CMD] & 0x8);
    if((opts[TRDOS_CMD] & 0x10)) set_current_track(opts[TRDOS_TRK]);
    addr_sec_index = 0;
    if(opts[TRDOS_CMD] & 0x04) {
        eseek = sESEEK;
        // проверяем соответствие идентификаторов на диске и в регистре дорожки
        if(ready()) {
            hld = 1;
            auto track_data = image->get_track(current_track(), head);
            auto sec_data = track_data ? track_data->get_sector(0) : nullptr;
            if(sec_data) eseek = (uint8_t) ((sec_data->ntrk != opts[TRDOS_TRK]) << 4);
        }
    }
    intrq = sINTRQ;
    set_busy(false);
}

void zxBetaDisk::read_sectors() {
    sec_data = find_sector();
    if(sec_data) {
        image->set_fpos(sec_data->fpos);
        trec = (uint8_t)(sec_data->is_deleted() << 6);
        set_states(ST_READ, sec_data->length());
        lost_timeout = (uint16_t)(sec_data->length() * LOST_TIME);
        read_next_byte();
    } else { states = ST_NONE; esector = sESECTOR; intrq = sINTRQ; drq = 0; set_busy(false); }
}

void zxBetaDisk::write_sectors() {
    sec_data = find_sector();
    if(sec_data) {
        image->set_fpos(sec_data->fpos);
        sec_data->is_deleted(state_mark);
        set_states(ST_WRITE, sec_data->length());
        lost_timeout = (uint16_t)(sec_data->length() * LOST_TIME);
        drq = sDRQ;
    } else { states = ST_NONE; esector = sESECTOR; intrq = sINTRQ; drq = 0; set_busy(false); }
}

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
void zxBetaDisk::read_track() {
    state_mult = 0;
    //auto rdy = ready();
    set_busy(false);
    states = ST_NONE;
    intrq = sINTRQ;
/*
    hld = 1;
    auto track_data = image->get_track(current_track(), head);
    auto sec_count = track_data->get_sec_count();
    auto rw = &TMP_BUF[ZX_BETADISK_INDEX]; auto tmp = rw;
    ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_IV));
    ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_SYNC));
    // Index Mark
    *tmp++ = 0xFC;
    // GAP I
    ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_I));
    // запись каждого сектора
    for(int sec_index = 0; sec_index < sec_count; sec_index++) {
        sec_data = track_data->get_sector(sec_index);
        ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_SYNC));
        if(mfm) { *tmp++ = 0xA1; *tmp++ = 0xA1; *tmp++ = 0xA1; }
        *tmp++ = 0xFE; // IDAM
        // ID
        *tmp++ = sec_data->ntrk; *tmp++ = sec_data->nhead; *tmp++ = sec_data->nsec; *tmp++ = sec_data->slen;
        // CRC
        crc_init();
        crc_add(sec_data->ntrk); crc_add(sec_data->nhead); crc_add(sec_data->nsec); crc_add(sec_data->slen);
        *tmp++ = crc_hi(); *tmp++ = crc_lo();
        ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_II));
        ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_SYNC));
        if(mfm) { *tmp++ = 0xA1; *tmp++ = 0xA1; *tmp++ = 0xA1; }
        *tmp++ = 0xFB; // DAM
        // Data
        crc_init();
        auto len = sec_data->length();
        for(int i = 0; i < len; i++) {
            *tmp++ = sec_data->get(i);
            crc_add(sec_data->get(i));
        }
        // CRC
        *tmp++ = crc_hi(); *tmp++ = crc_lo();
        ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_III));
    }
    // GAP IV b
    ssh_memcpy(&tmp, TMP_BUF, buildGAP(GAP_IV));
    // дополняем до приблизительной типичной длины дорожки
    auto len = (mfm ? 6250 : 3125);
    set_states(ST_READ, len);
    auto required_length = (tmp - rw) - len;
    tmp = ssh_memset(tmp, mfm ? 0x4E : 0xFF, required_length);
    read_next_byte();
*/
}

zxDiskImage::zxDiskImage(int _head, int _trk, bool _write, const char* _desc, const char* path) : file(nullptr) {
    desc = _desc; head = _head; trk = _trk; write = _write;
    tracks = new zxDiskTrack*[trk]; ntrk = 0;
    if(path) {
        file = new zxFile();
        file->open((FOLDER_FILES + path).c_str(), _write ? zxFile::open_read : zxFile::open_read_write);
    }
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
    auto image = new zxDiskImage(head_count, cyl_count, false, "", path);
    size_t data_index = 0;
    for(int cyl_index = 0; cyl_index < cyl_count; cyl_index++) {
        auto cyl = image->add_cyl();
        for(int head_index = 0; head_index < head_count; head_index++) {
            for(int sec_index = 0; sec_index < 16; sec_index++) {
                cyl[head_index].add_sector(new zxDiskSector(data_index, (uint8_t)cyl_index, (uint8_t)head_index,
                        (uint8_t)(sec_index + 1), SEC_LENGTH_0x0100, false));
                data_index += 256;
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
    auto image = new zxDiskImage(head_count, cyl_count, write_protect, comment, path);
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
                auto sec_data_offset    = (size_t)(track_data_offset + (fdi_data[sec_header_offset + 6] << 8) + fdi_data[sec_header_offset + 5]);
                cyl[head_index].add_sector(new zxDiskSector(sec_data_offset, cyl_byte, head_byte,
                        sec_byte, length_byte, (flags & 0x80) != 0));
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
    for(int sec_byte = 1; sec_byte <= 16; sec_byte++) track->add_sector(new zxDiskSector(-1, cur_cyl_index, cur_head, sec_byte, SEC_LENGTH_0x0100, false));

zxDiskImage* zxDiskImage::openSCL(const char *path) {
    // UNDONE: не проверяется контрольная сумма файла
    size_t length;
    auto scl_data = (uint8_t*)zxFile::readFile(path, TMP_BUF, true, &length);
    if(!scl_data || length < 9 || scl_data[0] != 'S' || scl_data[1] != 'I' || scl_data[2] != 'N' ||
       scl_data[3] != 'C' || scl_data[4] != 'L' || scl_data[5] != 'A' || scl_data[6] != 'I' || scl_data[7] != 'R' ) {
        LOG_DEBUG("Wrong SCL data", 0);
        return nullptr;
    }
    auto image = new zxDiskImage(2, 256, false, "", nullptr);
    zxDiskTrack* cur_cyl = nullptr;
    zxDiskTrack* catalog_track = nullptr;
    zxDiskTrack* data_track = nullptr;
    int cur_cyl_index = -1;
    int cur_head = 0;

    get_next_track(catalog_track);
    uint8_t catalog_sec_index = 0;
    size_t catalog_sec_data_index = 0;
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
        catalog_sec->fpos = catalog_sec_data_index;
        for(int i = 0; i < 14; i++) {
            catalog_sec->put(catalog_sec_data_index, scl_data[file_header_offset++]);
            catalog_sec_data_index++;
        }
        // кроме того, добавляем в каталог номер дорожки и номер сектора, где будет храниться файл
        catalog_sec->put(catalog_sec_data_index, data_sec_index);
        catalog_sec_data_index++;
        catalog_sec->put(catalog_sec_data_index, data_track_index);
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
            data_sec->fpos = (size_t)file_data_offset;
            for(int i = 0; i < 256; i++ ) {
                data_sec->put(i, scl_data[file_data_offset + i]);
            }
            file_data_offset += 256;
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
    serv_sector->put(0x00, 0x00);                             // признак конца каталога
    serv_sector->put(0xe1, first_free_sec);                   // первый свободный сектор
    serv_sector->put(0xe2, first_free_track);                 // первая свободная дорожка
    serv_sector->put(0xe3, 0x16);                             // дискета с 80-ю цилиндрами и двумя сторонами
    serv_sector->put(0xe4, file_count);                       // количество всех (вместе с удаленными) файлов
    serv_sector->put(0xe5, (uint8_t)(free_sec_count & 0xff)); // количество свободных секторов - младший байт
    serv_sector->put(0xe6, (uint8_t)(free_sec_count >> 8));   // --//-- - старший байт
    serv_sector->put(0xe7, 0x10);                             // код TR-DOS'а
    serv_sector->put(0xf4, 0x00);                             // количество удаленных файлов
    // имя диска
    for(int i = 0xf5; i <= 0xff; i++) serv_sector->put(i, 0x20);
    // дополняем диск до 160 дорожек
    while(data_track_index < 159) {
        get_next_track(data_track);
        data_track_index = (uint8_t)(cur_cyl_index * 2 + cur_head);
    }
    return image;
}

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
                    if(sector->slen != SEC_LENGTH_0x0100) trdos_compatible = false;
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
                image->set_fpos(sector->fpos);
                for(int i = 0; i < 0x0100; i++ ) trd_data[i] = sector->get(image->file, i);
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
    fdi_data[3]  = (uint8_t)(image->is_protected() ? 1 : 0);
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
                *fdi++ = sector->ntrk; *fdi++ = sector->nhead; *fdi++ = sector->nsec; *fdi++ = sector->slen; *fdi++ = sector->flags;
                *fdi++ = (uint8_t)((sector_data_offset >> 0) & 0xff);
                *fdi++ = (uint8_t)((sector_data_offset >> 8) & 0xff);
                sector_data_offset += sector->length();
            }
            track_data_offset += sector_data_offset;
        }
    }
    // сохраняем смещение описания
    fdi_data[8] = (uint8_t)(((fdi - fdi_data) >> 0) & 0xff);
    fdi_data[9] = (uint8_t)(((fdi - fdi_data) >> 8) & 0xff);
    for(int i = 0 ; i < image->desc.length(); i++) *fdi++ = (uint8_t)image->desc[i];
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
                image->set_fpos(sector->fpos);
                for(int i = 0; i < sector->length(); i++) *fdi++ = sector->get(image->file, i);
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
            auto filename_byte = sector->get(image->file, sec_offset);
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
            auto filename_byte = sector->get(image->file, sec_offset);
            if(filename_byte == 0x00) break; // конец каталога
            if(filename_byte != 0x01) {
                // если файл не удален, то копируем 14 байт его заголовка
                for(int i = 0; i < 14; i++) *tmp++ = sector->get(image->file, sec_offset + i);
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
            auto filename_byte = sector->get(image->file, sec_offset);
            if(filename_byte == 0x00) break; // конец каталога
            if(filename_byte != 0x01) {
                // если файл не удален, то копируем его данные
                auto data_sec_count = sector->get(image->file, sec_offset + 13);
                auto data_sec_index = sector->get(image->file, sec_offset + 14);
                auto data_trk_index = sector->get(image->file, sec_offset + 15);
                while(data_sec_count) {
                    auto data_cyl_index = (int)floorf(data_trk_index / image->get_head_count());
                    auto data_head_index = data_trk_index - ( data_cyl_index * image->get_head_count());
                    auto data_track = image->get_track(data_cyl_index, data_head_index);
                    auto data_sector = data_track->get_sector(data_sec_index);
                    for(int i = 0; i < 0x0100; i++) *tmp++ = data_sector->get(image->file, i);
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

bool zxBetaDisk::unpackDiskSectors(zxDiskImage* img, uint8_t* ptr) {
    for(int trk = 0 ; trk < img->ntrk; trk++) {
        for(int head = 0; head < img->head; head++) {
            auto track = img->get_track(trk, head);
            for(int sec = 0 ; sec < track->get_sec_count(); sec++) {
                auto sector = track->get_sector(sec);
                auto l = sector->length();
                auto sz = *(uint16_t*)ptr; ptr += 2;
                // распаковка
                if(!unpackBlock(ptr, TMP_BUF, &TMP_BUF[sz], l, true)) {
                    LOG_INFO("Ошибка при распаковке сектора <t:%i h:%i s:%i>!", trk, head, sec);
                    return false;
                }
                ptr += sz;
                for(int j = 0; j < l; j++) sector->put(j, TMP_BUF[j]);
            }
        }
    }
    return true;
}

uint8_t *zxBetaDisk::loadState(uint8_t *ptr) {
    // сигнатура
    if(ptr[0] != 'S' || ptr[1] != 'E' || ptr[2] != 'R' || ptr[3] != 'G') {
        LOG_INFO("Сигнатура бетадиска некорректна!", 0);
        return ptr;
    }
    ptr += 4;
    // 1. переменные
    // uint8_t
    mfm         = *ptr++; dirc          = *ptr++; busy          = *ptr++; hld   = *ptr++;
    head        = *ptr++; hlt           = *ptr++; intrq         = *ptr++; drq   = *ptr++;
    drive       = *ptr++; eseek         = *ptr++; esector       = *ptr++; ewrite= *ptr++;
    elost       = *ptr++; trec          = *ptr++; lcmd          = *ptr++;
    fmt_trk     = *ptr++; fmt_head      = *ptr++; fmt_sec       = *ptr++; 
    states      = *ptr++; state_mult    = *ptr++; state_check   = *ptr++; 
    state_head  = *ptr++; state_mark    = *ptr++; addr_sec_index= *ptr++;
    // bool
    int_on_ready        = (bool)*ptr++; int_on_unready = (bool)*ptr++;
    int_on_index_pointer= (bool)*ptr++; fmt_crc        = (bool)*ptr++;
    fmt_sec_addr        = (bool)*ptr++; fmt_sec_data   = (bool)*ptr++;
    // uint16_t
    fmt_sec_length  = *(uint16_t *)(ptr + 0); fmt_trk_length= *(uint16_t *)(ptr + 2);
    fmt_counter     = *(uint16_t *)(ptr + 4); state_length  = *(uint16_t *)(ptr + 6);
    state_index     = *(uint16_t *)(ptr + 8); crc           = *(uint16_t *)(ptr + 10);
    lost_timeout    = *(uint16_t *)(ptr + 12); ptr += 14;
    // 2. диски
    for(int i = 0 ; i < 4; i++) {
        auto img = disks[i].image;
        if(!img) continue;
        // смещение данных диска - 4 байта для каждого, если 0 - то нет
        auto offs = *(uint32_t*)(ptr + i * 4);
        if(!offs) continue;
        auto tmp = ptr + offs;
        // дорожка
        disks[i].track = *tmp++;
        // read only
        img->write = *tmp++;
        // позиция файлового указателя
        auto fpos = (size_t)(*(uint16_t*)(tmp) | (*(uint16_t*)(tmp + 2) << 16));
        if(img->file) img->file->set_pos(fpos, zxFile::begin);
        if(fpos == -1) {
            // 3. диск в памяти - распаковать по-секторно
            if(!unpackDiskSectors(img, tmp)) {
                // не удалось распаковать - удаляем диск
                eject(i);
            }
        }
    }
    // смещение конца файла
    auto offs = *(uint32_t*)(ptr + 16);
    return (ptr + offs);
}

uint8_t *zxBetaDisk::saveState(uint8_t *ptr) {
    // сигнатура
    *ptr++ = 'S'; *ptr++ = 'E'; *ptr++ = 'R'; *ptr++ = 'G';
    // 1. переменные
    // uint8_t
    *ptr++ = mfm;       *ptr++ = dirc;      *ptr++ = busy; *ptr++ = hld;
    *ptr++ = head;      *ptr++ = hlt;       *ptr++ = intrq; *ptr++ = drq;
    *ptr++ = drive;     *ptr++ = eseek;     *ptr++ = esector; *ptr++ = ewrite;
    *ptr++ = elost;     *ptr++ = trec;      *ptr++ = lcmd;
    *ptr++ = fmt_trk;   *ptr++ = fmt_head;  *ptr++ = fmt_sec;
    *ptr++ = states;    *ptr++ = state_mult;*ptr++ = state_check;
    *ptr++ = state_head;*ptr++ = state_mark;*ptr++ = addr_sec_index;
    // bool
    *ptr++ = (uint8_t)int_on_ready;         *ptr++ = (uint8_t)int_on_unready;
    *ptr++ = (uint8_t)int_on_index_pointer; *ptr++ = (uint8_t)fmt_crc;
    *ptr++ = (uint8_t)fmt_sec_addr;         *ptr++ = (uint8_t)fmt_sec_data;
    // uint16_t
    *(uint16_t *)(ptr + 0) = fmt_sec_length;*(uint16_t *)(ptr + 2) = fmt_trk_length;
    *(uint16_t *)(ptr + 4) = fmt_counter;   *(uint16_t *)(ptr + 6) = state_length;
    *(uint16_t *)(ptr + 8) = state_index;   *(uint16_t *)(ptr + 10)= crc;
    *(uint16_t *)(ptr + 12)= lost_timeout; ptr += 14;
    // 2. диски
    auto tmp = ptr; ptr += 5 * 4;
    memset(tmp, 0, 20);
    for(int i = 0 ; i < 4; i++) {
        auto dsk = &disks[i];
        auto img = dsk->image;
        if(!img) continue;
        // смещение данных диска
        *(size_t*)(tmp + i * 4) = (ptr - tmp);
        // дорожка
        *ptr++ = dsk->track;
        // read only
        *ptr++ = (uint8_t)img->write;
        // позиция файлового указателя
        auto fpos = img->file ? img->file->get_pos() : -1;
        *(uint16_t*)(ptr + 0) = (uint16_t)(fpos & 0xffff); *(uint16_t*)(ptr + 2) = (uint16_t)((fpos >> 16) & 0xffff);
        if(!img->file) {
            uint32_t nsize;
            // 3. если диск в памяти - паковать и сохранять посекторно
            for(int trk = 0 ; trk < img->ntrk; trk++) {
                for(int head = 0; head < img->head; head++) {
                    auto track = img->get_track(trk, head);
                    for(int sec = 0 ; sec < track->get_sec_count(); sec++) {
                        auto sector = track->get_sector(sec);
                        auto l = sector->length();
                        for(int j = 0 ; j < l; j++) TMP_BUF[j] = sector->get(j);
                        // упаковать
                        auto p = packBlock(TMP_BUF, &TMP_BUF[l], ptr + 2, false, nsize);
                        auto sz = p - ptr;
                        *(uint16_t*)ptr = (uint16_t)nsize;
                        ptr += sizeof(uint16_t) + nsize;

                    }
                }
            }
        }
    }
    *(size_t*)(tmp + 16) = (ptr - tmp);
    return ptr;
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

bool zxBetaDisk::save(int active, const char *path, int type) {
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

void zxBetaDisk::write_sysreg(uint8_t data) {
    // 0,1 - номер диска
    // 2 - апаратный сброс
    // 3 - блокировка согнала HLT(должна быть 1)
    // 4 - номер головки 0=первая
    // 6 - 1=MFM, 0=FM
    drive   = (uint8_t)(data & 0x03);
    hlt     = to_bit(data & 0x08);
    head    = to_bit(!(data & 0x10));
    mfm     = to_bit(data & 0x40);
    if(!(data & 0x04)) reset_controller();
    image   = disks[drive].image;
}

void zxBetaDisk::vg93_write(uint8_t address, uint8_t data) {
    switch(address) {
        case 0x1f: process_command(data);  break;
        case 0x3f: opts[TRDOS_TRK] = data; break;
        case 0x5f: opts[TRDOS_SEC] = data; break;
        case 0x7f: opts[TRDOS_DAT] = data; if(states) write_next_byte(); break;
        case 0xff: write_sysreg(data);     break;
    }
}

uint8_t zxBetaDisk::vg93_read(uint8_t address) {
    uint8_t result = 0x0;
    switch(address) {
        case 0x1f: result = get_status(); break;
        case 0x3f: result = opts[TRDOS_TRK]; break;
        case 0x5f: result = opts[TRDOS_SEC]; break;
        case 0x7f: result = opts[TRDOS_DAT]; if(states) read_next_byte(); break;
        case 0xff: result = intrq | drq; break;
    }
    return result;
}

void zxBetaDisk::format() {
    auto rw = &TMP_BUF[ZX_BETADISK_INDEX + state_index];
    if(fmt_counter == 0) {
        if(!track_data) { states = ST_NONE; ewrite = sEWRITE; intrq = sINTRQ; drq = 0; set_busy(false); return; }
        track_data->clear();
    }
    if(mfm) {
        if(!fmt_crc) {
            // нормальная дешифровка
            switch(opts[TRDOS_DAT]) {
                case 0xf5: *rw++ = 0xa1; crc_init(); break;
                case 0xf6: *rw++ = 0xc2; break;
                case 0xf7: *rw++ = crc_hi(); *rw++ = crc_lo(); fmt_crc = true; break;
                default: *rw++ = opts[TRDOS_DAT]; break;
            }
        } else { *rw++ = opts[TRDOS_DAT]; fmt_crc = false; } // баг дешифровки после записи контрольной суммы

    } else {
        if(!fmt_crc) {
            switch(opts[TRDOS_DAT]) {
                case 0xf7: *rw++ = crc_hi(); *rw++ = crc_lo(); fmt_crc = true; break;
                case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfe: *rw++ = opts[TRDOS_DAT]; crc_init(); break;
                default: *rw++ = opts[TRDOS_DAT]; break;
            }
        } else { *rw++ = opts[TRDOS_DAT]; fmt_crc = false; }
    }
    if(!fmt_crc) {
        switch(opts[TRDOS_DAT]) {
            case 0xfe:
                if(!fmt_sec_addr && !fmt_sec_data) { fmt_sec_addr = fmt_sec_data = false; state_index = 0xffff; } break;
            case 0xf8: case 0xfb:
                if(fmt_sec_addr) {
                    fmt_sec_addr = false;
                    fmt_sec_data = true;
                    state_index = 0xffff;
                    sec_data = new zxDiskSector((size_t)-1, fmt_trk, fmt_head, fmt_sec, (uint8_t)state_length, opts[TRDOS_DAT] == 0xf8);
                    fmt_sec_length = (uint16_t)(128 << state_length);
                    track_data->add_sector(sec_data);
                }
                break;
        }
    }
    auto len = (rw - TMP_BUF);
    for(int i = 0; i < len; i++) {
        if(fmt_sec_addr) {
            switch(state_index) {
                case 0: fmt_trk    = rw[i]; break;
                case 1: fmt_head   = rw[i]; break;
                case 2: fmt_sec    = rw[i]; break;
                case 3: state_length= rw[i]; break;
            }
        }
        if(fmt_sec_addr && state_index >= 0) {
            if(state_index < fmt_sec_length) { sec_data->put(get_file(), state_index, rw[i]); }
            else { fmt_sec_addr = fmt_sec_data = false; }
        }
        crc_add(rw[i]);
        state_index++;
        fmt_counter++;
        if(fmt_counter < fmt_trk_length) drq = sDRQ;
        else { states = ST_NONE; intrq = sINTRQ; drq = 0; set_busy(false); return; }
    }
    drq = sDRQ;
}
