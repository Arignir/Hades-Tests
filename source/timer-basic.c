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
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <stdio.h>

u16 nb_test_pass = 0;
u16 nb_test_fail = 0;

/*
** Ideas to explore:
**   - Start/Stop with reload=0xFFFF and see when the DMA/IRQ triggered
**   - Set the write-through or post-indexing bit of LDR (in ROM) while
**       reading the timer's counter to see when exactly the internal cycle
**       is spent.
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
** Start a timer and ensure it evolves consistently.
*/
NEW_TEST(1, 3,  {
    if (iwram) {
        expected[0] = 0x0000;
        expected[1] = 0x0003;
        expected[2] = 0x0006;
    } else {
        expected[0] = 0x0007;
        expected[1] = 0x0011;
        expected[2] = 0x001B;
    }

    __asm__ volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Stop the timer
        "mov r0, #0x00\n"
        "strh r0, [r1, #0x2]\n"

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r0, #0x80\n"
        "strh r0, [r1, #0x2]\n"

        // Read the timer's value 3 times in a row
        "ldrh r2, [r1]\n"
        "ldrh r3, [r1]\n" // 8NS + 1W + [READ] + 1I
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
});

/*
** Ensure the timer doesn't evolve anymore after being stopped.
*/
NEW_TEST(2, 3,  {
    if (iwram) {
        expected[0] = 0x0003;
        expected[1] = 0x0008;
        expected[2] = 0x0008;
    } else {
        expected[0] = 0x0019;
        expected[1] = 0x002A;
        expected[2] = 0x002A;
    }

    __asm__ volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Stop the timer
        "mov r0, #0x00\n"
        "strh r0, [r1, #0x2]\n"

        // Set the reload value to 0
        "mov r0, #0\n"
        "strh r0, [r1]\n"

        // Enable the timer
        "mov r0, #0x80\n"
        "strh r0, [r1, #0x2]\n"

        // Spend some time
        "nop\n" // 8NS
        "nop\n" // 6S
        "nop\n"

        // Read the timer's value
        "ldrh r2, [r1]\n" // 6S + 1W + [READ] + 1I

        // Stop the timer
        "mov r0, #0\n" // 8NS
        "strh r0, [r1, #0x2]\n" // 6S + 1W [WRITE]

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
});

/*
** Start and immediately stop the timer.
*/
NEW_TEST(3, 1,  {
    if (iwram) {
        expected[0] = 0x0001;
    } else {
        expected[0] = 0x0008;
    }

    __asm__ volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Stop the timer
        "mov r0, #0x00\n"
        "strh r0, [r1, #0x2]\n"

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
            [sample]"=r"(samples[0])
        :
        :
            "r0", "r1", "r2"
    );
});

/*
** Measure the time it takes to read REG_TM0CNT.
*/
NEW_TEST(4, 1,  {
    if (iwram) {
        expected[0] = 0x0005;
    } else {
        expected[0] = 0x0018;
    }

    __asm__ volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Stop the timer
        "mov r0, #0x00\n"
        "strh r0, [r1, #0x2]\n"

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
            [sample]"=r"(samples[0])
        :
        :
            "r0", "r1", "r2"
    );
});

/*
** Start, Reset and Stop the timer.
*/
NEW_TEST(5, 1,  {
    if (iwram) {
        expected[0] = 0x0009;
    } else {
        expected[0] = 0x0035;
    }

    __asm__ volatile(
        // Set r1 to REG_TM0CNT
        "ldr r1, =#0x04000100\n"

        // Stop the timer
        "mov r0, #0x00\n"
        "strh r0, [r1, #0x2]\n"

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
            [sample]"=r"(samples[0])
        :
        :
            "r0", "r1", "r2"
    );
});

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

    test_01_rom();
    test_02_rom();
    test_03_rom();
    test_04_rom();
    test_05_rom();

    test_01_iwram();
    test_02_iwram();
    test_03_iwram();
    test_04_iwram();
    test_05_iwram();

    printf("\n");
    printf("Total: %u/%u\n", nb_test_pass, nb_test_pass + nb_test_fail);

    while (true) {
        VBlankIntrWait();
    }

    return (0);
}
