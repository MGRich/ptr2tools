#pragma once

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#define S_ISDIR(m) (((m) & _S_IFDIR) == _S_IFDIR)
#endif

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
    uint fileSize;
};

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
