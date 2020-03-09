//
// Created by Sergey on 10.02.2020.
//

#pragma once

class zxFormats {
public:
    // образы памяти
    static bool openSnapshot(uint8_t *ptr, size_t size, int type);
    static bool openZ80(uint8_t* ptr, size_t size);
    static bool openSNA(uint8_t* ptr, size_t size);
    static uint8_t* saveZ80();
    static uint8_t* saveSNA();
    static uint8_t* saveSnapshot(int type);
    // лента
    static bool openTAP(zxDevTape* tape, uint8_t* ptr, size_t size);
    static bool openTZX(zxDevTape* tape, uint8_t* ptr, size_t size);
    static bool openCSW(zxDevTape* tape, uint8_t* ptr, size_t size);
    static uint8_t* saveTAP(zxDevTape* tape);
    static uint8_t* saveTZX(zxDevTape* tape);
    static uint8_t* saveRZX(zxDevTape* tape);
    static uint8_t* saveCSW(zxDevTape* tape);
    // диск
    static bool openTRD(zxFDD* fdd, uint8_t* ptr, size_t size);
    static bool openSCL(zxFDD* fdd, uint8_t* ptr, size_t size);
    static bool openFDI(zxFDD* fdd, uint8_t* ptr, size_t size);
    static bool openUDI(zxFDD* fdd, uint8_t* ptr, size_t size);
    static bool openTD0(zxFDD* fdd, uint8_t* ptr, size_t size);
    static uint8_t* saveTRD(zxFDD* fdd);
    static uint8_t* saveSCL(zxFDD* fdd);
    static uint8_t* saveFDI(zxFDD* fdd);
    static uint8_t* saveUDI(zxFDD* fdd);
    static uint8_t* saveTD0(zxFDD* fdd);
};
