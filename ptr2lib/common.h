#pragma once
#include <iostream>
#include <fstream>
typedef unsigned char byte;
typedef unsigned int uint;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

using namespace std;

// Define EXPORT for any platform
#if defined _WIN32 || defined __CYGWIN__
#ifdef WIN_EXPORT
  // Exporting...
#ifdef __GNUC__
#define EXPORT extern "C" __attribute__ ((dllexport))
#else
#define EXPORT extern "C" __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define EXPORT extern "C" __attribute__ ((dllimport))
#else
#define EXPORT extern "C" __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define NOEXPORT
#else
#if __GNUC__ >= 4
#define EXPORT extern "C" __attribute__ ((visibility ("default")))
#define NOEXPORT __attribute__ ((visibility ("hidden")))
#else
#define EXPORT extern "C"
#define NOEXPORT
#endif
#endif
class PTR2Reader
{
    public:
    PTR2Reader(char* filepath);
    
    int readInt();
    byte readByte();
    byte* readByteArray(int count);
    bool headerCheck(int header);
    void close();

    static void* openInMemory(char* filepath);
    static void* openInMemory(char* filepath, int& len);
    static int fetchFilesize(char* filepath);
    int filesize;
    ifstream stream;
};
    
