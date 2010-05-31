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
    rip {
	/* Connected interfaces are advertised if explicitly exported */
	export: "export-connected"

	interface dc0 {
	    vif dc0 {
		address 10.10.10.10 {
		    disable: false
/*
		    authentication {
			simple-password: "password";
			md5 @: u32 {
			    password: "password";
			    start-time: "2006-01-01.12:00"
			    end-time: "2007-01-01.12:00"
			}
		    }
*/
		}
	    }
	}
	interface dc1 {
	    vif dc1 {
		address 10.20.20.20 {
		    disable: false
		}
	    }
	}
    }
}
