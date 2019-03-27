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

/**
 * \internal
 * timer interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_preempt()
 */
void SysTick_Handler()   __attribute__((naked));
void SysTick_Handler()
{
    saveContext();
    //Call ISR_preempt(). Name is a C++ mangled name.
    asm volatile("jal _ZN14miosix_private11ISR_preemptEv");
    restoreContext();
}

/**
 * \internal
 * software interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void SVC_Handler() __attribute__((naked));
void SVC_Handler()
{
    saveContext();
	//Call ISR_yield(). Name is a C++ mangled name.
    asm volatile("jal _ZN14miosix_private9ISR_yieldEv");
    restoreContext();
}

#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void TIM3_IRQHandler() __attribute__((naked));
void TIM3_IRQHandler()
{
    saveContext();
    //Call ISR_auxTimer(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private12ISR_auxTimerEv");
    restoreContext();
}
#endif //SCHED_TYPE_CONTROL_BASED

namespace miosix_private {

/**
 * \internal
 * Called by the timer interrupt, preempt to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void ISR_preempt() __attribute__((noinline));
void ISR_preempt()
{
    IRQstackOverflowCheck();
    miosix::IRQtickInterrupt();
}

/**
 * \internal
 * Called by the software interrupt, yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void ISR_yield() __attribute__((noinline));
void ISR_yield()
{
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();
}

#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 */
void ISR_auxTimer() __attribute__((noinline));
void ISR_auxTimer()
{
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();//If the kernel is running, preempt
    if(miosix::kernel_running!=0) miosix::tick_skew=true;
}
#endif //SCHED_TYPE_CONTROL_BASED

void IRQstackOverflowCheck()
{

    const unsigned int watermarkSize=miosix::WATERMARK_LEN/sizeof(unsigned int);
    for(unsigned int i=0;i<watermarkSize;i++)
    {
        if(miosix::cur->watermark[i]!=miosix::WATERMARK_FILL){
            miosix::errorHandler(miosix::STACK_OVERFLOW);

        }
    }

    if(miosix::cur->ctxsave[1] < reinterpret_cast<unsigned int>(
            miosix::cur->watermark+watermarkSize))
        miosix::errorHandler(miosix::STACK_OVERFLOW);
}

void IRQsystemReboot()
{
    doDisableInterrupts();
    asm volatile("j _Z13Reset_Handlerv\n");
}

//todo: fix unaligned access
void IRQHandler() __attribute__((noinline));
void IRQHandler(){

    register int IRQ_vect, saved_ra;

    picorv32_getq_insn(t6,q0);
    asm volatile("add %0, t6, zero":"=r"(saved_ra));

    picorv32_getq_insn(t5, q1);
    asm volatile("add %0, t5, zero":"=r"(IRQ_vect));

    if(IRQ_vect & (IRQ_vect-1)) { //If IRQ_vect is not a power of 2, more than one bit is set
        //IRQerrorLog("More than one interrupt!!");
        //printUnsignedInt(IRQ_vect);
        //for (;;) {}
    }

    IRQstackOverflowCheck();

    if(IRQ_vect & UF_UNALIGNED){

        IRQerrorLog("MEM ERROR");

        /* void* x = malloc(sizeof(int));
         //unaligned memory access!
         //According to the RISC-V ISA, unaligned memory access
         //support is mandatory, be it implemented in hardware or
         //in the software fault handler. Since picorv32 doesn't support
         //it, we must do it by hand
         if(saved_ra & 0x1){
             //compressed instruction, TODO: implement
         } else{
             //standard 32-bit sized instruction
             uint32_t instr = *((uint32_t*)(saved_ra - 4));
             int opcode = instr & 0x7f;
             switch(opcode){
                 case 0: //load instr
                     break;
                 case 20: //store instr
                     break;
             }

         }*/

        for(;;){}
    }

    if(IRQ_vect & TIMER){
        //IRQerrorLog("TIMER");
        picorv32_setq_insn(q3, t6);

        asm volatile("mv t6, %0"::"r"(miosix::TIMER_CLOCK/miosix::TICK_FREQ));

        picorv32_timer_insn(zero, t6);

        picorv32_getq_insn(t6, q3);

        miosix::IRQtickInterrupt();
        if(miosix::kernel_running!=0) miosix::tick_skew=true;

    }
    if(IRQ_vect & 0xfffffff8){
        IRQerrorLog("UNEXPECTED IRQ");
        //todo: handle better
        for(;;){}
    }
}

void IRQEntrypoint() __attribute__((naked));
void IRQEntrypoint()
{
    saveContext();
    IRQHandler();
    restoreContext();
    picorv32_retirq_insn();
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv)
{

    ctxsave[0]=0;
    ctxsave[1]=(unsigned int)sp;//Initialize the thread's stack pointer
    ctxsave[2]=0;
    ctxsave[3]=0;
    ctxsave[4]=0;
    ctxsave[5]=0;
    ctxsave[6]=0;
    ctxsave[7]=0;
    ctxsave[8]=0;
    ctxsave[9]=(unsigned int)pc;
    ctxsave[10]=(unsigned int)argv;
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
    ctxsave[30]=0;
    ctxsave[31]=(unsigned int)&miosix::Thread::threadLauncher; //q0 contains the IRQ return address
    ctxsave[32]=0;


}

#ifdef WITH_PROCESSES

//
// class FaultData
//

void FaultData::print() const
{
    switch(id)
    {
        case MP:
            iprintf("* Attempted data access @ 0x%x (PC was 0x%x)\n",arg,pc);
            break;
        case MP_NOADDR:
            iprintf("* Invalid data access (PC was 0x%x)\n",pc);
            break;
        case MP_XN:
            iprintf("* Attempted instruction fetch @ 0x%x\n",pc);
            break;
        case UF_DIVZERO:
            iprintf("* Dvide by zero (PC was 0x%x)\n",pc);
            break;
        case UF_UNALIGNED:
            iprintf("* Unaligned memory access (PC was 0x%x)\n",pc);
            break;
        case UF_COPROC:
            iprintf("* Attempted coprocessor access (PC was 0x%x)\n",pc);
            break;
        case UF_EXCRET:
            iprintf("* Invalid exception return sequence (PC was 0x%x)\n",pc);
            break;
        case UF_EPSR:
            iprintf("* Attempted access to the EPSR (PC was 0x%x)\n",pc);
            break;
        case UF_UNDEF:
            iprintf("* Undefined instruction (PC was 0x%x)\n",pc);
            break;
        case UF_UNEXP:
            iprintf("* Unexpected usage fault (PC was 0x%x)\n",pc);
            break;
        case HARDFAULT:
            iprintf("* Hardfault (PC was 0x%x)\n",pc);
            break;
        case BF:
            iprintf("* Busfault @ 0x%x (PC was 0x%x)\n",arg,pc);
            break;
        case BF_NOADDR:
            iprintf("*Busfault (PC was 0x%x)\n",pc);
            break;
    }
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv, unsigned int *gotBase)
{
    unsigned int *stackPtr=sp;
    stackPtr--; //Stack is full descending, so decrement first
    *stackPtr=0x01000000; stackPtr--;                                 //--> xPSR
    *stackPtr=reinterpret_cast<unsigned long>(pc); stackPtr--;        //--> pc
    *stackPtr=0xffffffff; stackPtr--;                                 //--> lr
    *stackPtr=0; stackPtr--;                                          //--> r12
    *stackPtr=0; stackPtr--;                                          //--> r3
    *stackPtr=0; stackPtr--;                                          //--> r2
    *stackPtr=0; stackPtr--;                                          //--> r1
    *stackPtr=reinterpret_cast<unsigned long >(argv);                 //--> r0

    ctxsave[0]=reinterpret_cast<unsigned long>(stackPtr);             //--> psp
    ctxsave[6]=reinterpret_cast<unsigned long>(gotBase);              //--> r9 
    //leaving the content of r4-r8,r10-r11 uninitialized
    ctxsave[9]=0xfffffffd; //EXC_RETURN=thread mode, use psp, no floating ops
    //leaving the content of s16-s31 uninitialized
}

#endif //WITH_PROCESSES



void tostring(char str[], int num) {
    int i, rem, len = 0, n;
    n = num;
    while (n != 0) {
        len++;
        n /= 10; }
        for (i = 0; i < len; i++) {
            rem = num % 10;
            num = num / 10;
            str[len - (i + 1)] = rem + '0';
        }
        str[len] = '\0';
    }

void IRQportableStartKernel()
{
    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the scheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];

    IRQbootlog("Setting timer to");
    char buf[8];
    tostring(buf, miosix::TIMER_CLOCK/miosix::TICK_FREQ);
    IRQbootlog(buf);

    picorv32_setq_insn(q3, t6);

    asm volatile("mv t6, %0"::"r"(miosix::TIMER_CLOCK/miosix::TICK_FREQ));

    picorv32_timer_insn(zero, t6);

    picorv32_getq_insn(t6, q3);
    ctxsave=s_ctxsave;//make global ctxsave point to it
    //Note, we can't use enableInterrupts() now since the call is not mathced
    //by a call to disableInterrupts()
    __enable_irq();

    miosix::Thread::yield();
    //Never reaches here
}

void sleepCpu()
{
    //TODO: implement sleep_cpu
    //__WFI();
}


} //namespace miosix_private
