#pragma once
#include "common.h"
#include <vector>
typedef uint64_t int64;

template<typename t>
struct vec4 {
    t x;
    t y;
    t z;
    t a;
};
struct Vertex {
    vec4<float> pos;
    vec4<float> color;
    vec4<float> uv;
};

struct Polygon {
    uint unk1[11];
    uint vertexCount;
    uint unk2[13];
    uint format;
    int64 key;
    int64 tex0;
    Vertex verticies[0];
};

class EXPORT SPMFile
{
public:
    SPMFile(char* filepath);
    SPMFile(void* spmfile, int len);

    vector<Polygon*> polygons;
};

