//
// Created by Сергей on 17.12.2019.
//

#include "zxCommon.h"
#include "zxDebugger.h"
#include "stkMnemonic.h"
#include "zxDA.h"

static MNEMONIC* skipPrefix(int& entry, int& prefix, int& code, int& offset) {
    offset = 0;
    MNEMONIC* m(nullptr);
    prefix = 0;

    while(true) {
        code = rm8((uint16_t)entry);
        m = &mnemonics[code + offset];
        if(m->ops != O_PREF) break;
        entry++;
        offset = m->flags << 8;
        if(offset == 256 && prefix) {
            entry++;
//            continue;
        }
        prefix = m->tiks & 31;
    }
    return m;
}

static int computeCmdLength(int address) {
    auto entry = address;
    int prefix, code, offs;
    auto m = skipPrefix(entry, prefix, code, offs);
    // displacement
    entry += (uint8_t)(prefix && (m->regSrc == _RPHL || m->regDst == _RPHL));
    // operands
    entry += (m->tiks >> 5);
    return entry;
}

const char *zxDebugger::itemList(int cmd, int data, int flags){
    auto buf = (uint16_t *) &TMP_BUF[0];
    auto res = (char*) &TMP_BUF[512]; auto rtmp = res;
    auto tmp = (uint16_t)data;
    switch(cmd) {
        case ZX_DEBUGGER_MODE_PC:
            // disassembler
            zxDA::cmdParser(&tmp, buf, false);
            zxDA::cmdToString(buf, res, flags);
            break;
        case ZX_DEBUGGER_MODE_SP: {
            // stack addr       >content<     chars
            auto isSP = tmp == *ULA->cpu->_SP;
            int val;
            auto length = 5 - opts[ZX_PROP_SHOW_HEX];
            if(data < 65535 && data >= 0) ssh_strcpy(&rtmp, ssh_fmtValue(data, ZX_FV_OPS16, true));
            else ssh_char(&rtmp, '?', length);
            auto count = 8 - isSP;
            ssh_char(&rtmp, ' ', count);
            if(isSP) *rtmp++ = '>';
            if(data < 65535 && data >= 0) {
                val = rm16((uint16_t)data);
                ssh_strcpy(&rtmp, ssh_fmtValue(val, ZX_FV_OPS16, true));
            } else {
                ssh_char(&rtmp, '?', length);
                val = -1;
            }
            if(isSP) *rtmp++ = '<';
            ssh_char(&rtmp, ' ', count);
            uint8_t v;
            for (int i = 0; i < 16; i++) {
                if(val < 0) v = 0; else v = rm8((uint16_t) (val + i));
                if (v < 32) v = '.';
                else if(v > 127) v = '.';
                *rtmp++ = v;
            }
            tmp += 2;
            *rtmp = 0;
        }
            break;
        case ZX_DEBUGGER_MODE_DT: {
            // data
            uint8_t v;
            auto tres = rtmp + 256;
            // execFlags значений
            // address
            if(data < 65536 && data >= 0) ssh_strcpy(&rtmp, ssh_fmtValue(data, ZX_FV_OPS16, true));
            else ssh_char(&rtmp, '?', 5 - opts[ZX_PROP_SHOW_HEX]);
            ssh_char(&rtmp, ' ', 3);
            for (int i = 0; i < flags; i++) {
                auto t = (data + i);
                if(t < 65536 && t >= 0) {
                    v = rm8((uint16_t) t);
                    ssh_strcpy(&rtmp, ssh_fmtValue((int) v, ZX_FV_OPS8, true));
                } else {
                    ssh_char(&rtmp, '?', 3 - opts[ZX_PROP_SHOW_HEX]);
                    v = 0;
                }
                *rtmp++ = ' ';
                if (v < 32) v = '.';
                else if (v > 127) v = '.';
                *tres++ = v;
            }
            *tres = 0;
            ssh_char(&rtmp, ' ', 3);
            ssh_strcpy(&rtmp, res + 256);
            tmp += flags;
            *rtmp = 0;
            break;
        }
    }
    opts[ZX_PROP_JNI_RETURN_VALUE] = (uint8_t)(tmp - data);
    return res;
}

void zxDebugger::trace(int mode) {
    uint16_t yAddr, nAddr;
    auto pc = *ULA->cpu->_PC;
    if(mode == ZX_CMD_TRACE_OVER) yAddr = nAddr = *zxULA::_CALL;
    else {
        if(jump(pc, ZX_DEBUGGER_MODE_PC, mode == ZX_CMD_TRACE_IN) == ZX_DEBUGGER_MODE_PC) {
            yAddr = *(uint16_t *) (opts + ZX_PROP_JNI_RETURN_VALUE);
            nAddr = *(uint16_t *) (opts + ZX_PROP_JNI_RETURN_VALUE + 2);
        } else {
            yAddr = nAddr = (uint16_t)computeCmdLength(pc);
        }
    }
    auto tm = time(nullptr);
    auto _pc = ULA->cpu->_PC;
    while(true) {
        ULA->stepDebug();
        if(*_pc == yAddr || *_pc == nAddr) break;
        if((time(nullptr) - tm) >= 3) break;
    }
}

int zxDebugger::jump(uint16_t entry, int mode, bool isCall) {
    uint16_t jmp(0), next(0);
    int addr(entry), offs;
    MNEMONIC* m;
    auto cpu = ULA->cpu;
    int prefix, code;
    switch(mode) {
        case ZX_DEBUGGER_MODE_PC:
            // взять инструкцию по адресу
            m = skipPrefix(addr, prefix, code, offs);
            entry = (uint16_t)(addr + 1);
            switch(m->ops) {
                case O_SAVE: case O_LOAD:
                    if(m->regDst == _C16 || m->regSrc == _C16) {
                        jmp = rm16(entry);
                        next = (uint16_t)(entry + 2);
                        mode = ZX_DEBUGGER_MODE_DT;
                    } else mode = -1;
                    break;
                case O_RET:
                    jmp = rm16(*cpu->_SP);
                    next = entry;
                    break;
                case O_SPEC:
                    if(code == JP_HL) {
                        if(prefix == 12) jmp = *cpu->_IX;
                        else if(prefix == 24) jmp = *cpu->_IY;
                        else jmp = *cpu->_HL;
                        next = entry;
                        break;
                    } else if(code != DJNZ) {
                        mode = -1;
                        break;
                    }
                case O_JR:
                    code = rm8(entry++);
                    jmp = entry + (int8_t)code;
                    next = entry;
                    break;
                case O_RST:
                    if(!isCall) { mode = -1; break; }
                    jmp = (uint16_t)(code & 56);
                    next = entry;
                    break;
                case O_CALL:
                    if(!isCall) { mode = -1; break; }
                case O_JMP:
                    jmp = rm16(entry);
                    next = (uint16_t)(entry + 2);
                    break;
                default: mode = -1; break;
            }
            break;
        case ZX_DEBUGGER_MODE_SP:
            jmp = rm16(entry);
            mode = ZX_DEBUGGER_MODE_PC;
            break;
        case ZX_DEBUGGER_MODE_DT:
            mode = -1;
            break;
    }
    *(uint16_t*)(opts + ZX_PROP_JNI_RETURN_VALUE) = jmp;
    *(uint16_t*)(opts + ZX_PROP_JNI_RETURN_VALUE + 2) = next;
    return mode;
}

static int computeWndLength(int address, int finish, int count, int* length) {
    *length = 0;
    auto buf = (uint16_t*)&TMP_BUF[0];
    while(count-- > 0 && address < finish) {
        auto len = (uint16_t)(computeCmdLength(address) - address);
        *buf++ = len;
        *length += len;
        address += len;
    }
    return count + 1;
}

int zxDebugger::move(int entry, int delta, int count) {
    int length(0);
    int finish = 0;
    if(delta < 0) {
        auto buf = (uint16_t*)&TMP_BUF[0];
        delta = -delta;
        int wnd = delta + count;
        auto size = computeWndLength(entry, 65536, wnd, &length);
        // отмотать назад на count
        for(int i = 1 ; i <= count; i++) {
            auto ll = buf[wnd - i - size];
            length -= ll;
        }
        entry += length;
        finish = size != 0;
    } else if(delta > 0) {
        entry -= delta;
        if(entry < 0) { entry = 0; finish = 1; }
    }
    *(int *) (opts + ZX_PROP_JNI_RETURN_VALUE) = entry;
    return finish;
}
