//
// Created by Сергей on 17.12.2019.
//

#include "zxCommon.h"
#include "zxAssembler.h"
#include "stkMnemonic.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
struct ASM_CODE {
    uint8_t pref;
    uint8_t bas_cod;
    uint8_t alt_cod;
    uint8_t pcount;// 0 -> нет, 1 -> 1, 2 -> 2, 3 -> 1 или 2, 4 -> 2 или 3, 5 -> 0 или 1
};
#pragma clang diagnostic pop

static ASM_CODE asm_cod[] = {
        { 0x00, 0b00000000, 0b00000000, 2 },/* LD   */
        { 0x00, 0b11000011, 0b11000010, 3 },/* JP*/ { 0x00, 0b11001101, 0b11000100, 3 },/* CALL */ { 0x00, 0x10, 0x10, 1 },/* DJNZ */ { 0x00, 0x18, 0b00100000, 3 },/* JR */
        { 0xCB, 0x40, 0x40, 4 },/* BIT  */          { 0xCB, 0x80, 0x80, 4 },/* RES  */             { 0xCB, 0xC0, 0xC0, 4 },/* SET  */
        { 0x00, 0b00000011, 0b00000100, 1 },/* INC*/{ 0x00, 0b00001011, 0b00000101, 1 },/* DEC  */
        { 0x00, 0b11101011, 0b11100011, 2 },/* EX DE,HL/EX AF,AF' */
        { 0xED, 0b01000110, 0b01000110, 1 },/* IM 1/2/3 */
        { 0x00, 0b10000000, 0xC6, 3 },/* ADD  */    { 0x00, 0b10001000, 0xCE, 3 },/* ADC  */ { 0, 0b10010000, 0xD6, 3 },/* SUB */  { 0x00, 0b10011000, 0xDE, 3 },/* SBC */
        { 0x00, 0b10100000, 0xE6, 3 },/* AND  */    { 0x00, 0b10101000, 0xEE, 3 },/* XOR  */ { 0, 0b10110000, 0xF6, 3 },/* OR  */  { 0x00, 0b10111000, 0xFE, 3 },/* CP  */
        { 0xCB, 0x00, 0x00, 3 },/* RLC  */          { 0xCB, 0x08, 0x08, 3 },/* RRC  */
        { 0xCB, 0x10, 0x10, 3 },/* RL   */          { 0xCB, 0x18, 0x18, 3 },/* RR   */
        { 0xCB, 0x20, 0x20, 3 },/* SLA  */          { 0xCB, 0x28, 0x28, 3 },/* SRA  */
        { 0xCB, 0x30, 0x30, 3 },/* SLL  */          { 0xCB, 0x38, 0x38, 3 },/* SRL  */
        { 0xED, 0x40, 0xDB, 2 },/* IN   */          { 0xED, 0x41, 0xD3, 2 },/* OUT  */
        { 0x00, 0b11000111, 0b11000111, 1 },/* RST*/
        { 0x00, 0b11000101, 0b11000101, 1 },/*PUSH*/{ 0x00, 0b11000001, 0b11000001, 1 },/* POP  */ { 0, 0b11001001, 0b11000000, 5 },/* RET  */
        { 0x00, 0x00, 0x00 },/* NOP  */
        { 0x00, 0x08, 0x08, 2 },/* EX AF, AF  */
        { 0x00, 0x07, 0x07 },/* RLCA */          { 0x00, 0x0F, 0x0F },/* RRCA */
        { 0x00, 0x17, 0x17 },/* RLA  */          { 0x00, 0x1F, 0x1F },/* RRA  */
        { 0x00, 0x27, 0x27 },/* DAA  */          { 0x00, 0x2F, 0x2F },/* CPL  */ { 0x00, 0x37, 0x37 },/* SCF  */ { 0x00, 0x3F, 0x3F },/* CCF  */
        { 0x00, 0xF3, 0xF3 },/* DI   */          { 0x00, 0xFB, 0xFB },/* EI   */
        { 0xED, 0x67, 0x67 },/* RRD  */          { 0xED, 0x6F, 0x6F },/* RLD  */
        { 0xED, 0xA0, 0xA0 },/* LDI  */          { 0xED, 0xA1, 0xA1 },/* CPI  */ { 0xED, 0xA2, 0xA2 },/* INI  */ { 0xED, 0xA3, 0xA3 },/* OTI  */
        { 0xED, 0xA8, 0xA8 },/* LDD  */          { 0xED, 0xA9, 0xA9 },/* CPD  */ { 0xED, 0xAA, 0xAA },/* IND  */ { 0xED, 0xAB, 0xAB },/* OTD  */
        { 0xED, 0xB0, 0xB0 },/* LDIR */          { 0xED, 0xB1, 0xB1 },/* CPIR */ { 0xED, 0xB2, 0xB2 },/* INIR */ { 0xED, 0xB3, 0xB3 },/* OTIR */
        { 0xED, 0xB8, 0xB8 },/* LDDR */          { 0xED, 0xB9, 0xB9 },/* CPDR */ { 0xED, 0xBA, 0xBA },/* INIR */ { 0xED, 0xBB, 0xBB },/* OTIR */
        { 0x00, 0xD9, 0xD9 },/* EXX  */          { 0x00, 0xEB, 0xEB },/* EX DE,*/{ 0x00, 0xE3, 0xE3, 2 },/* EX (SP), */
        { 0xED, 0x4D, 0x4D },/* RETI */          { 0xED, 0x45, 0x45 },/* RETN */
        { 0x00, 0x76, 0x76 },/* HALT */
        { 0xED, 0x44, 0x44 },/* NEG  */
        { 0x00, 0xDD, 0xDD },/* *IX* */          { 0x00, 0xFD, 0xFD },/* *IY* */ { 0xED, 0x00, 0x00 } /* *ED* */
};

void zxAssembler::skip_spc(char** text) {
    auto txt = *text;
    txt--;
    while(*++txt == ' ') { }
    *text = txt;
}

int zxAssembler::get_cmd(const char* cmd, int len) {
    int i, idx = -1;
    const char* name;
    char ch = 0;
    while((name = namesCode[++idx])) {
        for(i = 0 ; i < len; i++) {
            ch = name[i];
            if(ch < 33) break;
            if(ch != cmd[i]) break;
        }
        if(i == len && name[i] <= ' ') return idx;
    }
    return ERROR_KEYWORD_NOT_FOUND;
}

int zxAssembler::get_word(char** text) {
    static uint8_t stop_sym[] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
                                  1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,1,1,1,1,1};
    skip_spc(text);
    char ch;
    auto txt = *text;
    cmd_begin = txt;
    auto current = cmd_begin - 1;
    while ((ch = *++current)) {
        if(ch > 127) break;
        if(stop_sym[ch]) break;
        *current &= -33;
    }
    auto len = (int)(current - cmd_begin);
    skip_spc(&current);
    *text = current;
    return len;
}

int zxAssembler::parse_operand(char** text) {
    // number/(number)/(HL/DE/BC)/(IX+-number/IY+-number)/REG/FLAG
    skip_spc(text);
    auto txt = *text;
    int ret;
    auto isBrakket = txt[0] == '(';
    if(isBrakket) { txt++; skip_spc(&txt); }
    cmd_begin = txt;
    auto tend = txt;
    // число
    auto n = *(int*)ssh_ston(txt, RADIX_DEC, &tend);
    if(tend > txt) {
        // число
        txt = tend;
        if(n < 0 || n > 65535) {
            // число вне диапазона
            ret = ERROR_NUMBER_OUT_OF_RANGE;
        } else {
            number = (uint16_t) n;
            ret = C_NN;
        }
    } else {
        auto len = get_word(&txt);
        ret = get_cmd(cmd_begin, len);
        if (ret < C_LD && ret >= 0) {
            // регистры
            cmd_begin = txt;
            if(isBrakket) {
                if(ret == C_IX || ret == C_IY) {
                    // disp
                    disp = 0;
                    n = *(int *) ssh_ston(txt, RADIX_DEC, &tend);
                    if(tend > txt) {
                        if (n < -128 || n > 127) {
                            // error - смещение вне диапазона
                            ret = ERROR_DISP_OUT_OF_RANGE;
                        } else {
                            disp = (uint8_t) n;
                        }
                        txt = tend;
                    } else {
                        // error - отсутствует смещение
                        ret = ERROR_DISP_NOT_FOUND;
                    }
                }
            }
        } else {
            // error - команда вместо регистра
            ret = ERROR_COMMAND_NOT_FOUND;
        }
    }
    if(ret >= 0) {
        skip_spc(&txt);
        if(isBrakket) {
            if(txt[0] != ')') {
                ret = ERROR_CLOSE_BRAKKET_NOT_FOUND;
            } else {
                txt++; skip_spc(&txt);
                // индекс ключевого слова привести к верному
                // IX+-/IY+-/BC/DE/number
                switch(ret) {
                    case C_NN: ret = C_PNN; break;
                    case C_IX: ret = C_PIX; break;
                    case C_IY: ret = C_PIY; break;
                    case C_HL: ret = C_PHL; break;
                    case C_DE: ret = C_PDE; break;
                    case C_BC: ret = C_PBC; break;
                    case C_SP: ret = C_PSP; break;
                    case C_C:  ret = C_PC;  break;
                    default: ret = ERROR_INVALID_INDEXED_OPERATION;
                }
            }
        }
    }
    *text = txt;
    skip_spc(text);
    return ret;
}

int zxAssembler::parser(int address, const char *txt) {
    // 1. ищем инструкцию
    int ret = 0;
    text = (char*)txt;
    auto len = get_word(&text);
    idx_cmd = get_cmd(cmd_begin, len);
    // 2. в зависимости от типа - переходим на процедуру детального парсинга
    if(idx_cmd >= C_LD && idx_cmd < C_IX_NONI) {
        cmd_begin = text;
        ret = cmd_parser((uint16_t)address);
    }
    // 3. регистрируем ошибка, при наличии
    if(ret != ERROR_OK) {
        // error
        if(ret == ERROR_CLOSE_BRAKKET_NOT_FOUND) {
            auto t = strchr(cmd_begin, ')');
            text = (t == nullptr ? (char*)(txt + strlen(txt)) : t);
        }
        opts[ZX_PROP_JNI_RETURN_VALUE] = (uint8_t)(cmd_begin - txt);
        opts[ZX_PROP_JNI_RETURN_VALUE + 1] = (uint8_t)(text - txt);
    } else {
        int idx = INDEX_TEMP;
        int count = (int)(buf - &TMP_BUF[INDEX_TEMP]);
        uint16_t idx_buf = (uint16_t)address;
        while(count--) {
            *realPtr(idx_buf++) = TMP_BUF[idx++];
        }
    }
    return ret;
}

#define PAR_COUNT_EMPTY         0 // C_NULL
#define PAR_COUNT_ONE           1
#define PAR_COUNT_TWO           2
#define PAR_COUNT_ONE_OR_TWO    3
#define PAR_COUNT_TWO_OR_THREE  4

int zxAssembler::cmd_parser(uint16_t addr) {
    static uint8_t asm_ops[]   = { 1, 0, 3, 2, 5, 4, 255, 7, 0, 6, 255, 0, 1, 2, 3, 3,
                                   0, 1, 2, 3, 4, 5, 6, 7, 5, 4, 5, 4, 6, 2, 2, 6, 6, 0, 1, 255 };
    static uint8_t asm_prefs[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   221, 221, 253, 253, 48, 221, 253, 221, 253, 0, 0, 0 };
    int src(0), tmp;
    uint8_t dstPref(0), srcPref(0), dstOps(0);
    int16_t dstNum(0);
    buf = &TMP_BUF[INDEX_TEMP];
    auto stk = &asm_cod[idx_cmd - C_LD];
    auto cmd_op1 = text;
    auto cmd_op2 = text;
    auto dst = parse_operand(&text);
    if(dst < 0) return dst;
    switch(stk->pcount) {
        case PAR_COUNT_EMPTY:
            if(dst != C_NULL) return ERROR_INVALID_COMMAND;
            break;
        case PAR_COUNT_ONE:
            dstNum = number;
            break;
        case PAR_COUNT_TWO:
        case PAR_COUNT_TWO_OR_THREE:
            if(skipComma(&text)) return ERROR_COMMA_NOT_FOUND;
            dstNum = number;
            cmd_op2 = text;
            src = parse_operand(&text);
            if(stk->pcount == PAR_COUNT_TWO_OR_THREE) {
                if(dst != C_NN || dstNum > 7) return ERROR_INVALID_COMMAND;
                dst = src; src = C_NULL;
                if(text[0] == ',') {
                    text++;
                    cmd_op2 = text;
                    src = parse_operand(&text);
                }
            }
            break;
        case PAR_COUNT_ONE_OR_TWO:
            if(text[0] == ',') {
                text++;
                cmd_op2 = text;
                src = parse_operand(&text);
            } else { src = dst; dst = C_NULL; }
            break;
    }
    if(src < 0) return ERROR_INVALID_COMMAND;
    if(text[0] != 0) {
        cmd_begin = text;
        text += strlen(text);
        return ERROR_EXPECT_END_OF_COMMAD;
    }
    switch(dst) {
        case C_IX: case C_IY: case C_PIX: case C_PIY:
        case C_XH: case C_YH: case C_XL: case C_YL:
            dstPref = asm_prefs[dst];
            break;
    }
    switch(src) {
        case C_IX: case C_IY: case C_PIX: case C_PIY:
        case C_XH: case C_YH: case C_XL: case C_YL:
            srcPref = asm_prefs[src];
            if((dst == C_HL || dst == C_L || dst == C_H || (dst >= C_XL && dst <= C_PIY)) && dstPref != srcPref) {
                cmd_begin = cmd_op2;
                return ERROR_INVALID_COMMAND;
            }
            break;
    }
    auto prf = stk->pref;
    auto cod = stk->bas_cod;
    if(prf == 0xED && (dstPref || srcPref)) {
        cmd_begin = dstPref ? cmd_op1 : cmd_op2;
        return ERROR_INVALID_COMMAND;
    }
    if(dstPref || srcPref) *buf++ = dstPref | srcPref;
    if(prf) *buf++ = prf;
    if(dst < C_LD) dstOps = asm_ops[dst];
    auto srcOps = asm_ops[src];
    switch(idx_cmd) {
        case C_IM:
            if(dst == C_NN && dstNum < 3) { *buf++ = (uint8_t)(cod | (((number & 3) + 1) << 3)); break; }
            cmd_begin = cmd_op1;
            return ERROR_INVALID_INTERRUPT_MODE;
        case C_RST:
            if(dst == C_NN && (number & ~56) == 0) { *buf++ = (uint8_t)(cod | number); break; }
            cmd_begin = cmd_op1;
            return ERROR_INVALID_RST_NUMBER;
        case C_EX:
            cmd_begin = cmd_op1;
            switch(dst) {
                // EX DE, HL
                case C_DE:
                    if(src == C_HL) { *buf++ = cod; return ERROR_OK; }
                    cmd_begin = cmd_op2;
                    break;
                // EX AF, AF
                case C_AF:
                    if(src == C_AF) { *buf++ = 0x08; return ERROR_OK; }
                    cmd_begin = cmd_op2;
                    break;
                    // EX (SP), HL/IX/IY
                case C_PSP:
                    if(src == C_IX || src == C_IY || src == C_HL) { *buf++ = stk->alt_cod; return ERROR_OK; }
                    cmd_begin = cmd_op2;
                    break;
            }
            return ERROR_INVALID_COMMAND;
        // INC/DEC REG8/REG16/(HL/IX/IY)
        case C_INC: case C_DEC:
            switch(dst) {
                case C_HL: case C_DE: case C_BC: case C_SP: case C_IX: case C_IY:
                    *buf++ = (uint8_t)(cod | (asm_ops[dst] << 4));
                    break;
                case C_PIX: case C_PIY:
                case C_XH: case C_XL: case C_YH: case C_YL:
                case C_B: case C_C: case C_D: case C_E: case C_H: case C_L: case C_A: case C_PHL:
                    *buf++ = (uint8_t)(stk->alt_cod | (dstOps << 3));
                    if(dst >= C_PIX) *buf++ = disp;
                    break;
                default:
                    cmd_begin = cmd_op1;
                    return ERROR_INVALID_COMMAND;
            }
            break;
        case C_RLC: case C_RRC: case C_RL: case C_RR: case C_SLA: case C_SRA: case C_SLI: case C_SRL:
            dstNum = (uint16_t)(idx_cmd - C_RLC);
            if(dst == C_NULL) { dst = src; src = C_NULL; }
        // CMD BIT,(PHL/PIX/PIY/SRC8)[,SRC8]
        case C_BIT: case C_RES: case C_SET:
            if(dst == C_PIX || dst == C_PIY) *buf++ = disp;
            switch(src) {
                case C_NULL: if(dstPref) srcOps = 6;
                case C_B: case C_C: case C_D: case C_E: case C_H: case C_L: case C_A:
                    *buf++ = (uint8_t)(cod | (dstNum << 3) | srcOps); break;
                default:
                    cmd_begin = cmd_op2;
                    return ERROR_INVALID_COMMAND;
            }
            break;
        case C_DJNZ:
            src = dst; dst = C_NULL;
        case C_JR:
            // JR [cc], NN
            cmd_begin = cmd_op1;
            if(checkFlags(dst, true)) {
                if(dst == C_C) dstOps = 3;
                cod = stk->alt_cod;
            } else if(dst != C_NULL) return ERROR_INVALID_COMMAND;
            cmd_begin = cmd_op2;
            if(src != C_NN) return ERROR_INVALID_ADDRESS_OPERAND;
            number -= (addr + 2);
            if(number < -128 || number > 127) return ERROR_RELATIVE_JUMP_OUT_OF_RANGE;
            *buf++ = (uint8_t)(cod | (dstOps << 3));
            *buf++ = (uint8_t)number;
            break;
        case C_JP:
            // JP IX/IY/HL
            if(src == C_IX || src == C_IY || src == C_HL) { *buf++ = 0xE9; return ERROR_OK; }
        case C_CALL:
            // cmd [cc], NN
            cmd_begin = cmd_op1;
            if(checkFlags(dst, false)) {
                if(dst == C_C) dstOps = 3;
                cod = stk->alt_cod;
            } else if(dst != C_NULL) return ERROR_INVALID_COMMAND;
            cmd_begin = cmd_op2;
            if(src != C_NN) return ERROR_INVALID_ADDRESS_OPERAND;
            *buf++ = (uint8_t)(cod | (dstOps << 3));
            *buf++ = (uint8_t)(number & 0xFF);
            *buf++ = (uint8_t)(number >> 8);
            break;
        case C_RET:
            // RET [cc]
            cmd_begin = cmd_op1;
            if(checkFlags(dst, false)) {
                if(dst == C_C) dstOps = 3;
                cod = stk->alt_cod;
            } else if(dst != C_NULL) return ERROR_INVALID_COMMAND;
            *buf++ = (uint8_t)(cod | (dstOps << 3));
            break;
        case C_PUSH: case C_POP:
            // PUSH/POP REG16
            cmd_begin = cmd_op1;
            if(checkReg16(dst, false)) {
                *buf++ = (uint8_t)(cod | (dstOps << 4)); break;
            } else return ERROR_INVALID_COMMAND;
            break;
        case C_OUT:
            cmd_begin = cmd_op2;
            if(!srcPref && (checkReg8(src) || src == C_NN)) {
                tmp = dst;
                if(dst == C_NN) {
                    if(src != C_A) return ERROR_INVALID_COMMAND;
                    if(dstNum > 255) {
                        cmd_begin = cmd_op1;
                        return ERROR_PARAMETER_OUT_OF_RANGE;
                    }
                    // OUT (N), A
                    buf--; cod = stk->alt_cod;
                    dst = C_PC; srcOps = 0;
                } else if(src == C_NN) {
                    if(number != 0) return ERROR_PARAMETER_OUT_OF_RANGE;
                    // OUT (C), 0
                    srcOps = 6;
                }
                if(dst != C_PC) {
                    cmd_begin = cmd_op1;
                    return ERROR_INVALID_COMMAND;
                }
                // OUT (C), REG8
                *buf++ = (uint8_t) (cod | (srcOps << 3));
                if (tmp == C_NN) *buf++ = (uint8_t)dstNum;
                break;
            } else return ERROR_INVALID_COMMAND;
            break;
        case C_IN:
            cmd_begin = cmd_op1;
            if(!dstPref && (checkReg8(dst) || dst == C_F)) {
                tmp = src;
                if(src == C_NN) {
                    if(dst != C_A) return ERROR_INVALID_COMMAND;
                    if(number > 255) {
                        cmd_begin = cmd_op2;
                        return ERROR_PARAMETER_OUT_OF_RANGE;
                    }
                    // IN A, (N)
                    buf--; cod = stk->alt_cod;
                    src = C_PC; dstOps = 0;
                }
                if(src != C_PC) {
                    cmd_begin = cmd_op2;
                    return ERROR_INVALID_COMMAND;
                }
                // IN REG8,(C)
                *buf++ = (uint8_t) (cod | (dstOps << 3));
                if (tmp == C_NN) *buf++ = (uint8_t) number;
                break;
            } else return ERROR_INVALID_COMMAND;
            break;
        case C_ADD:
            if(dst == C_IX || dst == C_IY || dst == C_HL) {
                if(checkReg16(src, true)) { *buf++ = (uint8_t)(0b00001001 | (asm_ops[src] << 4)); break; } else {
                    cmd_begin = cmd_op2;
                    return ERROR_INVALID_COMMAND;
                }
                return ERROR_OK;
            }
        case C_ADC: case C_SBC:
            if(dst == C_HL && idx_cmd != C_ADD) {
                if(!srcPref && checkReg16(src, true)) {
                    *buf++ = 0xED;
                    *buf++ = (uint8_t) ((idx_cmd == C_ADC ? 0b01001010 : 0b01000010) | (asm_ops[src] << 4));
                    return ERROR_OK;
                } else {
                    cmd_begin = cmd_op2;
                    return ERROR_INVALID_COMMAND;
                }
            }
        case C_SUB: case C_OR: case C_XOR: case C_AND: case C_CP:
            // ALU [A, ]N/SRC8
            if(dst == C_NULL) dst = C_A;
            if(dst != C_A) {
                cmd_begin = cmd_op1;
                return ERROR_INVALID_COMMAND;
            }
            cmd_begin = cmd_op2;
            switch(src) {
                case C_PIX: case C_PIY:
                case C_XH: case C_XL: case C_YH: case C_YL:
                case C_B: case C_C: case C_D: case C_E: case C_H: case C_L: case C_A: case C_PHL:
                    *buf++ = (uint8_t)(cod | srcOps);
                    if(srcPref) *buf++ = disp;
                    break;
                case C_NN:
                    if(number > 255) return ERROR_PARAMETER_OUT_OF_RANGE;
                    *buf++ = stk->alt_cod;
                    *buf++ = (uint8_t)number;
                    break;
                default: return ERROR_INVALID_COMMAND;
            }
            break;
        case C_LD:
            cmd_begin = cmd_op2;
            switch(dst) {
                case C_I: case C_R:
                    // LD I,A/LD R,A
                    if(src != C_A) return ERROR_INVALID_COMMAND;
                    *buf++ = 0xED; *buf++ = (uint8_t)(dst == C_I ? 0b01000111 : 0b01001111);
                    break;
                // LD (NN), SRC16
                case C_PNN:
                    switch(src) {
                        // LD (NN), IX/IY/HL/A
                        case C_IX: case C_IY: case C_A: case C_HL:
                            *buf++ = (uint8_t)(src == C_A ? 50 : 34);
                            *buf++ = (uint8_t)(number & 0xFF);
                            *buf++ = (uint8_t)(number >> 8);
                            break;
                        // LD (NN), BC/DE/SP
                        case C_BC: case C_DE: case C_SP:
                            *buf++ = 0xED;
                            *buf++ = (uint8_t)(0b01000011 | (srcOps << 4));
                            *buf++ = (uint8_t)(number & 0xFF);
                            *buf++ = (uint8_t)(number >> 8);
                            break;
                        default: return ERROR_INVALID_COMMAND;
                    }
                    break;
                // LD (BC/DE), A
                case C_PBC: case C_PDE:
                    if(src != C_A) return ERROR_INVALID_COMMAND;
                    *buf++ = (uint8_t)(0b00000010 | (dstOps << 4));
                    break;
                // LD HL/IX/IY/BC/DE/SP, NN/(NN)
                case C_IX: case C_IY: case C_HL: case C_DE: case C_BC: case C_SP:
                    switch(src) {
                        case C_NN:
                            *buf++ = (uint8_t)(1 + (dstOps << 4));
                            break;
                        case C_PNN:
                            if(!dstPref && dst != C_HL) *buf++ = 0xED;
                            *buf++ = (uint8_t)(dstOps == 2 ? 42 : (0b01001011 | (dstOps << 4)));
                            break;
                        // LD SP,HL/IX/IY
                        case C_IX: case C_IY: case C_HL:
                            if(dst == C_SP) { *buf++ = 0xF9; return ERROR_OK; }
                        default: return ERROR_INVALID_COMMAND;
                    }
                    *buf++ = (uint8_t)(number & 0xFF);
                    *buf++ = (uint8_t)(number >> 8);
                    break;
                // LD (PHL/PIX/PIY), N/SRC8
                case C_PIX: case C_PIY: case C_PHL:
                    switch(src) {
                        case C_NN:
                            if(number > 255) return ERROR_PARAMETER_OUT_OF_RANGE;
                            *buf++ = 54; if(dstPref) *buf++ = disp; *buf++ = (uint8_t)number;
                            break;
                        case C_B: case C_C: case C_D: case C_E: case C_H: case C_L: case C_A:
                            *buf++ = (uint8_t)(0b01110000 | srcOps); if(dstPref) *buf++ = disp;
                            break;
                        default: return ERROR_INVALID_COMMAND;
                    }
                    break;
                // LD REG8, N/(NN)/(BC/DE)/(HL/IX/IY)
                case C_XH: case C_XL: case C_YH: case C_YL:
                case C_B: case C_C: case C_D: case C_E: case C_H: case C_L: case C_A:
                    switch(src) {
                        // LD REG8, REG8
                        case C_XH: case C_XL: case C_YH: case C_YL:
                            if(dst == C_L || dst == C_H) return ERROR_INVALID_COMMAND;
                        case C_H: case C_L:
                            if(dstPref && (src == C_L || src == C_H)) return ERROR_INVALID_COMMAND;
                        case C_B: case C_C: case C_D: case C_E: case C_A:
                            *buf++ = (uint8_t)(0b01000000 | (dstOps << 3) | srcOps);
                            break;
                        // LD REG8, N
                        case C_NN:
                            if(number > 255) return ERROR_PARAMETER_OUT_OF_RANGE;
                            *buf++ = (uint8_t)(0b00000110 | (dstOps << 3));
                            *buf++ = (uint8_t)number;
                            break;
                        // LD A, (BC/DE)
                        case C_PBC: case C_PDE:
                            if(dst != C_A) return ERROR_INVALID_COMMAND;
                            *buf++ = (uint8_t)(0b00001010 | (srcOps << 4));
                            break;
                        // LD A, (NN)
                        case C_PNN:
                            if(dst != C_A) return ERROR_INVALID_COMMAND;
                            *buf++ = 58;
                            *buf++ = (uint8_t)(number & 0xFF);
                            *buf++ = (uint8_t)(number >> 8);
                            break;
                        // LD A,I/LD A,R
                        case C_I: case C_R:
                            *buf++ = 0xED; *buf++ = (uint8_t)(src == C_I ? 0b01010111 : 0b01011111);
                            break;
                        // LD REG8, (IX+-d/IY+-d)
                        case C_PIX: case C_PIY: case C_PHL:
                            *buf++ = (uint8_t)(0b01000110 | (dstOps << 3));
                            if(srcPref) *buf++ = disp;
                            break;
                        default: return ERROR_INVALID_COMMAND;
                    }
                    break;
                default:
                    cmd_begin = cmd_op1;
                    return ERROR_INVALID_COMMAND;
            }
            break;
        default: *buf++ = cod; break;
    }
    return ERROR_OK;
}

bool zxAssembler::checkFlags(int flags, bool is_jr) {
    auto ef = is_jr ? C_FC : C_FM;
    return ((flags >= C_FNZ && flags <= ef) || flags == C_C);
}

bool zxAssembler::checkReg16(int reg, bool is_sp) {
    auto sp_af = is_sp ? C_SP : C_AF;
    return (reg == C_IX || reg == C_IY || reg == C_HL || reg == C_DE || reg == C_BC || reg == sp_af);
}

bool zxAssembler::checkReg8(int reg) {
    return (reg == C_B || reg == C_C || reg == C_D || reg == C_E || reg == C_H || reg == C_L ||
            reg == C_A || reg == C_XH || reg == C_XL || reg == C_YH || reg == C_YL);
}
