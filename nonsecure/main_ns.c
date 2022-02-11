/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 *
 */

/* Standard includes. */
#include <string.h>

/* FreeRTOS include. */
#include "FreeRTOS.h"
#include "task.h"

/* Device includes. */
#include <nrf.h>
#include "board.h"

/* Task includes. */
#include "log.h"
#include "tz_demo.h"
#include "mpu_demo.h"
#include "blinky.h"
#include "cli.h"
/*-----------------------------------------------------------*/

/* Initialize the MPU symbols needed by the port code. */
extern uint32_t __PRIVILEGED_FLASH_NS_segment_start__[];
extern uint32_t __PRIVILEGED_FLASH_NS_segment_end__[];
extern uint32_t __SYSCALLS_FLASH_NS_segment_start__[];
extern uint32_t __SYSCALLS_FLASH_NS_segment_end__[];
extern uint32_t __UNPRIVILEGED_FLASH_NS_segment_start__[];
extern uint32_t __UNPRIVILEGED_FLASH_NS_segment_end__[];
extern uint32_t __PRIVILEGED_RAM_NS_segment_start__[];
extern uint32_t __PRIVILEGED_RAM_NS_segment_end__[];

uint32_t * __privileged_functions_start__   = __PRIVILEGED_FLASH_NS_segment_start__;
uint32_t * __privileged_functions_end__     = ( uint32_t * )( ( uint32_t )__PRIVILEGED_FLASH_NS_segment_end__ - ( uint32_t ) 1 );
uint32_t * __syscalls_flash_start__         = __SYSCALLS_FLASH_NS_segment_start__;
uint32_t * __syscalls_flash_end__           = ( uint32_t * )( ( uint32_t )__SYSCALLS_FLASH_NS_segment_end__ - ( uint32_t ) 1 );
uint32_t * __unprivileged_flash_start__     = __UNPRIVILEGED_FLASH_NS_segment_start__;
uint32_t * __unprivileged_flash_end__       = ( uint32_t * )( ( uint32_t )__UNPRIVILEGED_FLASH_NS_segment_end__ - ( uint32_t ) 1 );
uint32_t * __privileged_sram_start__        = __PRIVILEGED_RAM_NS_segment_start__;
uint32_t * __privileged_sram_end__          = ( uint32_t * )( ( uint32_t )__PRIVILEGED_RAM_NS_segment_end__ - ( uint32_t ) 1 );
/*-----------------------------------------------------------*/

/**
 * @brief Create all demo tasks.
 */
static void prvCreateTasks( void );

/**
 * @brief The mem fault handler.
 *
 * It calls a function called vHandleMemoryFault.
 */
void MemManage_Handler( void ) __attribute__ ( ( naked ) );

/**
 * @brief Initializes the privileged_data section.
 *
 * Called from the startup code.
 */
void InitializeUserMemorySections( void );
/*-----------------------------------------------------------*/

/* Non-Secure main. */
int main( void )
{
    /* Create tasks. */
    prvCreateTasks();

    /* Start scheduler. */
    vTaskStartScheduler();

    /* Will not get here if the scheduler starts successfully.  If you do end up
    here then there wasn't enough heap memory available to start either the idle
    task or the timer/daemon task.  https://www.freertos.org/a00111.html */

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

static void prvCreateTasks( void )
{
    /* Initialize static memory variables */
    InitializeUserMemorySections();

    /* Initialize peripherals on board */
    vBoardInit();

    /* Create debug interface tasks */
    vStartLogTask();
    vStartCLITask();

    /* Create tasks for the TZ Demo. */
    vStartTZDemo();

    /* Create tasks for the MPU Demo. */
    vStartMPUDemo();

    /* Create task for the Blinky Demo. */
    vStartBlinkyDemo();
}
/*-----------------------------------------------------------*/

void InitializeUserMemorySections( void )
{
    extern uint8_t __privileged_data_load_start__[];
    extern uint8_t __privileged_data_start__[];
    extern uint8_t __privileged_data_end__[];

    memcpy( __privileged_data_start__,
            __privileged_data_load_start__,
            __privileged_data_end__ - __privileged_data_start__ );
}
/*-----------------------------------------------------------*/

/* Stack overflow hook. */
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    /* Force an assert. */
    configASSERT( pcTaskName == 0 );
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that
 * is used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
     * function then they must be declared static - otherwise they will be
     * allocated on the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ] __attribute__( ( aligned( 32 ) ) );

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 * application must provide an implementation of vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
     * function then they must be declared static - otherwise they will be
     * allocated on the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ] __attribute__( ( aligned( 32 ) ) );

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

void MemManage_Handler( void )
{
    __asm volatile
    (
            " tst lr, #4                                      \n"
            " ite eq                                          \n"
            " mrseq r0, msp                                   \n"
            " mrsne r0, psp                                   \n"
            " ldr r1, handler_address_const                   \n"
            " bx r1                                           \n"
            "                                                 \n"
            " .align 4                                        \n"
            " handler_address_const: .word vHandleMemoryFault \n"
    );
}
/*-----------------------------------------------------------*/

#if 0
portDONT_DISCARD void vHandleMemoryFault( uint32_t * pulFaultStackAddress )
{
    uint32_t ulPC;
    uint16_t usOffendingInstruction;

    /* Read program counter. */
    ulPC = pulFaultStackAddress[ 6 ];

    /* Read the offending instruction. */
    usOffendingInstruction = *( uint16_t * ) ulPC;

    /* From ARM docs:
     * If the value of bits[15:11] of the halfword being decoded is one of
     * the following, the halfword is the first halfword of a 32-bit
     * instruction:
     * - 0b11101.
     * - 0b11110.
     * - 0b11111.
     * Otherwise, the halfword is a 16-bit instruction.
     */

    /* Extract bits[15:11] of the offending instruction. */
    usOffendingInstruction = usOffendingInstruction & 0xF800;
    usOffendingInstruction = ( usOffendingInstruction >> 11 );

    /* Determine if the offending instruction is a 32-bit instruction or
     * a 16-bit instruction. */
    if( ( usOffendingInstruction == 0x001F ) ||
        ( usOffendingInstruction == 0x001E ) ||
        ( usOffendingInstruction == 0x001D ) )
    {
        /* Since the offending instruction is a 32-bit instruction,
         * increment the program counter by 4 to move to the next
         * instruction. */
        ulPC += 4;
    }
    else
    {
        /* Since the offending instruction is a 16-bit instruction,
         * increment the program counter by 2 to move to the next
         * instruction. */
        ulPC += 2;
    }

    /* Save the new program counter on the stack. */
    pulFaultStackAddress[ 6 ] = ulPC;
}
/*-----------------------------------------------------------*/
#endif
