#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "debug.h"
#include "box64context.h"
#include "dynarec.h"
#include "emu/x64emu_private.h"
#include "emu/x64run_private.h"
#include "x64run.h"
#include "x64emu.h"
#include "box64stack.h"
#include "callback.h"
#include "emu/x64run_private.h"
#include "x64trace.h"
#include "dynarec_native.h"
#include "my_cpuid.h"
#include "emu/x87emu_private.h"
#include "emu/x64shaext.h"

#include "arm64_printer.h"
#include "dynarec_arm64_private.h"
#include "dynarec_arm64_functions.h"
#include "dynarec_arm64_helper.h"

uintptr_t dynarec64_AVX_F2_0F(dynarec_arm_t* dyn, uintptr_t addr, uintptr_t ip, int ninst, vex_t vex, int* ok, int* need_epilog)
{
    (void)ip; (void)need_epilog;

    uint8_t opcode = F8;
    uint8_t nextop, u8;
    uint8_t gd, ed, vd;
    uint8_t wback, wb1, wb2;
    uint8_t eb1, eb2, gb1, gb2;
    int32_t i32, i32_;
    int cacheupd = 0;
    int v0, v1, v2;
    int q0, q1, q2;
    int d0, d1, d2;
    int s0;
    uint64_t tmp64u;
    int64_t j64;
    int64_t fixedaddress;
    int unscaled;
    MAYUSE(wb1);
    MAYUSE(wb2);
    MAYUSE(eb1);
    MAYUSE(eb2);
    MAYUSE(gb1);
    MAYUSE(gb2);
    MAYUSE(q0);
    MAYUSE(q1);
    MAYUSE(d0);
    MAYUSE(d1);
    MAYUSE(s0);
    MAYUSE(j64);
    MAYUSE(cacheupd);

    rex_t rex = vex.rex;

    switch(opcode) {

        case 0x10:
            INST_NAME("VMOVSD Gx, [Vx,] Ex");
            nextop = F8;
            GETG;
            if(MODREG) {
                GETGX_empty_VXEX(v0, v2, v1, 0);
                if(v0!=v1) VMOVeD(v0, 0, v1, 0);
                if(v0!=v2) VMOVeD(v0, 1, v2, 1);
            } else {
                SMREAD();
                GETGX_empty(v0);
                addr = geted(dyn, addr, ninst, nextop, &ed, x1, &fixedaddress, &unscaled, 0xfff<<3, 7, rex, NULL, 0, 0);
                VLD64(v0, ed, fixedaddress); // upper part reseted
            }
            YMM0(gd);
            break;
        case 0x11:
            INST_NAME("VMOVSD Ex, Vx, Gx");
            nextop = F8;
            GETG;
            v0 = sse_get_reg(dyn, ninst, x1, gd, 0);
            if(MODREG) {
                GETVXEX(v2, 0, v1, 1, 0);
                if(v0!=v1) VMOVeD(v1, 0, v0, 0);
                if(v1!=v2) VMOVeD(v1, 1, v2, 1);
                YMM0((nextop&7)+(rex.b<<3));
            } else {
                WILLWRITE2();
                addr = geted(dyn, addr, ninst, nextop, &ed, x1, &fixedaddress, &unscaled, 0xfff<<3, 7, rex, NULL, 0, 0);
                VST64(v0, ed, fixedaddress);
                SMWRITE2();
            }
            break;
        case 0x12:
            INST_NAME("VMOVDDUP Gx, Ex");
            nextop = F8;
            GETG;
            if(MODREG) {
                v1 = sse_get_reg(dyn, ninst, x1, (nextop&7)+(rex.b<<3), 0);
                GETGX_empty(v0);
                VDUPQ_64(v0, v1, 0);
                if(vex.l) {
                    GETGY_empty_EY(v0, v1);
                    VDUPQ_64(v0, v1, 0);
                }
            } else {
                SMREAD();
                v0 = sse_get_reg_empty(dyn, ninst, x1, gd);
                addr = geted(dyn, addr, ninst, nextop, &ed, x3, &fixedaddress, NULL, 0, 0, rex, NULL, 0, 0);
                VLDQ1R_64(v0, ed);
                if(vex.l) {
                    GETGY_empty(v0, -1, -1, -1);
                    ADDx_U12(x3, ed, 16);
                    VLDQ1R_64(v0, ed);
                }
            }
            if(!vex.l) YMM0(gd);
            break;

        case 0x2A:
            INST_NAME("VCVTSI2SD Gx, Vx, Ed");
            nextop = F8;
            GETGX_empty_VX(v0, v1);
            GETED(0);
            d1 = fpu_get_scratch(dyn, ninst);
            if(rex.w) {
                SCVTFDx(d1, ed);
            } else {
                SCVTFDw(d1, ed);
            }
            if(v0!=v1) VMOVQ(v0, v1);
            VMOVeD(v0, 0, d1, 0);
            YMM0(gd);
            break;

        case 0x2C:
            INST_NAME("VCVTTSD2SI Gd, Ex");
            nextop = F8;
            GETGD;
            GETEXSD(q0, 0, 0);
            if(!box64_dynarec_fastround) {
                MRS_fpsr(x5);
                BFCw(x5, FPSR_IOC, 1);   // reset IOC bit
                MSR_fpsr(x5);
            }
            FCVTZSxwD(gd, q0);
            if(!box64_dynarec_fastround) {
                MRS_fpsr(x5);   // get back FPSR to check the IOC bit
                TBZ_NEXT(x5, FPSR_IOC);
                if(rex.w) {
                    ORRx_mask(gd, xZR, 1, 1, 0);    //0x8000000000000000
                } else {
                    MOV32w(gd, 0x80000000);
                }
            }
            break;
        case 0x2D:
            INST_NAME("VCVTSD2SI Gd, Ex");
            nextop = F8;
            GETGD;
            GETEXSD(q0, 0, 0);
            if(!box64_dynarec_fastround) {
                MRS_fpsr(x5);
                BFCw(x5, FPSR_IOC, 1);   // reset IOC bit
                MSR_fpsr(x5);
            }
            u8 = sse_setround(dyn, ninst, x1, x2, x3);
            d1 = fpu_get_scratch(dyn, ninst);
            FRINTID(d1, q0);
            x87_restoreround(dyn, ninst, u8);
            FCVTZSxwD(gd, d1);
            if(!box64_dynarec_fastround) {
                MRS_fpsr(x5);   // get back FPSR to check the IOC bit
                TBZ_NEXT(x5, FPSR_IOC);
                if(rex.w) {
                    ORRx_mask(gd, xZR, 1, 1, 0);    //0x8000000000000000
                } else {
                    MOV32w(gd, 0x80000000);
                }
            }
            break;

        case 0x58:
            INST_NAME("VADDSD Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FADDD(d1, v0, v1);
            VMOVeD(v0, 0, d1, 0);
            YMM0(gd)
            break;
        case 0x59:
            INST_NAME("VMULSD Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FMULD(d1, v0, v1);
            VMOVeD(v0, 0, d1, 0);
            YMM0(gd)
            break;
        case 0x5A:
            INST_NAME("VCVTSD2SS Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FCVT_S_D(d1, v1);
            VMOVeS(v0, 0, d1, 0);
            YMM0(gd)
            break;

        case 0x5C:
            INST_NAME("VSUBSD Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FSUBD(d1, v0, v1);
            VMOVeD(v0, 0, d1, 0);
            YMM0(gd)
            break;
        case 0x5D:
            INST_NAME("VMINSD Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FCMPD(v0, v1);
            B_NEXT(cLS);    //Less than or equal
            VMOVeD(v0, 0, v1, 0);   // to not erase uper part
            YMM0(gd)
            break;
        case 0x5E:
            INST_NAME("VDIVSD Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FDIVD(d1, v0, v1);
            VMOVeD(v0, 0, d1, 0);
            YMM0(gd)
            break;
        case 0x5F:
            INST_NAME("VMAXSD Gx, Vx, Ex");
            nextop = F8;
            d1 = fpu_get_scratch(dyn, ninst);
            GETGX_empty_VX(v0, v2);
            GETEXSD(v1, 0, 0);
            if(v0!=v2) {
                if(v0==v1)  {
                    VMOV(d1, v1);
                    v1 = d1;
                }
                VMOVQ(v0, v2);
            }
            FCMPD(v0, v1);
            B_NEXT(cGE);    //Greater than or equal
            VMOVeD(v0, 0, v1, 0);   // to not erase uper part
            YMM0(gd)
            break;

        default:
            DEFAULT;
    }
    return addr;
}
