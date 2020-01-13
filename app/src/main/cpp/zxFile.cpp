//
// Created by Сергей on 21.11.2019.
//

#include "zxCommon.h"
#include "zxFile.h"

bool zxFile::open(const char* name, int flags) {
    close();

    static const char* access[] = {"rb", "r+b", "wb", "w+b", "ab", "a+b"};
    std::string result(access[flags & (open_read | open_read_write | create_write | create_read_write | append_write | append_read_write)]);

    if(flags & access_temp_remove) result += 'D';
    else if(flags & access_temp) result += 'T';
    if(flags & access_random) result += 'R';
    else if(flags & access_sequential) result += 'S';
    if(flags & access_enable_deny) result += 'c';
    else result += 'n';

    return ((fd = fopen(name, result.c_str())) != nullptr);
}

size_t zxFile::length() const {
    auto cur = get_pos(); fseek(fd, 0, end);
    auto length = get_pos();
    set_pos(cur, begin);
    return length;
}

void* zxFile::readFile(const char* path, void* buffer, bool isFiles, size_t* size) {
    zxFile f;
    if(f.open(makePath(path, isFiles).c_str(), zxFile::open_read)) {
        auto sz = f.length();
        if(buffer) {
            f.read(buffer, sz);
        } else {
            buffer = new uint8_t[sz];
            f.read(buffer, sz);
        }
        if(size) *size = sz;
    } else {
        buffer = nullptr;
    }
    return buffer;
}

bool zxFile::writeFile(const char* path, void* buffer, size_t size, bool isFiles) {
    zxFile f;
    if(f.open(makePath(path, isFiles).c_str(), zxFile::create_write)) {
        return f.write(buffer, size);
    }
    return false;
}

std::string zxFile::makePath(const char* name, bool isFiles) {
    return ((isFiles ? FOLDER_FILES : FOLDER_CACHE) + name);
}
