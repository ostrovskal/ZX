//
// Created by Сергей on 17.12.2019.
//

#include "zxCommon.h"
#include "zxDebugger.h"
#include "stkMnemonic.h"
#include "zxDA.h"

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
            zxDA::cmdParser(&pc, (uint16_t*)&TMP_BUF[0], false);
            yAddr = nAddr = pc;
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

int zxDebugger::move(int entry, int delta) {
    /*
	_pc = pc;
	if(count > 0) {
		while(count--) {
			pos = 0;
			execute(0, 0);
		}
	} else if(count < 0) {
		_pc += count;
		return _pc;
		int cc = 200;
		int mn_diff = 1000000;
		int mn_addr = 0;
		int pc_t = _pc + count;
		while(cc--) {
			int addr = cmdJump(_pc);
			if(addr < pc_t) {
				int diff = pc_t - addr;
				int mn = diff + count;
				if(mn < mn_diff) {
					mn_diff = mn;
					mn_addr = addr;
					if(mn < 30) break;
				}
			}
			pos = 0;
			execute(0, 0);
		}
		if(mn_diff > 7000) _pc += count;
		else {
			int pc;
			_pc = mn_addr;
			while(mn_diff > 0) {
				pos = 0;
				pc = _pc;
				execute(0, 0);
				mn_diff -= (_pc - pc);
				if(_pc > pc_t) { _pc = pc; break; }
			}
		}
	}
	return _pc;
     *
     */
    return 0;
}
