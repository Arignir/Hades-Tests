/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2022 - The Hades Authors
**
\******************************************************************************/

#include <gba_console.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <stdio.h>

#define TEST_01_NB_SAMPLES 3
#define TEST_02_NB_SAMPLES 3
#define TEST_04_NB_SAMPLES 2

/*
** Ideas to explore:
**   - Start/Stop with reload=0xFFFF and see if the DMA/IRQ triggered
*/

/*
** Start a timer and ensure it evolves consistently.
*/
IWRAM_CODE
static
void
test_01(void)
{
    u16 samples[TEST_01_NB_SAMPLES];
    u16 expected[TEST_01_NB_SAMPLES];
    int i;

    expected[0] = 0x0000;
    expected[1] = 0x0003;
    expected[2] = 0x0006;

    asm volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r0, #0x80\n"
        "strh r0, [r1, #0x2]\n"

        // Read the timer's value 3 times in a row
        "ldrh r2, [r1]\n"
        "ldrh r3, [r1]\n"
        "ldrh r4, [r1]\n"

        // Move the return value to the local variables
        "mov %[sample1], r2\n"
        "mov %[sample2], r3\n"
        "mov %[sample3], r4\n"

        // Stop the timer
        "mov r0, #0\n"
        "strh r0, [r1, #0x2]\n"
        :
            [sample1]"=r"(samples[0]),
            [sample2]"=r"(samples[1]),
            [sample3]"=r"(samples[2])
        :
        :
            "r0", "r1", "r2", "r3", "r4"
    );

    for (i = 0; i < TEST_01_NB_SAMPLES; ++i) {
        if (samples[i] != expected[i]) {
            printf("1: FAIL 0x%04X != 0x%04X\n", samples[i], expected[i]);
            return ;
        }
    }

    printf("1: PASS\n");
}

/*
** Ensure the timer doesn't evolve anymore after being stopped.
*/
IWRAM_CODE
static
void
test_02(void)
{
    u16 samples[TEST_02_NB_SAMPLES];
    u16 expected[TEST_02_NB_SAMPLES];
    int i;

    expected[0] = 0x0003;
    expected[1] = 0x0008;
    expected[2] = 0x0008;

    asm volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r0, #0x80\n"
        "strh r0, [r1, #0x2]\n"

        // Spend some time
        "nop\n"
        "nop\n"
        "nop\n"

        // Read the timer's value
        "ldrh r2, [r1]\n"

        // Stop the timer
        "mov r0, #0\n"
        "strh r0, [r1, #0x2]\n"

        // Spend some time
        "nop\n"
        "nop\n"
        "nop\n"

        // Read the timer's value
        "ldrh r3, [r1]\n"

        // Reset the timer's reload value to a random value
        "ldr r0, =#0xDEAD\n"
        "ldrh r0, [r1]\n"

        // Spend some time
        "nop\n"
        "nop\n"
        "nop\n"

        // Read the timer's value
        "ldrh r4, [r1]\n"

        "mov %[sample1], r2\n"
        "mov %[sample2], r3\n"
        "mov %[sample3], r4\n"
        :
            [sample1]"=r"(samples[0]),
            [sample2]"=r"(samples[1]),
            [sample3]"=r"(samples[2])
        :
        :
            "r0", "r1", "r2", "r3", "r4"
    );

    for (i = 0; i < TEST_02_NB_SAMPLES; ++i) {
        if (samples[i] != expected[i]) {
            printf("2: FAIL 0x%04X != 0x%04X\n", samples[i], expected[i]);
            return ;
        }
    }

    printf("2: PASS\n");
}

/*
** Start and immediately stop the timer.
*/
IWRAM_CODE
static
void
test_03(void)
{
    u16 sample;
    u16 expected;

    expected = 0x0001;

    asm volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r2, #0x80\n"
        "strh r2, [r1, #0x2]\n"

        // Stop the timer
        "strh r0, [r1, #0x2]\n"

        // Read the timer's value
        "ldrh r2, [r1]\n"

        "mov %[sample], r2\n"
        :
            [sample]"=r"(sample)
        :
        :
            "r0", "r1", "r2"
    );

    if (sample != expected) {
        printf("3: FAIL 0x%04X != 0x%04X\n", sample, expected);
    } else {
        printf("3: PASS\n");
    }
}

/*
** Measure the time it takes to read REG_TM0CNT.
*/
IWRAM_CODE
static
void
test_04(void)
{
    u16 sample;
    u16 expected;

    expected = 0x0005;

    asm volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        /* First experiment */

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r0, #0x80\n"
        "strh r0, [r1, #0x2]\n"

        // Read REG_TM0CNT
        "ldrh r0, [r1, #0x2]\n"

        // Stop the timer
        "mov r0, #0\n"
        "strh r0, [r1, #0x2]\n"

        // Read the timer's value
        "ldrh r2, [r1]\n"

        "mov %[sample], r2\n"
        :
            [sample]"=r"(sample)
        :
        :
            "r0", "r1", "r2"
    );

    if (sample != expected) {
        printf("4: FAIL 0x%04X != 0x%04X\n", sample, expected);
    } else {
        printf("4: PASS\n");
    }
}

/*
** Start, Reset and Stop the timer.
*/
IWRAM_CODE
static
void
test_05(void)
{
    u16 sample;
    u16 expected;

    expected = 0x0009;

    asm volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r2, #0x80\n"
        "strh r2, [r1, #0x2]\n"

        // Spend some time
        "nop\n"
        "nop\n"
        "nop\n"

        // Re-Enable the timer
        "strh r2, [r1, #0x2]\n"

        // Spend some time
        "nop\n"
        "nop\n"
        "nop\n"

        // Stop the timer
        "strh r0, [r1, #0x2]\n"

        // Read the timer's value
        "ldrh r2, [r1]\n"

        "mov %[sample], r2\n"
        :
            [sample]"=r"(sample)
        :
        :
            "r0", "r1", "r2"
    );

    if (sample != expected) {
        printf("5: FAIL 0x%04X != 0x%04X\n", sample, expected);
    } else {
        printf("5: PASS\n");
    }
}


/*
** Quel est le comportement quand
*/

IWRAM_CODE
int
main(void)
{
    irqInit();
    consoleDemoInit();

    printf("Timer Tests\n");
    printf("  Basic tests\n\n\n");

    irqEnable(IRQ_VBLANK);
    VBlankIntrWait();

    test_01();
    test_02();
    test_03();
    test_04();
    test_05();

    while (true) {
        VBlankIntrWait();
    }

    return (0);
}