# termit
Terminal emulator based on VTE library with Lua scripting

## Installation Instructions
[General guide](INSTALL)
### Prepare dependancies
#### fedora
```bash
# dnf -y install cmake gcc gtk+ vte291 vte291-devel lua lua-devel
```

#### ubuntu
```bash
# apt-get install -y gcc cmake lua5.4-dev libgtk-3-dev libvte-2.91 gettext
```

### build
Download, configure, build and install
```bash
$ git clone https://github.com/nonstop/termit.git
$ cd termit
$ cmake -DCMAKE_BUILD_TYPE=Release .
$ make
$ sudo make install
```

## [Changelog](ChangeLog)

## [Copyright](COPYING)
