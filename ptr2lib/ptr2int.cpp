#include <iostream>
#include <fstream>
#include <vector>
#include "reader.h"
#include "ptr2int.h"
#include "dependencies/lzss.c"

extern "C" {
    __declspec(dllexport) int intextract(char* filepath, char* outfolder) {
        PTR2Reader reader(filepath);
        printf("Checking if file exists and is valid... ");
        if (!reader.stream.is_open() || !reader.headerCheck(0x44332211)) {
            printf("no! Please pass a valid INT file.\n");
            return 1; //this isnt an int file dumbass
        }
        printf("yes.\n");
        int len = reader.filesize;
        reader.close();

        printf("Storing INT file into memory...\n");
        FILE* f = fopen(filepath, "rb"); //open file
        void* file = malloc(len); //allocate file in memory
        if (!file) {
            printf("Error allocating! This should NEVER happen. If it does, contact me.");
            return 2;
        }
        fread(file, 1, len, f); //put file in there

        HeaderData* data = (HeaderData*)file;
        byte buffer[4096];
        char strbuf[256];

        printf("Creating output directory... ");
        printf(makedir(outfolder) ? "worked!\n" : "failed.\n");

        printf("Begin reading INT...\n");
        while (data->resourceType != RESOURCE_END) {
            memset(buffer, 0, sizeof(buffer));
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

                fwrite(uncompressedData + offsets[i], 1, entry.filesize, outfile);
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

    __declspec(dllexport) int intlist(char* filepath) {
        PTR2Reader reader(filepath);
        printf("Checking if file exists and is valid... ");
        if (!reader.stream.is_open() || !reader.headerCheck(0x44332211)) {
            printf("no! Please pass a valid INT file.\n");
            return 1; //this isnt an int file dumbass
        }
        printf("yes.\n");
        int len = reader.filesize;
        reader.close();

        printf("Storing INT file into memory...\n");
        FILE* f = fopen(filepath, "rb"); //open file
        void* file = malloc(len); //allocate file in memory
        if (!file) {
            printf("Error allocating! This should NEVER happen. If it does, contact me.");
            return 2;
        }
        fread(file, 1, len, f); //put file in there

        HeaderData* data = (HeaderData*)file;
        while (data->resourceType != RESOURCE_END) {
            FilenameEntry* entries = (FilenameEntry*)((byte*)data + data->fnTableOffset);
            char* filenames = (char*)(entries + data->fileCount);
            printf("Reading section %s.\n", typenames[data->resourceType]);
            for (int i = 0; i < data->fileCount; i++) {
                printf("- %s - %d bytes\n", filenames + entries[i].offset, entries[i].filesize); //list name - size
            }
            data = (HeaderData*)((byte*)((byte*)data + ((long long)data->fnTableOffset + data->fnTableSize)) + data->lzssSize);
        }
        printf("Listing complete!\n");
        return 1;
    }

    OffsetTable getOffsets(std::vector<INTFile> intfiles, int lzssSize) {
        OffsetTable r;
        r.offsets = r.header + sizeof(HeaderData);
        r.filenameTable = ALIGN((r.offsets + (sizeof(int) * (intfiles.size() + 1))), 0x10);
        r.filenames = r.filenameTable + (sizeof(FilenameEntry) * intfiles.size());
        uint fnsize = 0;
        for (int i = 0; i < intfiles.size(); i++) {
            fnsize += intfiles[i].filename.length() + 1;
        }
        r.lzss = ALIGN(r.filenames + fnsize, 0x800);
        r.end = ALIGN((r.lzss + lzssSize), 0x800);
        return r;
    }

    int pad_folderdata(byte* folderdata, int start, int end) {
        int remain = end - start;
        while (remain > 0) {
            //TODO: Repeating last few bytes of file might help compression
            //      instead of zero-filling. The official INT packer 
            //      zero-fills.
            //good for you posesix cause i am doing that fuck you!

            folderdata[end - remain] = 0; remain--;
        }
        return (end - start);
    }

    void build_int_section(int restype, OffsetTable r, std::vector<INTFile>& intfiles, const byte* lzss, int lzss_size, byte* out) {
        HeaderData* hdr = (HeaderData*)(out + r.header); //establish a header
        uint* offsets = (uint*)(out + r.offsets); //pointer of u32s for offsets
        FilenameEntry* fentries = (FilenameEntry*)(out + r.filenameTable); //pointer of filenames entries
        char* characters = (char*)(out + r.filenames);
        LZSSHeader* lzss_sec = (LZSSHeader*)(out + r.lzss); //get the lzss header for the section

        memset(out, 0, r.end); //set out to be all 0s

        hdr->unk[0] = 0; //what
        hdr->unk[1] = 0; //what
        hdr->lzssSize = r.end - r.lzss; //set the section size
        hdr->resourceType = restype; //set resource type
        hdr->fnTableSize = r.lzss - r.filenameTable; //get the table size
        hdr->magic = 0x44332211; //0x44332211 as usual
        hdr->fnTableOffset = r.filenameTable; //get offset from table
        hdr->fileCount = intfiles.size(); //get # of files

        uint offset = 0;
        uint fnoffs = 0; //set offset and fnoffs to 0
        for (uint i = 0; i < intfiles.size(); i += 1) { //for each file..
            INTFile& intfile = intfiles[i];
            printf("- %s, %d bytes\n", intfile.filename.c_str(), intfile.filesize);
            offsets[i] = offset; //set the offset
            fentries[i].offset = fnoffs; //set the filename offset
            fentries[i].filesize = intfile.filesize; //set the size of the file
            size_t fnlen = intfile.filename.length() + 1; //set length of filename + 1 null terminator
            const char* cstr = intfile.filename.c_str(); //get cstring of filename
            memcpy(characters + fnoffs, cstr, fnlen); //copy the cstring to characters + file 

            offset += ALIGN(intfile.filesize, 0x10); //add to the offset using align
            fnoffs += fnlen; //add to the filename offsets with fnlen
        }
        offsets[intfiles.size()] = offset; //set the new offset

        lzss_sec->uncompressedSize = offset; //uncompressed size is offset
        lzss_sec->compressedSize = lzss_size; //compressed size is the lzss size
        memcpy(lzss_sec->data, lzss, lzss_size); //copy lzss data to header data

        return;
    }

    __declspec(dllexport) int intcreate(char* intpath, char* infolder) {
        printf("Checking for directory... ");
        if (!direxists(infolder)) {
            printf("failed. Please make sure your directory exists.\n");
            return 1;
        }
        printf("success.\n");

        PTR2Reader t(intpath);
        if (t.stream.is_open()) {
            while (true) {
                printf("File %s exists! Are you sure you want to overwrite it? (Y/N) ", intpath);
                char ch;
                scanf("%c", &ch);
                if (ch == 'y' || ch == 'Y') break;
                if (ch == 'n' || ch == 'N') return 0;
            }
        }
        t.close();
        printf("Opening %s... ", intpath);
        FILE* out = fopen(intpath, "wb");
        if (!out) {
            printf("failed.\n");
            return 1;
        }
        printf("worked.\n");

        char strbuf[4096];
        std::vector<INTFile> intfiles;
        byte buffer[4096*2];
        for (int i = 1; i < 8; i++) {
            const char* nameused = typenames[i];
            snprintf(strbuf, 4096, "%s/%s", infolder, nameused);
            printf("Checking for %s... ", nameused);
            if (!direxists(strbuf)) {
                nameused = oldnames[i];
                printf("failed. Checking for %s... ", nameused);
                snprintf(strbuf, 4096, "%s/%s", infolder, nameused);
                if (!direxists(strbuf)) {
                    printf("failed. %s will not be put into the INT.\n", typenames[i]);
                    continue;
                }
            }
            printf("success.\n");
            intfiles.clear();

            printf("Opening iterator... ");
            FileIterator* iter = openiterator(strbuf);
            if (!iter) {
                printf("failed.\n");
                continue;
            }
            printf("success.\n");
            int folderlen = 0;
            byte* folderdata = (byte*)(malloc(4));
            const char* filename;
            while ((filename = iter->next()) != NULL) { //smart, i like it
                snprintf(strbuf, 4096, "%s/%s/%s", infolder, nameused, filename);
                printf("Check for %s...", filename);
                if (!isfile(strbuf)) {
                    printf("failed. File will be skipped.\n");
                    continue;
                }
                printf("success. Opening... ");
                int len = PTR2Reader::fetchFilesize(strbuf);
                FILE* f = fopen(strbuf, "rb");
                if (!f || len == -1) {
                    printf("failed. File will be skipped.\n");
                    continue;
                }
                printf("success.\n");
                int fdp = folderlen;
                folderlen = ALIGN(folderlen + len, 0x10);
                folderdata = (byte*)(realloc(folderdata, folderlen));
                fread(folderdata + fdp, 1, len, f);
                pad_folderdata(folderdata, fdp + len, folderlen);
                fclose(f);
                intfiles.push_back(INTFile(std::string(filename), len, fdp));
            }
            if (!folderlen) {
                printf("Folder length is 0. Check your _order.txt, this shouldn't happen.\n");
                continue;
            }

            printf("Calculating compression of %d bytes... ", folderlen);
            memset(buffer, 0, 4096 * 2);
            int complen = lzss_compress(12, 4, 2, 2, buffer, folderdata, folderlen, NULL);
            printf("into %d bytes. Compressing data...", complen);
            
            memset(buffer, 0, 4096 * 2);
            byte* lzssData = (byte*)(malloc(complen));
            lzss_compress(12, 4, 2, 2, buffer, folderdata, folderlen, lzssData);
            free(folderdata); //free the data

            printf("\nBuilding %s section with", nameused);
            OffsetTable offsets = getOffsets(intfiles, complen);
            int seclen = offsets.end;
            printf(" %d bytes...\n", seclen);

            byte* secdata = (byte*)(malloc(seclen)); //allocate that amount
            build_int_section(i, offsets, intfiles, lzssData, complen, secdata); //build it

            printf("Writing...\n");
            fwrite(secdata, 1, seclen, out); //write the sectiondata to the file

            //free gives a heap corruption error, however, both since this takes 
            //up like 43 MB of memory and it seems to self-deconstruct secdata, we're fine skipping it

            //free(secdata)
        }

        printf("Writing null header...\n");
        fwrite(&nullhdr, sizeof(nullhdr), 1, out);

        fclose(out);
        printf("Done.\n");
        return 0;

    }

}