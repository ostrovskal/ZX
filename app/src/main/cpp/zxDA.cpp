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

static uint8_t cnvRegs[] = {
        C_C, C_E, C_L, C_F, C_SP, C_B, C_D, C_H, C_A, C_I, C_R, C_PHL, C_A, 0, 0, 0, C_BC, C_DE, C_HL, C_AF, C_SP, 0, 0, 0,
        C_C, C_E, C_XL, C_F, C_SP, C_B, C_D, C_XH, C_A, C_I, C_R, C_PIX, C_A, 0, 0, 0, C_BC, C_DE, C_IX, C_AF, C_SP, 0, 0, 0,
        C_C, C_E, C_YL, C_F, C_SP, C_B, C_D, C_YH, C_A, C_I, C_R, C_PIY, C_A, 0, 0, 0, C_BC, C_DE, C_IY, C_AF, C_SP, 0, 0, 0
};

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

int zxDA::getDaOperand(uint8_t o, uint8_t oo, int prefix, uint16_t* v16, uint8_t* v8, uint16_t *pc, int* ticks, uint8_t* offset) {
    switch(o) {
        case _C8:
            *v8 = rm8((*pc)++);
            break;
        case _C16:
            *v16 = rm16(*pc);
            *pc += 2;
            break;
        case _RPHL:
            if(pc) {
                *offset = 0;
                if (prefix) { *ticks += 8; *offset = rm8((*pc)++); }
                // виртуальный адрес [HL/IX/IY + D]
                *v16 = *(uint16_t*)zxCPU::regs[_RL + prefix] + (int8_t)*offset;
            }
    }
    // индекс имени регистра
    if(oo == _RPHL) prefix = 0;
    return cnvRegs[(prefix << 1) + o];
}

char* zxDA::daMake(uint16_t* pc, int flags, int bp, int prefix, int offset, int ticks) {
    uint8_t v8Src(0), v8Dst(0), offsSrc(0), offsDst(0);
    uint16_t vSrc(0), vDst(0);
    uint8_t pos = 0;
    int n(0), o(0), a(0);
    int pref = 0;

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
        getDaOperand(_RPHL, _N_, prefix, &vSrc, &v8Src, pc, &ticks, &offsSrc);
        pref = prefix; prefix = 0;
    }

    auto code = rm8((*pc)++);

    auto m = &mnemonics[code + offset];
    auto regDst = m->regDst;
    auto regSrc = m->regSrc;

    getDaOperand(regDst, regSrc, prefix, &vDst, &v8Dst, pc, &ticks, &offsDst);
    getDaOperand(regSrc, regDst, prefix, &vSrc, &v8Src, pc, &ticks, &offsSrc);
    ticks += m->tiks;

    if(m->ops == O_PREF) {
        auto prefIX = code == PREF_DD;
        if(prefix && code != PREF_CB) {
            (*pc)--;
            return (char*)namesCode[prefIX ? C_IX_NONI : C_IY_NONI];
        }
        switch(code) {
            case PREF_CB: return daMake(pc, flags, bp, prefix, 256, ticks);
            case PREF_ED: return daMake(pc, flags, bp, 0, 512, ticks);
            case PREF_DD: case PREF_FD: return daMake(pc, flags, bp, prefIX ? 12 : 24, 0, ticks);
        }
    }

    auto cmd = m->name;
    auto fl = m->flags;

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
            if(pref) {
                DA(C_CB_PHL); if(regDst == _RPHL) break;
                DA(C_COMMA);
            }
        case C_INC: case C_DEC:
        case C_PUSH: case C_POP:
        case C_EX_SP:
            DA(C_DST);
            break;
        case C_JP:
            if(code == 233) { DA(C_HL); break; }
        case C_CALL: case C_DJNZ: case C_JR:
            // cmd [flag,] NN
            if(fl) { DA(C_FNZ + fl - 1); DA(C_COMMA); }
            if(cmd <= C_JR) vSrc = *pc + (int8_t)v8Src;
            DA(C_NN); DA(vSrc);
            break;
        case C_ADD: case C_ADC: case C_SUB: case C_SBC:
        case C_AND: case C_OR: case C_XOR: case C_CP:
            // cmd reg DST,SRC/N
            DA(C_DST); DA(C_COMMA);
            if(regSrc == _C8) { DA(C_N); DA(v8Src); } else DA(C_SRC);
            break;
        case C_RST:
            DA(C_NUM); DA(code & 56);
            break;
        case C_IM:
            n = (code >> 3) & 3;
            DA(C_NUM); DA(n ? n - 1 : 0);
            break;
        case C_IN:
            DA(C_DST); DA(C_COMMA);
            if(regSrc == _C8) { DA(C_N); DA(v8Src); } else DA(C_PC);
            break;
        case C_OUT:
            if(regSrc == _C8) { DA(C_N); DA(v8Src); } else DA(C_PC);
            DA(C_COMMA); DA(C_DST);
            break;
        case C_LD:
            switch(m->ops) {
                case O_ASSIGN:
                    // cmd reg, reg/N/NN
                    DA(C_DST); DA(C_COMMA);
                    if(regSrc == _C8) { DA(C_N); DA(v8Src); }
                    else if(regSrc == _C16) { DA(C_NN); DA(vSrc); }
                    else DA(C_SRC);
                    break;
                case O_LOAD:
                    // cmd reg, (NN)/(RP)/(HL/IX/IY + D)
                    DA(C_DST); DA(C_COMMA);
                    if(regSrc == _C16) { DA(C_PNN); DA(vSrc); }
                    else if(regSrc & _R16) DA(C_PBC + (code >> 4));
                    else DA(C_SRC);
                    break;
                case O_SAVE:
                    // cmd (NN)/(RP)/(HL/IX/IY + D), reg/N
                    if(regDst == _C16) { DA(C_PNN); DA(vDst); }
                    else if(regDst & _R16) DA(C_PBC + (code >> 4));
                    else DA(C_DST); // (HL/IX/IY + D)
                    DA(C_COMMA);
                    if(regSrc == _C8) { DA(C_N); DA(v8Src); }
                    else DA(C_SRC);
                    break;
            }
            break;
    }
    // проверка на метку
    auto label = searchLabel(_pc);
    if(label) {
        ssh_strcpy(&daResult, label);
        ssh_strcpy(&daResult, ":\r\n");
    }
    // заголовок
    auto hex = opts[ZX_PROP_SHOW_HEX];
    if(flags & DA_PC) {
        static const char* bpTypes[] = { "\t", "*\t", "+\t", "*+\t"};
        ssh_strcpy(&daResult, ssh_fmtValue(_pc, ZX_FV_NUM16, true));
        ssh_strcpy(&daResult, bpTypes[bp]);
    }
    if(flags & DA_CODE) {
        char* daCode = (char*)&TMP_BUF[65536 + 256]; auto cod = daCode;
        int length = *pc - _pc;
        for(int i = 0; i < length; i++) ssh_strcpy(&daCode, ssh_fmtValue(rm8((uint16_t)(_pc + i)), (i != (length - 1)) * 2, true));
        auto l = strlen(cod);
        ssh_strcpy(&daResult, cod);
        ssh_strcpy(&daResult, l >= 8 ? "\t" : "\t\t");
/*
        length = strlen(cod) / 4;
        n = hex ? 3 : 4; n -= length;
        while(n-- > 0 ) *daCode++ = '\t';
*/
    }
    auto posMnemonic = daResult;
    // сама инструкция
    int idx = 0;
    char* lex(nullptr);
    while(idx < pos) {
        auto token = codes[idx++];
        bool offs = false;
        if(token >=  C_N) {
            static int fmtTypes[] = {ZX_FV_OPS8, ZX_FV_OPS16, ZX_FV_PADDR16, ZX_FV_NUMBER, ZX_FV_CVAL, ZX_FV_PVAL8 };
            n = codes[idx++];
            lex = ssh_fmtValue(n, fmtTypes[token - C_N] , true);
            if(token == C_PNN) {
                label = searchLabel(n);
                ssh_strcpy(&daResult, label);
                if(flags & DA_PN) {
                    ssh_strcpy(&daResult, lex);
                    bool is16 = regDst & _R16 || regSrc & _R16;
                    if(is16) n = rm16((uint16_t)n); else n = rm8((uint16_t)n);
                    lex = ssh_fmtValue(n, is16 ? ZX_FV_PVAL16 : ZX_FV_PADDR8, true);
                }
            }
        } else switch(token) {
                case C_CB_PHL: // DDCB
                    n = getDaOperand(_RPHL, _N_, pref);
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
        if(token == C_SRC || token == C_DST || token == C_CB_PHL) {
            ssh_strcpy(&daResult, namesCode[n]);
            if(offs) {
                lex = ssh_fmtValue(o, ZX_FV_OFFS, true);
                if(flags & DA_PNN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue(a, ZX_FV_CVAL, true);
                }
                if(flags & DA_PN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue(rm8((uint16_t)a), ZX_FV_PVAL8, true);
                }
            } else lex = nullptr;
        }
        ssh_strcpy(&daResult, lex);
    }
    // эпилог (TICKS)
    if(flags & DA_TICKS) {
        auto length = strlen(posMnemonic) / 4;
/*
        n = hex ? 5 : 6; n -= length;
        while(n-- > 0) *daResult++ = '\t';
*/
        ssh_strcpy(&daResult, "; ");
        ssh_strcpy(&daResult, ssh_fmtValue(ticks, ZX_FV_NUMBER, false));
    }
    return result;
}

const char* zxDA::searchLabel(int address) {
    auto ptr = labels;
    int count = *(uint16_t*)ptr; ptr += sizeof(uint16_t);
    for(int i = 0 ; i < count; i++) {
        int len = *ptr++;
        if(*(uint16_t*)ptr == address) return (const char*)(ptr + 2);
        ptr += len;
    }
    return nullptr;
}

#pragma clang diagnostic pop