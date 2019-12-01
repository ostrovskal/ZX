//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <limits.h>
#include <cstring>
#include <stdarg.h>
#include <cstdlib>
#include <cstdint>
#include <string>

#include "zxFile.h"
#include "zxCPU.h"
#include "zxALU.h"

// Глобальные
extern zxALU* 				            ALU;
extern uint8_t* 			            opts;
extern uint8_t* 			            TMP_BUF;
extern std::string 			            FOLDER_FILES;
extern std::string 			            FOLDER_CACHE;

constexpr int ZX_SIZE_TMP_BUF           = 524288;
constexpr const char* ZX_AUTO_SAVE      = "auto_save.zx";

// Биты состояний
enum ZX_STATE {
    ZX_INT 	= 0x01,// прерывание
    ZX_NMI 	= 0x02,//
    ZX_HALT = 0x04,// останов. ждет прерывания
    ZX_TRDOS= 0x08 // режим диска
};

// Позиция ПЗУ различных моделей
constexpr int ZX_ROM_K48K				= 0;
constexpr int ZX_ROM_48K				= 16384;
constexpr int ZX_ROM_128K				= 32768;
constexpr int ZX_ROM_PENTAGON			= 65536;
constexpr int ZX_ROM_SCORPION			= 98304;
constexpr int ZX_ROM_TRDOS              = 163840;

// Разделяемые свойства
// 1. Булевы значения
constexpr int ZX_PROP_FIRST_LAUNCH    = 128; // Признак первого запуска
constexpr int ZX_PROP_TRAP_TAPE       = 129; // Признак перехвата загрузки/записи с ленты
constexpr int ZX_PROP_SHOW_JOY        = 130; // Признак отображения джойстика
constexpr int ZX_PROP_SHOW_KEY        = 131; // Признак отображения клавиатуры
constexpr int ZX_PROP_SHOW_FPS        = 132; // Признак отображения FPS
constexpr int ZX_PROP_TURBO_MODE      = 133; // Признак турбо-режима процессора
constexpr int ZX_PROP_SND_LAUNCH      = 134; // Признак запуска звукового процессора
constexpr int ZX_PROP_SND_BP          = 135; // Признак запуска бипера
constexpr int ZX_PROP_SND_AY          = 136; // Признак запуска AY
constexpr int ZX_PROP_SND_8BIT        = 137; // Признак 8 битного звука
constexpr int ZX_PROP_SND_SAVE        = 138; // Признак прямой записи
constexpr int ZX_PROP_SKIP_FRAMES     = 139; // Признак пропуска кадров при отображений
constexpr int ZX_PROP_EXECUTE         = 140; // Признак выполнения программы

// 2. Байтовые значения
constexpr int ZX_PROP_ACTIVE_DISK     = 150; // Номер активного диска
constexpr int ZX_PROP_BORDER_SIZE     = 151; // Размер границы
constexpr int ZX_PROP_MODEL_TYPE      = 152; // Модель памяти
constexpr int ZX_PROP_SND_TYPE_AY     = 153; // Раскладка каналов в звуковом процессоре AY
constexpr int ZX_PROP_SND_FREQUENCY   = 154; // Частота звука
constexpr int ZX_PROP_SND_VOLUME_BP   = 155; // Громкость бипера
constexpr int ZX_PROP_SND_VOLUME_AY   = 156; // Громкость AY
constexpr int ZX_PROP_BLINK_SPEED     = 157; // Скорость мерцания курсора
constexpr int ZX_PROP_CPU_SPEED       = 158; // Скорость процессора
constexpr int ZX_PROP_KEY_SIZE        = 159; // Размер экранной клавиатуры
constexpr int ZX_PROP_JOY_SIZE        = 160; // Размер экранного джойстика

// 3. Целые значения
constexpr int ZX_PROP_COLORS          = 170; // значения цветов (16 * 4) 170 - 233

// 4. Байтовые значения, вычисляемые во время работы программы
constexpr int ZX_PROP_JOY_TYPE        = 256; // Текущий тип джойстика
constexpr int ZX_PROP_JOY_KEYS        = 257; // Привазанные к джойстику коды кнопок клавиатуры (8) 257 - 264
constexpr int ZX_PROP_JOY_CROSS_VALUE = 265; // Нажатые кнопки джойстика-крестовины
constexpr int ZX_PROP_JOY_ACTION_VALUE= 266; // Нажатые кнопки джойстика-управления
constexpr int ZX_PROP_KEY_CURSOR_MODE = 267; // Режим курсора (E, G, L, K т.п.)
constexpr int ZX_PROP_KEY_MODE        = 268; // Режим клавиатуры (CAPS LOCK, SYMBOL SHIFT)
constexpr int ZX_PROP_VALUES_SEMI_ROW = 269; // Значения в полурядах клавиатуры (8) 269 - 276
constexpr int ZX_PROP_VALUES_KEMPSTON = 277; // Значение для кемпстон-джойстика

constexpr int ZX_PROPS_COUNT          = 300; // Размер буфера

// Модели памяти
constexpr int MODEL_48KK              = 0; // Компаньон 2.02 48К
constexpr int MODEL_48K               = 1; // Синклер 48К
constexpr int MODEL_128K              = 2; // Синклер 128К
constexpr int MODEL_PENTAGON          = 3; // Пентагон 128К
constexpr int MODEL_SCORPION          = 4; // Скорпион 256К

// Режимы курсора
constexpr uint8_t MODE_K              = 0;
constexpr uint8_t MODE_L              = 1;
constexpr uint8_t MODE_C              = 2;
constexpr uint8_t MODE_E              = 3;
constexpr uint8_t MODE_E1             = 4;
constexpr uint8_t MODE_E2             = 5;
constexpr uint8_t MODE_CL             = 6;
constexpr uint8_t MODE_CK             = 7;
constexpr uint8_t MODE_G              = 8;
constexpr uint8_t MODE_G1             = 9;

// Команды
constexpr int ZX_CMD_MODEL              = 0; // Установка модели памяти
constexpr int ZX_CMD_PROPS              = 1; // Установка свойств
constexpr int ZX_CMD_RESET              = 2; // Сброс
constexpr int ZX_CMD_UPDATE_KEY         = 3; // Обковление кнопок
constexpr int ZX_CMD_PRESETS            = 4; // Установка/получение пресетов джойстика
constexpr int ZX_CMD_POKE               = 5; // Установка POKE
constexpr int ZX_CMD_DIAG               = 6; // Диагностика
constexpr int ZX_CMD_PRESETS_SAVE       = 7; // Сохранить параметры джойстика
constexpr int ZX_CMD_PRESETS_LOAD       = 8; // Загрузить параметры джойстика
constexpr int ZX_CMD_PRESETS_LIST       = 9; // Получить список пресетов
constexpr int ZX_CMD_PRESETS_NAME       = 10;// Получить имя программы
constexpr int ZX_CMD_PRESETS_SET        = 11;// Установить имя программы

constexpr int ZX_CMD_KEY_MODE_CAPS      = 32; //
constexpr int ZX_CMD_KEY_MODE_SYMBOL    = 64; //

// Комманды ввода/вывода
constexpr int ZX_CMD_IO_STATE           = 0; // Загрузить/сохранить состояние
constexpr int ZX_CMD_IO_SCREEN          = 1; // Загрузить/сохранить экран
constexpr int ZX_CMD_IO_Z80             = 2; // Загрузить/сохранить снимок памяти
constexpr int ZX_CMD_IO_TAPE            = 3; // Загрузить/сохранить ленту
constexpr int ZX_CMD_IO_WAVE            = 4; // Загрузить/сохранить звук
constexpr int ZX_CMD_IO_TRD             = 5; // Загрузить/сохранить образ диска

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
#define modifySTATE(a, r)               { (*zxALU::_STATE) &= ~r; (*zxALU::_STATE) |= a; }
#define SWAP_REG(r1, r2)                { auto a = *(r1); auto b = *(r2); *(r1) = b; *(r2) = a; }

//
void info(const char* msg, ...);

const char** ssh_split(const char* str, const char* delim, int* count = nullptr);

char* ssh_ntos(void* v, int r, char** end = nullptr);

void* ssh_ston(const char* s, int r, const char** end = nullptr);

bool unpackBlock(uint8_t* ptr, uint8_t* dst, uint8_t* dstE, uint32_t sz, bool packed, bool sign);

uint8_t* packBlock(uint8_t* src, uint8_t* srcE, uint8_t* dst, bool sign, uint32_t& newSize);

uint8_t* realPtr(uint16_t address);

// вычисление полупереноса
inline uint8_t calcFH(uint8_t op1, uint8_t op2, uint8_t fc, uint8_t fn) {
    auto v = (uint8_t)(op1 & 15);
    auto o = (uint16_t)(op2 & 15) + fc;
    auto ret = (fn ? v - o : v + o);
    return (uint8_t) (ret > 15) << 4;
}

// читаем 8 бит из памяти
inline uint8_t rm8(uint16_t address) { return *realPtr(address); }

// читаем 16 бит из памяти
inline uint16_t rm16(uint16_t address) { return (rm8(address) | (rm8((uint16_t) (address + 1)) << 8)); }

// пишем в память 8 битное значение
inline void wm8(uint8_t* address, uint8_t val) {
    if (address >= ALU->ROMS && address < &ALU->ROMS[180224]) {
        //log_info("запись в ПЗУ (PC: %i ADDRESS: %i VALUE: %i)", *zxCPU::_PC - 1, address - zxALU::pageROM, val);
        return;
    }
    *address = val;
}

inline uint8_t* ssh_memset(void* ptr, int set, size_t count) {
    if (ptr) ptr = (uint8_t*) (memset(ptr, set, count)) + count;
    return (uint8_t*) ptr;
}

inline uint8_t* ssh_memzero(void* ptr, size_t count) {
    return ssh_memset(ptr, 0, count);
}

inline uint8_t* ssh_memcpy(void* dst, const void* src, size_t count) {
    if (dst && src) dst = (uint8_t*) (memcpy(dst, src, count)) + count;
    return (uint8_t*) dst;
}

inline char* ssh_strcpy(char* dst, const char* src) {
    auto l = strlen(src);
    if (dst && src) dst = (strcpy(dst, src) + l);
    return dst;
}
