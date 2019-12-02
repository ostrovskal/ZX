//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxCPU.h"
#include "zxMnemonic.h"
#include "stkMnemonic.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

static uint8_t tbl_parity[256];

uint8_t flags_cond[8] = {FZ, FZ, FC, FC, FPV, FPV, FS, FS};

static const char* namesCode[] = { "C", "B", "E", "D", "L", "H", "R", "A",
                                   "", "(HL)", "I", "BC", "DE", "HL", "AF", "SP",
                                   "NZ", "Z", "NC", "C", "PO", "PE", "P", "M",
                                   "XH", "XL", "YH", "YL", "(IX", "(IY", "F", "IX", "IY", "IM ",
                                   "(BC)", "(DE)",
                                   "NOP", "EX AF, AF'", "DJNZ ", "JR ",
                                   "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF",
                                   "DI", "EI",
                                   "ADD ", "ADC ", "SUB ", "SBC ", "AND ", "XOR ", "OR ", "CP ",
                                   "RLC ", "RRC ", "RL ", "RR ", "SLA ", "SRA ", "SLI ", "SRL ",
                                   "BIT ", "RES ", "SET ",
                                   "INC ", "DEC ",
                                   "RRD", "RLD",
                                   "LDI", "CPI", "INI", "OTI",
                                   "LDD", "CPD", "IND",  "OTD",
                                   "LDIR", "CPIR", "INIR", "OTIR",
                                   "LDDR", "CPDR", "INDR",  "OTDR",
                                   "EXX", "EX DE, HL", "EX (SP), ", "LD ", "JP ", "CALL ",
                                   "RET ", "RETI", "RETN", "RST ", "PUSH ", "POP ",
                                   "HALT", "NEG", "IN ", "OUT ", "*IX*", "*IY*", "*ED*",
                                   ", ", "0", "(C)", "n", "(nn)"};

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

    _I = &opts[RI]; _R = &opts[RR]; _IM = &opts[IM];
    _IFF1 = &opts[IFF1]; _IFF2 = &opts[IFF2];
    _A = &opts[RA]; _F = &opts[RF]; _B = &opts[RB]; _C = &opts[RC];
    _BC = (uint16_t*)&opts[RC]; _DE = (uint16_t*)&opts[RE]; _HL = (uint16_t*)&opts[RL];
    _AF = (uint16_t*)&opts[RF]; _SP = (uint16_t*)&opts[RSPL]; _PC = (uint16_t*)&opts[RPCL];
    _IX = (uint16_t*)&opts[RXL]; _IY = (uint16_t*)&opts[RYL];
    ssh_memzero(rRON, sizeof(rRON));
    ssh_memzero(rRP,  sizeof(rRP));

    rRP[0]  = (uint16_t*)_I; rRP[1]  = _BC; rRP[2]   = _DE; rRP[3]  = _HL; rRP[4]   = _AF; rRP[5]   = _SP;
    rRP[8]  = (uint16_t*)_I; rRP[9]  = _BC; rRP[10]  = _DE; rRP[11] = _IX; rRP[12]  = _AF; rRP[13]  = _SP;
    rRP[16] = (uint16_t*)_I; rRP[17] = _BC; rRP[18]  = _DE; rRP[19] = _IY; rRP[20]  = _AF; rRP[21]  = _SP;

    for(int i = 0 ; i < 8; i++) {
        auto reg = &opts[i];
        rRON[i] = reg; rRON[i + 8] = reg; rRON[i + 16] = reg;
    }
    rRON[12] = &opts[RXL]; rRON[13] = &opts[RXH];
    rRON[20] = &opts[RYL]; rRON[21] = &opts[RYH];
    rRON[6] = _R; rRON[14] = _R; rRON[22] = _R;
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
        if(codeExOps == PREF_ED && (codeOps == LD_A_R || codeOps == LD_A_I)) fpv = 0;
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
            v16 = *rRP[_RHL - _RI + prefix] + (int8_t)offs;
            // 8 бмт значение по виртуальному адресу
            v8 = rm8(v16);
            // реальный адрес
            //addr = realPtr(v16);
        } else {
            if(oo == _RPHL) prefix = 0;
            // адрес регистра
            if (o >= _RI) addr = (uint8_t*)rRP[(o - _RI) + prefix];
            else addr = rRON[o + prefix];
            // 16 бит значение регистра
            v16 = *(uint16_t*)addr;
            // 8 бит значение регистра
            v8 = (uint8_t)v16;
        }
    }
    return addr;
}

int zxCPU::step(int prefix, int offset) {
    uint8_t n8, v8Dst(0), v8Src(0), bit(0);
    uint16_t n16, vSrc(0), vDst(0);
    uint32_t n32;
    int ticks = 0;
    static bool debug = false;

    if(*zxALU::_STATE & ZX_HALT) return 4;

    incrementR();

    auto pc = *_PC;

    if(!offset) codeExOps = 0;
    else if(offset == 256 && prefix) {
        initOperand(_RPHL, _RN, prefix, vSrc, v8Src, &ticks);
        prefix = 0;
    }

    if(*_PC == 5633) {
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
        ticks++;
        ticks--;
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
                if(offset) { _FS(v8Src); _FZ(v8Src); fh = fn = 0; _FV(*_IFF2); }
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
            if (nDst >= _RBC) {
                // --***-0C
                n32 = vDst + vSrc;
                *(uint16_t *) dst = (uint16_t) n32;
                _FH((vDst >> 8), (vSrc >> 8), 0, 0); _FN(0); _FC16(n32);
            } else {
                // SZ5H3V0C / SZ5H3V1C
                auto sub = (uint8_t) (ops - O_ADD);
                v8Dst = *_A;
                n8 = (uint8_t) (n16 = sub ? (v8Dst - v8Src) : (v8Dst + v8Src));
                *dst = (uint8_t) n16;
                _FS(n8); _FZ(n8); _FH(v8Dst, v8Src, 0, sub); _FV8(v8Dst, v8Src, n8, 0, sub); _FN(sub); _FC8(n16);
            }
            break;
        case O_ADC:
        case O_SBC: {
                auto sbc = (uint8_t) (ops - O_ADC);
                auto fc = getFlag(FC);
                if (nDst >= _RBC) {
                    // SZ***V0C / SZ***V1C
                    n32 = sbc ? vDst - (vSrc + fc) : vDst + (vSrc + fc);
                    *(uint16_t*)dst = n16 = (uint16_t) n32;
                    _FS(n16 >> 8); _FZ(n16); _FH((vDst >> 8), (vSrc >> 8), fc, sbc); _FV16(vDst, vSrc, n16, fc, sbc); _FN(sbc); _FC16(n32);
                } else {
                    // SZ5H3V0C / SZ5H3V1C
                    v8Dst = *_A;
                    *dst = n8 = (uint8_t) (n16 = sbc ? (v8Dst - (v8Src + fc)) : (v8Dst + (v8Src + fc)));
                    _FS(n8); _FZ(n8); _FH(v8Dst, v8Src, fc, sbc); _FV8(v8Dst, v8Src, n8, fc, sbc); _FN(sbc); _FC8(n16);
                }
            }
            break;
        case O_INC:
        case O_DEC:
            n16 = (uint16_t)((ops - O_INC) + 1);
            if(nDst >= _RBC) *(uint16_t*)dst = (uint16_t)(vDst + n16);
            else {
                // SZ5H3V0- / SZ5H3V1-
                v8Src = (uint8_t)(v8Dst + (uint8_t)n16);
                if (nDst < _RPHL) *dst = v8Src;
                else wm8(vDst, v8Src);
                n8 = (uint8_t)(n16 == 65535);
                _FS(v8Src); _FZ(v8Src); _FH(v8Dst, n16, 0, n8); _FN(n8); _FV8(v8Dst, (uint8_t)n16, v8Src, 0, n8);
            }
            break;
        case O_XOR:
            // SZ503P00
            n8 = (*_A ^= v8Src);
            _FS(n8); _FZ(n8); _FP(n8); _FHV(0); _FN(0); _FC(0);
            break;
        case O_OR:
            // SZ503P00
            n8 = (*_A |= v8Src);
            _FS(n8); _FZ(n8); _FP(n8); _FHV(0); _FN(0); _FC(0);
            break;
        case O_AND:
            // SZ513P00
            n8 = (*_A &= v8Src);
            _FS(n8); _FZ(n8); _FHV(1); _FP(n8); _FN(0); _FC(0);
            break;
        case O_CP:
            // SZ*H*V1C
            n8 = (uint8_t)(n16 = (*_A - v8Src));
            _FS(n8); _FZ(n8); _FH(*_A, v8Src, 0, 1); _FV8(*_A, v8Src, n8, 0, 1); _FN(1); _FC8(n16);
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
            if(flg) { _FS(n8); _FZ(n8); _FP(n8); _FHV(0); _FN(0); }
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
            _FZ(v8Dst & bit); // Z
            fs = (uint8_t)(bit == 128 && fz); // S
            fh = 1; fpv = fz;
            break;
        case O_RST:
            call((uint16_t)(codeOps & 56));
            break;
        case O_PREFIX:
            codeExOps = codeOps;
            switch(codeOps) {
                case PREF_CB: ticks += step(prefix, 256); break;
                case PREF_ED: ticks += step(0, 512); break;
                // возможен stack overflow!!!
                case PREF_DD: ticks += step(8, 0); break;
                case PREF_FD: ticks += step(16, 0); break;
            }
            break;
        case O_SPEC:
            opsSpec(v8Dst, v8Src, vDst, vSrc, dst);
            break;
        case O_NEG:
            // SZ5H3V1C
            v8Dst = *_A;
            v8Src = (uint8_t) (0 - v8Dst);
            *_A = v8Src;
            _FS(v8Src); _FZ(v8Src); _FH(0, v8Dst, 0, 1); _FV((v8Dst == 0x80)); _FN(1); _FC(v8Dst != 0);
            break;
        case O_RLD:
            // SZ503P0-
            wm8(vDst, (uint8_t) ((v8Dst << 4) | (v8Src & 15)));
            n8 = *_A = (uint8_t) ((v8Src & 240) | (uint8_t) ((v8Dst & 240) >> 4));
            _FS(n8); _FZ(n8); _FP(n8); fh = fn = 0;
            break;
        case O_RRD:
            // SZ503P0-
            wm8(vDst, (uint8_t)((v8Dst >> 4) | (v8Src << 4)));
            n8 = *_A = (uint8_t)((v8Src & 240) | (v8Dst & 15));
            _FS(n8); _FZ(n8); _FP(n8); fh = fn = 0;
            break;
        case O_REP:
            opsRep(flg, &ticks);
            break;
        default:
            info("noni PC: %i (pref: %i op: %i)", pc, codeExOps, codeOps);
    }
    if(flg) {
        uint8_t f = fc;
        f |= fn << 1;
        f |= fpv << 2;
        f |= fh << 4;
        f |= fz << 6;
        f |= fs << 7;
        *_F &= ~flg;
        *_F |= f & flg;
    }
    return ticks;
}

void zxCPU::opsSpec(uint8_t v8Dst, uint8_t v8Src, uint16_t vDst, uint16_t vSrc, uint8_t* dst) {
    uint8_t n8;
    uint16_t n16;

    switch (codeOps) {
        case OUT_PC_0:
        ALU->writePort(v8Src, *_B, 0);
        break;
        case IN_F_PC:
        // SZ503P0-
        n8 = ALU->readPort(v8Src, *_B);
        _FS(n8); _FZ(n8); _FP(n8);
        break;
        case NOP:
            break;
        case CPL:
            // --*1*-1-
            *_A = ~*_A;
            fn = fh = 1;
            break;
        case CCF:
            // --***-0C
            n8 = getFlag(FC);
            _FHV(n8); _FC(!n8); _FN(0);
            break;
        case SCF:
            // --*0*-01
            fpv = fn = 0; fc = 1;
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
                        *_A -= 0x60;
                        fc = 1;
                    }
                }
                *_A = v8Dst;
                _FS(v8Dst); _FZ(v8Dst); _FP(v8Dst);
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
    fh = fn = 0;
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
    value |= nvalue;
    if(codeExOps == PREF_CB) { _FS(value); _FZ(value); _FP(value); }
    return value;
}

void zxCPU::opsRep(uint8_t flg, int* ticks) {
    uint8_t n1, n2;
    int dx = flg & F3 ? -1 : 1;
    auto rep = flg & F5 ? 1 : 0;
    switch(codeOps) {
        // --*0**0-
        case LDDR: case LDIR: case LDD: case LDI:
            wm8(*_DE, rm8(*_HL));
            *_DE += dx; *_HL += dx; *_BC -= 1;
            _FV(*_BC != 0); fh = fn = 0;
            if(rep && fpv) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
        // SZ*H**1-
        case CPDR: case CPIR: case CPD: case CPI:
            n1 = rm8(*_HL);
            *_HL += dx; *_BC -= 1;
            n2 = n1 - *_A;
            _FV(*_BC != 0); _FS(n2); _FZ(n2); _FH(n1, n2, 0, 1); _FN(1);
            if(fpv && rep) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
        // SZ5*3***
        case INDR: case INIR: case IND: case INI:
            n1 = ALU->readPort(*_C, *_B);
            wm8(*_HL, n1); *_HL += dx; *_B -= 1;
            _FV(*_B != 0); _FS(n1); _FZ(n1); _FN(1); _FC8(n1); _FHV(0);
            if(fpv && rep) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
        // SZ5*3***
        case OTDR: case OTIR: case OUTD: case OUTI:
            n1 = rm8(*_HL);
            ALU->writePort(*_C, *_B, n1);
            *_HL += dx; *_B -= 1;
            _FV(*_B != 0); _FS(n1); _FZ(n1); _FN(1); _FC8(n1); _FHV(0);
            if(rep && fpv) {
                *_PC -= 2;
                *ticks = 21;
            }
            break;
    }
}

std::string zxCPU::daMakeMnemonic(zxCPU::MNEMONIC *m, int prefix, int code, uint16_t pc, uint16_t vDst, uint16_t vSrc, uint8_t v8Src) {
    uint16_t codes[64];
    uint8_t pos = 0;

    auto cmd = m->name;
    auto regDst = m->regs & 15;
    auto regSrc = m->regs >> 4;
    //                                    "XH", "XL", "YH", "YL", "(IX", "(IY", "F", "IX", "IY", "IM ",
    auto ops = m->tiks & (CN | CNN);
    auto fl = (uint8_t)(m->ops >> 5) != 7;

    DA(cmd);

    switch(cmd) {
        case C_RET:
            if(fl) DA(C_FNZ + fl - 1);
            break;
        case C_BIT: case C_RES: case C_SET:
            DA(((code >> 3) & 7)); DA(C_COMMA);
            if(prefix) {

            }
        case C_RR: case C_RL: case C_RLC: case C_RRC:
        case C_SLA: case C_SLI: case C_SRA: case C_SRL:
        case C_INC: case C_DEC:
        case C_PUSH: case C_POP:
        case C_EX_SP:
            DA(regDst);
            break;
        case C_JP: case C_CALL: case C_DJNZ: case C_JR:
            // cmd [flag,] NN/N
            if(fl) { DA(C_FNZ + fl - 1); DA(C_COMMA); }
            if(cmd <= C_JR) {
                vDst = pc + (int8_t)v8Src;
            }
            DA(C_NN); DA(vDst);
            break;
        case C_LD:
            switch(m->ops) {
                case O_ASSIGN:
                    // cmd reg, reg/N/NN
                    DA(regDst); DA(C_COMMA);
                    if(ops & CN) { DA(C_NN); DA(v8Src); }
                    else if(ops & CNN) { DA(C_NN); DA(vSrc); }
                    else DA(regSrc);
                    break;
                case O_LOAD:
                    // cmd reg, (NN)/(RP)/(HL/IX/IY + D)
                    DA(regDst); DA(C_COMMA);
                    if(ops & CNN) { DA(C_PNN); DA(vSrc); }
                    else if(regSrc >= _RBC) DA(C_PBC + (code >> 4));
                    else DA(regSrc);
                    break;
                case O_SAVE:
                    // cmd (NN)/(RP)/(HL/IX/IY + D), reg/N
                    if(ops & CNN) { DA(C_PNN); DA(vDst); }
                    else if(regDst >= _RBC) DA(C_PBC + (code >> 4));
                    else DA(regDst); // (HL/IX/IY + D)
                    DA(C_COMMA);
                    if(ops & CN) { DA(C_NN); DA(v8Src); }
                    else DA(regSrc);
                    break;
            }
            break;
        case C_AND: case C_OR: case C_XOR: case C_CP:
        case C_ADD: case C_ADC: case C_SUB: case C_SBC:
            // cmd reg A,SRC/N
            DA(regDst); DA(C_COMMA);
            if(ops & CN) { DA(C_NN); DA(v8Src); } else DA(regSrc);
            break;
        case C_RST:
            DA(C_NN); DA(code & 56);
            break;
        case C_IM:
            DA(C_NN); DA((code >> 3) & 3);
            break;
        case C_IN:
            if(ops & CN) {
                DA(C_A); DA(C_COMMA); DA(C_NN); DA(v8Src);
            } else {
                DA(regSrc); DA(C_COMMA); DA(C_PC);
            }
            break;
        case C_OUT:
            if(ops & CN) {
                DA(C_NN); DA(v8Src); DA(C_COMMA); DA(C_A);
            } else {
                DA(C_PC); DA(C_COMMA); DA(regSrc);
            }
            break;
    }
    std::string res;
    int idx = 0;
    char* lex;
    char* elex;
    while(idx < pos) {
        auto token = codes[idx++];
        if(token >= C_NN) {
            int n = codes[idx++];
            lex = ssh_ntos(&n, RADIX_DEC, &elex);
            if(token == C_PNN) {
                *--lex = '(';
                *elex++ = ')'; *elex = 0;
            }
        } else lex = (char*)namesCode[token];
        res += lex;
    }
    return res;
}

#pragma clang diagnostic pop

/*
    static int idx = 0;
    static int pcs[16];
    pcs[idx++ & 15] = pc;
    if(!(idx & 15)) {
        std::string res;
        for(int i = 0 ; i < 16; i++) {
            res += ssh_ntos(&pcs[i], RADIX_DEC);
            res += " ";
        }
        info("PC: %s", res.c_str());
    }
    if(ops != O_PREFIX && opts[ZX_PROP_SHOW_FPS])
        info("PC: %i %s", pc, daMakeMnemonic(m, prefix, codeOps, *_PC, vDst, vSrc, v8Dst, v8Src).c_str());
*/
