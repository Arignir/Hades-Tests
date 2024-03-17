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
#include <gba_video.h>
#include <stdio.h>

int i;
bool b = false;

IWRAM_CODE
void
gamepak_irq_handler(void)
{
    //irqEnable(IRQ_GAMEPAK);
    *BG_PALETTE = RGB5(0, 0x1F, 0);
    while (true);
}

IWRAM_CODE
int
main(void)
{
    irqInit();
    consoleDemoInit();

    //irqEnable(IRQ_GAMEPAK);
    //irqSet(IRQ_GAMEPAK, &gamepak_irq_handler);

    irqEnable(IRQ_VBLANK);
    VBlankIntrWait();

    *BG_PALETTE = RGB5(0x1F, 0, 0);
    printf("Waiting for the game pak to be removed...\n");

    while (true) {
        i += 1;
        if (i >= 60) {
            i = 0;
            b ^= true;

            if (b) {
                *BG_PALETTE = RGB5(0, 0x1F, 0);
            } else {
                *BG_PALETTE = RGB5(0x1F, 0, 0);
            }
        }

        while (REG_VCOUNT != SCREEN_HEIGHT);
        while (REG_VCOUNT == SCREEN_HEIGHT);
    }
    //while (true) {
    //    printf(".");
    //}

    return (0);
}
