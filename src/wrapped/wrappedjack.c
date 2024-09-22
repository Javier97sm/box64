#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"

#include "generated/wrappedjackdefs.h"

const char* jackName = "libjack.so.0";
#define LIBNAME jack

#include "generated/wrappedjacktypes.h"

#include "wrappercallback.h"

// utility functions
#define SUPER() \
GO(0)   \
GO(1)   \
GO(2)   \
GO(3)   \
GO(4)   \
GO(5)   \
GO(6)   \
GO(7)   \
GO(8)   \
GO(9)   \
GO(10)  \
GO(11)  \
GO(12)  \
GO(13)  \
GO(14)  \
GO(15)  \

// on_shutdown
#define GO(A)                                               \
static uintptr_t my_on_shutdown_fct_##A = 0;                \
static void my_on_shutdown_##A(void* arg)                   \
{                                                           \
    RunFunctionFmt(my_on_shutdown_fct_##A, "p",  arg);      \
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

// set_process
#define GO(A)                                                       \
static uintptr_t my_set_process_fct_##A = 0;                        \
static int my_set_process_##A(uint32_t nframes, void* arg)          \
{                                                                   \
    RunFunctionFmt(my_set_process_fct_##A, "up",  nframes, arg);    \
}
SUPER()
#undef GO
static void* find_set_process_Fct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct))  return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_set_process_fct_##A == (uintptr_t)fct) return my_set_process_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_set_process_fct_##A == 0) {my_set_process_fct_##A = (uintptr_t)fct; return my_on_shutdown_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for jack set process callback\n");
    return NULL;
}

EXPORT void my_jack_on_shutdown(x64emu_t* emu, void* ext_client, void* callback, void* arg)
{
    my->jack_on_shutdown(ext_client, find_on_shutdown_Fct(callback), arg);
}

EXPORT int my_jack_set_process_callback(x64emu_t* emu, void* ext_client, void* callback, void* arg)
{
    my->jack_on_shutdown(ext_client, find_set_process_Fct(callback), arg);
}

#include "wrappedlib_init.h"
