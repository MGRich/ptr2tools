#include <iostream>
#include <fstream>
#include "reader.h"
#include "ptr2int.h"
#include "dependencies/lzss.c"

extern "C" {
    __declspec(dllexport) bool extract(char* filepath, char* outfolder) {
        PTR2Reader reader(filepath);
        if (!reader.headerCheck(0x44332211)) return 1; //this isnt an int file dumbass
        int len = reader.filesize;
        reader.close();

        printf("Storing INT file into memory...\n");
        FILE* f = fopen(filepath, "rb"); //open file
        void* file = malloc(len); //allocate file in memory
        fread(file, 1, len, f); //put file in there

        HeaderData* data = (HeaderData*)file;
        byte buffer[4096];
        char strbuf[256];

        printf("Creating output directory... ");
        printf(makedir(outfolder) ? "worked!\n" : "failed.\n");

        printf("Begin reading INT...\n");
        while (data->resourceType != RESOURCE_END) {
            printf("Begin reading %s (%d files)...\n", typenames[data->resourceType], data->fileCount);

            LZSSHeader* lzss = (LZSSHeader*)((byte*)data + ((long long)data->fnTableOffset + data->fnTableSize)); //janky but gets the data we want
            byte* uncompressedData = (byte*)(malloc(lzss->uncompressedSize));
            lzss_decompress(12, 4, 2, 2, buffer, lzss->data, lzss->compressedSize, uncompressedData, lzss->uncompressedSize);

            FilenameEntry* entries = (FilenameEntry*)((byte*)data + data->fnTableOffset);
            uint* offsets = (uint*)(data + 1); //WHAT LOL OK THEN
            char* filenames = (char*)(entries + data->fileCount);

            snprintf(strbuf, sizeof(strbuf), "%s/%s", outfolder, typenames[data->resourceType]);
            makedir(strbuf);

            snprintf(strbuf, sizeof(strbuf), "%s/%s/_order.txt", outfolder, typenames[data->resourceType]);
            FILE* orderfile = fopen(strbuf, "w"); 
            for (int i = 0; i < data->fileCount; i++) {
                FilenameEntry entry = entries[i];
                const char* fname = filenames + entry.offset;

                snprintf(strbuf, sizeof(strbuf), "%s/%s/%s", outfolder, typenames[data->resourceType], fname);
                printf("Write file %s... ", fname);
                FILE* outfile = fopen(strbuf, "wb");
                if (!outfile) {
                    printf("FAILED.\n");
                    continue;
                }

                fwrite(uncompressedData + offsets[i], 1, entry.fileSize, outfile);
                fclose(outfile);
                fprintf(orderfile, "%s\n", fname);
                printf("success!\n");
            }
            fclose(orderfile);
            data = (HeaderData*)((byte*)lzss + data->lzssSize);
            free(uncompressedData);
        }
        printf("File extracted!\n");
        return 0;
    }


}