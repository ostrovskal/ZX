//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxCPU.h"
#include "zxMnemonic.h"
#include "stkMnemonic.h"
#include "zxDA.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

static uint8_t tbl_parity[256];

uint8_t*  zxCPU::regs[48];

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

zxCPU::zxCPU() {
    makeTblParity();

    static uint8_t regsTbl[] = { RC, RB, RE, RD, RL, RH, RR, RA, 255, 255, RI, RC, RE, RL, RF, RSPL} ;

    _I = &opts[RI]; _R = &opts[RR]; _IM = &opts[IM];
    _IFF1 = &opts[IFF1]; _IFF2 = &opts[IFF2];
    _A = &opts[RA]; _F = &opts[RF]; _B = &opts[RB]; _C = &opts[RC];
    _BC = (uint16_t*)&opts[RC]; _DE = (uint16_t*)&opts[RE]; _HL = (uint16_t*)&opts[RL];
    _AF = (uint16_t*)&opts[RF]; _SP = (uint16_t*)&opts[RSPL]; _PC = (uint16_t*)&opts[RPCL];
    _IX = (uint16_t*)&opts[RXL]; _IY = (uint16_t*)&opts[RYL];

    ssh_memzero(regs, sizeof(regs));

    for(int i = 0 ; i < 16; i++) {
        auto idx = regsTbl[i];
        if(idx == 255) continue;
        auto reg = &opts[idx];
        regs[i] = reg; regs[i + 16] = reg; regs[i + 32] = reg;
    }
    regs[20] = &opts[RXL]; regs[21] = &opts[RXH]; regs[29] = &opts[RXL];
    regs[36] = &opts[RYL]; regs[37] = &opts[RYH]; regs[45] = &opts[RYL];
}

int zxCPU::call(uint16_t address) {
    (*_SP) -= 2;
    wm16(*_SP, *_PC);
    (*_PC) = address;
    return 17;
}

int zxCPU::signalINT() {
    if(*_IFF1) {
        modifySTATE(0, ZX_HALT);
        *_IFF1 = *_IFF2 = 0;
        // сбрасываем флаг FPV, если была инструкция LD A,R\LD A,I
        if(codeExOps == PREF_ED && (codeOps == LD_A_R || codeOps == LD_A_I)) *_F &= ~FPV;
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

uint8_t * zxCPU::initOperand(uint8_t o, uint8_t oo, int prefix, uint16_t& v16, uint8_t& v8, int* ticks) {
    uint8_t * addr = nullptr;
    if(o != _RN) {
        if (o == _RPHL) {
            uint8_t offs = 0;
            if(prefix) { *ticks += 8; offs = rm8PC(); }
            // виртуальный адрес [HL/IX/IY + D]
            v16 = *regs[_RHL - _RI + prefix] + (int8_t)offs;
            // 8 бмт значение по виртуальному адресу
            v8 = rm8(v16);
            // реальный адрес
            //addr = realPtr(v16);
        } else {
            if(oo == _RPHL) prefix = 0;
            // адрес регистра
            addr = regs[o + prefix];
            // 16 бит значение регистра
            v16 = *(uint16_t*)addr;
            // 8 бит значение регистра
            v8 = (uint8_t)v16;
        }
    }
    return addr;
}

int zxCPU::step(int prefix, int offset) {
    uint8_t n8, v8Dst(0), v8Src(0), bit(0), fch(0);
    uint16_t vSrc(0), vDst(0);
    uint32_t n32;
    int ticks = 0;
    flags = 0;
    static bool debug = false;

    if(*zxALU::_STATE & ZX_HALT) return 4;

    incrementR();

    auto pc = *_PC;

    if(!offset) codeExOps = 0;
    else if(offset == 256 && prefix) {
        initOperand(_RPHL, _RN, prefix, vSrc, v8Src, &ticks);
        prefix = 0;
    }

    if(*_PC == 0) {
        // 5633
        debug = true;
    }
    codeOps = rm8PC();

    auto m = &mnemonics[codeOps + offset];
    auto ops = m->ops;
    auto flg = m->flags;
    auto nDst = (uint8_t)(m->regs & 15);
    auto nSrc = (uint8_t)(m->regs >> 4);

    ticks += m->tiks & 31;

    initOperand(nSrc, nDst, prefix, vSrc, v8Src, &ticks);
    auto dst = initOperand(nDst, nSrc, prefix, vDst, v8Dst, &ticks);

    if(m->tiks & CN) v8Src = rm8PC();
    else if(m->tiks & CNN) {
        auto v = rm16PC();
        if(nDst == _RN) vDst = v; else vSrc = v;
    }

    if(debug) {
        if(!prefix && !offset) {
            auto ret = zxDA::daMake(&pc, DA_CODE | DA_PC | DA_TICKS | DA_PN | DA_PNN, 0);
            info(ret);
        }
    }
    if(ops >= O_JMP) {
        if(flg && !isFlag((uint8_t)(flg - 1))) return ticks;
        flg = 0;
    } else if(ops >= O_SET) {
        bit = (uint8_t)(1 << ((codeOps >> 3) & 7));
    }
    switch(ops) {
        case O_ASSIGN:
            // LD DST, SRC/N[N]
            if(nDst >= _RBC) *(uint16_t*)dst = vSrc;
            else {
                *dst = v8Src;
                // LD A_I/A_R - SZ503*0-
                if(flg) {
                    res = v8Src; flags = FH | FPV; fh = fn = 0; fpv = *_IFF2;
//                  _FS(res); _FZ(res);
                }
            }
            break;
        case O_LOAD:
            // LD DST, [NN]/[SRC16]/[HL/IX/IY + D]
            if(nDst < _RBC) *dst = rm8(vSrc);
            else *(uint16_t*)dst = rm16(vSrc);
            break;
        case O_SAVE:
            // LD [NN]/[DST16]/[HL/IX/IY + D], SRC/N
            if(nSrc >= _RBC) wm16(vDst, vSrc);
            else wm8(vDst, v8Src);
            break;
        case O_ADD:
        case O_SUB:
            fn = (uint8_t) (ops - O_ADD);
            if (nDst >= _RBC) {
                // --***-0C
                auto n = (uint16_t)(n32 = vDst + vSrc);
                *(uint16_t *) dst = (uint16_t) n32;
                v8Dst = (uint8_t)vDst >> 8; v8Src = (uint8_t)vSrc >> 8; res = (uint8_t)(n >> 8);
                flags = FC; fn = 0; _FC16(n32);
                //_FH(v8Dst, v8Src, 0, fn);
            } else {
                // SZ5H3V0C / SZ5H3V1C
                v8Dst = *_A;
                res = fn ? (v8Dst - v8Src) : (v8Dst + v8Src);
                *dst = (uint8_t) res;
                //_FS(res); _FZ(res); _FH(v8Dst, v8Src, 0, fn); _FV8(v8Dst, v8Src, res, 0, fn); _FC8(res);
            }
            break;
        case O_ADC:
        case O_SBC:
            fn = (uint8_t) (ops - O_ADC);
            fch = getFlag(FC);
            n32 = vSrc + fch;
            if (nDst >= _RBC) {
                // SZ***V0C / SZ***V1C
                auto n = (uint16_t)(n32 = fn ? vDst - n32 : vDst + n32);
                *(uint16_t*)dst = n;
                flags = FC; _FC16(n32);
                v8Dst = (uint8_t)vDst >> 8; v8Src = (uint8_t)vSrc >> 8; res = (uint8_t)(n >> 8);
                //_FS(res); _FZ(res); _FH(v8Dst, v8Src, fch, fn); _FV16(vDst, vSrc, res, fch, fn);
            } else {
                // SZ5H3V0C / SZ5H3V1C
                v8Dst = *_A;
                *dst = res = (uint8_t)(fn ? v8Dst - n32 : v8Dst + n32);
                //_FS(res); _FZ(res); _FH(v8Dst, v8Src, fch, fn); _FV8(v8Dst, v8Src, res, fch, fn); _FC8(n16);
            }
            break;
        case O_INC:
        case O_DEC:
            v8Src = (uint8_t)((ops - O_INC) + 1);
            if(nDst >= _RBC) *(uint16_t*)dst = (vDst + (int8_t)v8Src);
            else {
                // SZ5H3V0- / SZ5H3V1-
                n8 = (uint8_t)(res = (v8Dst + (int8_t)v8Src));
                if (nDst < _RPHL) *dst = n8;
                else wm8(vDst, n8);
                fn = (uint8_t)(v8Src == 255);
                //_FS(res); _FZ(res); _FH(v8Dst, v8Src, 0, fn); _FV8(v8Dst, v8Src, res, 0, fn);
            }
            break;
        case O_XOR:
            // SZ503P00
            res = (*_A ^= v8Src); flags = FC | FH | FPV; fh = fc = fn = 0; _FP(res);
            //_FS(res); _FZ(res);
            break;
        case O_OR:
            // SZ503P00
            res = (*_A |= v8Src); flags = FH | FC | FPV; fh = fn = fc = 0; _FP(res);
            //_FS(res); _FZ(res);
            break;
        case O_AND:
            // SZ513P00
            res = (*_A &= v8Src); flags = FH | FPV | FC; fn = fc = 0; fh = 1; _FP(res);
            //_FS(res); _FZ(res);
            break;
        case O_CP:
            // SZ*H*V1C
            v8Dst = *_A; res = (v8Dst - v8Src); fn = 1;
            flags = F3 | F5; f3 = (uint8_t)(v8Src & 8); f5 = (uint8_t)(v8Src & 32);
            //_FS(res); _FZ(res); _FH(v8Dst, v8Src, 0, fn); _FV8(v8Dst, v8Src, res, 0, fn); _FC8(res);
            break;
        case O_ROT:
            n8 = rotate(ticks > 15 ? v8Src : v8Dst);
            if(ticks > 15) {
                wm8(vSrc, n8);
                if(nDst == _RPHL) break;
            }
            if(nDst == _RPHL) wm8(vDst, n8);
            else *dst = n8;
            break;
        case O_IN:
            // SZ503P0-
            *dst = n8 = ALU->readPort(v8Src, nSrc == _RC ? *_B : v8Dst);
            if(flg) {
                res = n8; flags = FPV | FH; fh = fn = 0; _FP(res);
                //_FS(res); _FZ(res);
            }
            break;
        case O_OUT:
            ALU->writePort(v8Src, nSrc == _RC ? *_B : v8Dst, v8Dst);
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
            *_PC = vDst;
            ticks = 14;
            break;
        case O_JR:
            *_PC += (int8_t)v8Src;
            ticks = 12;
            break;
        case O_CALL:
            call(vDst);
            ticks = 17;
            break;
        case O_IM:
            n8 = (uint8_t)((codeOps >> 3) & 3);
            *_IM = (uint8_t)(n8 ? n8 - 1 : 0);
            break;
        case O_RETN:
            *_IFF1 = *_IFF2;
            ticks = 8;
        case O_RET:
            *_PC = rm16(*_SP);
            *_SP += 2;
            ticks += 6;
            break;
        case O_SET:
            // под префиксом?
            if(ticks > 15) {
                n8 = v8Src | bit;
                wm8(vSrc, n8);
                if(nDst == _RPHL) break;
            } else n8 = v8Dst | bit;
            if(nDst == _RPHL) wm8(vDst, n8);
            else *dst = n8;
        case O_RES:
            // под префиксом?
            if(ticks > 15) {
                n8 = v8Src & ~bit;
                wm8(vSrc, n8);
                if(nDst == _RPHL) break;
            } else n8 = v8Dst & ~bit;
            if(nDst == _RPHL) wm8(vDst, n8);
            else *dst = n8;
            break;
        /* Z=1 если проверяемый бит = 0; S=1 если bit = 7 и проверяемый бит = 1; PV = Z */
        case O_BIT:
            // *Z513*0-
            if(ticks > 15) v8Dst = v8Src;
            _FZ(v8Dst & bit); fs = (uint8_t)(bit == 128 && !fz);
            fn = 0; fh = 1; fpv = fz; flags = FS | FZ | FH | FPV;
            break;
        case O_RST:
            call((uint16_t)(codeOps & 56));
            break;
        case O_PREFIX:
            codeExOps = codeOps;
            switch(codeOps) {
                case PREF_CB: ticks += step(prefix, 256); break;
                case PREF_ED: ticks += step(0, 512); break;
                case PREF_DD: case PREF_FD: if(!prefix) ticks += step(codeOps == PREF_DD ? 16 : 32, 0); break;
            }
            break;
        case O_SPEC:
            opsSpec(v8Dst, v8Src, vDst, vSrc, dst);
            break;
        case O_NEG:
            // SZ5H3V1C
            v8Dst = 0; v8Src = *_A;
            res = (v8Dst - v8Dst); *_A = (uint8_t)res;
            fpv = (uint8_t)(v8Dst == 0x80); fc = (uint8_t)(v8Dst != 0); fn = 1; flags = FPV | FC;
            //_FS(res); _FZ(res); _FH(v8Dst, v8Src, 0, fn);
            break;
        case O_REP:
            opsRep(&ticks);
            break;
        default:
            info("noni PC: %i (pref: %i op: %i)", pc, codeExOps, codeOps);
    }
    if(flg) {
        flags = ~flags;
        if(flg & flags & FS) _FS(res);
        if(flg & flags & FZ) _FZ(res);
        if(flg & flags & FC) _FC(res);
        if(flg & flags & FH) _FH(v8Dst, v8Src, fch, fn);
        if(flg & flags & FPV) _FV(v8Dst, v8Src, res, fch, fn);
        if(flg & flags & F3) f3 = (uint8_t)(res & 8);
        if(flg & flags & F5) f5 = (uint8_t)(res & 32);
        uint8_t f = fc | (fn << 1) | (fpv << 2) | f3 | (fh << 4) | f5 | (fz << 6) | (fs << 7);
        *_F &= ~flg;
        *_F |= (f & flg);
    }
    return ticks;
}

void zxCPU::opsSpec(uint8_t v8Dst, uint8_t v8Src, uint16_t vDst, uint16_t vSrc, uint8_t* dst) {
    uint16_t n16;

    switch (codeOps) {
        case OUT_PC_0:
            ALU->writePort(v8Src, *_B, 0);
            break;
        case RLD:
            // SZ503P0-
            wm8(vDst, (uint8_t) ((v8Dst << 4) | (v8Src & 15)));
            *_A = res = (uint8_t)((v8Src & 240) | ((v8Dst & 240) >> 4));
            flags = FPV | FH; fh = fn = 0; _FP(res);
            //_FS(res); _FZ(res);
            break;
        case RRD:
            // SZ503P0-
            wm8(vDst, (uint8_t)((v8Dst >> 4) | (v8Src << 4)));
            *_A = res = (uint8_t)((v8Src & 240) | (v8Dst & 15));
            flags = FPV | FH; fh = fn = 0; _FP(res);
            //_FS(res); _FZ(res);
            break;
        case IN_F_PC:
            // SZ503P0-
            res = ALU->readPort(v8Src, *_B);
            flags = FH | FPV; fh = fn = 0; _FP(res);
            //_FS(res); _FZ(res);
            break;
//        case NOP: case NOP_1: case NOP_2:
//            break;
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
        case DAA: {
                // SZ5*3P-*
                v8Dst = *_A;
                auto oldA = v8Dst;
                auto oldC = getFlag(FC);
                fh = getFlag(FH);
                fc = 0;
                if (getFlag(FN)) {
                    if (((v8Dst & 0xf) > 9) || fh) {
                        v8Dst += 6;
                        fc = oldC | (v8Dst > 255);
                        fh = 1;
                    } else fh = 0;
                    if ((oldA > 0x99) || oldC) {
                        v8Dst += 0x60;
                        fc = 1;
                    }
                    else fc = 0;
                } else {
                    if (((*_A & 0xf) > 9) || fh) {
                        v8Dst -= 6;
                        fc = oldC | (v8Dst > 255);
                        fh = 1;
                    } else fh = 0;
                    if ((oldA > 0x99) || oldC) {
                        v8Dst -= 0x60;
                        fc = 1;
                    }
                }
                res = *_A = v8Dst;
                flags = FPV | FC | FH; _FP(res);
                //_FS(res); _FZ(res);
            }
            break;
        case EI: case DI:
            *_IFF1 = *_IFF2 = (uint8_t) ((codeOps & 8) >> 3);
            modifySTATE(0, ZX_INT);
            break;
        case HALT: modifySTATE(ZX_HALT, 0);
            break;
        case DJNZ:
            *_B -= 1;
            if (*_B) *_PC += (int8_t) v8Src;
            break;
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
            n16 = rm16(vSrc);
            wm16(vSrc, vDst);
            *(uint16_t *) dst = n16;
            break;
        case JP_HL:
            *_PC = vDst;
            break;
    }
}

uint8_t zxCPU::rotate(uint8_t value) {
    // --503-0C / SZ503P0C
    fh = fn = 0; flags = FH | FC;
    fc = (uint8_t)((value & ((codeOps & 8) ? 1 : 128)) != 0);
    auto nvalue = ((codeOps & 8) ? value >> 1 : value << 1);
    switch(codeOps & 56) {
        // RLC c <- 7 <----- 0 <- 7
        case 0: value = fc; break;
        // RRC 0 -> 7 ------> 0 -> c
        case 8: value = (fc << 7); break;
        // RL c <- 7 <------ 0 <- c
        case 16: value = (fc << 7); break;
        // RR c -> 7 -------> 0 -> c
        case 24: value = (getFlag(FC) << 7); break;
        // SLA c <- 7 <---------- 0 <- 0
        // SRL 0 -> 7 ------------> 0 -> c
        case 32: case 56: value = 0; break;
        // SRA 7 -> 7 ------------> 0 -> c
        case 40: value &= 128; break;
        // SLI c <- 7 <----------0 <- 1
        case 48: value = 1; break;
    }
    res = value | nvalue;
    if(codeExOps == PREF_CB) {
        flags |= FPV; _FP(res);
//        _FS(res); _FZ(res);
    }
    return (uint8_t)res;
}

void zxCPU::opsRep(int* ticks) {
    uint8_t n1, a = *_A;
    int dx = codeOps & 8 ? -1 : 1;
    auto rep = (codeOps >> 4) & 1;
    switch(codeOps) {
        // --*0**0-
        /*
            PV=1 если после декремента BC<>0
            F3=бит 3 операции переданный байт + A
            F5=бит 1 операции переданный байт + A
         */
        case LDDR: case LDIR: case LDD: case LDI:
            n1 = rm8(*_HL);
            wm8(*_DE, n1);
            *_DE += dx; *_HL += dx; *_BC -= 1;
            flags = F3 | F5 | FH | FPV;
            fh = fn = 0; fpv = (uint8_t)(*_BC != 0);
            f3 = (uint8_t)((n1 + a) & 8);
            f5 = (uint8_t)((n1 + a) & 2);
            if(rep && fpv) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
        // SZ*H**1-
        /*
            PV=1 если после декремента BC<>0
            S,Z,HC из A-[HL]
            F3=бит 3 операции A-[HL]-HC, где HC взят из F после предыдущей операции
            F5=бит 1 операции A-[HL]-HC
        */
        case CPDR: case CPIR: case CPD: case CPI:
            n1 = rm8(*_HL);
            res = a - n1;
            *_HL += dx; *_BC -= 1;
            flags = FPV | FH | F3 | F5;
            fn = 1; fpv = (uint8_t)(*_BC != 0); _FH(a, n1, 0, fn);
            f3 = (uint8_t)((res - fh) & 8);
            f5 = (uint8_t)((res - fh) & 2);
            //_FS(res); _FZ(res);
            if(fpv && rep) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
        // SZ5*3***
        case INDR: case INIR: case IND: case INI:
            res = ALU->readPort(*_C, *_B);
            wm8(*_HL, res); *_HL += dx; *_B -= 1;
            flags = FH | FPV | FC | FH; fn = 1; fh = 0;// _FV(*_B != 0);
            //_FS(res); _FZ(res);
            if(fpv && rep) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
        // SZ5*3***
        case OTDR: case OTIR: case OUTD: case OUTI:
            res = rm8(*_HL);
            ALU->writePort(*_C, *_B, res);
            *_HL += dx; *_B -= 1;
            flags = FH | FPV; fn = 1; fh = 0;// _FV(*_B != 0);
            //_FS(res); _FZ(res);
            if(rep && fpv) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
    }
}

#pragma clang diagnostic pop
