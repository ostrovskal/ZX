//
// Created by Sergey on 26.12.2019.
//

#pragma once

class zxTape {
public:
    zxTape() {}

    void control(int ticks);

    int trap(uint16_t PC);

    bool openWAV(const char *path);

    bool openTAP(const char *path);

    void loadState(uint8_t *ptr);

    uint8_t *saveState(uint8_t *ptr);

    bool saveWAV(const char *path);

    bool saveTAP(const char *path);

    void updateProps();

    void reset();

    uint8_t _MIC;
    uint8_t _BEEP;

    void writePort(uint8_t value);
};
