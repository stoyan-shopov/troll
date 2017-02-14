# Troll

The *troll* is a C-language source-level debugger for ARM Cortex-M systems,
accessed with the excellent *blackmagic* hardware debug probe, and a customized
variant of the *blackmagic* - the *vx/blackstrike* (or *blackstrike* for short).
The *troll* only supports source-level debugging of executable files in the
**ELF** format, containig **DWARF** debug information.

Note that this is a very special case of a debugger:
- it only targets ARM Cortex-M based systems
- only the C language is supported for source-level debugging
- only the *blackmagic* and *blackstrike* hardware debug probes are supported

### History and motivation
The *troll* is my third attempt to write an **ELF/DWARF** debugger for ARM
targets. My first attempt (more than 10 years ago) was a system I named
the *gear*, I wrote it purely in *C*, it targeted **ARM7TDMI** systems,
and the only hardware probe I had back then was a parallel port *wiggler*
clone from **Olimex**. I failed at this attempt. My second attempt was
some 4 years ago, I named it the *trollgear*, I coded it in *C++* this
time, with some parts taken from the first system, the *gear*. At that time,
*st-link* fitted boards were already abundant, I had some *stm32f103* based
board, I read the **ARM ADIv5** documents back then, and successfully reprogrammed
the board with firmware I wrote to access **ARM Cortex-M** chips over the
serial-wire-debug (**SWD**) protocol. That worked, but was inconvenient to use,
mainly because I used a custom usb-class device, for which I used the *WinUsb*
library. My memory does not serve me too well aready, but it must have been
about that time that I discovered the *blackmagic* project, and the *libopencm3*
library. About that time, I made a simple board with an *stm32f103rb* chip,
which I named the *vortex*, or *vx* for short - a hardware debug probe for
*Cortex* systems. I used a stock *gdbserver* template from the *gdb* source-code
base for a driver, and adopted a **cdc-acm** usb interface in the spirit of
the *blackmagic* as a means of interfacing with a usb host. Well, my memory
really serves me bad, it might have been a bit before discovering the
*blackmagic*, whatever, the name *blackstrike* has ***nothing to do***
with the *blackmagic*. 
