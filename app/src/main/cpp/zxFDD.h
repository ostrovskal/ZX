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

#define _ST_SET(current, next)      states = ((current) | ((next) << 4))
#define _ST_NEXT                    (uint8_t)((states & 240) >> 4)

extern "C" {
    inline uint16_t wordBE(const uint8_t* ptr);
};

class zxDisk {
public:
    zxDisk(int _trks, int _heads);
    ~zxDisk() { delete[] raw; raw = nullptr; }
    struct TRACK {
        struct SECTOR {
            SECTOR() : caption(nullptr), content(nullptr) {}
            uint8_t trk() const	        { return caption[ID_TRK]; }
            uint8_t head() const        { return caption[ID_HEAD]; }
            uint8_t sec() const		    { return caption[ID_SEC]; }
            uint16_t len() const   	    { return (uint16_t)(128 << (caption[ID_LEN] & 3)); }
            uint16_t crcCaption() const	{ return wordBE(caption + ID_CRC); }
            uint16_t crcContent() const { return wordBE(content + len()); }
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
        // содержимое дорожки - 6250 байт
        uint8_t* content;
        // массив секторов
        SECTOR sectors[MAX_SEC];
    };
    TRACK*      get_track(int trk, int head) { return &tracks[trk][head]; }
    int		    trks;
    int		    heads;
protected:
    TRACK	    tracks[MAX_TRK][MAX_HEAD];
    uint8_t*    raw;
    enum { ID_TRK = 0, ID_HEAD, ID_SEC, ID_LEN, ID_CRC };
};

class zxFDD {
    friend class zxFormats;
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
    bool open(uint8_t* data, size_t data_size, int type);
    uint8_t* save(int type);

    uint32_t ticks() const { return ts; }
    uint32_t engine(int v = -1) { if(v != -1) motor = (uint32_t)v; return motor; }
    uint8_t track(int v = -1) { if(v != -1) trk = (uint8_t)v; return trk; }

    zxDisk::TRACK* get_trk() { return disk->get_track(trk, head); }
    zxDisk::TRACK::SECTOR* get_sec(int sec) { return &get_trk()->sectors[sec]; }
    zxDisk::TRACK::SECTOR* get_sec(int trk, int head, int sec);

    bool    protect;
    uint8_t head;
protected:
    bool write_sec(int trk, int head, int sec, const uint8_t * data);
    void write_blk(int& pos, uint8_t val, int count) { for(int i = 0; i < count; ++i) get_trk()->write(pos++, val); }
    void make_trd();
    bool add_file(const uint8_t * hdr, const uint8_t * data);
    uint16_t CRC(uint8_t * src, int size) const;
protected:
    uint32_t motor, ts;
    uint8_t  trk;
    zxDisk*  disk;
};
