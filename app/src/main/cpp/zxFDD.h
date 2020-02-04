//
// Created by Sergey on 03.02.2020.
//

#pragma once

#define MAX_TRK     86
#define MAX_HEAD    2
#define MAX_SEC     32

#define ID_TRK      0
#define ID_HEAD     1
#define ID_SEC      2
#define ID_LEN      3
#define ID_CRC      4

#define FDD_RPS     5 // скорость вращения
#define PHYS_CYL    86
//#define SEEK_SET_TRACK 0x10

enum FDD_CMD {
    CB_SEEK_RATE	= 0x03,
    CB_SEEK_VERIFY	= 0x04,
    //CB_SEEK_HEADLOAD= 0x08,
    CB_SEEK_TRKUPD	= 0x10,
    CB_SEEK_DIR		= 0x20,
    CB_SYS_HLT      = 0x08,
    CB_WRITE_DEL	= 0x01,
    CB_SIDE_CMP		= 0x02,
    CB_DELAY		= 0x04,
    //CB_SIDE			= 0x08,
    CB_SIDE_SHIFT	= 3,
    CB_MULTIPLE		= 0x10
};
enum FDD_STATE {
    S_IDLE, S_WAIT, S_DELAY_BEFORE_CMD, S_CMD_RW, S_FOUND_NEXT_ID,
    S_READ, S_WRSEC, S_WRITE, S_WRTRACK, S_WR_TRACK_DATA, S_TYPE1_CMD,
    S_STEP, S_SEEKSTART, S_SEEK, S_VERIFY, S_RESET
};
enum FDD_REQ { R_NONE, R_DRQ = 0x40, R_INTRQ = 0x80 };
enum FDD_STATUS {
    ST_BUSY		= 0x01,
    ST_INDEX	= 0x02,
    ST_DRQ		= 0x02,
    ST_TRK00	= 0x04,
    ST_LOST		= 0x04,
    ST_CRCERR	= 0x08,
    ST_NOT_SEC	= 0x10,
    ST_SEEKERR	= 0x10,
    ST_RECORDT	= 0x20,
    ST_HEADL	= 0x20,
    //ST_WRFAULT	= 0x20,
    ST_WRITEP	= 0x40,
    ST_NOTRDY	= 0x80
};

enum {
    C_WTRK    = 0xF0,
    C_RTRK    = 0xE0,
    C_RSEC    = 0xC0,
    C_WSEC    = 0xA9,
    C_RADR    = 0xC0

};
static inline uint16_t wordLE(const uint8_t* ptr)	{ return ptr[0] | ptr[1] << 8; }
static inline uint16_t wordBE(const uint8_t* ptr)	{ return ptr[0] << 8 | ptr[1]; }
static inline uint32_t Dword(const uint8_t * ptr) { return ptr[0] | (uint32_t)(ptr[1]) << 8 | (uint32_t)(ptr[2]) << 16 | (uint32_t)(ptr[3]) << 24; }
inline uint16_t swap_byte_order(uint16_t v) { return (v >> 8) | (v << 8); }

class zxDisk {
public:
    zxDisk(int _trks, int _heads);
    ~zxDisk() { delete[] raw; raw = nullptr; }
    struct TRACK {
        struct SECTOR {
            SECTOR() : id(nullptr), data(nullptr) {}
            int trk() const	    	{ return id[ID_TRK]; }
            int head() const    	{ return id[ID_HEAD]; }
            int sec() const		    { return id[ID_SEC]; }
            int len() const	    	{ return 128 << (id[ID_LEN] & 3); }
            uint16_t crcId() const	{ return wordBE(id + ID_CRC); }
            uint16_t crcData() const{ return wordBE(data + len()); }
            uint8_t* id;
            uint8_t* data;
        };
        TRACK() : len(6400), data(nullptr), id(nullptr), total_sec(0) {}
        bool marker(int pos) const { return (id[pos / 8] & (1 << (pos & 7))) != 0; }
        void write(int pos, uint8_t val, bool marker = false);
        void update();
        SECTOR	 sectors[MAX_SEC];
        int		 total_sec;
        int		 len;
        uint8_t* id;
        uint8_t* data;
    };
    TRACK* track(int trk, int head) { return &tracks[trk][head]; }
    int		    trks;
    int		    heads;
protected:
    TRACK	    tracks[MAX_TRK][MAX_HEAD];
    uint8_t*    raw;

};

class zxFDD {
public:
    zxFDD() : motor(0), trk(0), head(0), ts(0), protect(false), disk(nullptr) { }
    ~zxFDD() { eject(); }

    void eject() { delete disk; disk = nullptr; }
    void seek(int _trk, int _head);
    void write(int pos, uint8_t val, bool marker = false) { get_trk()->write(pos, val, marker); }

    u_long ticks() const { return ts; }
    int track(int v = -1) { if(v != -1) trk = v; return trk; }

    bool is_disk() const	{ return disk != nullptr; }
    bool is_protect() const	{ return protect; }
    bool is_boot();

    bool open(const void* data, size_t data_size, int type);

    zxDisk::TRACK* get_trk() { return disk->track(trk, head); }
    zxDisk::TRACK::SECTOR* get_sec(int sec) { return &get_trk()->sectors[sec]; }
    uint32_t engine(int v = -1) { if(v != -1) motor = (uint32_t)v; return motor; }

    bool protect;
protected:
    bool write_sec(int trk, int head, int sec, const uint8_t * data);
    void write_blk(int& pos, uint8_t val, int count, bool marker = false) { for(int i = 0; i < count; ++i) get_trk()->write(pos++, val, marker); }
    void make_trd();
    bool add_file(const uint8_t * hdr, const uint8_t * data);
    bool read_scl(const void* data, size_t data_size);
    bool read_trd(const void* data, size_t data_size);
    bool read_fdi(const void* data, size_t data_size);
    void update_crc(zxDisk::TRACK::SECTOR* s) const;

    uint16_t CRC(uint8_t * src, int size) const;
    zxDisk::TRACK::SECTOR* get_sec(int trk, int head, int sec);

protected:
    uint32_t	motor;	// 0 - не вращается, > 0 - время, когда остановится
    int		    trk;
    int		    head;
    u_long	    ts;
    zxDisk*	    disk;
};

class zxVG93 {
public:
    zxVG93();
    bool open(const char* path, int drive, int type);
    bool is_boot(int drive) { return fdds[drive].is_boot(); }

    void reset() { }
    void eject(int num) { fdds[num].eject(); }
    uint8_t* loadState(uint8_t* ptr) { return ptr; }
    uint8_t* saveState(uint8_t* ptr) { return ptr; }
    uint8_t vg93_read(uint8_t port, int tact);
    void vg93_write(uint8_t port, uint8_t v, int tact);

    int is_readonly(int num, int write = -1) { if(write != -1) fdds[num].protect = write != 0; return fdds[num].is_protect(); }
    int save(const char *path, int num, int type);
    void updateProps();

protected:
    void    	exec(int tact);
    void    	read_first_byte();
    void    	find_marker();
    bool    	ready();
    void    	load() { fdd->seek(fdd->track(), head); }
    void    	get_index();
    uint16_t	CRC(uint8_t * src, int size) const;
    uint16_t	CRC(uint8_t v, uint16_t prev = 0xcdb4) const;

private:
    int		tshift;
    uint8_t	state;
    uint8_t	state_next;
    //uint8_t	cmd;
    //int		track;
    int		head;
    int		direction;

    uint8_t	rqs;
    //uint8_t	status;
    uint8_t	system;	// beta128 system register

    // read/write sector(s) data
    int     rwptr;
    int		rwlen;
    int		start_crc;

    uint16_t	crc;
    zxFDD*	    fdd;
    zxFDD	    fdds[4];
    uint32_t	next;
    uint32_t	end_waiting_am;
    zxDisk::TRACK::SECTOR*  found_sec;
    bool        wd93_nodelay;
};
