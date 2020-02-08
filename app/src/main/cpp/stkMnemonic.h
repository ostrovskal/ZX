//
// Created by Сергей on 01.12.2019.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma once

enum ZX_CODE_BUTTON {
    CB_RL = 0, CB_0, CB_1, CB_2, CB_3, CB_4, CB_5, CB_6, CB_7, CB_8, CB_9,
    CB_A, CB_B, CB_C, CB_D, CB_E, CB_F, CB_G, CB_H, CB_I, CB_J, CB_K, CB_L, CB_M, CB_N, CB_O, CB_P, CB_Q, CB_R, CB_S, CB_T, CB_U, CB_V, CB_W, CB_X, CB_Y, CB_Z,
    CB_a, CB_b, CB_c, CB_d, CB_e, CB_f, CB_g, CB_h, CB_i, CB_j, CB_k, CB_l, CB_m, CB_n, CB_o, CB_p, CB_q, CB_r, CB_s, CB_t, CB_u, CB_v, CB_w, CB_x, CB_y, CB_z,
    // 63
    CB_PLOT, CB_DRAW, CB_REM, CB_RUN, CB_RAND, CB_RET, CB_IF, CB_POKE, CB_INPUT, CB_PRINT, CB_SIN, CB_COS, CB_TAN, CB_INT, CB_RND, CB_STR_, CB_CHR_,
    CB_CODE, CB_PEEK, CB_TAB, CB_ASN, CB_ACS, CB_ATN, CB_VERIFY, CB_MERGE, CB_OUT, CB_AND, CB_OR, CB_AT, CB_INKEYS_, CB_FN,
    CB_NEW, CB_SAVE, CB_DIM, CB_FOR, CB_GOTO, CB_GOSUB, CB_LOAD,
    // 100
    CB_LIST, CB_LET, CB_COPY, CB_CLEAR,
    CB_READ, CB_RESTORE, CB_DATA, CB_SGN, CB_ABS, CB_VAL, CB_LEN, CB_USR, CB_STOP, CB_NOT, CB_STEP, CB_TO, CB_THEN, CB_CIRCLE, CB_VAL_, CB_SCRN_,
    CB_ATTR, CB_LN, CB_BEEP, CB_INK, CB_PAPER, CB_FLUSH, CB_BRIGHT, CB_OVER, CB_INVERSE, CB_CONT, CB_CLS, CB_BORDER, CB_NEXT, CB_PAUSE, CB_POINT,
    CB_MUL, CB_EQ, CB_NE, CB_LS, CB_GT, CB_GTE, CB_LSE, CB_COMMA, CB_DIV, CB_QUEST, CB_PT, CB_COLON, CB_SEMICOLON,
    CB_AMPENDACE, CB_PERCENT, CB_PLUS, CB_MINUS, CB_UNDERLINE, CB_APOSTROF, CB_QUOTE, CB_EXP, CB_LPRINT, CB_LLIST,
    CB_BIN, CB_SPACE, CB_EXCLAMATION, CB_LATTICE, CB_DOLLAR, CB_LBRAKKET, CB_RBRAKKET,
    // 165
    CB_MAIL, CB_SLBRAKKET, CB_SRBRAKKET, CB_FLBRAKKET, CB_FRBRAKKET, CB_DEF_FN, CB_OPEN, CB_CLOSE, CB_FORMAT,
    CB_LINE, CB_ERASE, CB_MOVE, CB_CAT, CB_CC, CB_GG, CB_TR, CB_INV, CB_SQR, CB_PI, CB_LL, CB_EE, CB_TILDA, CB_ILI, CB_IN,
    CB_TILE, CB_BREAK
    // 191
};

static const char* namesCode[] = { "C", "B", "E", "D", "L", "H", "R", "A",
                                   "", "(HL)", "I", "BC", "DE", "HL", "AF", "SP",
                                   "NZ", "Z", "NC", "C", "PO", "PE", "P", "M",
                                   "XL", "XH", "YL", "YH", "F", "IX", "IY",
                                   "(BC)", "(DE)", "(IX", "(IY", "(SP)",
                                   "LD ", "JP ", "CALL ","DJNZ ", "JR ",
                                   "BIT ", "RES ", "SET ",
                                   "INC ", "DEC ",
                                   "EX ", "IM ",
                                   "ADD ", "ADC ", "SUB ", "SBC ", "AND ", "XOR ", "OR ", "CP ",
                                   "RLC ", "RRC ", "RL ", "RR ", "SLA ", "SRA ", "SLL ", "SRL ",
                                   "IN ", "OUT ",
                                   "RST ", "PUSH ", "POP ",
                                   "RET ",
                                    // без операнда
                                   "NOP", "EX AF, AF'",
                                   "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF",
                                   "DI", "EI",
                                   "RRD", "RLD",
                                   "LDI", "CPI", "INI", "OTI",
                                   "LDD", "CPD", "IND",  "OTD",
                                   "LDIR", "CPIR", "INIR", "OTIR",
                                   "LDDR", "CPDR", "INDR",  "OTDR",
                                   "EXX", "EX DE, HL", "EX (SP), ",
                                   "RETI", "RETN",
                                   "HALT", "NEG", "*IX*", "*IY*", "*ED*",
                                   ", ", "0", "(C)", nullptr};

enum DA_MNEMONIC_NAMES {
    C_C, C_B, C_E, C_D, C_L, C_H, C_R, C_A,
    // 8
    C_NULL, C_PHL,
    C_I, C_BC, C_DE, C_HL, C_AF, C_SP,
    // 16
    C_FNZ, C_FZ, C_FNC, C_FC, C_FPO, C_FPE, C_FP, C_FM,
    // 24
    C_XL, C_XH, C_YL, C_YH, C_F, C_IX, C_IY,
    C_PBC, C_PDE, C_PIX, C_PIY, C_PSP,
    // 36
    C_LD, C_JP, C_CALL, C_DJNZ, C_JR,
    C_BIT, C_RES, C_SET,
    C_INC, C_DEC,
    C_EX, C_IM,
    // 48
    C_ADD, C_ADC, C_SUB, C_SBC, C_AND, C_XOR, C_OR, C_CP,
    C_RLC, C_RRC, C_RL, C_RR, C_SLA, C_SRA, C_SLI, C_SRL,
    C_IN, C_OUT,
    C_RST, C_PUSH, C_POP, C_RET,
    C_NOP, C_EX_AF,
    C_RLCA, C_RRCA, C_RLA, C_RRA, C_DAA, C_CPL, C_SCF, C_CCF,
    C_DI, C_EI,
    C_RRD, C_RLD,
    C_LDI, C_CPI, C_INI, C_OTI,
    C_LDD, C_CPD, C_IND, C_OTD,
    C_LDIR, C_CPIR, C_INIR, C_OTIR,
    C_LDDR, C_CPDR, C_INDR, C_OTDR,
    C_EXX, C_EX_DE, C_EX_SP,
    C_RETI, C_RETN,
    C_HALT, C_NEG, C_IX_NONI, C_IY_NONI, C_ED_NONI,
    C_COMMA, C_END, C_PC, C_SRC, C_DST, C_CB_PHL,
    C_N, C_NN, C_PNN, C_NUM
};

static uint8_t buttons[] = {
        CB_1, 0, CB_2, 0, CB_3, 0, CB_4, 0, CB_5, 0, CB_6, 0, CB_7, 0, CB_8, 0, CB_9, 0, CB_0, 0,
        CB_TILE, 5, CB_PLOT, 0, CB_DRAW, 0, CB_REM, 0, CB_RUN, 0, CB_RAND, 0, CB_RET, 0, CB_IF, 0, CB_INPUT, 0, CB_POKE, 0, CB_PRINT, 0, CB_TILE, 6, CB_TILE, 10, CB_NEW, 0,
        CB_SAVE, 0, CB_DIM, 0, CB_FOR, 0, CB_GOTO, 0, CB_GOSUB, 0, CB_LOAD, 0, CB_LIST, 0, CB_LET, 0, CB_TILE, 12, CB_EE, 0, CB_COPY, 0, CB_CLEAR, 0, CB_CONT, 0, CB_SPACE, 0,
        CB_CLS, 0, CB_BORDER, 0, CB_NEXT, 0, CB_PAUSE, 0, CB_RL, 0,

        CB_1, 0, CB_2, 0, CB_3, 0, CB_4, 0, CB_5, 0, CB_6, 0, CB_7, 0, CB_8, 0, CB_9, 0, CB_0, 0,
        CB_TILE, 5, CB_q, 0, CB_w, 0, CB_e, 0, CB_r, 0, CB_t, 0, CB_y, 0, CB_u, 0, CB_i, 0, CB_o, 0, CB_p, 0, CB_TILE, 6, CB_TILE, 10, CB_a, 0,
        CB_s, 0, CB_d, 0, CB_f, 0, CB_g, 0, CB_h, 0, CB_j, 0, CB_k, 0, CB_l, 0, CB_TILE, 12, CB_EE, 0, CB_z, 0, CB_x, 0, CB_c, 0, CB_SPACE, 0, CB_v, 0, CB_b, 0, CB_n, 0, CB_m, 0,
        CB_RL, 0,

        CB_1, 0, CB_2, 0, CB_3, 0, CB_4, 0, CB_5, 0, CB_6, 0, CB_7, 0, CB_8, 0, CB_9, 0, CB_0, 0,
        CB_TILE, 5, CB_Q, 0, CB_W, 0, CB_E, 0, CB_R, 0, CB_T, 0, CB_Y, 0, CB_U, 0, CB_I, 0, CB_O, 0, CB_P, 0, CB_TILE, 6, CB_TILE, 10, CB_A, 0,
        CB_S, 0, CB_D, 0, CB_F, 0, CB_G, 0, CB_H, 0, CB_J, 0, CB_K, 0, CB_L, 0, CB_TILE, 12, CB_EE, 0, CB_Z, 0, CB_X, 0, CB_C, 0, CB_SPACE, 0, CB_V, 0, CB_B, 0, CB_N, 0, CB_M, 0,
        CB_RL, 0,

        CB_TILE, 90, CB_TILE, 91, CB_TILE, 92, CB_TILE, 93, CB_TILE, 94, CB_TILE, 95, CB_TILE, 96, CB_SPACE, 0, CB_TILE, 49, CB_TILE, 97,
        CB_TILE, 5, CB_SIN, 0, CB_COS, 0, CB_TAN, 0, CB_INT, 0, CB_RND, 0, CB_STR_, 0, CB_CHR_, 0, CB_CODE, 0, CB_PEEK, 0, CB_TAB, 0, CB_TILE, 6, CB_TILE, 10, CB_READ, 0,
        CB_RESTORE, 0, CB_DATA, 0, CB_SGN, 0, CB_ABS, 0, CB_SQR, 0, CB_VAL, 0, CB_LEN, 0, CB_USR, 0, CB_TILE, 12, CB_LL, 0, CB_LN, 0, CB_EXP, 0, CB_LPRINT, 0, CB_SPACE, 0,
        CB_LLIST, 0, CB_BIN, 0, CB_INKEYS_, 0, CB_PI, 0, CB_RL, 0,

        CB_DEF_FN, 0, CB_FN, 0, CB_LINE, 0, CB_OPEN, 0, CB_CLOSE, 0, CB_MOVE, 0, CB_ERASE, 0, CB_POINT, 0, CB_CAT, 0, CB_FORMAT, 0,
        CB_TILE, 5, CB_ASN, 0, CB_ACS, 0, CB_ATN, 0, CB_VERIFY, 0, CB_MERGE, 0, CB_SLBRAKKET, 0, CB_SRBRAKKET, 0, CB_IN, 0, CB_OUT, 0, CB_MAIL, 0, CB_TILE, 6, CB_TILE, 10, CB_TILDA, 0,
        CB_ILI, 0, CB_DIV, 0, CB_FLBRAKKET, 0, CB_FRBRAKKET, 0, CB_CIRCLE, 0, CB_VAL_, 0, CB_SCRN_, 0, CB_ATTR, 0, CB_TILE, 13, CB_LL, 0, CB_BEEP, 0, CB_INK, 0, CB_PAPER, 0,
        CB_SPACE, 0, CB_FLUSH, 0, CB_BRIGHT, 0, CB_OVER, 0, CB_INVERSE, 0, CB_RL, 0,

        CB_EXCLAMATION, 0, CB_MAIL, 0, CB_LATTICE, 0, CB_DOLLAR, 0, CB_PERCENT, 0, CB_AMPENDACE, 0, CB_APOSTROF, 0, CB_LBRAKKET, 0, CB_RBRAKKET, 0, CB_UNDERLINE, 0,
        CB_TILE, 5, CB_LSE, 0, CB_NE, 0, CB_GTE, 0, CB_LS, 0, CB_GT, 0, CB_AND, 0, CB_OR, 0, CB_AT, 0, CB_SEMICOLON, 0, CB_QUOTE, 0, CB_TILE, 6, CB_TILE, 10, CB_STOP, 0,
        CB_NOT, 0, CB_STEP, 0, CB_TO, 0, CB_THEN, 0, CB_EXCLAMATION, 0, CB_MINUS, 0, CB_PLUS, 0, CB_EQ, 0, CB_TILE, 13, CB_EE, 0, CB_COLON, 0, CB_TILE, 7, CB_QUEST, 0,
        CB_SPACE, 0, CB_DIV, 0, CB_MUL, 0, CB_COMMA, 0, CB_PT, 0, CB_RL, 0,

        CB_TILE, 31, CB_CC, 0, CB_TR, 0, CB_INV, 0, CB_TILE, 24, CB_TILE, 27, CB_TILE, 26, CB_TILE, 25, CB_GG, 0, CB_TILE, 5,
        CB_TILE, 5, CB_Q, 0, CB_W, 0, CB_E, 0, CB_R, 0, CB_T, 0, CB_Y, 0, CB_U, 0, CB_I, 0, CB_O, 0, CB_P, 0, CB_TILE, 6, CB_TILE, 11, CB_A, 0,
        CB_S, 0, CB_D, 0, CB_F, 0, CB_G, 0, CB_H, 0, CB_J, 0, CB_K, 0, CB_L, 0, CB_TILE, 12, CB_EE, 0, CB_Z, 0, CB_X, 0, CB_C, 0, CB_BREAK, 0, CB_V, 0, CB_B, 0, CB_N, 0, CB_M, 0,
        CB_RL, 0,

        CB_TILE, 31, CB_CC, 0, CB_TR, 0, CB_INV, 0, CB_TILE, 24, CB_TILE, 27, CB_TILE, 26, CB_TILE, 25, CB_GG, 0, CB_TILE, 5,
        CB_TILE, 5, CB_PLOT, 0, CB_DRAW, 0, CB_REM, 0, CB_RUN, 0, CB_RAND, 0, CB_RET, 0, CB_IF, 0, CB_INPUT, 0, CB_POKE, 0, CB_PRINT, 0, CB_TILE, 6, CB_TILE, 11, CB_NEW, 0,
        CB_SAVE, 0, CB_DIM, 0, CB_FOR, 0, CB_GOTO, 0, CB_GOSUB, 0, CB_LOAD, 0, CB_LIST, 0, CB_LET, 0, CB_TILE, 12, CB_EE, 0, CB_COPY, 0, CB_CLEAR, 0, CB_CONT, 0,
        CB_BREAK, 0, CB_CLS, 0, CB_BORDER, 0, CB_NEXT, 0, CB_PAUSE, 0, CB_RL, 0,

        CB_TILE, 60, CB_TILE, 61, CB_TILE, 62, CB_TILE, 63, CB_TILE, 64, CB_TILE, 65, CB_TILE, 67, CB_TILE, 66, CB_GG, 0, CB_TILE, 5,
        CB_TILE, 5, CB_Q, 0, CB_INKEYS_, 0, CB_E, 0, CB_R, 0, CB_T, 0, CB_FN, 0, CB_U, 0, CB_I, 0, CB_O, 0, CB_P, 0, CB_TILE, 6, CB_TILE, 10, CB_A, 0,
        CB_S, 0, CB_D, 0, CB_F, 0, CB_G, 0, CB_H, 0, CB_J, 0, CB_K, 0, CB_L, 0, CB_TILE, 12, CB_EE, 0, CB_POINT, 0, CB_PI, 0, CB_C, 0, CB_SPACE, 0,
        CB_RND, 0, CB_B, 0, CB_N, 0, CB_M, 0, CB_RL, 0,

        CB_TILE, 70, CB_TILE, 71, CB_TILE, 72, CB_TILE, 73, CB_TILE, 74, CB_TILE, 75, CB_TILE, 76, CB_SPACE, 0, CB_GG, 0, CB_TILE, 5,
        CB_TILE, 5, CB_Q, 0, CB_INKEYS_, 0, CB_E, 0, CB_R, 0, CB_T, 0, CB_FN, 0, CB_U, 0, CB_I, 0, CB_O, 0, CB_P, 0, CB_TILE, 6, CB_TILE, 11, CB_A, 0,
        CB_S, 0, CB_D, 0, CB_F, 0, CB_G, 0, CB_H, 0, CB_J, 0, CB_K, 0, CB_L, 0, CB_TILE, 12, CB_EE, 0, CB_POINT, 0, CB_PI, 0, CB_C, 0, CB_SPACE, 0,
        CB_RND, 0, CB_B, 0, CB_N, 0, CB_M, 0, CB_RL, 0,

        CB_TILE, 98, CB_TILE, 88, CB_TILE, 78, CB_TILE, 68, CB_TILE, 58, CB_TILE, 48, CB_TILE, 38, CB_SPACE, 0, CB_TILE, 39, CB_TILE, 28,
        CB_TILE, 5, CB_ASN, 0, CB_ACS, 0, CB_ATN, 0, CB_VERIFY, 0, CB_MERGE, 0, CB_SLBRAKKET, 0, CB_SRBRAKKET, 0, CB_IN, 0, CB_OUT, 0, CB_MAIL, 0, CB_TILE, 6, CB_TILE, 11, CB_TILDA, 0,
        CB_ILI, 0, CB_DIV, 0, CB_FLBRAKKET, 0, CB_FRBRAKKET, 0, CB_CIRCLE, 0, CB_VAL_, 0, CB_SCRN_, 0, CB_ATTR, 0, CB_TILE, 13, CB_LL, 0, CB_BEEP, 0, CB_INK, 0, CB_PAPER, 0,
        CB_SPACE, 0, CB_FLUSH, 0, CB_BRIGHT, 0, CB_OVER, 0, CB_INVERSE, 0, CB_RL, 0
};


enum MNEMONIC_FLAGS {
    _NZ = 1, _Z, _NC, _C, _PO, _PE, _P, _M
};

// такты комманды(5 бит) и ее длина(3 бита)
#define _T(length, ticks)   (uint8_t)(ticks | (uint8_t)(length << 5))

#define ADD_A_X(x)      { _N_, x, O_ADD, _T(1, 4), C_ADD, F_ADSB8 }
#define ADC_A_X(x)      { _N_, x, O_ADC, _T(1, 4), C_ADC, F_ADSB8 }
#define SUB_A_X(x)      { _N_, x, O_SUB, _T(1, 4), C_SUB, F_ADSB8 }
#define SBC_A_X(x)      { _N_, x, O_SBC, _T(1, 4), C_SBC, F_ADSB8 }
#define XOR_A_X(x)      { _N_, x, O_XOR, _T(1, 4), C_XOR, F_XOR }
#define AND_A_X(x)      { _N_, x, O_AND, _T(1, 4), C_AND, F_AND }
#define OR_A_X(x)       { _N_, x, O_OR, _T(1, 4), C_OR, F_OR }
#define CP_A_X(x)       { _N_, x, O_CP, _T(1, 4), C_CP, F_CP }
#define LD_X_Y(x, y)    { x,  y, O_ASSIGN, _T(1, 4), C_LD }
#define LD_HL_Y(y)      { _RPHL, y, O_SAVE, _T(1, 7), C_LD }
#define LD_X_HL(x)      { x, _RPHL, O_LOAD, _T(1, 7), C_LD }

#define ROT_PX(c)       { _RPHL, _N_, O_ROTX, _T(1, 11), c, F_ROTX }

#define ROT_X(x, c)     { x, _N_, O_ROTX, _T(1, 4), c, F_ROTX }
#define BIT_X(x)        { x, _N_, O_BIT, _T(1, 4), C_BIT, F_BIT }
#define RES_X(x)        { x, _N_, O_RES, _T(1, 4), C_RES }
#define SET_X(x)        { x, _N_, O_SET, _T(1, 4), C_SET }


#define F_ADSBC16   FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_ADSB8     FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_XOR       FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_OR        FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_AND       FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_CP        FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_NEG       FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_ROTX      FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_INI       FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_OTI       FS | FZ | F5 | FH | F3 | FPV | FN | FC
#define F_IR        FS | FZ | F5 | FH | F3 | FPV | FN
#define F_ID        FS | FZ | F5 | FH | F3 | FPV | FN
#define F_IN        FS | FZ | F5 | FH | F3 | FPV | FN
#define F_BIT       FS | FZ | F5 | FH | F3 | FPV | FN
#define F_RLRD      FS | FZ | F5 | FH | F3 | FPV | FN
#define F_CPI       FS | FZ | F5 | FH | F3 | FPV | FN
#define F_DAA       FS | FZ | F5 | FH | F3 | FPV | FC
#define F_ADD16     F5 | FH | F3 | FN | FC
#define F_CCF       F5 | FH | F3 | FN | FC
#define F_SCF       F5 | FH | F3 | FN | FC
#define F_ROT       F5 | FH | F3 | FN | FC
#define F_LDI       F5 | FH | F3 | FPV | FN
#define F_CPL       F5 | FH | F3 | FN

#define STK_NONI    { _N_, _N_, O_UNDEF, _T(1, 4), C_ED_NONI }

#define _RBC        (_R16 | _RC)
#define _RDE        (_R16 | _RE)
#define _RHL        (_R16 | _RL)
#define _RAF        (_R16 | _RF)
#define _RSP        (_R16 | _RS)

static MNEMONIC mnemonics[] = {
        { _N_,  _N_,    O_SPEC,     _T(1, 4),  C_NOP           },  // NOP
        { _RBC, _C16,   O_ASSIGN,   _T(3, 10), C_LD            },  // LD_BC_NN
        { _RBC, _RA,    O_SAVE,     _T(1, 7),  C_LD            },  // LD_PBC_A
        { _RBC, _N_,    O_INC,      _T(1, 6),  C_INC           },  // INC_BC
        { _RB,  _N_,    O_INC,      _T(1, 4),  C_INC,  F_ID    },  // INC_B
        { _RB,  _N_,    O_DEC,      _T(1, 4),  C_DEC,  F_ID    },  // DEC_B
        { _RB,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_B_N
        { _RA,  _N_,    O_ROT,      _T(1, 4),  C_RLCA, F_ROT   },  // RLCA
        { _N_,  _N_,    O_EX,       _T(1, 4),  C_EX_AF         },  // EX_AF_AF'
        { _RHL, _RBC,   O_ADD,      _T(1, 11), C_ADD, F_ADD16  },  // ADD_HL_BC
        { _RA,  _RBC,   O_LOAD,     _T(1, 7),  C_LD            },  // LD_A_PBC
        { _RBC, _N_,    O_DEC,      _T(1, 6),  C_DEC           },  // DEC_BC
        { _RC,  _N_,    O_INC,      _T(1, 4),  C_INC, F_ID     },  // INC_C
        { _RC,  _N_,    O_DEC,      _T(1, 4),  C_DEC, F_ID     },  // DEC_C
        { _RC,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_C_N
        { _RA,  _N_,    O_ROT,      _T(1, 4),  C_RRCA, F_ROT   },  // RRCA
        { _N_,  _C8,    O_SPEC,     _T(2, 8),  C_DJNZ          },  // DJNZ
        { _RDE, _C16,   O_ASSIGN,   _T(3, 10), C_LD            },  // LD_DE_NN
        { _RDE, _RA,    O_SAVE,     _T(1, 7),  C_LD            },  // LD_PDE_A
        { _RDE, _N_,    O_INC,      _T(1, 6),  C_INC           },  // INC_DE
        { _RD,  _N_,    O_INC,      _T(1, 4),  C_INC, F_ID     },  // INC_D
        { _RD,  _N_,    O_DEC,      _T(1, 4),  C_DEC, F_ID     },  // DEC_D
        { _RD,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_D_N
        { _RA,  _N_,    O_ROT,      _T(1, 4),  C_RLA, F_ROT    },  // RLA
        { _N_,  _C8,    O_JR,       _T(2, 7),  C_JR            },  // JR_N
        { _RHL, _RDE,   O_ADD,      _T(1, 11), C_ADD, F_ADD16  },  // ADD_HL_DE
        { _RA,  _RDE,   O_LOAD,     _T(1, 7),  C_LD            },  // LD_A_PDE
        { _RDE, _N_,    O_DEC,      _T(1, 6),  C_DEC           },  // DEC_DE
        { _RE,  _N_,    O_INC,      _T(1, 4),  C_INC, F_ID     },  // INC_E
        { _RE,  _N_,    O_DEC,      _T(1, 4),  C_DEC, F_ID     },  // DEC_E
        { _RE,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_E_N
        { _RA,  _N_,    O_ROT,      _T(1, 4),  C_RRA, F_ROT    },  // RRA
        { _N_,  _C8,    O_JR,       _T(2, 7),  C_JR, _NZ       },  // JR_NZ
        { _RHL, _C16,   O_ASSIGN,   _T(3, 10), C_LD            },  // LD_HL_NN
        { _C16, _RHL,   O_SAVE,     _T(3, 16), C_LD            },  // LD_PNN_HL
        { _RHL, _N_,    O_INC,      _T(1, 6),  C_INC           },  // INC_HL
        { _RH,  _N_,    O_INC,      _T(1, 4),  C_INC, F_ID     },  // INC_H
        { _RH,  _N_,    O_DEC,      _T(1, 4),  C_DEC, F_ID     },  // DEC_H
        { _RH,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_H_N
        { _N_,  _N_,    O_SPEC,     _T(1, 4),  C_DAA, F_DAA    },  // DAA
        { _N_,  _C8,    O_JR,       _T(2, 7),  C_JR, _Z        },  // JR_Z
        { _RHL, _RHL,   O_ADD,      _T(1, 11), C_ADD, F_ADD16  },  // ADD_HL_HL
        { _RHL, _C16,   O_LOAD,     _T(3, 16), C_LD            },  // LD_HL_PNN
        { _RHL, _N_,    O_DEC,      _T(1, 6),  C_DEC           },  // DEC_HL
        { _RL,  _N_,    O_INC,      _T(1, 4),  C_INC, F_ID     },  // INC_L
        { _RL,  _N_,    O_DEC,      _T(1, 4),  C_DEC, F_ID     },  // DEC_L
        { _RL,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_L_N
        { _N_,  _N_,    O_SPEC,     _T(1, 4),  C_CPL, F_CPL    },  // CPL
        { _N_,  _C8,    O_JR,       _T(2, 12), C_JR, _NC       },  // JR_NC
        { _RSP, _C16,   O_ASSIGN,   _T(3, 10), C_LD            },  // LD_SP_NN
        { _C16, _RA,    O_SAVE,     _T(3, 13), C_LD            },  // LD_PNN_A
        { _RSP, _N_,    O_INC,      _T(1, 6),  C_INC           },  // INC_SP
        { _RPHL,_N_,    O_INC,      _T(1, 11), C_INC, F_ID     },  // INC_PHL
        { _RPHL,_N_,    O_DEC,      _T(1, 11), C_DEC, F_ID     },  // DEC_PHL
        { _RPHL,_C8,    O_SAVE,     _T(2, 10), C_LD            },  // LD_PHL_N
        { _N_,  _N_,    O_SPEC,     _T(1, 4),  C_SCF, F_SCF    },  // SCF
        { _N_,  _C8,    O_JR,       _T(2, 7),  C_JR, _C        },  // JR_C
        { _RHL, _RSP,   O_ADD,      _T(1, 11), C_ADD, F_ADD16  },  // ADD_HL_SP
        { _RA,  _C16,   O_LOAD,     _T(3, 13), C_LD            },  // LD_A_PNN
        { _RSP, _N_,    O_DEC,      _T(1, 6),  C_DEC           },  // DEC_SP
        { _RA,  _N_,    O_INC,      _T(1, 4),  C_INC, F_ID     },  // INC_A
        { _RA,  _N_,    O_DEC,      _T(1, 4),  C_DEC, F_ID     },  // DEC_A
        { _RA,  _C8,    O_ASSIGN,   _T(2, 7),  C_LD            },  // LD_A_N
        { _N_,  _N_,    O_SPEC,     _T(1, 4),  C_CCF, F_CCF    },  // CCF

        LD_X_Y(_RB, _RB), LD_X_Y(_RB, _RC), LD_X_Y(_RB, _RD), LD_X_Y(_RB, _RE), LD_X_Y(_RB, _RH), LD_X_Y(_RB, _RL), LD_X_HL(_RB), LD_X_Y(_RB, _RA),
        LD_X_Y(_RC, _RB), LD_X_Y(_RC, _RC), LD_X_Y(_RC, _RD), LD_X_Y(_RC, _RE), LD_X_Y(_RC, _RH), LD_X_Y(_RC, _RL), LD_X_HL(_RC), LD_X_Y(_RC, _RA),
        LD_X_Y(_RD, _RB), LD_X_Y(_RD, _RC), LD_X_Y(_RD, _RD), LD_X_Y(_RD, _RE), LD_X_Y(_RD, _RH), LD_X_Y(_RD, _RL), LD_X_HL(_RD), LD_X_Y(_RD, _RA),
        LD_X_Y(_RE, _RB), LD_X_Y(_RE, _RC), LD_X_Y(_RE, _RD), LD_X_Y(_RE, _RE), LD_X_Y(_RE, _RH), LD_X_Y(_RE, _RL), LD_X_HL(_RE), LD_X_Y(_RE, _RA),
        LD_X_Y(_RH, _RB), LD_X_Y(_RH, _RC), LD_X_Y(_RH, _RD), LD_X_Y(_RH, _RE), LD_X_Y(_RH, _RH), LD_X_Y(_RH, _RL), LD_X_HL(_RH), LD_X_Y(_RH, _RA),
        LD_X_Y(_RL, _RB), LD_X_Y(_RL, _RC), LD_X_Y(_RL, _RD), LD_X_Y(_RL, _RE), LD_X_Y(_RL, _RH), LD_X_Y(_RL, _RL), LD_X_HL(_RL), LD_X_Y(_RL, _RA),
        LD_HL_Y(_RB), LD_HL_Y(_RC), LD_HL_Y(_RD), LD_HL_Y(_RE), LD_HL_Y(_RH), LD_HL_Y(_RL), { _N_,  _N_, O_SPEC, _T(1, 4), C_HALT }, LD_HL_Y(_RA),
        LD_X_Y(_RA, _RB), LD_X_Y(_RA, _RC), LD_X_Y(_RA, _RD), LD_X_Y(_RA, _RE), LD_X_Y(_RA, _RH), LD_X_Y(_RA, _RL), LD_X_HL(_RA), LD_X_Y(_RA, _RA),

        ADD_A_X(_RB), ADD_A_X(_RC), ADD_A_X(_RD), ADD_A_X(_RE), ADD_A_X(_RH), ADD_A_X(_RL), { _N_, _RPHL, O_ADD, _T(1, 7), C_ADD, F_ADSB8 }, ADD_A_X(_RA),
        ADC_A_X(_RB), ADC_A_X(_RC), ADC_A_X(_RD), ADC_A_X(_RE), ADC_A_X(_RH), ADC_A_X(_RL), { _N_, _RPHL, O_ADC, _T(1, 7), C_ADC, F_ADSB8 }, ADC_A_X(_RA),
        SUB_A_X(_RB), SUB_A_X(_RC), SUB_A_X(_RD), SUB_A_X(_RE), SUB_A_X(_RH), SUB_A_X(_RL), { _N_, _RPHL, O_SUB, _T(1, 7), C_SUB, F_ADSB8 }, SUB_A_X(_RA),
        SBC_A_X(_RB), SBC_A_X(_RC), SBC_A_X(_RD), SBC_A_X(_RE), SBC_A_X(_RH), SBC_A_X(_RL), { _N_, _RPHL, O_SBC, _T(1, 7), C_SBC, F_ADSB8 }, SBC_A_X(_RA),
        AND_A_X(_RB), AND_A_X(_RC), AND_A_X(_RD), AND_A_X(_RE), AND_A_X(_RH), AND_A_X(_RL), { _N_, _RPHL, O_AND, _T(1, 7), C_AND, F_AND }, AND_A_X(_RA),
        XOR_A_X(_RB), XOR_A_X(_RC), XOR_A_X(_RD), XOR_A_X(_RE), XOR_A_X(_RH), XOR_A_X(_RL), { _N_, _RPHL, O_XOR, _T(1, 7), C_XOR, F_XOR }, XOR_A_X(_RA),
        OR_A_X(_RB), OR_A_X(_RC), OR_A_X(_RD), OR_A_X(_RE), OR_A_X(_RH), OR_A_X(_RL), { _N_, _RPHL, O_OR, _T(1, 7), C_OR, F_OR }, OR_A_X(_RA),
        CP_A_X(_RB), CP_A_X(_RC), CP_A_X(_RD), CP_A_X(_RE), CP_A_X(_RH), CP_A_X(_RL), { _N_, _RPHL, O_CP, _T(1, 7), C_CP, F_CP }, CP_A_X(_RA),

        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _NZ     },  // RET_NZ
        { _RBC, _N_,    O_POP,  _T(1, 10), C_POP           },  // POP_BC
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _NZ     },  // JP_NZ
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP            },  // JP_NN
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _NZ     },  // CALL_NZ
        { _RBC, _N_,    O_PUSH, _T(1, 11), C_PUSH          },  // PUSH_BC
        { _N_,  _C8,    O_ADD,  _T(2, 7),  C_ADD,  F_ADSB8 },  // ADD_A_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST0
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _Z      },  // RET_Z
        { _N_,  _N_,    O_RET,  _T(1, 10), C_RET           },  // RET
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _Z      },  // JP_Z
        { _N_,  _N_,    O_PREF, _T(1, 0),  C_NULL, 1       },  // PREF_CB
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _Z      },  // CALL_Z
        { _N_,  _C16,   O_CALL, _T(3, 17), C_CALL          },  // CALL_NN
        { _N_,  _C8,    O_ADC,  _T(2, 7),  C_ADC,  F_ADSB8 },  // ADC_A_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST8
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _NC     },  // RET_NC
        { _RDE, _N_,    O_POP,  _T(1, 10), C_POP           },  // POP_DE
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _NC     },  // JP_NC
        { _RA,  _C8,    O_OUT,  _T(2, 11), C_OUT           },  // OUT_N_A
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _NC     },  // CALL_NC
        { _RDE, _N_,    O_PUSH, _T(1, 11), C_PUSH          },  // PUSH_DE
        { _N_,  _C8,    O_SUB,  _T(2, 7),  C_SUB,  F_ADSB8 },  // SUB_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST16
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _C      },  // RET_C
        { _N_,  _N_,    O_EX,   _T(1, 4),  C_EXX           },  // EXX
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _C      },  // JP_C
        { _RA,  _C8,    O_IN,   _T(2, 11), C_IN            },  // IN_A_N
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _C      },  // CALL_C
        { _N_,  _N_,    O_PREF, _T(1, 12), C_NULL          },  // PREF_DD
        { _N_,  _C8,    O_SBC,  _T(2, 7),  C_SBC,  F_ADSB8 },  // SBC_A_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST24
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _PO     },  // RET_PO
        { _RHL, _N_,    O_POP,  _T(1, 10), C_POP           },  // POP_HL
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _PO     },  // JP_PO
        { _RHL, _N_,    O_EX,   _T(1, 19), C_EX_SP         },  // EX_PSP_HL
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _PO     },  // CALL_PO
        { _RHL, _N_,    O_PUSH, _T(1, 11), C_PUSH          },  // PUSH_HL
        { _N_,  _C8,    O_AND,  _T(2, 7),  C_AND,  F_AND   },  // AND_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST32
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _PE     },  // RET_PE
        { _RHL, _N_,    O_SPEC, _T(1, 4),  C_JP            },  // JP_HL
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _PE     },  // JP_PE
        { _N_,  _N_,    O_EX,   _T(1, 4),  C_EX_DE         },  // EX_DE_HL
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _PE     },  // CALL_PE
        { _N_,  _N_,    O_PREF, _T(1, 0),  C_NULL, 2       },  // PREF_ED
        { _N_,  _C8,    O_XOR,  _T(2, 7),  C_XOR,  F_XOR   },  // XOR_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST40
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _P      },  // RET_P
        { _RAF, _N_,    O_POP,  _T(1, 10), C_POP           },  // POP_AF
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _P      },  // JP_P
        { _N_,  _N_,    O_SPEC, _T(1, 4),  C_DI            },  // DI
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _P      },  // CALL_P
        { _RAF, _N_,    O_PUSH, _T(1, 11), C_PUSH          },  // PUSH_AF
        { _N_,  _C8,    O_OR,   _T(2, 7),  C_OR,   F_OR    },  // OR_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST48
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET,  _M      },  // RET_M
        { _RSP, _RHL,   O_ASSIGN,_T(1, 4), C_LD            },  // LD_SP_HL
        { _N_,  _C16,   O_JMP,  _T(3, 10), C_JP,   _M      },  // JP_M
        { _N_,  _N_,    O_SPEC, _T(1, 4),  C_EI            },  // EI
        { _N_,  _C16,   O_CALL, _T(3, 10), C_CALL, _M      },  // CALL_M
        { _N_,  _N_,    O_PREF, _T(1, 24), C_NULL          },  // PREF_FD
        { _N_,  _C8,    O_CP,   _T(2, 7),  C_CP,   F_CP    },  // CP_N
        { _N_,  _N_,    O_RST,  _T(1, 11), C_RST           },  // RST56
        // 203
        ROT_X(_RB, C_RLC), ROT_X(_RC, C_RLC), ROT_X(_RD, C_RLC), ROT_X(_RE, C_RLC), ROT_X(_RH, C_RLC), ROT_X(_RL, C_RLC), ROT_PX(C_RLC), ROT_X(_RA, C_RLC),
        ROT_X(_RB, C_RRC), ROT_X(_RC, C_RRC), ROT_X(_RD, C_RRC), ROT_X(_RE, C_RRC), ROT_X(_RH, C_RRC), ROT_X(_RL, C_RRC), ROT_PX(C_RRC), ROT_X(_RA, C_RRC),
        ROT_X(_RB, C_RL),  ROT_X(_RC, C_RL),  ROT_X(_RD, C_RL),  ROT_X(_RE, C_RL),  ROT_X(_RH, C_RL),  ROT_X(_RL, C_RL),  ROT_PX(C_RL),  ROT_X(_RA, C_RL),
        ROT_X(_RB, C_RR),  ROT_X(_RC, C_RR),  ROT_X(_RD, C_RR),  ROT_X(_RE, C_RR),  ROT_X(_RH, C_RR),  ROT_X(_RL, C_RR),  ROT_PX(C_RR),  ROT_X(_RA, C_RR),
        ROT_X(_RB, C_SLA), ROT_X(_RC, C_SLA), ROT_X(_RD, C_SLA), ROT_X(_RE, C_SLA), ROT_X(_RH, C_SLA), ROT_X(_RL, C_SLA), ROT_PX(C_SLA), ROT_X(_RA, C_SLA),
        ROT_X(_RB, C_SRA), ROT_X(_RC, C_SRA), ROT_X(_RD, C_SRA), ROT_X(_RE, C_SRA), ROT_X(_RH, C_SRA), ROT_X(_RL, C_SRA), ROT_PX(C_SRA), ROT_X(_RA, C_SRA),
        ROT_X(_RB, C_SLI), ROT_X(_RC, C_SLI), ROT_X(_RD, C_SLI), ROT_X(_RE, C_SLI), ROT_X(_RH, C_SLI), ROT_X(_RL, C_SLI), ROT_PX(C_SLI), ROT_X(_RA, C_SLI),
        ROT_X(_RB, C_SRL), ROT_X(_RC, C_SRL), ROT_X(_RD, C_SRL), ROT_X(_RE, C_SRL), ROT_X(_RH, C_SRL), ROT_X(_RL, C_SRL), ROT_PX(C_SRL), ROT_X(_RA, C_SRL),

        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _N_, O_BIT, _T(1, 8), C_BIT, F_BIT }, BIT_X(_RA),

        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _N_, O_RES, _T(1, 11), C_RES }, RES_X(_RA),

        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _N_, O_SET, _T(1, 11), C_SET }, SET_X(_RA),

        // 237
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _RB,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_B_BC = 64
        { _RB,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_B
        { _RHL, _RBC,   O_SBC,  _T(1, 11), C_SBC, F_ADSBC16},  // SBC_HL_BC
        { _C16, _RBC,   O_SAVE, _T(3, 16), C_LD            },  // LD_PNN_BC
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG
        { _N_,  _N_,    O_RETN, _T(1, 4),  C_RETN          },  // RETN
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM0
        { _RI,  _RA,    O_ASSIGN,_T(1, 5), C_LD            },  // LD_I_A
        { _RC,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_C_BC
        { _RC,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_C
        { _RHL, _RBC,   O_ADC,  _T(1, 11), C_ADC, F_ADSBC16},  // ADC_HL_BC
        { _RBC, _C16,   O_LOAD, _T(3, 16), C_LD            },  // LD_BC_PNN
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_1
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RETI          },  // RETI
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM0_1
        { _RR,  _RA,    O_ASSIGN,_T(1, 5), C_LD            },  // LD_R_A
        { _RD,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_D_BC
        { _RD,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_D
        { _RHL, _RDE,   O_SBC,  _T(1, 11), C_SBC, F_ADSBC16},  // SBC_HL_DE
        { _C16, _RDE,   O_SAVE, _T(3, 16), C_LD            },  // LD_PNN_DE
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_2
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET           },  // RET_0
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM1
        { _RA,  _RI,    O_ASSIGN,_T(1, 5), C_LD,   F_IR    },  // LD_A_I
        { _RE,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_E_BC
        { _RE,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_E
        { _RHL, _RDE,   O_ADC,  _T(1, 11), C_ADC, F_ADSBC16},  // ADC_HL_DE
        { _RDE, _C16,   O_LOAD, _T(3, 16), C_LD            },  // LD_DE_PNN
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_3
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET           },  // RET_1
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM2
        { _RA,  _RR,    O_ASSIGN,_T(1, 5), C_LD,   F_IR    },  // LD_A_R
        { _RH,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_H_BC
        { _RH,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_H
        { _RHL, _RHL,   O_SBC,  _T(1, 11), C_SBC, F_ADSBC16},  // SBC_HL_HL
        { _C16, _RHL,   O_SAVE, _T(3, 16), C_LD            },  // LD_PNN_HL1
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_4
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET           },  // RET_2=101
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM0_2
        { _RPHL,_RA,    O_SPEC, _T(1, 14), C_RRD,  F_RLRD  },  // RRD=103
        { _RL,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_L_BC
        { _RL,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_L
        { _RHL, _RHL,   O_ADC,  _T(1, 11), C_ADC, F_ADSBC16},  // ADC_HL_HL
        { _RHL, _C16,   O_LOAD, _T(3, 16), C_LD            },  // LD_HL1_PNN
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_5
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET           },  // RET_3
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM0_3
        { _RPHL,_RA,    O_SPEC, _T(1, 14), C_RLD,  F_RLRD  },  // RLD=111
        { _RF,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_F_BC
        { _N_,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_0
        { _RHL, _RSP,   O_SBC,  _T(1, 11), C_SBC, F_ADSBC16},  // SBC_HL_SP
        { _C16, _RSP,   O_SAVE, _T(3, 16), C_LD            },  // LD_PNN_SP
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_6
        { _N_,  _N_,    O_RET,  _T(1, 5),  C_RET           },  // RET_4
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM1_1
        { _N_,  _N_,    O_SPEC, _T(1, 8),  C_NOP           },  // NOP_1=119
        { _RA,  _RC,    O_IN,   _T(1, 8), C_IN,    F_IN    },  // IN_A_BC
        { _RA,  _RC,    O_OUT,  _T(1, 8), C_OUT            },  // OUT_BC_A
        { _RHL, _RSP,   O_ADC,  _T(1, 11), C_ADC,  F_ADSBC16}, // ADC_HL_SP
        { _RSP, _C16,   O_LOAD, _T(3, 16), C_LD            },  // LD_SP_PNN
        { _N_,  _N_,    O_NEG,  _T(1, 4),  C_NEG,  F_NEG   },  // NEG_7
        { _N_,  _N_,    O_RET,  _T(1, 5), C_RET            },  // RET_5
        { _N_,  _N_,    O_IM,   _T(1, 4),  C_IM            },  // IM2_1
        { _N_,  _N_,    O_SPEC, _T(1, 4),  C_NOP           },  // NOP_2 = 127
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, _T(1, 12), C_LDI, F_LDI },  // LDI
        { _N_, _N_, O_CPI, _T(1, 12), C_CPI, F_CPI },  // CPI
        { _N_, _N_, O_INI, _T(1, 12), C_INI, F_INI },  // INI
        { _N_, _N_, O_OTI, _T(1, 12), C_OTI, F_OTI },  // OTI
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, _T(1, 12), C_LDD, F_LDI },  // LDD
        { _N_, _N_, O_CPI, _T(1, 12), C_CPD, F_CPI },  // CPD
        { _N_, _N_, O_INI, _T(1, 12), C_IND, F_INI },  // IND
        { _N_, _N_, O_OTI, _T(1, 12), C_OTD, F_OTI },  // OTD
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, _T(1, 12), C_LDIR, F_LDI},  // LDIR
        { _N_, _N_, O_CPI, _T(1, 12), C_CPIR, F_CPI }, // CPIR
        { _N_, _N_, O_INI, _T(1, 12), C_INIR, F_INI }, // INIR
        { _N_, _N_, O_OTI, _T(1, 12), C_OTIR, F_OTI }, // OTIR
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, _T(1, 12), C_LDDR, F_LDI }, // LDDR
        { _N_, _N_, O_CPI, _T(1, 12), C_CPDR, F_CPI }, // CPDR
        { _N_, _N_, O_INI, _T(1, 12), C_INDR, F_INI }, // INDR
        { _N_, _N_, O_OTI, _T(1, 12), C_OTDR, F_OTI }, // OTDR = 187
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, 255, _T(1, 4), C_IY_NONI },
        { _N_, _N_, 255, _T(1, 4), C_IX_NONI }
};



enum MnemonicStd {
    NOP = 0, LD_BC_NN, LD_PBC_A, INC_BC, INC_B, DEC_B, LD_B_N, RLCA, EX_AF, ADD_HL_BC, LD_A_PBC, DEC_BC, INC_C, DEC_C,
    LD_C_N, RRCA, DJNZ, LD_DE_NN, LD_PDE_A, INC_DE, INC_D, DEC_D, LD_D_N, RLA, JR_N, ADD_HL_DE, LD_A_PDE, DEC_DE, INC_E, DEC_E,
    LD_E_N, RRA, JR_NZ, LD_HL_NN, LD_PNN_HL, INC_HL, INC_H, DEC_H, LD_H_N, DAA, JR_Z, ADD_HL_HL, LD_HL_PNN, DEC_HL, INC_L,
    DEC_L, LD_L_N, CPL, JR_NC, LD_SP_NN, LD_PNN_A, INC_SP, INC_PHL, DEC_PHL, LD_PHL_N, SCF, JR_C, ADD_HL_SP, LD_A_PNN, DEC_SP,
    INC_A, DEC_A, LD_A_N, CCF,/*64*/
    LD_B_B, LD_B_C, LD_B_D, LD_B_E, LD_B_H, LD_B_L, LD_B_PHL, LD_B_A, LD_C_B, LD_C_C, LD_C_D, LD_C_E,
    LD_C_H, LD_C_L, LD_C_PHL, LD_C_A, LD_D_B, LD_D_C, LD_D_D, LD_D_E, LD_D_H, LD_D_L, LD_D_PHL, LD_D_A, LD_E_B, LD_E_C,
    LD_E_D, LD_E_E, LD_E_H, LD_E_L, LD_E_PHL, LD_E_A, LD_H_B, LD_H_C, LD_H_D, LD_H_E, LD_H_H, LD_H_L, LD_H_PHL,
    LD_H_A, LD_L_B, LD_L_C, LD_L_D, LD_L_E, LD_L_H, LD_L_L, LD_L_PHL, LD_L_A, LD_PHL_B, LD_PHL_C, LD_PHL_D, LD_PHL_E,
    LD_PHL_H, LD_PHL_L, HALT, LD_PHL_A, LD_A_B, LD_A_C, LD_A_D, LD_A_E, LD_A_H, LD_A_L, LD_A_PHL, LD_A_A,/*128*/
    ADD_A_B, ADD_A_C, ADD_A_D, ADD_A_E, ADD_A_H, ADD_A_L, ADD_A_PHL, ADD_A_A,
    ADC_A_B, ADC_A_C, ADC_A_D, ADC_A_E, ADC_A_H, ADC_A_L, ADC_A_PHL, ADC_A_A,
    SUB_B, SUB_C, SUB_D, SUB_E, SUB_H, SUB_L, SUB_PHL, SUB_A,
    SBC_A_B, SBC_A_C, SBC_A_D, SBC_A_E, SBC_A_H, SBC_A_L, SBC_A_PHL, SBC_A_A,
    AND_B, AND_C, AND_D, AND_E, AND_H, AND_L, AND_PHL, AND_A,
    XOR_B, XOR_C, XOR_D, XOR_E, XOR_H, XOR_L, XOR_PHL, XOR_A,
    OR_B, OR_C, OR_D, OR_E, OR_H, OR_L, OR_PHL, OR_A,
    CP_B, CP_C, CP_D, CP_E, CP_H, CP_L, CP_PHL, CP_A,/* 192 */
    RET_NZ, POP_BC, JP_NZ, JP_NN, CALL_NZ, PUSH_BC, ADD_A_N, RST0, RET_Z, RET, JP_Z, PREF_CB, CALL_Z, CALL_NN, ADC_A_N, RST8, RET_NC,
    POP_DE, JP_NC, OUT_N_A, CALL_NC, PUSH_DE, SUB_N, RST16, RET_C, EXX, JP_C, IN_A_N, CALL_C, PREF_DD, SBC_A_N, RST24, RET_PO,
    POP_HL, JP_PO, EX_PSP_HL, CALL_PO, PUSH_HL, AND_N, RST32, RET_PE, JP_HL, JP_PE, EX_DE_HL, CALL_PE, PREF_ED, XOR_N, RST40, RET_P,
    POP_AF, JP_P, DI, CALL_P, PUSH_AF, OR_N, RST48, RET_M, LD_SP_HL, JP_M, EI, CALL_M, PREF_FD, CP_N, RST56,
};

enum MnemonicCB {
    RLC_B = 0, RLC_C, RLC_D, RLC_E, RLC_H, RLC_L, RLC_PHL, RLC_A,
    RRC_B, RRC_C, RRC_D, RRC_E, RRC_H, RRC_L, RRC_PHL, RRC_A,
    RL_B, RL_C, RL_D, RL_E, RL_H, RL_L, RL_PHL, RL_A,
    RR_B, RR_C, RR_D, RR_E, RR_H, RR_L, RR_PHL, RR_A,
    SLA_B, SLA_C, SLA_D, SLA_E, SLA_H, SLA_L, SLA_PHL, SLA_A,
    SRA_B, SRA_C, SRA_D, SRA_E, SRA_H, SRA_L, SRA_PHL, SRA_A,
    SLI_B, SLI_C, SLI_D, SLI_E, SLI_H, SLI_L, SLI_PHL, SLI_A,
    SRL_B, SRL_C, SRL_D, SRL_E, SRL_H, SRL_L, SRL_PHL, SRL_A,
    BIT_0_B, BIT_0_C, BIT_0_D, BIT_0_E, BIT_0_H, BIT_0_L, BIT_0_PHL, BIT_0_A,
    BIT_1_B, BIT_1_C, BIT_1_D, BIT_1_E, BIT_1_H, BIT_1_L, BIT_1_PHL, BIT_1_A,
    BIT_2_B, BIT_2_C, BIT_2_D, BIT_2_E, BIT_2_H, BIT_2_L, BIT_2_PHL, BIT_2_A,
    BIT_3_B, BIT_3_C, BIT_3_D, BIT_3_E, BIT_3_H, BIT_3_L, BIT_3_PHL, BIT_3_A,
    BIT_4_B, BIT_4_C, BIT_4_D, BIT_4_E, BIT_4_H, BIT_4_L, BIT_4_PHL, BIT_4_A,
    BIT_5_B, BIT_5_C, BIT_5_D, BIT_5_E, BIT_5_H, BIT_5_L, BIT_5_PHL, BIT_5_A,
    BIT_6_B, BIT_6_C, BIT_6_D, BIT_6_E, BIT_6_H, BIT_6_L, BIT_6_PHL, BIT_6_A,
    BIT_7_B, BIT_7_C, BIT_7_D, BIT_7_E, BIT_7_H, BIT_7_L, BIT_7_PHL, BIT_7_A,
    RES_0_B, RES_0_C, RES_0_D, RES_0_E, RES_0_H, RES_0_L, RES_0_PHL, RES_0_A,
    RES_1_B, RES_1_C, RES_1_D, RES_1_E, RES_1_H, RES_1_L, RES_1_PHL, RES_1_A,
    RES_2_B, RES_2_C, RES_2_D, RES_2_E, RES_2_H, RES_2_L, RES_2_PHL, RES_2_A,
    RES_3_B, RES_3_C, RES_3_D, RES_3_E, RES_3_H, RES_3_L, RES_3_PHL, RES_3_A,
    RES_4_B, RES_4_C, RES_4_D, RES_4_E, RES_4_H, RES_4_L, RES_4_PHL, RES_4_A,
    RES_5_B, RES_5_C, RES_5_D, RES_5_E, RES_5_H, RES_5_L, RES_5_PHL, RES_5_A,
    RES_6_B, RES_6_C, RES_6_D, RES_6_E, RES_6_H, RES_6_L, RES_6_PHL, RES_6_A,
    RES_7_B, RES_7_C, RES_7_D, RES_7_E, RES_7_H, RES_7_L, RES_7_PHL, RES_7_A,
    SET_0_B, SET_0_C, SET_0_D, SET_0_E, SET_0_H, SET_0_L, SET_0_PHL, SET_0_A,
    SET_1_B, SET_1_C, SET_1_D, SET_1_E, SET_1_H, SET_1_L, SET_1_PHL, SET_1_A,
    SET_2_B, SET_2_C, SET_2_D, SET_2_E, SET_2_H, SET_2_L, SET_2_PHL, SET_2_A,
    SET_3_B, SET_3_C, SET_3_D, SET_3_E, SET_3_H, SET_3_L, SET_3_PHL, SET_3_A,
    SET_4_B, SET_4_C, SET_4_D, SET_4_E, SET_4_H, SET_4_L, SET_4_PHL, SET_4_A,
    SET_5_B, SET_5_C, SET_5_D, SET_5_E, SET_5_H, SET_5_L, SET_5_PHL, SET_5_A,
    SET_6_B, SET_6_C, SET_6_D, SET_6_E, SET_6_H, SET_6_L, SET_6_PHL, SET_6_A,
    SET_7_B, SET_7_C, SET_7_D, SET_7_E, SET_7_H, SET_7_L, SET_7_PHL, SET_7_A,
};

enum MnemonicED {
    IN_B_BC = 64, OUT_BC_B, SBC_HL_BC, LD_PNN_BC, NEG, RETN, IM0, LD_I_A, IN_C_BC, OUT_BC_C, ADC_HL_BC, LD_BC_PNN, NEG_1, RETI,
    IM0_1, LD_R_A, IN_D_BC, OUT_BC_D, SBC_HL_DE, LD_PNN_DE, NEG_2, RET_0, IM1, LD_A_I, IN_E_BC, OUT_BC_E, ADC_HL_DE, LD_DE_PNN,
    NEG_3, RET_1, IM2, LD_A_R, IN_H_BC, OUT_BC_H, SBC_HL_HL, LD_PNN_HL1, NEG_4, RET_2, IM0_2, RRD, IN_L_BC, OUT_PC_L, ADC_HL_HL,
    LD_HL1_PNN, NEG_5, RET_3, IM0_3, RLD, IN_F_BC, OUT_BC_0, SBC_HL_SP, LD_PNN_SP, NEG_6, RET_4, IM1_1, NOP_1, IN_A_BC, OUT_BC_A,
    ADC_HL_SP, LD_SP_PNN, NEG_7, RET_5, IM2_1, NOP_2, LDI = 160, CPI, INI, OUTI, LDD = 168, CPD, IND, OUTD,
    LDIR = 176, CPIR, INIR, OTIR, LDDR = 184, CPDR, INDR, OTDR
};
// INC PNN
// DEC PNN
// ADD PNN, N
// ADD PNN, NN
// SUB PNN, N
// SUB PNN, NN
// ADD PNN, RP
// SUB PNN, RP
// ADD PNN, R
// SUB PNN, R
// ADD RP, PNN
// ADD R, PNN
// SUB RP, PNN
// SUB R, PNN

#pragma clang diagnostic pop