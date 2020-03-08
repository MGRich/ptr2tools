// ptr2app.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>

#define ALIGN(x, y) (((x) + (y-1)) & (~((y)-1)))
int main(int argc, char* argv[])
{
    typedef int (*two)(char*, char*);
    typedef int (*one)(char*);
    auto ptr2lib = LoadLibraryA("ptr2lib.dll");
    auto analyze = (one)GetProcAddress(ptr2lib, "spmanalyze");
    analyze(argv[1]);
}//*/

//PTR2INT
/*int main(int argc, char* argv[]) {
    typedef int (*two)(char*, char*);
    typedef int (*one)(char*);
    auto ptr2int = LoadLibraryA("ptr2lib.dll");
    if (!ptr2int) {
        printf("The ptr2lib.dll library couldn't be found! Please make sure it exists.");
        return 1;
    }

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
        auto extract = (two)GetProcAddress(ptr2int, "intextract");
        return extract(argv[2], argv[3]);
    }
    if (inArgs("l") || inArgs("list")) {
        auto list = (one)GetProcAddress(ptr2int, "intlist");
        return list(argv[2]);
    }
    if (inArgs("c") || inArgs("create") || inArgs("compress")) {
        auto comp = (two)GetProcAddress(ptr2int, "intcreate");
        return comp(argv[2], argv[3]);
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
