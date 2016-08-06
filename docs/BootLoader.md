# Boot Loader

## Files
* diskboot.S: MBR boot loader
* desc.S: Definition of GDT and IDT (included from entry16.S and pxeboot.S)
* bootmon.S: Boot monitor
* kernload.S: Kernel loader
* entry16.S: Entry point of 16-bit code (from the boot monitor to 32-bit code)
* entry32.S: Entry point of 32-bit code (from 16-bit code to 64-bit code)
* entry64.S: Entry point of 64-bit code (from 32-bit code to C code)
* boot.c: C code and jump to the kernel
* pxeboot.S: PXE boot loader
* const.h: Constant values for the boot loader

## Convention from the boot loader to the kernel

### Registers
No convention (currently).

### Memory

| Start       | End         | Description                      |
| :---------- | :---------- | :------------------------------- |
| `0000 0000` | `0000 7fff` | Reserved by the boot loader      |
| `0000 8000` | `0000 80ff` | Information from the boot loader |
| `0000 8100` | `0000 8fff` | System memory map                |
| `0000 9000` | `0000 ffff` | Boot loader code (stage 2)       |
| `0001 0000` | `0002 ffff` | Kernel (128 KiB)                 |
| `0003 0000` | `0006 ffff` | Initramfs (256 KiB)              |
| `0007 0000` | `0007 8fff` | Used by kernel                   |
| `0007 9000` | `0007 ffff` | Page table (linear mapping)      |
| `0008 0000` | `---- ----` | Used by kernel                   |



    // 0x8000--0x80ff
    struct bootinfo {
        u64 mm_num; // # of entries
        u64 mm_ptr; // Pointer to the memory map table
    };
    // 0x8100--
    struct bootinfo_mm {
        u64 base; // Base address
        u64 len;  // Length
        u32 type; // Type; 1: Usable, 2: Reserved, 3: ACPI reclaimable,
                  // 4: ACPI NVS, 5: Bad memory
        u32 attr; // Attribute; bit 1: non-volatile
    } [bootinfo.mm_num];

### Page table
Linear mapping with 1-Gbyte paging for the first 4 GiB space

### GDT

| Selector | Description          |
| :------- | :------------------- |
| 0x00     | Null                 |
| 0x08     | 64-bit Code (Ring 0) |
| 0x10     | 64-bit Data (Ring 0) |
| 0x18     | 32-bit Code (Ring 0) |
| 0x20     | 32-bit Data (Ring 0) |
| 0x28     | 16-bit Code          |
