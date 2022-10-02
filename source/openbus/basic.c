#include <gba_console.h>
#include <gba_systemcalls.h>
#include <gba_interrupt.h>
#include <stdio.h>

int main(void) {
    irqInit();
    consoleDemoInit();

    printf("BIOS Open Bus Tests\n");
    printf("  Unaligned Accesses\n\n");

    irqEnable(IRQ_VBLANK);
    VBlankIntrWait();

    uint32_t values[12][2] = {
        { 0xe3a02004, *(u32 *)0x0 },
        { 0x00002004, *(u16 *)0x0 },
        { 0x00000004, *(u8 *)0x0 },
        { 0x04e3a020, *(u32 *)0x1 },
        { 0x04000020, *(u16 *)0x1 },
        { 0x00000020, *(u8 *)0x1 },
        { 0x2004e3a0, *(u32 *)0x2 },
        { 0x0000e3a0, *(u16 *)0x2 },
        { 0x000000a0, *(u8 *)0x2 },
        { 0xa02004e3, *(u32 *)0x3 },
        { 0xa00000e3, *(u16 *)0x3 },
        { 0x000000e3, *(u8 *)0x3 },
    };

    for (int i = 0; i < 12; ++i) {
        bool success;

        success = values[i][0] == values[i][1];

        printf("Test %i: %s\n", i, success ? "success" : "fail");
    }

    while (true) {
        VBlankIntrWait();
    }
    return (0);
}