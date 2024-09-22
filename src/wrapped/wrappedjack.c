#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"

#include "generated/wrappedjackdefs.h"

const char* jackName = "libjack.so.0";
#define LIBNAME jack

#include "generated/wrappedjacktypes.h"

#include "wrappercallback.h"

// on_shutdown
#define GO(A)                                                           \
static uintptr_t my_on_shutdown_fct_##A = 0;                            \
static void my_on_shutdown_##A(void* ext_client, void* arg)             \
{                                                                       \
    RunFunctionFmt(my_on_shutdown_fct_##A, "pp", ext_client, arg);      \
}
SUPER()
#undef GO
static void* find_on_shutdown_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_on_shutdown_fct_##A == (uintptr_t)fct) return my_on_shutdown_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_on_shutdown_fct_##A == 0) {my_on_shutdown_fct_##A = (uintptr_t)fct; return my_on_shutdown_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for jack shutdown callback\n");
    return NULL;
}

EXPORT void my_jack_on_shutdown(x64emu_t* emu, void* ext_client, void* callback, void* arg)
{
    my->jack_on_shutdown(ext_client, find_on_shutdown_Fct(callback), arg);
}

#include "wrappedlib_init.h"
