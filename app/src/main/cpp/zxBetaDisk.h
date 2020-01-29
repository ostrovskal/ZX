//
// Created by Sergey on 24.01.2020.
//

#pragma once

#define SEC_LENGTH_0x0080   0
#define SEC_LENGTH_0x0100   1
#define SEC_LENGTH_0x0200   2
#define SEC_LENGTH_0x0400   3

extern "C" {
    u_long getTimeMillis();
};

class SectorData {
public:
    SectorData(uint8_t _cyl_byte, uint8_t _head_byte, uint8_t _sec_byte, uint8_t _length_byte, bool _deleted) {
        cyl_byte    = _cyl_byte;
        head_byte   = _head_byte;
        sec_byte    = _sec_byte;
        length_byte = _length_byte;
        flags       = (uint8_t)(_deleted ? 0x80 : 0);
        switch(length_byte) {
            case SEC_LENGTH_0x0080: flags |= 0x01; break;
            case SEC_LENGTH_0x0100: flags |= 0x02; break;
            case SEC_LENGTH_0x0200: flags |= 0x04; break;
            case SEC_LENGTH_0x0400: flags |= 0x08; break;
        }
    }
    uint8_t get_cyl_byte() { return cyl_byte; }
    uint8_t get_head_byte() { return head_byte; }
    uint8_t get_sec_byte() { return sec_byte; }
    uint8_t get_length_byte() { return length_byte; }
    uint8_t get_flags() { return flags; }
    bool is_deleted(int value) {
        if(value != -1) { if(value) flags |= 0x80; else flags &= 0x7f; }
        return (flags & 0x80) != 0;
    }
    uint8_t get_data_byte(int index) {
        if(is_data_index_valid(index)) return data[index];
        // Error('The data index is out of range');
        return 0;
    }
    void set_data_byte(int index, uint8_t value) {
        if(is_data_index_valid(index)) data[index] = value;
        // Error('The data index is out of range');
    }
    bool is_data_index_valid(int index) {
        if(index < 0) return false;
        return index < (0x0080 << length_byte);
    }
private:
    uint8_t cyl_byte;
    uint8_t head_byte;
    uint8_t sec_byte;
    uint8_t length_byte;
    uint8_t flags;
    // sector data bytes
    uint8_t data[1024];
};

class TrackData {
public:
    ~TrackData() { clear(); }
    int get_sec_count() { return sec_pos; }
    void clear() {
        for(int i = 0 ; i < sec_pos; i++) delete sectors[i];
        sec_pos = 0;
    }
    void add_sector(SectorData* sector) { sectors[sec_pos++] = sector; }
    SectorData* get_sector(int index) { if(index < sec_pos) return sectors[index]; return nullptr; }
private:
    SectorData** sectors;
    int sec_pos;
};

class zxDiskImage {
public:
    zxDiskImage(const char* _filename, int _head_count, int _cyl_count, bool _write_protect, const char* _description) {
        filename    = _filename;
        description = _description;
        head_count  = _head_count;
        cyl_count   = _cyl_count;
        write_protect = _write_protect;
        cylinders   = new TrackData*[_cyl_count];
        cyl_pos     = 0;
    }
    ~zxDiskImage() { clear(); }
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
    // восстановить состояние
    static zxDiskImage* openState(uint8_t* ptr);
    // сохранить состояние
    static uint8_t* saveState(uint8_t* ptr);
    // вернуть имя файла
    const char* get_filename() { return filename.c_str(); }
    // вернуть признак защиты от записи
    bool is_write_protected() { return write_protect; }
    // вернуть количество цилиндров
    int get_cyl_count() { return cyl_pos; }
    // вернуть количество головок
    int get_head_count() { return head_count; }
    // очистить массив цилиндров
    void clear() { for(int i = 0 ; i < cyl_pos; i++) delete [] cylinders[i]; cyl_pos = 0; }
    // добавить цилиндр
    TrackData* add_cyl() { TrackData* cyl(nullptr); if(cyl_pos < cyl_count) cylinders[cyl_pos++] = cyl = new TrackData[head_count]; return cyl; }
    // вернуть трек из цилиндра
    TrackData* get_track(int cyl_index, int head_index) {
        if(cyl_index < 0 || head_index < 0) return nullptr;
        if(cyl_index < cyl_pos && head_index < head_count) return &cylinders[cyl_index][head_index];
        return nullptr;
    }
protected:
    // количество головок
    int head_count;
    // количество цилиндров
    int cyl_count;
    // позиция для добавления
    int cyl_pos;
    // признак защиты от записи
    bool write_protect;
    // массив массивов треков
    TrackData** cylinders;
    // имя файла
    std::string filename;
    // описание диска
    std::string description;
};

class zxBetaDisk {
public:
    struct STATE {
        STATE() : data(nullptr) { }
        ~STATE() { delete[] data; data = nullptr; }
        bool action;
        int index, length;
        uint8_t* data;
    };
    struct DISK {
        zxDiskImage* image;
        int track;
    };
    zxBetaDisk();
    ~zxBetaDisk() { }

    void insert(int drive_code, zxDiskImage* image);
    void eject(int drive_code);
    void vg93_write(uint8_t address, uint8_t data);
    void process_command(uint8_t cmd);
    void read_sectors_begin();
    void read_sectors_end(bool multiple, uint8_t expected_side, bool check_side);
    void write_sectors_begin();
    void write_sectors_end(bool multiple, uint8_t expected_side, bool check_side, bool deleted_address_mark);
    void read_address();
    void read_track();
    void write_next_byte();
    void read_next_byte();
    void write_track();
    void store_sector_data(SectorData* sec_data, uint8_t* data);
    void write_sysreg(uint8_t data);
    void hardware_reset_controller();
    void reset();
    bool open(int active, const char* path, int type);
    uint8_t vg93_read(uint8_t address);
    size_t extract_sector_data(SectorData* sec_data);
    uint8_t* extract_sector_address(SectorData* sec_data);
    size_t build_GAP(int gap_num);
    size_t build_SYNC();
    uint8_t to_bit(int b) { return (uint8_t)(b ? 1 : 0); }
    uint8_t get_status();
    uint8_t read_sysreg(uint8_t data);
    SectorData* get_sector_data(uint8_t expected_side, bool check_side);
    void schedule_data(bool read, SectorData* sec, uint8_t* data, size_t length, bool multiple, uint8_t expected_side, bool check_side, bool action, bool mark);

    bool save(uint8_t active, const char *path, int type);

protected:
    void on_drive_ready(int drive_index) { if(drive_index == drive && int_on_ready) { intrq = 1; int_on_ready = false; } }
    void on_drive_unready(int drive_index) { if(drive_index == drive && int_on_unready) { intrq = 1; int_on_unready = false; } }
    void set_current_track(int value) { disks[drive].track = value; }
    void set_busy(int value) { busy = to_bit(value); if(value) hld = get_hld(); else idle_since = getTimeMillis(); }
    bool ready() { return disks[drive].image != nullptr; }
    bool index_pointer() { return ((getTimeMillis() - last_index_pointer_time ) < index_pointer_length); }
    bool is_busy() { return to_bool(busy); }
    bool to_bool(int b) { return b != 0; }
    int get_current_track() { return disks[drive].track; }
    zxDiskImage* get_current_image() { return disks[drive].image; }
    uint8_t get_hld() { if(is_busy()) return hld; return (uint8_t)(hld && (getTimeMillis() - idle_since) < hld_extratime); }

    DISK disks[4];
    int drive;
    int head; // 0 - нижняя головка, 1 - верхняя
    bool mfm; // 0 - MFM, 1 - FM ( UNDONE: not sure )
    // интервал появления индексного отверстия (ms)
    int index_pointer_interval;
    // длина индексного отверстия (ms)
    int index_pointer_length;
    // таймаут чтения/записи дорожки, после которого происходит прерывание операции
    //int track_rw_timeout;
    // Таймаут чтения или записи одного байта (ms). В чистом виде не используется, т.к. слишком мал, а используется на группе байтов.
    // Реальное время чтения/записи одного байта в режиме MFM составляет 0.032 ms, но из-за низкой производительности js используется большее число.
    //double rw_timeout;
    int hld_extratime;
    int intrq;
    int drq;
    int hlt; // приходит с порта 0xff
    // После прекращения выполнения команды, использующей загрузку головки, сигнал hld ещё держится 15 оборотов
    // диска (если только не осуществлен аппаратный сброс). Эмуляция данного поведения.
    uint8_t busy;
    uint8_t hld;
    u_long idle_since;
    uint8_t seek_error;
    uint8_t crc_error;
    uint8_t rnf_error;
    uint8_t lost_data_error;
    uint8_t write_fault;
    uint8_t record_type; // 0 - normal, 1 - deleted
    uint8_t dirc; // 1 - перемещение к центру, 0 - к краю
    u_long last_index_pointer_time;
    uint8_t r_sector; // для TR-DOS: от 1 по 16
    uint8_t r_track;
    uint8_t r_command;
    uint8_t r_data;
    // last_command в отличие от r_command не меняется, если поступает команда прерывания во время выполнения другой команды
    uint8_t last_command;
    // При перемещениях головки сбрасывается в 0. При чтении адреса указывает на следующий сектор, с заголовкка
    // которого будет прочитан адрес. После чтения значение увеличивается на 1 либо становится 0, если это был последний сектор на дорожке.
    int addr_sec_index;
    STATE* read_state;
    STATE* write_state;
    bool int_on_ready;
    bool int_on_unready;
    //bool int_on_index_pointer;
};