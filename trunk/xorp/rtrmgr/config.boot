interfaces {
  interface rl0 {
    description: "control interface"
    vif rl0 {
      address 192.150.187.67 {
        prefix-length: 25
        broadcast: 192.150.187.127
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
    bgp-id: 192.150.187.67
    local-as: 65030
    peer xorp-c4000 {
      local-ip: 192.150.187.67
      peer-ip: 192.150.187.109
      as: 65000
      holdtime: 90
      next-hop: 192.150.187.67
    }
  }
}


