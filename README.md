cecd
====

Shitty hack to work around broken CEC logic in XBMC.
XBMC will just not execute any CEC command when it can't find a TV in the CEC network.
I switch off my TV with sispmctl when i listen to music in the evening, so volup/down is broken.

This hack is basicaly throwing together the examples from libubus and libcec to create a frankendaemon that will respond to

    ubus-invoke /var/ubus/cec volup

on keypress in keymap.xml.


since xbmcs Exec() crashes, i use ExecScript(bla.py) which does os.system()


