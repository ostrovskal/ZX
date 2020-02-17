//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include "zxCommon.h"

extern "C" {
    uint8_t* realPtr(uint16_t address);
    void wm8(uint8_t* address, uint8_t val);
};

enum CPU_CYCLES {
    CT_EXE = 0,  // выполение команды
    CT_WAIT,     // задержка
    CT_OCR,      // (operation code read)    - чтение кода операции
    CT_CBR,      // (control uint8_t read)      - чтение управляющего байта кода операции (ED, DD, FD, CB)
    CT_IOP,      // (internal CPU-operation) - внутренние операции ЦП, выходные сигналы управления неактивны
    CT_MRD,      // (memory read)            - чтение байта из косвенно адресованной ячейки памяти
    CT_MRH,      // (memory rend, high uint8_t) - чтение старшего байта 16-разрядного слова из косвенно адресованной ячейки памяти
    CT_MRL,      // (memory read, low uint8_t)  - чтение младшего байта 16-разрядного слова из косвенно адресованной ячейки памяти
    CT_MWR,      // (memory write)           - запись байта в косвенно адресованную ячейку памяти
    CT_MWH,      // (memory write, high uint8_t)- запись старшего байта 16-разрядиого слова в косвенно адресованную ячейку памяти
    CT_MWL,      // (memory write, low uint8_t) - запись младшего байта 16-разрядного слова в косвенно адресованную ячейку памяти
    CT_ORD,      // (operand read)           - чтение операнда-байта, непосредственно представленного в команде
    CT_ORH,      // (operand read, high uint8_t)- чтение старшего байта 16-разрядного слова, непосредственно представленного в команде
    CT_ORL,      // (operand read, Ion uint8_t) - чтение младшего байта 16-разрядного слова, непосредственно представленного в команде
    CT_PRD,      // (port read)              - ввод данных из порта
    CT_PWR,      // (port write)             - вывод данных в порт
    CT_SRH,      // (stack read, high uint8_t)  - чтение из стека старшего байта
    CT_SRL,      // (stack read, low uint8_t)   - чтение из стека младшего байта
    CT_SWH,      // (stack write, high uint8_t) - запись в стек старшего байта
    CT_SWL,      // (ataok write, low uint8_t)  - запись в стек младшего байта
    CT_INTA,     // (interrupt .acknowledge) - подтверждение прерывания микропроцессором
    CT_SPI,      // (stack pointer increment)- инкремент указателя стека в конце цикла
    CT_SPD       // (atack pointer deorement)- декремент указателя стека в конце цикла
};

enum MNEMONIC_REGS {
    _RC, _RE, _RL, _RF, _RS, _RB, _RD, _RH, _RA, _RI, _RR, _RPHL,
    // 12
    _N_,
    // 13
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
    PORT_FE, PORT_1FFD, PORT_7FFD,
    RAM, ROM, VID,

    // 35
    AY_REG,
    AY_AFINE, AY_ACOARSE, AY_BFINE, AY_BCOARSE, AY_CFINE, AY_CCOARSE, AY_NOISEPER, AY_ENABLE, AY_AVOL,
    AY_BVOL, AY_CVOL, AY_EFINE, AY_ECOARSE, AY_ESHAPE, AY_PORTA, AY_PORTB, AY_BEEPER,
    // 53
    TRDOS_HEAD, TRDOS_TRK, TRDOS_SEC, TRDOS_DAT, TRDOS_STS, TRDOS_CMD, TRDOS_SYS,
    // 60
    MOUSE_K, MOUSE_X, MOUSE_Y,
    RTMP,
    // 64
    STATE,
    CALL0, CALL1,
    // 67
    RUS_LAT,
    MODEL,
    // 69
    COUNT_REGS
};

#pragma clang diagnostic pop

enum MNEMONIC_OPS {
    O_ADD, O_SUB, O_ADC, O_SBC, O_DEC, O_OR, O_INC, O_XOR, O_AND, O_CP,
    O_PUSH, O_POP,
    O_RST, O_ASSIGN, O_LOAD, O_SAVE,
    O_ROT, O_PREF,
    O_IM, O_NEG,
    // 20
    O_EX,
    // 21
    O_SPEC,
    // 22
    O_IN, O_OUT,
    // 24
    O_ROTX, O_SET, O_RES, O_BIT,
    // 28
    O_JMP, O_JR, O_CALL, O_RET, O_RETN,
    // 33
    O_LDI, O_CPI, O_INI, O_OTI,
    O_UNDEF
};

class zxCPU {
public:
    zxCPU();

    // выполнение операции
    int step();

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

    void shutdown() { }

    // стандартные операции
    void opsStd();

    // XOR/OR/AND
    void opsLogic();

    // ADC/SBC
    void opsAdcSbc();

    // ADD/SUB
    void opsAddSub();

    // блочные операции
    void opsBlock();

    // JMP/CALL/JR/RST/RET/RETN
    void opsJump();

    // IN/OUT
    void opsPort();

    // операции обменая регистров
    void opsExchange();

    // специальные операции
    void opsSpecial();

    // битовые операции
    void opsBits();

    // вызов подпрограммы
    int call(uint16_t address);

protected:

    inline void incrementR() {
        auto r = *_R; *_R = (uint8_t)((r & 128) | (((r + 1) & 127)));
    }

    // инициализация операнда
    uint8_t* initOperand(uint8_t o, uint8_t oo, int prefix, uint16_t& v16, uint8_t& v8);

    // выполнение сдвига
    void rotate(uint8_t value);

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
    void wm8(uint16_t address, uint8_t val);

    // код операции
    int codeOps;

    // такты
    int ticks;

    // обработанные флаги
    uint8_t execFlags;

    // значения установленных флагов
    int setFlags;

    // результат операций
    uint8_t res;

    // время обработки
    u_long ts_next;
};

typedef void (zxCPU::*fnOps)();
