//
// Created by Сергей on 03.12.2019.
//

#include "zxCommon.h"
#include "zxDA.h"
#include "zxMnemonic.h"
#include "stkMnemonic.h"

#define DA(token)   codes[pos++] = token

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"
static const char* namesCode[] = { "C", "B", "E", "D", "L", "H", "R", "A",
                                   "", "(HL)", "I", "BC", "DE", "HL", "AF", "SP",
                                   "NZ", "Z", "NC", "C", "PO", "PE", "P", "M",
                                   "XL", "XH", "YL", "YH", "(IX", "(IY", "F", "IX", "IY", "IM ",
                                   "(BC)", "(DE)",
                                   "NOP", "EX AF, AF'", "DJNZ ", "JR ",
                                   "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF",
                                   "DI", "EI",
                                   "ADD ", "ADC ", "SUB ", "SBC ", "AND ", "XOR ", "OR ", "CP ",
                                   "RLC ", "RRC ", "RL ", "RR ", "SLA ", "SRA ", "SLL ", "SRL ",
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
                                   ", ", "0", "(C)"};

int zxDA::getDaOperand(uint8_t o, uint8_t oo, int prefix, uint16_t* v16, uint16_t *pc, int* ticks, uint8_t* offset) {
    static uint8_t cnvRegs[] = {
            C_C, C_B, C_E, C_D, C_L, C_H, C_R, C_A, C_A, C_PHL, C_I, C_BC, C_DE, C_HL, C_AF, C_SP,
            C_C, C_B, C_E, C_D, C_XL, C_XH, C_R, C_A, C_A, C_PIX, C_I, C_BC, C_DE, C_IX, C_AF, C_SP,
            C_C, C_B, C_E, C_D, C_YL, C_YH, C_R, C_A, C_A, C_PIY, C_I, C_BC, C_DE, C_IY, C_AF, C_SP
    };
    if (o == _RPHL) {
        if(pc) {
            *offset = 0;
            if (prefix) { *ticks += 8; *offset = rm8((*pc)++); }
            // виртуальный адрес [HL/IX/IY + D]
            *v16 = *(uint16_t*)zxCPU::regs[_RHL + prefix] + (int8_t)*offset;
        }
        // индекс имени регистра
        if(!prefix) return o;
        // (IX / (IY
        return (C_PIX + (prefix >> 5));
    }
    // индекс имени регистра
    if(oo == _RPHL) prefix = 0;
    return cnvRegs[prefix + o];
}

char* zxDA::daMake(uint16_t* pc, int flags, int bp, int prefix, int offset, int ticks) {
    uint8_t v8Src(0), offsSrc(0), offsDst(0);
    uint16_t vSrc(0), vDst(0);
    uint8_t pos = 0;

    static uint16_t codes[64];

    char* daResult = (char*)&TMP_BUF[65536];
    auto result = daResult;

    static int _pc;
    // определение первого захода в функцию
    if(!ticks) {
        _pc = *pc;
        ssh_memzero(daResult, 512);
    }

    if(offset == 256 && prefix) {
        getDaOperand(_RPHL, _RN, prefix, &vSrc, pc, &ticks, &offsSrc);
        prefix = 0;
    }

    auto code = rm8((*pc)++);

    auto m = &mnemonics[code + offset];
    auto regDst = (uint8_t)(m->regs & 15);
    auto regSrc = (uint8_t)(m->regs >> 4);

    getDaOperand(regSrc, regDst, prefix, &vSrc, pc, &ticks, &offsSrc);
    getDaOperand(regDst, regSrc, prefix, &vDst, pc, &ticks, &offsDst);
    ticks += m->tiks & 31;

    if(m->tiks & CN) v8Src = rm8((*pc)++);
    else if(m->tiks & CNN) {
        auto v = rm16(*pc); *pc += 2;
        if(regDst == _RN) vDst = v; else vSrc = v;
    }
    if(m->ops == O_PREFIX) {
        auto pref = code == PREF_DD;
        if(prefix && code != PREF_CB) {
            (*pc)--;
            return (char*)namesCode[pref ? C_IX_NONI : C_IY_NONI];
        }
        switch(code) {
            case PREF_CB: return daMake(pc, flags, bp, prefix, 256, ticks);
            case PREF_ED: return daMake(pc, flags, bp, 0, 512, ticks);
            case PREF_DD: case PREF_FD: return daMake(pc, flags, bp, pref ? 16 : 32, 0, ticks);
        }
    }

    auto cmd = m->name;
    auto fl = m->flags;
    auto ops = m->tiks & (CN | CNN);

    DA(cmd);

    switch(cmd) {
        case C_RET:
            if(fl) DA(C_FNZ + fl - 1);
            break;
        case C_BIT: case C_RES: case C_SET:
            // cmd bit,
            DA(C_NUM); DA(((code >> 3) & 7)); DA(C_COMMA);
        case C_RR: case C_RL: case C_RLC: case C_RRC:
        case C_SLA: case C_SLI: case C_SRA: case C_SRL:
            if(prefix) { DA(C_PHL); DA(C_COMMA); }
        case C_INC: case C_DEC:
        case C_PUSH: case C_POP:
        case C_EX_SP:
            DA(C_DST);
            break;
        case C_JP: case C_CALL: case C_DJNZ: case C_JR:
            // cmd [flag,] NN
            if(fl) { DA(C_FNZ + fl - 1); DA(C_COMMA); }
            if(cmd <= C_JR) vDst = *pc + (int8_t)v8Src;
            DA(C_NN); DA(vDst);
            break;
        case C_ADD: case C_ADC: case C_SUB: case C_SBC:
        case C_AND: case C_OR: case C_XOR: case C_CP:
            // cmd reg DST,SRC/N
            DA(C_DST); DA(C_COMMA);
            if(ops & CN) { DA(C_N); DA(v8Src); } else DA(C_SRC);
            break;
        case C_RST:
            DA(C_NUM); DA(code & 56);
            break;
        case C_IM:
            DA(C_NUM); DA((code >> 3) & 3);
            break;
        case C_IN:
            if(ops & CN) {
                DA(C_A); DA(C_COMMA); DA(C_N); DA(v8Src);
            } else {
                DA(C_SRC); DA(C_COMMA); DA(C_PC);
            }
            break;
        case C_OUT:
            if(ops & CN) {
                DA(C_N); DA(v8Src); DA(C_COMMA); DA(C_A);
            } else {
                DA(C_PC); DA(C_COMMA); DA(C_SRC);
            }
            break;
        case C_LD:
            switch(m->ops) {
                case O_ASSIGN:
                    // cmd reg, reg/N/NN
                    DA(C_DST); DA(C_COMMA);
                    if(ops & CN) { DA(C_N); DA(v8Src); }
                    else if(ops & CNN) { DA(C_NN); DA(vSrc); }
                    else DA(C_SRC);
                    break;
                case O_LOAD:
                    // cmd reg, (NN)/(RP)/(HL/IX/IY + D)
                    DA(C_DST); DA(C_COMMA);
                    if(ops & CNN) { DA(C_PNN); DA(vSrc); }
                    else if(regSrc >= _RBC) DA(C_PBC + (code >> 4));
                    else DA(C_SRC);
                    break;
                case O_SAVE:
                    // cmd (NN)/(RP)/(HL/IX/IY + D), reg/N
                    if(ops & CNN) { DA(C_PNN); DA(vDst); }
                    else if(regDst >= _RBC) DA(C_PBC + (code >> 4));
                    else DA(C_DST); // (HL/IX/IY + D)
                    DA(C_COMMA);
                    if(ops & CN) { DA(C_N); DA(v8Src); }
                    else DA(C_SRC);
                    break;
            }
            break;
    }
    // заголовок(PC CODE)
    if(flags & DA_PC) {
        static const char* bpTypes[] = { "\t", "*\t", "+\t", "*+\t"};
        ssh_strcpy(&daResult, ssh_fmtValue(_pc, ZX_FV_NUM16, true));
        ssh_strcpy(&daResult, bpTypes[bp]);
    }
    if(flags & DA_CODE) {
        char* daCode = (char*)&TMP_BUF[65536 + 256]; auto cod = daCode;
        int length = *pc - _pc;
        for(int i = 0; i < length; i++) ssh_strcpy(&daCode, ssh_fmtValue(rm8((uint16_t)(_pc + i)), (i != (length - 1)) * 2, true));
        for(int i = 0 ; i < 5 - length; i++) *daCode++ = '\t';
        ssh_strcpy(&daResult, cod);
    }
    auto posMnemonic = daResult;
    // сама инструкция
    int idx = 0;
    char* lex(nullptr);
    int n(0), o(0), a(0);
    while(idx < pos) {
        auto token = codes[idx++];
        bool offs = false;
        if(token >=  C_N) {
            static int fmtTypes[] = {ZX_FV_OPS8, ZX_FV_OPS16, ZX_FV_PADDR16, ZX_FV_NUMBER, ZX_FV_CVAL, ZX_FV_PVAL };
            n = codes[idx++];
            lex = ssh_fmtValue(n, fmtTypes[token - C_N] , true);
            if(token == C_PNN) {
                if(flags & DA_PN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue(rm8((uint16_t)n), ZX_FV_PVAL, true);
                }
            }
        } else switch(token) {
                case C_PHL: // DDCB
                    n = getDaOperand(_RPHL, _RN, prefix);
                    ssh_strcpy(&daResult, namesCode[n]);
                    o = (int8_t) offsSrc;
                    a = vSrc;
                    offs = true;
                    break;
                case C_SRC:
                    n = getDaOperand(regSrc, regDst, prefix);
                    o = (int8_t) offsSrc;
                    a = vSrc;
                    offs = prefix && regSrc == _RPHL;
                    break;
                case C_DST:
                    n = getDaOperand(regDst, regSrc, prefix);
                    o = (int8_t) offsDst;
                    a = vDst;
                    offs = prefix && regDst == _RPHL;
                    break;
                default:
                    lex = (char *) namesCode[token];
            }
        if(token == C_SRC || token == C_DST || token == C_PHL) {
            ssh_strcpy(&daResult, namesCode[n]);
            if(offs) {
                lex = ssh_fmtValue(o, ZX_FV_OFFS, true);
                if(flags & DA_PNN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue(a, ZX_FV_CVAL, true);
                }
                if(flags & DA_PN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue(rm8((uint16_t)a), ZX_FV_PVAL, true);
                }
            } else lex = nullptr;
        }
        ssh_strcpy(&daResult, lex);
    }
    // эпилог (TICKS)
    if(flags & DA_TICKS) {
        auto l = strlen(posMnemonic) / 4;
        for (int i = 0; i < (7 - l); i++) *daResult++ = '\t';
        ssh_strcpy(&daResult, "; ");
        ssh_strcpy(&daResult, ssh_fmtValue(ticks, ZX_FV_NUMBER, false));
    }
    return result;
}

#pragma clang diagnostic pop