/*
** Configuration file for xorp0 that corrsponds to config4.xt.
**
** $XORP: xorp/rtrmgr/fea.boot,v 1.2 2002/03/18 20:23:02 atanu Exp $
*/
interfaces {
  interface.dc0 {
	disable;
    description: "control interface";
    mac: "00:82:c8:b9:3f:5d";
    vif.dc0 {
      multipoint;
      family . inet {
        address."10.0.4.100" {
          netmask: 24;
          broadcast: 10.0.4.255;
        }
        address."192.168.0.0" {
          netmask: 16;
          broadcast: 192.168.255.255;
        }
      }
    }
  }
}

interfaces {
  interface.dc1 {
    disable;
    description: "control interface";
    vif.dc1 {
      multipoint;
      family . inet {
        address."10.0.1.100" {
          netmask: 24;
          broadcast: 10.0.1.255;
        }
      }
    }
  }
}

interfaces {
  interface.dc2 {
    disable;
    description: "control interface";
    vif.dc2 {
      multipoint;
      family . inet {
        address."10.0.2.100" {
          netmask: 24;
          broadcast: 10.0.2.255;
        }
      }
    }
  }
}

interfaces {
  interface.dc3 {
    disable;
    description: "control interface";
    vif.dc3 {
      multipoint;
      family . inet {
        address."10.0.3.100" {
          netmask: 24;
          broadcast: 10.0.3.255;
        }
      }
    }
  }
}
