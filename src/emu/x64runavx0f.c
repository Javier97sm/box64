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
uintptr_t TestAVX_0F(x64test_t *test, vex_t vex, uintptr_t addr, int *step)
#else
uintptr_t RunAVX_0F(x64emu_t *emu, vex_t vex, uintptr_t addr, int *step)
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
    sse_regs_t *opex, *opgx, *opvx, eax1;
    sse_regs_t *opey, *opgy, *opvy, eay1;

#ifdef TEST_INTERPRETER
    x64emu_t *emu = test->emu;
#endif
    opcode = F8;

    rex_t rex = vex.rex;

    switch(opcode) {

        case 0x10:  /* VMOVUPS Gx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETGY;
            GX->q[0] = EX->q[0];
            GX->q[1] = EX->q[1];
            if(vex.l) {
                GETEY;
                GY->q[0] = EY->q[0];
                GY->q[1] = EY->q[1];
            } else {
                GY->u128 = 0;
            }
            break;
        case 0x11:  /* VMOVUPS Ex, Gx */
            nextop = F8;
            GETEX(0);
            GETGX;
            EX->q[0] = GX->q[0];
            EX->q[1] = GX->q[1];
            if(vex.l) {
                GETEY;
                GETGY;
                EY->q[0] = GY->q[0];
                EY->q[1] = GY->q[1];
            }
            break;

        case 0x14:  /* VUNPCKLPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GETGY;
            GX->ud[3] = EX->ud[1];
            GX->ud[2] = VX->ud[1];
            GX->ud[1] = EX->ud[0];
            GX->ud[0] = VX->ud[0];
            if(vex.l) {
                GETEY;
                GETVY;
                GY->ud[3] = EY->ud[1];
                GY->ud[2] = VY->ud[1];
                GY->ud[1] = EY->ud[0];
                GY->ud[0] = VY->ud[0];
            } else
                GY->u128 = 0;
            break;

        case 0x28:  /* VMOVAPS Gx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETGY;
            GX->q[0] = EX->q[0];
            GX->q[1] = EX->q[1];
            if(vex.l) {
                GETEY;
                GY->q[0] = EY->q[0];
                GY->q[1] = EY->q[1];
            } else {
                GY->u128 = 0;
            }
            break;
        case 0x29:  /* VMOVAPS Ex, Gx */
            nextop = F8;
            GETEX(0);
            GETGX;
            EX->q[0] = GX->q[0];
            EX->q[1] = GX->q[1];
            if(vex.l) {
                GETEY;
                GETGY;
                EY->q[0] = GY->q[0];
                EY->q[1] = GY->q[1];
            }
            break;
            
        case 0x2F:                      /* VCOMISS Gx, Ex */
            RESET_FLAGS(emu);
            nextop = F8;
            GETEX(0);
            GETGX;
            if(isnan(GX->f[0]) || isnan(EX->f[0])) {
                SET_FLAG(F_ZF); SET_FLAG(F_PF); SET_FLAG(F_CF);
            } else if(isgreater(GX->f[0], EX->f[0])) {
                CLEAR_FLAG(F_ZF); CLEAR_FLAG(F_PF); CLEAR_FLAG(F_CF);
            } else if(isless(GX->f[0], EX->f[0])) {
                CLEAR_FLAG(F_ZF); CLEAR_FLAG(F_PF); SET_FLAG(F_CF);
            } else {
                SET_FLAG(F_ZF); CLEAR_FLAG(F_PF); CLEAR_FLAG(F_CF);
            }
            CLEAR_FLAG(F_OF); CLEAR_FLAG(F_AF); CLEAR_FLAG(F_SF);
            break;

        case 0x52:                      /* VRSQRTPS Gx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETGY;
            for(int i=0; i<4; ++i) {
                if(EX->f[i]==0)
                    GX->f[i] = 1.0f/EX->f[i];
                else if (EX->f[i]<0)
                    GX->f[i] = -NAN;
                else if (isnan(EX->f[i]))
                    GX->f[i] = EX->f[i];
                else if (isinf(EX->f[i]))
                    GX->f[i] = 0.0;
                else
                    GX->f[i] = 1.0f/sqrtf(EX->f[i]);
            }
            if(vex.l) {
                GETEY;
                for(int i=0; i<4; ++i) {
                    if(EY->f[i]==0)
                        GY->f[i] = 1.0f/EY->f[i];
                    else if (EY->f[i]<0)
                        GY->f[i] = -NAN;
                    else if (isnan(EY->f[i]))
                        GY->f[i] = EY->f[i];
                    else if (isinf(EY->f[i]))
                        GY->f[i] = 0.0;
                    else
                        GY->f[i] = 1.0f/sqrtf(EY->f[i]);
                }
            } else
                GY->u128 = 0;
            #ifdef TEST_INTERPRETER
            test->notest = 1;
            #endif
            break;

        case 0x54:                      /* VANDPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GX->u128 = VX->u128 & EX->u128;
            GETGY;
            if(vex.l) {
                GETEY;
                GETVY;
                GY->u128 = VY->u128 & EY->u128;
            } else {
                GY->u128 = 0;
            }
            break;
        case 0x55:                      /* VANDNPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GX->u128 = (~VX->u128) & EX->u128;
            GETGY;
            if(vex.l) {
                GETEY;
                GETVY;
                GY->u128 = (~VY->u128) & EY->u128;
            } else {
                GY->u128 = 0;
            }
            break;

        case 0x57:                      /* XORPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GETGY;
            GX->u128 = VX->u128 ^ EX->u128;
            if(vex.l) {
                GETEY;
                GETVY;
                GY->u128 = VY->u128 ^ EY->u128;
            } else
                GY->u128 = 0;
            break;
        case 0x58:                      /* VADDPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GETGY;
            for(int i=0; i<4; ++i)
                GX->f[i] = VX->f[i] + EX->f[i];
            if(vex.l) {
                GETEY;
                GETVY;
                for(int i=0; i<4; ++i)
                    GY->f[i] = VY->f[i] + EY->f[i];
            } else
                GY->u128 = 0;
            break;
        case 0x59:                      /* VMULPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GETGY;
            for(int i=0; i<4; ++i)
                GX->f[i] = VX->f[i] * EX->f[i];
            if(vex.l) {
                GETEY;
                GETVY;
                for(int i=0; i<4; ++i)
                    GY->f[i] = VY->f[i] * EY->f[i];
            } else
                GY->u128 = 0;
            break;

        case 0x5B:                      /* VCVTDQ2PS Gx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETGY;
            GX->f[0] = EX->sd[0];
            GX->f[1] = EX->sd[1];
            GX->f[2] = EX->sd[2];
            GX->f[3] = EX->sd[3];
            if(vex.l) {
                GETEY;
                GY->f[0] = EY->sd[0];
                GY->f[1] = EY->sd[1];
                GY->f[2] = EY->sd[2];
                GY->f[3] = EY->sd[3];
            } else
                GY->u128 = 0;
            break;
        case 0x5C:                      /* VSUBPS Gx, Vx, Ex */
            nextop = F8;
            GETEX(0);
            GETGX;
            GETVX;
            GETGY;
            for(int i=0; i<4; ++i)
                GX->f[i] = VX->f[i] - EX->f[i];
            if(vex.l) {
                GETEY;
                GETVY;
                for(int i=0; i<4; ++i)
                    GY->f[i] = VY->f[i] - EY->f[i];
            } else
                GY->u128 = 0;
            break;

        case 0x77:
            if(!vex.l) {    // VZEROUPPER
                if(vex.v!=0) {
                    emit_signal(emu, SIGILL, (void*)R_RIP, 0);
                } else {
                    memset(emu->ymm, 0, sizeof(sse_regs_t)*(vex.rex.is32bits)?16:8);
                }
            } else
                return 0;
            break;

        case 0xC2:                      /* VCMPPS Gx, Vx, Ex, Ib */
            nextop = F8;
            GETEX(1);
            GETGX;
            GETVX;
            GETGY;
            tmp8u = F8;
            for(int i=0; i<4; ++i) {
                tmp8s = 0;
                switch(tmp8u&7) {
                    case 0: tmp8s=(VX->f[i] == EX->f[i]); break;
                    case 1: tmp8s=isless(VX->f[i], EX->f[i]); break;
                    case 2: tmp8s=islessequal(VX->f[i], EX->f[i]); break;
                    case 3: tmp8s=isnan(VX->f[i]) || isnan(EX->f[i]); break;
                    case 4: tmp8s=(VX->f[i] != EX->f[i]); break;
                    case 5: tmp8s=isnan(VX->f[i]) || isnan(EX->f[i]) || isgreaterequal(VX->f[i], EX->f[i]); break;
                    case 6: tmp8s=isnan(VX->f[i]) || isnan(EX->f[i]) || isgreater(VX->f[i], EX->f[i]); break;
                    case 7: tmp8s=!isnan(VX->f[i]) && !isnan(EX->f[i]); break;
                }
                GX->ud[i]=(tmp8s)?0xffffffff:0;
            }
            if(vex.l) {
                GETEY;
                GETVY;
                for(int i=0; i<4; ++i) {
                    tmp8s = 0;
                    switch(tmp8u&7) {
                        case 0: tmp8s=(VY->f[i] == EY->f[i]); break;
                        case 1: tmp8s=isless(VY->f[i], EY->f[i]); break;
                        case 2: tmp8s=islessequal(VY->f[i], EY->f[i]); break;
                        case 3: tmp8s=isnan(VY->f[i]) || isnan(EY->f[i]); break;
                        case 4: tmp8s=(VY->f[i] != EY->f[i]); break;
                        case 5: tmp8s=isnan(VY->f[i]) || isnan(EY->f[i]) || isgreaterequal(VY->f[i], EY->f[i]); break;
                        case 6: tmp8s=isnan(VY->f[i]) || isnan(EY->f[i]) || isgreater(VY->f[i], EY->f[i]); break;
                        case 7: tmp8s=!isnan(VY->f[i]) && !isnan(EY->f[i]); break;
                    }
                    GY->ud[i]=(tmp8s)?0xffffffff:0;
                }
            } else
                GY->u128 = 0;
            break;

        case 0xC6:                      /* VSHUFPS Gx, Vx, Ex, Ib */
            nextop = F8;
            GETEX(1);
            GETGX;
            GETVX;
            GETGY;
            GETVY;
            GETEY;
            tmp8u = F8;
            if(GX==VX) {
                eax1 = *VX;
                VX = &eax1;
            }
            if(GX==EX) {
                eay1 = *EX;
                EX = &eay1;
            }
            for(int i=0; i<2; ++i) {
                GX->ud[i] = VX->ud[(tmp8u>>(i*2))&3];
            }
            for(int i=2; i<4; ++i) {
                GX->ud[i] = EX->ud[(tmp8u>>(i*2))&3];
            }
            if(vex.l) {
                if(GY==VY) {
                    eax1 = *VY;
                    VY = &eax1;
                }
                if(GY==EY) {
                    eay1 = *EY;
                    EY = &eay1;
                }
                for(int i=0; i<2; ++i) {
                    GY->ud[i] = VY->ud[(tmp8u>>(i*2))&3];
                }
                for(int i=2; i<4; ++i) {
                    GY->ud[i] = EY->ud[(tmp8u>>(i*2))&3];
                }
            } else
                GY->u128 = 0;
            break;

        default:
            return 0;
    }
    return addr;
}
