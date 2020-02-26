//
// Created by Sergey on 19.02.2020.
//

#include "zxCommon.h"
#include "zxMagnitophon.h"
#include "zxFormats.h"

zxMagnitophon::zxMagnitophon() {
    max_pulses = 0;
    tape_err = 0;
    tape_image = nullptr;
    tapeinfo = nullptr;
    tape_imagesize = 0;
    tape_infosize = 0;
    appendable = 0;
}

void zxMagnitophon::stopTape() {
    findTapeIndex();
    if(tape.play_pointer >= tape.end_of_tape) tape.index = 0;
    tape.play_pointer = 0;
    tape.edge_change = 0x7FFFFFFFFFFFFFFFLL;
    tape.tape_bit = 0x40;
    //speccy->CPU()->HandlerStep(nullptr);
}

void zxMagnitophon::resetTape() {
    tape.index = 0;
    tape.play_pointer = 0;
    tape.edge_change = 0x7FFFFFFFFFFFFFFFLL;
    tape.tape_bit = 0x40;
    //speccy->CPU()->HandlerStep(NULL);
}

void zxMagnitophon::startTape() {
    if(!tape_image) return;
    tape.play_pointer = tape_image + tapeinfo[tape.index].pos;
    tape.end_of_tape = tape_image + tape_imagesize;
    tape.edge_change = *zxDevUla::_TICK;
    tape.tape_bit = 0x40;
//	speccy->CPU()->FastEmul(FastTapeEmul);
}

void zxMagnitophon::closeTape() {
    SAFE_A_DELETE(tape_image);
    SAFE_DELETE(tapeinfo);
    tape_err = max_pulses = tape_imagesize = tape_infosize = 0;
    tape.play_pointer   = 0;                    // stop tape
    tape.index          = 0;                    // rewind tape
    tape.edge_change    = 0x7FFFFFFFFFFFFFFFLL;
    tape.tape_bit       = 0x40;
}

void zxMagnitophon::reserve(uint32_t datasize) {
    // выделить с нуля/перевыделить память под образ блоками по 16384 байт
    const int blocksize = 16384;
    auto newsize = align_by(datasize + tape_imagesize + 1, blocksize);
    if(!tape_image) tape_image = new uint8_t[newsize];
    auto nsize = align_by(tape_imagesize, blocksize);
    if(nsize < newsize) {
        auto ti = tape_image;
        tape_image = new uint8_t[newsize];
        memcpy(tape_image, ti, nsize);
    }
}

void zxMagnitophon::findTapeIndex() {
    for(uint32_t i = 0; i < tape_infosize; i++) {
        if (tape.play_pointer >= tape_image + tapeinfo[i].pos)
            tape.index = i;
    }
}

void zxMagnitophon::findTapeSizes() {
    for(uint32_t i = 0; i < tape_infosize; i++) {
        auto end = (i == tape_infosize - 1) ? tape_imagesize : tapeinfo[i + 1].pos;
        uint32_t sz = 0;
        for(uint32_t j = tapeinfo[i].pos; j < end; j++) sz += tape_pulse[tape_image[j]];
        tapeinfo[i].t_size = sz;
    }
}

uint32_t zxMagnitophon::findPulse(uint32_t t) {
    uint32_t i;
    if(max_pulses < 256) {
        for(i = 0; i < max_pulses; i++) {
            if (tape_pulse[i] == t) return i;
        }
        tape_pulse[max_pulses] = t;
        return max_pulses++;
    }
    // если массив импульсов переполнен, тогда ищем ближайший импульс
    // нахрена - разве может быть более 256 разных импульсов на одну прогу???
    tape_err = 1;
    uint32_t nearest = 0;
    int delta = 0x7FFFFFFF;
    for(i = 0; i < 256; i++) {
        auto tt = abs((int)(t - tape_pulse[i]));
        if (delta > tt) nearest = i, delta = tt;
    }
    return nearest;
}

//        MakeBlock(ptr, size, 2168, 667, 735, 855, 1710, (*ptr < 4) ? 8064 : 3220, 1000);
void zxMagnitophon::makeBlock(uint8_t* data, uint32_t size, uint32_t pilot_t, uint32_t s1_t, uint32_t s2_t, uint32_t zero_t,
                uint32_t one_t, uint32_t pilot_len, uint32_t pause, uint8_t last) {
    // каждый байт расширяем в 16 раз(дважды по 8 бит???) + пилот + s1_t + s2_t + pause
    reserve(size * 16 + pilot_len + 3);
    // пилотные импульсы
    if(pilot_len != 0xFFFFFFFF) {
        auto t = (uint8_t)findPulse(pilot_t);
        for(uint32_t i = 0; i < pilot_len; i++) tape_image[tape_imagesize++] = t;
        // задержка после пилота
        tape_image[tape_imagesize++] = (uint8_t)findPulse(s1_t);
        tape_image[tape_imagesize++] = (uint8_t)findPulse(s2_t);
    }
    // длина на 0-й бит(855) и на 1-й бит(1710)
    auto t0 = (uint8_t)findPulse(zero_t), t1 = (uint8_t)findPulse(one_t);
    for(; size > 1; size--, data++) {
        for(uint8_t j = 128; j; j >>= 1) {
            auto idxPulse = (*data & j) ? t1 : t0;
            tape_image[tape_imagesize++] = idxPulse;
            tape_image[tape_imagesize++] = idxPulse;
        }
    }
    // последний байт
    for(uint8_t j = 128; j != (uint8_t)(128 >> last); j >>= 1) {
        auto idxPulse = (*data & j) ? t1 : t0;
        tape_image[tape_imagesize++] = idxPulse;
        tape_image[tape_imagesize++] = idxPulse;
    }
    if(pause) tape_image[tape_imagesize++] = (uint8_t)findPulse(pause * 3500);
}


uint8_t zxMagnitophon::tapeBit() {
    uint32_t cur = *zxDevUla::_TICK;
    while(cur > tape.edge_change) {
        uint32_t pulse;
        tape.tape_bit ^= 0x40;
        if(tape.play_pointer >= tape.end_of_tape || (pulse = tape_pulse[*tape.play_pointer++]) == 0xFFFFFFFF)
            stopTape();
        else
            tape.edge_change += pulse;
    }
    return tape.tape_bit;
}

bool zxMagnitophon::open(uint8_t* ptr, size_t size, int type) {
    switch(type) {
        case ZX_CMD_IO_TAP:
            return zxFormats::openTAP(this, ptr, size);
        case ZX_CMD_IO_TZX:
            return zxFormats::openTZX(this, ptr, size);
        case ZX_CMD_IO_CSW:
            return zxFormats::openCSW(this, ptr, size);
        case ZX_CMD_IO_WAV:
            return zxFormats::openWAV(this, ptr, size);
    }
    return false;
}

void zxMagnitophon::allocInfocell() {
    tapeinfo = (TAPEINFO*)realloc(tapeinfo, (tape_infosize + 1) * sizeof(TAPEINFO));
    tapeinfo[tape_infosize].pos = tape_imagesize;
    appendable = 0;
}

void zxMagnitophon::namedCell(const void* nm, uint32_t sz) {
    allocInfocell();
    if(sz) memcpy(tapeinfo[tape_infosize].desc, nm, sz), tapeinfo[tape_infosize].desc[sz] = 0;
    else strcpy(tapeinfo[tape_infosize].desc, (const char*)nm);
    tape_infosize++;
}

void zxMagnitophon::desc(const uint8_t * data, uint32_t size, char* dst) {
    uint8_t crc = 0;
    char prg[10];
    uint32_t i;
    for(i = 0; i < size; i++) crc ^= data[i];
    // если заголовок(для бейсика/кода)
    if(*data == 0 && size == 19 && (data[1] == 0 || data[1] == 3)) {
        for(i = 0; i < 10; i++) prg[i] = (data[i + 2] < ' ' || data[i + 2] >= 0x80) ? '?' : data[i + 2];
        for(i = 9; i && prg[i] == ' '; prg[i--] = 0);
        sprintf(dst, "%s: \"%s\" %d,%d", data[1] ? "Bytes" : "Program", prg, wordLE(data + 14), wordLE(data + 12));
    }
        // если блок, после заголовка
    else if(*data == 0xFF) sprintf(dst, "data block, %d bytes", size - 2);
        // если неизвестный блок
    else sprintf(dst, "#%02X block, %d bytes", *data, size - 2);
    sprintf(dst + strlen(dst), ", crc %s", crc ? "bad" : "ok");
}

void zxMagnitophon::createAppendableBlock() {
    if(tape_infosize == 0 || appendable) return;
    namedCell("set of pulses");
    appendable = 1;
}

void zxMagnitophon::parseHardware(uint8_t* ptr) {
    uint32_t n = *ptr++;
    if(!n) return;
    namedCell("- HARDWARE TYPE ");
    static const char ids[] = "computer\0"
                              "ZX Spectrum 16k\0"
                              "ZX Spectrum 48k, Plus\0"
                              "ZX Spectrum 48k ISSUE 1\0"
                              "ZX Spectrum 128k (Sinclair)\0"
                              "ZX Spectrum 128k +2 (Grey case)\0"
                              "ZX Spectrum 128k +2A, +3\0"
                              "Timex Sinclair TC-2048\0"
                              "Timex Sinclair TS-2068\0"
                              "Pentagon 128\0"
                              "Sam Coupe\0"
                              "Didaktik M\0"
                              "Didaktik Gama\0"
                              "ZX-81 or TS-1000 with  1k RAM\0"
                              "ZX-81 or TS-1000 with 16k RAM or more\0"
                              "ZX Spectrum 128k, Spanish version\0"
                              "ZX Spectrum, Arabic version\0"
                              "TK 90-X\0"
                              "TK 95\0"
                              "Byte\0"
                              "Elwro\0"
                              "ZS Scorpion\0"
                              "Amstrad CPC 464\0"
                              "Amstrad CPC 664\0"
                              "Amstrad CPC 6128\0"
                              "Amstrad CPC 464+\0"
                              "Amstrad CPC 6128+\0"
                              "Jupiter ACE\0"
                              "Enterprise\0"
                              "Commodore 64\0"
                              "Commodore 128\0"
                              "\0"
                              "ext. storage\0"
                              "Microdrive\0"
                              "Opus Discovery\0"
                              "Disciple\0"
                              "Plus-D\0"
                              "Rotronics Wafadrive\0"
                              "TR-DOS (BetaDisk)\0"
                              "Byte Drive\0"
                              "Watsford\0"
                              "FIZ\0"
                              "Radofin\0"
                              "Didaktik disk drives\0"
                              "BS-DOS (MB-02)\0"
                              "ZX Spectrum +3 disk drive\0"
                              "JLO (Oliger) disk interface\0"
                              "FDD3000\0"
                              "Zebra disk drive\0"
                              "Ramex Millenia\0"
                              "Larken\0"
                              "\0"
                              "ROM/RAM type add-on\0"
                              "Sam Ram\0"
                              "Multiface\0"
                              "Multiface 128k\0"
                              "Multiface +3\0"
                              "MultiPrint\0"
                              "MB-02 ROM/RAM expansion\0"
                              "\0"
                              "sound device\0"
                              "Classic AY hardware\0"
                              "Fuller Box AY sound hardware\0"
                              "Currah microSpeech\0"
                              "SpecDrum\0"
                              "AY ACB stereo; Melodik\0"
                              "AY ABC stereo\0"
                              "\0"
                              "joystick\0"
                              "Kempston\0"
                              "Cursor, Protek, AGF\0"
                              "Sinclair 2\0"
                              "Sinclair 1\0"
                              "Fuller\0"
                              "\0"
                              "mice\0"
                              "AMX mouse\0"
                              "Kempston mouse\0"
                              "\0"
                              "other controller\0"
                              "Trickstick\0"
                              "ZX Light Gun\0"
                              "Zebra Graphics Tablet\0"
                              "\0"
                              "serial port\0"
                              "ZX Interface 1\0"
                              "ZX Spectrum 128k\0"
                              "\0"
                              "parallel port\0"
                              "Kempston S\0"
                              "Kempston E\0"
                              "ZX Spectrum 128k +2A, +3\0"
                              "Tasman\0"
                              "DK'Tronics\0"
                              "Hilderbay\0"
                              "INES Printerface\0"
                              "ZX LPrint Interface 3\0"
                              "MultiPrint\0"
                              "Opus Discovery\0"
                              "Standard 8255 chip with ports 31,63,95\0"
                              "\0"
                              "printer\0"
                              "ZX Printer, Alphacom 32 & compatibles\0"
                              "Generic Printer\0"
                              "EPSON Compatible\0"
                              "\0"
                              "modem\0"
                              "VTX 5000\0"
                              "T/S 2050 or Westridge 2050\0"
                              "\0"
                              "digitaiser\0"
                              "RD Digital Tracer\0"
                              "DK'Tronics Light Pen\0"
                              "British MicroGraph Pad\0"
                              "\0"
                              "network adapter\0"
                              "ZX Interface 1\0"
                              "\0"
                              "keyboard / keypad\0"
                              "Keypad for ZX Spectrum 128k\0"
                              "\0"
                              "AD/DA converter\0"
                              "Harley Systems ADC 8.2\0"
                              "Blackboard Electronics\0"
                              "\0"
                              "EPROM Programmer\0"
                              "Orme Electronics\0"
                              "\0"
                              "\0";
    for(uint32_t i = 0; i < n; i++) {
        auto type_n = *ptr++;
        auto id_n   = *ptr++;
        auto value_n= *ptr++;
        const char* type = ids, *id, *value;
        uint32_t j; //Alone Coder 0.36.7
        for(/*dword*/j = 0; j < type_n; j++) {
            if(!*type) break;
            while(wordLE((uint8_t *)type)) type++;
            type += 2;
        }
        if(!*type) type = id = "??";
        else{
            id = type + strlen(type) + 1;
            for(j = 0; j < id_n; j++) {
                if(!*id) { id = "??"; break; }
                id += strlen(id) + 1;
            }
        }
        switch(value_n) {
            case 0: value = "compatible with"; break;
            case 1: value = "uses"; break;
            case 2: value = "compatible, but doesn't use"; break;
            case 3: value = "incompatible with"; break;
            default: value = "??";
        }
        char bf[512];
        sprintf(bf, "%s %s: %s", value, type, id);
        namedCell(bf);
    }
    namedCell("-");
}
