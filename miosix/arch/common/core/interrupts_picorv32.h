/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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

#include "interfaces-impl/custom_ops.h"

#ifndef INTERRUPTS_H
#define	INTERRUPTS_H


/**
  \brief   Enable IRQ Interrupts
  \details Enables IRQ interrupts by setting the IRQ bitmask to all zeroes
 */
__attribute__( ( always_inline ) ) static inline void __enable_irq(void)
{
    picorv32_maskirq_insn(zero, zero);
}


/**
  \brief   Disable IRQ Interrupts
  \details Disables IRQ interrupts by setting the IRQ bitmask to all ones
 */
__attribute__( ( always_inline ) ) static inline void __disable_irq(void)
{
    picorv32_setq_insn(q3,t6);
    asm volatile(
            "li t6, 0xffffffff \n"
            );
    picorv32_maskirq_insn(zero, t6);
    picorv32_getq_insn(t6,q3);
}

/**
 * Called when an unexpected interrupt occurs.
 * It is called by stage_1_boot.cpp for all weak interrupts not defined.
 */
void unexpectedInterrupt();

/**
 * Possible kind of faults that the PicoSoc can report.
 * Since interrupt 3-31 are user defined, they are not enumerated here
 */
enum FaultType
{
    TIMER = 1<<0,     //Timer Interrupt
    ECALL = 1<<1,     //Process executed an ECALL/EBREAK instruction (or illegal instruction)
    UF_UNALIGNED=1<<2,//Process attempted unaligned memory access
};


void printUnsignedInt(unsigned int x);

#endif	//INTERRUPTS_H
