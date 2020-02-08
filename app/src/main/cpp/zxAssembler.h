//
// Created by Сергей on 17.12.2019.
//

#pragma once

#define ERROR_OK                              1
#define ERROR_INVALID_COMMAND                -1
#define ERROR_COMMA_NOT_FOUND                -2
#define ERROR_PARAMETER_OUT_OF_RANGE         -3
#define ERROR_DISP_OUT_OF_RANGE              -4
#define ERROR_RELATIVE_JUMP_OUT_OF_RANGE     -5
#define ERROR_NUMBER_OUT_OF_RANGE            -6
#define ERROR_DISP_NOT_FOUND                 -7
#define ERROR_COMMAND_NOT_FOUND              -8
#define ERROR_CLOSE_BRAKKET_NOT_FOUND        -9
#define ERROR_INVALID_INDEXED_OPERATION     -10
#define ERROR_INVALID_RST_NUMBER            -11
#define ERROR_INVALID_INTERRUPT_MODE        -12
#define ERROR_KEYWORD_NOT_FOUND             -13
#define ERROR_EXPECT_END_OF_COMMAD          -14
#define ERROR_BIT_OUT_OF_RANGE              -15
#define ERROR_INVALID_ADDRESS_OPERAND       -16

class zxAssembler {
public:
    zxAssembler() : cmd_begin(nullptr), text(nullptr), number(0), idx_cmd(0), buf(nullptr), disp(0) { }
    int parser(int address, const char *text);
protected:
    void skip_spc(char** text);
private:
    int get_word(char** text);
    int parse_operand(char** text);
    int cmd_parser(uint16_t addr);
    int get_cmd(const char* cmd, int len);
    int skipComma(char** text) {
        cmd_begin = *text;
        if(cmd_begin[0] != ',') return 1;
        cmd_begin++;
        skip_spc(&cmd_begin);
        *text = cmd_begin;
        return 0;
    }
    int isEof(char *text) { return text[0] == 0 ? ERROR_OK : ERROR_EXPECT_END_OF_COMMAD; }
    char* cmd_begin;
    char* text;
    int idx_cmd;
    uint8_t disp;
    int16_t number;
    uint8_t* buf;

    bool checkFlags(int flags);

    bool checkReg16(int reg, bool is_sp);

    bool checkReg8(int reg);
};
