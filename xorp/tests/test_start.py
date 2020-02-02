#!/usr/bin/env python2

# Copyright (c) 2001-2009 XORP, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
# 
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $XORP: xorp/tests/test_start.py,v 1.8 2008/10/02 21:58:30 bms Exp $

import getopt,threading,time,sys
from test_process import Process
from test_builddir import builddir

class Start:
    """
    Start the router manager and the test harness processes.
    """

    def __init__(self, builddir="..", verbose = False):
        self.builddir = builddir
        self.plist = []
        self.verbose = verbose
        
    def start(self):
        """
        Start all the processes
        """

        rtrmgr = self.builddir + "rtrmgr/xorp_rtrmgr -t templates -b empty.boot"
        self.__start_process(rtrmgr)

        time.sleep(5)

        coord = self.builddir + "bgp/harness/coord"
        self.__start_process(coord)

        if self.verbose:
            peer = self.builddir + "bgp/harness/test_peer -t -v"
        else:
            peer = self.builddir + "bgp/harness/test_peer"
        for i in ["peer1", "peer2", "peer3"]:
            self.__start_process(peer + " -s " + i)

    def __start_process(self, process):
        """
        Start a single process and add it to the list
        """

        p = Process(command=process)
        p.start();
        self.plist.append(p)

    def check(self):
        """
        Make sure all the processes are still running
        """

        for i in self.plist:
            print "Testing: ", i.command()
            status = i.status()
            if "TERMINATED" == status:
                return False
            if "RUNNING" == status:
                continue
            if "INIT" == status:
                for a in range(5):
                    time.sleep(1)
                    if "RUNNING" == i.status():
                        break
                print "Not running", i.command(), i.status()

        return True

    def terminate(self):
        """
        Stop all the processes
        """

        for i in self.plist:
            i.terminate()
        
if __name__ == '__main__':
    def usage():
        us = "usage: %s [-h|--help] [-v|--verbose]"
        print us % sys.argv[0]

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hv", \
                                   ["help", \
                                    "verbose", \
                                    ])
    except getopt.GetoptError:
        usage()
        sys.exit(-1)

    verbose = False
    
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
        if o in ("-v", "--verbose"):
            verbose = True
    
    s = Start(builddir=builddir(), verbose=verbose)
    s.start()
    if not s.check():
        print "Processes did not start"
        s.terminate()
        sys.exit(-1)
    print "Hit return to kill processes"
    sys.stdin.readline()
    print "About to terminate processes"
    if not s.check():
        print "Processes no longer running"
        s.terminate()
        sys.exit(-1)
    s.terminate()
    sys.exit(0)

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
