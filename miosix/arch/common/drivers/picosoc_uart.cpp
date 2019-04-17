/***************************************************************************
 *   Copyright (C) 2019 by Terraneo Federico                               *
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

#include <limits>
#include <cstring>
#include <errno.h>
#include <termios.h>
#include "picosoc_uart.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "interfaces/gpio.h"
#include "filesystem/ioctl.h"

using namespace std;
using namespace miosix;

#define reg_uart_clkdiv (*(volatile uint32_t*)0x02000004)
#define reg_uart_data (*(volatile uint32_t*)0x02000008)


namespace miosix {

    PicoSoCUART::PicoSoCUART(int baudrate)
            : Device(Device::STREAM), baudrate(baudrate) {
        reg_uart_clkdiv = TIMER_CLOCK / baudrate;
    }

    ssize_t PicoSoCUART::readBlock(void *buffer, size_t size, off_t where) {

        Lock<FastMutex> l(useMutex);
        char *buf = reinterpret_cast<char *>(buffer);
        char c;
        unsigned int i;
        FastInterruptDisableLock dLock;

        buf[0] = readChar();
        return 1;
        //todo: brutto
        for(i = 0; i < size; i++){
            c = readChar();
            buf[i] = c;
            if(c == '\n') {
                i++;
                break;
            }
        }

        return i;

    }

    ssize_t PicoSoCUART::writeBlock(const void *buffer, size_t size, off_t where)
    {
        Lock<FastMutex> l(useMutex);
        FastInterruptDisableLock dLock;
        const char *buf=reinterpret_cast<const char*>(buffer);
        for(size_t i=0;i<size;i++)
        {
            PicoSoCUART::writeChar(*buf++);
        }
        return size;
    }

    void PicoSoCUART::IRQwrite(const char *str)
    {
        while(*str){
            writeChar(*str++);
        }
        writeChar('\n');
    }

    char PicoSoCUART::readChar() {
        int32_t c = -1;
        uint32_t cycles_begin, cycles_now, cycles;
        __asm__ volatile ("rdcycle %0" : "=r"(cycles_begin));

        while (c == -1) {
            __asm__ volatile ("rdcycle %0" : "=r"(cycles_now));
            cycles = cycles_now - cycles_begin;
            if (cycles > TIMER_CLOCK) {
                cycles_begin = cycles_now;
            }
            c = reg_uart_data;
        }
        return c;
    }

    void PicoSoCUART::writeChar(char c){
        if(c == '\n')
            writeChar('\r');
        reg_uart_data = c;
    }





}





