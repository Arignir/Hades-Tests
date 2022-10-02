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

    printf("Timer Tests\n");
    printf("  TODO FIXME\n\n");

    irqEnable(IRQ_VBLANK);
    VBlankIntrWait();

    while (true) {
        VBlankIntrWait();
    }

    return (0);
}