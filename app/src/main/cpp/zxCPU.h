//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include "zxCommon.h"

extern "C" {
    uint8_t* realPtr(uint16_t address);
    void wm8(uint8_t* address, uint8_t val);
};

extern uint8_t flags_cond[8];

enum MNEMONIC_REGS {
    _RC, _RE, _RL, _RF, _RS, _RB, _RD, _RH, _RA, _RI, _RR, _RPHL,
    // 12
    _N_, _BT,
    // 14
    _C8, _C16,
    // 16
    _R16 = 16
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

enum CPU_FLAGS {
    FC = 1, FN = 2, FPV = 4, F3 = 8, FH = 16, F5 = 32, FZ = 64, FS = 128
};

enum CPU_REGS {
    RC, RB, RE, RD, RL, RH, RF, RA,
    RC_, RB_, RE_, RD_, RL_, RH_, RF_, RA_,
    RXL, RXH, RYL, RYH,
    RSPL, RSPH, RPCL, RPCH,
    RI, RR, IFF1, IFF2, IM,
    STATE,
    PORT_FE, PORT_1F, PORT_FD,
    FE16, FE8,
    RAM, ROM, VID,
    TICK0, TICK1, TICK2, TICK3,
    AY_REG,
    AY_AFINE, AY_ACOARSE, AY_BFINE, AY_BCOARSE, AY_CFINE, AY_CCOARSE, AY_NOISEPER, AY_ENABLE, AY_AVOL,
    AY_BVOL, AY_CVOL, AY_EFINE, AY_ECOARSE, AY_ESHAPE, AY_PORTA, AY_BEEPER,
    TRDOS_CMD, TRDOS_TRK, TRDOS_SEC, TRDOS_DAT, TRDOS_SYS,
    RTMP,
    MODEL,
    COUNT_REGS
};

enum DA_MNEMONIC_NAMES {
    C_C, C_B, C_E, C_D, C_L, C_H, C_R, C_A,
    C_NULL, C_PHL,
    C_I, C_BC, C_DE, C_HL, C_AF, C_SP,
    C_FNZ, C_FZ, C_FNC, C_FC, C_FPO, C_FPE, C_FP, C_FM,
    C_XL, C_XH, C_YL, C_YH, C_PIX, C_PIY, C_F, C_IX, C_IY, C_IM,
    C_PBC, C_PDE,
    C_NOP, C_EX_AF, C_DJNZ, C_JR,
    C_RLCA, C_RRCA, C_RLA, C_RRA, C_DAA, C_CPL, C_SCF, C_CCF,
    C_DI, C_EI,
    C_ADD, C_ADC, C_SUB, C_SBC, C_AND, C_XOR, C_OR, C_CP,
    C_RLC, C_RRC, C_RL, C_RR, C_SLA, C_SRA, C_SLI, C_SRL,
    C_BIT, C_RES, C_SET,
    C_INC, C_DEC,
    C_RRD, C_RLD,
    C_LDI, C_CPI, C_INI, C_OTI,
    C_LDD, C_CPD, C_IND, C_OTD,
    C_LDIR, C_CPIR, C_INIR, C_OTIR,
    C_LDDR, C_CPDR, C_INDR, C_OTDR,
    C_EXX, C_EX_DE, C_EX_SP, C_LD, C_JP, C_CALL,
    C_RET, C_RETI, C_RETN, C_RST, C_PUSH, C_POP,
    C_HALT, C_NEG, C_IN, C_OUT, C_IX_NONI, C_IY_NONI, C_ED_NONI,
    C_COMMA, C_0, C_PC, C_SRC, C_DST, C_CB_PHL,
    C_N, C_NN, C_PNN, C_NUM
};

#pragma clang diagnostic pop

enum MNEMONIC_FLAGS {
    _NZ = 1, _Z, _NC, _C, _PO, _PE, _P, _M
};

enum MNEMONIC_OPS {
    O_ADD, O_SUB, O_ADC, O_SBC, O_DEC, O_OR, O_INC, O_XOR, O_AND, O_CP,
    O_PUSH, O_POP,
    O_RST, O_RETN, O_EX,
    O_ASSIGN, O_LOAD, O_SAVE,
    O_SPEC, O_ROT, O_PREF,
    O_IN, O_OUT,
    O_IM, O_NEG,
    // 25
    O_SET, O_RES, O_BIT,
    O_JMP, O_JR, O_CALL, O_RET,
    // 32
    O_LDI, O_CPI, O_INI, O_OTI
};

#define _FC16(val)              fc  = (uint8_t)((val) > 65535)
#define _FP(val)				fpv = tbl_parity[(uint8_t)(val)]
#define _FH(x, y, z, c, n)	    fh  = hcarry((uint8_t)(x), (uint8_t)(y), (uint8_t)(z), (uint8_t)(c), (uint8_t)(n))
#define _FZ(val)                fz  = (uint8_t)((val) == 0)
#define _FS(val)                fs  = (uint8_t)((val) >> 7)
#define _FC(val)                fc  = (uint8_t)((val) > 255)
#define _FV(x, y, z, c, n)      fpv = overflow((uint8_t)(x), (uint8_t)(y), (uint8_t)(z), (uint8_t)(c), (uint8_t)(n))

class zxCPU {
public:

    struct MNEMONIC {
        uint8_t regDst;
        uint8_t regSrc;
        uint8_t ops;
        uint8_t tiks;
        uint8_t name;
        uint8_t flags;
    };

    zxCPU();

    // выполнение операции
    int step(int prefix, int offset);

    // запрос на маскируемое прерывание
    int signalINT();

    // запрос на немаскируемое прерывание
    int signalNMI();

    // триггеры
    uint8_t* _IFF1, *_IFF2;

    // регистры
    uint8_t *_A, *_B, *_C, *_F, *_I, *_R, *_IM;
    uint16_t* _BC, *_DE, *_HL, *_AF, *_SP, *_IX, *_IY, *_PC;

    // адреса регистров
    static uint8_t*  regs[36];

protected:

    inline void incrementR() {
        auto r = *_R; *_R = (uint8_t)((r & 128) | (((r + 1) & 127)));
    }

    // вызов подпрограммы
    int call(uint16_t address);

    // инициализация операнда
    uint8_t * initOperand(uint8_t o, uint8_t oo, int prefix, uint16_t& v16, uint8_t& v8, int* ticks);

    // читаем 16 бит из PC
    inline uint16_t rm16PC() { return (rm8PC() | rm8PC() << 8); }

    // читаем 8 бит из PC
    inline uint8_t rm8PC() { return *realPtr((*_PC)++); }

    // пишем в память 16 бит
    inline void wm16(uint16_t address, uint16_t val) {
        wm8(address, (uint8_t)(val & 0xff));
        wm8((uint16_t)(address + 1), (uint8_t)(val >> 8));
    }

    // пишем в память 8 бит
    void wm8(uint16_t address, uint8_t val) { ::wm8(realPtr(address), val); }

    // проверить на наличие флага
    bool isFlag(uint8_t num) { return ((*_F & flags_cond[num]) ? 1 : 0) == (num & 1); }

    // получить значение флага
    inline uint8_t getFlag(uint8_t fl) { return (uint8_t)(*_F & fl); }

    // код операции
    int codeOps;

    // специальная операция
    void special(uint16_t vSrc, uint8_t v8Dst, uint8_t v8Src);

    // выполенение сдвига
    uint8_t rotate(uint8_t value, bool isCB);

    // флаги
    uint8_t fc, fn, fpv, f3, fh, f5, fz, fs;

    // спец. флаги
    uint8_t flags;

    // результат операций
    uint8_t res;

    // значение флага FPV
    bool resetPV;

    zxFile f;

    // значение FC для установки
    uint16_t  _fc;
};
