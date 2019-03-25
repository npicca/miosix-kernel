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

#ifndef UART_PICOSOC_H
#define UART_PICOSOC_H

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"
#include "board_settings.h"

namespace miosix {
    class PicoSoCUART : public Device
    {
    public:

        PicoSoCUART(int baudrate);

        ssize_t readBlock(void *buffer, size_t size, off_t where);

        ssize_t writeBlock(const void *buffer, size_t size, off_t where);

        void IRQwrite(const char *str);

    private:

        FastMutex useMutex;                ///< Only one lock because reads and writes happen at the same address
        int baudrate;                     ///< Baudrate



        char readChar();

        void writeChar(char c);

    };

}
#endif //UART_PICOSOC_H