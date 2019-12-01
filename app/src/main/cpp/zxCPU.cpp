//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxCPU.h"
#include "zxMnemonic.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "missing_default_case"

uint8_t tbl_parity[256];
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

#define REGS(dst, src)  (dst | src << 4)

#define F_INC_DEC   FS | FZ | FH | FPV | FN
#define F_CPX       FS | FZ | FH | FPV | FN
#define F_IN_PORT   FS | FZ | FH | FPV | FN
#define F_RLD       FS | FZ | FH | FPV | FN
#define F_ASSIGN_IR FS | FZ | FH | FPV | FN
#define F_ARIPTH    FS | FZ | FH | FPV | FN | FC
#define F_BIT       FS | FZ | FH | FN
#define STK_NONI    { REGS(_RNONE, _RNONE), O_NONI, 8, C_ED_NONI }

static zxCPU::MNEMONIC mnemonics[] = {
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_NOP },                         // NOP
        { REGS(_RBC, _RNONE), O_ASSIGN, 10 | C_16BIT, C_LD },               // LD_BC_NN
        { REGS(_RBC, _RA), O_SAVE, 7, C_LD },                               // LD_PBC_A
        { REGS(_RBC, _RNONE), O_INC, 6, C_INC },                            // INC_BC
        { REGS(_RB, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_B
        { REGS(_RB, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_B
        { REGS(_RB, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_B_N
        { REGS(_RA, _RNONE), O_ROT, 4, C_RLCA, FH | FN | FC },              // RLCA
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_EX_AF },                       // EX_AF_AF'
        { REGS(_RHL, _RBC), O_ADD, 11, C_ADD, FH | FN | FC },               // ADD_HL_BC
        { REGS(_RA, _RBC), O_LOAD, 7, C_LD },                               // LD_A_PBC
        { REGS(_RBC, _RNONE), O_DEC, 6, C_DEC },                            // DEC_BC
        { REGS(_RC, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_C
        { REGS(_RC, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_C
        { REGS(_RC, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_C_N
        { REGS(_RA, _RNONE), O_ROT, 4, C_RRCA, FH | FN | FC },              // RRCA
        { REGS(_RNONE, _RNONE), O_SPEC, 13 | C_8BIT, C_DJNZ },              // DJNZ
        { REGS(_RDE, _RNONE), O_ASSIGN, 10 | C_16BIT, C_LD },               // LD_DE_NN
        { REGS(_RDE, _RA), O_SAVE, 7, C_LD },                               // LD_PDE_A
        { REGS(_RDE, _RNONE), O_INC, 6, C_INC },                            // INC_DE
        { REGS(_RD, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_D
        { REGS(_RD, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_D
        { REGS(_RD, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_D_N
        { REGS(_RA, _RNONE), O_ROT, 4, C_RLA, FH | FN | FC },               // RLA
        { REGS(_RNONE, _RNONE), O_JR | O_JMP, 7 | C_8BIT, C_JR },           // JR_N
        { REGS(_RHL, _RDE), O_ADD, 11, C_ADD, FH | FN | FC },               // ADD_HL_DE
        { REGS(_RA, _RDE), O_LOAD, 7, C_LD },                               // LD_A_PDE
        { REGS(_RDE, _RNONE), O_DEC, 6, C_DEC },                            // DEC_DE
        { REGS(_RE, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_E
        { REGS(_RE, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_E
        { REGS(_RE, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_E_N
        { REGS(_RA, _RNONE), O_ROT, 4, C_RRA, FH | FN | FC },               // RRA
        { REGS(_RNONE, _RNONE), O_JR | O_JMP, 7 | C_8BIT, C_JR, _NZ },      // JR_NZ
        { REGS(_RHL, _RNONE), O_ASSIGN, 10 | C_16BIT, C_LD },               // LD_HL_NN
        { REGS(_RNONE, _RHL), O_SAVE, 16 | C_16BIT, C_LD },                 // LD_PNN_HL
        { REGS(_RHL, _RNONE), O_INC, 6, C_INC },                            // INC_HL
        { REGS(_RH, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_H
        { REGS(_RH, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_H
        { REGS(_RH, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_H_N
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_DAA, FS | FZ | FH | FPV | FC },// DAA
        { REGS(_RNONE, _RNONE), O_JR | O_JMP, 7 | C_8BIT, C_JR, _Z },       // JR_Z
        { REGS(_RHL, _RHL), O_ADD, 11, C_ADD, FH | FN | FC },               // ADD_HL_HL
        { REGS(_RHL, _RNONE), O_LOAD, 16 | C_16BIT, C_LD },                 // LD_HL_PNN
        { REGS(_RHL, _RNONE), O_DEC, 6, C_DEC },                            // DEC_HL
        { REGS(_RL, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_L
        { REGS(_RL, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_L
        { REGS(_RL, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_L_N
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_CPL, FH | FN },                // CPL
        { REGS(_RNONE, _RNONE), O_JR | O_JMP, 12 | C_8BIT, C_JR, _NC },     // JR_NC
        { REGS(_RSP, _RNONE), O_ASSIGN, 10 | C_16BIT, C_LD },               // LD_SP_NN
        { REGS(_RNONE, _RA), O_SAVE, 13 | C_16BIT, C_LD },                  // LD_PNN_A
        { REGS(_RSP, _RNONE), O_INC, 6, C_INC },                            // INC_SP
        { REGS(_RPHL, _RNONE), O_INC, 11, C_INC, F_INC_DEC },               // INC_PHL
        { REGS(_RPHL, _RNONE), O_DEC, 11, C_DEC, F_INC_DEC },               // DEC_PHL
        { REGS(_RPHL, _RNONE), O_SAVE, 10 | C_8BIT, C_LD },                 // LD_PHL_N
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_SCF, FH | FN | FC },           // SCF
        { REGS(_RNONE, _RNONE), O_JR | O_JMP, 7 | C_8BIT, C_JR, _C },       // JR_C
        { REGS(_RHL, _RSP), O_ADD, 11, C_ADD, FH | FN | FC },               // ADD_HL_SP
        { REGS(_RA, _RNONE), O_LOAD, 13 | C_16BIT, C_LD },                  // LD_A_PNN
        { REGS(_RSP, _RNONE), O_DEC, 6, C_DEC },                            // DEC_SP
        { REGS(_RA, _RNONE), O_INC, 4, C_INC, F_INC_DEC },                  // INC_A
        { REGS(_RA, _RNONE), O_DEC, 4, C_DEC, F_INC_DEC },                  // DEC_A
        { REGS(_RA, _RNONE), O_ASSIGN, 7 | C_8BIT, C_LD },                  // LD_A_N
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_CCF, FH | FN | FC },           // CCF
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
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_HALT }, /* HALT */ { REGS(_RPHL, _RA), O_SAVE, 7, C_LD }, /* LD_PHL_A */
        { REGS(_RA, _RB), O_ASSIGN, 4, C_LD }, /* LD_A_B */ { REGS(_RA, _RC), O_ASSIGN, 4, C_LD }, /* LD_A_C */ { REGS(_RA, _RD), O_ASSIGN, 4, C_LD }, /* LD_A_D */
        { REGS(_RA, _RE), O_ASSIGN, 4, C_LD }, /* LD_A_E */ { REGS(_RA, _RH), O_ASSIGN, 4, C_LD }, /* LD_A_H */ { REGS(_RA, _RL), O_ASSIGN, 4, C_LD }, /* LD_A_L */
        { REGS(_RA, _RPHL), O_LOAD, 7, C_LD }, /* LD_A_PHL */ { REGS(_RA, _RA), O_ASSIGN, 4, C_LD }, /* LD_A_A */ 

        { REGS(_RA, _RB), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_B */ { REGS(_RA, _RC), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_C */
        { REGS(_RA, _RD), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_D */ { REGS(_RA, _RE), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_E */
        { REGS(_RA, _RH), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_H */ { REGS(_RA, _RL), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_L */
        { REGS(_RA, _RPHL), O_ADD, 7, C_ADD, F_ARIPTH }, /* ADD_A_PHL */ { REGS(_RA, _RA), O_ADD, 4, C_ADD, F_ARIPTH }, /* ADD_A_A */

        { REGS(_RA, _RB), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_B */ { REGS(_RA, _RC), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_C */
        { REGS(_RA, _RD), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_D */ { REGS(_RA, _RE), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_E */
        { REGS(_RA, _RH), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_H */ { REGS(_RA, _RL), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_A */
        { REGS(_RA, _RPHL), O_ADC, 7, C_ADC, F_ARIPTH }, /* ADC_A_PHL */ { REGS(_RA, _RA), O_ADC, 4, C_ADC, F_ARIPTH }, /* ADC_A_A */
        
        { REGS(_RA, _RB), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_B */ { REGS(_RA, _RC), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_C */
        { REGS(_RA, _RD), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_D */ { REGS(_RA, _RE), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_E */
        { REGS(_RA, _RH), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_H */ { REGS(_RA, _RL), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_L */
        { REGS(_RA, _RPHL), O_SUB, 7, C_SUB, F_ARIPTH }, /* SUB_A_PHL */ { REGS(_RA, _RA), O_SUB, 4, C_SUB, F_ARIPTH }, /* SUB_A_A */
        
        { REGS(_RA, _RB), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_B */ { REGS(_RA, _RC), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_C */
        { REGS(_RA, _RD), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_D */ { REGS(_RA, _RE), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_E */
        { REGS(_RA, _RH), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_H */ { REGS(_RA, _RL), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_L */
        { REGS(_RA, _RPHL), O_SBC, 7, C_SBC, F_ARIPTH }, /* SBC_A_PHL */ { REGS(_RA, _RA), O_SBC, 4, C_SBC, F_ARIPTH }, /* SBC_A_A */
        
        { REGS(_RA, _RB), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_B */ { REGS(_RA, _RC), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_C */
        { REGS(_RA, _RD), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_D */ { REGS(_RA, _RE), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_E */
        { REGS(_RA, _RH), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_H */ { REGS(_RA, _RL), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_ */
        { REGS(_RA, _RPHL), O_AND, 7, C_AND, F_ARIPTH }, /* AND_A_PHL */ { REGS(_RA, _RA), O_AND, 4, C_AND, F_ARIPTH }, /* AND_A_A */
        
        { REGS(_RA, _RB), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_B */ { REGS(_RA, _RC), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_C */
        { REGS(_RA, _RD), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_D */ { REGS(_RA, _RE), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_E */
        { REGS(_RA, _RH), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_H */ { REGS(_RA, _RL), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_L */
        { REGS(_RA, _RPHL), O_XOR, 7, C_XOR, F_ARIPTH }, /* XOR_A_PHL */ { REGS(_RA, _RA), O_XOR, 4, C_XOR, F_ARIPTH }, /* XOR_A_A */
        
        { REGS(_RA, _RB), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_B */ { REGS(_RA, _RC), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_C */
        { REGS(_RA, _RD), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_D */ { REGS(_RA, _RE), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_E */
        { REGS(_RA, _RH), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_H */ { REGS(_RA, _RL), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_L */
        { REGS(_RA, _RPHL), O_OR, 7, C_OR, F_ARIPTH }, /* OR_A_PHL */ { REGS(_RA, _RA), O_OR, 4, C_OR, F_ARIPTH }, /* OR_A_A */
        
        { REGS(_RA, _RB), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_B */ { REGS(_RA, _RC), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_C */
        { REGS(_RA, _RD), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_D */ { REGS(_RA, _RE), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_E */
        { REGS(_RA, _RH), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_H */ { REGS(_RA, _RL), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_L */
        { REGS(_RA, _RPHL), O_CP, 7, C_CP, F_ARIPTH }, /* CP_A_PHL */ { REGS(_RA, _RA), O_CP, 4, C_CP, F_ARIPTH }, /* CP_A_A */
        
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _NZ },             // RET_NZ
        { REGS(_RBC, _RNONE), O_POP, 10, C_POP },                           // POP_BC
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _NZ },           // JP_NZ
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP },                // JP_NN
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _NZ },// CALL_NZ
        { REGS(_RBC, _RNONE), O_PUSH, 11, C_PUSH },                         // PUSH_BC
        { REGS(_RA, _RNONE), O_ADD, 7 | C_8BIT, C_ADD, F_ARIPTH },          // ADD_A_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST0
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _Z },              // RET_Z
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 10, C_RET },                 // RET
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _Z },            // JP_Z
        { REGS(_RNONE, _RNONE), O_PREFIX, 4, C_NULL },                      // PREF_CB
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _Z }, // CALL_Z
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 17 | C_16BIT, C_CALL },     // CALL_NN
        { REGS(_RA, _RNONE), O_ADC, 7 | C_8BIT, C_ADC, F_ARIPTH },          // ADC_A_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST8
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, C_RET, _NC },                // RET_NC
        { REGS(_RDE, _RNONE), O_POP, 10, C_POP },                           // POP_DE
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _NC },           // JP_NC
        { REGS(_RA, _RNONE), O_OUT, 11 | C_8BIT, C_OUT },                   // OUT_PN_A
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _NC },// CALL_NC
        { REGS(_RDE, _RNONE), O_PUSH, 11, C_PUSH },                         // PUSH_DE
        { REGS(_RA, _RNONE), O_SUB, 7 | C_8BIT, C_SUB, F_ARIPTH },          // SUB_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST16
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _C },              // RET_C
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_EXX },                         // EXX
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _C },            // JP_C
        { REGS(_RA, _RNONE), O_IN, 11 | C_8BIT, C_IN },                     // IN_A_PN
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _C }, // CALL_C
        { REGS(_RNONE, _RNONE), O_PREFIX, 4, C_NULL },                      // PREF_DD
        { REGS(_RA, _RNONE), O_SBC, 7 | C_8BIT, C_SBC, F_ARIPTH },          // SBC_A_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST24
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _PO },             // RET_PO
        { REGS(_RHL, _RNONE), O_POP, 10, C_POP },                           // POP_HL
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _PO },           // JP_PO
        { REGS(_RHL, _RSP), O_SPEC, 19, C_EX_SP },                          // EX_PSP_HL
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _PO },// CALL_PO
        { REGS(_RHL, _RNONE), O_PUSH, 11, C_PUSH },                         // PUSH_HL
        { REGS(_RA, _RNONE), O_AND, 7 | C_8BIT, C_AND, F_ARIPTH },          // AND_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST32
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _PE },             // RET_PE
        { REGS(_RHL, _RNONE), O_SPEC, 4, C_JP },                            // JP_HL
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _PE },           // JP_PE
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_EX_DE },                       // EX_DE_HL
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _PE },// CALL_PE
        { REGS(_RNONE, _RNONE), O_PREFIX, 4, C_NULL },                      // PREF_ED
        { REGS(_RA, _RNONE), O_XOR, 7 | C_8BIT, C_XOR, F_ARIPTH },          // XOR_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST40
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _P },              // RET_P
        { REGS(_RAF, _RNONE), O_POP, 10, C_POP },                           // POP_AF
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _P },            // JP_P
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_DI },                          // DI
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _P }, // CALL_P
        { REGS(_RAF, _RNONE), O_PUSH, 11, C_PUSH },                         // PUSH_AF
        { REGS(_RA, _RNONE), O_OR, 7 | C_8BIT, C_OR, F_ARIPTH },            // OR_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST48
        { REGS(_RNONE, _RNONE), O_RET | O_JMP, 5, C_RET, _M },              // RET_M
        { REGS(_RSP, _RHL), O_ASSIGN, 4, C_LD },                            // LD_SP_HL
        { REGS(_RNONE, _RNONE), O_JMP, 10 | C_16BIT, C_JP, _M },            // JP_M
        { REGS(_RNONE, _RNONE), O_SPEC, 4, C_EI },                          // EI
        { REGS(_RNONE, _RNONE), O_CALL | O_JMP, 10 | C_16BIT, C_CALL, _M }, // CALL_M
        { REGS(_RNONE, _RNONE), O_PREFIX, 4, C_NULL },                      // PREF_FD
        { REGS(_RA, _RNONE), O_CP, 7 | C_8BIT, C_CP, F_ARIPTH },            // CP_N
        { REGS(_RNONE, _RNONE), O_RST, 11, C_RST },                         // RST56
        // 203
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_RLC, F_ARIPTH }, /* RLC_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_RLC, F_ARIPTH }, /* RLC_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_RRC, F_ARIPTH }, /* RRC_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_RRC, F_ARIPTH }, /* RRC_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_RL, F_ARIPTH }, /* RL_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_RL, F_ARIPTH }, /* RL_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_RR, F_ARIPTH }, /* RR_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_RR, F_ARIPTH }, /* RR_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_SLA, F_ARIPTH }, /* SLA_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_SLA, F_ARIPTH }, /* SLA_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_SRA, F_ARIPTH }, /* SRA_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_SRA, F_ARIPTH }, /* SRA_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_SLI, F_ARIPTH }, /* SLI_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_SLI, F_ARIPTH }, /* SLI_A */
        { REGS(_RB, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_B */ { REGS(_RC, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_C */
        { REGS(_RD, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_D */ { REGS(_RE, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_E */
        { REGS(_RH, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_H */ { REGS(_RL, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_L */
        { REGS(_RPHL, _RNONE), O_ROT_CB, 15, C_SRL, F_ARIPTH }, /* SRL_PHL */ { REGS(_RA, _RNONE), O_ROT_CB, 8, C_SRL, F_ARIPTH }, /* SRL_A */
 
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_0_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_0_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_1_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_1_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_2_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_2_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_3_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_3_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_4_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_4_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_D*/ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_5_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_5_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_6_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_6_A */
        { REGS(_RB, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_B */ { REGS(_RC, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_C */
        { REGS(_RD, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_D */ { REGS(_RE, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_E */
        { REGS(_RH, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_H */ { REGS(_RL, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_L */
        { REGS(_RPHL, _RNONE), O_BIT, 12, C_BIT, F_BIT }, /* BIT_7_PHL */ { REGS(_RA, _RNONE), O_BIT, 8, C_BIT, F_BIT }, /* BIT_7_A */

        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_0_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_0_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_0_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_0_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_0_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_0_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_0_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_0_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_1_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_1_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_1_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_1_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_1_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_1_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_1_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_1_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_2_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_2_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_2_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_2_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_2_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_2_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_2_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_2_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_3_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_3_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_3_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_3_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_3_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_3_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_3_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_3_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_4_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_4_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_4_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_4_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_4_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_4_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_4_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_4_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_5_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_5_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_5_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_5_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_5_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_5_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_5_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_5_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_6_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_6_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_6_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_6_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_6_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_6_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_6_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_6_A */
        { REGS(_RB, _RNONE), O_RES, 8, C_RES }, /* RES_7_B */ { REGS(_RC, _RNONE), O_RES, 8, C_RES }, /* RES_7_C */ { REGS(_RD, _RNONE), O_RES, 8, C_RES }, /* RES_7_D */
        { REGS(_RE, _RNONE), O_RES, 8, C_RES }, /* RES_7_E */ { REGS(_RH, _RNONE), O_RES, 8, C_RES }, /* RES_7_H */ { REGS(_RL, _RNONE), O_RES, 8, C_RES }, /* RES_7_L */
        { REGS(_RPHL, _RNONE), O_RES, 15, C_RES }, /* RES_7_PHL */ { REGS(_RA, _RNONE), O_RES, 8, C_RES }, /* RES_7_A */

        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_0_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_0_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_0_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_0_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_0_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_0_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_0_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_0_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_1_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_1_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_1_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_1_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_1_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_1_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_1_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_1_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_2_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_2_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_2_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_2_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_2_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_2_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_2_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_2_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_3_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_3_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_3_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_3_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_3_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_3_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_3_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_3_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_4_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_4_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_4_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_4_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_4_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_4_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_4_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_4_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_5_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_5_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_5_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_5_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_5_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_5_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_5_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_5_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_6_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_6_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_6_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_6_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_6_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_6_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_6_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_6_A */
        { REGS(_RB, _RNONE), O_SET, 8, C_SET }, /* SET_7_B */ { REGS(_RC, _RNONE), O_SET, 8, C_SET }, /* SET_7_C */ { REGS(_RD, _RNONE), O_SET, 8, C_SET }, /* SET_7_D */
        { REGS(_RE, _RNONE), O_SET, 8, C_SET }, /* SET_7_E */ { REGS(_RH, _RNONE), O_SET, 8, C_SET }, /* SET_7_H */ { REGS(_RL, _RNONE), O_SET, 8, C_SET }, /* SET_7_L */
        { REGS(_RPHL, _RNONE), O_SET, 15, C_SET }, /* SET_7_PHL */ { REGS(_RA, _RNONE), O_SET, 8, C_SET }, /* SET_7_A */

        // 237
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        { REGS(_RB, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_B_BC = 64
        { REGS(_RB, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_B
        { REGS(_RHL, _RBC), O_SBC, 15, C_SBC, F_ARIPTH },     // SBC_HL_BC
        { REGS(_RNONE, _RBC), O_SAVE, 20 | C_16BIT, C_LD },   // LD_PNN_BC
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG
        { REGS(_RNONE, _RNONE), O_RETN, C_RETN, 14 },         // RETN
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM0
        { REGS(_RI, _RA), O_ASSIGN, 9, C_LD },                // LD_I_A
        { REGS(_RC, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_C_BC
        { REGS(_RC, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_C
        { REGS(_RHL, _RBC), O_ADC, 15, C_ADC, F_ARIPTH },     // ADC_HL_BC
        { REGS(_RBC, _RNONE), O_LOAD, 20 | C_16BIT, C_LD },   // LD_BC_PNN
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_1
        { REGS(_RNONE, _RNONE), O_RETI, 14, C_RETI },         // RETI
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM0_1
        { REGS(_RR, _RA), O_ASSIGN, 9, C_LD },                // LD_R_A
        { REGS(_RD, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_D_BC
        { REGS(_RD, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_D
        { REGS(_RHL, _RDE), O_SBC, 15, C_SBC, F_ARIPTH },     // SBC_HL_DE
        { REGS(_RNONE, _RDE), O_SAVE, 20 | C_16BIT, C_LD },   // LD_PNN_DE
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_2
        { REGS(_RNONE, _RNONE), O_RETN, 14, C_RETN },         // RETN_1
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM1
        { REGS(_RA, _RI), O_ASSIGN, 9, C_LD, F_ASSIGN_IR },   // LD_A_I
        { REGS(_RE, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_E_BC
        { REGS(_RE, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_E
        { REGS(_RHL, _RDE), O_ADC, 15, C_ADC, F_ARIPTH },     // ADC_HL_DE
        { REGS(_RDE, _RNONE), O_LOAD, 20 | C_16BIT, C_LD },   // LD_DE_PNN
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_3
        { REGS(_RNONE, _RNONE), O_RETI, 14, C_RETI },         // RETI_1
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM2
        { REGS(_RA, _RR), O_ASSIGN, 9, C_LD, F_ASSIGN_IR },   // LD_A_R
        { REGS(_RH, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_H_BC
        { REGS(_RH, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_H
        { REGS(_RHL, _RHL), O_SBC, 15, C_SBC, F_ARIPTH },     // SBC_HL_HL
        { REGS(_RNONE, _RHL), O_SAVE, 20 | C_16BIT, C_LD },   // LD_PNN_HL1
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_4
        { REGS(_RNONE, _RNONE), O_RETN, 14, C_RETN },         // RETN_2
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM0_2
        { REGS(_RHL, _RA), O_RRD, 18, C_RRD, F_RLD },         // RRD
        { REGS(_RL, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_L_BC
        { REGS(_RL, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_L
        { REGS(_RHL, _RHL), O_ADC, 15, C_ADC, F_ARIPTH },     // ADC_HL_HL
        { REGS(_RHL, _RNONE), O_LOAD, 20 | C_16BIT, C_LD },   // LD_HL1_PNN
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_5
        { REGS(_RNONE, _RNONE), O_RETI, 14, C_RETI },         // RETI_2
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM0_3
        { REGS(_RHL, _RA), O_RLD, 18, C_RLD, F_RLD },         // RLD
        { REGS(_RNONE, _RC), O_SPEC, 12, C_IN, F_IN_PORT },   // IN_F_PC
        { REGS(_RNONE, _RC), O_SPEC, 12, C_OUT },             // OUT_PC_0
        { REGS(_RHL, _RSP), O_SBC, 15, C_SBC, F_ARIPTH },     // SBC_HL_SP
        { REGS(_RNONE, _RSP), O_SAVE, 20 | C_16BIT, C_LD },   // LD_PNN_SP
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_6
        { REGS(_RNONE, _RNONE), O_RETN, 14, C_RETN },         // RETN_3
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM1_1
        { REGS(_RHL, _RA), O_RRD, 18, C_RRD, F_RLD },         // RRD_1
        { REGS(_RA, _RC), O_IN, 12, C_IN, F_IN_PORT },        // IN_A_BC
        { REGS(_RA, _RC), O_OUT, 12, C_OUT },                 // OUT_PC_A
        { REGS(_RHL, _RSP), O_ADC, 15, C_ADC, F_ARIPTH },     // ADC_HL_SP
        { REGS(_RSP, _RNONE), O_LOAD, 20 | C_16BIT, C_LD },   // LD_SP_PNN
        { REGS(_RNONE, _RNONE), O_NEG, 4, C_NEG, F_ARIPTH },  // NEG_7
        { REGS(_RNONE, _RNONE), O_RETI, 14, C_RETI },         // RETI_3
        { REGS(_RNONE, _RNONE), O_IM, 8, C_IM },              // IM2_1
        { REGS(_RHL, _RA), O_RLD, 18, C_RLD, F_RLD },         // RLD_1 = 127
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */
        { REGS(_RNONE, _RNONE), O_REP, 16, C_LDI, FH | FPV | FN },             // LDI
        { REGS(_RNONE, _RNONE), O_REP, 16, C_CPI, F_CPX },                     // CPI
        { REGS(_RNONE, _RNONE), O_REP, 16, C_INI, F_ARIPTH },                  // INI
        { REGS(_RNONE, _RNONE), O_REP, 16, C_OTI, F_ARIPTH },                  // OTI
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        { REGS(_RNONE, _RNONE), O_REP, 16, C_LDD, F3 | FH | FPV | FN },        // LDD
        { REGS(_RNONE, _RNONE), O_REP, 16, C_CPD, F3 | F_CPX },                // CPD
        { REGS(_RNONE, _RNONE), O_REP, 16, C_IND, F3 | F_ARIPTH },             // IND
        { REGS(_RNONE, _RNONE), O_REP, 16, C_OTD, F3 | F_ARIPTH },             // OTD
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        { REGS(_RNONE, _RNONE), O_REP, 16, C_LDIR, F5 | FH | FPV | FN },        // LDIR
        { REGS(_RNONE, _RNONE), O_REP, 16, C_CPIR, F5 | F_CPX },                // CPIR
        { REGS(_RNONE, _RNONE), O_REP, 16, C_INIR, F5 | F_ARIPTH },             // INIR
        { REGS(_RNONE, _RNONE), O_REP, 16, C_OTIR, F5 | F_ARIPTH },             // OTIR
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        { REGS(_RNONE, _RNONE), O_REP, 16, C_LDDR, F5 | F3 | FH | FPV | FN },   // LDDR
        { REGS(_RNONE, _RNONE), O_REP, 16, C_CPDR, F5 | F3 | F_CPX },           // CPDR
        { REGS(_RNONE, _RNONE), O_REP, 16, C_INDR, F5 | F3 | F_ARIPTH },        // INDR
        { REGS(_RNONE, _RNONE), O_REP, 16, C_OTDR, F5 | F3 | F_ARIPTH },        // OTDR
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */ STK_NONI, /* NONI */
        STK_NONI, /* NONI */ STK_NONI, /* NONI */
};

static void makeTblParity() {
    for(int a = 0; a < 256; a++) {
        uint8_t val = (uint8_t)a;
        val = (uint8_t)(((a >> 1) & 0x55) + (val & 0x55));
        val = (uint8_t)(((val >> 2) & 0x33) + (val & 0x33));
        val = (uint8_t)(((val >> 4) & 0x0F) + (val & 0x0F));
        tbl_parity[a] = (uint8_t)(!(val & 1) << 2);
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
        if(codeExOps == PREF_ED && (codeOps == LD_A_R || codeOps == LD_A_I)) uflags(FPV, 0);
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
    if(o != _RNONE) {
        if (o == _RPHL) {
            auto offs = prefix ? rm8PC() : (uint8_t)0;
            // виртуальный адрес [HL/IX/IY + D]
            v16 = *rRP[_RHL - _RI + prefix] + (int8_t)offs;
            // 8 бмт значение по виртуальному адресу
            v8 = rm8(v16);
            *ticks += 8;
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

    if(*zxALU::_STATE & ZX_HALT) return 4;

    incrementR();

    auto pc = *_PC;
    if(!offset) codeExOps = 0;
    else if(offset == 256 && prefix) {
        initOperand(_RPHL, _RNONE, prefix, vSrc, v8Src, &ticks);
        prefix = 0;
    }

    codeOps = rm8PC();

    auto m = &mnemonics[codeOps + offset];
    auto ops = m->ops;
    auto nDst = (uint8_t)(m->regs & 15);
    auto nSrc = (uint8_t)(m->regs >> 4);

    ticks += m->tiks & 31;

    initOperand(nSrc, nDst, prefix, vSrc, v8Src, &ticks);
    auto dst = initOperand(nDst, nSrc, prefix, vDst, v8Dst, &ticks);

    if(m->tiks & C_8BIT) v8Src = rm8PC();
    else if(m->tiks & C_16BIT) {
        auto v = rm16PC();
        if(nDst == _RNONE) vDst = v; else vSrc = v;
    }

    uint8_t fl = m->flgs, nfl(0);

    if(ops != O_PREFIX && opts[ZX_PROP_SHOW_FPS])
        info("PC: %i %s", pc, daMakeMnemonic(m, prefix, codeOps, *_PC, vDst, vSrc, v8Dst, v8Src).c_str());

    if(ops & O_JMP) {
        if(fl && !isFlag((uint8_t)(fl - 1))) return ticks;
    } else if(ops >= O_SET) {
        bit = (uint8_t)(1 << ((codeOps >> 3) & 7));
    }
    switch(ops) {
        case O_ASSIGN:
            // LD DST, SRC/N[N]
            if(nDst >= _RBC) *(uint16_t*)dst = vSrc;
            else {
                *dst = v8Src;
                if(fl) nfl = _FS(v8Src) | _FZ(v8Src) | ((*_IFF2) << 2);
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
                n32 = vDst + vSrc;
                *(uint16_t *) dst = (uint16_t) n32;
                nfl = _FH((uint8_t)(vDst >> 8), (uint8_t)(vSrc >> 8), 0, 0) | (n32 > 65535);
            } else {
                auto sub = (uint8_t) (ops - O_ADD);
                n8 = (uint8_t) (n16 = sub ? (v8Dst - v8Src) : (v8Dst + v8Src));
                *dst = (uint8_t) n16;
                nfl = _FS(n8) | _FZ(n8) | _FH(v8Dst, v8Src, 0, sub) | sub | _FV1(v8Dst, n8) | (uint8_t) (n16 > 255);
            }
            break;
        case O_ADC:
        case O_SBC: {
                auto sbc = (uint8_t) (ops - O_ADC);
                auto fc = getFlag(FC);
                if (nDst >= _RBC) {
                    n32 = sbc ? vDst - (vSrc + fc) : vDst + (vSrc + fc);
                    *(uint16_t*)dst = n16 = (uint16_t) n32;
                    nfl = (uint8_t) (_FS(n16 >> 8) | _FZ(n16) | _FH((uint8_t) (vDst >> 8), (uint8_t) (vSrc >> 8), fc, sbc) |
                            _FV1(vDst, n16) | sbc | (n32 > 65535));
                } else {
                    *dst = n8 = (uint8_t) (n16 = sbc ? (v8Dst - (v8Src + fc)) : (v8Dst + (v8Src + fc)));
                    nfl = _FS(n8) | _FZ(n8) | _FH(v8Dst, v8Src, fc, sbc) | _FV1(v8Dst, n8) | sbc | (uint8_t) (n16 > 255);
                }
            }
            break;
        case O_INC:
        case O_DEC:
            n16 = (uint16_t)((ops - O_INC) + 1);
            if(nDst >= _RBC) *(uint16_t*)dst = (uint16_t)(vDst + n16);
            else {
                v8Src = (uint8_t)(v8Dst + (uint8_t)n16);
                //::wm8(dst, v8Src);
                if (nDst < _RPHL) *dst = v8Src;
                else wm8(vDst, v8Src);
                n8 = (uint8_t)(n16 == 1 ? 0 : 2);
                nfl = _FS(v8Src) | _FZ(v8Src) | _FH(v8Dst, (uint8_t)n16, 0, n8) | n8 | ((v8Dst == 0x80) << 3);
            }
            break;
        case O_XOR:
            n8 = (*_A ^= v8Src);
            nfl = _FS(n8) | _FZ(n8) | _FP(n8);
            break;
        case O_OR:
            n8 = (*_A |= v8Src);
            nfl = _FS(n8) | _FZ(n8) | _FP(n8);
            break;
        case O_AND:
            n8 = (*_A &= v8Src);
            nfl = _FS(n8) | _FZ(n8) | (uint8_t)16 | _FP(n8);
            break;
        case O_CP:
            n8 = (uint8_t)(n16 = (v8Dst - v8Src));
            nfl = _FS(n8) | _FZ(n8) | _FH(v8Dst, v8Src, 0, 1) | _FV1(v8Dst, n8) | (uint8_t)2 | (uint8_t)(n16 > 255);
            break;
        case O_ROT:
            *dst = rotate(v8Dst, &nfl);
            nfl &= fl;
            break;
        case O_IN:
            *dst = n8 = ALU->readPort(v8Src, nSrc == _RC ? *_B : *_A);
            if(fl) nfl = _FS(n8) | _FZ(n8) | _FP(n8);
            break;
        case O_OUT:
            ALU->writePort(v8Src, nSrc == _RC ? *_B : *_A, v8Dst);
            break;
        case O_PUSH:
            (*_SP) -= 2;
            wm16(*_SP, vDst);
            break;
        case O_POP:
            *(uint16_t*)dst = rm16(*_SP);
            (*_SP) += 2;
            break;
        case O_JMP:
            *_PC = vDst;
            ticks = 14;
            break;
        case O_JR | O_JMP:
            (*_PC) += (int8_t)v8Src;
            ticks = 12;
            break;
        case O_CALL | O_JMP:
            call(vDst);
            ticks = 17;
            break;
        case O_RETN: case O_RETI:
            *_IFF1 = *_IFF2;
            ticks = 8;
        case O_RET | O_JMP:
            (*_PC) = rm16(*_SP);
            (*_SP) += 2;
            ticks += 6;
            break;
        case O_ROT_CB:
            n8 = rotate(ticks > 15 ? v8Src : v8Dst, &nfl);
            if(ticks > 15) {
                wm8(vSrc, n8);
                if(nDst == _RPHL) break;
            }
            if(nDst == _RPHL) wm8(vDst, n8);
            else *dst = n8;
            break;
        case O_SET:
            // под префиксом?
            if(ticks > 15) {
                n8 = v8Src | bit;
                //::wm8(src, n8);
                wm8(vSrc, n8);
                if(nDst == _RPHL) break;
            } else n8 = v8Dst | bit;
            //::wm8(dst, n8);
            if(nDst == _RPHL) wm8(vDst, n8);
            else *dst = n8;
        case O_RES:
            // под префиксом?
            if(ticks > 15) {
                n8 = v8Src & ~bit;
                //::wm8(src, n8);
                wm8(vSrc, n8);
                if(nDst == _RPHL) break;
            } else n8 = v8Dst & ~bit;
            //::wm8(dst, n8);
            if(nDst == _RPHL) wm8(vDst, n8);
            else *dst = n8;
            break;
        /* Z=1 если проверяемый бит = 0; S=1 если bit = 7 и проверяемый бит = 1; PV = Z */
        case O_BIT:
            if(ticks > 15) v8Dst = v8Src;
            n8 = (uint8_t)((v8Dst & bit) == 0); // Z
            v8Src = (uint8_t)(bit == 128 && n8 == 0); // S
            nfl = (v8Src << 7) | (n8 << 6) | (n8 << 2);
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
            nfl = opsSpec(v8Dst, v8Src, vDst, vSrc, dst);
            break;
        case O_NEG:
            v8Src = (uint8_t) (0 - v8Dst);
            *_A = v8Src;
            nfl = (uint8_t) (_FS(v8Src) | _FZ(v8Src) | _FH(0, v8Dst, 0, 1) | ((v8Dst == 0x80) << 2) | 2 | (v8Dst != 0));
            break;
        case O_IM:
            n8 = (uint8_t)((codeOps >> 3) & 3);
            *_IM = (uint8_t)(n8 ? n8 - 1 : 0);
            break;
        case O_RLD:
//            ::wm8(dst, (uint8_t)((v8Dst << 4) | (v8Src & 15)));
            wm8(vDst, (uint8_t)((v8Dst << 4) | (v8Src & 15)));
            n8 = *_A = (uint8_t)((v8Src & 240) | (uint8_t)((v8Dst & 240) >> 4));
            nfl = _FS(n8) | _FZ(n8) | _FP(n8);
            break;
        case O_RRD:
//            ::wm8(dst, (uint8_t)((v8Dst >> 4) | (v8Src << 4)));
            wm8(vDst, (uint8_t)((v8Dst >> 4) | (v8Src << 4)));
            n8 = *_A = (uint8_t)((v8Src & 240) | (v8Dst & 15));
            nfl = _FS(n8) | _FZ(n8) | _FP(n8);
            break;
        case O_REP:
            nfl = opsRep(fl, &ticks);
            break;
        case O_NONI: return noni();
    }
    if(fl) uflags(fl, nfl);
    return ticks;
}

uint8_t zxCPU::opsSpec(uint8_t v8Dst, uint8_t v8Src, uint16_t vDst, uint16_t vSrc, uint8_t* dst) {
    uint8_t nfl = 0, n8;
    uint16_t n16;
    switch (codeOps) {
        case OUT_PC_0:
            ALU->writePort(v8Src, *_B, 0);
            break;
        case IN_F_PC:
            n8 = ALU->readPort(v8Src, *_B);
            nfl = _FS(n8) | _FZ(n8) | _FP(n8);
            break;
        case NOP:
            break;
        case CPL:
            *_A = ~*_A;//v8Dst;
            nfl = 18;
            break;
        case CCF:
            n8 = getFlag(FC);
            nfl = (n8 << 4) | (n8 == 0);
            break;
        case SCF:
            nfl = 1;
            break;
        case DAA:
            v8Src = 0;
            n8 = getFlag(FC);
            if (((v8Dst & 15) > 9) || getFlag(FH)) v8Src = 6;
            if ((v8Dst > 0x99) || n8) { v8Src |= 96; n8 = 1; }
            n16 = v8Dst + (uint16_t)(getFlag(FN) ? -v8Src : v8Src);
            *_A = v8Dst = (uint8_t)n16;
            nfl = _FS(v8Dst) | _FZ(v8Dst) | _FP(v8Dst) | (uint8_t)(n8 | (n16 > 255));
            break;
        case EI: case DI:
            v8Dst = (uint8_t)((codeOps & 8) >> 3);
            *_IFF1 = *_IFF2 = v8Dst;
            modifySTATE(0, ZX_INT);
            break;
        case HALT:
            modifySTATE(ZX_HALT, 0);
            break;
        case DJNZ:
            (*_B)--;
            if (*_B) (*_PC) += (int8_t) v8Dst;
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
            *(uint16_t*)dst = n16;
            break;
        case JP_HL:
            *_PC = vDst;
            break;
    }
    return nfl;
}

int zxCPU::noni() {
    info("noni PC: %i (pref: %i op: %i)", (uint16_t)((*_PC) - 2), codeExOps, codeOps);
    return 8;
}

uint8_t zxCPU::rotate(uint8_t value, uint8_t* nfl) {
    auto fc = getFlag(FC);
//    auto ops = (uint8_t)((codeOps >> 3) & 7); // XX000XXX 32 16 8
//    auto nfc = (uint8_t)((value & ((ops & 1) ? 1 : 128)) != 0);
//    auto nvalue = ((ops & 1) ? value >> 1 : value << 1);
    auto nfc = (uint8_t)((value & ((codeOps & 8) ? 1 : 128)) != 0);
    auto nvalue = ((codeOps & 8) ? value >> 1 : value << 1);
    switch(codeOps & 56) {
        // RLC c <- 7 <----- 0 <- 7
//        case 0: value = nfc; break;
        case 0: value = nfc; break;
        // RRC 0 -> 7 ------> 0 -> c
//        case 1: value = (nfc << 7); break;
        case 8: value = (nfc << 7); break;
        // RL c <- 7 <------ 0 <- c
//        case 1: value = (nfc << 7); break;
        case 16: value = (nfc << 7); break;
        // RR c -> 7 -------> 0 -> c
//        case 3: value = (fc << 7); break;
        case 24: value = (fc << 7); break;
        // SLA c <- 7 <---------- 0 <- 0
        // SRL 0 -> 7 ------------> 0 -> c
//        case 4: case 7: value = 0; break;
        case 32: case 56: value = 0; break;
        // SRA 7 -> 7 ------------> 0 -> c
//        case 5: value &= 128; break;
        case 40: value &= 128; break;
        // SLI c <- 7 <----------0 <- 1
//        case 6: value = 1; break;
        case 48: value = 1; break;
    }
    value |= nvalue;
    *nfl = _FS(value) | _FZ(value) | _FP(value) | nfc;
    return value;
}

uint8_t zxCPU::opsRep(uint8_t fl, int* ticks) {
    uint8_t nfl(0), n1, n2, bc;
    int dx = fl & F3 ? -1 : 1;
    auto rep = fl & F5 ? 1 : 0;
    switch(codeOps) {
        case LDDR: case LDIR: case LDD: case LDI:
            wm8(*_DE, rm8(*_HL));
            (*_DE) += dx; (*_HL) += dx; (*_BC)--;
            nfl = (uint8_t)(((*_BC) != 0) << 2);
            if(rep && nfl) {
                (*_PC) -= 2;
                *ticks = 21;
            }
            break;
        case CPDR: case CPIR: case CPD: case CPI:
            n1 = rm8(*_HL);
            (*_HL) += dx; (*_BC)--;
            n2 = n1 - *_A;
            bc = (uint8_t)(((*_BC) != 0) << 2);
            nfl = _FS(n2) | _FZ(n2) | _FH(n1, n2, 0, 1) |  (uint8_t)(2 | bc);
            if(bc && rep) {
                (*_PC) -= 2;
                *ticks = 21;
            }
            break;
        case INDR: case INIR: case IND: case INI:
            n1 = ALU->readPort(*_C, *_B);
            wm8(*_HL, n1); (*_HL) += dx; (*_B)--;
            bc = (uint8_t)((*_B) != 0);
            nfl = _FS(n1) | _FZ(bc) | (uint8_t)2;
            if(bc && rep) {
                (*_PC) -= 2;
                *ticks = 21;
            }
            break;
        case OTDR: case OTIR: case OUTD: case OUTI:
            n1 = rm8(*_HL);
            ALU->writePort(*_C, *_B, n1);
            (*_HL) += dx; (*_B)--;
            bc = (uint8_t)((*_B) != 0);
            nfl = _FS(n1) | (uint8_t)2 | _FZ(bc);
            if(rep && bc) {
                (*_PC) -= 2;
                *ticks = 21;
            }
            break;
    }
    return nfl;
}

std::string zxCPU::daMakeMnemonic(zxCPU::MNEMONIC *m, int prefix, int code, uint16_t pc, uint16_t vDst, uint16_t vSrc, uint8_t v8Dst, uint8_t v8Src) {
    uint16_t codes[64];
    uint8_t pos = 0;

    auto cmd = m->name;
    auto regDst = m->regs & 15;
    auto regSrc = m->regs >> 4;
    //                                    "XH", "XL", "YH", "YL", "(IX", "(IY", "F", "IX", "IY", "IM ",
    auto ops = m->tiks & (C_8BIT | C_16BIT);
    uint8_t fl = m->flgs;

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
                    if(ops & C_8BIT) { DA(C_NN); DA(v8Src); }
                    else if(ops & C_16BIT) { DA(C_NN); DA(vSrc); }
                    else DA(regSrc);
                    break;
                case O_LOAD:
                    // cmd reg, (NN)/(RP)/(HL/IX/IY + D)
                    DA(regDst); DA(C_COMMA);
                    if(ops & C_16BIT) { DA(C_PNN); DA(vSrc); }
                    else if(regSrc >= _RBC) DA(C_PBC + (code >> 4));
                    else DA(regSrc);
                    break;
                case O_SAVE:
                    // cmd (NN)/(RP)/(HL/IX/IY + D), reg/N
                    if(ops & C_16BIT) { DA(C_PNN); DA(vDst); }
                    else if(regDst >= _RBC) DA(C_PBC + (code >> 4));
                    else DA(regDst); // (HL/IX/IY + D)
                    DA(C_COMMA);
                    if(ops & C_8BIT) { DA(C_NN); DA(v8Src); }
                    else DA(regSrc);
                    break;
            }
            break;
        case C_AND: case C_OR: case C_XOR: case C_CP:
        case C_ADD: case C_ADC: case C_SUB: case C_SBC:
            // cmd reg A,SRC/N
            DA(regDst); DA(C_COMMA);
            if(ops & C_8BIT) { DA(C_NN); DA(v8Src); } else DA(regSrc);
            break;
        case C_RST:
            DA(C_NN); DA(code & 56);
            break;
        case C_IM:
            DA(C_NN); DA((code >> 3) & 3);
            break;
        case C_IN:
            if(ops & C_8BIT) {
                DA(C_A); DA(C_COMMA); DA(C_NN); DA(v8Src);
            } else {
                DA(regSrc); DA(C_COMMA); DA(C_PC);
            }
            break;
        case C_OUT:
            if(ops & C_8BIT) {
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