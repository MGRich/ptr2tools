#include "iso9660.h"
#define fseek fseeko64
#define ftell ftello64
#include <chrono>
#include <sys/timeb.h>
#include <ctime>
#include <inttypes.h>
#define bitflip(data) ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) | ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24)  

typedef unsigned int uint;

inline int min(int a, int b) { return (a < b) ? a : b; }
inline int max(int a, int b) { return (a > b) ? a : b; }

bool scmplen(const char* s1, const char* s2, int len) {
    if (len <= 0) return false;
    for (int i = 0; i < len; i += 1) {
        if (tolower(s1[i]) != tolower(s2[i])) {
            return false;
        }
    }
    return true;
}
bool streq(const char* s1, const char* s2) {
    //return (strcmp(s1, s2) == 0);
    return scmplen(s1, s2, strlen(s1));
}

ISO9660::ISO9660(char* filepath, bool make) {
    u32 i = 1;
    onBigEndian = (!(char*)&i);
    sectorSize = -1;
    file = fopen(filepath, "rb+");
    if (!file || make) return;
    const char* search = "\1CD001";
    byte* buf = (byte*)malloc(6);
    //explicitly check sector size 0x800
    sectorSize = 0x800;
    seekSector(16);
    fread(buf, 1, 6, file);
    if (memcmp(buf, search, 6)) {
        int i = 0;
        while (!fseek(file, i++, SEEK_SET)) {
            printf("%" PRId64 "X\n", ftell(file));
            fread(buf, 1, 6, file);
            if (!memcmp(buf, search, 6)) {
                fseek(file, -6, SEEK_CUR);
                break;
            }
        }
    }
    else seekSector(16);
    free(buf);
    buf = (byte*)malloc(sizeof(PrimaryDescriptor));
    fread(buf, 1, sizeof(PrimaryDescriptor), file);
    info = *(PrimaryDescriptor*)buf;
    infosec = tellSector();
    sectorSize = onBigEndian ? info.sectorSizeMSB : info.sectorSize;
    free(buf);
}

void ISO9660::readSectors(void* buf, int sectors) {
    fread(buf, sectorSize, sectors, file);
}

void ISO9660::seekSector(int sector) {
    fseek(file, sector * (u64)sectorSize, SEEK_SET);
}
int ISO9660::tellSector() {
    return ftell(file) / sectorSize;
}

ISODirectory ISO9660::getDir(const char* filep) {
    char* filepath = (char*)(malloc(strlen(filep) + 1));
    memcpy(filepath, filep, strlen(filep) + 1);
    for (uint i = 0; i < strlen(filepath); i++) {
        if (filepath[i] == ' ') filepath[i] = '_';
        else filepath[i] = toupper(filepath[i]);
    }
    char* dirsplit = filepath;
    char* lastper = filepath;
    dirsplit = nullptr; //shut up vs
    while (true) {
        lastper = strchr(lastper + 1, '.');
        if (lastper) dirsplit = strchr(lastper + 1, '.');
        if (!dirsplit) break;
        *dirsplit = '_';
        if (dirsplit) lastper = nullptr;
    }
    byte* sec = (byte*)malloc(sectorSize);
    ISODirectory* dir = (ISODirectory*)(sec);
    int off = 0;
    int secres = onBigEndian ? info.root.lbaMSB : info.root.lba;
    do {
        off = 0;
        seekSector(secres);
        readSectors(sec, 1);
        dir = (ISODirectory*)(sec + off);
        dirsplit = strchr(filepath, '/');
        //printf("search %X\n", secres);
        int old = secres;
        if (dirsplit) *dirsplit = 0;
        while (dir->len) {
            int sclen = dir->filenameLength - 2;
            if (dir->filename[dir->filenameLength - 2] != ';') sclen += 2;
            if (scmplen(filepath, dir->filename, min(sclen, 8))) {
                secres = onBigEndian ? dir->lbaMSB : dir->lba;
                if (dirsplit) filepath = dirsplit + 1;
                break;
            }
            off += dir->len;
            if (off >= sectorSize) {
                readSectors(sec, 1);
                off %= sectorSize;
            }
            dir = (ISODirectory*)(sec + off);
        }
        if (secres == old) return *dir;
    } while (dirsplit);
    ISODirectory ret = *&*dir;
    return ret;
}

void* ISO9660::findFile(const char* filep, int& len) {
    ISODirectory dir = getDir(filep);
    if (dir.flags & 0b10) return false;
    seekSector(onBigEndian ? dir.lbaMSB : dir.lba);
    len = dir.fileLength;
    void* buf = calloc(1, len);
    fread(buf, 1, dir.fileLength, file);
    seekSector(onBigEndian ? dir.lbaMSB : dir.lba);
    return buf;
}

using namespace std::chrono;

int ISO9660::writeFile(void* buf, char* filepath, int len, bool push) {
    int oldlen;
    void* tbf = findFile(filepath, oldlen);
    if (!tbf) return 1;
    free(tbf);
    int aligned = ALIGN(len, sectorSize);
    int seclen = aligned / sectorSize;
    int oldaligned = ALIGN(oldlen, sectorSize);
    int pastaligned = oldaligned;
    uint sec = tellSector();
    seekSector(sec); //just in case
    bool done = false;
    void* padding = calloc(1, sectorSize);
    recheck:
    int seccount = (aligned - oldaligned) / sectorSize;
    if (done && pastaligned != oldaligned) 
        printf("Max was able to be changed from %X to %X (%d more sectors)\n", pastaligned, oldaligned, (oldaligned - pastaligned) / sectorSize);
    if (aligned > oldaligned) {
        printf("Aligned file size (%X) is %sbigger than max (%X) by %d sectors,\nso we will ", aligned, done ? "still " : "", oldaligned, seccount);
        if (!push) {
            printf("abort.\n");
            return 1;
        }
        if (!done) printf("check if the sectors are free.\n");
        seekSector(sec + oldaligned / sectorSize);
        while (!done) {
            byte* secbuf = (byte*)malloc(sectorSize);
            *secbuf = 1; //incase we read over
            readSectors(secbuf, 1);
            for (int i = 0; i < sectorSize; i++) {
                if (secbuf[i] != 0) {
                    seekSector(sec);
                    done = true;
                    goto recheck;
                }
            }
            oldaligned += sectorSize;
            free(secbuf);
        }
        printf("push back sectors.\n");
        fflush(file);
        sec += seclen;
        fseek(file, sectorSize, SEEK_END);
        int lastsec = tellSector();
        fseek(file, 0, SEEK_END);
        for (int i = 0; i < seccount; i++) 
            fwrite(padding, 1, sectorSize, file);
        fflush(file);
        fseek(file, sectorSize, SEEK_END);
        printf("Extended %d sectors to %d sectors.\nMoving sectors (please wait, this can take lots of time)\n", lastsec, tellSector());
        void* oldsec = malloc(sectorSize);
        fseek(file, sectorSize, SEEK_END);
        auto start = high_resolution_clock::now();
        for (uint i = lastsec; i > sec - seccount; i--) {
            //this is going to be WILD
            //we are going to essentially move every sector after it
            //depending on the size, this could go on for quite a while
            //printf("SECTOR %X\n", i);
            //cout << "SECTOR " << uppercase << hex << i << '\n'; //cout is faster i dont like it but its a lot faster
            seekSector(i);
            readSectors(oldsec, 1);
            fflush(file);
            //seekSector(i);
            //fwrite(padding, 1, sectorSize, file);
            seekSector(i + seccount);
            fwrite(oldsec, 1, sectorSize, file);
            fflush(file);
        } 
        free(oldsec);
        printf("Took %" PRId64 "ms\n", duration_cast<milliseconds>(high_resolution_clock::now() - start).count());
        //system("pause");
        printf("Moving LBAs of root\n");
        if (!onBigEndian) {
            if (info.root.lba > sec) info.root.lba += seccount;
            info.root.lbaMSB = bitflip(info.root.lba);

            if (info.pathTableOpt > sec) info.pathTableOpt += seccount;
            if (info.pathTableSector > sec) info.pathTableSector += seccount;
        
            info.pathTableOptMSB = bitflip(info.pathTableOptMSB);
            info.pathTableSectorMSB = bitflip(info.pathTableSectorMSB);
            if (info.pathTableOptMSB > sec) info.pathTableOptMSB += seccount;
            if (info.pathTableSectorMSB > sec) info.pathTableSectorMSB += seccount;
            info.pathTableOptMSB = bitflip(info.pathTableOptMSB);
            info.pathTableSectorMSB = bitflip(info.pathTableSectorMSB);
        }
        else {
            if (info.root.lbaMSB > sec) info.root.lbaMSB += seccount;
            info.root.lba = bitflip(info.root.lbaMSB);

            if (info.pathTableOptMSB > sec) info.pathTableOptMSB += seccount;
            if (info.pathTableSectorMSB > sec) info.pathTableSectorMSB += seccount;

            info.pathTableOpt = bitflip(info.pathTableOpt);
            info.pathTableSector = bitflip(info.pathTableSector);
            if (info.pathTableOpt > sec) info.pathTableOpt += seccount;
            if (info.pathTableSector > sec) info.pathTableSector += seccount;
            info.pathTableOpt = bitflip(info.pathTableOpt);
            info.pathTableSector = bitflip(info.pathTableSector);
        }
        seekSector(infosec);
        fwrite(&info, sizeof(info), 1, file);
        sec -= seclen;
        printf("Editing pathtables\n");
        seekSector(onBigEndian ? info.pathTableSectorMSB : info.pathTableSector);
        byte* secbuf = (byte*)malloc(sectorSize);
        int off = 0;
        readSectors(secbuf, 1);
        for (uint i = 0; i < info.pathTableSize; i++) {
            PathTableEntry* entry = (PathTableEntry*)(secbuf + off);
            bool changed = (entry->lba > sec);
            if (changed) entry->lba += seclen;
            int tsec = tellSector();
            if (changed) {
                int tsec = tellSector() - 1;
                fflush(file);
                seekSector(tsec);
                fseek(file, off, SEEK_CUR);
                fwrite(entry, 1, entry->len, file);
                fflush(file);
                seekSector(tsec + 1);
            }
            off += entry->len;
            if (off >= sectorSize) {
                readSectors(secbuf, 1);
                off %= sectorSize;
            }
        }
        free(secbuf);
    }
    byte* secbuf = (byte*)malloc(sectorSize);
    ISODirectory* dir = (ISODirectory*)(secbuf);
    int off[8] = { 0,0,0,0,0,0,0 };
    int secoff[8] = { 0,0,0,0,0,0,0 };
    RootDirectory backdir[8];
    int secres = onBigEndian ? info.root.lbaMSB : info.root.lba;
    int subdirlvl = 0;
    bool fullbreak = false;
    bool recursive = (aligned > oldaligned);
    int genbuf;

    printf("Finding and modding dirs\n");
    while (true) {
        bool changed = false;
        seekSector(secres + secoff[subdirlvl]);
        readSectors(secbuf, 1);
        dir = (ISODirectory*)(secbuf + off[subdirlvl]);
        int old = secres;
        while (dir->len) {
            if ((onBigEndian ? dir->lbaMSB : dir->lba) == sec) {
                changed = true;
                time_t current = system_clock::to_time_t(system_clock::now());
                std::tm* now = std::gmtime(&current);
                if (!onBigEndian) {
                    dir->fileLength = len;
                    dir->fileLengthMSB = bitflip(dir->fileLength);
                }
                else {
                    dir->fileLengthMSB = len;
                    dir->fileLength = bitflip(dir->fileLengthMSB);
                }
                dir->timestamp.day = now->tm_mday;
                dir->timestamp.month = now->tm_mon;
                dir->timestamp.year = now->tm_year;
                dir->timestamp.hour = now->tm_hour;
                dir->timestamp.minute = now->tm_min;
                dir->timestamp.second = now->tm_sec;
                struct timeb tp;
                ftime(&tp);
                dir->timestamp.offset = (-tp.timezone / 15) + (tp.dstflag * 60) + 48;
                if (!recursive) {
                    fullbreak = true;
                    genbuf = off[subdirlvl];
                    goto partbreak;
                }
            }
            if (recursive && dir->lba > sec) {
                changed = true;
                if (onBigEndian) {
                    dir->lbaMSB += seccount;
                    dir->lba = bitflip(dir->lbaMSB);
                }
                else {
                    dir->lba += seccount;
                    dir->lbaMSB = bitflip(dir->lba);
                }
            }
            if (*dir->filename == 1) backdir[subdirlvl] = *dir;
            if (*dir->filename != 0 && *dir->filename != 1) {
                if (dir->flags & 2) {
                    secres = onBigEndian ? dir->lbaMSB : dir->lba;
                    genbuf = off[subdirlvl];
                    off[subdirlvl] += dir->len;
                    subdirlvl++;
                    goto partbreak;
                }
            }
            if (changed) {
                int tsec = tellSector() - 1;
                fflush(file);
                seekSector(tsec);
                fseek(file, off[subdirlvl], SEEK_CUR);
                fwrite(dir, 1, dir->len, file);
                fflush(file);
                seekSector(tsec + 1);
            }
            off[subdirlvl] += dir->len;
            if (off[subdirlvl] >= sectorSize) {
                readSectors(secbuf, 1);
                off[subdirlvl] %= sectorSize;
                secoff[subdirlvl]++;
            }
            dir = (ISODirectory*)(secbuf + off[subdirlvl]);
        }
        if (!subdirlvl) break;
        secres = onBigEndian ? backdir[subdirlvl].lbaMSB : backdir[subdirlvl].lba;
        genbuf = off[subdirlvl];
        off[subdirlvl] = 0;
        secoff[subdirlvl] = 0;
        subdirlvl--;

    partbreak:
        if (changed) {
            int tsec = tellSector() - 1;
            fflush(file);
            seekSector(tsec);
            fseek(file, genbuf, SEEK_CUR);
            fwrite(dir, 1, dir->len, file);
            fflush(file);
            seekSector(tsec + 1);
        }
        if (fullbreak) break;
    }
    seekSector(sec);
    //fill the sectors to be filled with 0
    for (int i = 0; i < max(seclen, oldaligned / sectorSize); i++)
        fwrite(padding, 1, sectorSize, file);
    seekSector(sec);
    fwrite(buf, 1, len, file);
    fflush(file);
    free(padding);
    return 0;
}

bool ISO9660::isoname(char* filename, char type) {
    /*for (int i = 0; i < strlen(filename); i++) {
        char check = toupper(filename[i]);
        if ((check >= 'A' && check <= 'Z') || (check >= '0' && check <= '9') || 
        type == 'a' && ((check == '!' || check == '"' || (check)
    }//*/
    return true;
}
void ISO9660::close() {
    fclose(file);
    delete this;
}
