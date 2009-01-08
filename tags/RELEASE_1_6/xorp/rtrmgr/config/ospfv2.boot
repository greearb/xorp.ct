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
    interface dc2 {
	vif dc2 {
	    address 10.30.30.30 {
		prefix-length: 24
	    }
	}
    }
    interface dc3 {
	vif dc3 {
	    address 10.40.40.40 {
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
    ospf4 {
	router-id: 10.10.10.10

	area 0.0.0.0 {

/*
	    virtual-link 20.20.20.20 {
		transit-area: 0.0.0.1
	    }
*/

/*
	    area-range: 10.0.0.0/8 {
		advertise: true
	    }
*/

	    interface dc0 {
		/* link-type: "broadcast" */
		vif dc0 {
		    address 10.10.10.10 {
			/* priority: 128 */
			/* hello-interval: 10 */
			/* router-dead-interval: 40 */
			/* interface-cost: 1 */
			/* retransmit-interval: 5 */
			/* transit-delay: 1 */

/*
			authentication {
			    simple-password: "password";

			    md5 @: u32 {
				password: "password";
				start-time: "2006-01-01.12:00"
				end-time: "2007-01-01.12:00"
				max-time-drift: 3600
			    }
			}
*/

			/* passive: false */

			/* disable: false */			

		    }
		}
	    }
	}

	area 0.0.0.1 {
	    /* area-type: "normal" */	

	    interface dc1 {
		vif dc1 {
		    address 10.10.11.10 {
		    }
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
		    address 10.10.12.10 {
		    }
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
		    address 10.10.13.10 {
		    }
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
