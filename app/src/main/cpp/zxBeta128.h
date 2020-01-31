//
// Created by Sergey on 30.01.2020.
//

#pragma once

#include "zxFile.h"

enum REGS {
    R_STS, R_TRK, R_SEC, R_DAT, R_CMD
};
enum CMD {
    C_IDLE, C_DELAY, C_STATUS, C_STEP, C_READ_ADDRESS, C_READ_SECTOR, C_WRITE_SECTOR, C_READ_TRACK, C_WRITE_TRACK
};

#define stsBusy             1
#define stsDRQ              2
#define stsImpulse          2
#define stsLostData         4
#define stsTrack0           4
//#define stsVerify           8
//#define stsSeek             16
#define stsLoadHead         32
#define stsWriteProtect     64
//#define stsNotReady         128

#define reqINTRQ            128
#define reqDRQ              64

#define STEP_TIME           200
#define HEAD_TIME           15
#define LOST_DATA_TIMEOUT   25

class zxBeta128 {
public:
    struct DISK {
        DISK() : isOpen(false), write(0), track(0), nsec(0), ntrk(0), nhead(0), nsize(0) { }
        // установка позиции чтения/записи
        void setPos(uint8_t trk, uint8_t head, uint8_t sec);
        // признак открытого файла
        bool isOpen;
        // количество секторов, дорожек, головок и байт в секторе
        uint8_t nsec, ntrk, nhead, nsize;
        // текущая дорожка
        uint8_t track;
        // защита от записи
        uint8_t write;
        // имя файла
        std::string path;
        // файл
        zxFile file;
    };
    zxBeta128() : ndsk(-1) { reset_controller(); }
    ~zxBeta128() { }
    // монтировать образ диска
    bool openTRD(int active, const char* path);
    // сброс
    void reset() { }
    // запись в контроллер
    void vg93_write(uint8_t addr, uint8_t val);
    // чтение из контроллера
    uint8_t vg93_read(uint8_t addr);
    // восстановить состояние
    uint8_t *loadState(uint8_t* ptr);
    // сохранить состояние
    uint8_t* saveState(uint8_t* ptr);
protected:
    void status();
    // сброс контроллера
    void reset_controller();
    // установка системных переменных
    void write_sys(uint8_t val);
    // выполнение команды
    void cmd_exec(uint8_t cmd);
    // признак готовности диска
    bool ready() { return curDisk()->isOpen; }
    // текущий диск
    DISK* curDisk() { return &disks[ndsk]; }

    // регистры
    uint8_t regs[5];
    // направление шага
    uint8_t sdir;
    // запрос по адресу 0xFF
    uint8_t req;
    // текущая головка
    uint8_t head;
    // блокировка
    uint8_t hlt;
    // режим диска
    uint8_t mfm;
    // время для задержек
    u_long time;
    // все диски
    DISK disks[4];
    // индекс в буфере данных
    int index;
    // команда
    int cmd;
    // чтение/запись нескольких секторов
    int sec_mult;
    // текущий диск
    int ndsk;
    // задержка
    int delay;
    // состояние
    int state, nextState;
    // для форматирования
    int formatCounter;
};
