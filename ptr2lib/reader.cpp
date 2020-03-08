#include "common.h"

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

int PTR2Reader::fetchFilesize(char* filepath)
{
    PTR2Reader t(filepath);
    if (!t.stream.is_open()) return -1;
    int res = t.filesize;
    t.close();
    return res;
}

void* PTR2Reader::openInMemory(char* filepath, int& len) {
    len = fetchFilesize(filepath);
    if (len == -1) return NULL;
    return openInMemory(filepath);
}
void* PTR2Reader::openInMemory(char* filepath) {
    int len = fetchFilesize(filepath);
    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;
    void* buf = malloc(len);
    fread(buf, 1, len, f);
    return buf;
}