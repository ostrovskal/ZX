//
// Created by Sergey on 06.01.2020.
//

#pragma once

class zxDisk {
public:
    zxDisk() { }
    ~zxDisk() { }

    bool openTRD(const char *path);

    void updateProps();

    void reset();

    void write(uint8_t value);

    void writePort(uint8_t A0A7, uint8_t value);
};
