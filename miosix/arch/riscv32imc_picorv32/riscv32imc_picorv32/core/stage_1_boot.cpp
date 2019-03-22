
#include "interfaces/arch_registers.h"
#include "core/interrupts.h" //For the unexpected interrupt call
#include "kernel/stage_2_boot.h"
#include <string.h>

/*
 * startup.cpp
 * STM32 C++ startup.
 * NOTE: for stm32f4 devices ONLY.
 * Supports interrupt handlers in C++ without extern "C"
 * Developed by Terraneo Federico, based on ST startup code.
 * Additionally modified to boot Miosix.
 */

/**************************************************************************//**
 * @brief
 *   Initialize the system.
 *
 * @details
 *   Do required generic HW system init.
 *
 * @note
 *   This function is invoked during system init, before the main() routine
 *   and any data has been initialized. For this reason, it cannot do any
 *   initialization of variables etc.
 *****************************************************************************/


void reset_stub(void) __attribute__((section(".bootstub"), naked));
void reset_stub(void){
    asm volatile(\
    "j _Z13Reset_Handlerv\n"\
    ".balign 16          \n"\
    "j _Z13IRQEntrypointv\n"\
    );
}


 // TODO: decide where to put SystemInit and NVIC_SystemReset
void inline SystemInit(void)
{
	// TODO: implement SystemInit
}


/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //SystemInit() is called *before* initializing .data and zeroing .bss
	//Despite all startup files provided by ST do the opposite, there are three
	//good reasons to do so:
	//First, the CMSIS specifications say that SystemInit() must not access
	//global variables, so it is actually possible to call it before
	//Second, when running Miosix with the xram linker scripts .data and .bss
	//are placed in the external RAM, so we *must* call SystemInit(), which
	//enables xram, before touching .data and .bss
	//Third, this is a performance improvement since the loops that initialize
	//.data and zeros .bss now run with the CPU at full speed instead of 8MHz

    //asm volatile ( \
            "li t5, 0x200\n"\
            "li t6, 0x2000\n"\
            "mem_loop:\n"\
            "sw zero, 0(t5)\n"\
            "addi t5, t5, 4\n"\
            "bne t5, t6, mem_loop\n"\
            );

    SystemInit();

	//These are defined in the linker script
	extern unsigned char _etext asm("_etext");
	extern unsigned char _data asm("_data");
	extern unsigned char _edata asm("_edata");
	extern unsigned char _bss_start asm("_bss_start");
	extern unsigned char _bss_end asm("_bss_end");

    //Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;
    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

	//Move on to stage 2
	_init();

	//If main returns, reboot
	asm volatile ("j _Z15program_startupv");
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler() __attribute__((__interrupt__, noreturn, naked));
void Reset_Handler()
{
    asm volatile("la sp,  _heap_end");

    program_startup();
}

/**
 * All unused interrupts call this function.
 */
extern "C" void Default_Handler() 
{
    unexpectedInterrupt();
}

//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");
