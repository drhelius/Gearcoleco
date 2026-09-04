# Coleco ADAM technical notes

Gearcoleco emulates the base Coleco ADAM with a cycle-scheduled high-level ADAMnet controller.

## Memory and I/O

- The MIOC latch is mirrored through ports `$60-$7f`; only bits 0-3 select the map.
- ADAM control is mirrored through ports `$20-$3f`; bit 0 controls ADAMnet reset and bit 1 selects EOS in the lower ROM view.
- The CPU map changes dynamically between SmartWriter, EOS, OS-7, intrinsic RAM, cartridge, and open bus.
- ADAMnet DMA always accesses the physical 64 KiB intrinsic RAM, independently of CPU ROM overlays.
- MCP memory area `CPU MAP` is the live CPU-visible map. `ADAM RAM` is physical intrinsic RAM.

## Firmware

Computer mode requires separate raw OS-7, EOS, and SmartWriter images. Firmware is identified by role, exact size, and CRC. Firmware bytes are not included in save states.

## ADAMnet structures

- The initial Processor Control Block is at `$fec0` and is 4 bytes.
- Up to 15 Device Control Blocks follow it; each DCB is 21 bytes.
- DCB fields use little-endian buffer address, length, and block values.
- Base device IDs are keyboard `$01`, printer `$02`, floppy drives `$04/$05`, and data packs `$08/$18`.

## Media

- Disk 1 and Disk 2 accept 160 or 320 KiB `.dsk` images.
- Data Pack 1 and Data Pack 2 accept 256 KiB `.ddp` images.
- Media may be dirty or write-protected and has a generation value used to reject a transfer after a swap.
- Desktop source files remain immutable; writes go to complete working-copy images.
