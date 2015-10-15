defaults = {}
defaults.windowTitle = 'Termit'
defaults.tabName = 'Terminal'
defaults.encoding = 'UTF-8'
defaults.wordChars = '+-AA-Za-z0-9,./?%&#:_~'
defaults.scrollbackLines = 4096
defaults.font = 'Monospace 10'
defaults.geometry = '80x24'
defaults.hideSingleTab = false
defaults.showScrollbar = true
defaults.fillTabbar = false
defaults.topMenu = false
defaults.hideMenubar = false
defaults.allowChangingTitle = false
defaults.visibleBell = false
defaults.audibleBell = false
defaults.urgencyOnBell = false
setOptions(defaults)

setKbPolicy('keysym')
