//
// Created by Сергей on 03.12.2019.
//

#include "zxCommon.h"
#include "stkMnemonic.h"
#include "zxDA.h"

#define DA(token)   codes[pos++] = token

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

static uint8_t cnvRegs[] = {
        C_C, C_E, C_L, C_F, C_SP, C_B, C_D, C_H, C_A, C_I, C_R, C_PHL, C_A, 0, 0, 0, C_BC, C_DE, C_HL, C_AF, C_SP, 0, 0, 0,
        C_C, C_E, C_XL, C_F, C_SP, C_B, C_D, C_XH, C_A, C_I, C_R, C_PIX, C_A, 0, 0, 0, C_BC, C_DE, C_IX, C_AF, C_SP, 0, 0, 0,
        C_C, C_E, C_YL, C_F, C_SP, C_B, C_D, C_YH, C_A, C_I, C_R, C_PIY, C_A, 0, 0, 0, C_BC, C_DE, C_IY, C_AF, C_SP, 0, 0, 0
};

int zxDA::getOperand(uint8_t o, uint8_t oo, int prefix, uint16_t* v16, uint16_t *pc, int* ticks, uint8_t* offset) {
    switch(o) {
        case _C8:
            *v16 = rm8((*pc)++);
            break;
        case _C16:
            *v16 = rm16(*pc);
            *pc += 2;
            break;
        case _RPHL:
            if(pc && offset) {
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

MNEMONIC* zxDA::skipPrefix(uint16_t* pc, uint16_t* code, int* pref, int* prefix, int* ticks, uint16_t* v, uint8_t* offs) {
    int offset(0), pr(0), cod(0);
    MNEMONIC* m(nullptr);

    while(true) {
        cod = rm8((*pc)++) + offset;
        *code = (uint16_t)cod;
        m = &mnemonics[cod];
        if(m->ops != O_PREF) break;
        *ticks += 4;
        offset = m->flags << 8;
        if(offset == 256 && pr) {
            getOperand(_RPHL, _N_, pr, v, pc, ticks, offs);
            *pref = pr;
        }
        pr = m->tiks & 31;
    }
    *prefix = pr;
    return m;
}

size_t zxDA::cmdParser(uint16_t* pc, uint16_t* buffer, bool regSave) {
    uint8_t offsSrc(0), offsDst(0);
    uint16_t vSrc(0), vDst(0), code;
    int ticks(0), pref(0), prefix(0), n;

    auto _pc = *pc;
    auto buf = buffer;

    *buffer++ = _pc;

    auto m = skipPrefix(pc, &code, &pref, &prefix, &ticks, &vSrc, &offsSrc);

    auto regDst = m->regDst;
    auto regSrc = m->regSrc;

    getOperand(regDst, regSrc, prefix, &vDst, pc, &ticks, &offsDst);
    getOperand(regSrc, regDst, prefix, &vSrc, pc, &ticks, &offsSrc);
    // кэшированные значения
    *buffer++ = (uint16_t)pref; *buffer++ = (uint16_t)prefix;
    *buffer++ = vDst; *buffer++ = vSrc;
    *buffer++ = offsDst; *buffer++ = offsSrc;
    if(pref || regSrc == _C16 || regSrc == _RPHL) {
        n = rm16(vSrc);
    } else if(regDst == _C16 || regDst == _RPHL) {
        n = rm16(vDst);
    } else n = 0;
    *buffer++ = (uint16_t)n; *buffer++ = code;
    // длина кодов и они сами
    auto codeLength = (uint16_t)(*pc - _pc);
    *buffer++ = codeLength;
    while(codeLength-- > 0) *buffer++ = rm8(_pc++);
    // значения регистров(если надо)
    *buffer++ = (uint16_t)regSave;
    if(regSave) {
        auto cpu = ALU->cpu;
        *buffer++ = *cpu->_AF; *buffer++ = *cpu->_BC; *buffer++ = *cpu->_DE; *buffer++ = *cpu->_HL;
        *buffer++ = *cpu->_IX; *buffer++ = *cpu->_IY; *buffer++ = *cpu->_SP;
    }
    return (size_t)(buffer - buf);
}

const char* zxDA::cmdToString(uint16_t* buffer, char* daResult, int flags) {
    auto result = daResult;
    ssh_memzero(daResult, 512);

    int pos(0), n(0), a(0), o(0);
    uint16_t codes[64];

    auto _pc = *buffer++;
    auto pref = *buffer++; auto prefix = *buffer++;
    auto vDst = *buffer++; auto vSrc = *buffer++;
    auto offsDst = *buffer++; auto offsSrc = *buffer++;
    auto val16 = *buffer++; auto code = *buffer++;
    auto m = &mnemonics[code];
    auto cmd = m->name;
    auto fl = m->flags;
    auto regDst = m->regDst;
    auto regSrc = m->regSrc;

    code &= 255;

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
            if(code == 233) { DA(C_DST); break; }
        case C_CALL: case C_DJNZ: case C_JR:
            // cmd [flag,] NN
            if(fl) { DA(C_FNZ + fl - 1); DA(C_COMMA); }
            if(cmd <= C_JR) vSrc = (uint16_t)(_pc + 2) + (int8_t)vSrc;
            DA(C_NN); DA(vSrc);
            break;
        case C_ADD: case C_ADC: case C_SUB: case C_SBC:
        case C_AND: case C_OR: case C_XOR: case C_CP:
            // cmd reg DST,SRC/N
            DA(C_DST); DA(C_COMMA);
            if(regSrc == _C8) { DA(C_N); DA(vSrc); } else DA(C_SRC);
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
            if(regSrc == _C8) { DA(C_N); DA(vSrc); } else DA(C_PC);
            break;
        case C_OUT:
            if(regSrc == _C8) { DA(C_N); DA(vSrc); } else DA(C_PC);
            DA(C_COMMA); DA(C_DST);
            break;
        case C_LD:
            switch(m->ops) {
                case O_ASSIGN:
                    // cmd reg, reg/N/NN
                    DA(C_DST); DA(C_COMMA);
                    if(regSrc == _C8) { DA(C_N); DA(vSrc); }
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
                    if(regSrc == _C8) { DA(C_N); DA(vSrc); }
                    else DA(C_SRC);
                    break;
            }
            break;
    }
    // проверка на метку
    auto hex = opts[ZX_PROP_SHOW_HEX];
    if(flags & DA_LABEL) {
        auto label = searchLabel(_pc);
        if (label) {
            ssh_strcpy(&daResult, label);
            ssh_strcpy(&daResult, ":\n");
        }
    }
    // заголовок
    if(flags & DA_PC) {
        static const char* bpTypes[] = { "   ", " * ", " + ", " *+"};
        auto isExec = ALU->quickCheckBPs(_pc, ZX_BP_EXEC) != nullptr;
        auto isMem = ALU->quickCheckBPs(_pc, ZX_BP_WMEM) != nullptr;
        ssh_strcpy(&daResult, ssh_fmtValue(_pc, ZX_FV_NUM16, true));
        ssh_strcpy(&daResult, bpTypes[isExec | (isMem << 1)]);
    }
    if(flags & DA_CODE) {
        char* daCode = (char*)&TMP_BUF[65536 + 256]; auto cod = daCode;
        int length = *buffer++;
        for(int i = 0; i < length; i++) ssh_strcpy(&daCode, ssh_fmtValue(*buffer++, (i != (length - 1)) * 2, true));
        //auto l = strlen(cod);
        //ssh_strcpy(&daResult, l >= 8 ? "\t" : "\t\t");
        length = strlen(cod);
        n = hex ? 12 : 16; ssh_char(&daCode, ' ', n - length);
        *daCode = 0;
        ssh_strcpy(&daResult, cod);
    }
    auto posMnemonic = daResult;
    opts[ZX_PROP_JNI_RETURN_VALUE + 1] = (uint8_t)(daResult - result);
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
                if(flags & DA_LABEL) {
                    auto label = searchLabel(n);
                    ssh_strcpy(&daResult, label);
                }
                if(flags & DA_PN) {
                    ssh_strcpy(&daResult, lex);
                    bool is16 = regDst & _R16 || regSrc & _R16;
                    if(!is16) val16 = (uint8_t)val16;
                    lex = ssh_fmtValue(val16, is16 ? ZX_FV_PVAL16 : ZX_FV_PVAL8, true);
                }
            }
        } else switch(token) {
                case C_CB_PHL: // DDCB
                    n = getOperand(_RPHL, _N_, pref);
                    o = (int8_t) offsSrc;
                    a = vSrc;
                    offs = true;
                    break;
                case C_SRC:
                    n = getOperand(regSrc, regDst, prefix);
                    o = (int8_t) offsSrc;
                    a = vSrc;
                    offs = prefix && regSrc == _RPHL;
                    break;
                case C_DST:
                    n = getOperand(regDst, regSrc, prefix);
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
                if (flags & DA_PNN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue(a, ZX_FV_CVAL, true);
                }
            } else lex = nullptr;
            if((token == C_DST && regDst == _RPHL) || token == C_CB_PHL || (token == C_SRC && regSrc == _RPHL)) {
                if (flags & DA_PN) {
                    ssh_strcpy(&daResult, lex);
                    lex = ssh_fmtValue((uint8_t) val16, ZX_FV_PVAL8, true);
                }
            }
        }
        ssh_strcpy(&daResult, lex);
    }
    // эпилог (REGS)
    if(flags & DA_REGS) {
        auto flag = *buffer++;
        if(flag != 0) {
            auto length = strlen(posMnemonic);
            n = hex ? 20 : 24; ssh_char(&daResult, ' ', n - length);
            // AF
            ssh_strcpy(&daResult, "; AF = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer++, ZX_FV_CVAL, true));
            // BC
            ssh_strcpy(&daResult, " BC = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer++, ZX_FV_CVAL, true));
            // DE
            ssh_strcpy(&daResult, " DE = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer++, ZX_FV_CVAL, true));
            // HL
            ssh_strcpy(&daResult, " HL = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer++, ZX_FV_CVAL, true));
            // IX
            ssh_strcpy(&daResult, " IX = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer++, ZX_FV_CVAL, true));
            // IY
            ssh_strcpy(&daResult, " IY = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer++, ZX_FV_CVAL, true));
            // SP
            ssh_strcpy(&daResult, " SP = ");
            ssh_strcpy(&daResult, ssh_fmtValue(*buffer, ZX_FV_CVAL, true));
        }
    }
    return result;
}

const char* zxDA::searchLabel(int address) {
    if(address < 16384) {
        auto ptr = labels;
        int count = *(uint16_t *) ptr;
        ptr += sizeof(uint16_t);
        for (int i = 0; i < count; i++) {
            int len = *ptr++;
            if (*(uint16_t *) ptr == address) return (const char *) (ptr + 2);
            ptr += len;
        }
    }
    return nullptr;
}

#pragma clang diagnostic pop