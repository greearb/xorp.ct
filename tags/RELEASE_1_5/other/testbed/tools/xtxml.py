#! /usr/bin/env python

# $XORP: other/testbed/tools/xtxml.py,v 1.10 2002/03/08 22:57:36 atanu Exp $

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces
import string
import getopt, sys

from xtutils import nw, val, port2portid, Physical

"""
Try and place all the access routines for the XML databases <testbed_physical>
and <testbed_config> in here.
"""

# XXX
# Do not use this class anymore in new code use Xtphysical instead
#
class Xorp(saxutils.DefaultHandler):
    """
    Read in the xorp xml <testbed_physical> database and produce a list of
    tuples of ports which are connected to the switch. All the ethernet
    cards can be in the XML database. However not all are connected to the
    switch.
    """
    level = 0
    xorp = []

    def __init__(self, fname, debug = 0):
	self.fname = fname
	self.debug = debug

    def start(self):
	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(self)
	parser.parse(self.fname)

	return self.xorp
    
    def startElement(self, name, attrs):
	self.ch = ""

	if "host" == name:
	    self.host = nw(attrs.getValue('name'))


	# We can have entries for cards which are not currently
	# connected to the switch.
	try:
            if "mac" == name:
                self.port = nw(attrs.getValue('port'))
                # Generate the portid from the port number
                self.portid = port2portid(self.port)
                self.type = nw(attrs.getValue("type"))
                self.cable = nw(attrs.getValue("cable"))
                try:
                    self.name = nw(attrs.getValue("name"))
                except:
                    self.name = "unknown"
                try:
                    self.available = nw(attrs.getValue("use"))
                except:
                    self.available = "yes"
        except:
	    self.port = ""
	    self.portid = ""

        if self.debug:
	    whole = "\t"*self.level + "<%s" % nw(name)
	    self.level += 1

	if self.debug:
	    for i in attrs.getNames():
		whole += " %s=%s" % (i ,nw(attrs.getValue(i)))

	if self.debug:
	    whole += ">"
	    print whole

    def endElement(self, name):
	if self.debug:
	    self.level -= 1
	    if "" != nw(self.ch):
		print "\t"*self.level + nw(self.ch)
	    print "\t"*self.level + "</" + nw(name) + ">"
	
	#
	# Don't put an entry into the list if it is not connected to
	# the switch.
	#
	if "mac" == name and self.port != "":
	    self.xorp = self.xorp + [(self.host, \
				      nw(self.ch), \
				      self.port, \
				      self.portid, \
				      self.type, \
				      self.cable, \
				      self.name, \
				      self.available, \
				      )]

	self.ch = ""

    # Accumulate the characters
    def characters(self, ch):
        self.ch += ch

class Xtphysical(saxutils.DefaultHandler):
    """
    Parse a <testbed_config> file and return a list of dictionaries which
    allows the contents to be referenced by name.
    """
    # A typical entry is of this form:
    #
    # <testbed_physical>
    #     <host name="xorp5">
    #         <mac name="main" port="b6" cable='224' type="sis" use="no">
    #             <!-- Connection to main net -->
    #              00:00:24:c0:0e:dc
    #         </mac>
    #         <mac name="s0" port="c6" cable='507' type="sis" man="">
    #              00:00:24:c0:0e:dd
    #         </mac>
    #         <mac name="s1" port="d6" cable='506' type="sis" man="">
    #              00:00:24:c0:0e:de
    #         </mac>
    #     </host>
    # </testbed_physical>
    # This will be returned as a list of dictionaries of the form:
    #
    # { "host" : "xorp5", "vif" : "main", "port" : "b6", "cable" : "224",
    #   "type" : "sis", "man" : "", "use" : "no", "mac" : "00:00:24:c0:0e:dc" }

    inphysical = 0
    inhost = 0

    list = []

    def __init__(self, config, debug = 0):
	self.config = config
	self.debug = debug

    def characters(self, ch):
        self.ch += ch

    def start(self):
	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(self)
	parser.parse(self.config)

        return self.list

    def startElement(self, name, attrs):
	self.ch = ""

        if "testbed_physical" == name:
            self.inphysical = 1

        if self.inphysical != 1:
            return

        if "host" == name:
            self.inhost = 1
            self.host = getAttrValue(attrs, 'name')

        if self.inhost != 1:
            return

        if "mac" == name:
            self.vif = getAttrValue(attrs, 'name')
            self.port = getAttrValue(attrs, 'port')
            self.cable = getAttrValue(attrs, 'cable')
            self.type = getAttrValue(attrs, 'type')
            self.man = getAttrValue(attrs, 'man')
            self.use = getAttrValue(attrs, 'use')
        
    def endElement(self, name):
        if "testbed_physical" == name:
            self.inphysical = 0

        if "host" == name:
            self.inhost = 0

        if "mac" == name:
            mac = nw(self.ch)

            dict = { \
                "host" : self.host, \
                "vif" : self.vif, \
                "port" : self.port, \
                "cable" : self.cable, \
                "type" : self.type, \
                "man" : self.man, \
                "use" : self.use, \
                "mac" : mac}
            self.list = self.list + [dict]

        self.ch = ""
    
class Xtconfig(saxutils.DefaultHandler):
    """
    Parse a <testbed_config> file and return a list of dictionaries which
    allows the contents to be referenced by name.
    """

    # A typical entry is of this form:
    #
    # <vlan name="xorp0-xorp1" ipv4="10.0.6.254" mask="255.255.255.248">  
    #         <host name="xorp0">                                        
    #                 <ipv4 name="t3" mac="00:80:c8:b9:3f:60" port="c1"> 
    #                         10.0.6.1                                   
    #                 </ipv4>                                            
    #         </host>                                                    
    #         <host name="xorp1">                                        
    #                 <ipv4 name="t3" mac="00:80:c8:b9:05:5c" port="c2"> 
    #                         10.0.6.2                                   
    #                 </ipv4>                                            
    #         </host>                                                    
    # </vlan>
    #
    # This will be returned as a list of dictionaries of the form:
    #
    # {"vlan":"xorp0-xorp1", "network":"10.0.6.254", "mask":"255.255.255.248",
    #   "host":"xorp0", "vif":"t3", "mac":"00:80:c8:b9:3f:60", "port" : "c1",
    #   "addr" : "10.0.6.1"}

    # There may also be kernel entries:
    #
    # <kernel>
    # <src> /home/xorpc/u2/freebsd.kernels/kernel </src>
    # <dst> xorp3 </dst>
    # </kernel>
    # 
    # These entries will be returned (source, destination) pairs.
    # [["/home/xorpc/u2/freebsd.kernels/kernel", "xorp0"],
    #  ["/home/xorpc/u2/freebsd.kernels/kernel", "xorp1"]]
    #

    # There may also be noconfig entries:
    #
    # <noconfig>
    # xorp0
    # </noconfig>
    #
    
    # There may also be a router entry
    #
    # <router>
    #   xorp0
    # </router>
    #
    # This entry will be returned as a single string
    # "xorp0"
    
    inconfig = 0
    inkernel = 0
    insrc = 0
    indst = 0
    invlan = 0
    inhost = 0

    list = []

    src = ""
    dst = ""
    kernels = []
    noconfig = []
    
    router = ""
    
    def __init__(self, config, debug = 0):
	self.config = config
	self.debug = debug

    def characters(self, ch):
        self.ch += ch

    def start(self):
	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(self)
	parser.parse(self.config)

        return self.list, self.kernels, self.noconfig, self.router

    def startElement(self, name, attrs):
	self.ch = ""

        if "testbed_config" == name:
            self.inconfig = 1

        if self.inconfig != 1:
            return

        if "kernel" == name:
            self.inkernel = 1

        if 1 == self.inkernel:
            if "src" == name:
                self.insrc = 1
            if "dst" == name:
                self.indst = 1
            return
            
        if "noconfig" == name:
            return

        if "router" == name:
            return

        if "vlan" == name:
            self.invlan = 1
            self.vlan = getAttrValue(attrs, 'name')
            self.network = getAttrValue(attrs, 'ipv4')
            self.mask = getAttrValue(attrs, 'mask')
		
        if self.invlan != 1:
            return

        if "host" == name:
            self.inhost = 1
            self.host = getAttrValue(attrs, 'name')

        if self.inhost != 1:
            return

        if "ipv4" == name:
            self.vif = getAttrValue(attrs, 'name')
            self.mac = getAttrValue(attrs, 'mac')
            self.port = getAttrValue(attrs, 'port')
        
    def endElement(self, name):
        if "testbed_config" == name:
            self.inconfig = 0

        if "kernel" == name:
            self.kernels += [[self.src, self.dst]]
            self.src = self.dst = ""
            self.inkernel = 0

        if 1 == self.inkernel:
            if "src" == name and 1 == self.insrc:
                self.src = nw(self.ch)
                self.insrc = 0
            if "dst" == name and 1 == self.indst:
                self.dst = nw(self.ch)
                self.indst = 0

        if "noconfig" == name:
            self.noconfig = self.noconfig + [nw(self.ch)]
            self.ch = ""
            return

        if "router" == name:
            self.router = nw(self.ch)
            self.ch = ""
            return

        if "vlan" == name:
            self.invlan = 0

        if "host" == name:
            self.inhost = 0

        if "ipv4" == name:
            address = nw(self.ch)

            dict = { \
                "vlan" : self.vlan, \
                "network" : self.network, \
                "mask" : self.mask, \
                "host" : self.host, \
                "vif" : self.vif, \
                "mac" : self.mac, \
                "port" : self.port, \
                "addr" : address}
            self.list = self.list + [dict]

        self.ch = ""

def getAttrValue(attrs, value):
    """
    Extract an xml attribute. If the attribute does't exist return the
    empty string.
    """
    
    try:
        result = nw(attrs.getValue(value))
    except:
        result = ""

    return result

us=\
"""usage: %s [-h|--help] [-d|--debug] [-v|-values]
-p|--physical xorp.xml | -c|--config config.xml"""

def main():
    def usage():
	print us % sys.argv[0]
    try:
	opts, args = getopt.getopt(sys.argv[1:], "hvp:c:d", \
				   ["help", "values", "physical=", "config=",
                                   "debug"])
    except getopt.GetoptError:
	usage()
	sys.exit(1)

    physical = ""
    config = ""
    debug = 0
    values = 0
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
	if o in ("-v", "--values"):
            values = 1
	if o in ("-p", "--physical"):
	    physical = a
	if o in ("-c", "--config"):
	    config = a
	if o in ("-d", "--debug"):
	    debug = 1

    p = 0
    if "" == physical and "" == config:
        p = Physical(debug)
        physical = p.get()

    if "" != physical:
        c = Xtphysical(physical, debug).start()
        for i in c:
            if 1 == values:
                print i.values()
            else:
                print i

    if p:
        p.remove()
    
    if "" != config:
        c, k, n, r = Xtconfig(config, debug).start()
        for i in c:
            if 1 == values:
                print i.values()
            else:
                print i
        for i in k:
            print i
        for i in n:
            print i
        if r:
            print r
        
    sys.exit()

if __name__ == '__main__':
    main()
