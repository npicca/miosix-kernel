/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
 *   Copyright (C) 2019 by Cremonese Filippo, Picca Niccolò                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
//Miosix kernel

#ifndef PORTABILITY_IMPL_H
#define PORTABILITY_IMPL_H

#include "interfaces/arch_registers.h"
#include "interfaces/portability.h"
#include "config/miosix_settings.h"
#include "core/interrupts.h"

#include "custom_ops.h"
/**
 * \addtogroup Drivers
 * \{
 */

/*
 * This pointer is used by the kernel, and should not be used by end users.
 * this is a pointer to a location where to store the thread's registers during
 * context switch. It requires C linkage to be used inside asm statement.
 * Registers are saved in the following order:
 *
 * *ctxsave+0   --> x1
 * *ctxsave+4   --> x2
 * *ctxsave+8   --> x4
 * *ctxsave+12  --> x5
 * ...
 * *ctxsave+120 --> x31
 * *ctxsave+120 --> q0
 * Register x0 (zero register) is not saved, since is constant
 * Register gp (x3) is not saved, since its value must be constant
 * to allow for linker relaxation)
 */
extern "C" {
extern volatile unsigned int *ctxsave;
}

/**
 * \internal
 * \def saveContext()
 * Save context from an interrupt<br>
 * Must be the first line of an IRQ where a context switch can happen.
 */

#define saveContext()                                                          \
{                                                                              \
    picorv32_setq_insn(q2, t0); /* save t0 to q2 */                            \
    asm volatile(                                                              \
            "la t0,    ctxsave                     \n"                         \
            "lw t0,      0(t0)                     \n"                         \
            "sw ra,  0*4+0(t0)                     \n"                         \
            "sw sp,  1*4+0(t0)                     \n"                         \
            "sw tp,  2*4+0(t0)                     \n"                         \
             /*not t0  for now*/                                               \
            "sw t1,  4*4+0(t0)                     \n"                         \
            "sw t2,  5*4+0(t0)                     \n"                         \
            "sw s0,  6*4+0(t0)                     \n"                         \
            "sw s1,  7*4+0(t0)                     \n"                         \
            "sw a0,  8*4+0(t0)                     \n"                         \
            "sw a1,  9*4+0(t0)                     \n"                         \
            "sw a2, 10*4+0(t0)                     \n"                         \
            "sw a3, 11*4+0(t0)                     \n"                         \
            "sw a4, 12*4+0(t0)                     \n"                         \
            "sw a5, 13*4+0(t0)                     \n"                         \
            "sw a6, 14*4+0(t0)                     \n"                         \
            "sw a7, 15*4+0(t0)                     \n"                         \
            "sw s2, 16*4+0(t0)                     \n"                         \
            "sw s3, 17*4+0(t0)                     \n"                         \
            "sw s4, 18*4+0(t0)                     \n"                         \
            "sw s5, 19*4+0(t0)                     \n"                         \
            "sw s6, 20*4+0(t0)                     \n"                         \
            "sw s7, 21*4+0(t0)                     \n"                         \
            "sw s8, 22*4+0(t0)                     \n"                         \
            "sw s9, 23*4+0(t0)                     \n"                         \
            "sw s10,24*4+0(t0)                     \n"                         \
            "sw s11,25*4+0(t0)                     \n"                         \
            "sw t3, 26*4+0(t0)                     \n"                         \
            "sw t4, 27*4+0(t0)                     \n"                         \
            "sw t5, 28*4+0(t0)                     \n"                         \
            "sw t6, 29*4+0(t0)                     \n"                         \
    );                                                                         \
    /* Save q0 */                                                              \
    picorv32_getq_insn(t1, q0);                                                \
    asm volatile("sw t1, 30*4+0(t0)":::"t1");                                  \
    /* Save original t0 value */                                               \
    picorv32_getq_insn(t1,q2);                                                 \
    asm volatile("sw t1, 3*4+0(t0)":::"t1");                                   \
    /* Restore t0 and t1 */                                                    \
    picorv32_getq_insn(t0,q2);                                                 \
    asm volatile("la sp, _main_stack_top");                                    \
}

/**
 * \def restoreContext()
 * Restore context in an IRQ where saveContext() is used. Must be the last line
 * of an IRQ where a context switch can happen. The IRQ must be "naked" to
 * prevent the compiler from generating context restore.
 */
#define restoreContext()                                                       \
{                                                                              \
    asm volatile(                                                              \
            "la t0, ctxsave                       \n"                          \
            "lw t0, 0(t0)                         \n"                          \
            "lw ra,  0*4+0(t0)                    \n"                          \
            "lw sp,  1*4+0(t0)                    \n"                          \
            "lw tp,  2*4+0(t0)                    \n"                          \
            /* not t0 for now */                                               \
            /* not t1 for now */                                               \
            "lw t2,  5*4+0(t0)                    \n"                          \
            "lw s0,  6*4+0(t0)                    \n"                          \
            "lw s1,  7*4+0(t0)                    \n"                          \
            "lw a0,  8*4+0(t0)                    \n"                          \
            "lw a1,  9*4+0(t0)                    \n"                          \
            "lw a2, 10*4+0(t0)                    \n"                          \
            "lw a3, 11*4+0(t0)                    \n"                          \
            "lw a4, 12*4+0(t0)                    \n"                          \
            "lw a5, 13*4+0(t0)                    \n"                          \
            "lw a6, 14*4+0(t0)                    \n"                          \
            "lw a7, 15*4+0(t0)                    \n"                          \
            "lw s2, 16*4+0(t0)                    \n"                          \
            "lw s3, 17*4+0(t0)                    \n"                          \
            "lw s4, 18*4+0(t0)                    \n"                          \
            "lw s5, 19*4+0(t0)                    \n"                          \
            "lw s6, 20*4+0(t0)                    \n"                          \
            "lw s7, 21*4+0(t0)                    \n"                          \
            "lw s8, 22*4+0(t0)                    \n"                          \
            "lw s9, 23*4+0(t0)                    \n"                          \
            "lw s10, 24*4+0(t0)                   \n"                          \
            "lw s11, 25*4+0(t0)                   \n"                          \
            "lw t3, 26*4+0(t0)                    \n"                          \
            "lw t4, 27*4+0(t0)                    \n"                          \
            "lw t5, 28*4+0(t0)                    \n"                          \
            "lw t6, 29*4+0(t0)                    \n"                       ); \
                                                                               \
    asm volatile ("lw t1, 30*4+0(t0)");  /* load saved q0 in t1   */           \
    picorv32_setq_insn(q0,t1);                                                 \
    asm volatile("lw t1, 4*4+0(t0)\n"    /*now we can restore t0 and t1 */     \
                 "lw t0, 3*4+0(t0)\n");                                        \
}

/**
 * \}
 */

namespace miosix_private {
    
/**
 * \addtogroup Drivers
 * \{
 */

inline void doYield()
{
    asm volatile("ecall");

}

inline void doDisableInterrupts()
{
    __disable_irq();
}

inline void doEnableInterrupts()
{
    __enable_irq();
}



inline bool checkAreInterruptsEnabled()
{
    // PicoRV32 doesn't offer a way to check the value of the IRQ_Mask register without writing to it
    // First we disable all interrupts, and save old value of IRQ_Mask in t6
    register int i;

    picorv32_setq_insn(q3,t6);
    asm volatile("li t6, 0xffffffff");
    picorv32_maskirq_insn(t6,t6);

    asm volatile("add %0,t6, zero":"=r"(i));

    //Then we set IRQ_Mask as it was before, to avoid issues
    picorv32_maskirq_insn(zero, t6);

    picorv32_getq_insn(t6,q3);
    return (i == 0);
}

#ifdef WITH_PROCESSES

//
// class SyscallParameters
//

inline SyscallParameters::SyscallParameters(unsigned int *context) :
        registers(reinterpret_cast<unsigned int*>(context[0])) {}

inline int SyscallParameters::getSyscallId() const
{
    return registers[3];
}

inline unsigned int SyscallParameters::getFirstParameter() const
{
    return registers[0];
}

inline unsigned int SyscallParameters::getSecondParameter() const
{
    return registers[1];
}

inline unsigned int SyscallParameters::getThirdParameter() const
{
    return registers[2];
}

inline void SyscallParameters::setReturnValue(unsigned int ret)
{
    registers[0]=ret;
}

inline void portableSwitchToUserspace()
{
    asm volatile("movs r3, #1\n\t"
                 "svc  0"
                 :::"r3");
}

#endif //WITH_PROCESSES

/**
 * \}
 */

} //namespace miosix_private

#endif //PORTABILITY_IMPL_H
