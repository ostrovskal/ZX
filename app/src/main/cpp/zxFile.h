//
// Created by Сергей on 21.11.2019.
//

#pragma once

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "zxCommon.h"

class zxFile final {
public:
    enum Seek {
        begin, current, end
    };
    enum OpenFlags {
        open_read			= 0x000,// r
        open_read_write		= 0x001,// r+
        create_write		= 0x002,// w
        create_read_write	= 0x003,// w+
        append_write		= 0x004,// a
        append_read_write	= 0x005,// a+
        access_temp_remove	= 0x008,// D
        access_temp			= 0x010,// T
        access_random		= 0x020,// R
        access_sequential	= 0x040,// S
        access_enable_deny	= 0x100 // C
    };
    zxFile() : fd(nullptr) {}
    ~zxFile() { close(); }
    // открытие
    bool open(const char* name, int flags);
    // закрытие
    void close() { if(fd) fclose(fd); fd = nullptr; }
    // чтение
    bool read(void* buf, size_t size) const {
        return fread(buf, 1, size, fd) == size;
    }
    // чтение с позиции
    bool read(void* buf, size_t size, size_t pos, int flags) const {
        set_pos(pos, flags);
        return read(buf, size);
    }
    // чтение одного байта
    uint8_t read_byte() { static uint8_t bt; read(&bt, 1); return bt; }
    // чтение одного байта
    void write_byte(uint8_t bt) { write(&bt, 1); }
    // запись
    bool write(void* buf, size_t size) const {
        return fwrite(buf, 1, size, fd) == size;
    }
    // запись по определенной позиции
    bool write(void* buf, size_t size, size_t pos, int flags) const {
        set_pos(pos, flags);
        return write(buf, size);
    }
    // признак открытия
    bool isOpen() const { return fd != nullptr; }
    // сервис
    size_t get_pos() const { return (size_t)ftell(fd); }
    // вернуть длину
    size_t length() const;
    // установить текущую позицию
    size_t set_pos(size_t pos, int flags) const { return (size_t)fseek(fd, (long)pos, flags); }
    // записать файл
    static bool writeFile(const char* path, void* buffer, size_t size, bool isFiles = true);
    // прочитать файл
    static void* readFile(const char* path, void* buffer, bool isFiles = true, size_t* size = nullptr);
    // создать директорию
    static bool makeDir(const char* path) { return mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == 0; }
    // вернуть путь
    static std::string makePath(const char* name, bool isFiles);
protected:
    // дескриптор
    FILE* fd;
};
