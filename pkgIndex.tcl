# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded mqttc 0.16 \
	    [list load [file join $dir libtcl9mqttc0.16.so] [string totitle mqttc]]
} else {
    package ifneeded mqttc 0.16 \
	    [list load [file join $dir libmqttc0.16.so] [string totitle mqttc]]
}
