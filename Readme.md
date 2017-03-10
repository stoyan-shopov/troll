# Troll

The *troll* is a C-language source-level debugger for *ARM Cortex-M* systems,
accessed with the excellent *blackmagic* hardware debug probe, and a customized
variant of the *blackmagic* - the *vx/blackstrike* (or *blackstrike* for short).
The *troll* only supports source-level debugging of source code programs
written in the *C* programming language, compiled to executable files in the
*ELF* format, containig *DWARF* debug information.

Note that this is a very special case of a debugger:
- it only targets *ARM* Cortex-M based systems
- only the C language is supported for source-level debugging
- only the *blackmagic* and *blackstrike* hardware debug probes are supported
- tre *troll* itself is written in *C++*
- the *troll* itself heavily relies on the Qt framework

Here is a screenshot of the *troll*'s graphical user interface, as of 28022017:

![alt text](documentation/troll-screenshot-28022017-complete.png)

This is an annotated version of the screenshot above, showing the various display
views of the *troll*:

![alt text](documentation/troll-screenshot-28022017-complete-annotated.png)

Here is a simplified, more realistic in everyday usage, view, as of 28022017::

![alt text](documentation/troll-screenshot-28022017-basic.png)

