//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "stkMnemonic.h"
#include "zxCPU.h"
#include "zxDA.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

uint8_t*  zxCPU::regs[36];

static uint8_t tbl_parity[256];
static uint8_t flags_vals[9] = { 0, FZ, FZ, FC, FC, FPV, FPV, FS, FS };
static uint8_t flags_cond[9] = { 0, 0, 64, 0, 1, 0, 4, 0, 128 };
static uint8_t* dst(nullptr);
static uint8_t cb8, ops, mskFlags, fch;
static uint8_t regSrc, regDst;
static uint8_t v8Dst, v8Src;
static uint16_t vSrc, vDst;
static uint32_t n32;
static int prefix;
static MNEMONIC* m;

fnOps funcs[] = {
        &zxCPU::opsAddSub, &zxCPU::opsAddSub, &zxCPU::opsAdcSbc, &zxCPU::opsAdcSbc, &zxCPU::opsStd, &zxCPU::opsLogic,
        &zxCPU::opsStd, &zxCPU::opsLogic, &zxCPU::opsLogic, &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsJump,
        &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsStd, &zxCPU::opsStd,
        &zxCPU::opsExchange, &zxCPU::opsSpecial,
        &zxCPU::opsPort, &zxCPU::opsPort,
        &zxCPU::opsBits, &zxCPU::opsBits, &zxCPU::opsBits, &zxCPU::opsBits,
        &zxCPU::opsJump, &zxCPU::opsJump, &zxCPU::opsJump, &zxCPU::opsJump, &zxCPU::opsJump,
        &zxCPU::opsBlock, &zxCPU::opsBlock, &zxCPU::opsBlock, &zxCPU::opsBlock,
        &zxCPU::opsStd
};

/*
Целочисленное переполнение со знаком выражения x + y + c (где c равно 0 или 1) происходит тогда и только тогда,
когда x и y имеют одинаковый знак, а результат имеет знак, противоположный знаку операндов, и
Целочисленное переполнение со знаком выражения x - y - c (где c снова равно 0 или 1) происходит тогда и только тогда,
когда x и y имеют противоположные знаки, а знак результата противоположен знаку x (или, что эквивалентно, такой же как у у )
*/

// вычисление арифметического переполнения
inline uint8_t overflow(uint8_t x, uint8_t y, uint8_t z, uint8_t c, uint8_t n) {
    static uint8_t tbl[] = { 0, 4, 0, 0, 0, 0, 4, 0, 0, 0, 0, 4, 4, 0, 0, 0 };
    return tbl[((x >> 5) & 4) | (((y + c) >> 6) & 2) | (z >> 7) | (n << 2)];
}

// вычисление полупереноса
inline uint8_t hcarry(uint8_t x, uint8_t y, uint8_t z, uint8_t c, uint8_t n) {
    static uint8_t tbl[16] = { 0, 0, 16, 0, 16, 0, 16, 16, 0, 16, 16, 16, 0, 0, 0, 16 };
    return tbl[((x & 0x8) >> 1) | (((y + c) & 0x8) >> 2) | ((z & 0x8) >> 3) | (n << 2)];
}

// вычисление таблицы паритета
static void makeTblParity() {
    for(int a = 0; a < 256; a++) {
        uint8_t val = (uint8_t)a;
        val = (uint8_t)(((a >> 1) & 0x55) + (val & 0x55));
        val = (uint8_t)(((val >> 2) & 0x33) + (val & 0x33));
        val = (uint8_t)(((val >> 4) & 0x0F) + (val & 0x0F));
        tbl_parity[a] = (uint8_t)((!(val & 1)) << 2);
    }
}

// пишем в память 8 бит
void zxCPU::wm8(uint16_t address, uint8_t val) {
    if(checkSTATE(ZX_DEBUG)) { if(zx->checkBPs(address, ZX_BP_WMEM)) return; }
    ::wm8(realPtr(address), val);
}

zxCPU::zxCPU() {
    makeTblParity();

    _I = &opts[RI]; _R = &opts[RR]; _IM = &opts[IM];

    _IFF1 = &opts[IFF1]; _IFF2 = &opts[IFF2];

    _A = &opts[RA]; _F = &opts[RF]; _B = &opts[RB]; _C = &opts[RC];

    _BC = (uint16_t*)&opts[RC]; _DE = (uint16_t*)&opts[RE];   _HL = (uint16_t*)&opts[RL];
    _AF = (uint16_t*)&opts[RF]; _SP = (uint16_t*)&opts[RSPL]; _PC = (uint16_t*)&opts[RPCL];

    _IX = (uint16_t*)&opts[RXL]; _IY = (uint16_t*)&opts[RYL];

    static uint8_t regsTbl[] = { RC, RE, RL, RF, RSPL, RB, RD, RH, RA, RI, RR, _RPHL };

    for(int i = 0 ; i < 11; i++) {
        auto reg = &opts[regsTbl[i]];
        regs[i] = reg; regs[i + 12] = reg; regs[i + 24] = reg;
    }
    regs[14] = &opts[RXL]; regs[19] = &opts[RXH];
    regs[26] = &opts[RYL]; regs[31] = &opts[RYH];
}

int zxCPU::call(uint16_t address) {
    *_SP -= 2;
    *zxSpeccy::_CALL = *_PC;
    wm16(*_SP, *_PC);
    *_PC = address;
    return 17;
}

int zxCPU::signalINT() {
    if(*_IFF1) {
        *zxSpeccy::_STATE &= ~ZX_HALT;
        *_IFF1 = *_IFF2 = 0;
        incrementR();
        switch(*_IM) {
            case 0: case 1: call(0x38); return 13;
            case 2: call(rm16((uint16_t)((*_I << 8) | 255))); return 19;
        }
    }
    return 0;
}

int zxCPU::signalNMI() {
    *_IFF1 = 0;
    call(0x66);
    return 11;
}

uint8_t * zxCPU::initOperand(uint8_t o, uint8_t oo, int prefix, uint16_t& v16, uint8_t& v8) {
    switch(o) {
        // константа 8 бит
        case _C8: v8 = rm8PC(); break;
        // константа/адрес 16 бит
        case _C16: v16 = rm16PC(); break;
        // без операнда
        case _N_: break;
        // (HL)/(IX/IY +- d)
        case _RPHL: {
            uint8_t offs(0);
            if (prefix) { ticks += 8; offs = rm8PC(); }
            // виртуальный адрес [HL/IX/IY + D]
            v16 = *(uint16_t *) regs[_RL + prefix] + (int8_t) offs;
            // 8 бит значение по виртуальному адресу
            v8 = rm8(v16);
            break;
        }
        default: {
            if (oo == _RPHL) prefix = 0;
            // адрес регистра
            auto addr = regs[(o & 15) + prefix];
            // 16 бит значение регистра
            v16 = *(uint16_t *) addr;
            // 8 бит значение регистра
            v8 = (uint8_t) v16;
            return addr;
        }
    }
    return nullptr;
}

// выполнение инструкции процессора
int zxCPU::step() {
    if(checkSTATE(ZX_HALT)) return 4;

    int offset(0);

    prefix = 0;
    fch = 0;
    ticks = 0;
    execFlags = 0;
    setFlags = 0;

    // пропустить префикс(ы)
    while(true) {
        codeOps = rm8PC();
        incrementR();
        m = &mnemonics[codeOps + offset];
        if(m->ops != O_PREF) break;
        ticks += 4;
        offset = m->flags << 8;
        if(offset == 256 && prefix) {
            initOperand(_RPHL, _N_, prefix, vSrc, cb8);
            fch = 1;
        }
        prefix = m->tiks & 31;
    }

    ops = m->ops;
    mskFlags = m->flags;
    regDst = m->regDst;
    regSrc = m->regSrc;

    ticks += m->tiks & 31;

    dst = initOperand(regDst, regSrc, prefix, vDst, v8Dst);
    initOperand(regSrc, regDst, prefix, vSrc, v8Src);

    (this->*funcs[ops])();

    // установить флаги
    if(mskFlags) {
        auto flg = mskFlags & ~execFlags;
        if(flg & FS)  setFlags |= (res & 0x80);
        if(flg & FZ)  setFlags |= ((res == 0) << 6);
        if(flg & F5)  setFlags |= (res & 32);
        if(flg & FH)  setFlags |= hcarry(v8Dst, v8Src, res, fch, (uint8_t)(setFlags & 2));
        if(flg & F3)  setFlags |= (res & 8);
        if(flg & FPV) setFlags |= overflow(v8Dst, v8Src, res, fch, (uint8_t)(setFlags & 2));
        *_F &= ~mskFlags;
        *_F |= (setFlags & mskFlags);
    }
    return ticks;
}

void zxCPU::rotate(uint8_t value) {
    static uint8_t rot[]   = { 7, 0, 7, 0, 7, 0, 7, 0, 1, 7, 1, 7, 1, 7, 1, 7 };
    static uint8_t idx[]   = { 0, 0, 1, 1, 3, 2, 4, 3 };
    static uint8_t shift[] = { 0, 7, 0, 7, 0, 0, 0, 0 };
    static uint8_t vals[]  = { 0, 0, 0, 0, 1 };
    // --503-0C / SZ503P0C
    execFlags = FH | FPV;
    auto r  = (codeOps & 56) >> 3;
    auto i  = (uint16_t)(value) << rot[r + 8];
    auto c  = (uint8_t)((value >> rot[r]) & 1);
    vals[0] = c << shift[r]; // new c
    vals[1] = (uint8_t)(*_F & 1) << shift[r]; // old c
    vals[2] = (uint8_t)(value & 128);
    res     = vals[idx[r]] | (uint8_t)(i >> (codeOps & 8));
    setFlags= tbl_parity[res] | c;
}

void zxCPU::opsBlock() {
    auto dir = codeOps & 8 ? -1 : 1;
    auto rep = codeOps & 16;
    v8Dst = *_A; v8Src = rm8(*_HL);
    uint8_t fpv;
    switch(ops) {
        // PV=1 если после декремента BC<>0
        // FX=бит 3 операции переданный байт + A
        // FY=бит 1 операции переданный байт + A
        case O_LDI:
            // --*0**0-
            wm8(*_DE, v8Src); *_DE += dir; *_HL += dir; *_BC -= 1;
            execFlags = F5 | FH | F3 | FPV;
            cb8 = (uint8_t)(v8Src + v8Dst);
            fpv = (uint8_t)((*_BC != 0) << 2);
            setFlags = ((cb8 & 2) << 4) | (cb8 & 8) | fpv;
            if(rep && fpv) { *_PC -= 2; ticks += 5; }
            break;
        // PV=1 если после декремента BC<>0
        // S,Z,HC из A - [HL]
        // FX=бит 3 операции A - [HL] - HC, где HC взят из F после предыдущей операции
        // FY=бит 1 операции A - [HL] - HC
        case O_CPI:
            // SZ*H**1-
            res = v8Dst - v8Src; *_HL += dir; *_BC -= 1;
            execFlags = F5 | F3 | FPV;
            cb8 = res - (uint8_t)((*_F & 16) >> 4);
            fpv = (uint8_t)((*_BC != 0) << 2);
            setFlags = ((cb8 & 2) << 4) | (cb8 & 8) | fpv | 2;
            if(fpv && rep && res) { *_PC -= 2; ticks += 5; }
            break;
        // S, Z, F5, F3 из декремента B
        // fn — копия 7-го бита значения, полученного из порта
        // fh, fc = ((HL) + ((C + dx) & 255) > 255)
        // fpv = _FP(((HL) + ((C + dx) & 255)) & 7) xor B)
        case O_INI:
            // SZ5*3***
            v8Dst = zx->readPort(*_C, *_B); wm8(*_HL, v8Dst); *_HL += dir; *_B -= 1;
            execFlags = FH | FPV;
            res = *_B;
            cb8 = v8Src + (uint8_t)(*_C + dir);
            fpv = tbl_parity[(cb8 & 7) ^ res];
            setFlags = ((cb8 > 255) << 4) | fpv | ((v8Src >> 6) & 2) | (cb8 > 255);
            if(fpv && rep) { *_PC -= 2; ticks += 5; }
            break;
        // S, Z, F5, F3 из декремента B
        // fn — копия 7-го бита значения, переданного в порт;
        // fh, fc = ((HL) + L) > 255)
        // fpv = _FP(((HL) + L) & 7) xor B)
        case O_OTI:
            // SZ5*3***
            zx->writePort(*_C, *_B, v8Src); *_HL += dir; *_B -= 1;
            execFlags = FH | FPV;
            res = *_B;
            cb8 = v8Src + opts[RL];
            fpv = tbl_parity[(cb8 & 7) ^ res];
            setFlags = ((cb8 > 255) << 4) | fpv | ((v8Src >> 6) & 2) | (cb8 > 255);
            if(fpv && rep) { *_PC -= 2; ticks += 5; }
            break;
    }
}

void zxCPU::opsJump() {
    // проверить на наличие флага
    if(!(mskFlags && ((*_F & flags_vals[mskFlags]) != flags_cond[mskFlags]))) {
        switch (ops) {
            case O_RST:
                call((uint16_t)(codeOps & 56));
                break;
            case O_JMP:
                *_PC = vSrc;
                ticks = 14;
                break;
            case O_JR:
                *_PC += (int8_t) v8Src;
                opts[RTMP] = (uint8_t) (*_PC >> 8);
                ticks = 12;
                break;
            case O_CALL:
                call(vSrc);
                ticks = 17;
                break;
            case O_RETN:
                *_IFF1 = *_IFF2;
                ticks = 4;
            case O_RET:
                *_PC = rm16(*_SP);
                *_SP += 2;
                ticks += 6;
                break;
        }
    }
    mskFlags = 0;
}

void zxCPU::opsBits() {
    v8Src = numBits[(codeOps >> 3) & 7];
    if(fch) v8Dst = cb8;
    switch(ops) {
        case O_ROTX:
            rotate(v8Dst);
            v8Dst = res;
            break;
        case O_SET: case O_RES:
            v8Dst ^= (-(ops == O_SET) ^ v8Dst) & v8Src;
            break;
        case O_BIT:
            // *Z513*0-
            execFlags = FS | FZ | F5 | FH | F3 | FPV;
            cb8 = (uint8_t)((v8Dst & v8Src) == 0);
            setFlags = ((v8Src == 128 && cb8 == 0) << 7) | (cb8 << 6) | 16 | (cb8 << 2);
            if (ticks > 15) {
                // из старшего байта IX/IY + D
                setFlags |= ((vDst >> 8) & 40);
            } else if (regDst == _RPHL) {
                // из внутреннего регистра
                setFlags |= (opts[RTMP] & 40);
            } else {
                // из операции
                setFlags |= ((v8Src == 8 && cb8 == 0) << 3) | ((v8Src == 32 && cb8 == 0) << 5);
            }
            return;
    }
    if (fch) {
        if(dst) *dst = v8Dst;
        vDst = vSrc;
        regDst = _RPHL;
    }
    if (regDst == _RPHL) wm8(vDst, v8Dst); else *dst = v8Dst;
}

void zxCPU::opsExchange() {
    switch(codeOps) {
        case EX_AF:
            SWAP_REG(_AF, _AF + 4);
            break;
        case EX_DE_HL:
            SWAP_REG(_DE, _HL);
            break;
        case EXX:
            SWAP_REG(_BC, _BC + 4);
            SWAP_REG(_DE, _DE + 4);
            SWAP_REG(_HL, _HL + 4);
            break;
        case EX_PSP_HL:
            vSrc = rm16(*_SP);
            wm16(*_SP, vDst);
            *(uint16_t *) dst = vSrc;
            break;
    }
}

void zxCPU::opsSpecial() {
    switch (codeOps) {
        case CPL:
            // --*1*-1-
            res = *_A = ~*_A;
            execFlags = FH;
            setFlags = 18;
            break;
        case CCF:
            // --***-0C
            res = *_A;
            execFlags = FH;
            setFlags = ((*_F & 1) << 4) | ((*_F & 1) == 0);
            break;
        case SCF:
            // --*0*-01
            res = *_A;
            execFlags = FH;
            setFlags = 1;
            break;
        case DAA: {
            // SZ5*3P-*
            res = *_A;
            execFlags = FH | FPV;
            auto c = (uint8_t)(*_F & 1);
            auto h = (uint8_t)(*_F & 16);
            auto n = (uint8_t)(*_F & 2);
            if(((res & 15) > 9) || h) {
                res += (uint8_t)(n ? -6 : 6);
                setFlags = (c | (uint8_t)(res > 255)) | 16;
            }
            if((*_A > 0x99) || c) {
                res += (n ? -96 : 96);
                setFlags |= 1;
            } else setFlags &= ~1;
            *_A = (uint8_t)res;
            setFlags |= tbl_parity[res] | n;
            break;
        }
        case RLD: case RRD: {
            // SZ503P0-
            auto r0 = (codeOps & 8) >> 1;
            auto r1 = r0 ^ 4;
            auto _1 = ((uint16_t) (v8Dst) << 4);
            auto _2 = _1 >> (r1 << 1);
            wm8(vDst, (uint8_t) (_2 | ((v8Src & 15) << r1)));
            *_A = res = (uint8_t) ((v8Src & 240) | ((v8Dst >> r0) & 15));
            execFlags = FH | FPV;
            setFlags = tbl_parity[res];
            break;
        }
        case EI: case DI:
            *_IFF1 = *_IFF2 = (uint8_t) ((codeOps & 8) >> 3);
            break;
        case HALT:
            *zxSpeccy::_STATE |= ZX_HALT;
            *_IFF1 = *_IFF2 = 1;
            break;
        case DJNZ:
            *_B -= 1;
            if (*_B) { *_PC += (int8_t) v8Src; ticks += 5; }
            break;
        case JP_HL:
            *_PC = vDst;
            break;
    }
}

void zxCPU::opsPort() {
    auto A8A15 = (regSrc == _RC ? *_B : v8Dst);
    if(ops == O_IN) {
        // IN DST, NA/IN DST, CB
        *dst = res = zx->readPort(v8Src, A8A15);
        if (mskFlags) {
            // SZ503P0-
            execFlags = FH | FPV;
            setFlags = tbl_parity[res];
        }
    } else {
        // OUT NA/CB, SRC
        zx->writePort(v8Src, A8A15, v8Dst);
    }
}

void zxCPU::opsStd() {
    switch(ops) {
        case O_ASSIGN:
            // LD DST, SRC/NN
            if (regDst & _R16) {
                *(uint16_t *) dst = vSrc;
            }
            else {
                // LD DST, SRC/N
                *dst = v8Src;
                if (mskFlags) {
                    // LD A_I/A_R -
                    // SZ503*0-
                    execFlags = FH | FPV;
                    res = v8Src; setFlags = *_IFF2 << 2;
                }
            }
            break;
        case O_LOAD:
            // LD DST, [NN]
            if (regDst & _R16) {
                *(uint16_t *) dst = rm16(vSrc);
            }
            else {
                if (regSrc == _RPHL) {
                    // LD DST, [HL/IX/IY + D]
                    *dst = v8Src;
                    if(prefix) opts[RTMP] = (uint8_t) (vSrc >> 8);
                } else {
                    // LD A, [NN] / LD A, (BC/DE)
                    *dst = rm8(vSrc);
                }
            }
            break;
        case O_SAVE:
            // LD [NN], SRC16
            if(regSrc & _R16) wm16(vDst, vSrc);
            // LD [NN], A / LD (BC/DE), A / LD [HL/IX/IY + D], SRC
            else wm8(vDst, v8Src);
            break;
        case O_INC: case O_DEC:
            cb8 = (uint8_t)((ops - O_INC) + 1);
            if(regDst & _R16) *(uint16_t*)dst = (vDst + (int8_t)cb8);
            else {
                // SZ5H3V0- / SZ5H3V1-
                v8Src = 1;
                res = (v8Dst + (int8_t)cb8);
                if(dst) *dst = res; else wm8(vDst, res);
                setFlags = ((cb8 == 255) << 1);
            }
            break;
        case O_CP:
            // SZ*H*V1C
            v8Dst = *_A; res = (uint8_t)(n32 = (v8Dst - v8Src));
            execFlags = F3 | F5;
            setFlags = (v8Src & 40) | 2 | (n32 > 255);
            break;
        case O_NEG:
            // SZ5H3V1C
            v8Dst = 0; v8Src = *_A;
            *_A = res = (v8Dst - v8Src);
            execFlags = FPV;
            setFlags = ((v8Src == 0x80) << 2) | 2 | (v8Src != 0);
            break;
        case O_PUSH:
            *_SP -= 2;
            wm16(*_SP, vDst);
            break;
        case O_POP:
            *(uint16_t *) dst = rm16(*_SP);
            *_SP += 2;
            break;
        case O_ROT:
            rotate(v8Dst);
            if(dst) *dst = res; else wm8(vDst, res);
            break;
        case O_IM:
            v8Src = (uint8_t)((codeOps >> 3) & 3);
            *_IM = (uint8_t)(v8Src ? v8Src - 1 : 0);
            break;
        default: LOG_DEBUG("found NONI(%i) from PC: %i", m->name, zxSpeccy::PC);
    }
}

void zxCPU::opsAddSub() {
    cb8 = (uint8_t)((ops - O_ADD) << 1);
    if (regDst & _R16) {
        // --***-0C
        *(uint16_t*)dst = (uint16_t)(n32 = (vDst + vSrc));
        v8Dst = (uint8_t) (vDst >> 8);
        v8Src = (uint8_t) (vSrc >> 8);
        res   = (uint8_t)(n32 >> 8);
        if(ops == O_ADD) opts[RTMP] = v8Dst;
    } else {
        // SZ5H3V0C / SZ5H3V1C
        v8Dst = *_A;
        res = *_A = (uint8_t)(n32 = cb8 ? (v8Dst - v8Src) : (v8Dst + v8Src));
        n32 <<= 8;
    }
    setFlags = cb8 | (n32 > 65535);
}

void zxCPU::opsAdcSbc() {
    // SZ***V0C / SZ***V1C
    fch = (uint8_t)(*_F & 1);
    cb8 = (uint8_t)((ops - O_ADC) << 1);
    if (regDst & _R16) {
        *(uint16_t*)dst = (uint16_t)(n32 = cb8 ? (vDst - (vSrc + fch)) : (vDst + (vSrc + fch)));
        v8Dst = (uint8_t) (vDst >> 8);
        v8Src = (uint8_t) (vSrc >> 8);
        res = (uint8_t)(n32 >> 8);
        execFlags = FZ;
        setFlags = ((n32 == 0) << 6);
    } else {
        // SZ5H3V0C / SZ5H3V1C
        v8Dst = *_A;
        res = *_A = (uint8_t)(n32 = cb8 ? (v8Dst - (v8Src + fch)) : (v8Dst + (v8Src + fch)));
        n32 <<= 8;
    }
    setFlags |= cb8 | (n32 > 65535);
}

void zxCPU::opsLogic() {
    switch(ops) {
        // SZ513P00
        case O_AND: *_A &= v8Src; setFlags = 16; break;
        // SZ503P00
        case O_XOR: *_A ^= v8Src; break;
        // SZ503P00
        case O_OR:  *_A |= v8Src; break;
    }
    res = *_A;
    execFlags = FH | FPV;
    setFlags |= tbl_parity[res];
}

#pragma clang diagnostic pop
