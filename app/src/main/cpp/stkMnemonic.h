//
// Created by Сергей on 01.12.2019.
//

#pragma once

#define REGS(dst, src)  (dst | src << 4)

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

#define STK_NONI    { REGS(_RN, _RN), 255, 8, C_ED_NONI }

static zxCPU::MNEMONIC mnemonics[] = {
        { REGS(_RN, _RN), O_SPEC, 4, C_NOP },           // NOP
        { REGS(_RBC, _RN), O_ASSIGN, 10 | CNN, C_LD },  // LD_BC_NN
        { REGS(_RBC, _RA), O_SAVE, 7, C_LD },           // LD_PBC_A
        { REGS(_RBC, _RN), O_INC, 6, C_INC },           // INC_BC
        { REGS(_RB, _RN), O_INC, 4, C_INC, F_ID },      // INC_B
        { REGS(_RB, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_B
        { REGS(_RB, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_B_N
        { REGS(_RA, _RN), O_ROT, 4, C_RLCA, F_ROT },    // RLCA
        { REGS(_RN, _RN), O_SPEC, 4, C_EX_AF },         // EX_AF_AF'
        { REGS(_RHL, _RBC), O_ADD, 11, C_ADD, F_ADD16 },// ADD_HL_BC
        { REGS(_RA, _RBC), O_LOAD, 7, C_LD },           // LD_A_PBC
        { REGS(_RBC, _RN), O_DEC, 6, C_DEC },           // DEC_BC
        { REGS(_RC, _RN), O_INC, 4, C_INC, F_ID },      // INC_C
        { REGS(_RC, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_C
        { REGS(_RC, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_C_N
        { REGS(_RA, _RN), O_ROT, 4, C_RRCA, F_ROT },    // RRCA
        { REGS(_RN, _RN), O_SPEC, 13 | CN, C_DJNZ },    // DJNZ
        { REGS(_RDE, _RN), O_ASSIGN, 10 | CNN, C_LD },  // LD_DE_NN
        { REGS(_RDE, _RA), O_SAVE, 7, C_LD },           // LD_PDE_A
        { REGS(_RDE, _RN), O_INC, 6, C_INC },           // INC_DE
        { REGS(_RD, _RN), O_INC, 4, C_INC, F_ID },      // INC_D
        { REGS(_RD, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_D
        { REGS(_RD, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_D_N
        { REGS(_RA, _RN), O_ROT, 4, C_RLA, F_ROT },     // RLA
        { REGS(_RN, _RN), O_JR, 7 | CN, C_JR },         // JR_N
        { REGS(_RHL, _RDE), O_ADD, 11, C_ADD, F_ADD16 },// ADD_HL_DE
        { REGS(_RA, _RDE), O_LOAD, 7, C_LD },           // LD_A_PDE
        { REGS(_RDE, _RN), O_DEC, 6, C_DEC },           // DEC_DE
        { REGS(_RE, _RN), O_INC, 4, C_INC, F_ID },      // INC_E
        { REGS(_RE, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_E
        { REGS(_RE, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_E_N
        { REGS(_RA, _RN), O_ROT, 4, C_RRA, F_ROT },     // RRA
        { REGS(_RN, _RN), O_JR, 7 | CN, C_JR, _NZ },    // JR_NZ
        { REGS(_RHL, _RN), O_ASSIGN, 10 | CNN, C_LD },  // LD_HL_NN
        { REGS(_RN, _RHL), O_SAVE, 16 | CNN, C_LD },    // LD_PNN_HL
        { REGS(_RHL, _RN), O_INC, 6, C_INC },           // INC_HL
        { REGS(_RH, _RN), O_INC, 4, C_INC, F_ID },      // INC_H
        { REGS(_RH, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_H
        { REGS(_RH, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_H_N
        { REGS(_RN, _RN), O_SPEC, 4, C_DAA, F_DAA },    // DAA
        { REGS(_RN, _RN), O_JR, 7 | CN, C_JR, _Z },     // JR_Z
        { REGS(_RHL, _RHL), O_ADD, 11, C_ADD, F_ADD16 },// ADD_HL_HL
        { REGS(_RHL, _RN), O_LOAD, 16 | CNN, C_LD },    // LD_HL_PNN
        { REGS(_RHL, _RN), O_DEC, 6, C_DEC },           // DEC_HL
        { REGS(_RL, _RN), O_INC, 4, C_INC, F_ID },      // INC_L
        { REGS(_RL, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_L
        { REGS(_RL, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_L_N
        { REGS(_RN, _RN), O_SPEC, 4, C_CPL, F_CPL},     // CPL
        { REGS(_RN, _RN), O_JR, 12 | CN, C_JR, _NC },   // JR_NC
        { REGS(_RSP, _RN), O_ASSIGN, 10 | CNN, C_LD },  // LD_SP_NN
        { REGS(_RN, _RA), O_SAVE, 13 | CNN, C_LD },     // LD_PNN_A
        { REGS(_RSP, _RN), O_INC, 6, C_INC },           // INC_SP
        { REGS(_RPHL, _RN), O_INC, 11, C_INC, F_ID },   // INC_PHL
        { REGS(_RPHL, _RN), O_DEC, 11, C_DEC, F_ID },   // DEC_PHL
        { REGS(_RPHL, _RN), O_SAVE, 10 | CN, C_LD },    // LD_PHL_N
        { REGS(_RN, _RN), O_SPEC, 4, C_SCF, F_SCF },    // SCF
        { REGS(_RN, _RN), O_JR, 7 | CN, C_JR, _C },     // JR_C
        { REGS(_RHL, _RSP), O_ADD, 11, C_ADD, F_ADD16 },// ADD_HL_SP
        { REGS(_RA, _RN), O_LOAD, 13 | CNN, C_LD },     // LD_A_PNN
        { REGS(_RSP, _RN), O_DEC, 6, C_DEC },           // DEC_SP
        { REGS(_RA, _RN), O_INC, 4, C_INC, F_ID },      // INC_A
        { REGS(_RA, _RN), O_DEC, 4, C_DEC, F_ID },      // DEC_A
        { REGS(_RA, _RN), O_ASSIGN, 7 | CN, C_LD },     // LD_A_N
        { REGS(_RN, _RN), O_SPEC, 4, C_CCF, F_CCF },    // CCF
        { REGS(_RB, _RB), O_ASSIGN, 4, C_LD }, /* LD_B_B */ { REGS(_RB, _RC), O_ASSIGN, 4, C_LD }, /* LD_B_C */ { REGS(_RB, _RD), O_ASSIGN, 4, C_LD }, /* LD_B_D */
        { REGS(_RB, _RE), O_ASSIGN, 4, C_LD }, /* LD_B_E */ { REGS(_RB, _RH), O_ASSIGN, 4, C_LD }, /* LD_B_H */ { REGS(_RB, _RL), O_ASSIGN, 4, C_LD }, /* LD_B_L */
        { REGS(_RB, _RPHL), O_LOAD, 7, C_LD }, /* LD_B_PHL */ { REGS(_RB, _RA), O_ASSIGN, 4, C_LD }, /* LD_B_A */
        { REGS(_RC, _RB), O_ASSIGN, 4, C_LD }, /* LD_C_B */ { REGS(_RC, _RC), O_ASSIGN, 4, C_LD }, /* LD_C_C */ { REGS(_RC, _RD), O_ASSIGN, 4, C_LD }, /* LD_C_D */
        { REGS(_RC, _RE), O_ASSIGN, 4, C_LD }, /* LD_C_E */ { REGS(_RC, _RH), O_ASSIGN, 4, C_LD }, /* LD_C_H */ { REGS(_RC, _RL), O_ASSIGN, 4, C_LD }, /* LD_C_L */
        { REGS(_RC, _RPHL), O_LOAD, 7, C_LD }, /* LD_C_PHL */ { REGS(_RC, _RA), O_ASSIGN, 4, C_LD }, /* LD_C_A */
        { REGS(_RD, _RB), O_ASSIGN, 4, C_LD }, /* LD_D_B */ { REGS(_RD, _RC), O_ASSIGN, 4, C_LD }, /* LD_D_C */ { REGS(_RD, _RD), O_ASSIGN, 4, C_LD }, /* LD_D_D */
        { REGS(_RD, _RE), O_ASSIGN, 4, C_LD }, /* LD_D_E */ { REGS(_RD, _RH), O_ASSIGN, 4, C_LD }, /* LD_D_H */ { REGS(_RD, _RL), O_ASSIGN, 4, C_LD }, /* LD_D_L */
        { REGS(_RD, _RPHL), O_LOAD, 7, C_LD }, /* LD_D_PHL */ { REGS(_RD, _RA), O_ASSIGN, 4, C_LD }, /* LD_D_A */
        { REGS(_RE, _RB), O_ASSIGN, 4, C_LD }, /* LD_E_B */ { REGS(_RE, _RC), O_ASSIGN, 4, C_LD }, /* LD_E_C */ { REGS(_RE, _RD), O_ASSIGN, 4, C_LD }, /* LD_E_D */
        { REGS(_RE, _RE), O_ASSIGN, 4, C_LD }, /* LD_E_E */ { REGS(_RE, _RH), O_ASSIGN, 4, C_LD }, /* LD_E_H */ { REGS(_RE, _RL), O_ASSIGN, 4, C_LD }, /* LD_E_L*/
        { REGS(_RE, _RPHL), O_LOAD, 7, C_LD }, /* LD_E_PHL */ { REGS(_RE, _RA), O_ASSIGN, 4, C_LD }, /* LD_E_A*/
        { REGS(_RH, _RB), O_ASSIGN, 4, C_LD }, /* LD_H_B */ { REGS(_RH, _RC), O_ASSIGN, 4, C_LD }, /* LD_H_C */ { REGS(_RH, _RD), O_ASSIGN, 4, C_LD }, /* LD_H_D */
        { REGS(_RH, _RE), O_ASSIGN, 4, C_LD }, /* LD_H_E */ { REGS(_RH, _RH), O_ASSIGN, 4, C_LD }, /* LD_H_H */ { REGS(_RH, _RL), O_ASSIGN, 4, C_LD }, /* LD_H_L */
        { REGS(_RH, _RPHL), O_LOAD, 7, C_LD }, /* LD_H_PHL */ { REGS(_RH, _RA), O_ASSIGN, 4, C_LD }, /* LD_H_A*/
        { REGS(_RL, _RB), O_ASSIGN, 4, C_LD }, /* LD_L_B */ { REGS(_RL, _RC), O_ASSIGN, 4, C_LD }, /* LD_L_C */ { REGS(_RL, _RD), O_ASSIGN, 4, C_LD }, /* LD_L_D */
        { REGS(_RL, _RE), O_ASSIGN, 4, C_LD }, /* LD_L_E */ { REGS(_RL, _RH), O_ASSIGN, 4, C_LD }, /* LD_L_H */ { REGS(_RL, _RL), O_ASSIGN, 4, C_LD }, /* LD_L_L */
        { REGS(_RL, _RPHL), O_LOAD, 7, C_LD }, /* LD_L_PHL */ { REGS(_RL, _RA), O_ASSIGN, 4, C_LD }, /* LD_L_A */
        { REGS(_RPHL, _RB), O_SAVE, 7, C_LD }, /* LD_PHL_B */ { REGS(_RPHL, _RC), O_SAVE, 7, C_LD }, /* LD_PHL_C */ { REGS(_RPHL, _RD), O_SAVE, 7, C_LD }, /* LD_PHL_D */
        { REGS(_RPHL, _RE), O_SAVE, 7, C_LD }, /* LD_PHL_E */ { REGS(_RPHL, _RH), O_SAVE, 7, C_LD }, /* LD_PHL_H */ { REGS(_RPHL, _RL), O_SAVE, 7, C_LD }, /* LD_PHL_L */
        { REGS(_RN, _RN), O_SPEC, 4, C_HALT }, /* HALT */ { REGS(_RPHL, _RA), O_SAVE, 7, C_LD }, /* LD_PHL_A */
        { REGS(_RA, _RB), O_ASSIGN, 4, C_LD }, /* LD_A_B */ { REGS(_RA, _RC), O_ASSIGN, 4, C_LD }, /* LD_A_C */ { REGS(_RA, _RD), O_ASSIGN, 4, C_LD }, /* LD_A_D */
        { REGS(_RA, _RE), O_ASSIGN, 4, C_LD }, /* LD_A_E */ { REGS(_RA, _RH), O_ASSIGN, 4, C_LD }, /* LD_A_H */ { REGS(_RA, _RL), O_ASSIGN, 4, C_LD }, /* LD_A_L */
        { REGS(_RA, _RPHL), O_LOAD, 7, C_LD }, /* LD_A_PHL */ { REGS(_RA, _RA), O_ASSIGN, 4, C_LD }, /* LD_A_A */

        { REGS(_RN, _RB), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_B */ { REGS(_RN, _RC), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_C */
        { REGS(_RN, _RD), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_D */ { REGS(_RN, _RE), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_E */
        { REGS(_RN, _RH), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_H */ { REGS(_RN, _RL), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_L */
        { REGS(_RN, _RPHL), O_ADD, 7, C_ADD, F_ADSB8 }  , /* ADD_A_PHL */ { REGS(_RN, _RA), O_ADD, 4, C_ADD, F_ADSB8 }  , /* ADD_A_A */

        { REGS(_RN, _RB), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_B */ { REGS(_RN, _RC), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_C */
        { REGS(_RN, _RD), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_D */ { REGS(_RN, _RE), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_E */
        { REGS(_RN, _RH), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_H */ { REGS(_RN, _RL), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_A */
        { REGS(_RN, _RPHL), O_ADC, 7, C_ADC, F_ADSB8 }, /* ADC_A_PHL */ { REGS(_RN, _RA), O_ADC, 4, C_ADC, F_ADSB8 }, /* ADC_A_A */

        { REGS(_RN, _RB), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_B */ { REGS(_RN, _RC), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_C */
        { REGS(_RN, _RD), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_D */ { REGS(_RN, _RE), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_E */
        { REGS(_RN, _RH), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_H */ { REGS(_RN, _RL), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_L */
        { REGS(_RN, _RPHL), O_SUB, 7, C_SUB, F_ADSB8 }, /* SUB_A_PHL */ { REGS(_RN, _RA), O_SUB, 4, C_SUB, F_ADSB8 }, /* SUB_A_A */

        { REGS(_RN, _RB), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_B */ { REGS(_RN, _RC), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_C */
        { REGS(_RN, _RD), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_D */ { REGS(_RN, _RE), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_E */
        { REGS(_RN, _RH), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_H */ { REGS(_RN, _RL), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_L */
        { REGS(_RN, _RPHL), O_SBC, 7, C_SBC, F_ADSB8 }, /* SBC_A_PHL */ { REGS(_RN, _RA), O_SBC, 4, C_SBC, F_ADSB8 }, /* SBC_A_A */

        { REGS(_RN, _RB), O_AND, 4, C_AND, F_AND }, /* AND_A_B */ { REGS(_RN, _RC), O_AND, 4, C_AND, F_AND }, /* AND_A_C */
        { REGS(_RN, _RD), O_AND, 4, C_AND, F_AND }, /* AND_A_D */ { REGS(_RN, _RE), O_AND, 4, C_AND, F_AND }, /* AND_A_E */
        { REGS(_RN, _RH), O_AND, 4, C_AND, F_AND }, /* AND_A_H */ { REGS(_RN, _RL), O_AND, 4, C_AND, F_AND }, /* AND_A_ */
        { REGS(_RN, _RPHL), O_AND, 7, C_AND, F_AND }, /* AND_A_PHL */ { REGS(_RN, _RA), O_AND, 4, C_AND, F_AND }, /* AND_A_A */

        { REGS(_RN, _RB), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_B */ { REGS(_RN, _RC), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_C */
        { REGS(_RN, _RD), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_D */ { REGS(_RN, _RE), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_E */
        { REGS(_RN, _RH), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_H */ { REGS(_RN, _RL), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_L */
        { REGS(_RN, _RPHL), O_XOR, 7, C_XOR, F_XOR }, /* XOR_A_PHL */ { REGS(_RN, _RA), O_XOR, 4, C_XOR, F_XOR }, /* XOR_A_A */

        { REGS(_RN, _RB), O_OR, 4, C_OR, F_OR }, /* OR_A_B */ { REGS(_RN, _RC), O_OR, 4, C_OR, F_OR }, /* OR_A_C */
        { REGS(_RN, _RD), O_OR, 4, C_OR, F_OR }, /* OR_A_D */ { REGS(_RN, _RE), O_OR, 4, C_OR, F_OR }, /* OR_A_E */
        { REGS(_RN, _RH), O_OR, 4, C_OR, F_OR }, /* OR_A_H */ { REGS(_RN, _RL), O_OR, 4, C_OR, F_OR }, /* OR_A_L */
        { REGS(_RN, _RPHL), O_OR, 7, C_OR, F_OR }, /* OR_A_PHL */ { REGS(_RN, _RA), O_OR, 4, C_OR, F_OR }, /* OR_A_A */

        { REGS(_RN, _RB), O_CP, 4, C_CP, F_CP }, /* CP_A_B */ { REGS(_RN, _RC), O_CP, 4, C_CP, F_CP }, /* CP_A_C */
        { REGS(_RN, _RD), O_CP, 4, C_CP, F_CP }, /* CP_A_D */ { REGS(_RN, _RE), O_CP, 4, C_CP, F_CP }, /* CP_A_E */
        { REGS(_RN, _RH), O_CP, 4, C_CP, F_CP }, /* CP_A_H */ { REGS(_RN, _RL), O_CP, 4, C_CP, F_CP }, /* CP_A_L */
        { REGS(_RN, _RPHL), O_CP, 7, C_CP, F_CP }, /* CP_A_PHL */ { REGS(_RN, _RA), O_CP, 4, C_CP, F_CP }, /* CP_A_A */

        { REGS(_RN, _RN), O_RET, 5, C_RET, _NZ },           // RET_NZ
        { REGS(_RBC, _RN), O_POP, 10, C_POP },              // POP_BC
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _NZ },     // JP_NZ
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP },          // JP_NN
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _NZ },  // CALL_NZ
        { REGS(_RBC, _RN), O_PUSH, 11, C_PUSH },            // PUSH_BC
        { REGS(_RN, _RN), O_ADD, 7 | CN, C_ADD, F_ADSB8 },  // ADD_A_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST0
        { REGS(_RN, _RN), O_RET, 5, C_RET, _Z },            // RET_Z
        { REGS(_RN, _RN), O_RET, 10, C_RET },               // RET
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _Z },      // JP_Z
        { REGS(_RN, _RN), O_PREFIX, 4, C_NULL },            // PREF_CB
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _Z },   // CALL_Z
        { REGS(_RN, _RN), O_CALL, 17 | CNN, C_CALL },       // CALL_NN
        { REGS(_RN, _RN), O_ADC, 7 | CN, C_ADC, F_ADSB8 },  // ADC_A_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST8
        { REGS(_RN, _RN), O_RET, C_RET, _NC },              // RET_NC
        { REGS(_RDE, _RN), O_POP, 10, C_POP },              // POP_DE
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _NC },     // JP_NC
        { REGS(_RA, _RN), O_OUT, 11 | CN, C_OUT },          // OUT_PN_A
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _NC },  // CALL_NC
        { REGS(_RDE, _RN), O_PUSH, 11, C_PUSH },            // PUSH_DE
        { REGS(_RN, _RN), O_SUB, 7 | CN, C_SUB, F_ADSB8 },  // SUB_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST16
        { REGS(_RN, _RN), O_RET, 5, C_RET, _C },            // RET_C
        { REGS(_RN, _RN), O_SPEC, 4, C_EXX },               // EXX
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _C },      // JP_C
        { REGS(_RA, _RN), O_IN, 11 | CN, C_IN },            // IN_A_PN
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _C },   // CALL_C
        { REGS(_RN, _RN), O_PREFIX, 4, C_NULL },            // PREF_DD
        { REGS(_RN, _RN), O_SBC, 7 | CN, C_SBC, F_ADSB8 },  // SBC_A_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST24
        { REGS(_RN, _RN), O_RET, 5, C_RET, _PO },           // RET_PO
        { REGS(_RHL, _RN), O_POP, 10, C_POP },              // POP_HL
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _PO },     // JP_PO
        { REGS(_RHL, _RSP), O_SPEC, 19, C_EX_SP },          // EX_PSP_HL
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _PO },  // CALL_PO
        { REGS(_RHL, _RN), O_PUSH, 11, C_PUSH },            // PUSH_HL
        { REGS(_RN, _RN), O_AND, 7 | CN, C_AND, F_AND },    // AND_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST32
        { REGS(_RN, _RN), O_RET, 5, C_RET, _PE },           // RET_PE
        { REGS(_RHL, _RN), O_SPEC, 4, C_JP },               // JP_HL
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _PE },     // JP_PE
        { REGS(_RN, _RN), O_SPEC, 4, C_EX_DE },             // EX_DE_HL
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _PE },  // CALL_PE
        { REGS(_RN, _RN), O_PREFIX, 4, C_NULL },            // PREF_ED
        { REGS(_RN, _RN), O_XOR, 7 | CN, C_XOR, F_XOR },    // XOR_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST40
        { REGS(_RN, _RN), O_RET, 5, C_RET, _P },            // RET_P
        { REGS(_RAF, _RN), O_POP, 10, C_POP },              // POP_AF
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _P },      // JP_P
        { REGS(_RN, _RN), O_SPEC, 4, C_DI },                // DI
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _P },   // CALL_P
        { REGS(_RAF, _RN), O_PUSH, 11, C_PUSH },            // PUSH_AF
        { REGS(_RN, _RN), O_OR, 7 | CN, C_OR, F_OR },       // OR_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST48
        { REGS(_RN, _RN), O_RET, 5, C_RET, _M },            // RET_M
        { REGS(_RSP, _RHL), O_ASSIGN, 4, C_LD },            // LD_SP_HL
        { REGS(_RN, _RN), O_JMP, 10 | CNN, C_JP, _M },      // JP_M
        { REGS(_RN, _RN), O_SPEC, 4, C_EI },                // EI
        { REGS(_RN, _RN), O_CALL, 10 | CNN, C_CALL, _M },   // CALL_M
        { REGS(_RN, _RN), O_PREFIX, 4, C_NULL },            // PREF_FD
        { REGS(_RN, _RN), O_CP, 7 | CN, C_CP, F_CP },       // CP_N
        { REGS(_RN, _RN), O_RST, 11, C_RST },               // RST56
        // 203
        { REGS(_RB, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_B */ { REGS(_RC, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_C */
        { REGS(_RD, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_D */ { REGS(_RE, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_E */
        { REGS(_RH, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_H */ { REGS(_RL, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_RLC, F_ROTX }, /* RLC_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_RLC, F_ROTX }, /* RLC_A */
        { REGS(_RB, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_B */ { REGS(_RC, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_C */
        { REGS(_RD, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_D */ { REGS(_RE, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_E */
        { REGS(_RH, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_H */ { REGS(_RL, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_RRC, F_ROTX }, /* RRC_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_RRC, F_ROTX }, /* RRC_A */
        { REGS(_RB, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_B */ { REGS(_RC, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_C */
        { REGS(_RD, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_D */ { REGS(_RE, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_E */
        { REGS(_RH, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_H */ { REGS(_RL, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_RL, F_ROTX }, /* RL_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_RL, F_ROTX }, /* RL_A */
        { REGS(_RB, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_B */ { REGS(_RC, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_C */
        { REGS(_RD, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_D */ { REGS(_RE, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_E */
        { REGS(_RH, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_H */ { REGS(_RL, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_RR, F_ROTX }, /* RR_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_RR, F_ROTX }, /* RR_A */
        { REGS(_RB, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_B */ { REGS(_RC, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_C */
        { REGS(_RD, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_D */ { REGS(_RE, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_E */
        { REGS(_RH, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_H */ { REGS(_RL, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_SLA, F_ROTX }, /* SLA_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_SLA, F_ROTX }, /* SLA_A */
        { REGS(_RB, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_B */ { REGS(_RC, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_C */
        { REGS(_RD, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_D */ { REGS(_RE, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_E */
        { REGS(_RH, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_H */ { REGS(_RL, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_SRA, F_ROTX }, /* SRA_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_SRA, F_ROTX }, /* SRA_A */
        { REGS(_RB, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_B */ { REGS(_RC, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_C */
        { REGS(_RD, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_D */ { REGS(_RE, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_E */
        { REGS(_RH, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_H */ { REGS(_RL, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_SLI, F_ROTX }, /* SLI_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_SLI, F_ROTX }, /* SLI_A */
        { REGS(_RB, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_B */ { REGS(_RC, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_C */
        { REGS(_RD, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_D */ { REGS(_RE, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_E */
        { REGS(_RH, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_H */ { REGS(_RL, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_L */
        { REGS(_RPHL, _RN), O_ROT, 15, C_SRL, F_ROTX }, /* SRL_PHL */ { REGS(_RA, _RN), O_ROT, 8, C_SRL, F_ROTX }, /* SRL_A */

        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_0_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_1_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_2_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_3_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_4_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_D*/ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_5_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_6_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_A */
        { REGS(_RB, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_B */ { REGS(_RC, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_C */
        { REGS(_RD, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_D */ { REGS(_RE, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_E */
        { REGS(_RH, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_H */ { REGS(_RL, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_L */
        { REGS(_RPHL, _RN), O_BIT, 12, C_BIT, F_BIT }, /* BIT_7_PHL */ { REGS(_RA, _RN), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_A */

        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_0_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_0_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_0_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_0_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_0_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_0_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_0_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_0_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_1_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_1_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_1_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_1_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_1_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_1_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_1_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_1_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_2_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_2_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_2_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_2_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_2_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_2_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_2_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_2_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_3_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_3_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_3_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_3_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_3_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_3_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_3_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_3_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_4_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_4_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_4_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_4_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_4_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_4_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_4_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_4_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_5_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_5_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_5_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_5_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_5_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_5_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_5_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_5_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_6_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_6_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_6_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_6_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_6_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_6_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_6_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_6_A */
        { REGS(_RB, _RN), O_RES, 8, C_RES }, /* RES_7_B */ { REGS(_RC, _RN), O_RES, 8, C_RES }, /* RES_7_C */ { REGS(_RD, _RN), O_RES, 8, C_RES }, /* RES_7_D */
        { REGS(_RE, _RN), O_RES, 8, C_RES }, /* RES_7_E */ { REGS(_RH, _RN), O_RES, 8, C_RES }, /* RES_7_H */ { REGS(_RL, _RN), O_RES, 8, C_RES }, /* RES_7_L */
        { REGS(_RPHL, _RN), O_RES, 15, C_RES }, /* RES_7_PHL */ { REGS(_RA, _RN), O_RES, 8, C_RES }, /* RES_7_A */

        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_0_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_0_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_0_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_0_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_0_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_0_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_0_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_0_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_1_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_1_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_1_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_1_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_1_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_1_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_1_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_1_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_2_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_2_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_2_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_2_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_2_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_2_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_2_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_2_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_3_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_3_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_3_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_3_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_3_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_3_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_3_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_3_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_4_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_4_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_4_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_4_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_4_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_4_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_4_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_4_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_5_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_5_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_5_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_5_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_5_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_5_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_5_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_5_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_6_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_6_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_6_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_6_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_6_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_6_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_6_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_6_A */
        { REGS(_RB, _RN), O_SET, 8, C_SET }, /* SET_7_B */ { REGS(_RC, _RN), O_SET, 8, C_SET }, /* SET_7_C */ { REGS(_RD, _RN), O_SET, 8, C_SET }, /* SET_7_D */
        { REGS(_RE, _RN), O_SET, 8, C_SET }, /* SET_7_E */ { REGS(_RH, _RN), O_SET, 8, C_SET }, /* SET_7_H */ { REGS(_RL, _RN), O_SET, 8, C_SET }, /* SET_7_L */
        { REGS(_RPHL, _RN), O_SET, 15, C_SET }, /* SET_7_PHL */ { REGS(_RA, _RN), O_SET, 8, C_SET }, /* SET_7_A */

        // 237
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { REGS(_RB, _RC), O_IN, 12, C_IN, F_IN },           // IN_B_BC = 64
        { REGS(_RB, _RC), O_OUT, 12, C_OUT },               // OUT_PC_B
        { REGS(_RHL, _RBC), O_SBC, 15, C_SBC, F_ADSBC16 },  // SBC_HL_BC
        { REGS(_RN, _RBC), O_SAVE, 20 | CNN, C_LD },        // LD_PNN_BC
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG
        { REGS(_RN, _RN), O_RETN, C_RETN, 14 },             // RETN
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM0
        { REGS(_RI, _RA), O_ASSIGN, 9, C_LD },              // LD_I_A
        { REGS(_RC, _RC), O_IN, 12, C_IN, F_IN },           // IN_C_BC
        { REGS(_RC, _RC), O_OUT, 12, C_OUT },               // OUT_PC_C
        { REGS(_RHL, _RBC), O_ADC, 15, C_ADC, F_ADSBC16 },  // ADC_HL_BC
        { REGS(_RBC, _RN), O_LOAD, 20 | CNN, C_LD },        // LD_BC_PNN
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_1
        { REGS(_RN, _RN), O_RET, 14, C_RETI },              // RETI
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM0_1
        { REGS(_RR, _RA), O_ASSIGN, 9, C_LD },              // LD_R_A
        { REGS(_RD, _RC), O_IN, 12, C_IN, F_IN },           // IN_D_BC
        { REGS(_RD, _RC), O_OUT, 12, C_OUT },               // OUT_PC_D
        { REGS(_RHL, _RDE), O_SBC, 15, C_SBC, F_ADSBC16 },  // SBC_HL_DE
        { REGS(_RN, _RDE), O_SAVE, 20 | CNN, C_LD },        // LD_PNN_DE
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_2
        { REGS(_RN, _RN), O_RETN, 14, C_RETN },             // RETN_1
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM1
        { REGS(_RA, _RI), O_ASSIGN, 9, C_LD, F_IR },        // LD_A_I
        { REGS(_RE, _RC), O_IN, 12, C_IN, F_IN },           // IN_E_BC
        { REGS(_RE, _RC), O_OUT, 12, C_OUT },               // OUT_PC_E
        { REGS(_RHL, _RDE), O_ADC, 15, C_ADC, F_ADSBC16 },  // ADC_HL_DE
        { REGS(_RDE, _RN), O_LOAD, 20 | CNN, C_LD },        // LD_DE_PNN
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_3
        { REGS(_RN, _RN), O_RET, 14, C_RETI },              // RETI_1
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM2
        { REGS(_RA, _RR), O_ASSIGN, 9, C_LD, F_IR},         // LD_A_R
        { REGS(_RH, _RC), O_IN, 12, C_IN, F_IN },           // IN_H_BC
        { REGS(_RH, _RC), O_OUT, 12, C_OUT },               // OUT_PC_H
        { REGS(_RHL, _RHL), O_SBC, 15, C_SBC, F_ADSBC16 },  // SBC_HL_HL
        { REGS(_RN, _RHL), O_SAVE, 20 | CNN, C_LD },        // LD_PNN_HL1
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_4
        { REGS(_RN, _RN), O_RETN, 14, C_RETN },             // RETN_2
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM0_2
        { REGS(_RHL, _RA), O_SPEC, 18, C_RRD, F_RLRD },     // RRD
        { REGS(_RL, _RC), O_IN, 12, C_IN, F_IN },           // IN_L_BC
        { REGS(_RL, _RC), O_OUT, 12, C_OUT },               // OUT_PC_L
        { REGS(_RHL, _RHL), O_ADC, 15, C_ADC, F_ADSBC16 },  // ADC_HL_HL
        { REGS(_RHL, _RN), O_LOAD, 20 | CNN, C_LD },        // LD_HL1_PNN
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_5
        { REGS(_RN, _RN), O_RET, 14, C_RETI },              // RETI_2
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM0_3
        { REGS(_RHL, _RA), O_SPEC, 18, C_RLD, F_RLRD },     // RLD
        { REGS(_RN, _RC), O_SPEC, 12, C_IN, F_F_PC },       // IN_F_PC
        { REGS(_RN, _RC), O_SPEC, 12, C_OUT },              // OUT_PC_0
        { REGS(_RHL, _RSP), O_SBC, 15, C_SBC, F_ADSBC16 },  // SBC_HL_SP
        { REGS(_RN, _RSP), O_SAVE, 20 | CNN, C_LD },        // LD_PNN_SP
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_6
        { REGS(_RN, _RN), O_RETN, 14, C_RETN },             // RETN_3
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM1_1
        { REGS(_RHL, _RA), O_SPEC, 8, C_NOP },              // NOP_1
        { REGS(_RA, _RC), O_IN, 12, C_IN, F_IN },           // IN_A_BC
        { REGS(_RA, _RC), O_OUT, 12, C_OUT },               // OUT_PC_A
        { REGS(_RHL, _RSP), O_ADC, 15, C_ADC, F_ADSBC16 },  // ADC_HL_SP
        { REGS(_RSP, _RN), O_LOAD, 20 | CNN, C_LD },        // LD_SP_PNN
        { REGS(_RN, _RN), O_NEG, 4, C_NEG, F_NEG },         // NEG_7
        { REGS(_RN, _RN), O_RET, 14, C_RETI },              // RETI_3
        { REGS(_RN, _RN), O_IM, 8, C_IM },                  // IM2_1
        { REGS(_RHL, _RA), O_SPEC, 8, C_NOP },              // NOP_2 = 127
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { REGS(_RN, _RN), O_REP, 16, C_LDI, F_LDI },  // LDI
        { REGS(_RN, _RN), O_REP, 16, C_CPI, F_CPI },  // CPI
        { REGS(_RN, _RN), O_REP, 16, C_INI, F_INI },  // INI
        { REGS(_RN, _RN), O_REP, 16, C_OTI, F_OTI },  // OTI
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { REGS(_RN, _RN), O_REP, 16, C_LDD, F_LDI },  // LDD
        { REGS(_RN, _RN), O_REP, 16, C_CPD, F_CPI },  // CPD
        { REGS(_RN, _RN), O_REP, 16, C_IND, F_INI },  // IND
        { REGS(_RN, _RN), O_REP, 16, C_OTD, F_OTI },  // OTD
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { REGS(_RN, _RN), O_REP, 16, C_LDIR, F_LDI},  // LDIR
        { REGS(_RN, _RN), O_REP, 16, C_CPIR, F_CPI }, // CPIR
        { REGS(_RN, _RN), O_REP, 16, C_INIR, F_INI }, // INIR
        { REGS(_RN, _RN), O_REP, 16, C_OTIR, F_OTI }, // OTIR
        STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        { REGS(_RN, _RN), O_REP, 16, C_LDDR, F_LDI }, // LDDR
        { REGS(_RN, _RN), O_REP, 16, C_CPDR, F_CPI }, // CPDR
        { REGS(_RN, _RN), O_REP, 16, C_INDR, F_INI }, // INDR
        { REGS(_RN, _RN), O_REP, 16, C_OTDR, F_OTI }, // OTDR
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI,
        STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI, STK_NONI
};
