//
// Created by Сергей on 01.12.2019.
//

#pragma once

//#define REGS(dst, src)  (dst | src << 8)
#define ADD_A_X(x)      { _N_, x, O_ADD, 4, C_ADD, F_ADSB8 }
#define ADC_A_X(x)      { _N_, x, O_ADC, 4, C_ADC, F_ADSB8 }
#define SUB_A_X(x)      { _N_, x, O_SUB, 4, C_SUB, F_ADSB8 }
#define SBC_A_X(x)      { _N_, x, O_SBC, 4, C_SBC, F_ADSB8 }
#define XOR_A_X(x)      { _N_, x, O_XOR, 4, C_XOR, F_XOR }
#define AND_A_X(x)      { _N_, x, O_AND, 4, C_AND, F_AND }
#define OR_A_X(x)       { _N_, x, O_OR, 4, C_OR, F_OR }
#define CP_A_X(x)       { _N_, x, O_CP, 4, C_CP, F_CP }
#define LD_X_Y(x, y)    { x,  y, O_ASSIGN, 4, C_LD }
#define LD_HL_Y(y)      { _RPHL, y, O_SAVE, 7, C_LD }
#define LD_X_HL(x)      { x, _RPHL, O_LOAD, 7, C_LD }

#define ROT_X(x, c)     { x, _N_, O_ROT, 8, c, F_ROTX }
#define ROT_PX(c)       { _RPHL, _N_, O_ROT, 8, c, F_ROTX }

#define BIT_X(x)        { x, _BT, O_BIT, 8, C_BIT, F_BIT }
#define RES_X(x)        { x, _BT, O_RES, 8, C_RES }
#define SET_X(x)        { x, _BT, O_RES, 15, C_RES }

#define F_ADSBC16   FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_ADSB8     FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_XOR       FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_OR        FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_AND       FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_CP        FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_NEG       FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_ROTX      FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_INI       FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_OTI       FS | FZ | FH | FPV | FN | FC | F3 | F5
#define F_IR        FS | FZ | FH | FPV | FN | F3 | F5
#define F_ID        FS | FZ | FH | FPV | FN | F3 | F5
#define F_IN        FS | FZ | FH | FPV | FN | F3 | F5
#define F_BIT       FS | FZ | FH | FPV | FN | F3 | F5
#define F_RLRD      FS | FZ | FH | FPV | FN | F3 | F5
#define F_F_PC      FS | FZ | FH | FPV | FN | F3 | F5
#define F_CPI       FS | FZ | FH | FPV | FN | F3 | F5
#define F_DAA       FS | FZ | FH | FPV | FC | F3 | F5
#define F_ADD16     FH | FN | FC | F3 | F5
#define F_CCF       FH | FN | FC | F3 | F5
#define F_SCF       FH | FN | FC | F3 | F5
#define F_ROT       FH | FN | FC | F3 | F5
#define F_LDI       FH | FPV | FN | F3 | F5
#define F_CPL       FH | FN | F3 | F5

#define STK_NONI    { _N_, _N_, 255, 8, C_ED_NONI }

#define _RBC        (_R16 | _RC)
#define _RDE        (_R16 | _RE)
#define _RHL        (_R16 | _RL)
#define _RAF        (_R16 | _RF)
#define _RSP        (_R16 | _RS)

static zxCPU::MNEMONIC mnemonics[] = {
        { _N_,  _N_,    O_SPEC,     4,  C_NOP           },  // NOP
        { _RBC, _C16,   O_ASSIGN,   10, C_LD            },  // LD_BC_NN
        { _RBC, _RA,    O_SAVE,     7,  C_LD            },  // LD_PBC_A
        { _RBC, _N_,    O_INC,      6,  C_INC           },  // INC_BC
        { _RB,  _N_,    O_INC,      4,  C_INC,  F_ID    },  // INC_B
        { _RB,  _N_,    O_DEC,      4,  C_DEC,  F_ID    },  // DEC_B
        { _RB,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_B_N
        { _RA,  _N_,    O_ROT,      4,  C_RLCA, F_ROT   },  // RLCA
        { _N_,  _N_,    O_EX,       4,  C_EX_AF         },  // EX_AF_AF'
        { _RHL, _RBC,   O_ADD,      11, C_ADD, F_ADD16  },  // ADD_HL_BC
        { _RA,  _RBC,   O_LOAD,     7,  C_LD            },  // LD_A_PBC
        { _RBC, _N_,    O_DEC,      6,  C_DEC           },  // DEC_BC
        { _RC,  _N_,    O_INC,      4,  C_INC, F_ID     },  // INC_C
        { _RC,  _N_,    O_DEC,      4,  C_DEC, F_ID     },  // DEC_C
        { _RC,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_C_N
        { _RA,  _N_,    O_ROT,      4,  C_RRCA, F_ROT   },  // RRCA
        { _N_,  _C8,    O_SPEC,     13, C_DJNZ          },  // DJNZ
        { _RDE, _C16,   O_ASSIGN,   10, C_LD            },  // LD_DE_NN
        { _RDE, _RA,    O_SAVE,     7,  C_LD            },  // LD_PDE_A
        { _RDE, _N_,    O_INC,      6,  C_INC           },  // INC_DE
        { _RD,  _N_,    O_INC,      4,  C_INC, F_ID     },  // INC_D
        { _RD,  _N_,    O_DEC,      4,  C_DEC, F_ID     },  // DEC_D
        { _RD,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_D_N
        { _RA,  _N_,    O_ROT,      4,  C_RLA, F_ROT    },  // RLA
        { _N_,  _C8,    O_JR,       7,  C_JR            },  // JR_N
        { _RHL, _RDE,   O_ADD,      11, C_ADD, F_ADD16  },  // ADD_HL_DE
        { _RA,  _RDE,   O_LOAD,     7,  C_LD            },  // LD_A_PDE
        { _RDE, _N_,    O_DEC,      6,  C_DEC           },  // DEC_DE
        { _RE,  _N_,    O_INC,      4,  C_INC, F_ID     },  // INC_E
        { _RE,  _N_,    O_DEC,      4,  C_DEC, F_ID     },  // DEC_E
        { _RE,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_E_N
        { _RA,  _N_,    O_ROT,      4,  C_RRA, F_ROT    },  // RRA
        { _N_,  _C8,    O_JR,       7,  C_JR, _NZ       },  // JR_NZ
        { _RHL, _C16,   O_ASSIGN,   10, C_LD            },  // LD_HL_NN
        { _C16, _RHL,   O_SAVE,     16, C_LD            },  // LD_PNN_HL
        { _RHL, _N_,    O_INC,      6,  C_INC           },  // INC_HL
        { _RH,  _N_,    O_INC,      4,  C_INC, F_ID     },  // INC_H
        { _RH,  _N_,    O_DEC,      4,  C_DEC, F_ID     },  // DEC_H
        { _RH,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_H_N
        { _N_,  _N_,    O_SPEC,     4,  C_DAA, F_DAA    },  // DAA
        { _N_,  _C8,    O_JR,       7,  C_JR, _Z        },  // JR_Z
        { _RHL, _RHL,   O_ADD,      11, C_ADD, F_ADD16  },  // ADD_HL_HL
        { _RHL, _C16,   O_LOAD,     16, C_LD            },  // LD_HL_PNN
        { _RHL, _N_,    O_DEC,      6,  C_DEC           },  // DEC_HL
        { _RL,  _N_,    O_INC,      4,  C_INC, F_ID     },  // INC_L
        { _RL,  _N_,    O_DEC,      4,  C_DEC, F_ID     },  // DEC_L
        { _RL,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_L_N
        { _N_,  _N_,    O_SPEC,     4,  C_CPL, F_CPL    },  // CPL
        { _N_,  _C8,    O_JR,       12, C_JR, _NC       },  // JR_NC
        { _RSP, _C16,   O_ASSIGN,   10, C_LD            },  // LD_SP_NN
        { _C16, _RA,    O_SAVE,     13, C_LD            },  // LD_PNN_A
        { _RSP, _N_,    O_INC,      6,  C_INC           },  // INC_SP
        { _RPHL,_N_,    O_INC,      11, C_INC, F_ID     },  // INC_PHL
        { _RPHL,_N_,    O_DEC,      11, C_DEC, F_ID     },  // DEC_PHL
        { _RPHL,_C8,    O_SAVE,     10, C_LD            },  // LD_PHL_N
        { _N_,  _N_,    O_SPEC,     4,  C_SCF, F_SCF    },  // SCF
        { _N_,  _C8,    O_JR,       7,  C_JR, _C        },  // JR_C
        { _RHL, _RSP,   O_ADD,      11, C_ADD, F_ADD16  },  // ADD_HL_SP
        { _RA,  _C16,   O_LOAD,     13, C_LD            },  // LD_A_PNN
        { _RSP, _N_,    O_DEC,      6,  C_DEC           },  // DEC_SP
        { _RA,  _N_,    O_INC,      4,  C_INC, F_ID     },  // INC_A
        { _RA,  _N_,    O_DEC,      4,  C_DEC, F_ID     },  // DEC_A
        { _RA,  _C8,    O_ASSIGN,   7,  C_LD            },  // LD_A_N
        { _N_,  _N_,    O_SPEC,     4,  C_CCF, F_CCF    },  // CCF

        LD_X_Y(_RB, _RB), LD_X_Y(_RB, _RC), LD_X_Y(_RB, _RD), LD_X_Y(_RB, _RE), LD_X_Y(_RB, _RH), LD_X_Y(_RB, _RL), LD_X_HL(_RB), LD_X_Y(_RB, _RA),
        LD_X_Y(_RC, _RB), LD_X_Y(_RC, _RC), LD_X_Y(_RC, _RD), LD_X_Y(_RC, _RE), LD_X_Y(_RC, _RH), LD_X_Y(_RC, _RL), LD_X_HL(_RC), LD_X_Y(_RC, _RA),
        LD_X_Y(_RD, _RB), LD_X_Y(_RD, _RC), LD_X_Y(_RD, _RD), LD_X_Y(_RD, _RE), LD_X_Y(_RD, _RH), LD_X_Y(_RD, _RL), LD_X_HL(_RD), LD_X_Y(_RD, _RA),
        LD_X_Y(_RE, _RB), LD_X_Y(_RE, _RC), LD_X_Y(_RE, _RD), LD_X_Y(_RE, _RE), LD_X_Y(_RE, _RH), LD_X_Y(_RE, _RL), LD_X_HL(_RE), LD_X_Y(_RE, _RA),
        LD_X_Y(_RH, _RB), LD_X_Y(_RH, _RC), LD_X_Y(_RH, _RD), LD_X_Y(_RH, _RE), LD_X_Y(_RH, _RH), LD_X_Y(_RH, _RL), LD_X_HL(_RH), LD_X_Y(_RH, _RA),
        LD_X_Y(_RL, _RB), LD_X_Y(_RL, _RC), LD_X_Y(_RL, _RD), LD_X_Y(_RL, _RE), LD_X_Y(_RL, _RH), LD_X_Y(_RL, _RL), LD_X_HL(_RL), LD_X_Y(_RL, _RA),
        LD_HL_Y(_RB), LD_HL_Y(_RC), LD_HL_Y(_RD), LD_HL_Y(_RE), LD_HL_Y(_RH), LD_HL_Y(_RL), { _N_,  _N_, O_SPEC, 4, C_HALT }, LD_HL_Y(_RA),
        LD_X_Y(_RA, _RB), LD_X_Y(_RA, _RC), LD_X_Y(_RA, _RD), LD_X_Y(_RA, _RE), LD_X_Y(_RA, _RH), LD_X_Y(_RA, _RL), LD_X_HL(_RA), LD_X_Y(_RA, _RA),

        ADD_A_X(_RB), ADD_A_X(_RC), ADD_A_X(_RD), ADD_A_X(_RE), ADD_A_X(_RH), ADD_A_X(_RL), { _N_, _RPHL, O_ADD, 7, C_ADD, F_ADSB8 }, ADD_A_X(_RA),
        ADC_A_X(_RB), ADC_A_X(_RC), ADC_A_X(_RD), ADC_A_X(_RE), ADC_A_X(_RH), ADC_A_X(_RL), { _N_, _RPHL, O_ADC, 7, C_ADC, F_ADSB8 }, ADC_A_X(_RA),
        SUB_A_X(_RB), SUB_A_X(_RC), SUB_A_X(_RD), SUB_A_X(_RE), SUB_A_X(_RH), SUB_A_X(_RL), { _N_, _RPHL, O_SUB, 7, C_SUB, F_ADSB8 }, SUB_A_X(_RA),
        SBC_A_X(_RB), SBC_A_X(_RC), SBC_A_X(_RD), SBC_A_X(_RE), SBC_A_X(_RH), SBC_A_X(_RL), { _N_, _RPHL, O_SBC, 7, C_SBC, F_ADSB8 }, SBC_A_X(_RA),
        AND_A_X(_RB), AND_A_X(_RC), AND_A_X(_RD), AND_A_X(_RE), AND_A_X(_RH), AND_A_X(_RL), { _N_, _RPHL, O_AND, 7, C_AND, F_AND }, AND_A_X(_RA),
        XOR_A_X(_RB), XOR_A_X(_RC), XOR_A_X(_RD), XOR_A_X(_RE), XOR_A_X(_RH), XOR_A_X(_RL), { _N_, _RPHL, O_XOR, 7, C_XOR, F_XOR }, XOR_A_X(_RA),
        OR_A_X(_RB), OR_A_X(_RC), OR_A_X(_RD), OR_A_X(_RE), OR_A_X(_RH), OR_A_X(_RL), { _N_, _RPHL, O_OR, 7, C_OR, F_OR }, OR_A_X(_RA),
        CP_A_X(_RB), CP_A_X(_RC), CP_A_X(_RD), CP_A_X(_RE), CP_A_X(_RH), CP_A_X(_RL), { _N_, _RPHL, O_OR, 7, C_CP, F_CP }, CP_A_X(_RA),

        { _N_,  _N_,    O_RET,  5,  C_RET,  _NZ     },  // RET_NZ
        { _RBC, _N_,    O_POP,  10, C_POP           },  // POP_BC
        { _N_,  _C16,   O_JMP,  10, C_JP,   _NZ     },  // JP_NZ
        { _N_,  _C16,   O_JMP,  10, C_JP            },  // JP_NN
        { _N_,  _C16,   O_CALL, 10, C_CALL, _NZ     },  // CALL_NZ
        { _RBC, _N_,    O_PUSH, 11, C_PUSH          },  // PUSH_BC
        { _N_,  _C8,    O_ADD,  7,  C_ADD,  F_ADSB8 },  // ADD_A_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST0
        { _N_,  _N_,    O_RET,  5,  C_RET,  _Z      },  // RET_Z
        { _N_,  _N_,    O_RET,  10, C_RET           },  // RET
        { _N_,  _C16,   O_JMP,  10, C_JP,   _Z      },  // JP_Z
        { _N_,  _N_,    O_PREF, 4,  C_NULL          },  // PREF_CB
        { _N_,  _C16,   O_CALL, 10, C_CALL, _Z      },  // CALL_Z
        { _N_,  _C16,   O_CALL, 17, C_CALL          },  // CALL_NN
        { _N_,  _C8,    O_ADC,  7,  C_ADC,  F_ADSB8 },  // ADC_A_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST8
        { _N_,  _N_,    O_RET,  5,  C_RET,  _NC     },  // RET_NC
        { _RDE, _N_,    O_POP,  10, C_POP           },  // POP_DE
        { _N_,  _C16,   O_JMP,  10, C_JP,   _NC     },  // JP_NC
        { _RA,  _C8,    O_OUT,  11, C_OUT           },  // OUT_PN_A
        { _N_,  _C16,   O_CALL, 10, C_CALL, _NC     },  // CALL_NC
        { _RDE, _N_,    O_PUSH, 11, C_PUSH          },  // PUSH_DE
        { _N_,  _C8,    O_SUB,  7,  C_SUB,  F_ADSB8 },  // SUB_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST16
        { _N_,  _N_,    O_RET,  5,  C_RET,  _C      },  // RET_C
        { _N_,  _N_,    O_EX,   4,  C_EXX           },  // EXX
        { _N_,  _C16,   O_JMP,  10, C_JP,   _C      },  // JP_C
        { _RA,  _C8,    O_IN,   11, C_IN            },  // IN_A_PN
        { _N_,  _C16,   O_CALL, 10, C_CALL, _C      },  // CALL_C
        { _N_,  _N_,    O_PREF, 4,  C_NULL          },  // PREF_DD
        { _N_,  _C8,    O_SBC,  7,  C_SBC,  F_ADSB8 },  // SBC_A_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST24
        { _N_,  _N_,    O_RET,  5,  C_RET,  _PO     },  // RET_PO
        { _RHL, _N_,    O_POP,  10, C_POP           },  // POP_HL
        { _N_,  _C16,   O_JMP,  10, C_JP,   _PO     },  // JP_PO
        { _RHL, _N_,    O_EX,   19, C_EX_SP         },  // EX_PSP_HL
        { _N_,  _C16,   O_CALL, 10, C_CALL, _PO     },  // CALL_PO
        { _RHL, _N_,    O_PUSH, 11, C_PUSH          },  // PUSH_HL
        { _N_,  _C8,    O_AND,  7,  C_AND,  F_AND   },  // AND_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST32
        { _N_,  _N_,    O_RET,  5,  C_RET,  _PE     },  // RET_PE
        { _RHL, _N_,    O_SPEC, 4,  C_JP            },  // JP_HL
        { _N_,  _C16,   O_JMP,  10, C_JP,   _PE     },  // JP_PE
        { _N_,  _N_,    O_EX,   4,  C_EX_DE         },  // EX_DE_HL
        { _N_,  _C16,   O_CALL, 10, C_CALL, _PE     },  // CALL_PE
        { _N_,  _N_,    O_PREF, 4,  C_NULL          },  // PREF_ED
        { _N_,  _C8,    O_XOR,  7,  C_XOR,  F_XOR   },  // XOR_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST40
        { _N_,  _N_,    O_RET,  5,  C_RET,  _P      },  // RET_P
        { _RAF, _N_,    O_POP,  10, C_POP           },  // POP_AF
        { _N_,  _C16,   O_JMP,  10, C_JP,   _P      },  // JP_P
        { _N_,  _N_,    O_SPEC, 4,  C_DI            },  // DI
        { _N_,  _C16,   O_CALL, 10, C_CALL, _P      },  // CALL_P
        { _RAF, _N_,    O_PUSH, 11, C_PUSH          },  // PUSH_AF
        { _N_,  _C8,    O_OR,   7,  C_OR,   F_OR    },  // OR_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST48
        { _N_,  _N_,    O_RET,  5,  C_RET,  _M      },  // RET_M
        { _RSP, _RHL,   O_ASSIGN,4, C_LD            },  // LD_SP_HL
        { _N_,  _C16,   O_JMP,  10, C_JP,   _M      },  // JP_M
        { _N_,  _N_,    O_SPEC, 4,  C_EI            },  // EI
        { _N_,  _C16,   O_CALL, 10, C_CALL, _M      },  // CALL_M
        { _N_,  _N_,    O_PREF, 4,  C_NULL          },  // PREF_FD
        { _N_,  _C8,    O_CP,   7,  C_CP,   F_CP    },  // CP_N
        { _N_,  _N_,    O_RST,  11, C_RST           },  // RST56
        // 203
        ROT_X(_RB, C_RLC), ROT_X(_RC, C_RLC), ROT_X(_RD, C_RLC), ROT_X(_RE, C_RLC), ROT_X(_RH, C_RLC), ROT_X(_RL, C_RLC), ROT_PX(C_RLC), ROT_X(_RA, C_RLC),
        ROT_X(_RB, C_RRC), ROT_X(_RC, C_RRC), ROT_X(_RD, C_RRC), ROT_X(_RE, C_RRC), ROT_X(_RH, C_RRC), ROT_X(_RL, C_RRC), ROT_PX(C_RRC), ROT_X(_RA, C_RRC),
        ROT_X(_RB, C_RL),  ROT_X(_RC, C_RL),  ROT_X(_RD, C_RL),  ROT_X(_RE, C_RL),  ROT_X(_RH, C_RL),  ROT_X(_RL, C_RL),  ROT_PX(C_RL),  ROT_X(_RA, C_RL),
        ROT_X(_RB, C_RR),  ROT_X(_RC, C_RR),  ROT_X(_RD, C_RR),  ROT_X(_RE, C_RR),  ROT_X(_RH, C_RR),  ROT_X(_RL, C_RR),  ROT_PX(C_RR),  ROT_X(_RA, C_RR),
        ROT_X(_RB, C_SLA), ROT_X(_RC, C_SLA), ROT_X(_RD, C_SLA), ROT_X(_RE, C_SLA), ROT_X(_RH, C_SLA), ROT_X(_RL, C_SLA), ROT_PX(C_SLA), ROT_X(_RA, C_SLA),
        ROT_X(_RB, C_SRA), ROT_X(_RC, C_SRA), ROT_X(_RD, C_SRA), ROT_X(_RE, C_SRA), ROT_X(_RH, C_SRA), ROT_X(_RL, C_SRA), ROT_PX(C_SRA), ROT_X(_RA, C_SRA),
        ROT_X(_RB, C_SLI), ROT_X(_RC, C_SLI), ROT_X(_RD, C_SLI), ROT_X(_RE, C_SLI), ROT_X(_RH, C_SLI), ROT_X(_RL, C_SLI), ROT_PX(C_SLI), ROT_X(_RA, C_SLI),
        ROT_X(_RB, C_SRL), ROT_X(_RC, C_SRL), ROT_X(_RD, C_SRL), ROT_X(_RE, C_SRL), ROT_X(_RH, C_SRL), ROT_X(_RL, C_SRL), ROT_PX(C_SRL), ROT_X(_RA, C_SRL),

        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),
        BIT_X(_RB), BIT_X(_RC), BIT_X(_RD), BIT_X(_RE), BIT_X(_RH), BIT_X(_RL), { _RPHL, _BT, O_BIT, 12, C_BIT, F_BIT }, BIT_X(_RA),

        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),
        RES_X(_RB), RES_X(_RC), RES_X(_RD), RES_X(_RE), RES_X(_RH), RES_X(_RL), { _RPHL, _BT, O_RES, 15, C_RES }, RES_X(_RA),

        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),
        SET_X(_RB), SET_X(_RC), SET_X(_RD), SET_X(_RE), SET_X(_RH), SET_X(_RL), { _RPHL, _BT, O_RES, 15, C_SET }, SET_X(_RA),

        // 237
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _RB,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_B_BC = 64
        { _RB,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_B
        { _RHL, _RBC,   O_SBC,  15, C_SBC,  F_ADSBC16}, // SBC_HL_BC
        { _C16, _RBC,   O_SAVE, 20, C_LD            },  // LD_PNN_BC
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG
        { _N_,  _N_,    O_RETN, 14, C_RETN          },  // RETN
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM0
        { _RI,  _RA,    O_ASSIGN,9, C_LD            },  // LD_I_A
        { _RC,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_C_BC
        { _RC,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_C
        { _RHL, _RBC,   O_ADC,  15, C_ADC,  F_ADSBC16}, // ADC_HL_BC
        { _RBC, _C16,   O_LOAD, 20, C_LD            },  // LD_BC_PNN
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_1
        { _N_,  _N_,    O_RETN, 14, C_RETI          },  // RETI
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM0_1
        { _RR,  _RA,    O_ASSIGN,9, C_LD            },  // LD_R_A
        { _RD,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_D_BC
        { _RD,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_D
        { _RHL, _RDE,   O_SBC,  15, C_SBC,  F_ADSBC16}, // SBC_HL_DE
        { _C16, _RDE,   O_SAVE, 20, C_LD            },  // LD_PNN_DE
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_2
        { _N_,  _N_,    O_RET,  14, C_RET           },  // RET_0
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM1
        { _RA,  _RI,    O_ASSIGN,9, C_LD,   F_IR    },  // LD_A_I
        { _RE,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_E_BC
        { _RE,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_E
        { _RHL, _RDE,   O_ADC,  15, C_ADC,  F_ADSBC16}, // ADC_HL_DE
        { _RDE, _C16,   O_LOAD, 20, C_LD            },  // LD_DE_PNN
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_3
        { _N_,  _N_,    O_RET,  14, C_RET           },  // RET_1
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM2
        { _RA,  _RR,    O_ASSIGN,9, C_LD,   F_IR    },  // LD_A_R
        { _RH,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_H_BC
        { _RH,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_H
        { _RHL, _RHL,   O_SBC,  15, C_SBC,  F_ADSBC16}, // SBC_HL_HL
        { _C16, _RHL,   O_SAVE, 20, C_LD            },  // LD_PNN_HL1
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_4
        { _N_,  _N_,    O_RET,  14, C_RET           },  // RET_2=101
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM0_2
        { _RHL, _RA,    O_SPEC, 18, C_RRD,  F_RLRD  },  // RRD
        { _RL,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_L_BC
        { _RL,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_L
        { _RHL, _RHL,   O_ADC,  15, C_ADC,  F_ADSBC16}, // ADC_HL_HL
        { _RHL, _C16,   O_LOAD, 20, C_LD            },  // LD_HL1_PNN
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_5
        { _N_,  _N_,    O_RET,  14, C_RET           },  // RET_3
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM0_3
        { _RHL, _RA,    O_SPEC, 18, C_RLD,  F_RLRD  },  // RLD
        { _N_,  _N_,    O_SPEC, 12, C_IN,   F_F_PC  },  // IN_F_PC
        { _N_,  _N_,    O_SPEC, 12, C_OUT           },  // OUT_PC_0
        { _RHL, _RSP,   O_SBC,  15, C_SBC,  F_ADSBC16}, // SBC_HL_SP
        { _C16, _RSP,   O_SAVE, 20, C_LD            },  // LD_PNN_SP
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_6
        { _N_,  _N_,    O_RET,  14, C_RET           },  // RET_4
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM1_1
        { _RHL, _RA,    O_SPEC, 8,  C_NOP           },  // NOP_1=119
        { _RA,  _RC,    O_IN,   12, C_IN,   F_IN    },  // IN_A_BC
        { _RA,  _RC,    O_OUT,  12, C_OUT           },  // OUT_PC_A
        { _RHL, _RSP,   O_ADC,  15, C_ADC,  F_ADSBC16}, // ADC_HL_SP
        { _RSP, _C16,   O_LOAD, 20, C_LD            },  // LD_SP_PNN
        { _N_,  _N_,    O_NEG,  4,  C_NEG,  F_NEG   },  // NEG_7
        { _N_,  _N_,    O_RET,  14, C_RET           },  // RET_5
        { _N_,  _N_,    O_IM,   8,  C_IM            },  // IM2_1
        { _RHL, _RA,    O_SPEC, 8,  C_NOP           },  // NOP_2 = 127
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, 16, C_LDI, F_LDI },  // LDI
        { _N_, _N_, O_CPI, 16, C_CPI, F_CPI },  // CPI
        { _N_, _N_, O_INI, 16, C_INI, F_INI },  // INI
        { _N_, _N_, O_OTI, 16, C_OTI, F_OTI },  // OTI
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, 16, C_LDD, F_LDI },  // LDD
        { _N_, _N_, O_CPI, 16, C_CPD, F_CPI },  // CPD
        { _N_, _N_, O_INI, 16, C_IND, F_INI },  // IND
        { _N_, _N_, O_OTI, 16, C_OTD, F_OTI },  // OTD
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, 16, C_LDIR, F_LDI},  // LDIR
        { _N_, _N_, O_CPI, 16, C_CPIR, F_CPI }, // CPIR
        { _N_, _N_, O_INI, 16, C_INIR, F_INI }, // INIR
        { _N_, _N_, O_OTI, 16, C_OTIR, F_OTI }, // OTIR
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { _N_, _N_, O_LDI, 16, C_LDDR, F_LDI }, // LDDR
        { _N_, _N_, O_CPI, 16, C_CPDR, F_CPI }, // CPDR
        { _N_, _N_, O_INI, 16, C_INDR, F_INI }, // INDR
        { _N_, _N_, O_OTI, 16, C_OTDR, F_OTI }, // OTDR
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI
};
