//
// Created by Sergey on 24.01.2020.
//

#pragma once

#include "zxCommon.h"

#define SEC_LENGTH_0x0100   1
#define STEP_TIME           200
#define LOST_TIME           0.05
#define HLD_EXTRA_TIME      STEP_TIME * 15

#define sELOST              4
#define sESECTOR            16
#define sESEEK              16
#define sEWRITE             32
#define sDRQ                64
#define sINTRQ              128

enum GAPS { GAP_I, GAP_II, GAP_III, GAP_IV, GAP_SYNC };

enum STATES { ST_NONE = 0, ST_READ = 1, ST_WRITE = 2 };

enum VG_CMD {
    VG_NONE, VG_INTERRUPT, VG_RESTORE, VG_SEEK, VG_STEP, VG_STEP0, VG_STEP1,
    VG_RSECTOR, VG_WSECTOR, VG_RADDRESS, VG_RTRACK, VG_WTRACK
};

class zxDiskSector {
public:
    zxDiskSector(size_t pos, uint8_t trk, uint8_t head, uint8_t sec, uint8_t len, bool _deleted) {
        fpos = pos; ntrk = trk; nhead = head; nsec = sec; slen = len;
        flags = (uint8_t)((1 << len) | (_deleted ? 0x80 : 0));
    }
    size_t length() { return (size_t)(128 << slen); }
    bool is_deleted(int value = -1) {
        if(value != -1) { if(value) flags |= 0x80; else flags &= 0x7f; }
        return (flags & 0x80) != 0;
    }
    void put(zxFile* file, int index, uint8_t value) { if(is_index_valid(index)) { file ? file->write_byte(value) : put(index, value); } }
    uint8_t get(zxFile* file, int index) { return (uint8_t)(is_index_valid(index) ? (file ? file->read_byte() : get(index)) : 0); }
    // если диск в памяти
    void put(int index, uint8_t value) { if(is_index_valid(index)) data[index] = value; }
    uint8_t get(int index) { return (uint8_t)(is_index_valid(index) ? data[index] : 0); }
    // номер дорожки
    uint8_t ntrk;
    // номер головки
    uint8_t nhead;
    // номер сектора
    uint8_t nsec;
    // длина сектора
    uint8_t slen;
    // признак удаленного сектора
    uint8_t flags;
    // позиция в файле
    size_t fpos;
private:
    bool is_index_valid(int index) { return (index >= 0 && index < (128 << slen)); }
    // данные сектора
    uint8_t data[1024];
};

class zxDiskTrack {
public:
    zxDiskTrack() : nsec(0) { }
    ~zxDiskTrack() { clear(); }
    void clear() { for(int i = 0 ; i < nsec; i++) delete sectors[i]; nsec = 0; }
    void add_sector(zxDiskSector* sector) { sectors[nsec++] = sector; }
    int get_sec_count() { return nsec; }
    zxDiskSector* get_sector(int index) { return (index < nsec ? sectors[index] : nullptr); }
private:
    // количество секторов
    int nsec;
    // массив секторов
    zxDiskSector* sectors[16];
};

class zxDiskImage {
    friend class zxBetaDisk;
public:
    zxDiskImage(int _head, int _trk, bool _write, const char* _desc, const char* path);
    ~zxDiskImage() { clear(); delete file; }
    // проверка на совместимый формат
    static bool getCompatibleFormats(zxDiskImage* image, int fmt);
    // открыть образ формата TRD
    static zxDiskImage* openTRD(const char* path);
    // открыть образ формата SCL
    static zxDiskImage* openSCL(const char* path);
    // открыть образ формата FDI
    static zxDiskImage* openFDI(const char* path);
    // сохранить в формате TRD
    static uint8_t* saveToTRD(zxDiskImage* image, uint32_t* length);
    // сохранить в формате SCL
    static uint8_t* saveToSCL(zxDiskImage* image, uint32_t* length);
    // сохранить в формате FDI
    static uint8_t* saveToFDI(zxDiskImage* image, uint32_t* length);
    // признак защиты от записи
    bool is_protected() { return write; }
    // вернуть количество цилиндров
    int get_cyl_count() { return ntrk; }
    // вернуть количество головок
    int get_head_count() { return head; }
    // очистить массив цилиндров
    void clear() { for(int i = 0 ; i < ntrk; i++) delete [] tracks[i]; ntrk = 0; }
    // добавить цилиндр
    zxDiskTrack* add_cyl() { return tracks[ntrk++] = new zxDiskTrack[head]; }
    // вернуть трек из цилиндра
    zxDiskTrack* get_track(int cyl_index, int head_index) {
        if(cyl_index < 0 || head_index < 0) return nullptr;
        if(cyl_index < ntrk && head_index < head) return &tracks[cyl_index][head_index];
        return nullptr;
    }
protected:
    // количество головок
    int head;
    // количество треков
    int trk;
    // количество треков на текущий момент
    int ntrk;
    // признак защиты от записи
    bool write;
    // массив массивов треков
    zxDiskTrack** tracks;
    // описание диска
    std::string desc;
    // файл
    zxFile* file;
};

class zxBetaDisk {
public:
    struct DISK {
        DISK() : image(nullptr), track(0) { }
        ~DISK() { delete image; image = nullptr; }
        zxDiskImage* image;
        uint8_t track;
    };
    zxBetaDisk();
    ~zxBetaDisk() { }
    void vg93_write(uint8_t address, uint8_t data);
    void reset();
    bool open(int active, const char* path, int type);
    bool save(uint8_t active, const char *path, int type);
    // восстановить состояние
    uint8_t * loadState(uint8_t* ptr);
    // сохранить состояние
    uint8_t* saveState(uint8_t* ptr);
    uint8_t vg93_read(uint8_t address);
protected:
    size_t buildGAP(int gap);
    void set_states(int type, int length) { state_length = length; states = type; state_index = 0; }
    void reset_controller();
    void insert(int drive_code, zxDiskImage* image);
    void eject(int drive_code);
    void process_command(uint8_t cmd);

    void read_sectors();
    void read_next_byte();
    void read_track();
    void write_sectors();
    void write_next_byte();
    void step(int cmd);
    void format();

    void write_sysreg(uint8_t data);
    void crc_add(uint8_t v8);
    void crc_init() { crc = 0xffff; }
    void on_drive_ready(int drive_index) { if(drive_index == drive && int_on_ready) { intrq = 1; int_on_ready = false; } }
    void on_drive_unready(int drive_index) { if(drive_index == drive && int_on_unready) { intrq = 1; int_on_unready = false; } }
    void set_current_track(int value) { disks[drive].track = value; }
    bool ready() { return image != nullptr; }
    void set_busy(int value);
    bool index_pointer();
    bool unpackDiskSectors(zxDiskImage* img, uint8_t* ptr);
    int  current_track() { return disks[drive].track; }
    zxFile* get_file() { return image->file; }
    uint8_t crc_lo() { return (uint8_t)(crc & 0xff); }
    uint8_t crc_hi() { return (uint8_t)(crc >> 8); }
    uint8_t get_hld();
    uint8_t to_bit(int b) { return (uint8_t)(b != 0); }
    uint8_t get_status();
    zxDiskSector* find_sector();
    // массив дисков
    DISK disks[4];
    // текущий диск
    zxDiskImage* image;
    // время срабатывания команды. Используется для задержки
    u_long cmd_time;
    // 0 - MFM, 1 - FM ( UNDONE: not sure )
    uint8_t mfm;
    // 1 - перемещение к центру, 0 - к краю
    uint8_t dirc;
    // Признак выполнения команды
    uint8_t busy;
    // После прекращения выполнения команды, использующей загрузку головки, сигнал hld ещё держится 15 оборотов
    // диска (если только не осуществлен аппаратный сброс). Эмуляция данного поведения.
    uint8_t hld;
    // 0 - нижняя головка, 1 - верхняя
    uint8_t head;
    // блокировка контроллера
    uint8_t hlt;
    // запрос комманды/запрос данных
    uint8_t intrq, drq;
    // активный диск
    uint8_t drive;
    // ошибка позиционирования
    uint8_t eseek;
    // сектор не найден
    uint8_t esector;
    // ошибка записи
    uint8_t ewrite;
    // данные потеряны
    uint8_t elost;
    // 0 - normal, 1 - deleted
    uint8_t trec;
    // в отличие от r_cmd не меняется, если поступает команда прерывания во время выполнения другой команды
    uint8_t lcmd;
    // контрольная сумма
    uint16_t crc;
    // текущее состояние
    uint8_t states;
    // При перемещениях головки сбрасывается в 0. При чтении адреса указывает на следующий сектор, с заголовка которого будет прочитан адрес.
    // После чтения значение увеличивается на 1, либо становится 0, если это был последний сектор на дорожке.
    uint8_t addr_sec_index;
    // задержка потери данных при чтении/записи
    uint16_t lost_timeout;
    // уровни сигналов
    bool int_on_ready, int_on_unready;
    bool int_on_index_pointer;
    // параметры управления состоянием
    uint16_t state_length, state_index;
    uint8_t state_mult, state_check, state_head, state_mark;
    // параметры управления форматированием
    // признак вычисленной CRC
    bool fmt_crc;
    // признак адресной зоны сектора
    bool fmt_sec_addr;
    // признак зоны данных сектора
    bool fmt_sec_data;
    // длина сектора в байтах
    uint16_t fmt_sec_length;
    // длина трека
    uint16_t fmt_trk_length;
    // текущее количество отформатированных треков
    uint16_t fmt_counter;
    // форматируемая дорожка/головка/сектор
    uint8_t fmt_trk, fmt_head, fmt_sec;
};