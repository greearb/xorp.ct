interfaces {
  interface xl0 {
    description: "control interface"
    vif xl0 {
      address 192.150.187.7 {
        prefix-length: 24
        broadcast: 192.150.187.255
      }
    }
  }
}

/*
protocols {
  ospf {
    router-id: 192.150.187.20
    area 0.0.0.0 {
      stub: false
      interface xl0 {
	hello-interval: 5
      }
    }
  }
}
*/

protocols {
  bgp {
    bgp-id: 192.150.187.20
    local-as: 65009
    peer xorp-c4000 {
      local-ip: 192.150.187.20
      peer-ip: 192.150.187.109
      as: 65000
      holdtime: 90
      next-hop: 192.150.187.20
    }
  }
}