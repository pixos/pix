# Interrupts

* 0x00-0x1f: Exceptions
* 0x20 + IRQ# : External interrupts via I/O APIC (ioapic_map_intr())
* 0x30-0x3f: Kernel use
* 0x40: Local APIC timer
* 0x41-0x4f: Kernel use
* 0x50-0xdf: Driver use
* 0xe0-0xfd: Kernel use
* 0xfe: Crash IPI
* 0xff: Spurious interrupt


## Exceptions

| Vector    | Name                                    | Type       | Error code |
| :-------- | :-------------------------------------- | :--------- | :--------- |
| 0x00      | Divide-by-zero Error (#DE)              | Fault      | N          |
| 0x01      | Debug (#DB)                             | Fault/Trap | N          |
| 0x02      | Non-maskable Interrupt                  | Interrupt  | N          |
| 0x03      | Breakpoint (#BP)                        | Trap       | N          |
| 0x04      | Overflow (#OF)                          | Trap       | N          |
| 0x05      | Bound Range Exceeded (#BR)              | Fault      | N          |
| 0x06      | Invalid Opcode (#UD)                    | Fault      | N          |
| 0x07      | Device Not Available (#NM)              | Fault      | N          |
| 0x08      | Double Fault (#DF)                      | Abort      | Y          |
| 0x09      | Coprocessor Segment Overrun             | Fault      | N          |
| 0x0a      | Invalid TSS (#TS)                       | Fault      | Y          |
| 0x0b      | Segment Not Present (#NP)               | Fault      | Y          |
| 0x0c      | Stack-Segment Fault (#SS)               | Fault      | Y          |
| 0x0d      | General Protection Fault (#GP)          | Fault      | Y          |
| 0x0e      | Page Fault (#PF)                        | Fault      | Y          |
| 0x0f      | Reserved                                |            | N          |
| 0x10      | x87 Floating-Point Exception (#MF)      | Fault      | N          |
| 0x11      | Alignment Check (#AC)                   | Fault      | Y          |
| 0x12      | Machine Check (#MC)                     | Abort      | N          |
| 0x13      | SIMD Floating-Point Exception (#XM/#XF) | Fault      | N          |
| 0x14      | Virtualization Exception (#VE)          | Fault      | N          |
| 0x15-0x1d | Reserved                                |            | N          |
| 0x1e      | Security Exception (#SX)                |            | Y          |
| 0x1f      | Reserved                                |            | N          |
| --        | Triple Fault                            |            | N          |


## IRQs

| IRQ | Description                             |
| :-- | :-------------------------------------- |
| 0   | Programmable Interrupt Timer Interrupt  |
| 1   | Keyboard Interrupt                      |
| 2   | Cascade                                 |
| 3   | COM2                                    |
| 4   | COM1                                    |
| 5   | LPT2                                    |
| 6   | Floppy                                  |
| 7   | LPT1 / Unreliable "spurious" interrupt  |
| 8   | CMOS real-time clock                    |
| 9   | Free for peripherals                    |
| 10  | Free for peripherals                    |
| 11  | Free for peripherals                    |
| 12  | PS2 Mouse                               |
| 13  | FPU / Coprocessor / Inter-processor     |
| 14  | Primary ATA Hard Disk                   |
| 15  | Secondary ATA Hard Disk                 |
