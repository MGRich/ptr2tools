#include "ptr2spm.h"
#include "common.h"
#include "spmfile.h"
#ifdef _WIN32
#define PRIx64 "llx"
#define PRIx8 "hhx"
#endif
typedef uint64_t int64;

extern "C" {

    EXPORT int spmanalyze(char* filepath) {
        PTR2Reader reader(filepath);
        printf("Checking if file exists and is valid... ");
        if (!reader.stream.is_open() 
        || !reader.headerCheck(0x18DF540A)) {
            printf("no! Please pass a valid SPM file.\n");
            return 1; //this isnt an int file dumbass
        }
        printf("yes.\n");
        reader.close();

        int len;
        void* tmp = PTR2Reader::openInMemory(filepath, len);

        SPMFile *file = new SPMFile(tmp, len);

        for (int i = 0; i < file->polygons.size(); i++) {
            printf("Polygon %d:\n", i);
            Polygon* poly = file->polygons[i];
            for (int v = 0; v < poly->vertexCount; v++) {
                Vertex* vert = &poly->verticies[v];
                printf("- Vertex %d\n", v);
                printf("- * POSITION: %f %f %f\n", vert->pos.x, vert->pos.y, vert->pos.z);
                printf("- * COLOR:    %f %f %f %f\n", vert->color.x, vert->color.y, vert->color.z, vert->color.a);
                printf("- * UV:       %f %f %f\n", vert->uv.x, vert->uv.y, vert->uv.z);
            }
        }
    }
    __declspec(dllexport) int spmgettex0(char* filepath, char* outfile) {
        PTR2Reader reader(filepath);
        printf("Checking if file exists and is valid... ");
        if (!reader.stream.is_open() || !reader.headerCheck(0x18DF540A)) {
            printf("no! Please pass a valid SPM file.\n");
            return 1; //this isnt an int file dumbass
        }
        printf("yes.\n");
        reader.close();

        printf("Storing SPM file into memory...\n");
        int len;
        void* file = PTR2Reader::openInMemory(filepath, len);
        if (!file) {
            printf("Error allocating! This should NEVER happen. If it does, contact me.");
            return 2;
        }
        int polcount = 0;
        for (int i = 8; (i + 8) <= len; i += 0x10) {
            int64 v = *(int64*)((byte*)file + i);
            if (v == 0xEEEEEEEEEEEEEEEE) {

            }
        }
    }
}