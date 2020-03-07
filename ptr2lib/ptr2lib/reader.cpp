#include "reader.h"

PTR2Reader::PTR2Reader(char* filepath)
{
    stream = ifstream(filepath, ios::binary | ios::ate);
    filesize = stream.tellg();
    stream.seekg(0);
}

int PTR2Reader::readInt()
{
    int t;
    stream.read((char*)&t, 4);
    return t;
}

byte PTR2Reader::readByte() {
    byte t;
    stream.read((char*)&t, 1);
    return t;
}

byte* PTR2Reader::readByteArray(int count) {
    byte* t = (byte*)malloc(count);
    stream.read((char*)t, count);
    return t;
}

bool PTR2Reader::headerCheck(int header) {
    return (readInt() == header);
}

void PTR2Reader::close() {
    return stream.close();
    delete this;
}