# PC/SC IFD Handler for swICC-based Cards
This is a PC/SC IFD (interface device) handler for [swICC-based cards](https://github.com/tomasz-lisowski/swicc). In other words this acts like a smart card reader but instead of being a driver for a physical device, it is instead an all-software card reader for swICC-based cards.

## Building
Make sure to have `make`, `gcc`, and `pkg-config` installed. Also make sure that you clone the repository recursively so that all sub-modules get cloned as expected. The dependencies are: `pcscd` (the pcsc-lite middleware daemon), `libpcsclite1` (library for the pcsc-lite middleware), and `libpcsclite-dev` (other dev files).
The make targets are as follows:
- **main**: This builds the IFD handler shared library.
- **main-dbg**: This builds a debug IFD handler shared library with debug information and the address sanitizer enabled.
- **clean**: Performs a cleanup of the project and all sub-modules.
- **install**: Install the IFD handler so it can get loaded by the PC/SC middleware.
- **uninstall**: Uninstall the IFD handler.

## Instructions
Make sure that the PC/SC middleware is not running. Install the IFD handler, then start the middleware again. The driver will be started immediately and wait for cards to connect to it. Note that **multiple cards can be connected at once**, the card limit is set by modifying `SWICC_NET_CLIENT_COUNT_MAX` in the swICC library.
