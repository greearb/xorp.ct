#! /usr/bin/env python

# $XORP: other/testbed/tools/xtreset.py,v 1.1.1.1 2002/12/11 23:55:14 hodson Exp $

import os
import getopt
import sys
import string
from xtvars import HOST

"""
Reset testbed machines. Using NET-SNMP.
All configuration is hardcoded in here.
"""

APCMIBLOC="-M /usr/local/xorp/mibs:/usr/local/share/snmp/mibs/ -m PowerNet-MIB"

dict = { "xorp0" : \
           [APCMIBLOC, "xorppwr1", "public", "private", "sPDUOutletCtl.1"],
         "xorp1" : \
           [APCMIBLOC, "xorppwr1", "public", "private", "sPDUOutletCtl.2"],
         "xorp2" : \
           [APCMIBLOC, "xorppwr1", "public", "private", "sPDUOutletCtl.3"],
         "xorp3" : \
           [APCMIBLOC, "xorppwr1", "public", "private", "sPDUOutletCtl.4"],
         "xorp4" : \
           [APCMIBLOC, "xorppwr1", "public", "private", "sPDUOutletCtl.5"],
         "xorp5" : \
           [APCMIBLOC, "xorppwr1", "public", "private", "sPDUOutletCtl.6"],
         "xorp-c4000" : \
           [APCMIBLOC, "xorppwr2", "public", "private", "sPDUOutletCtl.1"],
         "xorp6" : \
           [APCMIBLOC, "xorppwr2", "public", "private", "sPDUOutletCtl.2"],
         "xorp7" : \
           [APCMIBLOC, "xorppwr2", "public", "private", "sPDUOutletCtl.3"],
         "xorp8" : \
           [APCMIBLOC, "xorppwr2", "public", "private", "sPDUOutletCtl.4"],
         "xorp10" : \
           [APCMIBLOC, "xorppwr2", "public", "private", "sPDUOutletCtl.5"],
         "xorp11" : \
           [APCMIBLOC, "xorppwr2", "public", "private", "sPDUOutletCtl.6"],
         }

def cmdstring(cmd):
    """
    Generate the appropriate command string prefix
    """

    global HOST
    import os
    if HOST == os.uname()[1]:
        return cmd + " "
    else:
        return "ssh xorpc " + cmd + " "

def lookup(host):
    """
    Given a host return all its access parameters
    """

    try:
        l = dict[host]
        reboot = ["i", "3"]    # outletReboot  (3)
        l.extend(reboot)
        return 1, l
    except:
        return 0, []

    return 0, []

def snmpget(mibinfo, host, community, objectid):
    """
    Just map this call onto net-snmp
    """

    com = cmdstring("snmpget")
    for i in [mibinfo, host, community, objectid]:
        com += " " + i

    print com

    return command(com)

def get(host):
    """
    See if we know about this host and if we do retrieve is status
    """

    status, tup = lookup(host)
    if 0 == status:
        return 0, "Unknown host " + host

    return 1, snmpget(tup[0], tup[1], tup[2], tup[4])

def snmpset(mibinfo, host, community, objectid, type, value):
    """
    Just map this call onto net-snmp
    """

    com = cmdstring("snmpset")
    for i in [mibinfo, host, community, objectid, type, value]:
        com += " " + i

    print com

    return command(com)

def reboot(host):
    """
    Reboot the named host
    """

    # XXX
    #
    # A gross hack. If the host is "xorpsw1" then we can't use
    # snmp. Well strictly we can as it is on the apc masterswitch but
    # we would prefer not to.
    if "xorpsw1" == host:
        return reboot_hpswitch("xorpsw1")

    status, tup = lookup(host)
    if 0 == status:
        return 0, "Unknown host " + host

    return 1, snmpset(tup[0], tup[1], tup[3], tup[4], tup[5], tup[6])

def reboot_hpswitch(host):
    """
    Reboot a HP Switch using the telnet interface
    """

    global HOST
    import os
    if HOST != os.uname()[1]:
        return 0, "Sorry must be on " + HOST
    
    import socket
    import select
    from string import join, printable, rfind
##    host = "localhost"
##    port = 9004
    port = 23 # telnet port
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    s.send("\n6 \n")
    res = ""
    while 1:
        i, o, e = select.select([], [s], [s], 5)
        if e != []:
            break
        elif o != []:
            line = s.recv(1024)
            if not line:
                break
            else:
                res += line
##                res += [i for i in line if i in printable]
##                for l in line:
##                    if l in printable:
##                        res += l
##                        #sys.stdout.write(l)
        else:
            break
    s.close()

    return 1, remove_vt100(res)[-22:]

def db(debug, s, pos, len):
    """
    Debugging routine for remove_vt100
    """
    if 0 == debug:
        return

    print s[pos + 1 : pos + len + 1]
    
def remove_vt100(s):
    """
    Remove vt100 dross from string
    """

    debug = 0
    res = ""
    
    count = len(s)
    pos = 0;
    while pos < count:
        if 27 == ord(s[pos]):
            if "[" == s[pos + 1]:
                if "?" == s[pos + 2]:           # ESC [ ? 7 l
                    off = 4
                    if s[pos + off] in string.digits:
                        off += 1
                    db(debug, s, pos, off)
                    pos += off
                elif ";" == s[pos + 3]:         # ESC [ pl ; pc f
                    off = 5
                    if s[pos + off] in string.digits:
                        off += 1
                    db(debug, s, pos, off)
                    pos += off
                elif ";" == s[pos + 4]:         # ESC [ pl ; pc f
                    off = 6
                    if s[pos + off] in string.digits:
                        off += 1
                    db(debug, s, pos, off)
                    pos += off
                else:                           # ESC M
                    db(debug, s, pos, 3)
                    pos += 3
            else:
                pos += 1
        else:
            res += s[pos]
        pos += 1

    return res

def command(com):
    """
    Run command
    """

    stdin, stdout_stderr = os.popen4(com)
    stdin.close()
    all = ""
    while 1:
	lines = stdout_stderr.readline()
	if not lines:
	    break
        all += lines
    stdout_stderr.close()

    return all

us= \
"""\
usage: %s [-h|--help] [-d|--debug] -g|--get | -r|--reboot host"""

def main():
    def usage():
        global us
        print us % sys.argv[0]

    try:
	opts, args = getopt.getopt(sys.argv[1:], "hgrd", \
				   ["help", \
                                    "get", \
                                    "reboot", \
                                    "debug"])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    g = 0
    r = 0
    debug = 0
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
        if o in ("-g", "--get"):
            g = 1
	if o in ("-r", "--reboot"):
	    r = 1
	if o in ("-d", "--debug"):
	    debug = 1

    if args:
        h = args[0]
    else:
        h = ""

    if 0 == g and 0 == r:
        usage()
        sys.exit(1)

    if ((1 == g) or (1 == r)) and h != "":
        if 1 == g:
            status, result = get(h)
        if 1 == r:
            status, result = reboot(h)
        if 0 == status:
            print >> sys.stderr, result
        else:
            print result,

if __name__ == '__main__':
    main()

#print snmpget(APCMIBLOC, "xorppwr1", "public", "sPDUOutletCtl.1"),
