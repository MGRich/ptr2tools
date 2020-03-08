#pragma once
#include <iostream>
#include <fstream>
typedef unsigned char byte;
using namespace std;

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
    
