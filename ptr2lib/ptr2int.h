#pragma once

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#include "dependencies/dirent.h"
#define mkdir(x,y) _mkdir(x)
#endif

#define ALIGN(x, y) (((x) + (y-1)) & (~((y)-1)))

typedef unsigned int uint;

struct TableOffsets {
    uint hdr;
    uint offsets;
    uint filenames;
    uint characters;
    uint lzss;
    uint end;
};
struct HeaderData {
    uint magic;
    uint fileCount;
    uint resourceType;
    uint fnTableOffset;
    uint fnTableSize;
    uint lzssSize;
    uint unk[2];
};
struct LZSSHeader {
    uint uncompressedSize;
    uint compressedSize;
    byte data[0];
};
struct FilenameEntry {
    uint offset;
    uint filesize;
};

struct INTFile {
    INTFile(std::string fn, int filesize, int fdp);
    std::string filename;
    int filesize;
    int offset;
};

INTFile::INTFile(std::string fn, int filesize, int fdp) :
    filename(fn), filesize(filesize), offset(fdp) {}

struct OffsetTable {
    uint header = 0;
    uint offsets = sizeof(HeaderData);
    uint filenameTable;
    uint filenames;
    uint lzss;
    uint end;
};

struct FileIterator {
    virtual ~FileIterator() {};
    virtual const char* next() = 0;
};

struct DirIterator : FileIterator {
    DIR* dir;
    struct dirent* de;
    DirIterator(DIR* _dir) : dir(_dir) {};
    ~DirIterator() { closedir(dir); }
    const char* next();
};

const char* DirIterator::next() {
    de = readdir(dir);
    if (de == NULL) return NULL;
    return de->d_name;
}

struct OrderIterator : FileIterator {
    FILE* orderfile;
    char buf[256];
    OrderIterator(FILE* f) : orderfile(f) {}
    ~OrderIterator() { this->close(); }
    const char* next();
    void close() { if (NULL != orderfile) { fclose(orderfile); orderfile = NULL; } return; }
};

const char* OrderIterator::next() {
    buf[0] = char(0);
    while (!feof(orderfile)) {
        fscanf(orderfile, "%s\n", buf);
        if (buf[0] == char(0)) continue;
        return buf;
    }
    return NULL;
}

bool isfile(const char* fn) {
    struct stat st;
    if (stat(fn, &st) != 0) {
        return false;
    }
    if (S_ISREG(st.st_mode)) {
        return true;
    }
    else {
        return false;
    }
}


FileIterator* openiterator(const char* dirname) {
    char lbuf[256];
    snprintf(lbuf, sizeof(lbuf), "%s/_order.txt", dirname);
    if (isfile(lbuf)) {
        FILE* orderfile = fopen(lbuf, "r");
        if (NULL != orderfile) {
            return new OrderIterator(orderfile);
        }
    }
    DIR* dir = opendir(dirname);
    if (NULL == dir) {
        return NULL;
    }
    return new DirIterator(dir);
}


enum {
    RESOURCE_END,
    RESOURCE_TM0,
    RESOURCE_SFX,
    RESOURCE_STAGE,
    RESOURCE_RED,
    RESOURCE_BLUE,
    RESOURCE_PINK,
    RESOURCE_YELLOW
};

const HeaderData nullhdr = {
    0x44332211, 0, 0, 0x20, 0, 0, {0,0}
};

const char* typenames[8] = {
    "END",
    "Textures",
    "SFX",
    "Props",
    "Red Hat",
    "Blue Hat",
    "Pink Hat",
    "Yellow Hat"
};

const char* oldnames[8] = {
    "END",
    "TEXTURES",
    "SOUNDS",
    "PROPS",
    "HAT_RED",
    "HAT_BLUE",
    "HAT_PINK",
    "HAT_YELL"
};


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
