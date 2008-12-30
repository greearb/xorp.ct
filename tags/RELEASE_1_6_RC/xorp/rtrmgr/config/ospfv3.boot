/* $XORP$ */

interfaces {
    interface dc0 {
	vif dc0 {
	    address 2001:DB8:10:10:10:10:10:10 {
		prefix-length: 64
	    }
	    /* Note: The IPv6 link-local address must be configured */
	    address fe80::1111:1111:1111:1111 {
		prefix-length: 64
	    }
	}
    }
    interface dc1 {
	vif dc1 {
	    address 2001:DB8:20:20:20:20:20:20 {
		prefix-length: 64
	    }
	    /* Note: The IPv6 link-local address must be configured */
	    address fe80::2222:2222:2222:2222 {
		prefix-length: 64
	    }
	}
    }
    interface dc2 {
	vif dc2 {
	    address 2001:DB8:30:30:30:30:30:30 {
		prefix-length: 64
	    }
	    /* Note: The IPv6 link-local address must be configured */
	    address fe80::3333:3333:3333:3333 {
		prefix-length: 64
	    }
	}
    }
    interface dc3 {
	vif dc3 {
	    address 2001:DB8:40:40:40:40:40:40 {
		prefix-length: 64
	    }
	    /* Note: The IPv6 link-local address must be configured */
	    address fe80::4444:4444:4444:4444 {
		prefix-length: 64
	    }
	}
    }
}

fea {
    unicast-forwarding6 {
	disable: false
    }
}

protocols {
    ospf6 0 {	/* Instance ID */
	router-id: 10.10.10.10

	/* export: "static" */

	area 0.0.0.0 {
/*
	    virtual-link 20.20.20.20 {
		transit-area: 0.0.0.1
	    }
*/

/*
	    area-range: 2001:DB8:20:20::/64 {
		advertise: true
	    }
*/

	    interface dc0 {
		/* link-type: "broadcast" */
		vif dc0 {
		    /* priority: 128 */
		    /* hello-interval: 10 */
		    /* router-dead-interval: 40 */
		    /* interface-cost: 1 */
		    /* retransmit-interval: 5 */
		    /* transit-delay: 1 */

		    /* passive: false */

		    /* disable: false */			
		}
	    }
	}

	area 0.0.0.1 {
	    /* area-type: "normal" */	

	    interface dc1 {
		vif dc1 {
		}
	    }
	}

/*
	area 0.0.0.2 {
	    area-type: "stub"

	    default-lsa {
		metric: 0
	    }

	    summaries {
		disable: false
	    }
	
	    interface dc2 {
		vif dc2 {
		}
	    }
	}
*/

/*
	area 0.0.0.3 {
	    area-type: "nssa"

	    default-lsa {
		metric: 0
	    }

	    summaries {
		disable: true
	    }
	
	    interface dc3 {
		vif dc3 {
		}
	    }
	}
*/

/*
	traceoptions {
	    flag {
		all {
		    disable: false
		}
	    }
	}
*/
    }
}
