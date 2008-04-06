#!/bin/env python

import re

kb = re.compile('(Alt|Ctrl|Shift)-(left|right|insert|[a-zA-Z])')


vals = {}
vals['next_tab'] = 'Alt-right'
vals['prev_tab'] = 'Alt-Left'
vals['copy'] = 'Ctrl-insert'
vals['paste'] = 'Shift-insert'
vals['open_tab'] = 'Ctrl-t'
vals['close_tab'] = 'Ctrl-w'

badvals = {}
badvals[1] = 'Meta-a'

for val in vals:
    print val, vals[val], kb.match(vals[val])

for val in badvals:
    print val, badvals[val], kb.match(badvals[val])

