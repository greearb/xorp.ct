/*XORP Configuration File*/
/* $XORP: xorp/rtrmgr/config.boot,v 1.18 2004/05/16 07:15:16 pavlin Exp $ */

/* router config file for tinderbox test on xorp8 */ 

/*
 * Please don't modify this file for use on other machines -
 * use the -b command line flag to rtrmgr instead.
 */

interfaces {
    interface rl0 {
	description: "control interface"
	/* default-system-config */
	vif rl0 {
	    address 192.150.187.108 {
		prefix-length: 25
		broadcast: 192.150.187.255
	    }
	}
    }
}

fea {
    enable-unicast-forwarding4: true
    /* enable-unicast-forwarding6: true */
}

/*
protocols {
    static {
	route4 10.10.0.0/16 {
	    nexthop: 192.150.187.108
	}

	route4 10.20.0.0/16 {
	    nexthop: 192.150.187.108
	    metric: 10
	}
    }
}
*/

/*
protocols {
    ospf {
	router-id: 192.150.187.20
	area 0.0.0.0 {
	    stub: false
	    interface xl0 {
		hello-interval: 5
	    }
	}
    }
}
*/

protocols {
    bgp {
	bgp-id: 192.150.187.108
	local-as: 65017
	peer 192.150.187.109 {
	    local-ip: 192.150.187.108
	    as: 65000
	    holdtime: 90
	    next-hop: 192.150.187.108

	    enable-ipv4-multicast

	    enable-ipv6-unicast
	    enable-ipv6-multicast
	}
    }
}

/*
 * See xorp/mibs/snmpdscripts/README on how to configure Net-SNMP in your host
 * before uncommenting the snmp section below.
 */

/* 
protocols {
    snmp {
	mib-module bgp4_mib_1657 {
	    abs-path: "/home/panther/u0/jcardona/nobackup/cvs/xorp/mibs/bgp4_mib_1657.so"
	}
    }
}
*/
