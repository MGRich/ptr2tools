#pragma once
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#include "dependencies/dirent.h"
#define mkdir(x,y) _mkdir(x)
#endif


struct FileIterator {
    virtual ~FileIterator() {};
    virtual dirent* next() = 0;
};

struct DirIterator : FileIterator {
    DIR* dir;
    struct dirent* de;
    DirIterator(DIR* _dir) : dir(_dir) {};
    ~DirIterator() { closedir(dir); }
    dirent* next();
};

dirent* DirIterator::next() {
    de = readdir(dir);
    if (de == NULL) return NULL;
    return de;
}

bool isfile(const char* fn) {
    struct stat st;
    if (!stat(fn, &st)) {
        return false;
    }
    if (S_ISREG(st.st_mode)) {
        return true;
    }
    else {
        return false;
    }
}

bool direxists(const char* dirname) {
    struct stat st;
    if (stat(dirname, &st) != 0) {
        return false;
    }
    if (S_ISDIR(st.st_mode)) {
        return true;
    }
    else {
        return false;
    }
}

bool makedir(const char* newdir) {
    struct stat st;
    if (direxists(newdir)) return true;
    int err = stat(newdir, &st);
    if (err == -1) {
        if (mkdir(newdir, S_IRWXU) != 0) {
            return false;
        }
    }
    else if (!err) {
        return (S_ISDIR(st.st_mode));
    }
    return true;
}

FileIterator* openiterator(const char* dirname) {
    DIR* dir = opendir(dirname);
    if (NULL == dir) {
        return NULL;
    }
    return new DirIterator(dir);
}
