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
#include <stdio.h>

IWRAM_CODE
int
main(void)
{
    uint16_t nb_test_pass;
    uint16_t nb_test_fail;

    irqInit();
    consoleDemoInit();

    printf("Open Bus Tests\n");
    printf("  BIOS Unaligned Access\n\n");

    irqEnable(IRQ_VBLANK);
    VBlankIntrWait();

    uint32_t values[12][2] = {
        { 0xe3a02004, *(vu32 *)0x0 },
        { 0x00002004, *(vu16 *)0x0 },
        { 0x00000004, *(vu8 *)0x0 },
        { 0x04e3a020, *(vu32 *)0x1 },
        { 0x00000020, *(vu16 *)0x1 },
        { 0x00000020, *(vu8 *)0x1 },
        { 0x2004e3a0, *(vu32 *)0x2 },
        { 0x0000e3a0, *(vu16 *)0x2 },
        { 0x000000a0, *(vu8 *)0x2 },
        { 0xa02004e3, *(vu32 *)0x3 },
        { 0x000000e3, *(vu16 *)0x3 },
        { 0x000000e3, *(vu8 *)0x3 },
    };

    nb_test_pass = 0;
    nb_test_fail = 0;
    for (int i = 0; i < 12; ++i) {
        bool success;

        success = values[i][0] == values[i][1];

        if (success) {
            ++nb_test_pass;
            printf("%02i: PASS\n", i + 1);
        } else {
            ++nb_test_fail;
            printf("%02i: FAIL %08lX != %08lX\n", i + 1, values[i][0], values[i][1]);
        }
    }

    printf("\n");
    printf("Total: %u/%u\n", nb_test_pass, nb_test_pass + nb_test_fail);

    while (true) {
        VBlankIntrWait();
    }
    return (0);
}
