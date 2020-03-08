#pragma once
#include <iostream>
#include <fstream>
typedef unsigned char byte;
typedef unsigned int uint;

using namespace std;

//borrowed from https://stackoverflow.com/questions/2164827
#if defined(_MSC_VER)
//  Microsoft 
#define EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
//  GCC
#define EXPORT __attribute__((visibility("default")))
#else
//  do nothing and hope for the best?
#define EXPORT
#pragma warning Unknown dynamic link import/export semantics.
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

    static int fetchFilesize(char* filepath);
    int filesize;
    ifstream stream;
};
    
