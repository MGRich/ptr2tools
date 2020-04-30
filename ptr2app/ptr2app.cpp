// ptr2app.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
//#include "ptr2lib.h"
#include "iso9660.h"
#include <string> 
#include <vector>
#include "isomod.h"

//#define bitflip(data) ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) | ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24)  


int main(int argc, char* argv[]) {
    
}//*/

//ISOMOD
/*using namespace std;
int main(int argc, char* argv[]) {
    auto inArgs = [argc, argv](std::string arg, int i = -1) {
        if (i == -1)
            for (int i = 1; i < argc; i++) {
                if (!strcmp(argv[i], arg.c_str())) {
                    return true;
                }
            }
        else return (!strcmp(argv[i + 1], arg.c_str()));
        return false;
    };

    if (inArgs("put", 0)) {
        ISO9660 iso(argv[2]);
        if (iso.sectorSize == -1) {
            printf("Error reading ISO file!\n");
            return 1;
        }

        if (argc == 5) {
            FILE* file = fopen(argv[4], "rb");
            if (!file) {
                printf("Error reading given file.\n");
                return 1;
            }
            fseek(file, 0, SEEK_END);
            int flen = ftell(file);
            byte* buf = (byte*)malloc(flen);
            fseek(file, 0, SEEK_SET);
            fread(buf, 1, flen, file);
            fclose(file);
            int len;
            if (!iso.findFile(argv[3], len)) {
                printf("Error finding file on ISO file.\n");
                return 1;
            }
            printf("%d bytes (on ISO) to %d bytes (from file given).\n", len, flen);
            iso.writeFile(buf, argv[3], flen);
            printf("Done.");
            return 0;
        }
        else if (argc == 4) {
            printf("If only one file was changed, it should be completely safe to CTRL+C on when you see the file change.\n");
            int subdir = 0;
            vector<FileIterator*> iters;
            vector<char*> pastnames;
            iters.push_back(openiterator(argv[3]));
            int good = 0;
            while (true) {
            nextdir:
                while (dirent* file = iters[subdir]->next()) {
                    if (good++ < 2) goto nextdir;
                    if (good > 3) good = 3;
                    string baseString = argv[3]; baseString += "/";
                    for (int i = 0; i < subdir; i++) {
                        baseString += pastnames[i];
                        baseString += "/";
                    }
                    baseString += file->d_name;
                    if (file->d_type == DT_DIR) {
                        iters.push_back(openiterator(baseString.c_str()));
                        pastnames.push_back(file->d_name);
                        good = 0;
                        subdir++;
                    }
                    if (file->d_type == DT_REG) {
                        string isoString = baseString.substr(strlen(argv[3]) + 1);
                        FILE* file = fopen(baseString.c_str(), "rb");
                        if (!file) {
                            printf("Error reading given file %s.\n", isoString.c_str());
                            goto nextdir;
                        }
                        fseek(file, 0, SEEK_END);
                        int flen = ftell(file);
                        byte* buf = (byte*)malloc(flen);
                        fseek(file, 0, SEEK_SET);
                        fread(buf, 1, flen, file);
                        fclose(file);
                        int len;
                        void* iniso = iso.findFile(isoString.c_str(), len);
                        if (!iniso) {
                            printf("Error finding file %s on ISO file.\n", isoString.c_str());
                            free(buf);
                            goto nextdir;
                        }
                        if (len == flen) {
                            if (!memcmp(buf, iniso, len)) {
                                printf("The file %s in the ISO is unchanged, so we can skip it.\n", isoString.c_str());
                                free(buf);
                                free(iniso);
                                goto nextdir;
                            }
                        }
                        printf("%d bytes (on ISO) to %d bytes (from file given).\n", len, flen);
                        iso.writeFile(buf, (char*)isoString.c_str(), flen);
                        printf("Done with file %s.\n", isoString.c_str());
                    }
                }
                if (!subdir) break;
                iters.pop_back();
                pastnames.pop_back();
                subdir--;
            }
        }
        else printf("isomod put [iso] [directory or iso file] <file to be put in if iso file is given>");
    }
    else if (inArgs("list", 1)) {
        ISO9660 iso(argv[2]);
        if (iso.sectorSize == -1) {
            printf("Error reading ISO file!\n");
            return 1;
        }
        u32 i = 1;
        bool onBigEndian = (!(char*)&i);

        byte* secbuf = (byte*)malloc(iso.sectorSize);
        ISODirectory* dir = (ISODirectory*)(secbuf);
        int off[8] = { 0,0,0,0,0,0,0 };
        int secoff[8] = { 0,0,0,0,0,0,0 };
        char* names[8];
        RootDirectory backdir[8];
        int secres = onBigEndian ? iso.info.root.lbaMSB : iso.info.root.lba;
        int subdirlvl = 0;
        std::vector<ISODirectory> root;
        std::vector<char*> rootnames;

        while (true) {
        partbreak:
            bool changed = false;
            iso.seekSector(secres + secoff[subdirlvl]);
            iso.readSectors(secbuf, 1);
            dir = (ISODirectory*)(secbuf + off[subdirlvl]);
            int old = secres;
            while (dir->len) {
                std::string tobewritten = "";
                if (*dir->filename == 1) backdir[subdirlvl] = *dir;
                if (*dir->filename != 0 && *dir->filename != 1) {
                    char* safefname = (char*)calloc(1, dir->filenameLength + 1);
                    strncpy(safefname, dir->filename, dir->filenameLength);
                    if (strchr(safefname, ';')) *strchr(safefname, ';') = 0;
                    if (!subdirlvl && !(dir->flags & 2)) {
                        root.push_back(*dir);
                        rootnames.push_back(safefname);
                        goto cont;
                    }
                    for (int i = 0; i < subdirlvl; i++) {
                        if (i + 1) {
                            tobewritten += names[i];
                            tobewritten += '/';
                        }
                    }
                    tobewritten += safefname;
                    if (!(dir->flags & 2)) {
                        printf("%s (%d bytes)\n", tobewritten.c_str(), dir->fileLength);
                        free(safefname);
                    }
                    else {
                        secres = onBigEndian ? dir->lbaMSB : dir->lba;
                        off[subdirlvl] += dir->len;
                        names[subdirlvl] = safefname;
                        subdirlvl++;
                        for (int i = 1; i < subdirlvl; i++) {
                            printf(names[i - 1]);
                            printf("/");
                        }
                        printf(safefname);
                        for (int i = dir->filenameLength; i < 0x20; i++) {
                            printf("-");
                        }
                        printf("\n");
                        goto partbreak;
                    }
                }
                cont:
                off[subdirlvl] += dir->len;
                if (off[subdirlvl] >= iso.sectorSize) {
                    iso.readSectors(secbuf, 1);
                    off[subdirlvl] %= iso.sectorSize;
                    secoff[subdirlvl]++;
                }
                dir = (ISODirectory*)(secbuf + off[subdirlvl]);
            }
            if (!subdirlvl) break;
            secres = onBigEndian ? backdir[subdirlvl].lbaMSB : backdir[subdirlvl].lba;
            off[subdirlvl] = 0;
            secoff[subdirlvl] = 0;
            subdirlvl--;
        }
        printf("FILES IN ROOT-------------------\n");
        for (int i = 0; i < root.size(); i++)
            printf("%s (%d bytes)\n", rootnames[i], root[i].fileLength);
        return 0;
    }
    else if (inArgs("ext", 1) || inArgs("extract", 1)) {
        ISO9660 iso(argv[3]);
        if (iso.sectorSize == -1) {
            printf("Error reading ISO file!\n");
            return 1;
        }

        if (inArgs("dir", 1)) {
            //dir
        }
        else if (inArgs("file", 1)) {
            
        }
        else if (inArgs("all", 1)) {
        }
    }
}//*/

//PTR2INT
/*int main(int argc, char* argv[]) {
    /*typedef int (*two)(char*, char*);
    typedef int (*one)(char*);
    auto ptr2int = LoadLibraryA("ptr2lib.dll");
    if (!ptr2int) {
        printf("The ptr2lib.dll library couldn't be found! Please make sure it exists.");
        return 1;
    }//

    auto inArgs = [argc, argv](std::string arg) {

        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], arg.c_str())) {
                return true;
            }
        }
        return false;
    };
    auto argDetail = [argc, argv, inArgs](std::string arg) {
        if (!inArgs(arg)) return std::string();
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], arg.c_str())) {
                return std::string(argv[i + 1]);
            }
        }
        return std::string();
    };
    

    if (inArgs("e") || inArgs("extract")) {
        //auto extract = (two)GetProcAddress(ptr2int, "intextract");
        return intextract(argv[2], argv[3]);
    }
    if (inArgs("l") || inArgs("list")) {
        //auto list = (one)GetProcAddress(ptr2int, "intlist");
        return intlist(argv[2]);
    }
    if (inArgs("c") || inArgs("create") || inArgs("compress")) {
        //auto comp = (two)GetProcAddress(ptr2int, "intcreate");
        return intcreate(argv[2], argv[3]);
    }

    printf("PTR2INT [RMG REWRITE]\n-------------\n");
    printf("extract [intfile] [outfolder]\n");
    printf("  Extracts intfile to outfolder.\n");
    printf("list [intfile]\n");
    printf("  Lists all files in the INT file specified.\n");
    printf("create [intfile] [infolder]\n");
    printf("  Packs infolder into an INT. posesix ptr2int folders work too.\n");
}//*/

/*int main() {
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 100; x++) {
            printf("%d %d %d\n", x, y, ALIGN(x, y));
        }
    }
    system("pause");
}//*/

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
