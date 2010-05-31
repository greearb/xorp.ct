/* $XORP$ */

/*
 * See xorp/mibs/snmpdscripts/README on how to configure Net-SNMP in your host
 * before adding the snmp section below to your configuration.
 * Also check that the "bgp4_mib_1657.so" exists in the correct location.
 */
protocols {
    snmp {
	mib-module bgp4_mib_1657 {
	    abs-path: "/usr/local/xorp/mibs/bgp4_mib_1657.so"
	}
    }
}
