#!/usr/bin/env python

# Copyright (c) 2001-2007 International Computer Science Institute
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software")
# to deal in the Software without restriction, subject to the conditions
# listed in the XORP LICENSE file. These conditions include: you must
# preserve this copyright notice, and you cannot mention the copyright
# holders in advertising related to the Software without their permission.
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
# notice is a summary of the XORP LICENSE file; the license in that file is
# legally binding.

# $XORP: xorp/tests/test_start.py,v 1.4 2007/07/06 00:02:44 atanu Exp $

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
