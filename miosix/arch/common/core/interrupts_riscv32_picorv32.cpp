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

#include "kernel/logging.h"
#include "kernel/kernel.h"
#include "config/miosix_settings.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "interrupts.h"
#include "interfaces-impl/custom_ops.h"
#include "miosix.h"

using namespace miosix;

#ifdef WITH_ERRLOG

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * Note that printf/iprintf cannot be used inside an IRQ, so that's why there's
 * this function.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    static const char hexdigits[]="0123456789abcdef";
    char result[]="0x........\r\n";
    for(int i=9;i>=2;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    IRQerrorLog(result);
}

#endif //WITH_ERRLOG

/**
 * \internal
 * \return the program counter of the thread that was running when the exception
 * occurred.
 */
static unsigned int getProgramCounter()
{
    register int reg;
    picorv32_getq_insn(t6, q0);
    asm volatile (
            "add t6, t6, zero"
            :"=r"(reg));
    return reg;
}

void NMI_Handler()
{
    IRQerrorLog("\r\n***Unexpected NMI\r\n");
    miosix_private::IRQsystemReboot();
}

//todo: make it do something useful
void IRQEntrypoint() {
    register int IRQ_vect, saved_ra;
    //first of all, let's save the registers
    //asm volatile("  addi sp, sp, -4*32                       \n"                         \
                    "sw x1,   0*4+0(sp)                    \n"                         \
                    "sw x2,   1*4+0(sp)                    \n"                         \
                    "sw x3,   2*4+0(sp)                    \n"                         \
                    "sw x4,   3*4+0(sp)                    \n"                         \
                    "sw x5,   4*4+0(sp)                    \n"                         \
                    "sw x6,   5*4+0(sp)                    \n"                         \
                    "sw x7,   6*4+0(sp)                    \n"                         \
                    "sw x8,   7*4+0(sp)                    \n"                         \
                    "sw x9,   8*4+0(sp)                    \n"                         \
                    "sw x10,  9*4+0(sp)                    \n"                         \
                    "sw x11, 10*4+0(sp)                    \n"                         \
                    "sw x12, 11*4+0(sp)                    \n"                         \
                    "sw x13, 12*4+0(sp)                    \n"                         \
                    "sw x14, 13*4+0(sp)                    \n"                         \
                    "sw x15, 14*4+0(sp)                    \n"                         \
                    "sw x16, 15*4+0(sp)                    \n"                         \
                    "sw x17, 16*4+0(sp)                    \n"                         \
                    "sw x18, 17*4+0(sp)                    \n"                         \
                    "sw x19, 18*4+0(sp)                    \n"                         \
                    "sw x20, 19*4+0(sp)                    \n"                         \
                    "sw x21, 20*4+0(sp)                    \n"                         \
                    "sw x22, 21*4+0(sp)                    \n"                         \
                    "sw x23, 22*4+0(sp)                    \n"                         \
                    "sw x24, 23*4+0(sp)                    \n"                         \
                    "sw x25, 24*4+0(sp)                    \n"                         \
                    "sw x26, 25*4+0(sp)                    \n"                         \
                    "sw x27, 26*4+0(sp)                    \n"                         \
                    "sw x28, 27*4+0(sp)                    \n"                         \
                    "sw x29, 28*4+0(sp)                    \n"                         \
                    "sw x30, 29*4+0(sp)                    \n"                         \
                    "sw x31, 30*4+0(sp)                    \n"                         \
    );

    picorv32_getq_insn(t0,q0);
    asm volatile("add %0, t0, zero":"=r"(saved_ra));

    picorv32_getq_insn(t1, q1);
    asm volatile("add %0, t1, zero":"=r"(IRQ_vect));

    if(IRQ_vect & UF_UNALIGNED){

        void* x = malloc(sizeof(int));
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

        }
        for(;;){}
    }
    picorv32_retirq_insn();
}

void unexpectedInterrupt()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected Peripheral interrupt\r\n");
    #endif //WITH_ERRLOG
    miosix_private::IRQsystemReboot();
}
