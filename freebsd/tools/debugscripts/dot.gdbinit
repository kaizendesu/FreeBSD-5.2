# $FreeBSD$
# .gdbinit file for remote serial debugging.
# see gdbinit(9) for further details.
#
# The following lines (down to "end" comment) may need to be changed
set output-radix 16
set height 70
set width 120

# Connect to remote target
define tr
set remotebaud 9600
# Remote debugging port
target remote /dev/cuaa0
end

# Get symbols from klds.  This is a little fiddly, but very fast.
define getsyms
# This should be the path of the real modules directory.
shell asf -f -k MODPATH
source .asf
end

# End of things you're likely to need to change.

set remotetimeout 1
set complaints 1
set print pretty
dir ../../..
document tr
Attach to a remote kernel via serial port
end

source gdbinit.kernel
source gdbinit.vinum
source gdbinit.machine

echo Ready to go.  Enter 'tr' to connect to remote target\n
echo and 'getsyms' after connection to load kld symbols.\n
