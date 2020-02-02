#!/usr/bin/env python2

# $XORP: xorp/bgp/harness/harness.py,v 1.1 2005/03/23 19:58:44 atanu Exp $

# Use the test harness code to establish a BGP session.
# This session can be used to inject updates or to log all data from the peer.
# The MRTD data format is used.

# Preconditions:
# xorp_finder
# coord
# test_peer -s peer1

import os
import time
import sys
import getopt


CALL_XRL_LOCATION="../../sbin/call_xrl"
if (not os.path.isfile(CALL_XRL_LOCATION)):
    CALL_XRL_LOCATION="../../libxipc/call_xrl"
    
def call_xrl(command):
    """
    Call an XRL
    """

    #print command

    global CALL_XRL_LOCATION
    stdout = os.popen(CALL_XRL_LOCATION + " " + "\"" + command + "\"", 'r')
    out=""
    while 1:
        lines = stdout.readline()
        if not lines:
            break
        out += lines
    stdout.close()

    return out

def coord(command):
    """
    Send a command to the coordinator
    """

    call_xrl("finder://coord/coord/0.1/command?command:txt=%s" % command)

    # Wait up to five seconds for this command to complete
    for i in range(5):
        if pending() == 0:
            return
        time.sleep(1)

    print >> sys.stderr, "Still pending"

def pending():
    """
    Check the previous command has completed
    """

    ret=call_xrl("finder://coord/coord/0.1/pending")
    if ret == "pending:bool=false\n":
        return 0
    else:
        return 1

def establish(host, myas, myid, ipv4, logfile, remove_logfile):
    """
    Establish a session with host and conditionally log the whole session.
    """

    coord("reset")
    coord("target %s 179" % host)
    coord("initialise attach peer1")

    if logfile:
        if remove_logfile:
            os.unlink(logfile)
        coord("peer1 dump sent mrtd ipv4 traffic %s" % logfile)
        coord("peer1 dump recv mrtd ipv4 traffic %s" % logfile)
    
    if ipv4:
        family = ""
    else:
        family = "ipv6 true"
        
    coord("peer1 establish AS %s holdtime 0 id %s keepalive false %s" %
          (myas, myid, family))

def inject(injectfile, count):
    """
    Inject count update packets into the session. If count is zero inject
    whole file.
    """

    if 0 == count:
        coord("peer1 send dump mrtd update %s" % injectfile)
    else:
         coord("peer1 send dump mrtd update %s %s" % (injectfile, count))
   
def stop():
    """
    Stop collecting data into the log file
    """

    coord("peer1 dump sent mrtd ipv4 traffic")
    coord("peer1 dump recv mrtd ipv4 traffic")

us= \
"""\
usage: %s [-h|--help]
\t -e -p peer -a AS -b BGP-ID [-i injectfile] [-c route_count] [-l logfile]
\t -r
\t -s
\t -4
\t -6
"""

def main():
    def usage():
        global us
        print us % sys.argv[0]

    try:
	opts, args = getopt.getopt(sys.argv[1:], "hei:c:p:a:b:l:rs46", \
				   ["help", \
                                    "establish", \
                                    "inject=", \
                                    "count=", \
                                    "peer=", \
                                    "asnum=", \
                                    "bgp-id=", \
                                    "logfile=", \
                                    "remove", \
                                    "stop", \
                                    "ipv4", \
                                    "ipv6", \
                                    ])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    establishp = False
    injectp = False
    stopp = False
    ipv4 = True
    peer = ""
    asnum = "0"
    router_id = "10.0.0.1"
    injectfile = None
    route_count = 0
    logfile = None
    remove_logfile = False
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
        if o in ("-e", "--establish"):
            establishp = True
        if o in ("-i", "--inject"):
            injectp = True
            injectfile = a
        if o in ("-c", "--count"):
            route_count = a
        if o in ("-p", "--peer"):
            peer = a
	if o in ("-a", "--asnum"):
	    asnum = a
	if o in ("-b", "--bgp-id"):
	    router_id = a
	if o in ("-l", "--logfile"):
	    logfile = a
	if o in ("-r", "--remove"):
	    remove_logfile = True
	if o in ("-s", "--stop"):
            stopp = True
	if o in ("-4", "--ipv4"):
            ipv4 = True
	if o in ("-6", "--ipv6"):
            ipv4 = False
        
    if establishp:
        establish(peer, asnum, router_id, ipv4, logfile, remove_logfile)

    if injectp:
        inject(injectfile, route_count)

    if stopp:
        stop()

    if (not establish) and (not injectp) and (not stopp):
        usage()
        sys.exit(1)

if __name__ == '__main__':
    main()
