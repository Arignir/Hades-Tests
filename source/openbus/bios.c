/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2022 - The Hades Authors
**
\******************************************************************************/

#include <gba_console.h>
#include <gba_systemcalls.h>
#include <gba_interrupt.h>
#include <stdio.h>

IWRAM_CODE
int
main(void)
{
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

    for (int i = 0; i < 12; ++i) {
        bool success;

        success = values[i][0] == values[i][1];

        if (success) {
            printf("%i: success\n", i + 1);
        } else {
            printf("%i: fail %08lx != %08lx\n", i + 1, values[i][0], values[i][1]);
        }
    }

    while (true) {
        VBlankIntrWait();
    }
    return (0);
}