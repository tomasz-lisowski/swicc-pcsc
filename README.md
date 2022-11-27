# PC/SC IFD Handler for swICC-based Cards

> Project **needs** to be cloned recursively. Downloading the ZIP is not enough.

This is a PC/SC IFD (interface device) handler for [swICC-based cards](https://github.com/tomasz-lisowski/swicc). In other words this acts like a smart card reader but instead of being a driver for a physical device, it is instead an all-software card reader for swICC-based cards.

## Install
- These instructions are for Debian and Ubuntu, for Arch, Fedora, and openSUSE take a look at `./doc/install.md`.
- You need `make`, `cmake`, `gcc`, `pkg-config`, `libpcsclite1` (library for the pcsc-lite middleware), and `libpcsclite-dev` (other dev files for pcsc-lite) to compile the project. At runtime you will need the `pcscd` package which contains the pcsc-lite daemon.
1. `sudo apt-get install make cmake gcc pkg-config libpcsclite1 libpcsclite-dev pcscd`
2. `git clone --recurse-submodules git@github.com:tomasz-lisowski/swicc-drv-ifd.git`
3. `cd swicc-drv-ifd`
4. `make main-dbg` (for more info on building and installing, take a look at `./doc/install.md`).
5. `sudo make install`

## Usage
1. `pkill -x pcscd` to stop the PC/SC daemon.
2. In a separate terminal run `sudo pcscd -f -d -T`. This will run the PC/SC daemon in the foreground with debug output.
3. The swICC PC/SC reader will wait for cards to connect to it.
4. Once a card connects to the reader, it can be interacted with, just like with a real smart card connected to a hardware card reader.

Note that **multiple cards can be connected at once**, the card limit is set by modifying `SWICC_NET_CLIENT_COUNT_MAX` in the swICC library.
