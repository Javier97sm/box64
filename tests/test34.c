#define _GNU_SOURCE

#include <stdarg.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <dlfcn.h>
#include <limits.h>

// Built with `gcc -g test34.c -o test34 -ldl -lpthread`

typedef intptr_t (*RunFuncWithEmulatorFunction)(const void *, const char *);
static RunFuncWithEmulatorFunction RunFuncWithEmulator = NULL;

typedef intptr_t (*LoadLibraryWithEmulatorFunction)(const char*);
static LoadLibraryWithEmulatorFunction LoadLibraryWithEmulator = NULL;

typedef intptr_t lib_eld_handle;

static void InitBox64() {
    char box64_lib_path[PATH_MAX] = "/home/javier/Documents/Github/box64/build/libbox64.so";
    char box64_ld_library_path[PATH_MAX] = "/home/javier/Documents/Github/box64/tests";

    setenv("BOX64_LD_LIBRARY_PATH", box64_ld_library_path, 1);

    void* box64_lib_handle = dlopen(box64_lib_path, RTLD_GLOBAL | RTLD_NOW);
    if (!box64_lib_handle) {
        fprintf(stderr, "Error loading box64 library: %s\n", dlerror());
        abort();
    }

    LoadLibraryWithEmulator = dlsym(box64_lib_handle, "LoadX64Library");
    if (!LoadLibraryWithEmulator) {
        fprintf(stderr, "Error getting symbol \"LoadX64Library\" from box64 library: %s\n", dlerror());
        abort();
    }

    RunFuncWithEmulator = dlsym(box64_lib_handle, "RunX64Function");
    if (!RunFuncWithEmulator) {
        fprintf(stderr, "Error getting symbol \"RunX64Function\" from box64 library: %s\n", dlerror());
        abort();
    }

    printf("box64 library initialized.\n");
}


/**
 * Test Points for API `BuildBridge` of libbox64.so: 
 *     - Build a bridge for an arm64 symbol(function)
*/
int main() {
    InitBox64();
    
    lib_eld_handle lib = LoadLibraryWithEmulator("/home/javier/Documents/Github/box64/tests/libx64functions1.so");
    char* x64_str = RunFuncWithEmulator(lib, "hello_word_str");
    printf("%s\n",x64_str);

    printf("All done.\n");
    return 0;
}
