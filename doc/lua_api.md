## Functions

#### `activateTab(tab_index)`
Activates tab by index.
+ `tab_index` - index of tab to activate. Index of the first tab is 1.

#### `addMenu(menu)`
Adds menu in menubar.
+ `menu` - table of _TermitMenuItem_ type.

#### `addPopupMenu(menu)`
Adds menu in popup menu, similar to `addMenu`.
+ `menu` - table of _TermitMenuItem_ type.

#### `bindKey(keys, luaFunction)`
Sets new keybinding. If **luaFunction** is nil removes keybinding.
+ `keys` - _string_ with keybinding. Available modifiers are **Alt** **Ctrl** **Shift** **Meta** **Super** **Hyper**.
+ `luaFunction` - callback _function_
  > Example: don't close tab with **Ctrl-w**, use **CtrlShift-w**
  > ```lua
  > bindKey('Ctrl-w', nil)
  > bindKey('CtrlShift-w', closeTab)
  > ```

#### `bindMouse(event, luaFunction)`
Sets new mouse-binding. If `luaFunction` is `nil` removes mouse-binding.
+ `event` - _string_ with one of those values: **DoubleClick**
+ `luaFunction` - callback _function_

#### `closeTab()`
Closes active tab.

#### `copy()`
Copies selection into tab's buffer.

#### `currentTab()`
Returns current tab of _TermitTabInfo_.

#### `currentTabIndex()`
Returns current tab index.

#### `feed(data)`
Interprets data as if it were data received from a terminal process.

#### `feedChild(data)`
Sends a data to the terminal as if it were data entered by the user at the keyboard. 

#### `findDlg()`
Opens find entry. _(Enabled when build with vte version >= 0.26)_

#### `findNext()`
Searches the next string matching search regex.

#### `findPrev()`
Searches the previous string matching search regex.

#### `forEachRow(func)`
For each terminal row in entire scrollback buffer executes function passing row as argument.
+ `func` - _function_ to be called.

#### `forEachVisibleRow(func)`
For each visible terminal row executes function passing row as argument.
+ `func` - _function_ to be called.

#### `loadSessionDlg()`
Displays "Load session" dialog.

#### `nextTab()`
Activates next tab.

#### `openTab(tabInfo)`
Opens new tab.
+ `tabinfo` - _table_ of _TermitTabInfo_ with tab settings.
  > Example:
  > ```lua
  > tabInfo = {}
  > tabInfo.title = 'Zsh tab'
  > tabInfo.command = 'zsh'
  > tabInfo.encoding = 'UTF-8'
  > tabInfo.workingDir = '/tmp'
  > openTab(tabInfo)
  > ```

#### `paste()`
Pastes tab's buffer.

#### `preferencesDlg()`
Displays "Preferences" dialog.

#### `prevTab()`
Activates previous tab.

#### `quit()`
Quit.

#### `reconfigure()`
Sets all configurable variables to defaults and forces rereading `rc.lua`.

#### `saveSessionDlg()`
Displays "Save session" dialog.

#### `selection()`
Returns selected text from current tab.

#### `setColormap(colormap)`
Changes colormap for active tab.
+ `colormap` - _array_ with 8 or 16 or 24 colors.

#### `setEncoding(encoding)`
Changes encoding for active tab.
+ `encoding` - _string_ with encoding name.
  > Example:
  > ```lua
  > setEncoding('UTF-8')
  > ```

#### `setKbPolicy(kb_policy)`
Sets keyuboard policy for shortcuts.
+ `kb_policy` - _string_ with one of those values:
  * `'keycode'` - use hardware keyboard codes (XkbLayout-independent)
  * `'keysym'` - use keysym values (XkbLayout-dependent)
  > Example: treat keys via current XkbLayout
  > ```lua
  > setKbPolicy('keysym')
  > ```

#### `setOptions(opts)`
Changes default options.
+ `opts` - _table_ of TermitOptions type with new options:

#### `setTabBackgroundColor(color)`
Changes background color for active tab.
+ `color` - _string_ with new color.

#### `setTabFont(font)`
Changes font for active tab.
+ `font` - _string_ with new font.

#### `setTabForegroundColor(color)`
Changes foreground (e.g. font) color for active tab.
+ `color` - _string_ with new color.

#### `setTabTitle(tabTitle)`
Changes title for active tab.
+ `tabTitle` - _string_ with new tab title.

#### `setTabTitleDlg()`
Displays "Set tab title" dialog.

#### `setWindowTitle(title)`
Changes termit window title.
+ `title` - _string_ with new title.

#### `spawn(command)`
Spawns command (works via shell).
+ `command` - string with command and arguments.

#### `toggleMenubar()`
Displays or hides menubar.

#### `toggleTabbar()`
Displays or hides tabbar.

## Types

#### `TermitCursorBlinkMode`
one of those _string_ values:
* `'System'` - Follow GTK+ settings for cursor blinking
* `'BlinkOn'` - Cursor blinks
* `'BlinkOff'` - Cursor does not blink

#### `TermitCursorShape`
one of those _string_ values:
* `'Block'` - Draw a block cursor
* `'Ibeam'` - Draw a vertical bar on the left side of character
* `'Underline'` - Draw a horizontal bar below the character

#### `TermitEraseBinding`
one of those _string_ values:
* `'Auto'` - `VTE_ERASE_AUTO`
* `'AsciiBksp'` - `VTE_ERASE_ASCII_BACKSPACE`
* `'AsciiDel'` - `VTE_ERASE_ASCII_DELETE`
* `'EraseDel'` - `VTE_ERASE_DELETE_SEQUENCE`
* `'EraseTty'` - `VTE_ERASE_TTY`
  >  For detailed description look into Vte docs.

#### `TermitKeybindings`
_table_ with predefined keybindings.
* `closeTab` - **Ctrl-w**
* `copy` - **Ctrl-Insert**
* `nextTab` - **Alt-Right**
* `openTab` - **Ctrl-t**
* `paste` - **Shift-Insert**
* `prevTab` - **Alt-Left**
  > Example: enable Gnome-like tab switching
  > ```lua
  > keys = {}
  > keys.nextTab = 'Ctrl-Page_Down'
  > keys.prevTab = 'Ctrl-Page_Up'
  > setKeys(keys)
  > ```

#### `TermitMenuItem`
_table_ for menuitems.
* `accel` - _string_ accelerator for menuitem. String with keybinding
* `action` - _function_ lua-function to execute when item activated
* `name` - _string_ name for menuitem

#### `TermitOptions`
_table_ with termit options.
* `allowChangingTitle` - _boolean_ auto change title (similar to xterm)
* `audibleBell` - _boolean_ enables audible bell
* `backgroundColor` - _string_ background color
* `backspaceBinding` - _TermitEraseBinding_ reaction on backspace key
* `colormap` - _array_ with 8 or 16 or 24 colors
* `cursorBlinkMode` - _TermitCursorBlinkMode_ cursor blink mode
* `cursorShape` - _TermitCursorShape_ cursor shape
* `deleteBinding` - _TermitEraseBinding_ reaction on delete key
* `encoding` - _string_ default encoding
* `fillTabbar` - _boolean_ expand tabs' titles to fill whole tabbar
* `font` - _string_ font name
* `foregroundColor` - _string_ foreground color
* `geometry` - _string_ cols x rows to start with
* `getTabTitle` - lua _function_ to generate new tab title
* `getWindowTitle` - lua _function_ to generate new window title
* `hideMenubar` - _boolean_ hide menubar
* `hideTabbar` - _boolean_ hide tabbar
* `hideSingleTab` - _boolean_ hide tabbar when only 1 tab present
* `hideTitlebarWhenMaximized` - _boolean_ hide window titlebar when mazimized
* `matches` - _table_ with items of TermitMatch type
* `scrollbackLines` - _number_ the length of scrollback buffer
* `scrollOnKeystroke` - _boolean_ enables scroll to the bottom when the user presses a key
* `scrollOnOutput` - _boolean_ enables scroll to the bottom when new data is received
* `setStatusbar` - lua _function_ to generate new statusbar message
* `showBorder` - _boolean_ show notebook border
* `showScrollbar` - _boolean_ display scrollbar
* `startMaximized` - _boolean_ maximize window on start
* `tabName` - _string_ default tab name
* `tabPos` - tabbar position (Top, Bottom, Left, Right)
* `tabs` - _table_ with items of _TermitTabInfo_ type
* `urgencyOnBell` - _boolean_ set WM-hint 'urgent' on termit window when bell
* `wordCharExceptions` - _string_ additional word characters (double click selects word)

#### `TermitTabInfo`
_table_ with tab settings:
+ `command`
+ `encoding`
+ `font` - font _string_
+ `fontSize` - font size
+ `pid` - process id
+ `title`
+ `workingDir`

## Globals
- `tabs` - Readonly _table_ with tab settings, access specific tabs by index.

## Bugs
- After start sometimes there is black screen. Resizing termit window helps.
- In options table 'tabs' field should be the last one. When loading all settings are applied in the same order as they are set in options table. So if you set tabs and only then colormap, these tabs would have default colormap.
