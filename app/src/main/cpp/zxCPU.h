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
extern uint8_t tbl_parity[256];

#define DA(token)   codes[pos++] = token
//#define DA2(token)  {codes[pos++] = token & 255; codes[pos++] = token >> 8; }

enum MNEMONIC_REGS {
    _RC, _RB, _RE, _RD, _RL, _RH, _RR, _RA,
    _RNONE, _RPHL,
    _RI, _RBC, _RDE, _RHL, _RAF, _RSP,
};

enum MNEMONIC_FLAGS {
    _NZ = 1, _Z, _NC, _C, _PO, _PE, _P, _M
};

enum MNEMONIC_NAMES {
    C_C, C_B, C_E, C_D, C_L, C_H, C_R, C_A,
    C_NULL, C_PHL,
    C_I, C_BC, C_DE, C_HL, C_AF, C_SP,
    C_FNZ, C_FZ, C_FNC, C_FC, C_FPO, C_FPE, C_FP, C_FM,
    C_XH, C_XL, C_YH, C_YL, C_PIX, C_PIY, C_F, C_IX, C_IY, C_IM,
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
    C_COMMA, C_0, C_PC, C_NN, C_PNN
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
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
    MODEL,
    COUNT_REGS
};
#pragma clang diagnostic pop

enum MNEMONIC_OPS {
    O_ADD, O_ADC, O_SUB, O_SBC, O_DEC, O_OR, O_INC, O_XOR, O_AND, O_CP,
    O_PUSH, O_POP, O_JR, O_CALL, O_RET, O_RST, O_RETN, O_RETI,
    O_ASSIGN, O_LOAD, O_SAVE,
    O_SPEC, O_NONI, O_ROT, O_REP, O_PREFIX,
    O_IN, O_OUT,
    O_IM, O_NEG, O_RLD, O_RRD,
    // 32
    O_ROT_CB, O_SET, O_RES, O_BIT,
    O_JMP = 64// передача управления
};

enum MNEMONIC_CONRROL {
    C_8BIT = 32, C_16BIT = 64
};

enum CPU_FLAGS {
    FC = 1, FN = 2, FPV = 4, F3 = 8, FH = 16, F5 = 32, FZ = 64, FS = 128
};

#define _FS(val)				(uint8_t)(val & 128)
#define _FZ(val)				(uint8_t)((val == 0) << 6)
#define _FV1(op1, res)          (uint8_t)(res ? ((op1 ^ res) & 0x80) >> 5 : 0)
#define _FH(op1, op2, fc, fn)	calcFH(op1, op2, fc, fn)
#define _FP(val)				tbl_parity[val]

class zxCPU {
public:

    struct MNEMONIC {
        uint8_t regs;
        uint8_t ops;
        uint8_t tiks;
        uint8_t name;
        uint8_t flgs;
    };

    zxCPU();

    // выполнение операции
    int step(int prefix, int offset);

    // запрос на маскируемое прерывание
    int signalINT();

    // запрос на немаскируемое прерывание
    int signalNMI();

    // формирование дизассемблерной строки инструкции
    std::string daMakeMnemonic(MNEMONIC* m, int prefix, int code, uint16_t pc, uint16_t vDst, uint16_t vSrc, uint8_t v8Dst, uint8_t v8Src);

    // триггеры
    uint8_t* _IFF1, *_IFF2;

    // регистры
    uint8_t *_A, *_B, *_C, *_F, *_I, *_R, *_IM;
    uint16_t* _BC, *_DE, *_HL, *_AF, *_SP, *_IX, *_IY, *_PC;

protected:

    uint8_t*  rRON[24];
    uint16_t* rRP[24];

    inline void incrementR() {
        auto r = *_R; *_R = (uint8_t)((r & 128) | (((r + 1) & 127)));
    }

    // вызов подпрограммы
    int call(uint16_t address);

    // недействительная операция
    int noni();

    // инициализация операнда
    uint8_t * initOperand(uint8_t o, uint8_t oo, int prefix, uint16_t& v16, uint8_t& v8, int* ticks);

    // проверить на наличие флага
    bool isFlag(uint8_t num) { return (((*_F) & flags_cond[num]) ? 1 : 0) == (num & 1); }

    // получить значение флага
    inline uint8_t getFlag(uint8_t fl) { return (uint8_t)((*_F) & fl); }

    // установка флагов по маске
    inline void uflags(int msk, uint8_t val) { *_F = (uint8_t)(((*_F) & ~msk) | val); }

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

    // код операции
    int codeOps, codeExOps;

    // специальная операция
    uint8_t opsSpec(uint8_t v8Dst, uint8_t v8Src, uint16_t vDst, uint16_t vSrc, uint8_t* dst);

    // выполенение сдвига
    uint8_t rotate(uint8_t value, uint8_t* nfl);

    // выполнение повторяющихся операций
    uint8_t opsRep(uint8_t fl, int* ticks);
};
