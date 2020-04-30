#pragma once
#if defined _WIN32 || defined __CYGWIN__
#ifdef PTR2EXPORT
// Exporting...
#ifdef __GNUC__
#define EXPORT extern "C" __attribute__ ((dllexport))
#else
#define EXPORT extern "C" __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define EXPORT extern "C" __attribute__ ((dllimport))
#else
#define EXPORT extern "C" __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define NOEXPORT
#else
#if __GNUC__ >= 4
#define EXPORT extern "C" __attribute__ ((visibility ("default")))
#define NOEXPORT __attribute__ ((visibility ("hidden")))
#else
#define EXPORT extern "C"
#define NOEXPORT
#endif
#endif

///PTR2INT///
EXPORT int intextract(char* intfile, char* outfolder);
EXPORT int intlist(char* intfile);
EXPORT int intcreate(char* intfile, char* infolder);

