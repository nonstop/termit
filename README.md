Termit
======

Simple terminal emulator based on vte library, extensible via lua.

Features:

* multiple tabs
* switching encodings
* sessions
* configurable keybindings
* embedded Lua

Configuration can be changed via $HOME/.config/termit/rc.lua file.
Example configurations can be found in doc/rc.lua.example.

Command-line arguments:

    termit --help
    Options:
      -h, --help             - print this help message
      -v, --version          - print version number
      -e, --execute          - execute command
      -i, --init=init_file   - use init_file instead of standard rc.lua
      -n, --name=name        - set window name hint
      -c, --class=class      - set window class hint
      -r, --role=role        - set window role (Gtk hint)
      -T, --title=title      - set window title

Keybindings can be configured via rc.lua.
Defaults are:

    Alt-Left     -  previous tab
    Alt-Right    -  next tab
    Ctrl-t       -  open tab
    Ctlr-w       -  close tab
    Ctrl-Insert  -  copy
    Shift-Insert -  paste

Lua API is described in doc/lua_api.md and in man page.

Download termit from [here](https://github.com/nonstop/termit/releases).
