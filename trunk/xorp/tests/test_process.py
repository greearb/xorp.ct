#!/usr/bin/env python

# Copyright (c) 2001-2008 XORP, Inc.
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

# $XORP: xorp/tests/test_process.py,v 1.5 2008/07/23 05:11:49 pavlin Exp $

import thread,threading,time,sys,os,popen2

class Process(threading.Thread):
    """
    Start a process in a separate thread
    """

    def __init__(self, command=""):
        threading.Thread.__init__(self)
        self._status = "INIT"
        self._command = command
        self.lock = thread.allocate_lock()
        
    def run(self):
        self.lock.acquire()
        print "command:", self._command
        self.process = popen2.Popen4("exec " + self._command)
        print "PID:", self.process.pid
        self._status = "RUNNING"
        while 1:
            o = self.process.fromchild.read(1)
            if not o:
                break
            os.write(1, o)
        self._status = "TERMINATED"
        print "exiting:", self._command
        self.lock.release()

    def status(self):
        return self._status

    def command(self):
        return self._command

    def terminate(self):
        """
        Terminate this process
        """

        print "sending kill to", self._command, self.process.pid
        if self._status == "RUNNING":
            os.kill(self.process.pid, 9)
            self.lock.acquire()
            self.lock.release()
        else:
            print self._command, "not running"


# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
