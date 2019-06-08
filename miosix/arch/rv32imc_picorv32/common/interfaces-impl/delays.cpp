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

#include "interfaces/delays.h"

namespace miosix {

void delayMs(unsigned int mseconds)
{
    register const unsigned int count=77;

    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimization
        asm volatile("             add  t0, zero, %0            \n"
                     "             addi t1, zero, 1             \n"
                     "___loop_a_start:                          \n"
                     "             beq  t0, zero, ___loop_a_out \n"
                     "             sub  t0, t0, t1              \n"
                     "             j           ___loop_a_start  \n"
                     "___loop_a_out:                            \n"::"r"(count):"t0", "t1");

    }
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimization
    useconds /= 24;
    asm volatile("             add  t0, zero, %0            \n"
                 "             addi t1, zero, 1             \n"
                 "___loop_b_start:                          \n"
                 "             beq  t0, zero, ___loop_b_out \n"
                 "             sub  t0, t0, t1              \n"
                 "             j    ___loop_b_start         \n"
                 "___loop_b_out:                            \n"::"r"(useconds):"t0", "t1");
}

} //namespace miosix
