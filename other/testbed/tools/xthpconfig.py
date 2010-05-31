#! /usr/bin/env python

# $XORP: other/testbed/tools/xthpconfig.py,v 1.9 2002/12/06 03:17:50 pavlin Exp $

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces
import getopt, sys, os

from xtutils import nw, port2portid, portid2port, portmap, mode

"""
Given a <testbed_config> create a configuration file that can be loaded
into the HP Switch. Note: the first vlan in <testbed_config> is configured as
the default vlan.
"""

# ipv4 address and mask of the switch available through the CONTROL vlan.
# If hp_init is 3 then DHCP will be used. If the address needs to be
# explicitly set change hp_init to 2 and set ipv4 and mask
hp_init=3
hp_control_ipv4="0.0.0.0"
hp_control_mask="0.0.0.0"
hp_ports=72     # Number of ports on switch

class HPConfig(saxutils.DefaultHandler):
    inconfig = 0
    vlans = ()
    
    def __init__(self, fname, script_name, debug = 0):
	self.fname = fname
        self.script_name = script_name
	self.debug = debug

    def characters(self, ch):
        self.ch += ch

    def start(self):
	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(self)
	parser.parse(self.fname)

        # Before we write out the config file we need to create another
        # VLAN which contains all the ports which have not been included
        # in any vlans. The switch seems to require this.
        ports = {}
        for i in range(1, hp_ports + 1):
            ports[i] = 0

        for i in self.vlans:
            for j in i[1]:
                ports[port2portid(j)] =  1

        port = ()
        for i in range(1, hp_ports + 1):
            if 0 == ports[i]:
                port = port + (portid2port(i),)

        self.vlans = self.vlans + \
                         (((("OTHER", "0.0.0.0", "0.0.0.0"),) + \
                           (port,)),)

        self.spit()
        
    def startElement(self, name, attrs):
	self.ch = ""

        if "testbed_config" == name:
            self.inconfig = 1

        if self.inconfig != 1:
            return

        if "vlan" == name:
            self.port = ()
            self.vlan_name = nw(attrs.getValue('name'))
            try:
                self.ipv4 = nw(attrs.getValue('ipv4'))
                self.mask = nw(attrs.getValue('mask'))
            except:
                self.ipv4 = "0.0.0.0"
                self.mask = "0.0.0.0"

        if "host" == name:
            self.host_name = nw(attrs.getValue('name'))

        if "ipv4" == name:
            port = (nw(attrs.getValue('port')),)
            self.port = self.port + (port)

    def endElement(self, name):
        if "testbed_config" == name:
            self.inconfig = 0

# Build tuple of vlan's
# ((u'CONTROL', u'193.168.255.249', u'255.255.255.248'), (u'a6', u'a7'))
# ((u'MAIN', '', ''), (u'a1', u'a2', u'a3', u'a4', u'a5', u'a8')) 

        if "vlan" == name:
            self.vlans = self.vlans + \
                         ((((self.vlan_name, self.ipv4, self.mask),) + \
                           (self.port,)),)
        self.ch = ""

    #
    # Spit out a HP config file
    #
    def spit(self):
        # Print out the header

        # It is a useful debugging aid to store the name of
        # the original config script in the switch config.

        # Three fields are obvious candidates:
        # NAME          15
        # CONTACT       43
        # LOCATION      33
        # The number is the length of this field. So the contact field is
        # used as the longest field. It should be noted that when the
        # NAME field was exceeded the switch continually rebooted. The
        # field lengths were determined by filling the fields using the
        # web interface.
        
        if 43 < len(self.script_name):
            script = self.script_name[len(self.script_name) - 43:]
        else:
            script = self.script_name
        
        print \
"""; J4121A Configuration Editor; Created on release #C.08.22

SYSTEM (
NAME=~xorpsw1~
CONTACT=~%s~
LOCATION=~xorp testbed~
SUPPORT_URL=~http://xorpc.icir.org/testbed/config.xml.txt~
DST_RULE=1
)

CONSOLE (
)""" % script

        for i in range(1, hp_ports + 1):
            print \
"""
CCT (
NAME=%s
PORT_ID=%d
TYPE=62
)""" % (portid2port(i), i)


        print \
"""
STP (
)

IPX (

IPX_NW (
NODE_ADDR=0001e722d880
INIT=1
GW_ENCAP=5
)"""

        # XXXXX
        # Generate some stupid IPX NODE_ADDR we don't deal with overflow
        #

        vlan_id = 2
        addr = 0x81
        for i in self.vlans[1:]: # Start from the second entry
            print """
IPX_NW (
VLAN_ID=%d
NODE_ADDR=0001e722d8%x
INIT=1
GW_ENCAP=5
)""" % (vlan_id, addr)
            vlan_id += 1
            addr += 1

        global hp_control_ipv4, hp_control_mask

        print ")"

        print \
"""
IP (
TIMEP=3

IP_NW (
INIT=%d
ADDR=%s
SNET_MSK=%s
)""" % (hp_init, hp_control_ipv4, hp_control_mask)

        # Print out vlan info

        vlan_id = 2
        for i in self.vlans[1:]: # Start from the second entry
            print """
IP_NW (
VLAN_ID=%d
INIT=2
ADDR=%s
SNET_MSK=%s
GATEWAY=0.0.0.0
)""" % (vlan_id, i[0][1], i[0][2])
            vlan_id += 1

        print ")"

        print \
"""
SNMPS (
ROW_STATUS=1
NAME=public
VIEW=5
MODE=5
)

TRAPS (
)
"""

        # More vlan maps
        vlan_id = 2
        for i in self.vlans:
            if i[0][0] == self.vlans[0][0][0]: # First entry default VLAN 1
                print \
"""VLAN (
NAME=%s
PORT_MAP=%s
MODE=%s
)
""" % (i[0][0], portmap(hp_ports), mode(hp_ports,i[1]))
            else:
                print \
"""VLAN (
VLAN_ID=%d
NAME=%s
VLAN_QID=%d
PORT_MAP=%s
MODE=%s
)
""" % (vlan_id, i[0][0], vlan_id, portmap(hp_ports), mode(hp_ports,i[1]))
                vlan_id += 1
                

        print \
"""PROBE (
INIT=2
PROBE_TYPE=1
MONITORED_PORT_MASK=000000000000000000000000
)

IGMP (
)"""

        vlan_id = 2
        for i in self.vlans[1:]:
            print \
"""
IGMP (
VLAN_ID=%d
)""" % vlan_id
            vlan_id += 1

        print \
"""
ABC (

ABC_NW (
ABC_CONF=4
)"""

        vlan_id = 2
        for i in self.vlans[1:]:
            print \
"""
ABC_NW (
VLAN_ID=%d
ABC_CONF=4
)""" % vlan_id
            vlan_id += 1

        print \
""")

FAULT_FINDER (
FAULT_ID=1
)

FAULT_FINDER (
FAULT_ID=2
)

FAULT_FINDER (
FAULT_ID=3
)

FAULT_FINDER (
FAULT_ID=4
)

FAULT_FINDER (
FAULT_ID=5
)

FAULT_FINDER (
FAULT_ID=6
)

FAULT_FINDER (
FAULT_ID=11
)

GVRP (
GVRP_VLAN=30
)

COS_PROTO (
TYPE=1
)

COS_PROTO (
TYPE=2
)

COS_PROTO (
TYPE=3
)

COS_PROTO (
TYPE=4
)

COS_PROTO (
TYPE=5
)

COS_PROTO (
TYPE=6
)

COS_PROTO (
TYPE=7
)

COS_TOS (
TOS_MODE=1
)

COS_TOSDS (
REC_MASK=ffffffffffffffff00000000
)

HPDP (
)

STACK (
ADMIN_STATUS=0
)
"""

us="usage: %s [-h|--help] [-d|--debug] -c|--config config.xml"

def main():

    def usage():
        global us
	print us % sys.argv[0]
    try:
	opts, args = getopt.getopt(sys.argv[1:], "hdc:", \
				   ["help", "debug", "config="])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    config = ""
    debug = 0
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
	if o in ("-c", "--config"):
	    config = a
	if o in ("-d", "--debug"):
	    debug = 1

    if "" == config:
	usage()
	sys.exit(1)

    HPConfig(config, os.path.abspath(config), debug).start()

    sys.exit()

if __name__ == '__main__':
    main()

