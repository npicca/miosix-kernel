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

#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "interfaces/bsp.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/scheduler/tick_interrupt.h"
#include "core/interrupts.h"
#include "kernel/process.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "kernel/logging.h"


namespace miosix_private {


/*This hack is needed because, while inside an irq, all interrupts are permanently disabled until
 * a retirq occurs.
 *
 * */
void IRQErrorHandler(miosix::Error e) __attribute__((naked));
void IRQErrorHandler(miosix::Error e){
    asm volatile("la t6, _ZN6miosix12errorHandlerENS_5ErrorE":::"t6");
    picorv32_setq_insn(q0,t6);
    __disable_irq();
    picorv32_retirq_insn();
}

void IRQstackOverflowCheck() __attribute__((noinline));
void IRQstackOverflowCheck()
{

    const unsigned int watermarkSize=miosix::WATERMARK_LEN/sizeof(unsigned int);
    for(unsigned int i=0;i<watermarkSize;i++)
    {
        if(miosix::cur->watermark[i]!=miosix::WATERMARK_FILL){
            IRQErrorHandler(miosix::STACK_OVERFLOW);
        }
    }

    if(miosix::cur->ctxsave[1] < reinterpret_cast<unsigned int>(
            miosix::cur->watermark+watermarkSize)) {
        IRQErrorHandler(miosix::STACK_OVERFLOW);
    }
}

void IRQsystemReboot()
{
    IRQerrorLog("rebooting\n");
    doDisableInterrupts();

    asm volatile("j _Z13Reset_Handlerv\n");
}

void IRQHandler() __attribute__((noinline));
void IRQHandler() {

    register int IRQ_vect;

    picorv32_getq_insn(t5, q1);

    asm volatile("add %0, t5, zero":"=r"(IRQ_vect)::"t5");

    if(IRQ_vect & UF_UNALIGNED){

        /* According to the riscv standard, unaligned memory access should be implemented
         * by the IRQ handler (if hardware support is missing).
         * However, since GCC can't generate code that performs unaligned accesses,
         * we can treat an unaligned access as an error and reset the board as such
         */
        IRQerrorLog("Memory alignment error");
        unexpectedInterrupt();
    }

    if(IRQ_vect & TIMER){

        picorv32_setq_insn(q3, t6);
        asm volatile("mv t6, %0"::"r"(miosix::TIMER_CLOCK/miosix::TICK_FREQ):"t6");
        picorv32_timer_insn(zero, t6);
        picorv32_getq_insn(t6, q3);
        miosix::IRQtickInterrupt();

    } else if(IRQ_vect & ECALL){

        miosix::Scheduler::IRQfindNextThread();
    }

    if(IRQ_vect & 0xfffffff8){
        /* these interrupts are disabled by default on the PicoSoC, as they
         * are hardware driven (and configurable via the Verilog source)
         */
        unexpectedInterrupt();
    }
}

void IRQEntrypoint() __attribute__((naked));
void IRQEntrypoint()
{
    saveContext();
    IRQstackOverflowCheck();
    IRQHandler();
    restoreContext();
    picorv32_retirq_insn();
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv)
{

    ctxsave[0]=0;
    ctxsave[1]=(unsigned int)sp; //Initialize the thread's stack pointer
    ctxsave[2]=0;
    ctxsave[3]=0;
    ctxsave[4]=0;
    ctxsave[5]=0;
    ctxsave[6]=0;
    ctxsave[7]=0;
    ctxsave[8]=(unsigned int)pc; //a0 is the first argument to threadLauncher
    ctxsave[9]=(unsigned int)argv; //a1 is the second argument to threadLauncher
    ctxsave[10]=0;
    ctxsave[11]=0;
    ctxsave[12]=0;
    ctxsave[13]=0;
    ctxsave[14]=0;
    ctxsave[15]=0;
    ctxsave[16]=0;
    ctxsave[17]=0;
    ctxsave[18]=0;
    ctxsave[19]=0;
    ctxsave[20]=0;
    ctxsave[21]=0;
    ctxsave[22]=0;
    ctxsave[23]=0;
    ctxsave[24]=0;
    ctxsave[25]=0;
    ctxsave[26]=0;
    ctxsave[27]=0;
    ctxsave[28]=0;
    ctxsave[29]=0;
    ctxsave[30]=(unsigned int)miosix::Thread::threadLauncher; //q0 contains the IRQ return address


}


void IRQportableStartKernel()
{
    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the scheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];

    picorv32_setq_insn(q3, t6);

    asm volatile("mv t6, %0"::"r"(miosix::TIMER_CLOCK/miosix::TICK_FREQ):"t6");

    picorv32_timer_insn(zero, t6);

    picorv32_getq_insn(t6, q3);

    ctxsave=s_ctxsave;//make global ctxsave point to it

    //Note, we can't use enableInterrupts() now since the call is not matched
    //by a call to disableInterrupts()
    __enable_irq();

    miosix::Thread::yield();
    //Never reaches here
}

void sleepCpu()
{
    return; //picorv32 doesn't support a low power mode
}


} //namespace miosix_private
