/* $XORP$ */

interfaces {
    interface dc0 {
	vif dc0 {
	    address 2001:DB8:10:10:10:10:10:10 {
		prefix-length: 64
	    }
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
	    address fe80::2222:2222:2222:2222 {
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

policy {
    policy-statement export-connected {
	term 100 {
	    from {
		protocol: "connected"
	    }
	}
    }
}

protocols {
    ripng {
	/* Connected interfaces are advertised if explicitly exported */
	export: "export-connected"

	interface dc0 {
	    vif dc0 {
		address fe80::1111:1111:1111:1111 {
		    disable: false
		}
	    }
	}
	interface dc1 {
	    vif dc1 {
		address fe80::2222:2222:2222:2222 {
		    disable: false
		}
	    }
	}
    }
}
