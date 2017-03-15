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


### Testing the *troll*

It is simple and easy to give the *troll* a try!
For debugging and evaluation purposes, the *troll* supports operation
in a so-called *static test-drive* mode.
In this mode, no connection to a live target system is established, and
instead the *troll* reads the contents of target memory and registers
from files. In this mode, the *troll* does not invoke any external
tools it otherwise relies upon, and all necessary data for the *troll*'s
operation is being fetched from files with hard-coded names.
This *static test-drive mode of operation* is intended for *troll*
rapid prototyping and debugging, and demonstration of the capabilities
of the *troll*.

To give the *troll* a try, first clone it from github, and initialize
and update submodules:
```sh
git clone https://github.com/stoyan-shopov/troll.git
cd troll
git submodule init
git submodule update
```
Next, build the troll. The *troll* makes use of the *Qt* framework.
I prefer building the *troll* from within the *Qt Creator* IDE, but
you may also build it by using the *qmake* and *make* utilities,
directly from the command line.

Finally, do obtain a *static test-drive* set of test files from here:
https://github.com/stoyan-shopov/troll-test-drive-snapshots

Extract the `troll-test-drive-files` directory in the *troll*'s working directory.


Now you are all set up, and ready to go - just run the *troll*.

