/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2024 - The Hades Authors
**
\******************************************************************************/

#include <gba_console.h>
#include <gba_interrupt.h>
#include <gba_timers.h>
#include <gba_dma.h>
#include <gba_systemcalls.h>
#include <stdio.h>

#define REG_WAITCNT         *(vu32*)(REG_BASE + 0x204)
#define WAITCNT_PREFETCH    (1 << 14)

u16 nb_test_pass = 0;
u16 nb_test_fail = 0;

enum test_kind {
    TEST_KIND_IWRAM,
    TEST_KIND_EWRAM,
    TEST_KIND_ROM_WITH_PREFETCH,
    TEST_KIND_ROM_WITHOUT_PREFETCH,

    TEST_KIND_MAX,
};

#define TEST_INNER_FN(_kind, _kind_str, _idx, _test_results, _code) \
    { \
        u16 samples[sizeof(_test_results[0]) / sizeof(u16)]; \
        u32 samples_count; \
        u32 i; \
        \
        samples_count = sizeof(_test_results[0]) / sizeof(u16); \
        \
        if ((_kind) == TEST_KIND_ROM_WITH_PREFETCH) { \
            REG_WAITCNT |= WAITCNT_PREFETCH; \
        } else { \
            REG_WAITCNT &= ~WAITCNT_PREFETCH; \
        } \
        \
        for (i = 0; i < samples_count; ++i) { \
            samples[i] = 0xDEAD; \
        } \
        \
        _code; \
        \
        for (i = 0; i < samples_count; ++i) { \
            if (samples[i] != _test_results[(_kind)][i]) { \
                printf(_kind_str " %i: FAIL 0x%04X != 0x%04X", (_idx), samples[i], _test_results[(_kind)][i]); \
                nb_test_fail += 1; \
                return ; \
            } \
        } \
        printf(_kind_str " %i: PASS\n", (_idx)); \
        nb_test_pass += 1; \
    }

#define NEW_TEST(_idx, _test_results, _code) \
    IWRAM_CODE \
    static \
    void \
    test_0##_idx##_iwram(void) \
    { \
        TEST_INNER_FN(TEST_KIND_IWRAM, "IWRAM", (_idx), (_test_results), _code); \
    } \
    \
    EWRAM_CODE \
    static \
    void \
    test_0##_idx##_ewram(void) \
    { \
        TEST_INNER_FN(TEST_KIND_EWRAM, "EWRAM", (_idx), (_test_results), _code); \
    } \
    \
    static \
    void \
    test_0##_idx##_rom_with_prefetch(void) \
    { \
        TEST_INNER_FN(TEST_KIND_ROM_WITH_PREFETCH, "ROM P", (_idx), (_test_results), _code); \
    } \
    \
    static \
    void \
    test_0##_idx##_rom_without_prefetch(void) \
    { \
        TEST_INNER_FN(TEST_KIND_ROM_WITHOUT_PREFETCH, "ROM  ", (_idx), (_test_results), _code); \
    } \

/*
** Run DMA0 with TM0CNT as the source address.
*/
u16 TEST_01_RESULTS[TEST_KIND_MAX][2] = {
    [TEST_KIND_IWRAM]                   = { 0x0016, 0x0022 },
    [TEST_KIND_EWRAM]                   = { 0x005B, 0x008C },
    [TEST_KIND_ROM_WITH_PREFETCH]       = { 0x0067, 0x009E },
    [TEST_KIND_ROM_WITHOUT_PREFETCH]    = { 0x0073, 0x00AE },
};
NEW_TEST(1, TEST_01_RESULTS, {
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
u16 TEST_02_RESULTS[TEST_KIND_MAX][3] = {
    [TEST_KIND_IWRAM]                   = { 0x0016, 0x0020, 0x0032 },
    [TEST_KIND_EWRAM]                   = { 0x005B, 0x0085, 0x00CE },
    [TEST_KIND_ROM_WITH_PREFETCH]       = { 0x0067, 0x0098, 0x00E4 },
    [TEST_KIND_ROM_WITHOUT_PREFETCH]    = { 0x0073, 0x00A7, 0x00FE },
};
NEW_TEST(2, TEST_02_RESULTS, {
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

    test_01_rom_without_prefetch();
    test_01_rom_with_prefetch();
    test_01_iwram();
    test_01_ewram();

    test_02_rom_without_prefetch();
    test_02_rom_with_prefetch();
    test_02_iwram();
    test_02_ewram();

    printf("\n");
    printf("Total: %u/%u\n", nb_test_pass, nb_test_pass + nb_test_fail);

    while (true) {
        VBlankIntrWait();
    }

    return (0);
}
