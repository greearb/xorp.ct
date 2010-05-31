#!/usr/bin/env python

# $XORP: xorp/bgp/harness/lookup.py,v 1.3 2007/12/10 23:26:32 mjh Exp $

# On standard input take a list of update packets in XORP text format
# On standard output generate a list of lookup xrls that can be passed to
# call_xrl 

import sys
import getopt

AS_PREFIX_STRING = '\tAS Path Attribute ASPath: '

def coord(command):
    """
    """

    command = command.replace(' ', '+')
    print "finder://coord/coord/0.1/command?command:txt=%s" % command

def lookup(peer, trie, add, remove):
    """
    Read through XORP BGP routes on stdin and generate lookup xrls on stdout
    """

    global AS_PREFIX_STRING

    cache = {}

    while 1:
        line = sys.stdin.readline()
        if not line:
            break
        # Collect PATH attribute
        if line.startswith(AS_PREFIX_STRING):
            aspath = line.replace(AS_PREFIX_STRING, '')
            aspath = aspath.replace('AS/', '')
            aspath = aspath.replace(' ', '')
            aspath = aspath.replace('[', '(')
            aspath = aspath.replace(']', ')')
            # AS SET
            aspath = aspath.replace('{',',(')
            aspath = aspath.replace('}',')')
            if "" != add:
                aspath = '(' + add + "," + aspath[1:]
            if "" != remove:
                aspath = aspath.replace(remove + ',', '')
            aspath = aspath[:-1]
            continue
        # Store the latest NLRI
        if line.startswith(" - Nlri"):
            nlri = line.split()[2]
            cache[nlri] = ("%s trie %s lookup %s aspath %s" %
                  (peer, trie, nlri, aspath))
            continue
        # Don't look for it if its been withdrawn
        if line.startswith(" - Withdrawn"):
            nlri = line.split()[2]
            cache[nlri] = None
            continue

    for i in cache:
        if cache[i]:
            coord(cache[i])

us= \
"""\
usage: %s [-h|--help]
\t -p peer -t trie [-a add-as] [-r remove-as]
"""

def main():
    def usage():
        global us
        print >> sys.stderr, us % sys.argv[0]

    try:
	opts, args = getopt.getopt(sys.argv[1:], "hp:t:a:r:", \
				   ["help", \
                                    "peer=", \
                                    "trie=", \
                                    "add=", \
                                    "remove=", \
                                    ])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    peer = ""
    trie = ""
    add = ""
    remove = ""
    for o, a in opts:
	if o in ("-h", "-help"):
	    usage()
	    sys.exit()
        if o in ("-p", "--peer"):
            peer = a
        if o in ("-t", "--trie"):
            trie = a
        if o in ("-a", "--add"):
            add = a 
        if o in ("-r", "--remove"):
            remove = a
        
    if not peer and not trie:
	usage()
	sys.exit(1)

    lookup(peer, trie, add, remove)

if __name__ == '__main__':
    main()
