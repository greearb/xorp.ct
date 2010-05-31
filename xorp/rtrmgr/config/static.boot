/* $XORP$ */

interfaces {
    interface dc0 {
	vif dc0 {
	    address 10.10.10.10 {
		prefix-length: 24
	    }
	}
    }
    interface dc1 {
	vif dc1 {
	    address 10.20.20.20 {
		prefix-length: 24
	    }
	}
    }
}

fea {
    unicast-forwarding4 {
	disable: false
    }
}

protocols {
    static {
	route 10.30.0.0/16 {
	    next-hop: 10.10.10.20
	}
    }
}
