//
// Created by Sergey on 06.01.2020.
//

#pragma once

constexpr int PORT_CMD  = 0x1F;
constexpr int PORT_TRK  = 0x3F;
constexpr int PORT_SEC  = 0x5F;
constexpr int PORT_DAT  = 0x7F;
constexpr int PORT_SYS  = 0xFF;

class zxDisk {
public:
    struct rs_type {
        unsigned b0 : 1;// занят выполнением комманды
        unsigned b1 : 1;
        unsigned b2 : 1;
        unsigned b3 : 1;
        unsigned b4 : 1;
        unsigned b5 : 1;
        unsigned b6 : 1;
        unsigned b7 : 1;// дисковод готов к работе
    };

    union unk {
        uint8_t byte;
        rs_type bit;
    };
    struct DISK {
        DISK(): track(0), ro(0) { memset(filename, 0, sizeof(filename)); }
        // имя файла диска
        char filename[256];
        // текущий трек ???
        uint8_t track;
        // ??
        uint8_t ro;
    };

    zxDisk() : cmd_done(0), spin(0), posRead(0), lenRead(0), posWrite(0), lenWrite(0), side(0), dir(0), bufferData(nullptr) { }
    ~zxDisk() { reset(); }

    // открыть диск
    bool openTRD(int num, const char *path);

    // обновить свойства
    void updateProps();

    // сброс
    void reset();

    // записать в порт
    void writePort(uint8_t A0A7, uint8_t value);

    // прочитать из порта
    uint8_t readPort(uint8_t A0A7);

    // сохранить состояние
    uint8_t* saveState(uint8_t* ptr);

    // восстановить состояние
    bool loadState(uint8_t* ptr);

protected:

    // задержка в поиске
    void seekDelay(uint8_t dst_track);

    // вернуть текущий диск
    DISK& currentDisk();

    // установить флаги поиска
    void setFlagsSeeks();

    // записать параметры комманды
    void writeCommand(uint8_t val);

    // записать данные на диск
    void writeData(uint8_t val);

    // вернуть признак отсутствия диска
    bool isDiskNoReady() { return currentDisk().filename[0] == 0; }

    // прочитать данные диска
    uint8_t readData();

    // время на заверщения команды
    long long cmd_done;

    // признак вращения диска(при операциях с головкой)
    int spin;

    // сторона диска
    int side;

    // направление операции(например, поиска сектора)
    int dir;

    // позиция чтения данных/длина данных
    int posRead, lenRead;

    // позиция записи данных/длина данных
    int posWrite, lenWrite;

    // буфер данных
    uint8_t* bufferData;

    // параметры дисков
    DISK disks[4];

    // статус операций
    unk vg_rs;

    // файл для операций
    zxFile file;
};
