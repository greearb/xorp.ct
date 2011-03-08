#!/bin/bash

TARG=www.xorp.org:/home/VHOSTS/xorp.org/html

make

scp *.html *.ico *.png *.css robots.txt $TARG/
scp images/*.png images/*.gif $TARG/images/
scp images/sponsors/* $TARG/images/sponsors/
scp images/livecd/* $TARG/images/livecd/
scp images/mad/* $TARG/images/mad/



