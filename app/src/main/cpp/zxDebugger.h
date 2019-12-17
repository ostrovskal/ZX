//
// Created by Сергей on 17.12.2019.
//

#pragma once

class zxDebugger {
public:
    zxDebugger() { }

    // трассировка
    void trace(int mode);

    // построение списков
    const char* itemList(int cmd, int data, int flags);

    // сдвиг указателя на ПС
    int move(int entry, int delta);

    // получение адреса в памяти/адреса перехода в инструкции/адреса из стека
    int jump(int address, int mode, bool isCall);


};
