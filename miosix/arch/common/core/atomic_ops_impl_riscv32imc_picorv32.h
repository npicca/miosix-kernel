/***************************************************************************
 *   Copyright (C) 2013, 2014, 2015 by Terraneo Federico                   *
 *   Copyright (C) 2019 by Filippo Cremonese, Nicolò Picca                 *
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

#ifndef ATOMIC_OPS_IMPL_RISCV32_PICORV32_H
#define	ATOMIC_OPS_IMPL_RISCV32_PICORV32_H

/**
 * picorv32 does not support atomic instructions (riscv ISA A extension)
 * so we have to redefine the atomic operations using functions
 * that disable the interrupts.
 * 
 * TODO: actually this implementation is not very efficient
 * 
 */

#include "interfaces/arch_registers.h"
#include <kernel/kernel.h>

namespace miosix {


inline int atomicSwap(volatile int *p, int v)
{
    //InterruptDisableLock dLock;
    disableInterrupts();
    int result = *p;
    *p = v;
    enableInterrupts();
    return result;
}

inline void atomicAdd(volatile int *p, int incr)
{
    //InterruptDisableLock dLock;
    disableInterrupts();
    *p += incr;
    enableInterrupts();
}

inline int atomicAddExchange(volatile int *p, int incr)
{
    //InterruptDisableLock dLock;
    disableInterrupts();
    int result = *p;
    *p += incr;
    enableInterrupts();
    return result;
}

inline int atomicCompareAndSwap(volatile int *p, int prev, int next)
{
    //InterruptDisableLock dLock;
    disableInterrupts();
    int result = *p;
    if(*p == prev) *p = next;
    enableInterrupts();
    return result;
}

inline void *atomicFetchAndIncrement(void * const volatile * p, int offset,
        int incr)
{
    //InterruptDisableLock dLock;
    disableInterrupts();
    volatile uint32_t *pt;
    
    void *result = *p;
    if(result == 0) return 0;
    pt = reinterpret_cast<uint32_t*>(result) + offset;
    *pt += incr;
    enableInterrupts();
    return result;
}

} //namespace miosix

#endif //ATOMIC_OPS_IMPL_M0_H
