# Coleco ADAM technical notes

Gearcoleco emulates the base Coleco ADAM with a cycle-scheduled high-level ADAMnet controller.

## Memory and I/O

- The MIOC latch is mirrored through ports `$60-$7f`; only bits 0-3 select the map.
- ADAM control is mirrored through ports `$20-$3f`; bit 0 controls ADAMnet reset and bit 1 selects EOS in the lower ROM view.
- The CPU map changes dynamically between SmartWriter, EOS, OS-7, intrinsic RAM, cartridge, and open bus.
- ADAMnet DMA always accesses the physical 64 KiB intrinsic RAM, independently of CPU ROM overlays.
- MCP memory area `CPU MAP` is the live CPU-visible map. `ADAM RAM` is physical intrinsic RAM.
- MCP tool `get_adam_status` reports the firmware identities, raw and decoded mapping latches, and all eight live CPU pages.

## Firmware

Computer mode requires separate raw OS-7, EOS, and SmartWriter images. Firmware is identified by role, exact size, and CRC. Firmware bytes are not included in save states.

ADAM save states validate firmware, cartridge CRC/size/mapper, and mounted-media identity. Experimental ADAM states before state version 108 are rejected because they lack cartridge identity. Raw media and working copies remain usable.

MCP tool `start_adam` starts a firmware-only SmartWriter session and returns per-role path, size, and CRC details when firmware is missing or invalid.

## ADAMnet structures

- The initial Processor Control Block is at `$fec0` and is 4 bytes.
- Up to 15 Device Control Blocks follow it; each DCB is 21 bytes.
- DCB fields use little-endian buffer address, length, and block values.
- Base device IDs are keyboard `$01`, printer `$02`, floppy drives `$04/$05`, and data packs `$08/$18`.
- `get_media_info` exposes the same canonical device table with type, media slot, maximum transfer, and timing class.
- `get_adamnet_status` exposes the controller timing, active transfer, all 15 decoded DCBs, and keyboard FIFO/modifier/repeat state.

## Tracing

ADAM tracing is disabled by default. `set_trace_log` accepts `adam.map`, `adam.commands`, `adam.dma`, and `adam.errors`. Events are emitted for mapping-latch changes and transfer boundaries, never per byte or per cycle.

## Media

- Disk 1 and Disk 2 accept 160 or 320 KiB `.dsk` images.
- Data Pack 1 and Data Pack 2 accept 256 KiB `.ddp` images.
- Media may be dirty or write-protected and has a generation value used to reject a transfer after a swap.
- Desktop source files remain immutable; writes go to complete working-copy images.
- `list_adam_media` combines core geometry, position, cache, generation, and dirty state with desktop source and working-copy paths.

## Keyboard control

MCP tool `adam_keyboard` accepts semantic key names with `press`, `release`, or `tap`. It covers letters, digits, punctuation, editing keys, SmartKeys I-VI, directional keys, Shift, Control, and Lock.
