//
// Created by Sergey on 10.02.2020.
//

#pragma once

#include "zxFormats.h"

#define Z80FQ           3500000
#define MAX_TRACK_LEN   6250
//#define FDD_RPS         5
#define MAX_CYLS        86
#define MAX_PHYS_CYL    86
//#define MAX_SEC         256

enum SEEK_MODE { JUST_SEEK = 0, LOAD_SECTORS = 1 };

enum CMDBITS {
    CMD_SEEK_RATE     = 0x03,
    CMD_SEEK_VERIFY   = 0x04,
//    CMD_SEEK_HEADLOAD = 0x08,
    CMD_SEEK_TRKUPD   = 0x10,
    CMD_SEEK_DIR      = 0x20,
    CMD_WRITE_DEL     = 0x01,
    CMD_SIDE_CMP_FLAG = 0x02,
    CMD_DELAY         = 0x04,
//    CMD_SIDE          = 0x08,
    CMD_SIDE_SHIFT    = 3,
    CMD_MULTIPLE      = 0x10,
};

/*
enum WDSTATE {
    S_IDLE = 0, S_WAIT, S_PREPARE_CMD, S_CMD_RW, S_FIND_SEC,
    S_READ, S_WRSEC, S_WRITE, S_WRTRACK, S_WR_TRACK_DATA,
    S_TYPE1_CMD, S_STEP, S_SEEKSTART, S_SEEK, S_VERIFY, S_RESET
};

*/
enum BETA_STATUS {
    DRQ   = 0x40, INTRQ = 0x80
};

enum WD_STATUS {
    WDS_BUSY      = 0x01, WDS_INDEX     = 0x02, WDS_DRQ       = 0x02,
    WDS_TRK00     = 0x04, WDS_LOST      = 0x04, WDS_CRCERR    = 0x08,
    WDS_NOTFOUND  = 0x10, WDS_SEEKERR   = 0x10, WDS_RECORDT   = 0x20,
    WDS_HEADL     = 0x20, WDS_WRITEP    = 0x40, WDS_NOTRDY    = 0x80
};

class SECHDR {
public:
    uint8_t c, s, n, l;
    uint16_t crc;
    uint8_t c1, c2;
    uint8_t *data, *id;
    int datlen;
    uint16_t crcd;
};

class FDD;
class TRKCACHE {
public:
    TRKCACHE() { clear(); }
    void set_i(int pos) { trki[pos / 8] |= 1 << (pos & 7); }
    void clr_i(int pos) { trki[pos / 8] &= ~(1 << (pos & 7)); }
    void write(int pos, uint8_t byte, char index) { trkd[pos] = byte; if (index) set_i(pos); else clr_i(pos); }
    void seek(FDD *d, int cyl, int side, SEEK_MODE fs);
    void format();
    void clear() { drive = 0; trkd = 0; }
    int write_sector(int sec, uint8_t *data);

    uint8_t test_i(int pos) { return (uint8_t)(trki[pos / 8] & (1 << (pos & 7))); }
    SECHDR *get_sector(int sec);

    SEEK_MODE  sf;
    int cyl, side;
    int trklen;
    uint8_t *trkd, *trki;
    int ts_byte;
    int s;
    SECHDR hdr[MAX_SEC];
    struct FDD *drive;
};

class FDD {
public:
    ~FDD() { free(); }
    void free();
    int index();
    int addfile(uint8_t *hdr, uint8_t *data);

    void format_trd();
    void emptydisk();
    void newdisk(int cyls, int sides);
    void addboot();

    TRKCACHE t;
    uint64_t motor;
    uint8_t track;

    uint8_t *rawdata;
    int rawsize;
    int cyls, sides;
    int trklen[MAX_CYLS][2];
    uint8_t *trkd[MAX_CYLS][2];
    uint8_t *trki[MAX_CYLS][2];
    uint8_t optype; // bits: 0-not modified, 1-write sector, 2-format track
    char name[512];
};

class WD1793 {
public:
    WD1793() : state(S_IDLE) { seldrive = &fdd[0]; }
    uint8_t vg93_read(uint8_t port, int tact);
    void vg93_write(uint8_t port, uint8_t val, int tact);

    uint8_t* restoreState(uint8_t* ptr) { return ptr; }
    uint8_t* saveState(uint8_t* ptr) { return ptr; }
    void updateProps() { }
    void reset() { }

    uint8_t is_readonly(int num, int value = -1) { return 1; }
    void eject(int num) { }
    uint8_t read_sector(int num, int sec) { return 0; }
    uint8_t  open(const char* path, int num, int type) { return zxFormats::openTRD(path); }
    uint8_t  save(const char* path, int num, int type) { return 0; }

    void process();
    void find_marker();
    char notready();
    void load();
    void getindex();
    //void trdos_traps();

    void cmdVerify();
    void cmdSeek();
    void cmdStep();
    void cmdType1();
    void cmdWriteTrackData();
    void cmdWriteTrack();
    void cmdWrite();
    void cmdWriteSec();
    void cmdRead();
    void cmdFindSec();
    void cmdPrepare();
    void cmdReadWrite();

    int foundid;
    int rwptr, rwlen;
    int start_crc;
    int tshift;
    int drive, side;

    uint8_t state, state2, cmd;
    uint8_t data, track, sector;
    uint8_t rqs, status;
    uint8_t system;
    int8_t stepdirection;

    uint64_t end_waiting_am;
    uint64_t next, time;

    TRKCACHE trkcache;
    FDD *seldrive;
    FDD fdd[4];
};
