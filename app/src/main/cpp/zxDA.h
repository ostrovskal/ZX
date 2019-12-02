//
// Created by Сергей on 03.12.2019.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma once

enum DA_INSTRUCTION_FLAGS {
    DA_PC   = 1,   // адрес
    DA_CODE = 2,   // машинные коды
    DA_TICKS= 4,   // такты
    DA_PN   = 8,   // содержимое адреса
    DA_PNN  = 16   // вычисляемое значение
};

class zxDA {
public:
    zxDA() {}

    // формирование дизассемблерной строки инструкции
    static char* daMake(uint16_t* pc, int flags, int bp, int prefix = 0, int offset = 0, int ticks = 0);

protected:
    // получение информации об операндах инструкции
    static int getDaOperand(uint8_t o, uint8_t oo, int prefix, uint16_t* v16 = nullptr,
                     uint16_t *pc = nullptr, int* ticks = nullptr, uint8_t* offset = nullptr);

};

#pragma clang diagnostic pop