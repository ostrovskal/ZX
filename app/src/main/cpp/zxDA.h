//
// Created by Сергей on 03.12.2019.
//

#pragma once

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

enum DA_INSTRUCTION_FLAGS {
    DA_PC   = 1,  // адрес
    DA_CODE = 2,  // машинные коды
    DA_PN   = 4,  // содержимое адреса
    DA_PNN  = 8,  // вычисляемое значение
    DA_LABEL= 16, // метки
    DA_REGS = 32  // гегистры
};

class zxDA {
public:
    // парсер инструкции
    static size_t cmdParser(uint16_t* pc, uint16_t* buffer, bool regsSave);

    // формирование распарсенной дизассемблерной строки инструкции
    static const char* cmdToString(uint16_t* buffer, char* daResult, int flags);
protected:
    // получение информации об операндах инструкции
    static int getOperand(uint8_t o, uint8_t oo, int prefix, uint16_t* v16 = nullptr, uint16_t *pc = nullptr, int* ticks = nullptr, uint8_t* offset = nullptr);

    static MNEMONIC* skipPrefix(uint16_t* pc, uint16_t* code, int* pref, int* prefix, int* ticks, uint16_t* v, uint8_t* offs);

    static const char* searchLabel(int address);
};

#pragma clang diagnostic pop