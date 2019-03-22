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
 *
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
            "la t0, ctxsave                        \n"                         \
            "sw x1,   0*4+0(t0)                    \n"                         \
            "sw x2,   1*4+0(t0)                    \n"                         \
            "sw x3,   2*4+0(t0)                    \n"                         \
            "sw x4,   3*4+0(t0)                    \n"                         \
            "sw x5,   4*4+0(t0)                    \n"   /* x5 is t0 */        \
            "sw x6,   5*4+0(t0)                    \n"                         \
            "sw x7,   6*4+0(t0)                    \n"                         \
            "sw x8,   7*4+0(t0)                    \n"                         \
            "sw x9,   8*4+0(t0)                    \n"                         \
            "sw x10,  9*4+0(t0)                    \n"                         \
            "sw x11, 10*4+0(t0)                    \n"                         \
            "sw x12, 11*4+0(t0)                    \n"                         \
            "sw x13, 12*4+0(t0)                    \n"                         \
            "sw x14, 13*4+0(t0)                    \n"                         \
            "sw x15, 14*4+0(t0)                    \n"                         \
            "sw x16, 15*4+0(t0)                    \n"                         \
            "sw x17, 16*4+0(t0)                    \n"                         \
            "sw x18, 17*4+0(t0)                    \n"                         \
            "sw x19, 18*4+0(t0)                    \n"                         \
            "sw x20, 19*4+0(t0)                    \n"                         \
            "sw x21, 20*4+0(t0)                    \n"                         \
            "sw x22, 21*4+0(t0)                    \n"                         \
            "sw x23, 22*4+0(t0)                    \n"                         \
            "sw x24, 23*4+0(t0)                    \n"                         \
            "sw x25, 24*4+0(t0)                    \n"                         \
            "sw x26, 25*4+0(t0)                    \n"                         \
            "sw x27, 26*4+0(t0)                    \n"                         \
            "sw x28, 27*4+0(t0)                    \n"                         \
            "sw x29, 28*4+0(t0)                    \n"                         \
            "sw x30, 29*4+0(t0)                    \n"                         \
            "sw x31, 30*4+0(t0)                    \n"                         \
    );                                                                         \
}

/**
 * \def restoreContext()
 * Restore context in an IRQ where saveContext() is used. Must be the last line
 * of an IRQ where a context switch can happen. The IRQ must be "naked" to
 * prevent the compiler from generating context restore.
 */
#define restoreContext()                                                       \
{                                                                              \
        asm volatile(                                                          \
            "la t0, ctxsave                        \n"                         \
            "lw x1,   0*4+0(t0)                    \n"                         \
            "lw x2,   1*4+0(t0)                    \n"                         \
            "lw x3,   2*4+0(t0)                    \n"                         \
            "lw x4,   3*4+0(t0)                    \n"                         \
            "lw x5,   4*4+0(t0)                    \n"   /* x5 is t0 */        \
            "lw x6,   5*4+0(t0)                    \n"                         \
            "lw x7,   6*4+0(t0)                    \n"                         \
            "lw x8,   7*4+0(t0)                    \n"                         \
            "lw x9,   8*4+0(t0)                    \n"                         \
            "lw x10,  9*4+0(t0)                    \n"                         \
            "lw x11, 10*4+0(t0)                    \n"                         \
            "lw x12, 11*4+0(t0)                    \n"                         \
            "lw x13, 12*4+0(t0)                    \n"                         \
            "lw x14, 13*4+0(t0)                    \n"                         \
            "lw x15, 14*4+0(t0)                    \n"                         \
            "lw x16, 15*4+0(t0)                    \n"                         \
            "lw x17, 16*4+0(t0)                    \n"                         \
            "lw x18, 17*4+0(t0)                    \n"                         \
            "lw x19, 18*4+0(t0)                    \n"                         \
            "lw x20, 19*4+0(t0)                    \n"                         \
            "lw x21, 20*4+0(t0)                    \n"                         \
            "lw x22, 21*4+0(t0)                    \n"                         \
            "lw x23, 22*4+0(t0)                    \n"                         \
            "lw x24, 23*4+0(t0)                    \n"                         \
            "lw x25, 24*4+0(t0)                    \n"                         \
            "lw x26, 25*4+0(t0)                    \n"                         \
            "lw x27, 26*4+0(t0)                    \n"                         \
            "lw x28, 27*4+0(t0)                    \n"                         \
            "lw x29, 28*4+0(t0)                    \n"                         \
            "lw x30, 29*4+0(t0)                    \n"                         \
            "lw x31, 30*4+0(t0)                    \n"                         \
            );                                                                 \
        picorv32_getq_insn(t0, q2);                                            \
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

    //asm volatile("movs r3, #0\n\t"\
                 "svc  0"\
                 :::"r3");

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
    asm volatile(
    "li t6, 0xffffffff"
    );
    picorv32_maskirq_insn(t6,t6);

    //Then we set IRQ_Mask as it was before, to avoid issues
    picorv32_maskirq_insn(t6, zero);

    register int i;

    asm volatile("add t6,t6, zero":"=r"(i));
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
