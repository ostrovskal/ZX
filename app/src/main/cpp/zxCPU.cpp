//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "stkMnemonic.h"
#include "zxCPU.h"
#include "zxDA.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

static uint8_t tbl_parity[256];

uint8_t*  zxCPU::regs[36];
uint8_t flags_cond[8] = {FZ, FZ, FC, FC, FPV, FPV, FS, FS};

static void makeTblParity() {
    for(int a = 0; a < 256; a++) {
        uint8_t val = (uint8_t)a;
        val = (uint8_t)(((a >> 1) & 0x55) + (val & 0x55));
        val = (uint8_t)(((val >> 2) & 0x33) + (val & 0x33));
        val = (uint8_t)(((val >> 4) & 0x0F) + (val & 0x0F));
        tbl_parity[a] = (uint8_t)(!(val & 1));
    }
}

// пишем в память 8 бит
void zxCPU::wm8(uint16_t address, uint8_t val) {
    if(*ALU->_STATE & ZX_DEBUG) { if(ALU->checkBPs(address, ZX_BP_WMEM)) return; }
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
    *zxALU::_CALL = *_PC;
    wm16(*_SP, *_PC);
    *_PC = address;
    return 17;
}

int zxCPU::signalINT() {
    if(*_IFF1) {
        modifySTATE(0, ZX_HALT);
        *_IFF1 = *_IFF2 = 0;
        // сбрасываем флаг FPV, если была инструкция LD A,R\LD A,I
        if(resetPV) *_F &= ~FPV;
        incrementR();
        switch(*_IM) {
            case 0: case 1: call(0x38); return 13;
            case 2: call(rm16((uint16_t)((*_I << 8) | 254))); return 19;
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
    uint8_t * addr = nullptr;
    uint8_t offs = 0;
    switch(o) {
        case _RPHL:
            if(prefix) { ticks += 8; offs = rm8PC(); }
            // виртуальный адрес [HL/IX/IY + D]
            v16 = *(uint16_t*)regs[_RL + prefix] + (int8_t)offs;
            // 8 бит значение по виртуальному адресу
            v8 = rm8(v16);
            break;
        case _C8:
            // константа 8 бит
            v8 = rm8PC();
            break;
        case _C16:
            // константа/адрес 16 бит
            v16 = rm16PC();
            break;
        case _BT:
            // вычисление бита
            v8 = numBits[(codeOps >> 3) & 7];
            break;
        case _N_:
            // без операнда
            break;
        default:
            if(oo == _RPHL) prefix = 0;
            // адрес регистра
            addr = regs[(o & 15) + prefix];
            // 16 бит значение регистра
            v16 = *(uint16_t*)addr;
            // 8 бит значение регистра
            v8 = (uint8_t)v16;
            break;
    }
    return addr;
}

int zxCPU::step() {
    if((*zxALU::_STATE) & ZX_HALT) return 4;

    int offset(0), prefix(0);
    uint8_t fch(0), cb8(0);
    uint8_t v8Dst, v8Src;
    uint16_t vSrc, vDst;
    uint32_t n32;

    // для блочных операций
    int dir(0), rep(0);

    bool isExecute = true;
    bool isPrefCB = false;

    ticks = 0;
    flags = 0;
    resetPV = false;

    incrementR();

    zxCPU::MNEMONIC* m(nullptr);
    while(isExecute) {
        if(offset == 256 && prefix) {
            initOperand(_RPHL, _N_, prefix, vSrc, cb8);
            prefix = 0; isPrefCB = true;
        }
        codeOps = rm8PC();
        m = &mnemonics[codeOps + offset];
        if(m->ops != O_PREF) { isExecute = false; break; }
        ticks += m->tiks;
        switch(codeOps) {
            case PREF_CB: offset = 256; break;
            case PREF_ED: prefix = 0; offset = 512; break;
            case PREF_DD: case PREF_FD: {
                auto pxy = (codeOps == PREF_DD);
                if(prefix) { (*_PC)--; m = &mnemonics[768 + pxy]; isExecute = false; break; }
                prefix = (pxy ? 12 : 24);
                offset = 0;
                break;
            }
        }
    }

    auto ops = m->ops;
    auto mskFlags = m->flags;
    auto regDst = m->regDst;
    auto regSrc = m->regSrc;

    ticks += m->tiks & 31;

    auto dst = initOperand(regDst, regSrc, prefix, vDst, v8Dst);
    initOperand(regSrc, regDst, prefix, vSrc, v8Src);

    if(ops >= O_LDI) {
        dir = codeOps & 8 ? -1 : 1;
        rep = codeOps & 16;
        v8Dst = *_A; v8Src = rm8(*_HL);
    } else if(ops >= O_JMP) {
        isExecute = (mskFlags && !isFlag((uint8_t)(mskFlags - 1)));
        mskFlags = 0;
    }

    if(!isExecute) {
        switch(ops) {
            case O_ASSIGN:
                // LD DST, SRC/NN
                if (regDst & _R16) *(uint16_t *) dst = vSrc;
                else {
                    // LD DST,SRC/N
                    *dst = v8Src;
                    if (mskFlags) {
                        // LD A_I/A_R - SZ503*0-
                        resetPV = true; flags = FH | FPV;
                        res = v8Src; fh = fn = 0; fpv = *_IFF2;
                    }
                }
                break;
            case O_LOAD:
                // LD DST, [NN]
                if (regDst & _R16) *(uint16_t *) dst = rm16(vSrc);
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
                // SZ***V0C / SZ***V1C
            case O_ADC: case O_SBC:
                fch = getFlag(FC);
                vSrc += fch;
                // --***-0C
            case O_ADD: case O_SUB:
                fn = (uint8_t) ((ops - O_ADD) & 1);
                if (regDst & _R16) {
                    if(ops == O_ADD) opts[RTMP] = (uint8_t)(vDst >> 8);
                    *(uint16_t*)dst = _fc = (uint16_t)(n32 = fn ? (vDst - vSrc) : (vDst + vSrc));
                    flags = FC | FZ; _FC16(n32); _FZ(_fc); fch = 0;
                    v8Dst = (uint8_t) (vDst >> 8); v8Src = (uint8_t) (vSrc >> 8); res = (uint8_t) (_fc >> 8);
                } else {
                    // SZ5H3V0C / SZ5H3V1C
                    v8Dst = *_A;
                    *_A = res = (uint8_t)(_fc = (uint16_t)(fn ? (v8Dst - (v8Src + fch)) : (v8Dst + (v8Src + fch))));
                }
                break;
            case O_INC: case O_DEC:
                v8Src = 1;
                cb8 = (uint8_t)((ops - O_INC) + 1);
                if(regDst & _R16) *(uint16_t*)dst = (vDst + (int8_t)cb8);
                else {
                    // SZ5H3V0- / SZ5H3V1-
                    res = (v8Dst + (int8_t)cb8);
                    if (regDst == _RPHL) wm8(vDst, res); else *dst = res;
                    fn = (uint8_t)(cb8 == 255);
                }
                break;
            case O_XOR:
                // SZ503P00
                res = (*_A ^= v8Src); flags = FC | FH | FPV; fh = fn = fc = 0; _FP(res);
                break;
            case O_OR:
                // SZ503P00
                res = (*_A |= v8Src); flags = FH | FC | FPV; fh = fn = fc = 0; _FP(res);
                break;
            case O_AND:
                // SZ513P00
                res = (*_A &= v8Src); flags = FH | FPV | FC; fh = 1; fn = fc = 0; _FP(res);
                break;
                // SZ*H*V1C
            case O_CP:
                v8Dst = *_A; res = (uint8_t)(_fc = (uint16_t)(v8Dst - v8Src)); fn = 1;
                flags = F3 | F5; fx = (uint8_t)(v8Src & 8); fy = (uint8_t)(v8Src & 32);
                break;
                // SZ503P0-
            case O_IN:
                *dst = res = ALU->readPort(v8Src, regSrc == _RC ? *_B : v8Dst);
                if(mskFlags) { flags = FPV | FH; fh = fn = 0; _FP(res); }
                break;
            case O_OUT:
                ALU->writePort(v8Src, regSrc == _RC ? *_B : v8Dst, v8Dst);
                break;
            case O_PUSH:
                *_SP -= 2;
                wm16(*_SP, vDst);
                break;
            case O_POP:
                *(uint16_t *) dst = rm16(*_SP);
                *_SP += 2;
                break;
            case O_JMP:
                *_PC = vSrc;
                CMD_CACHE(vSrc);
                ticks = 14;
                break;
            case O_JR:
                *_PC += (int8_t)v8Src;
                opts[RTMP] = (uint8_t)(*_PC >> 8);
                ticks = 12;
                break;
            case O_CALL:
                call(vSrc);
                CMD_CACHE(vSrc);
                ticks = 17;
                break;
            case O_RETN:
                *_IFF1 = *_IFF2;
                ticks = 8;
            case O_RET:
                *_PC = rm16(*_SP);
                CMD_CACHE(*_PC);
                *_SP += 2;
                ticks += 6;
                break;
            case O_ROT:
                res = rotate(isPrefCB ? cb8 : v8Dst, codeOps > 64);
                if(isPrefCB) { wm8(vSrc, res); if(regDst == _RPHL) break; }
                if(regDst == _RPHL) wm8(vDst, res); else *dst = res;
                break;
            case O_SET:
                // под префиксом?
                if(isPrefCB) { cb8 |= v8Src; wm8(vSrc, cb8); if(regDst == _RPHL) break;}
                else cb8 = v8Dst | v8Src;
                if(regDst == _RPHL) wm8(vDst, cb8); else *dst = cb8;
                break;
            case O_RES:
                // под префиксом?
                if(isPrefCB) { cb8 &= ~v8Src; wm8(vSrc, cb8); if(regDst == _RPHL) break; }
                else cb8 = v8Dst & ~v8Src;
                if(regDst == _RPHL) wm8(vDst, cb8); else *dst = cb8;
                break;
            case O_BIT:
                // *Z513*0-
                if(isPrefCB) v8Dst = cb8;
                _FZ(v8Dst & v8Src); fpv = fz; fn = 0; fh = 1;
                flags = FS | FZ | FH | FPV | F3 | F5;
                fs = (uint8_t)(v8Src == 128 && !fz);
                if(ticks > 15) {
                    // из старшего байта IX/IY + D
                    vDst >>= 8;
                    fx = (uint8_t)(vDst & 8);
                    fy = (uint8_t)(vDst & 32);
                } else if(regDst == (_RHL)) {
                    // из внутреннего регистра
                    fx = (uint8_t)(opts[RTMP] & 8);
                    fy = (uint8_t)(opts[RTMP] & 32);
                } else {
                    // из операции
                    fx = (uint8_t) ((v8Src == 8 && !fz) << 3);
                    fy = (uint8_t) ((v8Src == 32 && !fz) << 5);
                }
                break;
            case O_RST:
                call((uint16_t)(codeOps & 56));
                break;
            case O_SPEC:
                special(vDst, v8Dst, v8Src);
                break;
            case O_IM:
                v8Src = (uint8_t)((codeOps >> 3) & 3);
                *_IM = (uint8_t)(v8Src ? v8Src - 1 : 0);
                break;
            case O_NEG:
                // SZ5H3V1C
                v8Dst = 0; v8Src = *_A;
                *_A = res = (v8Dst - v8Src);
                fpv = (uint8_t)(v8Src == 0x80); fc = (uint8_t)(v8Src != 0); fn = 1; flags = FPV | FC;
                break;
            case O_EX:
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
                break;
                /*
                    --*0**0-
                    PV=1 если после декремента BC<>0
                    F3=бит 3 операции переданный байт + A
                    F5=бит 1 операции переданный байт + A
                 */
            case O_LDI:
                wm8(*_DE, v8Src);
                *_DE += dir; *_HL += dir; *_BC -= 1;
                flags = F3 | F5 | FH | FPV;
                fh = fn = 0; fpv = (uint8_t)(*_BC != 0);
                fx = (uint8_t)((v8Src + v8Dst) & 8);
                fy = (uint8_t)((v8Src + v8Dst) & 2);
                if(rep && fpv) { *_PC -= 2; ticks = 21; }
                break;
                /*
                    SZ*H**1-
                    PV=1 если после декремента BC<>0
                    S,Z,HC из A-[HL]
                    F3=бит 3 операции A-[HL]-HC, где HC взят из F после предыдущей операции
                    F5=бит 1 операции A-[HL]-HC
                */
            case O_CPI:
                res = v8Dst - v8Src; *_HL += dir; *_BC -= 1; fh = getFlag(FH);
                fx = (uint8_t)((res - fh) & 8); fy = (uint8_t)((res - fh) & 2);
                flags = FPV | F3 | F5; fn = 1; fpv = (uint8_t)(*_BC != 0);
                if(fpv && rep && res) { *_PC -= 2; ticks = 21; }
                break;
                /*
                    SZ5*3***
                    S, Z, F5, F3 из декремента B
                    fn — копия 7-го бита значения, полученного из порта
                    fh, fc = ((HL) + ((C + dx) & 255) > 255)
                    fpv = _FP(((HL) + ((C + dx) & 255)) & 7) xor B)
                 */
            case O_INI:
                v8Src = ALU->readPort(*_C, *_B); wm8(*_HL, v8Src); *_HL += dir; *_B -= 1;
                fh = fc = (uint8_t)(v8Src + ((*_C + dir) & 255) > 255); fn = v8Src >> 7;
                fpv = _FP(((v8Src + ((*_C + dir) & 255)) & 7) ^ *_B);
                res = *_B; flags = FH | FC | FPV;
                if(fpv && rep) { *_PC -= 2; ticks = 21; }
                break;
                /*
                    SZ5*3***
                    S, Z, F5, F3 из декремента B
                    fn — копия 7-го бита значения, переданного в порт;
                    fh, fc = ((HL) + L) > 255)
                    fpv = _FP(((HL) + L) & 7) xor B)
                 */
            case O_OTI:
                ALU->writePort(*_C, *_B, v8Src); *_HL += dir; *_B -= 1;
                fh = fc = (uint8_t)((v8Src + opts[RL]) > 255); fn = (v8Src >> 7);
                fpv = _FP(((v8Src + opts[RL]) & 7) ^ *_B);
                res = *_B; flags = FH | FC | FPV;
                if(fpv && rep) { *_PC -= 2; ticks = 21; }
                break;
            default: debug("found NONI(%i) from PC: %i", m->name, zxALU::PC);
        }
    }
    if(mskFlags) {
        auto flg = mskFlags & ~flags;
        if(flg & FS) _FS(res);
        if(flg & FZ) _FZ(res);
        if(flg & F5) fy = (uint8_t)(res & 32);
        if(flg & FH) _FH(v8Dst, v8Src, res, fch, fn);
        if(flg & F3) fx = (uint8_t)(res & 8);
        if(flg & FPV) _FV(v8Dst, v8Src, res, fch, fn);
        if(flg & FC) _FC(_fc);
        uint8_t f = fc | (fn << 1) | (fpv << 2) | fx | (fh << 4) | fy | (fz << 6) | (fs << 7);
        *_F &= ~mskFlags;
        *_F |= (f & mskFlags);
    }
/*
    if (opts[ZX_PROP_LAUNCH_TRACCER]) {
        if (!prefix && !offset) {
            auto length = zxDA::cmdParser(&zxALU::PC, (uint16_t *) &TMP_BUF[0], true);
            ALU->ftracer.write(&length, 1);
            ALU->ftracer.write(TMP_BUF, (size_t)length);
        }
    }
*/
    return ticks;
}

void zxCPU::special(uint16_t vDst, uint8_t v8Dst, uint8_t v8Src) {
    switch (codeOps) {
        case OUT_PC_0:
            ALU->writePort(*_C, *_B, 0);
            break;
        case IN_F_PC:
            // SZ503P0-
            res = ALU->readPort(*_C, *_B);
            flags = FH | FPV; fh = fn = 0; _FP(res);
            break;
        case RLD:
            // SZ503P0-
            wm8(vDst, (uint8_t) ((v8Dst << 4) | (v8Src & 15)));
            *_A = res = (uint8_t)((v8Src & 240) | ((v8Dst & 240) >> 4));
            flags = FPV | FH; fh = fn = 0; _FP(res);
            break;
        case RRD:
            // SZ503P0-
            wm8(vDst, (uint8_t)((v8Dst >> 4) | (v8Src << 4)));
            *_A = res = (uint8_t)((v8Src & 240) | (v8Dst & 15));
            flags = FPV | FH; fh = fn = 0; _FP(res);
            break;
        case CPL:
            // --*1*-1-
            res = *_A = ~*_A;
            flags = FH; fn = fh = 1;
            break;
        case CCF:
            // --***-0C
            res = *_A;
            fc = (uint8_t)((fh = getFlag(FC)) == 0);
            flags = FH | FC; fn = 0;
            break;
        case SCF:
            // --*0*-01
            res = *_A;
            flags = FH | FC; fh = fn = 0; fc = 1;
            break;
        case DAA:
            // SZ5*3P-*
            res = *_A;
            v8Dst = getFlag(FC);
            fh = getFlag(FH);
            fn = getFlag(FN);
            fc = 0;
            if(((res & 15) > 9) || fh) {
                _fc = res += (uint8_t)(fn ? -6 : 6);
                fc = v8Dst | (uint8_t)(_fc > 255);
                fh = 1;
            } else fh = 0;
            if((*_A > 0x99) || v8Dst) {
                res += (fn ? -96 : 96);
                fc = 1;
            } else fc = 0;
            *_A = res; flags = FH | FPV | FC; _FP(res);
            break;
        case EI: case DI:
            *_IFF1 = *_IFF2 = (uint8_t) ((codeOps & 8) >> 3);
            modifySTATE(0, ZX_INT);
            break;
        case HALT:
            modifySTATE(ZX_HALT, 0);
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

uint8_t zxCPU::rotate(uint8_t value, bool isCB) {
    // --503-0C / SZ503P0C
    fh = fn = 0; flags = FH | FC;
    auto ofc = getFlag(FC);
    fc = (uint8_t)((value & ((codeOps & 8) ? 1 : 128)) != 0);
    auto nvalue = ((codeOps & 8) ? value >> 1 : value << 1);
    switch(codeOps & 56) {
        // RLC c <- 7 <----- 0 <- 7
        case 0: value = fc; break;
        // RRC 0 -> 7 ------> 0 -> c
        case 8: value = (fc << 7); break;
        // RL c <- 7 <------ 0 <- c
        case 16: value = ofc; break;
        // RR c -> 7 -------> 0 -> c
        case 24: value = (ofc << 7); break;
        // SLA c <- 7 <---------- 0 <- 0
        // SRL 0 -> 7 ------------> 0 -> c
        case 32: case 56: value = 0; break;
        // SRA 7 -> 7 ------------> 0 -> c
        case 40: value &= 128; break;
        // SLL c <- 7 <----------0 <- 1
        case 48: value = 1; break;
    }
    value |= nvalue;
    if(isCB) { flags |= FPV; _FP(value); }
    return value;
}

#pragma clang diagnostic pop
