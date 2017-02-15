# Troll

The *troll* is a C-language source-level debugger for **ARM Cortex-M** systems,
accessed with the excellent *blackmagic* hardware debug probe, and a customized
variant of the *blackmagic* - the *vx/blackstrike* (or *blackstrike* for short).
The *troll* only supports source-level debugging of executable files in the
**ELF** format, containig **DWARF** debug information.

Note that this is a very special case of a debugger:
- it only targets **ARM** Cortex-M based systems
- only the C language is supported for source-level debugging
- only the *blackmagic* and *blackstrike* hardware debug probes are supported

### History and motivation

I am working with embedded systems, it just happened to be so.
In embedded systems, the predominant language used is *C*.
I have mostly worked with **ARM** systems, *ARM7TDMI-S* a long
time ago, and currently*Cortex-M*. I am used to working on a lower level than most
of my colleagues are accustomed to. I mostly use *vim* and *make* instead
of some integrated development environment. I mainly use the *gcc*
compiler, and do my debugging in command-line *gdb*. This is a
very fast and convenient work flow for me. Years ago, I used
*j-link* clones with *gdb*. I did give *OpenOCD* a try once,
then after a week of hunting witches I found that *OpenOCD*
misprogrammed the chips I was using, I used a *wiggler* clone.
I gave up in disgust, and never bothered to look at *OpenOCD*
again, maybe things are better now. Anyway, I felt it is a
shame that no one had created an affordable, easy to build,
modify and use hardware debug probe for *ARM* targets, I
alredy knew that all the information to do so is publicly
available directly from *ARM*. My first attempts were with
a parallel port *wiggler* targeting an *ARM7TDMI-S* *LPC* chip,
I intended this as a proof-of-concept system, and to then
make some more convenient to use custom board. Some things
happened back then and I halted work on this project for
a couple of years. When I decided to resume work on the
project, there were already available demonstration boards
from various manufacturers, I purchased some *ST* demonstration
board with an embedded *st-link* system, and used it as
a development board to reprogram the *st-link* chip itself.
I already had working serial-wire-debug (*SWD*) code for accessing the target,
and an embedded gdb server, but I enumerated over usb as
a custom usb device, and for that reason I needed a small
proxy program to make connection from *gdb* to the debug
probe possible. This was very inconvenient.
Around that time I discovered the excellent *blackmagic* probe.
  
The *troll* is my third attempt to write an **ELF/DWARF** debugger for **ARM**
targets. My first attempt (more than 10 years ago) was a system I named
the *gear*, I wrote it purely in *C*, it targeted **ARM7TDMI** systems,
and the only hardware probe I had back then was a parallel port *wiggler*
clone from **Olimex**. I failed at this attempt. My second attempt was
some 4 years ago, I named it the *trollgear*, I coded it in *C++* this
time, with some parts taken from the first system, the *gear*. At that time,
*st-link* fitted boards were already abundant, I had some *stm32f103* based
board, I read the **ARM ADIv5** documents, and successfully reprogrammed
the board with firmware I wrote to access **ARM Cortex-M** chips over the
serial-wire-debug (**SWD**) protocol. That worked, but was inconvenient to use,
mainly because I used a custom usb-class device, for which I used the *WinUsb*
library. My memory does not serve me too well aready, but it must have been
about that time that I discovered the *blackmagic* project, and the *libopencm3*
library. About that time, I made a simple board with an *stm32f103rb* chip,
which I named the *vortex*, or *vx* for short - a hardware debug probe for
*Cortex* systems. I used a stock *gdbserver* template from the *gdb* source-code
base for a driver, and adopted a **cdc-acm** usb interface in the spirit of
the *blackmagic* as a means of interfacing with a usb host. 
I worked intermittenlty on the *blackstrike* for a couple more years before deciding
to give up this project, and instead us the *blackmagic* firmware,
customized for my board.

