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
	/* Use the default setup as configured in the system */
	default-system-config
    }
    interface discard0 {
	description: "discard interface"
	disable: false
	discard: true
	vif discard0 {
	    disable: false
	    address 192.0.2.1 {
		prefix-length: 32
		disable: false
	    }
	}
    }
}
