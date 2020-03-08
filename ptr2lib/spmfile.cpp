#include "spmfile.h"

SPMFile::SPMFile(char* filepath)
{
    int len;
    void* file = PTR2Reader::openInMemory(filepath, len);
    SPMFile(file, len);
}

SPMFile::SPMFile(void* file, int len)
{
    byte* bytes = (byte*)file;
    if (*(int*)(file) != 0x18DF540A) return;
    int polcount = 0;
    for (int i = 8; (i + 8) <= len; i += 0x10) {
        int64 v = *(int64*)(bytes + i);
        if (v == 0xEEEEEEEEEEEEEEEE) {
            polygons.push_back((Polygon*)(bytes + i - 0x68));
        }
    }
}

