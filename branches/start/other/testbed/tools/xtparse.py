#! /usr/bin/env python

# $XORP: other/testbed/tools/xtparse.py,v 1.7 2002/03/08 22:57:36 atanu Exp $

import getopt, sys, string
from shlex import shlex
from xtutils import MyError

"""
Parse our fancy config file and generate an XML entries that can be
included in <tested_config> files
"""

vlanid = 0              # vlan number - used to derive a vlan name
network = 1             # Base network number
mask="255.255.255.0"    # Netmask for generated entries

list_separator = ","

def process(debug, physical, fancy):
    """
    Generate a <testbed_config> entry from the fancy config
    """
    
    vlans, kernels, noconfigs, router = parse(debug, physical, fancy)
    if 1 == debug:
        for i in vlans:
            print i
        for i in kernels:
            print i
        for i in noconfigs:
            print i
        print router
        
    # Verify that each vlan has a name. If a vlan doesn't have a name
    # generate one for it.
    for i in range(len(vlans)):
        if 1 == debug:
            print i, vlans[i]
        if "" == vlans[i][0][0]:
            vlans[i][0][0] = generate_vlan_name(debug, \
                                                [vn[0][0] for vn in vlans])

    #
    # Verify that the vlan names are unique
    #
    from xtutils import unique
    unique("vlan", [vn[0][0] for vn in vlans])

    #
    # For netmasks of the form x.x.x.x/n convert the prefix to a mask
    #
    for i in range(len(vlans)):
        if 1 == debug:
            print i, vlans[i]
        if "" != vlans[i][0][1]:
            # If there is a "/" generate a mask
            if -1 != string.find(vlans[i][0][1], "/"):
                vlans[i][0][1], vlans[i][0][2] = \
                                generate_mask(debug, vlans[i][0][1])

    # Each vlan requires a network number and netmask
    for i in range(len(vlans)):
        if 1 == debug:
            print i, vlans[i]
        if "" == vlans[i][0][1]:
            vlans[i][0][1], vlans[i][0][2] = \
                            generate_network(debug,[vn[0][1] for vn in vlans])

    # By this stage every entry should have a network number. It
    # may not however have a netmask so derive it from the network number
    for i in range(len(vlans)):
        if 1 == debug:
            print i, vlans[i]
        if "" == vlans[i][0][2]:
            vlans[i][0][2] = net2mask(debug, vlans[i][0][1])

    #
    # Verify that the network numbers are unique
    #
    unique("network", [vn[0][1] for vn in vlans])

    #
    # It is not necessary to specify an interface so try and allocate
    # unused interfaces.
    # 1) Generate the full list of interfaces that are available
    # 2) Knock out the used interfaces from the list
    # 3) Make the unused ones available

    vifs = []
    from xtxml import Xtphysical
    for i in Xtphysical(physical, debug).start():
        if i["use"] != "no":
            vifs = vifs + [[i["host"], i["vif"]]]

    for i in vlans:
        for vlan in i[1:]:
            if 1 == debug:
                print vlan
            if "" != vlan[1]:
                for i in range(len(vifs)):
                    if vlan[0] == vifs[i][0] and vlan[1] == vifs[i][1]:
                        vifs[i][0] = vifs[i][1] = ""
                        break
                else:
                    error = \
"%s.%s Is either not available or multiple use has been attempted" % \
(vlan[0], vlan[1])
                    raise MyError, error

    for i in range(len(vlans)):
        for j in range(1, len(vlans[i][1:]) + 1):
            if 1 == debug:
                print i, j, vlans[i][j]
            if "" == vlans[i][j][1]:
                vifs, vlans[i][j][1] = generatevif(debug, vifs, vlans[i][j][0])

    #
    # We may need to generate addresses
    #
    for i in range(len(vlans)):
	vlans[i] = [vlans[i][0]] + \
		   [generate_address(debug, vlans[i][0][1], vlans[i][1:])]


    #
    # Generate the XML that can be included in <testbed_config>
    #
    for i in vlans:
        if 1 == debug:
            print i
        v = i[0]
        print '<vlan name="%s" ipv4="%s" mask="%s">' % (v[0], v[1], v[2])
        for h in i[1:][0]:
            for p in Xtphysical(physical, debug).start():
                if h[0] == p["host"] and h[1] == p["vif"]:
                    mac = p["mac"]
                    port = p["port"]

            print '\t<host name="%s">' % h[0]
            print '\t\t<ipv4 name="%s" mac="%s" port="%s">' % \
                  (h[1], mac, port)
            print '\t\t\t%s' % h[2]
            print '\t\t</ipv4>'
            print '\t</host>'
        print '</vlan>'

    #
    # Generate the kernel location XML
    #
    for i in kernels:
        if 1 == debug:
            print i
        print '<kernel>'
        print '\t<src>', i[0], '</src>'
        print '\t<dst>', i[1], '</dst>'
        print '</kernel>'

    #
    # Generate the list of hosts that should not be configured.
    #
    for i in noconfigs:
        if 1 == debug:
            print i
        print '<noconfig>'
        print i
        print '</noconfig>'

    #
    # The router host if present
    #
    if "" != router:
        print '<router>'
        print router
        print '</router>'
        

#
# PARSER START
#

def parse(debug, phy, fan):
    """
    Parse the fancy config and generate an entry for <testbed_config>
    """

    sh = shlex(open(fan), fan)
    sh.wordchars += "./-"

    vlans = []
    kernels = []
    noconfigs = []
    router = ""
    
    while 1:
        tok = sh.get_token()
        if "" == tok:
            break
        if 1 == debug:
            print "parse: " + tok

        #
        # This token should be a keyword
        #
        if "vlan" == tok:
            vl = vlan(debug, sh)
            if 1 == debug:
                print "parse (vlan): ", vl
            vlans += [vl]
        elif "kernel" == tok:
            kl = kernel(debug, sh)
            if 1 == debug:
                print "parse (kernel): ", kl
            kernels += kl
        elif "noconfig" == tok:
            nc = noconfig(debug, sh)
            if 1 == debug:
                print "parse (noconfig): ", nc
            noconfigs += nc
        elif "router" == tok:
            if "" != router:
                raise MyError, sh.error_leader() + "only one router allowed"
            expect(sh, "=")
            router = noteof(sh)
            if 1 == debug:
                print "parse (router): ", router
        else:
            raise MyError, sh.error_leader() + "unknown keyword: " + tok

    return vlans, kernels, noconfigs, router

def vlan(debug, sh):
    """
    Deal with a single vlan
    """

    tok = sh.get_token()
    if "" == tok:
        return
    if 1 == debug:
        print "vlan: " + tok
    #
    # We have just entered vlan configuration.
    # We need a vlan name , network number and mask.
    # This information is either on the vlan line or we need to generate
    # them.
    #

    vlan_name = ""
    network_number = ""
        
    # We are looking for two optional fields. The vlan name and the network.
    # Note that "net" is a keyword.
    #
    # vlan[vlan1]
    # vlan[net 10.0.0.1]
    # vlan[vlan1 net 10.0.0.1/24]
    # vlan[vlan1 net 10.0.0.1/255.255.255.0]
    #
    if "[" == tok:
        list = ["", ""]
        offset = 0
        while 1:
            tok = sh.get_token()
            if 1 == debug:
                print "vlan: " + tok
            if "" == tok or "]" == tok:
                break
            if "net" == tok:
                offset = 1
            else:
                list[offset] = tok

        vlan_name = list[0]
        network_number = list[1]

    if "=" != tok:
        tok = sh.get_token()
    if 1 == debug:
        print "vlan: " + tok

    if 1 == debug:
        print "vlan name: " + vlan_name + " " + network_number

    #
    # This token must be an "="
    #
    if "=" != tok:
        raise MyError,  sh.error_leader() + "expected " + '"="' + " got " + tok

    hosts = []
    hosts = host(debug, sh)
    if 1 == debug:
        print "vlan: ", hosts
                                        # Place holder for netmask
    return [[vlan_name, network_number, ""]] + hosts

def host(debug, sh):
    """
    Return the list of host and address pairs on this vlan
    """

    tok = sh.get_token()
    if "" == tok:
        return []
    if 1 == debug:
        print "\thost: " + tok

    count = 0
    interface = ""

    for i in string.split(tok, "."):
        if 0 == count:
            name = i
        if 1 == count:
            interface = i
        count += 1
        
    tok = sh.get_token()
    if 1 == debug:
        print "\thost: " + tok
    if "" == tok:
        return [[name, interface, ""]]

    address = ""

    #
    # Pick out the address
    # 
    if "[" == tok:
        tok = sh.get_token()
        if 1 == debug:
            print "\thost: " + tok
        address = tok
        # Soak up the closing "]"
        tok = sh.get_token()
        if 1 == debug:
            print "\thost: " + tok
        tok = sh.get_token()
        if 1 == debug:
            print "\thost: " + tok
        if "" == tok:
            return [[name, interface, address]]

    global list_separator
    if list_separator == tok:
        # Recursive call
        return [[name, interface, address]] + host(debug, sh)
    else:
        sh.push_token(tok)

    return [[name, interface, address]]

def expect(sh, ch):
    """
    Read a character from the stream and generate an error if the it is
    not the expected character.
    """

    tok = sh.get_token()
    if ch != tok:
        raise MyError,  sh.error_leader() + "expected " + ch + " got " + tok

#
# not eof
#
def noteof(sh):
    """
    Read and return a token. If we are at the end of the stream raise
    an exception
    """
    tok = sh.get_token()
    if "" == tok:
        raise MyError,  sh.error_leader() + "unexpected end of input"
    
    return tok

#
# Process entries of the form:
#
# kernel[/home/xorpc/u2/freebsd.kernels/kernel] = xorp0
# kernel[/home/xorpc/u2/freebsd.kernels/kernel] = xorp0, xorp1
#
# The "kernel" keyword has already been read.
# The source pathname in brackets is mandatory and there must be at least
# one destination.

def kernel(debug, sh):
    """
    Process a single kernel entry
    """

    expect(sh, "[")

    source = noteof(sh)
    if 1 == debug:
        print "kernel: " + source

    expect(sh, "]")
    expect(sh, "=")

    tok = noteof(sh)
    if 1 == debug:
        print "kernel: " + tok

    kernels = [[source, tok]]

    # At this point there are optionally more destinations separated by
    # ',''s.

    global list_separator
    while 1:
        tok = sh.get_token()
        if 1 == debug:
            print "kernel: " + tok
        if list_separator != tok:
            sh.push_token(tok)
            break
        else:
            tok = noteof(sh)
            if 1 == debug:
                print "kernel: " + tok
            kernels += [[source, tok]]

    return kernels


#
# Process entries of the form:
#
# noconfig = xorp0
# noconfig = xorp0, xorp1
#
# The "noconfig" keyword has already been read.
# The source pathname in brackets is mandatory and there must be at least
# one destination.

def noconfig(debug, sh):
    """
    Process a noconfig entry
    """

    expect(sh, "=")

    tok = noteof(sh)
    if 1 == debug:
        print "noconfig: " + tok

    router = [tok]
    
    # We can optionally have a ',' separated list of hosts that should not
    # be configured.
    global list_separator
    while 1:
        tok = sh.get_token()
        if 1 == debug:
            print "noconfig: " + tok
        if list_separator != tok:
            sh.push_token(tok)
            break
        else:
            tok = noteof(sh)
            if 1 == debug:
                print "noconfig: " + tok
            router += [tok]

    return router

#
# PARSER END
#

def generate_vlan_name(debug, vlans):
    """
    Generate a unique vlan name. That doesn't clash with any already
    allocated names.
    """

    if 1 == debug:
        print "generate_vlan_name: ", vlans

    global vlanid
    for vlanid in range(vlanid, 100):
        vlan_name = "vlan" + repr(vlanid)
        for n in vlans:
            if vlan_name == n:
                break
        else:
            return vlan_name

    raise MyError, "Could not generate a unique vlan name"

    return "generate_vlan_name failed"

def prefix2mask(debug, prefix):
    """
    Convert a prefix to a mask
    """

    val = 0
    for i in range(int(prefix)):
        val >>= 1
        val |= 0x80000000

    import socket
    import struct

    ret = socket.inet_ntoa(struct.pack("i", socket.htonl(val)))

    if 1 == debug:
        print "prefix2mask(%s, %s)" % (prefix, ret)

    return ret

def net2mask(debug, network):
    """
    From a network derive the mask
    """

    def aton(a):
        import socket
        import struct

        return socket.ntohl(struct.unpack("i", socket.inet_aton(a))[0])

    net = aton(network)

    count = 0
    while 1:
        if 0 == (net & 1):
            count += 1
            net >>= 1
        else:
            break

    ret = prefix2mask(debug, 32 - count)

    if 1 == debug:
        print "net2mask(%s, %s)" % (network, ret)

    return ret
    
    
def generate_mask(debug, net):
    """
    Given x.x.x.x/n or x.x.x.x/y.y.y.y return a network and a mask
    """

    f = string.split(net, "/")
    if -1 != string.find(f[1], "."):
        mask = f[1]
    else:
        mask = prefix2mask(debug, f[1])

    return f[0], mask

def generate_network(debug, nets):
    """
    Generate a unique network number and mask
    """

    if 1 == debug:
        print "generate_network: ", nets

    global network, mask
    for network in range(network, 100):
        net = "10." + repr(network) + ".0.0"
        for n in nets:
            if net == n:
                break
        else:
            return net, mask

    raise MyError, "Unable to generate a new network number"

    return "generate_network failed", "mask generation failed"

def generatevif(debug, vifs, host):
    """
    Find an unused interface on this host
    """

    for i in range(len(vifs)):
        if vifs[i][0] == host:
            int = vifs[i][1]
            vifs[i][0] = vifs[i][1] = ""
            return vifs, int

    raise MyError, "No more interfaces available on host %s" % host

    return vifs, "No interface available"

def generate_address(debug, network, vlan):
    """
    If required generate an address
    """

    # Extract the already allocated addresses
    addrs = []
    for i in vlan:
	if "" != i[2]:
	    addrs += [i[2]]

    if 1 == debug:
	print "generate_address: ", addrs

    #
    # Make sure the addresses are unique
    #
    from xtutils import unique
    unique("address", addrs)
    
    def incr(a):
        """
        Increment the x.x.x.x style address
        """

        import socket
        import struct

        n = struct.unpack("i", socket.inet_aton(a))[0]
        n = socket.ntohl(n)
        n += 1
        n = socket.htonl(n)
        a = socket.inet_ntoa(struct.pack("i", n))

        return a

    address = network
    for i in range(len(vlan)):
	if "" == vlan[i][2]:
	    while 1:
		address = incr(address)
		for a in addrs:
		    if a == address:
			break
		else:
		    break
	    vlan[i][2] = address
            addrs += [address]

    return vlan


us=\
"usage: %s [-h|--help] [-d|--debug]" \
"-p|--physical xorp.xml -f|--fancy fancy.xt"

def main():

    def usage():
        global us
	print us % sys.argv[0]
    try:
	opts, args = getopt.getopt(sys.argv[1:], "hp:f:d", \
				   ["help", \
                                    "physical=", "fancy=", \
                                    "debug"])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    physical = ""
    fancy = ""
    debug = 0
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
	if o in ("-p", "--physical"):
	    physical = a
	if o in ("-f", "--fancy"):
            fancy = a
	if o in ("-d", "--debug"):
	    debug = 1

    if "" == fancy or "" == physical:
        usage()
        sys.exit(1)

    try:
        process(debug, physical, fancy)
        
    except MyError, error:
        sys.stderr.write("%s\n" % str(error))
        sys.exit(1)


if __name__ == '__main__':
    main()
