# Install

## Make Targets
- `main`: This builds the IFD handler shared library.
- `main-dbg`: This builds a debug IFD handler shared library with debug information.
- `clean`: Performs a cleanup of the project and all sub-modules.
- `install`: Install the IFD handler so it can get loaded by the PC/SC middleware.
- `uninstall`: Uninstall the IFD handler.

## Distro-Specific Steps

### Arch
- Tested on Arch 20221120.0.103865.
1. `sudo pacman -S openssh git make cmake pkg-config gcc pcsclite`
2. `git clone --recurse-submodules git@github.com:tomasz-lisowski/swicc-drv-ifd.git`
3. `cd swicc-drv-ifd`
4. `make main-dbg`
5. `sudo make install`

### Fedora
- Tested on Fedora 37.
1. `sudo dnf install git make cmake gcc pkg-config pcsc-lite-libs pcsc-lite-devel pcsc-lite`
2. `git clone --recurse-submodules git@github.com:tomasz-lisowski/swicc-drv-ifd.git`
3. `cd swicc-drv-ifd`
4. `make main-dbg`
5. `sudo make install`

### openSUSE
- Tested on openSUSE Leap 15.4.
1. `zypper install git make cmake pkg-config gcc11 libpcsclite1 pcsc-lite-devel pcsc-lite`
2. `update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11`
3. `git clone --recurse-submodules git@github.com:tomasz-lisowski/swicc-drv-ifd.git`
4. `cd swicc-drv-ifd`
5. `make main-dbg`
6. `sudo make install`
