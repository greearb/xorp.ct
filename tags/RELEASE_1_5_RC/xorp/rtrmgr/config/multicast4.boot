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

plumbing {
    mfea4 {
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
    igmp {
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
	mrib-route 10.30.0.0/16 {
	    next-hop: 10.10.10.20
	}
    }
}
*/

protocols {
    pimsm4 {
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
	    rp 10.10.10.10 {
		group-prefix 224.0.0.0/4 {
		}
	    }
	}
/*
	bootstrap {
	    disable: false
	    cand-bsr {
		scope-zone 224.0.0.0/4 {
		    cand-bsr-by-vif-name: "dc0"
		}
	    }
	    cand-rp {
		group-prefix 224.0.0.0/4 {
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
