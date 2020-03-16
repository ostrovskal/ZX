//
// Created by Sergey on 10.02.2020.
//

#pragma once

constexpr int TZX_NORMAL_BLOCK         = 0x10;
constexpr int TZX_TURBO_BLOCK          = 0x11;
constexpr int TZX_PURE_TONE            = 0x12;
constexpr int TZX_SEQ_PULSES_DIFF_LEN  = 0x13;
constexpr int TZX_PURE_DATA_BLOCK      = 0x14;
constexpr int TZX_DIRECT_RECORD        = 0x15;
constexpr int TZX_PAUSE                = 0x20;
constexpr int TZX_GROUP_START          = 0x21;
constexpr int TZX_GROUP_END            = 0x22;
constexpr int TZX_JUMP                 = 0x23;
constexpr int TZX_LOOP_START           = 0x24;
constexpr int TZX_LOOP_END             = 0x25;
constexpr int TZX_CALL                 = 0x26;
constexpr int TZX_RETURN               = 0x27;
constexpr int TZX_SELECT_BLOCK         = 0x28;
constexpr int TZX_STOP_IF_48K          = 0x2A;
constexpr int TZX_TEXT                 = 0x30;
constexpr int TZX_MESSAGE              = 0x31;
constexpr int TZX_ARCHIVE_INFO         = 0x32;
constexpr int TZX_HARDWARE_TYPE        = 0x33;
constexpr int TZX_EMULATION_INFO       = 0x34;
constexpr int TZX_CUSTOM_INFO          = 0x35;
constexpr int TZX_SNAPSHOT             = 0x40;
constexpr int TZX_Z                    = 0x5A;

class zxFormats {
public:
    // образы памяти
    static bool openZ80(uint8_t* ptr, size_t size);
    static uint8_t* saveZ80();
    // лента
    static bool openTAP(zxDevTape* tape, uint8_t* ptr, size_t size);
    static bool openTZX(zxDevTape* tape, uint8_t* ptr, size_t size);
    static bool openCSW(zxDevTape* tape, uint8_t* ptr, size_t size);
    static uint8_t* saveTAP(zxDevTape* tape);
    static uint8_t* saveTZX(zxDevTape* tape);
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
