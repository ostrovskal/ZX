//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <limits.h>
#include <cstring>
#include <stdarg.h>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <sys/time.h>

#include "zxFile.h"
#include "zxCPU.h"
#include "zxSpeccy.h"

struct MNEMONIC {
    // приемник
    uint8_t regDst;
    // источник
    uint8_t regSrc;
    // операция
    uint8_t ops;
    // такты|длина
    uint8_t tiks;
    // имя
    uint8_t name;
    // флаги
    uint8_t flags;
};

// Глобальные
extern zxSpeccy* 				        zx;
extern uint8_t* 			            opts;
extern uint8_t* 			            labels;
extern uint8_t* 			            TMP_BUF;
extern std::string 			            FOLDER_FILES;
extern std::string 			            FOLDER_CACHE;
extern BREAK_POINT*			            bps;
extern uint8_t                          numBits[8];
extern uint32_t                         frequencies[];

inline uint16_t wordLE(const uint8_t* ptr)	{ return ptr[0] | ptr[1] << 8; }

// вывод отладочной информации
void info(const char* msg, const char* file, const char* func, int line, ...);
void debug(const char* msg, const char* file, const char* func, int line, ...);

#ifdef DEBUG
    #define LOG_DEBUG(m, ...)           debug(m, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#else
    #define LOG_DEBUG(m, ...)
#endif

#define LOG_INFO(m, ...)                info(m, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

#define LOG_NAME                        "ZX"

constexpr int ZX_TOTAL_RAM              = 512 * 1024;
constexpr int ZX_SIZE_TMP_BUF           = 1024 * 1024;
constexpr int INDEX_OPEN                = 512 * 1024;
constexpr int INDEX_DA                  = 256 * 1024;
constexpr int INDEX_TEMP                = 500 * 1024;
constexpr int INDEX_CNV                 = 504 * 1024;
constexpr int INDEX_FMT                 = 506 * 1024;

constexpr int ZX_BP_NONE                = 0; // не учитывается
constexpr int ZX_BP_EXEC                = 1; // исполнение
constexpr int ZX_BP_WMEM                = 2; // запись в память
constexpr int ZX_BP_RPORT               = 3; // чтение из порта
constexpr int ZX_BP_WPORT               = 4; // запись в порт

constexpr int ZX_BP_OPS_EQ              = 0; // ==
constexpr int ZX_BP_OPS_NQ              = 1; // !=
constexpr int ZX_BP_OPS_GT              = 2; // >
constexpr int ZX_BP_OPS_LS              = 3; // <
constexpr int ZX_BP_OPS_GTE             = 4; // >=
constexpr int ZX_BP_OPS_LSE             = 5; // <=

// Биты состояний
enum ZX_STATE {
    //ZX_SCR  = 0x01, // когда рисуется экран
    ZX_PAUSE= 0x02, // пауза между загрузкой блоков TAP
    ZX_HALT = 0x04, // останов. ждет прерывания
    ZX_TRDOS= 0x08, // режим диска
    ZX_BP   = 0x10, // сработала точка останова
    ZX_DEBUG= 0x20, // режим отладки активирован
};

// Разделяемые свойства
// 0. Байтовые значения, вычисляемые во время работы программы
//constexpr int ZX_PROP_JOY_TYPE        = 90;// Текущий тип джойстика
constexpr int ZX_PROP_JOY_KEYS        = 91;  // Привазанные к джойстику коды кнопок клавиатуры (8) 91 - 98
constexpr int ZX_PROP_JOY_CROSS_VALUE = 99;  // Нажатые кнопки джойстика-крестовины
constexpr int ZX_PROP_JOY_ACTION_VALUE= 100; // Нажатые кнопки джойстика-управления
constexpr int ZX_PROP_KEY_CURSOR_MODE = 101; // Режим курсора (E, G, L, K т.п.)
constexpr int ZX_PROP_KEY_MODE        = 102; // Режим клавиатуры (CAPS LOCK, SYMBOL SHIFT)
constexpr int ZX_PROP_VALUES_SEMI_ROW = 103; // Значения в полурядах клавиатуры (8) 103 - 110
constexpr int ZX_PROP_VALUES_KEMPSTON = 111; // Значение для кемпстон-джойстика
constexpr int ZX_PROP_JNI_RETURN_VALUE= 112; // Значение передаваемое из JNI
//constexpr int ZX_PROP_PORT_FEFC       = 116; // Значение передаваемое в порт компаньона
constexpr int ZX_PROP_VALUES_BUTTON   = 322; // Значение для обновления кнопок клавиатуры(текст, иконка) (42 * 2) 322 - 405
constexpr int ZX_PROP_VALUES_SECTOR   = 410; // Массив значений требуемого сектора
constexpr int ZX_PROP_VALUES_TAPE     = 410; // Массив значений требуемого сектора

// 1. Булевы значения
constexpr int ZX_PROP_FIRST_LAUNCH    = 128; // Признак первого запуска
constexpr int ZX_PROP_TRAP_TAPE       = 129; // Признак перехвата загрузки/записи с ленты
constexpr int ZX_PROP_SHOW_JOY        = 130; // Признак отображения джойстика
constexpr int ZX_PROP_SHOW_KEY        = 131; // Признак отображения клавиатуры
constexpr int ZX_PROP_TURBO_MODE      = 132; // Признак турбо-режима процессора
constexpr int ZX_PROP_SND_LAUNCH      = 133; // Признак запуска звукового процессора
constexpr int ZX_PROP_SND_BP          = 134; // Признак запуска бипера
constexpr int ZX_PROP_SND_AY          = 135; // Признак запуска AY
constexpr int ZX_PROP_EXECUTE         = 136; // Признак выполнения программы
constexpr int ZX_PROP_SHOW_HEX        = 137; // Признак 16-тиричного вывода
constexpr int ZX_PROP_BASIC_AUTOSTART = 143; // Признак автостарта бейсик программы при загрузке с ленты

// 2. Байтовые значения
constexpr int ZX_PROP_BORDER_SIZE     = 150; // Размер границы
constexpr int ZX_PROP_MODEL_TYPE      = 151; // Модель памяти
constexpr int ZX_PROP_SND_CHIP_AY     = 152; // Тип звукового сопроцессора
constexpr int ZX_PROP_SND_CHANNEL_AY  = 153; // Раскладка каналов в звуковом процессоре
constexpr int ZX_PROP_SND_FREQUENCY   = 154; // Частота звука
constexpr int ZX_PROP_SND_VOLUME_BP   = 155; // Громкость бипера
constexpr int ZX_PROP_SND_VOLUME_AY   = 156; // Громкость AY
constexpr int ZX_PROP_CPU_SPEED       = 157; // Скорость процессора

// 3. Целые значения
constexpr int ZX_PROP_COLORS          = 170; // значения цветов (16 * 4) 170 - 233

// 4. Значение структур
constexpr int ZX_PROP_BPS             = 192; // значения точек останова (8 * 8) 258 - 321

constexpr int ZX_PROPS_COUNT          = 410; // Размер буфера

// Модели памяти при загрузке *.Z80
constexpr int MODEL_48                = 0; // Синклер 48К
constexpr int MODEL_128               = 1; // Синклер 128К
constexpr int MODEL_PENTAGON          = 2; // Пентагон 256К
constexpr int MODEL_SCORPION          = 3; // Скорпион 256К

// Режимы курсора
constexpr uint8_t MODE_K              = 0;
constexpr uint8_t MODE_L              = 1;
constexpr uint8_t MODE_C              = 2;
constexpr uint8_t MODE_E              = 3;
constexpr uint8_t MODE_SE             = 4;
constexpr uint8_t MODE_SK             = 5;
constexpr uint8_t MODE_CL             = 6;
constexpr uint8_t MODE_CK             = 7;
constexpr uint8_t MODE_G              = 8;
constexpr uint8_t MODE_G1             = 9;
constexpr uint8_t MODE_CE             = 10;

// Варианты форматирования чисел
//constexpr int ZX_FV_CODE_LAST			= 0; // "3X", "2X"
//constexpr int ZX_FV_CODE				= 2; // "3X ", "2X "
constexpr int ZX_FV_PADDR16				= 4; // "5(X)", "4(#X)"
//constexpr int ZX_FV_PADDR8				= 6; // "3(X)", "2(#X)"
constexpr int ZX_FV_OFFS				= 8; // "3+-X)", "2+-#X)"
constexpr int ZX_FV_NUM16				= 10;// "5X", "4X"
constexpr int ZX_FV_OPS16				= 12;// "5X", "4#X"
constexpr int ZX_FV_OPS8				= 14;// "3X", "2#X"
constexpr int ZX_FV_CVAL				= 16;// "5[X]", "4[#X]"
constexpr int ZX_FV_PVAL8				= 18;// "3{X}", "2{#X}"
constexpr int ZX_FV_PVAL16				= 20;// "5{X}", "4{#X}"
constexpr int ZX_FV_NUMBER				= 22;// "0X", "0#X"
//constexpr int ZX_FV_SIMPLE				= 24;// "0X", "0X"

// Команды
constexpr int ZX_CMD_MODEL              = 0; // Установка модели памяти
constexpr int ZX_CMD_PROPS              = 1; // Установка свойств
constexpr int ZX_CMD_RESET              = 2; // Сброс
constexpr int ZX_CMD_UPDATE_KEY         = 3; // Обковление кнопок
constexpr int ZX_CMD_INIT_GL            = 4; // Инициализация GL
constexpr int ZX_CMD_POKE               = 5; // Установка POKE
constexpr int ZX_CMD_ASSEMBLER          = 6; // Ассемблирование
constexpr int ZX_CMD_TAPE_OPS           = 7; // Операции с лентой
constexpr int ZX_CMD_QUICK_BP           = 8; // Быстрая установка точки останова
constexpr int ZX_CMD_TRACE_X            = 9; // Трассировка
constexpr int ZX_CMD_STEP_DEBUG         = 10;// Выполнение в отладчике
constexpr int ZX_CMD_MOVE_PC            = 11;// Выполнение сдвига ПС
constexpr int ZX_CMD_JUMP               = 12;// Получение адреса в памяти/адреса перехода в инструкции
constexpr int ZX_CMD_MAGIC              = 13;// Нажатие на кнопку MAGIC
constexpr int ZX_CMD_DISK_OPS           = 14;// Операции с диском - 0 = get readonly, 1 - Извлечь, 2 - Вставить, 3 - save, 4 - set readonly, 5 - trdos
constexpr int ZX_CMD_QUICK_SAVE         = 15;// Быстрое сохранение
constexpr int ZX_CMD_VALUE_REG          = 16;// Получить адрес из регистра/значения

constexpr int ZX_DISK_OPS_GET_READONLY  = 0; //
constexpr int ZX_DISK_OPS_EJECT         = 1; //
constexpr int ZX_DISK_OPS_OPEN          = 2; //
constexpr int ZX_DISK_OPS_SAVE          = 3; //
constexpr int ZX_DISK_OPS_SET_READONLY  = 4; //
constexpr int ZX_DISK_OPS_TRDOS         = 5; //
constexpr int ZX_DISK_OPS_RSECTOR       = 6; //

constexpr int ZX_TAPE_OPS_COUNT         = 0; //
constexpr int ZX_TAPE_OPS_RESET         = 1; //
constexpr int ZX_TAPE_OPS_BLOCKC        = 2; //
constexpr int ZX_TAPE_OPS_BLOCKP        = 3; //
constexpr int ZX_TAPE_OPS_TRAP_SAVE     = 4; //
constexpr int ZX_TAPE_OPS_TRAP_LOAD     = 5; //

constexpr int ZX_CMD_KEY_MODE_CAPS      = 32; //
constexpr int ZX_CMD_KEY_MODE_SYMBOL    = 64; //

constexpr int ZX_CMD_TRACE_IN           = 0; // Трассировка с заходом
//constexpr int ZX_CMD_TRACE_OUT        = 1; // Трассировка с обходом
constexpr int ZX_CMD_TRACE_OVER         = 2; // Трассировка с выходом

constexpr int ZX_DEBUGGER_MODE_PC       = 0; // Список ДА
constexpr int ZX_DEBUGGER_MODE_SP       = 1; // Список СП
constexpr int ZX_DEBUGGER_MODE_DT       = 2; // Список данных

// Комманды ввода/вывода
constexpr int ZX_CMD_IO_STATE           = 0; // Загрузить/сохранить состояние
constexpr int ZX_CMD_IO_EZX             = 1; //
constexpr int ZX_CMD_IO_Z80             = 2; //
constexpr int ZX_CMD_IO_SNA             = 3; //
constexpr int ZX_CMD_IO_TAP             = 4; //
constexpr int ZX_CMD_IO_TZX             = 5; //
constexpr int ZX_CMD_IO_CSW             = 6; //
constexpr int ZX_CMD_IO_TRD             = 7; //
constexpr int ZX_CMD_IO_SCL             = 8; //
constexpr int ZX_CMD_IO_FDI             = 9;//
constexpr int ZX_CMD_IO_UDI             = 10;//
constexpr int ZX_CMD_IO_TD0             = 11;//

// Система счистления для преобразования строк/чисел
constexpr int RADIX_DEC 				= 0;
constexpr int RADIX_HEX 				= 1;
constexpr int RADIX_BIN 				= 2;
constexpr int RADIX_OCT 				= 3;
constexpr int RADIX_DBL 				= 4;
constexpr int RADIX_FLT 				= 5;
constexpr int RADIX_BOL 				= 6;

#define SAFE_A_DELETE(ptr)              if(ptr) { delete[] ptr; (ptr) = nullptr; }
#define SAFE_DELETE(ptr)                if(ptr) { delete (ptr); (ptr) = nullptr; }
#define modifySTATE(a, r)               { (*zxSpeccy::_STATE) &= ~(r); (*zxSpeccy::_STATE) |= (a); }
#define SWAP_REG(r1, r2)                { auto a = *(r1); auto b = *(r2); *(r1) = b; *(r2) = a; }

inline uint32_t Dword(const uint8_t * ptr)  { return ptr[0] | (uint32_t)(ptr[1]) << 8 | (uint32_t)(ptr[2]) << 16 | (uint32_t)(ptr[3]) << 24; }
inline uint16_t swap_byte_order(uint16_t v) { return (v >> 8) | (v << 8); }
//inline uint16_t SwapWord(uint16_t v)	    { return swap_byte_order(v); }

// число в строку
char* ssh_ntos(void* v, int r, char** end = nullptr);

// строку в число
void* ssh_ston(const char* s, int r, char** end = nullptr);

// число в строку с формированием
char* ssh_fmtValue(int value, int type, bool hex);

bool unpackBlock(uint8_t* ptr, uint8_t* dst, uint8_t* dstE, uint32_t sz, bool packed);

uint8_t* packBlock(uint8_t* src, uint8_t* srcE, uint8_t* dst, bool sign, uint32_t& newSize);

int parseExtension(const char* name);

// вернуть реальный адрес памяти
inline uint8_t* realPtr(uint16_t address) { return &zxDevMem::memPAGES[address >> 14][address & 16383]; }

// проверка на состояние
inline bool checkSTATE(uint8_t state) { return (*zxSpeccy::_STATE & (state)); }

// читаем 8 бит из памяти
inline uint8_t rm8(uint16_t address) { return *realPtr(address); }

// читаем 16 бит из памяти
inline uint16_t rm16(uint16_t address) { return (rm8(address) | (rm8((uint16_t) (address + 1)) << 8)); }

// пишем в память 8 битное значение
inline void wm8(uint8_t* address, uint8_t val) { if(address < zxDevMem::ROMb || address > zxDevMem::ROMe) *address = val; }

// пропуск пробелов
inline void ssh_skip_spc(char** s) {
    auto t = *s; t--;
    while(*++t == ' ') { }
    *s = t;
}

// возвращает количество миллисекунд
inline u_long currentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (u_long)(((tv.tv_sec * 1000) + (tv.tv_usec / 1000)));
}

inline uint8_t* ssh_memset(void* ptr, int set, size_t count) {
    if (ptr) ptr = (uint8_t*) (memset(ptr, set, count)) + count;
    return (uint8_t*) ptr;
}

inline uint8_t* ssh_memzero(void* ptr, size_t count) {
    return ssh_memset(ptr, 0, count);
}

inline uint8_t* ssh_memcpy(uint8_t ** dst, const void* src, size_t count) {
    if (dst && src) {
        *dst = (uint8_t*)(memcpy(*dst, src, count)) + count;
    }
    return dst ? *dst : nullptr;
}

inline char* ssh_char(char** dst, char ch, int count) {
    if (dst && count) {
        auto tmp = *dst;
        while(count-- > 0) *tmp++ = ch;
        *dst = tmp;
    }
    return dst ? *dst : nullptr;
}

inline char* ssh_strcpy(char** dst, const char* src) {
    if (dst && src) {
        auto l = strlen(src);
        *dst = (strcpy(*dst, src) + l);
    }
    return dst ? *dst : nullptr;
}
