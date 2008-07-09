#
# $XORP$ 
#

# This script strips the user and creation specific comments out of
# fig2dev generated postscript files.  This is basically a hack to
# stop needless commits since, for whatever reason, sometimes the
# datestamps on the fig files end up newer than the eps files and the
# eps files then get regenerated.

/^%%Creat/ d
/^%%For:/ d
