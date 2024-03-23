/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2024 - The Hades Authors
**
\******************************************************************************/

/*
** Some of those tests were inspired by NBA's hardware tests and mGBA's technical blog.
**
** References:
**   - https://github.com/nba-emu/hw-test/
**   - https://mgba.io/2020/01/25/infinite-loop-holy-grail/
*/

#include <gba_console.h>
#include <gba_interrupt.h>
#include <gba_timers.h>
#include <gba_dma.h>
#include <gba_systemcalls.h>
#include <gba_sound.h>
#include <stdio.h>

/*
** Check that DMA's reads are latched.
*/
IWRAM_CODE
bool
dma_latch_test_1(
    void
) {
    bool success;
    u32 data;
    u32 res;
    u32 unused;

    data = 0xCAFEBABE;
    res = 0;
    unused = 0;

    // Fill DMA0's latch with 0xCAFEBABE
    REG_DMA0SAD = (u32)&data;
    REG_DMA0DAD = (u32)&unused;
    REG_DMA0CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA0CNT & DMA_ENABLE);

    // Read from invalid memory (< 0x02000000) using DMA0 should read the last
    // latched value
    REG_DMA0SAD = 0x0;
    REG_DMA0DAD = (u32)&res;
    REG_DMA0CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA0CNT & DMA_ENABLE);

    if (res == 0xCAFEBABE) {
        printf("DMA LATCH 1: PASS\n");
        success = true;
    } else {
        printf("DMA LATCH 1: FAIL\n");
        printf("    0x%08lx != 0x%08lx\n", res, (u32)0xCAFEBABE);
        success = false;
    }

    REG_DMA0CNT = 0;
    REG_DMA1CNT = 0;

    return success;
}

/*
** Check that each DMA has its own latch, different from each other.
*/
IWRAM_CODE
bool
dma_latch_test_2(
    void
) {
    bool success;
    u32 data_dma0;
    u32 data_dma1;
    u32 unused;
    u32 res;

    data_dma0 = 0x1BADF00D;
    data_dma1 = 0x2BADCAFE;
    unused = 0;
    res = 0;

    // Fill DMA1's latch with 0x2BADCAFE
    REG_DMA1SAD = (u32)&data_dma1;
    REG_DMA1DAD = (u32)&unused;
    REG_DMA1CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA1CNT & DMA_ENABLE);

    // Fill DMA0's latch with 0x1BADFOOD
    REG_DMA0SAD = (u32)&data_dma0;
    REG_DMA0DAD = (u32)&unused;
    REG_DMA0CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA0CNT & DMA_ENABLE);

    // Read from invalid memory (< 0x02000000) using DMA1 should read the value
    // of that DMA's latch.
    // Therefore, the latch isn't shared among DMAs.
    REG_DMA1SAD = 0x0;
    REG_DMA1DAD = (u32)&res;
    REG_DMA1CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA1CNT & DMA_ENABLE);

    if (res == 0x2BADCAFE) {
        printf("DMA LATCH 2: PASS\n");
        success = true;
    } else {
        printf("DMA LATCH 2: FAIL\n");
        printf("    0x%08lx != 0x%08lx\n", res, (u32)0x2BADCAFE);
        success = false;
    }

    REG_DMA0CNT = 0;
    REG_DMA1CNT = 0;

    return success;
}

/*
** Check that the first access of a DMA, if invalid, reads the last prefetched opcode.
*/
IWRAM_CODE
bool
dma_latch_test_3(
    void
) {
    bool success;
    u32 data;
    u32 unused;
    u32 res;

    data = 0xDEADBEEF;
    unused = 0;
    res = 0;

    // Fill DMA0's latch with 0xDEADBEEF.
    REG_DMA0SAD = (u32)&data;
    REG_DMA0DAD = (u32)&unused;
    REG_DMA0CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA0CNT & DMA_ENABLE);

    // The first read using DMA0, if from invalid memory (>= 0x02000000), should read what's
    // inside the memory bus, aka the last prefetched opcode and not the value latched above.
    REG_DMA0SAD = 0x04000FF0;
    REG_DMA0DAD = (u32)&res;
    REG_DMA0CNT = DMA_ENABLE | DMA32 | 1;

    while (REG_DMA0CNT & DMA_ENABLE);

    if (res == 0xe3530000) {
        printf("DMA LATCH 3: PASS\n");
        success = true;
    } else {
        printf("DMA LATCH 3: FAIL\n");
        printf("    0x%08lx != 0x%08x\n", res, 0xe3530000);
        success = false;
    }

    REG_DMA0CNT = 0;

    return success;
}

/*
** Check that the first invalid access that follows a DMA reads the latest value read by that DMA.
**
** This is kind-of a reproduction of the famous "Hello Kitty" bug.
**
** Reference:
**   - https://mgba.io/2020/01/25/infinite-loop-holy-grail/
*/
IWRAM_CODE
bool
dma_latch_test_4(
    void
) {
    bool success;
    u32 data;

    data = 0xFEEDC0DE;

    REG_SOUNDCNT_X |= SNDSTAT_ENABLE;

    // Setup DMA1 to copy 0xFEEDC0DE to a random location every time Timer 0 overflows.
    REG_DMA1SAD = (u32)&data;
    REG_DMA1DAD = (u32)&REG_FIFO_A;
    REG_DMA1CNT = DMA_ENABLE | DMA_SPECIAL | DMA_SRC_FIXED | DMA_DST_FIXED | DMA32 | DMA_REPEAT | 1;

    // Set the timeout to a very short timer to facilitate
    REG_TM0CNT_L = 0xFFF0;
    REG_TM0CNT_H = TIMER_START;

    // Those timers are used to calculate the timeout
    REG_TM1CNT_L = 0x0000;
    REG_TM1CNT_H = TIMER_START | 2;
    REG_TM2CNT_L = 0x0000;
    REG_TM2CNT_H = TIMER_START | TIMER_COUNT;

    printf("DMA LATCH 4: ...\n");

    while (42) {
        // Read from invalid memory, hoping the DMA left our desired value
        // in the memory bus.
        u32 x = *(u32 volatile *)0x04000FF0;

        if (x == 0xFEEDC0DE) {
            printf(CON_UP(1) "DMA LATCH 4: PASS\n");
            success = true;
            break;
        } else if (REG_TM2CNT_L >= 3) {
            printf(CON_UP(1) "DMA LATCH 4: FAIL (Timeout)\n");
            success = false;
            break;
        }
    }

    REG_TM0CNT_H = 0;
    REG_TM1CNT_H = 0;
    REG_TM2CNT_H = 0;
    REG_DMA1CNT = 0;

    return success;
}

IWRAM_CODE
int
main(
    void
) {
    size_t nb_tests;
    u32 nb_tests_passed;
    u32 i;

    irqInit();
    consoleDemoInit();

    printf("DMA Tests\n");
    printf("  Latch & Open Bus\n\n");

    bool (* const tests[])(void) = {
        dma_latch_test_1,
        dma_latch_test_2,
        dma_latch_test_3,
        dma_latch_test_4,
    };

    nb_tests = sizeof(tests) / sizeof(tests[0]);

    for (i = 0; i < nb_tests; ++i) {
        nb_tests_passed += tests[i]();
    }

    printf("\n");
    printf("Total: %lu/%zu\n", nb_tests_passed, nb_tests);

    while (true) {
        VBlankIntrWait();
    }

    return (0);
}

