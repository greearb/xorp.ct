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
}

fea {
    unicast-forwarding6 {
	disable: false
    }
}

plumbing {
    mfea6 {
	disable: false
	interface dc0 {
	    vif dc0 {
		disable: false
	    }
	}
	interface dc1 {
	    vif dc1 {
		disable: false
	    }
	}
	interface register_vif {
	    vif register_vif {
		/* Note: this vif should be always enabled */
		disable: false
	    }
	}

/*
	traceoptions {
	    flag all {
		disable: false
	    }
	}
*/
    }
}

protocols {
    mld {
	interface dc0 {
	    vif dc0 {
		disable: false
	    }
	}
	interface dc1 {
	    vif dc1 {
		disable: false
	    }
	}

/*
	traceoptions {
	    flag all {
		disable: false
	    }
	}
*/
    }
}

/*
protocols {
    static {
	mrib-route 2001:DB8:AAAA:20::/64 {
	    next-hop: 2001:DB8:10:10:10:10:10:30
	}
    }
}
*/

protocols {
    pimsm6 {
	interface dc0 {
	    vif dc0 {
		disable: false
	    }
	}
	interface dc1 {
	    vif dc1 {
		disable: false
	    }
	}
	interface register_vif {
	    vif register_vif {
		/* Note: this vif should be always enabled */
		disable: false
	    }
	}

	/* Note: static-rps and bootstrap should not be mixed */
	static-rps {
	    rp 2001:DB8:10:10:10:10:10:10 {
		group-prefix ff00::/8 {
		}
	    }
	}
/*
	bootstrap {
	    disable: false
	    cand-bsr {
		scope-zone ff00::/8 {
		    cand-bsr-by-vif-name: "dc0"
		}
	    }
	    cand-rp {
		group-prefix ff00::/8 {
		    cand-rp-by-vif-name: "dc0"
		}
	    }
	}
*/

	switch-to-spt-threshold {
	    /* approx. 1K bytes/s (10Kbps) threshold */
	    disable: false
	    interval: 100
	    bytes: 102400
	}

/*
	traceoptions {
	    flag all {
		disable: false
	    }
	}
*/
    }
}

protocols {
    fib2mrib {
	disable: false
    }
}
