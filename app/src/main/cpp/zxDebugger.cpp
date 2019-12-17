//
// Created by Сергей on 17.12.2019.
//

#include "zxCommon.h"
#include "zxDebugger.h"
#include "stkMnemonic.h"
#include "zxDA.h"

static uint8_t computeCmdLength(int address) {
    auto v = rm8((uint16_t)address++);
    auto m = &mnemonics[v];
    uint8_t len = 0;
    if(m->ops == O_PREF) {
        if(v == PREF_DD || v == PREF_FD) {
            v = rm8((uint16_t)address);
            if(v == PREF_DD || v == PREF_FD || v == PREF_ED) return 1;
            if(v == PREF_CB) return 4;
            m = &mnemonics[v];
            len = (uint8_t)(1 + (uint8_t)(m->regSrc == _RPHL || m->regDst == _RPHL));
        } else if(v == PREF_CB) {
            return 2;
        } else if(v == PREF_ED) {
            m = &mnemonics[rm8((uint16_t)address) + 512];
        }
    }
    return len + (m->tiks >> 5);
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
            auto isSP = tmp == *ALU->cpu->_SP;
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
            // flags значений
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
        }
            break;
        default:break;
    }
    opts[ZX_PROP_JNI_RETURN_VALUE] = (uint8_t)(tmp - data);
    return res;
}

void zxDebugger::trace(int mode) {
    uint16_t yAddr, nAddr;
    auto pc = *ALU->cpu->_PC;
    if(mode == ZX_CMD_TRACE_OVER) yAddr = nAddr = *zxALU::_CALL;
    else {
        if(jump(pc, ZX_DEBUGGER_MODE_PC, mode == ZX_CMD_TRACE_IN) == ZX_DEBUGGER_MODE_PC) {
            yAddr = *(uint16_t *) (opts + ZX_PROP_JNI_RETURN_VALUE);
            nAddr = *(uint16_t *) (opts + ZX_PROP_JNI_RETURN_VALUE + 2);
        } else {
            yAddr = nAddr = (uint16_t)(pc + computeCmdLength(pc));
        }
    }
    auto tm = time(nullptr);
    auto _pc = ALU->cpu->_PC;
    while(true) {
        ALU->stepDebug();
        if(*_pc == yAddr || *_pc == nAddr) break;
        if((time(nullptr) - tm) >= 3) break;
    }
}

int zxDebugger::jump(int address, int mode, bool isCall) {
    uint16_t ret(0), retn(0), addr((uint16_t)address);
    uint8_t tmp;
    int offs = 0;
    zxCPU::MNEMONIC* m;
    switch(mode) {
        case ZX_DEBUGGER_MODE_PC:
            // взять инструкцию по адресу
            mode = ZX_DEBUGGER_MODE_PC;
            tmp = rm8(addr++);
            if(tmp == PREF_ED) { tmp = rm8(addr++); offs = 512; }
            retn = addr;
            m = &mnemonics[tmp + offs];
            switch(m->ops) {
                case O_SAVE:
                case O_LOAD:
                    if(m->regDst == _C16 || m->regSrc == _C16) {
                        ret = rm16(addr);
                        mode = ZX_DEBUGGER_MODE_DT;
                    } else mode = -1;
                    break;
                case O_RET:
                    ret = rm16(*ALU->cpu->_SP);
                    break;
                case O_SPEC:
                    if(tmp == JP_HL) {
                        retn = ret = *ALU->cpu->_HL;
                        break;
                    } else if(tmp != DJNZ) { mode = -1; break; }
                case O_JR:
                    tmp = rm8(addr++);
                    ret = addr + (int8_t)tmp;
                    retn = addr;
                    break;
                case O_RST:
                    if(!isCall) { mode = -1; break; }
                    ret = (uint16_t)(tmp & 56);
                    break;
                case O_CALL:
                    if(!isCall) { mode = -1; break; }
                case O_JMP:
                    ret = rm16(addr);
                    retn = (uint16_t)(addr + 2);
                    break;
                default: mode = -1; break;
            }
            break;
        case ZX_DEBUGGER_MODE_SP:
            ret = rm16(addr);
            mode = ZX_DEBUGGER_MODE_PC;
            break;
        case ZX_DEBUGGER_MODE_DT:
            mode = -1;
            break;
    }
    *(uint16_t*)(opts + ZX_PROP_JNI_RETURN_VALUE) = ret;
    *(uint16_t*)(opts + ZX_PROP_JNI_RETURN_VALUE + 2) = retn;
    return mode;
}

static int computeWndLength(int address, int finish, int count, int* length) {
    *length = 0;
    auto buf = &TMP_BUF[0];
    while(count-- > 0 && address < finish) {
        auto len = computeCmdLength(address);
        *buf++ = len;
        *length += len;
        address += len;
    }
    return count + 1;
}

bool zxDebugger::findBackAddress(int entry, int address, int* addr) {
    int nmin = 1000000;
    *addr = -1;
    int n, nn;
    // искать в кэше
    for(int i = 0 ; i < 512; i++) {
        n = cmdCache[i];
        if(n == 0) continue;
        if(n > address) continue;
        nn = address - n;
        if(nmin > nn) {
            nmin = nn; *addr = n;
            if(nmin < 50) return true;
        }
    }
    if(nmin < 300) return true;
    // искать из переходов после
    for(int i = 0 ; i < 1000; i++) {
        if(jump(entry, ZX_DEBUGGER_MODE_PC, true) == ZX_DEBUGGER_MODE_PC) {
            n = *(uint16_t*)(opts + ZX_PROP_JNI_RETURN_VALUE);
            if(n < address) {
                nn = address - n;
                if(nmin > nn) {
                    nmin = nn; *addr = n;
                    if(nmin < 50) return true;
                }
            }
            entry = *(uint16_t*)(opts + ZX_PROP_JNI_RETURN_VALUE + 2);;
        } else {
            entry += computeCmdLength(entry);
        }
    }
    return nmin < 300;
}

int zxDebugger::move(int entry, int delta, int count) {
    int length(0);
    int finish = 0;
    if(delta < 0) {
        delta = -delta;
        int wnd = delta + count;
        auto size = computeWndLength(entry, 65536, wnd, &length);
        // отмотать назад на count
        for(int i = 1 ; i <= count; i++) length -= TMP_BUF[wnd - i - size];
        entry += length;
        finish = size != 0;
    } else if(delta > 0) {
        auto nentry = entry - (delta * 3 + 10);
        int adr;
        // найти ближайший к нему адрес
        if(!findBackAddress(entry, nentry, &adr)) {
            // не нашли - тупо отнимаем
            entry -= delta;
        } else {
            // нашли
            auto size = computeWndLength(adr, entry, 65535, &length);
            auto sz = 65535 - size;
            //info("move- nadr: %i adr: %i size: %i sz: %i length: %i delta: %i", nentry, adr, size, sz, length, delta);
            if(sz < delta) {
                // команд меньше, чем ожидалось
                // что делать?? - ну сдвинем не на дельту тогда
            }
            for (int i = 1; i <= delta; i++)
                entry -= TMP_BUF[sz - i];
        }
        if(entry < 0) { entry = 0; finish = 1; }
    }
    *(int *) (opts + ZX_PROP_JNI_RETURN_VALUE) = entry;
    return finish;
}
