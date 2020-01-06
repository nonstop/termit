colormaps = require('termit.colormaps')
utils = require('termit.utils')

defaults = {}
defaults.windowTitle = 'Termit'
defaults.tabName = 'Terminal'
defaults.encoding = 'UTF-8'
defaults.scrollbackLines = 4096
defaults.font = 'Monospace 10'
defaults.geometry = '80x24'
defaults.hideSingleTab = false
defaults.showScrollbar = true
defaults.fillTabbar = false
defaults.hideMenubar = false
defaults.allowChangingTitle = false
defaults.audibleBell = false
defaults.urgencyOnBell = false
setOptions(defaults)

setKbPolicy('keysym')
