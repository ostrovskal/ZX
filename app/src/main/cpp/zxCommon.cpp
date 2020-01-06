#include <alloca.h>
//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"

uint8_t*    TMP_BUF     = nullptr;
zxALU*      ALU         = nullptr;
uint8_t*    opts        = nullptr;
uint8_t*    labels      = nullptr;
BREAK_POINT* bps        = nullptr;

std::string FOLDER_FILES("");
std::string FOLDER_CACHE("");

// кэш адресов перехода и текущая позиция занесения в кэш
uint16_t    cmdCache[512];
int         currentCmdPos(0);
uint8_t     numBits[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static uint8_t sym[] =  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
static uint8_t tbl[] =  { 0,  4,  4,  4,  4,  4,  4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 15, 14, 14, 14, 14, 14, 14, 12, 12 };
static uint8_t valid[] ={ 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   1,  2,  3,  4,  5,  6,  7,  8,  9 };

void info(const char* msg, ...) {
    va_list varArgs;
    va_start(varArgs, msg);
    __android_log_vprint(ANDROID_LOG_WARN, "ZX", msg, varArgs);
    __android_log_print(ANDROID_LOG_WARN, "ZX", "\n");
    va_end(varArgs);
}

#ifdef DEBUG
    void debug(const char* msg, ...) {
        va_list varArgs;
        va_start(varArgs, msg);
        __android_log_vprint(ANDROID_LOG_DEBUG, "ZX", msg, varArgs);
        __android_log_print(ANDROID_LOG_DEBUG, "ZX", "\n");
        va_end(varArgs);
    }
#else
    void debug(const char* msg, ...) { }
#endif

// вернуть адрес памяти
uint8_t* realPtr(uint16_t address) {
    if(address < 16384) return &zxALU::pageROM[address];
    else if(address < 32768) return &zxALU::pageVRAM[address - 16384];
    else if(address < 49152) return &zxALU::PAGE_RAM[2][address - 32768];
    return &zxALU::pageRAM[address - 49152];
}

// распаковка блока памяти (с учетом или без завершающей сигнатуры)
bool unpackBlock(uint8_t* src, uint8_t* dst, uint8_t* dstE, uint32_t sz, bool packed, bool sign) {
    if(packed) {
        while(sz > 0 && dst < dstE) {
            sz--;
            uint8_t b = *src++;
            uint8_t c = 1;
            if (b == 0xed && *src == 0xed) {
                src++;
                c = *src++;
                b = *src++;
                sz -= 3;
            }
            if (c == 1) *dst++ = b; else dst = ssh_memset(dst, b, c);
        }
        if(sz > 0) {
            if(sign) return (sz == 4 && (*(u_long*) src == 0x00eded00));
        }
        return (sz == 0);
    }
    memcpy(dst, src, sz);
    return true;
}

static void packSegment(uint8_t ** dst, uint32_t count, uint8_t block) {
    auto dest = *dst;
    if (count == 1) {
        *dest++ = block;
    } else if (count < 5 && block != 0xed) {
        ssh_memset(*dst, block, count);
        dest += count;
    } else {
        if (*(dest - 1) == 0xed) {
            *dest++ = block;
            count--;
        }
        *dest++ = 0xed; *dest++ = 0xed;
        *dest++ = (uint8_t)count; *dest++ = block;
    }
    *dst = dest;
}

// распаковка блока памяти (с учетом или без завершающей сигнатуры)
uint8_t* packBlock(uint8_t* src, uint8_t* srcE, uint8_t* blk, bool sign, uint32_t& newSize) {
    uint8_t *dst = blk;
    uint8_t block = 0;
    uint32_t count = 0;
    while(src < srcE) {
        block = *src++;
        count++;
        if (count == 255 || block != *src) {
            packSegment(&dst, count, block);
            count = 0;
        }
    }
    if (count) packSegment(&dst, count, block);
    if(sign) {
        *(u_long *)dst = 0x00eded00;
        dst += 4;
    }
    newSize = (uint32_t)(dst - blk);
    return dst;
}

static void itos(int n, char** buffer) {
    auto buf = *buffer;
    uint8_t s = 0;
    if(n < 0) { s = '-'; n = -n; }
    do { *--buf = (uint8_t)((n % 10) + '0'); n /= 10; } while(n);
    if(s) *--buf = s;
    *buffer = buf;
}

// преобразовать число в строку, в зависимости от системы счисления
char* ssh_ntos(void* v, int r, char** end) {
    static char buffer[32];
    static uint8_t tbl[] = { 0, 10, 15, 4, 1, 1, 7, 3, 8, 10, 4, 10, 0, 0 };
    auto buf = &buffer[16]; *buf = 0;
    if(end) *end = buf;
    char* st;
    uint8_t c(tbl[r * 2 + 0]), s(tbl[r * 2 + 1]);
    int n; double d(0);
    uint8_t sgn = 0;
    switch(r) {
        case RADIX_DEC:
            itos(*(int*)v, &buf);
            break;
        case RADIX_HEX: case RADIX_OCT:
        case RADIX_BIN:
            n = *(int*)v;
            if(n < 0) { sgn = '-'; n = -n; }
            do { *--buf = sym[(uint8_t) (n & c)]; n >>= s; } while(n);
            if(sgn) *--buf = sgn;
            break;
        case RADIX_FLT: d = (double)(*(float*)v);
        case RADIX_DBL: if(r == RADIX_DBL) d = *(double*)v;
            // отбросим дробную часть
            n = (int)d;
            itos(n, &buf);
            st = buf; buf = &buffer[16]; *buf++ = '.';
            for(int i = 0 ; i < c; i++) {
                d -= n; d *= 10.0; n = (uint32_t)d;
                *buf++ = (uint8_t)(n + '0');
            }
            if(end) *end = buf;
            *buf = 0; buf = st;
            break;
        case RADIX_BOL:
            memcpy(buf , (*(bool*)v) ? "true\0\0" : "false\0", 6);
            break;
        default: debug("Неизвестная система счисления! %i", r); break;
    }
    return buf;
}

static int stoi(const char** s, uint8_t order, uint8_t msk) {
    const char* str = *s, *end = str - 1;
    int n(0), nn(1);
    // поиск конца
    char ch;
    while((ch = *++end)) {
        if(ch >='a') ch -= 32;
        if(ch < '0' || ch > 'F') break;
        if (!(tbl[ch & 31] & msk)) break;
    }
    *s = end;
    while(end-- != str) {
        n += (valid[(*end) & -97] * nn);
        nn *= order;
    }
    return n;
}

// преобразовать строку в число в зависимости от системы счисления
void* ssh_ston(const char* s, int r, const char** end) {
    static uint8_t rdx[] = { 10, 8, 16, 4, 2, 1, 8, 2, 10, 8, 10, 8, 0, 0 };
    static uint64_t res;

    int sign = 1;
    if(*s == '+' || *s == '-') sign = (*s++ == '-') ? -1 : 1;
    if(*s == '#') { r = RADIX_HEX; s++; }
    auto msk(rdx[r * 2 + 1]);
    int n(0); double d(0.0);
    switch(r) {
        case RADIX_DEC: case RADIX_HEX: case RADIX_BIN: case RADIX_OCT:
            n = stoi(&s, rdx[r * 2 + 0], msk) * sign;
            *(int*)&res = n;
            break;
        case RADIX_DBL: case RADIX_FLT:
            n = stoi(&s, 10, 8);
            if(*s == '.') {
                auto e = s;
                // поиск конца
                while(*++e) {
                    auto ch = *e & -33;
                    if (ch > 'F') break;
                    if (!(tbl[ch & 31] & msk)) break;
                }
                if(end) *end = e;
                end = nullptr;
                while(--e != s) {
                    auto ch = *e & -33;
                    if (ch > 'F') break;
                    ch &= 31;
                    if (!(tbl[ch] & msk)) break;
                    d += valid[ch]; d /= 10.0;
                }
            }
            d += n; d *= sign;
            if(r == RADIX_FLT) *(float*)&res = (float)d; else *(double*)&res = d;
            break;
        case RADIX_BOL:
            if(!strcasecmp(s, "true")) { s += 4; n = 1; }
            else if(!strcasecmp(s, "false")) { s += 5; n = 0; }
            *(uint8_t*)&res = (uint8_t)n;
            break;
        default: debug("Неизвестная система счисления! %i", r); break;
    }
    if(end) *end = s;
    return &res;
}

char __unused ** ssh_split(const char* str, const char* delim, int* count) {
    static char* tmp[32];
    int c = 0;
    auto ld = strlen(delim);
    while(*str) {
        auto nstr = strstr((char*)str, delim);
        tmp[c++] = (char*)str;
        if(nstr) *nstr = 0; else break;
        str = nstr + ld;
    }
    if(count) *count = c;
    return tmp;
}

static const char* fmtTypes[]	= { "3X", "2X", "3X ", "2X ",
                                     "5(X)", "4(#X)", "3(X)", "2(#X)",
                                     "3X)", "2#X)",
                                     "5X", "4X",
                                     "5X", "4#X", "3X", "2#X",
                                     "5[X]", "4[#X]",
                                     "3{X}", "2{#X}",
                                     "5{X}", "4{#X}",
                                     "0X", "0#X",
                                     "0X", "0X"
};

char* ssh_fmtValue(int value, int offs, bool hex) {
    static char buffer[32];
    char ch;
    char* end = nullptr;
    auto tmp = &buffer[0];
    auto rdx = offs + (hex ? opts[ZX_PROP_SHOW_HEX] : 0);
    auto res = ssh_ntos(&value, rdx & 1, &end);
    auto spec = fmtTypes[rdx];
     auto lnum = (spec[0] - '0') - (end - res);
    if(*res == '-') {
        *tmp++ = *res++; lnum++;
    }
    else if(offs == ZX_FV_OFFS) {
        *tmp++ = '+';
    }
    while((ch = *++spec)) {
        if(ch == 'X') {
            while(lnum-- > 0) *tmp++ = '0';
            while(*res) *tmp++ = *res++;
        } else *tmp++ = ch;
    }
    *tmp = 0;
    return buffer;
}
