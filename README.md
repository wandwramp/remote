# `remote`

Remote is a terminal emulator made to interface with a physical WRAMP board.
It is based on `minicom`, written by Miquel van Smoorenburg.

It communicates with a WRAMP board whose first serial port is accessible as
a device within the Linux filesystem, at a location defined in `remoterc.dfl`.
Most commonly, this will be a USB port which appears at /dev/ttyUSBX, where X
is a number.

`remoterc.dfl` is found in a location specified at build time by the BINDIR
variable in the [Makefile](Makefile).
Alternatively, a file named `.remoterc.dfl` can be created in the user's home
directory. An example of a configuration file can be found at [remoterc.dfl](remoterc.dfl).

The main relevant entries are `pu pprog1`, `pu port`, and `pu baudrate`.
* `pu pprog1` should be the location of a `down` binary as built by this project.
Alternatively, place `down` in a location seen by your $PATH and give pprog1
a value of `down`.
* `pu port` should be the location of the serial port attached to a WRAMP board.
* `pu baudrate` should be the baud rate that the serial port is running at.
WRAMPmon will use a value of 38400 by default.

## Usage

Once the configuration file is valid and in place, simply ensure the WRAMP
board is plugged in and turned on, and run `remote`.
Once WRAMP is reset, its welcome message will appear on the screen.
`remote` can be used like any other terminal emulator, but the behaviour of
the responses might be determined by user code running on WRAMP.

When loading a program, pressing CTRL-A S will allow you to specify a file to
send. The filename is relative to the working directory `remote` was launched
from. `down` will then send the file to WRAMPmon.

## Building

The [Makefile](Makefile) is set up for a Linux environment with ncurses installed.
Once the desired location for `remoterc.dfl` is set by the BINDIR variable,
run `make`.
Two binaries should be generated: `down` and `remote`.
