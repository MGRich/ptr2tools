#pragma once
//#include "common.h"
#include <vector>
#include <iostream>

using namespace std;

typedef unsigned char byte;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define ALIGN(N, S) ((((N) + (S) - 1) / (S)) * (S))

#ifdef _WIN32
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef ISOEXPORT
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#define EXPORTNC __attribute__ ((dllexport))
#else
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#define EXPORTNC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define EXPORTNC __attribute__ ((dllimport))
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#else
#define EXPORTNC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif
#endif
#endif
#define NOEXPORT


struct DateTime {
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char milli[2];
    u8 offset;
};
struct DirectoryDateTime {
    u8 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 offset;
};

PACK(
    extern "C" struct EXPORTNC RootDirectory {
        byte len;
        byte extLen;
        u32 lba;
        u32 lbaMSB;
        u32 fileLength;
        u32 fileLengthMSB;
        DirectoryDateTime timestamp;
        byte flags;
        byte unitSize;
        byte interleaveSize;
        u16 volume;
        u16 volumeMSB;
        byte filenameLength;
});
extern "C" struct EXPORTNC ISODirectory : RootDirectory {
    char filename[0];
};

struct PathTableEntry {
    byte len;
    byte extLen;
    u32 lba;
    u16 dirNumber;
    char filename[0];
};

struct VolumeDescriptor {
    byte type = 0x01;
    char id[5] = {'C', 'D', '0', '0', '1'};
    byte version = 0x01;
};
PACK(
extern "C" struct EXPORTNC PrimaryDescriptor : VolumeDescriptor {
    byte unused1;

    char sysID[32];
    char volID[32];
    byte unused2[8];

    u32 volSpaceSize;
    u32 volSpaceSizeMSB;
    byte unused3[32];
    u16 volSetSize;
    u16 volSetSizeMSB;

    u16 volSeqNumber;
    u16 volSeqNumberMSB;

    u16 sectorSize;
    u16 sectorSizeMSB;

    u32 pathTableSize;
    u32 pathTableSizeMSB;
    u32 pathTableSector;
    u32 pathTableOpt;
    u32 pathTableSectorMSB;
    u32 pathTableOptMSB;

    RootDirectory root;

    char volSetID[128];
    char publisher[128];
    char dataPreparer[128];
    char appID[128];
    char copyrightFile[38];
    char abstractFile[36];
    char bibliographicFile[37];

    DateTime creation;
    DateTime lastModified;
    DateTime expiration;
    DateTime effective;

    byte unused4;
});

///ISO9660///
extern "C" {
    class EXPORTNC ISO9660
    {
    public:
        ISO9660(char* filepath, bool make = false);
        void readSectors(void* buf, int secCount);
        void seekSector(int sector);
        int tellSector();
        void writeSector(void* buf, int secCount);

        void* findFile(const char* filepath, int &len);
        int writeFile(void* buf, char* filepath, int len, bool push = true);
        ISODirectory getDir(const char* dirpath);

        static bool isoname(char* filename, char type = 'd');
        void close();
        PrimaryDescriptor info;
        int sectorSize;
    private:
        FILE* file;
        int infosec;
        bool onBigEndian;
    };
}