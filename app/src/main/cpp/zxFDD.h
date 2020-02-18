//
// Created by Sergey on 03.02.2020.
//

#pragma once

extern uint32_t Z80FQ;

#define MAX_TRK     86
#define MAX_HEAD    2
#define MAX_SEC     32
#define MAX_TRK_LEN 6250

#define FDD_RPS     5 // скорость вращения
#define PHYS_CYL    86

inline uint16_t wordLE(const uint8_t* ptr)	{ return ptr[0] | ptr[1] << 8; }
inline uint16_t wordBE(const uint8_t* ptr)	{ return ptr[0] << 8 | ptr[1]; }

#define _ST_SET(current, next)      state = ((current) | ((next) << 4))
#define _ST_NEXT                    (uint8_t)((state & 240) >> 4)

class zxDisk {
public:
    zxDisk(int _trks, int _heads);
    ~zxDisk() { delete[] raw; raw = nullptr; }
    struct TRACK {
        struct SECTOR {
            SECTOR() : caption(nullptr), content(nullptr) {}
            uint8_t trk() const	    { return caption[ID_TRK]; }
            uint8_t head() const    { return caption[ID_HEAD]; }
            uint8_t sec() const		{ return caption[ID_SEC]; }
            uint16_t len() const   	{ return (uint16_t)(128 << (caption[ID_LEN] & 3)); }
            uint16_t crcId() const	{ return wordBE(caption + ID_CRC); }
            uint16_t crcData() const{ return wordBE(content + len()); }
            // заголовок - идет сразу после A1,A1,A1,FE
            uint8_t* caption;
            // содержимое - идет сразу после A1,A1,A1,(F8/FB)
            uint8_t* content;
        };
        TRACK() : len(6400), content(nullptr), total_sec(0) { memset(sectors, 0, sizeof(sectors)); }
        void write(int pos, uint8_t val) { content[pos] = val; }
        void update();
        // длина дорожки - 6250 байт
        uint16_t len;
        // количество секторов
        uint8_t total_sec;
        // содержимое дорожи - 6250 байт
        uint8_t* content;
        // массив секторов
        SECTOR sectors[MAX_SEC];
    };
    TRACK* track(int trk, int head) { return &tracks[trk][head]; }
    int		    trks;
    int		    heads;
protected:
    TRACK	    tracks[MAX_TRK][MAX_HEAD];
    uint8_t*    raw;
    enum { ID_TRK = 0, ID_HEAD, ID_SEC, ID_LEN, ID_CRC };
};

class zxFDD {
public:
    zxFDD() : motor(0), trk(0), head(0), ts(0), protect(false), disk(nullptr) { }
    ~zxFDD() { eject(); }

    void eject() { delete disk; disk = nullptr; }
    void seek(int _trk, int _head) { if(disk) { trk = (uint8_t)_trk; head = (uint8_t)_head; ts = Z80FQ / (get_trk()->len * FDD_RPS); } }
    void write(int pos, uint8_t val) { get_trk()->write(pos, val); }
    void update_crc(zxDisk::TRACK::SECTOR* s) const;
    void fillDiskConfig(zxDisk::TRACK::SECTOR *s);

    bool is_disk() const	{ return disk != nullptr; }
    bool is_protect() const	{ return protect; }
    //bool is_boot();
    bool open(const void* data, size_t data_size, int type);

    uint32_t ticks() const { return ts; }
    uint32_t engine(int v = -1) { if(v != -1) motor = (uint32_t)v; return motor; }
    uint8_t track(int v = -1) { if(v != -1) trk = (uint8_t)v; return trk; }

    zxDisk::TRACK* get_trk() { return disk->track(trk, head); }
    zxDisk::TRACK::SECTOR* get_sec(int sec) { return &get_trk()->sectors[sec]; }
    zxDisk::TRACK::SECTOR* get_sec(int trk, int head, int sec);

    bool    protect;
    uint8_t head;
protected:
    bool write_sec(int trk, int head, int sec, const uint8_t * data);
    void write_blk(int& pos, uint8_t val, int count) { for(int i = 0; i < count; ++i) get_trk()->write(pos++, val); }
    void make_trd();
    bool add_file(const uint8_t * hdr, const uint8_t * data);
    bool read_scl(const void* data, size_t data_size);
    bool read_trd(const void* data, size_t data_size);
    bool read_fdi(const void* data, size_t data_size);

    uint16_t CRC(uint8_t * src, int size) const;

protected:
    uint32_t motor, ts;
    uint8_t  trk;
    zxDisk*  disk;
};

class zxVG93 {
public:
    enum FDD_REQ { R_NONE, R_DRQ = 0x40, R_INTRQ = 0x80 };
    enum { C_WTRK = 0xF0, C_RTRK = 0xE0, C_RSEC = 0x80, C_WSEC = 0xA0, C_RADR = 0xC0, C_INTERRUPT = 0xD0 };
    enum FDD_STATUS {
        ST_BUSY	= 0x01, ST_INDEX = 0x02, ST_DRQ	= 0x02, ST_TRK00 = 0x04,
        ST_LOST	= 0x04, ST_CRCERR = 0x08, ST_NOT_SEC = 0x10, ST_SEEKERR	= 0x10,
        ST_RECORDT = 0x20, ST_HEADL = 0x20, /*ST_WRFAULT = 0x20, */ST_WRITEP = 0x40, ST_NOTRDY = 0x80
    };
    enum FDD_STATE {
        S_IDLE, S_WAIT, S_PREPARE_CMD, S_CMD_RW, S_FIND_SEC,
        S_READ, S_WRSEC, S_WRITE, S_WRTRACK, S_WR_TRACK_DATA, S_TYPE1_CMD,
        S_STEP, S_SEEKSTART, S_SEEK, S_VERIFY
    };
    enum FDD_CMD {
        CB_SEEK_VERIFY = 0x04, CB_SEEK_TRKUPD = 0x10, CB_SEEK_DIR = 0x20, CB_SYS_HLT = 0x08, CB_WRITE_DEL = 0x01,
        CB_DELAY = 0x04, CB_MULTIPLE = 0x10, CB_RESET = 0x04
    };
    // конструктор
    zxVG93();
    // монтировать образ
    bool open(const char* path, int drive, int type);
    // признак наличия бута
    //bool is_boot(int drive) { return fdds[drive].is_boot(); }
    // сброс
    void reset();
    // извлечь
    void eject(int num) { fdds[num].eject(); }
    // записать в порт
    void vg93_write(uint8_t port, uint8_t v);
    // перехват команд
    void trap(uint16_t pc);
    // восстановить состояние
    uint8_t* restoreState(uint8_t* ptr);
    // сохранить состояние
    uint8_t* saveState(uint8_t* ptr);
    // прочитать из порта
    uint8_t vg93_read(uint8_t port);
    // установка/чтение режима защиты записи
    int is_readonly(int num, int write = -1) { if(write != -1) fdds[num].protect = write != 0; return fdds[num].is_protect(); }
    // сохранение диска
    int save(const char *path, int num, int type);
    // обновление свойств
    void updateProps();
    // прочитать содержимое сектора
    int read_sector(int num, int sec);
protected:
    // выполнение
    void exec();
    // чтение первого байта
    void read_byte();
    // имем сектор на дорожке
    void find_sec();
    // признак готовности
    bool ready();
    // загрузка трека
    void load();
    // получение импульса
    void get_index(int s_next);
    // вычисление КК
    uint16_t CRC(uint8_t * src, int size) const;
    // вычисление КК
    uint16_t CRC(uint8_t v, uint16_t prev = 0xcdb4) const;
private:
    // процедуры обработки команд
    void cmdReadWrite();
    void cmdWriteTrackData();
    void cmdType1();
    void cmdStep();
    void cmdWrite();
    void cmdPrepareRW();
    void cmdPrepareSec();
    void cmdRead();
    void cmdWriteSector();
    void cmdWriteTrack();
    void cmdSeek();
    void cmdVerify();
    void log_to_data(bool is_text, const char* title, int trk, int sec, int head);
    // текущая операция чтения/записи
    uint8_t ops;
    // текущий дисковод
    uint8_t nfdd;
    // следующее время/время ожидания сектора
    uint32_t time, next, wait_drq;
    // напраление
    int8_t direction;
    // начальный адрес вычисления КК
    uint8_t* start_crc;
    // адрес чтения/записи
    uint8_t* _rwptr;
    uint8_t* rwptr;
    // длина буфера чтения/записи
    uint16_t rwlen;
    // текущее состояние
    uint8_t	state;
    // состояние порта 0xFF
    uint8_t	rqs;
    // текущий КК
    uint16_t crc;
    // текущий дисковод
    zxFDD* fdd;
    // все дисководы
    zxFDD fdds[4];
    // текущий сектор
    zxDisk::TRACK::SECTOR* found_sec;
};
