# ColecoVision Super Game Module Technical Notes

Converted from `supergamemodule.txt` resource and expanded with selected hardware notes from Opcode's Super Game Module technical forum posts on AtariAge.

## SGM AY-3-8910 Sound Mapping

Source: Opcode Games technical posts by Eduardo Mello, AtariAge, October 2012 and July 2021.

- The SGM adds an AY-3-8910 PSG. Its three channels are mixed with the ColecoVision SN76489 output.
- The AY-3-8910 is independent of the SGM RAM expansion and is always accessible when the SGM is present.
- I/O port `$50` selects the AY register number.
- I/O port `$51` writes data to the selected AY register.
- I/O port `$52` reads data from the selected AY register.
- AY registers are readable; Opcode recommends detecting the SGM PSG by writing and reading registers back, preferably not in a simple sequential write/read pattern because some ADAM configurations may otherwise fool detection code.
- SGM memory control uses I/O port `$53` for the 24 KB upper RAM area and I/O port `$7F` for BIOS/RAM mapping compatibility with ADAM-style memory control.

## SGM RAM Expansion Notes

Since the SGM specs are now set in stone, I think it is a good time to start publishing the technical info so that homebrewers wanting to develop to it can get started.

This is the first part of a two parts technical discussion about the SGM and in this part I am going to cover the RAM expansion.

As most of you are well aware, ColecoVision work RAM is limited to 1KB, mapped from 6000h to 63FFh, and then mirrored to 6400h until 7FFFh (so 8 times).

The ColecoVision also includes 8KB of BIOS routines, mapped from 0000h to 1FFFh.

The SGM expands the work RAM from 1KB to 32KB maximum. Here is how:
    24KB can be mapped from 2000h to 7FFFh. When mapped, the internal 1KB of RAM is no longer accessible. All legacy software still works normally under this mode. However, since the SGM must also be compatible with the ADAM, the 24KB of extra RAM is NOT enabled by default. In order to use that expanded memory in a ColecoVision system, the programmer must first enable it. But before doing so, you also need to make sure the module isn't attached to an ADAM system, otherwise there may be a memory conflict with potential for damaging the ADAM and/or the SGM. So the first step is to check if the 24KB are already available. If they are, then you have an ADAM system, and the SGM expanded memory should NOT be enabled. If no memory is found (in the 2000h-5FFF range), then it is a ColecoVision system and the expanded memory can be enabled. To do so, set bit0 in I/O port 53h to 1, like this (in assembly): LD A,00000001b ; OUT(53h),A
    You can also map 8KB of RAM (for a total of 32KB) to address range 0000h-1FFFh. However, when doing so, the ColecoVision BIOS will be disabled. RAM and BIOS can be mapped back and forth, though. To map RAM to the BIOS area, simply set bit1 in I/O port 7Fh to "0". In order to keep full compatibility with ADAM systems, all bits in I/O port 7Fh must be set to specific values, though. Here is how you should set the port: if you want the BIOS, set the port to "0001111b". If you want RAM instead, set the port to "0001101b" (see example above on how to do that in Assembly). Make sure you respect those values, or your game may not work on ADAM systems.

I plan to release complete ASM libraries for detecting ADAM, SGM, and then setting RAM appropriately. I recommend using those libraries, so that we eliminate the risk of enabling the SGM expanded RAM on ADAM systems.

Here are the steps required to initialize a Super Game Module game:

1) When initializing your game, use RAM in the 6000h-63FFh range, because the SGM expanded RAM is disabled by default, and you must make sure it isn't an ADAM before enabling it.
2) Run some memory tests to establish if RAM is already present in the 2000h-5FFFh. If it is, then you have an ADAM system. In that case, DO NOT enable the expanded RAM. Go to step 4
3) If no memory is found, then we have a ColecoVision system, and the SGM expanded RAM can be enabled. Do so by setting I/O port 53h to 01h. You shouldn't disable expanded RAM after that (i.e., do not access I/O port 53h again after initializing it).
4) Now that you know that 24KB of RAM is available, your game can start using it.
5) Optionally you can replace the BIOS with RAM, using I/O port 7Fh. Make sure you only use the two values described above ('00001111b', '00001101b'), or your game may not work on an ADAM system.

I final note about the SGM and ADAM: AFAIK, I/O port 53h isn't used by any known ADAM device, so it should be safe to use the SGM with all ADAM expansion cards in place. However I cannot guarantee that a future (or even current) device will not use (or may be already using) that specific port, so users should make sure there is no other device in their ADAM system using port 53h.

For ColecoVision users, there is no risk of conflict.
