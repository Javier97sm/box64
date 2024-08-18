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

typedef intptr_t (*RunFuncWithEmulatorFunction)(const char *, const char *, int, ...);
static RunFuncWithEmulatorFunction RunFuncWithEmulator = NULL;

_Atomic int threads_done = 0;

static char cwd[PATH_MAX] = {0};
static char path1[PATH_MAX] = {0};
static char path2[PATH_MAX] = {0};

__attribute__((constructor))
static void init() {
    Dl_info dlinfo;
    if (dladdr((void*)init, &dlinfo) != 0 && dlinfo.dli_fname != NULL) {
        const char* last_slash = strrchr(dlinfo.dli_fname, '/');
        if (last_slash != NULL) {
            strncpy(cwd, dlinfo.dli_fname, last_slash - dlinfo.dli_fname);
        }
    } else {
        perror("init() error");
        abort();
    }

    int ret1 = snprintf(path1, sizeof(path1), "%s/libx64functions1.so", cwd);
    int ret2 = snprintf(path2, sizeof(path2), "%s/libx64functions2.so", cwd);

    if (ret1 >= sizeof(path1) || ret2 >= sizeof(path2)) {
        fprintf(stderr, "Path buffer size is not enough\n");
        abort();
    }
}

static const char* libx64functions1_path() {
    return path1;
}

static const char* libx64functions2_path() {
    return path2;
}
void x64_free(void* ptr) {
    RunFuncWithEmulator(libx64functions1_path(), "x64_free", 1, ptr);
}

char* x64_hell_word_str() {
    return (char*)RunFuncWithEmulator(libx64functions1_path(), "hello_word_str", 0);
}

int x64_tolower(int c) {
    return (double)RunFuncWithEmulator(libx64functions2_path(), "x64_tolower", 1, c);
}

static void InitBox64() {
    char box64_lib_path[PATH_MAX] = {0};
    int ret = snprintf(box64_lib_path, sizeof(box64_lib_path), "%s/../build/libbox64.so", cwd);
    if (ret >= sizeof(box64_lib_path)) {
        fprintf(stderr, "Path buffer size is not enough\n");
        abort();
    }

    char box64_ld_library_path[PATH_MAX] = {0};
    ret = snprintf(box64_ld_library_path, sizeof(box64_ld_library_path), "%s/../x64lib", cwd);
    if (ret >= sizeof(box64_lib_path)) {
        fprintf(stderr, "Path buffer size is not enough\n");
        abort();
    }
    setenv("BOX64_LD_LIBRARY_PATH", box64_ld_library_path, 1);

    void* box64_lib_handle = dlopen(box64_lib_path, RTLD_GLOBAL | RTLD_NOW);
    if (!box64_lib_handle) {
        fprintf(stderr, "Error loading box64 library: %s\n", dlerror());
        abort();
    }

    void* box64_init_func = dlsym(box64_lib_handle, "Initialize");
    if (!box64_init_func) {
        fprintf(stderr, "Error getting symbol \"Initialize\" from box64 library: %s\n", dlerror());
        abort();
    }
    int (*Initialize)() = box64_init_func;
    if (Initialize() != 0) {
        fprintf(stderr, "Error initializing box64 library\n");
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


    printf("All done.\n");
    return 0;
}
