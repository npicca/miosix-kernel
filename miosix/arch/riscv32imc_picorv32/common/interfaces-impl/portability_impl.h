/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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
 * *ctxsave+4   --> r2
 * ...
 * *ctxsave+120 --> x31
 * *ctxsave+124 --> q0
 * *ctxsave+128 --> q1
 * Register x0 is not saved
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
    picorv32_setq_insn(q3, t1); /* save t1 to q3 */                            \
    asm volatile(                                                              \
            "la t0,  ctxsave                       \n"                         \
            "lw t0, 0(t0)                          \n"                         \
            "sw ra,   0*4+0(t0)                    \n"                         \
            "sw sp,   1*4+0(t0)                    \n"                         \
            "sw gp,   2*4+0(t0)                    \n"                         \
            "sw tp,   3*4+0(t0)                    \n"                         \
             /*not t0  for now*/                                               \
            "sw t1,   5*4+0(t0)                    \n"                         \
            "sw t2,   6*4+0(t0)                    \n"                         \
            "sw s0,   7*4+0(t0)                    \n"                         \
            "sw s1,   8*4+0(t0)                    \n"                         \
            "sw a0,  9*4+0(t0)                     \n"                         \
            "sw a1, 10*4+0(t0)                     \n"                         \
            "sw a2, 11*4+0(t0)                     \n"                         \
            "sw a3, 12*4+0(t0)                     \n"                         \
            "sw a4, 13*4+0(t0)                     \n"                         \
            "sw a5, 14*4+0(t0)                     \n"                         \
            "sw a6, 15*4+0(t0)                     \n"                         \
            "sw a7, 16*4+0(t0)                     \n"                         \
            "sw s2, 17*4+0(t0)                     \n"                         \
            "sw s3, 18*4+0(t0)                     \n"                         \
            "sw s4, 19*4+0(t0)                     \n"                         \
            "sw s5, 20*4+0(t0)                     \n"                         \
            "sw s6, 21*4+0(t0)                     \n"                         \
            "sw s7, 22*4+0(t0)                     \n"                         \
            "sw s8, 23*4+0(t0)                     \n"                         \
            "sw s9, 24*4+0(t0)                     \n"                         \
            "sw s10, 25*4+0(t0)                    \n"                         \
            "sw s11, 26*4+0(t0)                    \n"                         \
            "sw t3, 27*4+0(t0)                     \n"                         \
            "sw t4, 28*4+0(t0)                     \n"                         \
            "sw t5, 29*4+0(t0)                     \n"                         \
            "sw t6, 30*4+0(t0)                     \n"                         \
    );                                                                         \
    /* Save q0 and q1 */                                                       \
    picorv32_getq_insn(t1, q0);                                                \
    asm volatile("sw t1, 31*4+0(t0)");                                         \
    picorv32_getq_insn(t1, q1);                                                \
    asm volatile("sw t1, 32*4+0(t0)");                                         \
    /* Save original t0 value */                                               \
    picorv32_getq_insn(t1,q2);                                                 \
    asm volatile("sw t1, 4*4+0(t0)");                                          \
    /* Restore t0 and t1 */                                                    \
    picorv32_getq_insn(t0,q2);                                                 \
    picorv32_getq_insn(t1,q3);                                                 \
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
                                                                               \
            "lw ra,  0*4+0(t0)                    \n"                          \
            "lw sp,  1*4+0(t0)                    \n"                          \
            "lw gp,  2*4+0(t0)                    \n"                          \
            "lw tp,  3*4+0(t0)                    \n"                          \
            /* not t0  for now */                                              \
            /* not t1 for now */                                               \
            "lw t2,  6*4+0(t0)                    \n"                          \
            "lw s0,  7*4+0(t0)                    \n"                          \
            "lw s1,  8*4+0(t0)                    \n"                          \
            "lw a0,  9*4+0(t0)                    \n"                          \
            "lw a1, 10*4+0(t0)                    \n"                          \
            "lw a2, 11*4+0(t0)                    \n"                          \
            "lw a3, 12*4+0(t0)                    \n"                          \
            "lw a4, 13*4+0(t0)                    \n"                          \
            "lw a5, 14*4+0(t0)                    \n"                          \
            "lw a6, 15*4+0(t0)                    \n"                          \
            "lw a7, 16*4+0(t0)                    \n"                          \
            "lw s2, 17*4+0(t0)                    \n"                          \
            "lw s3, 18*4+0(t0)                    \n"                          \
            "lw s4, 19*4+0(t0)                    \n"                          \
            "lw s5, 20*4+0(t0)                    \n"                          \
            "lw s6, 21*4+0(t0)                    \n"                          \
            "lw s7, 22*4+0(t0)                    \n"                          \
            "lw s8, 23*4+0(t0)                    \n"                          \
            "lw s9, 24*4+0(t0)                    \n"                          \
            "lw s10, 25*4+0(t0)                   \n"                          \
            "lw s11, 26*4+0(t0)                   \n"                          \
            "lw t3, 27*4+0(t0)                    \n"                          \
            "lw t4, 28*4+0(t0)                    \n"                          \
            "lw t5, 29*4+0(t0)                    \n"                          \
            "lw t6, 30*4+0(t0)                    \n"                         ); \
    asm volatile ("lw t1, 31*4+0(t0)");                                        \
    picorv32_setq_insn(q0,t1);                                                 \
    asm volatile ("lw t1, 32*4+0(t0)");                                        \
    picorv32_setq_insn(q1, t1);                                                \
    asm volatile("lw t1, 5*4+0(t0)\n"                                          \
                 "lw t0, 4*4+0(t0)\n");                                        \
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
    picorv32_setq_insn(q3,t6);
    asm volatile(
    "li t6, 0xffffffff"
    );
    picorv32_maskirq_insn(t6,t6);

    //Then we set IRQ_Mask as it was before, to avoid issues
    picorv32_maskirq_insn(t6, zero);

    register int i;

    asm volatile("add %0,t6, zero":"=r"(i));
    picorv32_getq_insn(t6,q0);
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
