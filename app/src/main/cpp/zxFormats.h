//
// Created by Sergey on 10.02.2020.
//

#pragma once

class zxFormats {
public:
    // образы памяти
    static bool openZ80(const char *path);
    static bool saveZ80(const char *path);
    static bool openSNA(const char *path);
    static bool saveSNA(const char *path);
    // лента
    static bool openTAP(const char *path);
    static bool saveTAP(const char *path);
    static bool openTZX(const char *path);
    static bool saveTZX(const char *path);
    static bool openRZX(const char *path);
    static bool saveRZX(const char *path);
    static bool openWAV(const char *path);
    static bool saveWAV(const char *path);
    // диск
    static bool openTRD(const char *path);
    static bool saveTRD(const char *path);
    static bool openSCL(const char *path);
    static bool saveSCL(const char *path);
    static bool openFDI(const char *path);
    static bool saveFDI(const char *path);
    static bool openUDI(const char *path);
    static bool saveUDI(const char *path);
    static bool openTD0(const char *path);
    static bool saveTD0(const char *path);
};
