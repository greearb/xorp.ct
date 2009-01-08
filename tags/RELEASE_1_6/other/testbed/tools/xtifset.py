#! /usr/bin/env python

# $XORP: other/testbed/tools/xtifset.py,v 1.19 2002/07/03 00:18:58 atanu Exp $

import sys
import os
import string
import getopt

"""
1) Assign IP addresses to the interfaces using a <testbed_config> file.
2) Create corresponding /etc/hosts file
"""

#
# Translation table for xorp0 and xorp3
#
trans1 = { "fxp0" : "main" , "fxp1" : "f0" , "fxp2" : "f1", "fxp3" : "f2", \
           "dc0" : "t0", "dc1" : "t1", "dc2" : "t2", "dc3" : "t3" , \
           "xl0": "x0" }

#
# Translation table for xorp1 and xorp2
#
trans2 = { "fxp0" : "main" , "fxp1" : "f0" , "fxp2" : "f1", "fxp3" : "f2", \
           "dc0" : "t4","dc1" : "t0","dc2" : "t1", "dc3" : "t2" , "dc4" : "t3"}

#
# Translation table for xorp5
#
trans3 = { "sis0" : "main", "sis1" : "s0", "sis2" : "s1"}

#
# Translation table for xorp4
#
trans4 = { "fxp0" : "main" , \
           "dc0" : "t4" , "dc1" : "t5", "dc2" : "t6", "dc3" : "t7", \
           "dc4" : "t0", "dc5" : "t1", "dc6" : "t2" , "dc7" : "t3"}

#
# Translation table for xorp6 and xorp7
#
trans5 = {"dc0" : "t0" , "dc1" : "t1", "dc2" : "t2", "dc3" : "t3", \
          "fxp0" : "main"}

#
# Portmapper for xorp0, xorp1, xorp2 and xorp3
#
portmap1 = { "main" : "a", "f0" : "f", "f1" : "e", "f2" : "d", \
            "t0" : "h", "t1" : "g", "t2" : "b", "t3" : "c", \
            "t4" : "i" , "x0" : "i"}
#
# Portmapper for xorp5
#
portmap2 = { "main" : "b", "s0" : "c", "s1" : "d"}             

#
# Portmapper for xorp4 
#
portmap3 =  { "main" : "a", \
              "t4" : "i", "t5" : "f", "t6" : "e", "t7" : "d", \
              "t0" : "h", "t1" : "g", "t2" : "b", "t3" : "c"}

#
# Portmapper for xorp6 and xorp7
#
portmap4 =  { "main" : "i", \
              "t0" : "h", "t1" : "g", "t2" : "b", "t3" : "c"}

def vif2interface(host, vif):
    """
    Given a hostname and our virtual interface name return the name
    of the physical interface. Well for freebsd anyway.
    """

    if "xorp0" == host:
        table = trans1
    elif "xorp1" == host:
        table = trans2
    elif "xorp2" == host:
        table = trans2
    elif "xorp3" == host:
        table = trans1
    elif "xorp4" == host:
        table = trans4
    elif "xorp5" == host:
        table = trans3
    elif "xorp6" == host:
        table = trans5
    elif "xorp7" == host:
        table = trans5
    else:
        return "unknown host"

    for i in table.items():
        if i[1] == vif:
            return i[0]
        
    return "unknown"

def portmap(interfaces, int):
    """
    Given an interface return the switch row to which it should be connected
    """

    if "dc0" == interfaces[0][0] and "fxp0" == interfaces[4][0]:
        table = portmap4
    elif "fxp0" == interfaces[0][0] and "dc0" == interfaces[1][0]:
        table = portmap3
    elif "sis0" != interfaces[0][0]:
        table = portmap1
    else:
        table = portmap2
    try:
        return table[int]
    except:
        return "unknown"

def trans(interfaces, int):
    """
    Translate from FreeBSD interface names to our idealized names.
    """

    if "dc0" == interfaces[0][0] and "fxp0" == interfaces[4][0]:
        table = trans5
    elif "fxp0" == interfaces[0][0] and "dc0" == interfaces[1][0]:
        table = trans4
    elif "fxp0" == interfaces[0][0] or "xl0" == interfaces[0][0]:
        table = trans1
    elif "sis0" == interfaces[0][0]:
        table = trans3
    else:
        table = trans2
    try:
        return table[int]
    except:
        return "unknown" # Probably running on Linux
    

def interface2mac_freebsd(fd):
    """
    Parse the output of a freebsd ifconfig command and produce a dictionary
    of interfaces indexed by mac addresses.
    """
    # Dictionary of mac addresses
    macs = {}
    interfaces = ()
    lines = fd.readlines()
    for i in lines:
        if not i[0][0].isspace():
            interface = i.split(':')[0]
        else:
            if -1 != i.find('ether'):
                mac = i.split()[1].lower()
#               print interface + " " + mac
                macs[mac] = interface
                interfaces = interfaces + ((interface, mac,),)
    return interfaces, macs

def interface2mac_linux(fd):
    """
    Parse the output of a linux ifconfig command and produce a dictionary
    of interfaces indexed by mac addresses.
    """
    # Dictionary of mac addresses
    macs = {}
    interfaces = ()
    lines = fd.readlines()
    for i in lines:
        if not i[0][0].isspace():
            interface = i.split()[0]
            if -1 != i.find("HWaddr"):
                mac = i.split()[4].lower()
#               print interface + " " + mac
                macs[mac] = interface
                interfaces = interfaces + ((interface, mac,),)
    return interfaces, macs

def interface2mac(fd):
    """
    Given the output of an ifconfig command call either a FreeBSD or Linux
    parser to process it.
    """

    if "FreeBSD" == os.uname()[0]:
        return interface2mac_freebsd(fd)
    
    if "Linux" == os.uname()[0]:
        return interface2mac_linux(fd)

    print "interface2mac(): No OS specific call for %s" % os.uname()[0]
    
#
# This function is called from xtconf
#
def verify(host, interface, mac, port):
    error = "verify(%s,%s,%s,%s) " % (host, interface, mac, port)
    # XXX
    # We should be running the ifconfig's not looking in files.
#   fname = "/home/lemming/u0/atanu/xorp/other/testbed/config/ifconfig/" + host
#   try:
#       interfaces, macs = interface2mac(open(fname, 'r'))
#   except:
#       error += "No information for host: " + host
#       return 0, error

    command = "ssh %s ifconfig" % host
    interfaces, macs = interface2mac(os.popen(command, 'r'))
    
    # Check for the existence of this mac address
    try:
        int = macs[mac.lower()]
    except:
        error += "Mac address not found: " + mac
        return 0, error
    vint = trans(interfaces, int)
    if vint != interface:
        error += "name should be " + vint + " not " + interface
        return 0, error
    # Check this is connected to the correct switch port
    if -1 != host.find("xorp"):
        col = string.atoi(host[4]) + 1
        row = portmap(interfaces, interface)
        p = row + repr(col)
        if p != port:
            error += "Port should be %s is %s" % (p, port)
            return 0, error
        
    return 1, error
    
def test():
    """
    Given the output of ifconfig  extract the
    interface names and mac addresses. Also add our virtual interface name.
    """
    interfaces, macs = interface2mac(sys.stdin)
#   print interfaces, macs
    for i in interfaces:
        print i[0], i[1], trans(interfaces, i[0])


def noconfig(config, host, debug):
    """
    See if the host argument matches that of one of the config hosts on the
    <testebd_config> file. This is used by the xorp.sh shell script to decide
    if the host should have its interfaces enabled and if it should have its
    routes configured.
    """

    # Get the list from the <testbed_config> file
    from xtxml import Xtconfig
    list, kernels, noconfig, router = Xtconfig(config, debug).start()

    for i in noconfig:
        if i == host:
            return 1

    return 0

def ifconfig(config, debug):
    """
    Take a <testbed_config> file and use it to ifconfig the interfaces.
    """

    # Get the list from the <testbed_config> file
    from xtxml import Xtconfig
    list, kernels, noconfig, router = Xtconfig(config, debug).start() 
    if 1 == debug:
        for i in list:
            print i

    # Find the interfaces on this machine
    stdin, stdout_stderr = os.popen4("ifconfig -a", 'r')
    interfaces, macs = interface2mac(stdout_stderr)
    stdin.close()
    stdout_stderr.close()
    
    if 1 == debug:
        for i in interfaces:
            print i["mac"], i["addr"], trans(interfaces, i["mac"])

    for i in list:
        if "" != i["mac"] and "" != i["addr"] and "" != i["mask"]:
            if macs.has_key(i["mac"]):
                if 1 == debug:
                    print macs[i["mac"]], i["mac"], i["addr"], i["mask"]
                command = "ifconfig %s %s netmask %s up" % \
                          (macs[i["mac"]], i["addr"], i["mask"])
                print command
                stdout = os.popen(command, 'r')
                while 1:
                    lines = stdout.readline()
                    if not lines:
                        break
                    print lines
                stdout.close()

def route(config_file, debug):
    """
    If there is a designated router in <testbed_config> then configure
    all networks to go through the router.
    """

    from xtxml import Xtconfig
    vlans, kernels, noconfig, router = Xtconfig(config_file, debug).start()

    if "" == router:
        print "No router specified"
        return

    # If we are the router enable forwarding and get out of here:-)

    host = os.uname()[1]

    if 1 == debug:
        print "router: " + router
        print "This host: " + host

    if 0 == host.find(router):
        enable_forwarding(debug)
        return

    #
    # Collect the list of networks that exist.
    # Remove any networks that we are directly connected to.
    # Then add routes for all these networks towards the router.

    nets = []
    myvlans = []
    othervlans = []

    for i in vlans:
        if "" != i["network"]:
            nets = nets + [i]


    myvlans, othervlans = split(host, nets)
        
    if 1 == debug:
        print "My vlans", myvlans
        print "Other vlans", othervlans

    def find(host, vlans, myvlans):
        """
        Find if this host is connected to myvlans
        """
        
        for v in vlans:
            for m in myvlans:
                if 0 == host.find(v["host"]) and m == v["vlan"]:
                    return v
                    
        return []

    #
    # The simple case is that the router shares a vlan with this host.
    # Use a simple routine to find this adjacency. Use a more complex
    # routine to find a full path. If however there is more than one
    # intermediate host we will create routing loops so don't try any
    # automatic configuration.
    # 
    f = find(router, nets, myvlans)
    if [] == f:
        print "No direct route from " + host + " to " + router
        f = search(host, router, nets, debug)
        if 1 == debug:
            print f
        if [] == f:
            print "No route from " + host + " to " + router
            return
        if len(f) > 2:
            print "Too many hops (" + repr(len(f)) + ") from " + host + \
                  " to " + router
            h = host
            for i in f:
                x = find(h, nets, [i["vlan"]])
                print "\t" + x["host"] + "." + x["vif"]
                h = i["host"]
        
            return
        f = f[0]

    if 1 == debug:
        print f
        print f["host"] + "." + f["vif"]

    print "Route all non local traffic through " + f["host"] + "." + f["vif"]
    if 1 == debug:
        print f
    for o in othervlans:
        for i in nets:
            if o == i["vlan"]:
                if 1 == debug:
                    print o
                route_add(i["network"], i["mask"], f["addr"], debug)
                break

def prune(host, nets):
    """
    Remove this host from the list of vlans
    """

    n = []
    for i in nets:
        if i["host"] != host:
            n += [i]

    return n
    
def search(host, router, nets, debug):
    """
    Search for a route from host to router
    """

    if 1 == debug:
        print "host: " + host
        print "router: " + router

    res = []
    for n in nets:
        mine, others = split(host, nets)
        for m in mine:
            if m == n["vlan"]:
                if 0 == router.find(n["host"]):
                    if 1 == debug:
                        print "Result: " + n["host"] + "." + n["vif"]
                    return [n]
                else:
                    if 1 == debug:
                        print "Deeper: " + n["host"] + "." + n["vif"]
                    tmp = search(n["host"], router, prune(host, nets), debug)
                    if [] != tmp:
                        if len(tmp) + 1 < len(res) or [] == res:
                            res = [n] + tmp

    return res

def split(host, nets):
    """
    Return the vlans that I am connected to and the others
    """

    allvlans = []
    myvlans = []
    othervlans = []

    def unique_add(vlans, v):
        for o in vlans:
            if o == v:
                return vlans

        return vlans + [v]

    for i in nets:
        allvlans = unique_add(allvlans, i["vlan"])
        if 0 == host.find(i["host"]):
            myvlans += [i["vlan"]]

    for a in allvlans:
        for m in myvlans:
            if a == m:
                break
        else:
            othervlans += [a]

    return myvlans, othervlans

def enable_forwarding(debug):
    """
    Enable forwarding on this host
    """

    print "Enable forwarding"
    if "FreeBSD" == os.uname()[0]:
        command("sysctl -w net.inet.ip.forwarding=1")
    elif "Linux" == os.uname()[0]:
        command("echo 1 > /proc/sys/net/ipv4/ip_forward")
    else:
        print "Unrecognised system " + os.uname()[0]

def command(com, output = "true"):
    """
    Run command
    """

    # XXX
    # There is a piece of code in popen2.py:
    #
    #     def _run_child(self, cmd):
    #         if type(cmd) == type(''):
    #              cmd = ['/bin/sh', '-c', cmd]
    #
    # This is presumably trying to differentiate <type 'string'> from
    # <type 'list'>. Unfortunately if "cmd" is of type <type 'unicode'>
    # the code believes that a <type 'list'> has been passed in and the
    # code fails.
    #
    # So convert <type 'unicode'> to <type 'string'>
    if repr(type(com)) == "<type 'unicode'>":
	com = com.encode("ascii")    

    stdin, stdout = os.popen4(com)
    stdin.close()
    while 1:
	lines = stdout.readline()
	if not lines:
	    break
	if "true" == output:
	    print lines,
    stdout.close()
        
def route_add(net, mask, addr, debug):
    """
    Add a routing entry
    """

    if 1 == debug:
        print "route_add(%s, %s, %s)" % (net, mask, addr)
        show = "true"
    else:
        show = "false"        
    if "FreeBSD" == os.uname()[0]:
        command("route delete %s" % net, show)
        command("route add -net %s -netmask %s %s" % (net, mask, addr))
    elif "Linux" == os.uname()[0]:
        command("route del -net %s netmask %s" % (net, mask), show)
        command("route add -net %s netmask %s gw %s" % (net, mask, addr))
    else:
        print "Unrecognised system " + os.uname()[0]

def host(config_file, debug):
    """
    From the <testbed_config> file generate a /etc/hosts file.
    """

    from xtxml import Xtconfig
    vlans, kernels, noconfig, router = Xtconfig(config_file, debug).start()

    print
    print "# " + "Generated by %s" % sys.argv[0]
    from time import strftime, localtime, time
    print "# " + strftime("%a, %d %b %Y %H:%M:%S %Z", localtime(time()))
    print
    
    for i in vlans:
        if "" != i["vif"] and "" != i["addr"] and "" != i["host"]:
            print "%s %s-%s\t# %s" % (i["addr"], i["host"], i["vif"], \
                                   vif2interface(i["host"], i["vif"]))
    

us= \
"""\
usage: %s [-h|--help] [-d|--debug] [-t|--test]
          [-i|--ifconfig] [-r|--route] [-H|--host]
          [-c|--config config1.xml]"""

def main():

    def usage():
        global us
        print us % sys.argv[0]

    try:
	opts, args = getopt.getopt(sys.argv[1:], "htn:irHc:d", \
				   ["help", \
                                    "test", \
                                    "noconfig=", \
                                    "ifconfig", \
                                    "route", \
                                    "host",\
                                    "config=", \
                                    "debug"])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    i = 0
    r = 0
    h = 0
    this_host = ""
    config = ""
    debug = 0
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
        if o in ("-t", "--test"):
            test()
            sys.exit(0)
	if o in ("-n", "--noconfig"):
	    this_host = a
	if o in ("-i", "--ifconfig"):
	    i = 1
	if o in ("-r", "--route"):
	    r = 1
        if o in ("-H", "--host"):
            h = 1
	if o in ("-c", "--config"):
	    config = a
	if o in ("-d", "--debug"):
	    debug = 1

    if "" != this_host:
        if "" != config:
            sys.exit(noconfig(config, this_host, debug))
        else:
            usage()

    if 1 == i:
        if "" != config:
            ifconfig(config, debug)
        else:
            usage()

    if 1 == r:
        if "" != config:
            route(config, debug)
        else:
            usage()

    if 1 == h:
        if "" != config:
            host(config, debug)
        else:
            usage()

    sys.exit()

if __name__ == '__main__':
    main()

#status, msg = verify("xorp0", "main", "00:02:B3:10:DF:4F", "a1")
#if 1 != status:
#    print msg
