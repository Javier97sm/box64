#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fenv.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "box64stack.h"
#include "x64emu.h"
#include "x64run.h"
#include "x64emu_private.h"
#include "x64run_private.h"
#include "x64primop.h"
#include "x64trace.h"
#include "x87emu_private.h"
#include "box64context.h"
#include "my_cpuid.h"
#include "bridge.h"
#include "signals.h"
#include "x64shaext.h"
#ifdef DYNAREC
#include "custommem.h"
#include "../dynarec/native_lock.h"
#endif

#include "modrm.h"

#ifdef TEST_INTERPRETER
uintptr_t TestAVX(x64test_t *test, vex_t vex, uintptr_t addr, int *step)
#else
uintptr_t RunAVX(x64emu_t *emu, vex_t vex, uintptr_t addr, int *step)
#endif
{
    uint8_t opcode;
    uint8_t nextop;
    uint8_t tmp8u;
    int8_t tmp8s;
    int32_t tmp32s, tmp32s2;
    uint32_t tmp32u, tmp32u2;
    uint64_t tmp64u, tmp64u2;
    int64_t tmp64s;
    reg64_t *oped, *opgd;
    sse_regs_t *opex, *opgx, eax1;
    mmx87_regs_t *opem, *opgm, eam1;

#ifdef TEST_INTERPRETER
    x64emu_t *emu = test->emu;
#endif
    if( (vex.m==VEX_M_0F) && (vex.p==VEX_P_NONE))
        return RunAVX_0F(emu, vex, addr, step);
    if( (vex.m==VEX_M_0F) && (vex.p==VEX_P_66))
        return RunAVX_660F(emu, vex, addr, step);
    if( (vex.m==VEX_M_0F) && (vex.p==VEX_P_F2))
        return RunAVX_F20F(emu, vex, addr, step);
    if( (vex.m==VEX_M_0F) && (vex.p==VEX_P_F3))
        return RunAVX_F30F(emu, vex, addr, step);
    if( (vex.m==VEX_M_0F38) && (vex.p==VEX_P_66))
        return RunAVX_660F38(emu, vex, addr, step);
    if( (vex.m==VEX_M_0F3A) && (vex.p==VEX_P_66))
        return RunAVX_660F3A(emu, vex, addr, step);

    return 0;
}
