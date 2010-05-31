# $XORP: other/testbed/tools/xtutils.py,v 1.6 2001/11/12 23:57:08 atanu Exp $

import os
import string
from xtvars import PHYSICAL_CONFIG_URL

"""
Utility functions
"""

class MyError:
    """
    Error class for throwing exceptions.
    """
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return self.value

def nw(text):
    "Remove redundant whitespace from a string"
    return string.join(string.split(text), ' ')

def val(ch):
    "Assign a value to a string"
    result = "abcdefghi".find(ch.lower())
    if -1 == result:
        print "utils.val(): Cannot convert %s" % ch
    return result

def port2portid(port):
    "Convert a port of the form a5 to a number i.e. 5"
    try:
	return (val(port[0]) - val('a')) * 8 + \
			   int(port[1])
    except:
	print "error in port2portid translating: " + port

def portid2port(id):
    "Convert an id of the 5 to a port i.e. A5"
    id -= 1
    return "ABCDEFGHI"[(id / 8)] + str((id % 8) + 1)

def portmap(ports):
    "Generate a portmap string based on the number of ports on switch"
    x = ""
    for i in range(24):
        if (i * 4) < ports:
            x = x + 'f'
        else:
            x = x + '0'
    return x

def mode(ports, list):
    "given a list of ports return the map"
    x = ["-" for i in range(ports)]
    for i in list:
        x[port2portid(i) - 1] = "2"
    return string.join(x, '')
    
def unique(msg, l):
    """
    Verify that all elements of the tuple are unique
    """
    dict = {}
    for i in l:
        if "" != i:
            if dict.has_key(i):
                raise MyError, "Duplicate %s entry %s" % (msg, i)
                #print "Duplicate %s entry %s" % (msg, i)
            else:
                dict[i] = 1

class Physical:
    """
    Return a copy of the physical testbed configuration
    """

    file = ""

    def __init__(self, debug = 0):
	self.debug = debug
	
    def get(self):
        """
        Extract the latest copy of <testbed_physical> from the cvs
        repository. Use the web interface.
        """

        # Fetch the physical config and put it into a file.
        import urllib
        self.file, header = urllib.urlretrieve(PHYSICAL_CONFIG_URL)

        if 1 == self.debug:
            print "Physical configuration in " + self.file

        return self.file

    def remove(self):
        """
        Remove the temporary configuration file
        """

        if "" == self.file:
            if 1 == self.debug:
                print "No file to remove"
        else:
            if 1 == self.debug:
                print "Removing: " + self.file
            os.remove(self.file)
            
