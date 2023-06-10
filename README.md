# smartfin-fw2

This is the firmware for the Smartfin project.

## Target Environment
This firmware targets the 2g/3g/4g E Series SoMs from Particle.  We specifcally target the electron 3.0.0 OS.

Primary entry point is in src/smartfin-fw2.ino.

All product/firmware configuration flags should be defined in product.hpp.

The firmware is organized into primary tasks which form a state machine.  All behaviors are tied to tasks.

All hardware should have a corresponding driver, and should all be referenced from the global `pSystemDesc`.  This global is a structure containing pointers to all initialized drivers. Tasks must only reference hardware from this global, and should not reach into specific driver modules to grab global handles.
