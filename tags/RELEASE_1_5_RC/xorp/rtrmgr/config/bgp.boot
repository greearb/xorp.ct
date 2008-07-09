/* $XORP$ */

interfaces {
    interface dc0 {
	vif dc0 {
	    address 10.10.10.10 {
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
    bgp {
	bgp-id: 10.10.10.10
	local-as: 65002
	export: "export-connected"

	peer 10.30.30.30 {
	    local-ip: 10.10.10.10
	    as: 65000
	    next-hop: 10.10.10.20
	}

/*
	traceoptions {
	    flag {
		verbose
		message-in
		message-out
		state-change
		policy-configuration
	    }
	}
*/
    }
}
