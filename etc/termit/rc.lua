require("termit.colormaps")
require("termit.utils")

defaults = {}
defaults.windowTitle = 'Termit'
defaults.tabName = 'Terminal'
defaults.encoding = 'UTF-8'
defaults.wordChars = '+-AA-Za-z0-9,./?%&#:_~'
defaults.font = 'Monospace 10'
defaults.showScrollbar = true
defaults.transparency = 0.7
defaults.hideSingleTab = true
defaults.hideMenubar = false
defaults.fillTabbar = false
defaults.scrollbackLines = 4096
defaults.geometry = '80x24'
defaults.allowChangingTitle = true
defaults.colormap = termit.colormaps.tango
setOptions(defaults)

setKbPolicy('keycode')

addMenu(termit.utils.encMenu(), "Encodings")
addPopupMenu(termit.utils.encMenu(), "Encodings")
