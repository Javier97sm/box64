#include <dirent.h>
#include <stdio.h>
#include <limits.h>
#include <fileutils.h>
#include <dlfcn.h>
#include <pthread.h>

#include "box64context.h"
#include "elfloader.h"
#include "box64.h"
#include "core.h"
#include "librarian.h"

/**
 * box64.c contains all APIs.
 */

extern char** environ;

void* my_dlopen(x64emu_t* emu, void *filename, int flag);   // defined in wrappedlibdl.c

EXPORT elfheader_t* LoadX64Library(const char* x64_libname)
{
    // Init box64 thread
    if (!x64_libname) {
        printf_log(LOG_NONE, "x64 library not found!");
        abort();
    }

    int argc = 2;
    const char **argv = (const char**)malloc(argc * sizeof(const char*));
    // Set dummy argv[0] and dummy argv[1], which are required by initialize().

    // Allocate a single block of memory to hold both strings contiguously
    char *block = (char*)malloc(1024);  // Enough space for both "param1" and "param2"

    argv[0] = block;
    argv[1] = block + 512;  // argv[1] starts 512 bytes after argv[0]

    strcpy((char*)argv[0], "/usr/local/bin/box64");
    strcpy((char*)argv[1], x64_libname);

    x64emu_t* dummy_emu = NULL;
    elfheader_t* dummy_elf_header = NULL;
    if (initialize(argc, argv, environ, &dummy_emu, &dummy_elf_header, 0)) {
        return -1;
    }

    printf_log(LOG_DEBUG, "%s initialzied.\n", x64_libname);

    // 1. Load the library.
    // Note: We choose binding all symbols when opening the library,
    // which might output some warnings like "Warning: Weak Symbol
    // xxx not found ...", so we could know ahead which libraries
    // need to wrap. If there are too many noisy noise warnings,
    // we could choose to bind symbol on demand by using RTLD_LAZY
    // flag when opening the library.
    void* lib_handle = my_dlopen(thread_get_emu(), x64_libname, RTLD_NOW);
    if (!lib_handle) {
        printf_log(LOG_NONE, "Load x64 library fail! x64_libname: %s. \n", x64_libname);
        abort();
    }

    // 2. Find the elf header of the library.
    library_t* lib = GetLibInternal(x64_libname);
    if (!lib) {
        printf_log(LOG_NONE, "X64 library %s not found in box64 context !\n", x64_libname);
        abort();
    }
    elfheader_t* elf_header = GetElf(lib);
    if (!elf_header) {
        printf_log(LOG_NONE, "Elf header of library %s not found in box64 context !\n", x64_libname);
        abort();
    }

    return elf_header;
}

EXPORT uintptr_t RunX64Function(elfheader_t* elf_header, const char* funcname, int nargs, ...)
{
    // 3. Call func in emulator.
    uintptr_t x64_symbol_addr = 0;
    int ver = -1, veropt = 0;
    const char* vername = NULL;
    if (!ElfGetGlobalSymbolStartEnd(elf_header, &x64_symbol_addr, NULL, funcname, &ver, &vername, 1, &veropt)) {
        printf_log(LOG_NONE, "Symbol %s not found!\n", funcname);
        abort();
    }

    va_list va;
    va_start(va, nargs);
    uint64_t ret = VRunFunctionWithEmu(thread_get_emu(), 0, x64_symbol_addr, nargs, va);
    va_end(va);

    return ret;
}
