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
#include <gba_timers.h>
#include <gba_dma.h>
#include <gba_systemcalls.h>
#include <stdio.h>

uint16_t nb_test_pass = 0;
uint16_t nb_test_fail = 0;

/*
** Ideas to explore:
**   - Disable a DMA before it starts
**   - Enable a DMA during a very long insn and see
**      if is triggered during the previous instruction.
*/

#define NEW_TEST(_idx, _samples_nb, _code) \
    IWRAM_CODE \
    static \
    void \
    test_0##_idx##_iwram(void) \
    { \
        u16 samples[_samples_nb]; \
        u16 expected[_samples_nb]; \
        bool iwram __unused; \
        int i; \
        \
        for (i = 0; i < _samples_nb; ++i) { \
            samples[i] = 0xDEAD; \
        } \
        \
        iwram = true; \
        \
        _code; \
        \
        for (i = 0; i < _samples_nb; ++i) { \
            if (samples[i] != expected[i]) { \
                printf("IWRAM %i: FAIL 0x%04X != 0x%04X", (_idx), samples[i], expected[i]); \
                nb_test_fail += 1; \
                return ; \
            } \
        } \
        printf("IWRAM %i: PASS\n", (_idx)); \
        nb_test_pass += 1; \
    } \
    \
    static \
    void \
    test_0##_idx##_rom(void) \
    { \
        u16 samples[_samples_nb]; \
        u16 expected[_samples_nb]; \
        bool iwram __unused; \
        int i; \
        \
        for (i = 0; i < _samples_nb; ++i) { \
            samples[i] = 0xDEAD; \
        } \
        \
        iwram = false; \
        \
        _code; \
        \
        for (i = 0; i < _samples_nb; ++i) { \
            if (samples[i] != expected[i]) { \
                printf("ROM   %i: FAIL 0x%04X != 0x%04X", (_idx), samples[i], expected[i]); \
                nb_test_fail += 1; \
                return ; \
            } \
        } \
        printf("ROM   %i: PASS\n", (_idx)); \
        nb_test_pass += 1; \
    } \

/*
** Run DMA0 with TM0CNT as the source address.
*/
NEW_TEST(1, 2,  {
    if (iwram) {
        expected[0] = 0x0016;
        expected[1] = 0x0023;
    } else {
        expected[0] = 0x0073;
        expected[1] = 0x00AE;
    }

    REG_TM0CNT_H = 0;
    REG_TM0CNT_L = 0;
    REG_TM0CNT_H = TIMER_START;

    REG_DMA0SAD = (u32)&REG_TM0CNT_L;
    REG_DMA0DAD = (u32)&samples[0];
    REG_DMA0CNT = DMA_ENABLE | DMA16 | DMA_IMMEDIATE | 1;

    while (REG_DMA0CNT & DMA_ENABLE);

    samples[1] = REG_TM0CNT_L;

    REG_TM0CNT_H = 0;
    REG_DMA0CNT = 0;
})

/*
** Run DMA0 with TM0CNT as the source address, but immediately stop it.
*/
NEW_TEST(2, 3,  {
    if (iwram) {
        expected[0] = 0x0016;
        expected[1] = 0x0021;
        expected[2] = 0x0033;
    } else {
        expected[0] = 0x0073;
        expected[1] = 0x00A7;
        expected[2] = 0x00FE;
    }

    REG_TM0CNT_H = 0;
    REG_TM0CNT_L = 0;
    REG_TM0CNT_H = TIMER_START;

    REG_DMA0SAD = (u32)&REG_TM0CNT_L;
    REG_DMA0DAD = (u32)&samples[0];
    REG_DMA0CNT = DMA_ENABLE | DMA16 | DMA_IMMEDIATE | 1;
    REG_DMA0CNT = 0;

    samples[1] = REG_TM0CNT_L;

    while (REG_DMA0CNT & DMA_ENABLE);

    samples[2] = REG_TM0CNT_L;

    REG_TM0CNT_H = 0;
    REG_DMA0CNT = 0;
})

IWRAM_CODE
int
main(void)
{
    irqInit();
    consoleDemoInit();

    printf("DMA Tests\n");
    printf("  Start Delay\n\n");

    irqEnable(IRQ_VBLANK);
    VBlankIntrWait();

    test_01_rom();
    test_01_iwram();

    test_02_rom();
    test_02_iwram();

    printf("\n");
    printf("Total: %u/%u\n", nb_test_pass, nb_test_pass + nb_test_fail);


    while (true) {
        VBlankIntrWait();
    }
    return (0);
}
