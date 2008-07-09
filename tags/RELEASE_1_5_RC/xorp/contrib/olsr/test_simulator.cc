// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/contrib/olsr/test_simulator.cc,v 1.1 2008/04/24 15:19:55 bms Exp $"

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/tokenize.hh"
#include "libxorp/test_main.hh"

#include "olsr.hh"
#include "debug_io.hh"
#include "emulate_net.hh"
//#include "delay_queue.hh"
//#include "vertex.hh"

#include "test_args.hh"

static const uint16_t MY_OLSR_PORT = 6698;

// TODO: Simulation of ETX links, and verification thereof.
// TODO: Support verification of HNA metrics and multiple matches.

class NoSuchAddress : public XorpReasonedException {
public:
    NoSuchAddress(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrNoSuchAddress", file, line, init_why) {}
};

class Simulator;

class NodeTuple {
public:
    NodeTuple() : _id(0), _olsr(0), _io(0) {}

    explicit NodeTuple(const int id,
		       TestInfo* info,
		       Olsr* olsr,
		       DebugIO* io,
		       const string& ifvifname,
		       const string& instancename)
     : _id(id),
       _info(info),
       _olsr(olsr),
       _io(io),
       _ifvifname(ifvifname),
       _instancename(instancename)
    {}

    int id() const { return _id; }
    TestInfo* info() { return _info; }

    Olsr* olsr() { return _olsr; }
    DebugIO* io() { return _io; }

    string ifname() const { return _ifvifname; }
    string vifname() const { return _ifvifname; }
    string instancename() const { return _instancename; }

private:
    int		_id;
    TestInfo*	_info;
    Olsr*	_olsr;
    DebugIO*	_io;
    string	_ifvifname;
    string	_instancename;
};

class Nodes {
public:
    Nodes(Simulator* r, EventLoop& ev);
    ~Nodes();

    /**
     * Simulation graph creation.
     */

    /**
     * Create an OLSR node in the simulator.
     *
     * @param addrs the interface addresses to create the node with;
     *              the first address is always the main address.
     * @return true if ok, false if any error occurred.
     */
    bool create_node(const vector<IPv4>& addrs);

    /**
     * Destroy an OLSR node in the simulator.
     *
     * @param main_addr the main address of the node to destroy.
     * @return true if ok, false if any error occurred.
     */
    bool destroy_node(const IPv4& main_addr);

    /**
     * Given the main address of an OLSR node, destroy all links
     * referencing that node.
     *
     * @param main_addr The OLSR interface address of the node.
     * @return the number of links which were destroyed.
     * @throw NoSuchAddress if link_addr does not exist.
     */
    size_t purge_links_by_node(const IPv4& main_addr)
	throw(NoSuchAddress);

    /**
     * Mark an OLSR node address as administratively up.
     *
     * @param iface_addr the interface address to configure up.
     * @return true if ok, false if any error occurred.
     */
    bool configure_address(const IPv4& iface_addr);

    /**
     * Mark an OLSR node address as administratively down.
     *
     * @param iface_addr the interface address to configure down.
     * @return true if ok, false if any error occurred.
     */
    bool unconfigure_address(const IPv4& iface_addr);

    /*
     * default protocol variable settings;
     * don't take effect until a new node is created.
     */
    inline void set_default_hello_interval(const int value) {
	_default_hello_interval = value;
    }

    inline void set_default_mid_interval(const int value) {
	_default_mid_interval = value;
    }

    inline void set_default_mpr_coverage(const int value) {
	_default_mpr_coverage = value;
    }

    inline void set_default_refresh_interval(const int value) {
	_default_refresh_interval = value;
    }

    inline void set_default_tc_interval(const int value) {
	_default_tc_interval = value;
    }

    inline void set_default_hna_interval(const int value) {
	_default_hna_interval = value;
    }

    inline void set_default_dup_hold_time(const int value) {
	_default_dup_hold_time = value;
    }

    inline void set_default_willingness(const int value) {
	_default_willingness = value;
    }

    inline void set_default_tc_redundancy(const int value) {
	_default_tc_redundancy = value;
    }

    /*
     * per-node settings
     */

    /**
     * Set HELLO_INTERVAL variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the HELLO_INTERVAL protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_hello_interval(const IPv4& main_addr, const int value);

    /**
     * Set MID_INTERVAL variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the MID_INTERVAL protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_mid_interval(const IPv4& main_addr, const int value);

    /**
     * Set MPR_COVERAGE variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the MPR_COVERAGE protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_mpr_coverage(const IPv4& main_addr, const int value);

    /**
     * Set REFRESH_INTERVAL variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the REFRESH_INTERVAL protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_refresh_interval(const IPv4& main_addr, const int value);

    /**
     * Set TC_INTERVAL variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the TC_INTERVAL protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_tc_interval(const IPv4& main_addr, const int value);

    /**
     * Set TC_REDUNDANCY variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the TC_REDUNDANCY protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_tc_redundancy(const IPv4& main_addr, const int value);

    /**
     * Set WILLINGNESS variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the WILLINGNESS protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_willingness(const IPv4& main_addr, const int value);

    /**
     * Set HNA_INTERVAL variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the HNA_INTERVAL protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_hna_interval(const IPv4& main_addr, const int value);

    /**
     * Set DUP_HOLD_TIME variable for the node given by main_addr.
     *
     * @param main_addr the address of the node to set the variable for.
     * @param value the new value of the DUP_HOLD_TIME protocol variable.
     * @return true if ok, false if any error occurred.
     */
    bool set_dup_hold_time(const IPv4& main_addr, const int value);

    /**
     * Select the OLSR node with the given main address for the
     * verification commands which follow.
     *
     * @param main_addr the address of the node to select.
     * @return true if ok, false if any error occurred.
     */
    bool select_node(const IPv4& main_addr);

    /**
     * Dump a node's routing table to cout.
     *
     * @param main_addr the address of the node to dump routes for.
     * @return true if ok, false if any error occurred.
     */
    bool dump_routing_table(const IPv4& main_addr);

    /*
     * Neighborhood verification
     */

    /**
     * Verify that the link state database at every node in the topology
     * is empty (N1/N2/TC/MPR/MID).
     * HNA state is NOT checked.
     *
     * @return true if ok, false if any error occurred.
     */
    bool verify_all_link_state_empty();

    /**
     * Verify that the link state database in the given node
     * is empty (N1/N2/TC/MPR/MID).
     * HNA state is NOT checked.
     *
     * @param nat the NodeTuple to verify.
     * @return true if ok, false if any error occurred.
     */
    bool verify_node_link_state_empty(NodeTuple& nat);

    /**
     * Verify that the neighbor given by n1_addr is a one-hop neighbor
     * of the currently selected node.
     *
     * @param n1_addr the address of the one-hop neighbor to verify.
     * @param expected_is_n1 the expected status
     * @return true if ok, false if any error occurred.
     */
    bool verify_n1(const IPv4& n1_addr, const bool expected_is_n1);

    /**
     * Verify that n2_addr is the address of a two-hop neighbor
     * of the currently selected node.
     *
     * @param n2_addr the address of the two-hop neighbor to verify.
     * @param expected_is_n2 the expected status
     * @return true if ok, false if any error occurred.
     */
    bool verify_n2(const IPv4& n2_addr, const bool expected_is_n2);

    /*
     * MPR verification
     */

    /**
     * Verify the MPR status of the selected node.
     *
     * @param expected_is_mpr the expected MPR status.
     * @return true if ok, false if any error occurred.
     */
    bool verify_is_mpr(const bool expected_is_mpr);

    /**
     * Verify the MPR set of the selected node.
     *
     * @param mpr_addrs the expected contents of the MPR set.
     * @return true if ok, false if any error occurred.
     */
    bool verify_mpr_set(const vector<IPv4>& mpr_addrs);

    /**
     * Verify the MPR selector set of the selected node.
     *
     * @param mprs_addrs the expected contents of the MPR selector set.
     * @return true if ok, false if any error occurred.
     */
    bool verify_mpr_selector_set(const vector<IPv4>& mprs_addrs);

    /*
     * Topology verification
     */

    /**
     * Verify the MID database of the selected node contains the
     * expected number of nodes
     * (Note: this is NOT the same as entries; rather, it's the
     *  number of unique keys we have in the _mid_addr multimap.)
     *
     * @param expected_size the expected dize of the MID table.
     * @return true if ok, false if any error occurred.
     */
    bool verify_mid_node_count(const size_t expected);

    /**
     * Verify the MID database of the selected node contains the given
     * entries for the given protocol address.
     *
     * @param origin the main address to look up in the MID database.
     * @param addrs the expected addresses for this node; may be empty.
     * @return true if ok, false if any error occurred.
     */
    bool verify_mid_node_addrs(const IPv4& origin,
			       const vector<IPv4>& addrs);

    /**
     * Verify the MID database of the selected node contains an entry
     * for the given main address and interface address, at the given
     * network distance measured in hops.
     *
     * @param main_addr the main address to look up in the MID database.
     * @param iface_addr the interface address for the MID entry.
     * @param expected_distance the expected distance in hops.
     * @return true if ok, false if any error occurred.
     */
    bool verify_mid_address_distance(const IPv4& origin,
				     const IPv4& iface_addr,
				     const uint16_t expected_distance);

    /**
     * Verify the TC database of the selected node contains entries which
     * have originated from the given number of unique OLSR nodes.
     *
     * @param expected_count the expected number of TC origins.
     * @return true if ok, false if any error occurred.
     */
    bool verify_tc_origin_count(const size_t expected_count);

    /**
     * Verify that the Advertised Neighbor Set of 'origin', as seen
     * at the currently selected node, has the expected ANSN and the
     * expected neighbor set contents.
     *
     * @param origin the origin to look up in the TC database.
     * @param expected_ansn the ANSN we expect to see.
     * @param expected_addrs the expected neighbor set contents;
     *                       may be empty to indicate set should be empty.
     * @return true if ok, false if any error occurred.
     */
    bool verify_tc_ans(const IPv4& origin,
		       const uint16_t expected_ansn,
		       const vector<IPv4>& expected_addrs);

    /**
     * Verify that the selected node has seen TC broadcasts from 'origin'
     * at least once during the lifetime of the simulation.
     *
     * If it has never been seen then TopologyManager::get_tc_neighbor_set()
     * will throw an exception -- there is a check which takes place to
     * look for final recorded ANSN values.
     *
     * @param origin the origin to look up in the TC database.
     * @param expected_seen true if we expect to have seen the origin.
     * @return true if ok, false if any error occurred.
     */
    bool verify_tc_origin_seen(const IPv4& origin,
			       const bool expected_seen);

    /**
     * Verify that the given TC entry has the expected hop count, as observed
     * at the currently selected node's TC database.
     *
     * NOTE: The distance thus observed can never be less than 2, this
     * accounts for the hop used to reach us, and the hop from the TC's
     * origin to the advertised neighbor. This is used for the local TC
     * routing computation.
     *
     * @param origin the origin to look up in the TC database.
     * @param neighbor_addr the neighbor to look up for origin.
     * @param expected_distance the distance we expect to see recorded
     *                          for this TC entry.
     * @return true if ok, false if any error occurred.
     */
    bool verify_tc_distance(const IPv4& origin,
			    const IPv4& neighbor_addr,
			    const uint16_t expected_distance);

    /**
     * Verify that the currently selected node has a given number of
     * TC entries pointing to the given destination.
     *
     * @param addr the destination to look up in the TC database.
     * @param expected_count the number of TC entries we expect to see.
     * @return true if ok, false if any error occurred.
     */
    bool verify_tc_destination(const IPv4& dest_addr,
			       const size_t expected_count);

    /*
     * External routing verification
     */

    /**
     * Verify the HNA database of the selected node contains an entry
     * for the given destination from the given origin.
     *
     * @param dest the IPv4 destination to look up in HNA.
     * @param origin the expected OLSR main address of the origin.
     * @return true if ok, false if any error occurred.
     */
    bool verify_hna_entry(const IPv4Net& dest, const IPv4& origin);

    /**
     * Verify the HNA database of the selected node contains an entry
     * for the given destination from the given origin, at
     * the given distance.
     *
     * @param dest the IPv4 destination to look up in HNA.
     * @param origin the expected OLSR main address of the origin.
     * @param distance the expected distance to the origin, as measured
     *                 from the HNA messages themselves.
     * @return true if ok, false if any error occurred.
     */
    bool verify_hna_distance(const IPv4Net& dest, const IPv4& origin,
			     const uint16_t distance);

    /**
     * Verify the HNA database of the selected node contains
     * exactly the given number of entries.
     *
     * @param expected_count the expected number of HNA entries.
     * @return true if ok, false if any error occurred.
     */
    bool verify_hna_entry_count(const size_t expected_count);

    /**
     * Verify the HNA database of the selected node contains entries which
     * have originated from the given number of unique OLSR nodes.
     *
     * @param expected_count the expected number of HNA origins.
     * @return true if ok, false if any error occurred.
     */
    bool verify_hna_origin_count(const size_t expected_count);

    /**
     * Verify the HNA database of the selected node contains the
     * given number of leanred network prefixes.
     *
     * @param expected_count the expected number of HNA prefixes.
     * @return true if ok, false if any error occurred.
     */
    bool verify_hna_dest_count(const size_t expected_count);

    /**
     * Add a prefix to the HNA "routes out" database of the currently
     * selected node, and start advertising it.
     *
     * @param dest the network prefix to originate.
     * @return true if ok, false if any error occurred.
     */
    bool originate_hna(const IPv4Net& dest);

    /**
     * Withdraw a prefix from the HNA "routes out" database of the currently
     * selected node, and stop advertising it.
     *
     * @param dest the network prefix to withdraw.
     * @return true if ok, false if any error occurred.
     */
    bool withdraw_hna(const IPv4Net& dest);

    /*
     * General routing verification
     */

    /**
     * Verify the routing table of the selected node contains exactly
     * the given number of entries.
     *
     * @param rtsize the expected number of routing table entries.
     * @return true if ok, false if any error occurred.
     */
    bool verify_routing_table_size(const size_t rtsize);

    /**
     * Verify the routing table of the selected node contains the
     * given route entry.
     * TODO deal with multiple matches.
     *
     * @param dest the destination.
     * @param nexthop the expected next-hop of the entry.
     * @param metric the expected metric of the entry.
     * @return true if ok, false if any error occurred.
     */
    bool verify_routing_entry(IPv4Net dest, IPv4 nexthop, uint32_t metric);

    /*
     * Utility methods.
     */

    /**
     * Given an OLSR protocol address, return the corresponding
     * NodeTuple instance by value.
     *
     * @param addr the OLSR protocol address to look up in the simulator.
     * @return the NodeTuple corresponding to addr.
     * @throw NoSuchAddr if the address does not exist.
     */
    NodeTuple& get_node_tuple_by_addr(const IPv4& addr) throw(NoSuchAddress);

protected:
    inline bool node_is_selected() {
	return (_selected_node.olsr() != 0);
    }

private:
    Simulator* _parent;
    EventLoop& _eventloop;

    /**
     * @short The next available instance ID for a pair of
     * OLSR/DebugIO instances.
     */
    int		_next_instance_id;

    /*
     * protocol defaults
     */
    int		_default_hello_interval;
    int		_default_mid_interval;
    int		_default_mpr_coverage;
    int		_default_refresh_interval;
    int		_default_tc_interval;
    int		_default_hna_interval;
    int		_default_dup_hold_time;
    int		_default_willingness;
    int		_default_tc_redundancy;

    /**
     * Database of current simulation instances, keyed by protocol
     * address; one entry for each OLSR address.
     * Multiple addresses may map to the same OLSR instance (see MID).
     */
    map<IPv4, NodeTuple>	_nodes;

    /**
     * The currently selected node.
     */
    NodeTuple			_selected_node;
};

class Links {
public:
    /**
     * @short A structure describing an emulated link between two nodes.
     */
    struct LinkTuple {
    public:
	EmulateSubnet*	_es;	// always a unique instance.
	NodeTuple	_left;
	NodeTuple	_right;

	explicit LinkTuple(
	    EmulateSubnet* es,
	    const NodeTuple& left,
	    const NodeTuple& right)
	 : _es(es), _left(left), _right(right)
	 {}
    private:
	LinkTuple() {}
    };

public:
    Links(Simulator* r, EventLoop& ev, Nodes* n)
     : _parent(r),
       _eventloop(ev),
       _nodes(n)
    {}

    ~Links() {}

    /**
     * Add a link between two OLSR instances, given their addresses.
     *
     * If no previously created link exists, a new instance of
     * EmulateSubnet is created. The link local broadcast address
     * is always assumed to be 255.255.255.255 on the link.
     *
     * @param left_addr The OLSR interface address of the left hand node.
     * @param right_addr The OLSR interface address of the right hand node.
     * @return true if the link was successfully created, otherwise false.
     * @throw NoSuchAddress if neither address exists.
     */
    bool add_link(const IPv4& left_addr, const IPv4& right_addr)
	throw(NoSuchAddress);

    /**
     * Remove a link between two OLSR instances, given their addresses.
     *
     * @param left_addr The OLSR interface address of the left hand node.
     * @param right_addr The OLSR interface address of the right hand node.
     * @return true if the link was successfully remove, otherwise false.
     * @throw NoSuchAddress if neither address exists.
     */
    bool remove_link(const IPv4& left_addr, const IPv4& right_addr)
	throw(NoSuchAddress);

    /**
     * Remove all links in the topology, without removing the nodes.
     *
     * @return the number of links removed.
     */
    size_t remove_all_links();

    /**
     * Purge a link given a reference to it.
     *
     * @param lt reference to a LinkTuple.
     * @param left_addr The OLSR interface address of the left hand node.
     * @param right_addr The OLSR interface address of the right hand node.
     */
    void purge_link_tuple(LinkTuple& lt,
			  const IPv4& left_addr,
			  const IPv4& right_addr);

    /**
     * Given the interface address of an OLSR node, remove all links
     * referencing that interface and destroy them.
     *
     * @param left_addr The OLSR interface address of the left hand node.
     * @return true if the link was successfully created, otherwise false.
     * @throw NoSuchAddress if link_addr does not exist.
     */
    bool remove_all_links_for_addr(const IPv4& link_addr)
	throw(NoSuchAddress);

    Simulator* parent() { return _parent; }
    EventLoop& eventloop() { return _eventloop; }
    Nodes* nodes() { return _nodes; }

private:
    Simulator* _parent;
    EventLoop& _eventloop;
    Nodes* _nodes;

    multimap<pair<IPv4, IPv4>, LinkTuple>   _links;
};

class Simulator {
 public:
    Simulator(bool verbose, int verbose_level);
    ~Simulator();

    bool cmd(Args& args) throw(InvalidString);

    /**
     * @short Wait @param secs in EventLoop, whilst running other
     * callbacks.
     */
    void wait(const int secs);

    inline EventLoop& eventloop() { return _eventloop; }
    inline TestInfo& info() { return _info; }
    inline Nodes& nodes() { return _nodes; }
    inline Links& links() { return _links; }

 private:
    // common eventloop must be used for serialization in time domain.
    EventLoop _eventloop;
    TestInfo _info;
    Nodes   _nodes;
    Links   _links;
};

Simulator::Simulator(bool verbose, int verbose_level)
    :  _info("simulator", verbose, verbose_level, cout),
       _nodes(this, _eventloop),
       _links(this,  _eventloop, &_nodes)
{}

Simulator::~Simulator()
{}

void
Simulator::wait(const int secs)
{
    bool timeout = false;
    XorpTimer t = eventloop().set_flag_after(TimeVal(secs ,0), &timeout);
    while (! timeout)
	eventloop().run();
}

Nodes::Nodes(Simulator* r, EventLoop& ev)
 : _parent(r),
   _eventloop(ev),
   _next_instance_id(1),
   _default_hello_interval(1),
   _default_mid_interval(1),
   _default_mpr_coverage(1),
   _default_refresh_interval(1),
   _default_tc_interval(1),
   _default_hna_interval(1),
   _default_dup_hold_time(4),
   _default_willingness(OlsrTypes::WILL_DEFAULT)
{
}

Nodes::~Nodes()
{
}

bool
Nodes::create_node(const vector<IPv4>& addrs)
{
    XLOG_ASSERT(! addrs.empty());

    bool result = true;
    IPv4 main_addr;

    int instanceid = _next_instance_id++;
    string instancename = c_format("olsr%u", XORP_UINT_CAST(instanceid));

    TestInfo* n_info = new TestInfo(instancename,
				    _parent->info().verbose(),
				    _parent->info().verbose_level(),
				    _parent->info().out());

    DebugIO* dio = new DebugIO(*n_info, _eventloop);
    dio->startup();

    Olsr* olsr = new Olsr(_eventloop, dio);

    /*
     * apply simulation-wide defaults.
     */
    olsr->set_hello_interval(TimeVal(_default_hello_interval, 0));
    olsr->set_mid_interval(TimeVal(_default_mid_interval, 0));
    olsr->set_mpr_coverage(_default_mpr_coverage);
    olsr->set_refresh_interval(TimeVal(_default_refresh_interval, 0));
    olsr->set_tc_interval(TimeVal(_default_tc_interval, 0));
    olsr->set_hna_interval(TimeVal(_default_hna_interval, 0));
    olsr->set_dup_hold_time(TimeVal(_default_dup_hold_time, 0));
    olsr->set_willingness(_default_willingness);
    olsr->set_tc_redundancy(_default_tc_redundancy);

    size_t face_index = 0;

    vector<IPv4>::const_iterator ii;
    for (ii = addrs.begin(); ii != addrs.end(); ii++) {
	if (ii == addrs.begin()) {
	    main_addr = (*ii);
	}

	// create, but do not activate, the interface.
	string ifvifname = c_format("eth%u", XORP_UINT_CAST(face_index));
	const string& ifname = ifvifname;
	const string& vifname = ifvifname;

	OlsrTypes::FaceID faceid =
	    olsr->face_manager().create_face(ifname, vifname);

	olsr->face_manager().set_local_addr(faceid, (*ii));
	olsr->face_manager().set_local_port(faceid, MY_OLSR_PORT);

	olsr->face_manager().set_all_nodes_addr(faceid, IPv4::ALL_ONES());
	olsr->face_manager().set_all_nodes_port(faceid, MY_OLSR_PORT);

	//olsr->set_interface_cost(interface_1, vif_1, interface_cost);

	// establish a top level mapping for this node in the simulation.
	// XXX TODO use a list and a map...
	_nodes.insert(make_pair((*ii),
				NodeTuple(instanceid, n_info, olsr, dio,
					  ifvifname,
					  instancename)));

	// now and only now do we activate the interface.
	result = olsr->face_manager().activate_face(ifname, vifname);

	// enable the interface.
	result = olsr->face_manager().set_face_enabled(faceid, true);

	++face_index;
    }

    olsr->set_main_addr(main_addr);
    //olsr->trace().all(verbose);
    //olsr->set_testing(true);

    return result;
}

bool
Nodes::destroy_node(const IPv4& main_addr)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;

    delete nat.olsr();
    delete nat.io();
    delete nat.info();

    _nodes.erase(ii);

    return true;
}

size_t
Nodes::purge_links_by_node(const IPv4& main_addr)
    throw(NoSuchAddress)
{
    size_t found_count = 0;

    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return found_count;
    }

    // use olsr instance pointer as unique key;
    // find all NodeTuple such that olsr instance is same;
    // delete all links referencing each link address.
    Olsr* olsrp = (*ii).second.olsr();
    for (ii = _nodes.begin(); ii != _nodes.end(); ii++) {
	if ((*ii).second.olsr() == olsrp) {
	    _parent->links().remove_all_links_for_addr((*ii).first);
	    ++found_count;
	}
    }

    return found_count;
}

bool
Nodes::configure_address(const IPv4& iface_addr)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(iface_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    OlsrTypes::FaceID faceid = olsr->face_manager().
	get_faceid(nat.ifname(), nat.vifname());

    bool result = olsr->face_manager().set_face_enabled(faceid, true);

    return result;
}

bool
Nodes::unconfigure_address(const IPv4& iface_addr)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(iface_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    OlsrTypes::FaceID faceid = olsr->face_manager().
	get_faceid(nat.ifname(), nat.vifname());

    bool result = olsr->face_manager().set_face_enabled(faceid, false);

    return result;
}

bool
Nodes::set_hello_interval(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_hello_interval(TimeVal(value, 0));

    return result;
}

bool
Nodes::set_mid_interval(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_mid_interval(TimeVal(value, 0));

    return result;
}

bool
Nodes::set_mpr_coverage(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_mpr_coverage(value);

    return result;
}

bool
Nodes::set_refresh_interval(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_refresh_interval(TimeVal(value, 0));

    return result;
}

bool
Nodes::set_tc_interval(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_tc_interval(TimeVal(value, 0));

    return result;
}

bool
Nodes::set_tc_redundancy(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_tc_redundancy(value);

    return result;
}

bool
Nodes::set_willingness(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_willingness(value);

    return result;
}

bool
Nodes::set_hna_interval(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_hna_interval(TimeVal(value, 0));

    return result;
}

bool
Nodes::set_dup_hold_time(const IPv4& main_addr, const int value)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    Olsr* olsr = nat.olsr();

    bool result = olsr->set_dup_hold_time(TimeVal(value, 0));

    return result;
}

bool
Nodes::select_node(const IPv4& main_addr)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    _selected_node = nat;

    return true;
}

bool
Nodes::dump_routing_table(const IPv4& main_addr)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(main_addr);
    if (ii == _nodes.end()) {
	return false;
    }

    NodeTuple& nat = (*ii).second;
    DebugIO* io = nat.io();

    cout << "{{{ Routing table at " << main_addr.str() << ":" << endl;
    io->routing_table_dump(cout);
    cout << "}}}" << endl;

    return true;
}

bool
Nodes::verify_all_link_state_empty()
{
    bool all_empty = true;

    map<IPv4, NodeTuple>::iterator ii;
    for (ii = _nodes.begin(); ii != _nodes.end(); ii++) {
	NodeTuple& nat = (*ii).second;
	if (! verify_node_link_state_empty(nat)) {
	    all_empty = false;
	    break;
	}
    }

    return all_empty;
}

bool
Nodes::verify_node_link_state_empty(NodeTuple& nat)
{
    Olsr* olsr = nat.olsr();
    XLOG_ASSERT(olsr != 0);

    DebugIO* io = nat.io();
    Neighborhood& nh = olsr->neighborhood();
    TopologyManager& tm = olsr->topology_manager();

    // XXX hodgepodge.
    NodeTuple save_selected = _selected_node;
    _selected_node = nat;

    bool is_link_state_empty = false;
    do {
	// N1 database must be empty.
	list<OlsrTypes::NeighborID> n1_list;
	nh.get_neighbor_list(n1_list);
	if (! n1_list.empty())
	    break;

	// N2 database must be empty.
	list<OlsrTypes::TwoHopNodeID> n2_list;
	nh.get_twohop_neighbor_list(n2_list);
	if (! n2_list.empty())
	    break;

	// MPR set must be empty.
	vector<IPv4> mpr_addrs;
	if (! verify_mpr_set(mpr_addrs)) {
	    debug_msg("non-zero MPR set\n");
	    break;
	}

	// MPR selector set must be empty.
	vector<IPv4> mprs_addrs;
	if (! verify_mpr_selector_set(mpr_addrs)) {
	    debug_msg("non-zero MPR selector set\n");
	    break;
	}

	// MID origin count must be 0.
	if (0 != tm.mid_node_count()) {
	    debug_msg("non-zero MID origin count\n");
	    break;
	}

	// TC origin count must be 0.
	if (0 != tm.tc_node_count()) {
	    debug_msg("non-zero TC origin count\n");
	    break;
	}

	// Routing table size must be 0.
	if (0 != io->routing_table_size()) {
	    debug_msg("non-zero routing table size\n");
	    break;
	}

	is_link_state_empty = true;
    } while (0);

    _selected_node = save_selected;

    return is_link_state_empty;
}

bool
Nodes::verify_n1(const IPv4& n1_addr, const bool expected_is_n1)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    bool is_n1 = false;
    try {
	OlsrTypes::NeighborID nid = olsr->neighborhood().
	    get_neighborid_by_main_addr(n1_addr);

	// TODO: Should we verify if it's sym or not?
	UNUSED(nid);

	is_n1 = true;
    } catch (...) {}

    return is_n1 == expected_is_n1;
}

bool
Nodes::verify_n2(const IPv4& n2_addr, const bool expected_is_n2)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    bool is_n2 = false;
    try {
	OlsrTypes::TwoHopNodeID tnid = olsr->neighborhood().
	    get_twohop_nodeid_by_main_addr(n2_addr);

	// TODO: should we look at other properties of this n2?
	UNUSED(tnid);

	is_n2 = true;
    } catch (...) {}

    return is_n2 == expected_is_n2;
}

// shorthand for "verify that my mpr selector set is empty".
bool
Nodes::verify_is_mpr(const bool expected_is_mpr)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    bool is_mpr = olsr->neighborhood().is_mpr();

    return is_mpr == expected_is_mpr;
}

bool
Nodes::verify_mpr_set(const vector<IPv4>& mpr_addrs)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    // NOTE: If for any reason we race with MPR set computation
    // then this test is invalid. We shouldn't because MPR computation
    // takes place strictly within a XorpCallback which can't be
    // preempted by other tasks, and state is updated idempotently.

    //
    // Verify MPR set by direct set comparison. A true invariant.
    //
    bool result = false;
    set<OlsrTypes::NeighborID> expected_mpr_set;

    // Look up runtime neighbor IDs in selected OLSR instance
    // for each provided MPR address and build comparison set.
    vector<IPv4>::const_iterator ii;
    for (ii = mpr_addrs.begin(); ii != mpr_addrs.end(); ii++) {
	try {
	    OlsrTypes::NeighborID nid = olsr->neighborhood().
		get_neighborid_by_main_addr((*ii));
	    expected_mpr_set.insert(nid);
	} catch (...) {
	    throw;
	}
    }

    // Evaluate the invariant by testing for set equality.
    result = (olsr->neighborhood().mpr_set() == expected_mpr_set);

    return result;
}

bool
Nodes::verify_mpr_selector_set(const vector<IPv4>& mprs_addrs)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    //
    // Verify MPR selector set, the long way round: a full lookup
    // and comparison of each member (a true invariant).
    // [MPR selector state is always in phase with HELLO receive processing.]
    //
    bool result = false;
    set<OlsrTypes::NeighborID> expected_mpr_selector_set;

    // Look up runtime neighbor IDs in selected OLSR instance
    // for each provided MPR selector address and build comparison set.
    vector<IPv4>::const_iterator ii;
    for (ii = mprs_addrs.begin(); ii != mprs_addrs.end(); ii++) {
	try {
	    OlsrTypes::NeighborID nid = olsr->neighborhood().
		get_neighborid_by_main_addr((*ii));
	    expected_mpr_selector_set.insert(nid);
	} catch (...) {
	    throw;
	}
    }

    // Evaluate the invariant by testing for set equality.
    result = (olsr->neighborhood().mpr_selector_set() ==
	      expected_mpr_selector_set);

    return result;
}

bool
Nodes::verify_mid_node_count(const size_t expected)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    size_t actual = olsr->topology_manager().mid_node_count();
    if (expected == actual)
	return true;

    debug_msg("expected %u, got %u\n",
	      XORP_UINT_CAST(expected),
	      XORP_UINT_CAST(actual));

    return false;
}

bool
Nodes::verify_mid_node_addrs(const IPv4& origin, const vector<IPv4>& addrs)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    bool is_identical = false;
    try {
	vector<IPv4> actual_addrs = olsr->topology_manager().
					get_mid_addresses(origin);
	is_identical = (addrs == actual_addrs);
    } catch(...) {}

    return is_identical;
}

bool
Nodes::verify_mid_address_distance(const IPv4& origin,
				   const IPv4& iface_addr,
				   const uint16_t expected_distance)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();

    bool is_identical = false;
    try {
	uint16_t actual_distance = olsr->topology_manager().
	    get_mid_address_distance(origin, iface_addr);

	is_identical = (expected_distance == actual_distance);
	if (! is_identical) {
	    debug_msg("expected %u, got %u\n",
		      XORP_UINT_CAST(expected_distance),
		      XORP_UINT_CAST(actual_distance));
	}
    } catch(...) {}

    return is_identical;
}

bool
Nodes::verify_hna_entry(const IPv4Net& dest, const IPv4& origin)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    bool is_found = false;
    try {
	OlsrTypes::ExternalID erid = er.get_hna_route_in_id(dest, origin);
	is_found = true;
	UNUSED(erid);
    } catch (...) {}

    return is_found;
}

bool
Nodes::verify_hna_distance(const IPv4Net& dest, const IPv4& origin,
			   const uint16_t distance)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    bool is_as_expected = false;
    try {
	const ExternalRoute* e = er.get_hna_route_in(dest, origin);
	if (e->distance() == distance)
	    is_as_expected = true;
    } catch (...) {}

    return is_as_expected;
}

bool
Nodes::verify_hna_entry_count(const size_t expected_count)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    size_t actual_hna_entry_count = er.hna_entry_count();

    if (expected_count == actual_hna_entry_count)
	return true;

    debug_msg("expected %u, got %u\n",
	XORP_UINT_CAST(expected_count),
	XORP_UINT_CAST(actual_hna_entry_count));

    return false;
}

bool
Nodes::verify_hna_origin_count(const size_t expected_count)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    size_t actual_hna_origin_count = er.hna_origin_count();

    if (expected_count == actual_hna_origin_count)
	return true;

    debug_msg("expected %u, got %u\n",
	XORP_UINT_CAST(expected_count),
	XORP_UINT_CAST(actual_hna_origin_count));

    return false;
}

bool
Nodes::verify_hna_dest_count(const size_t expected_count)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    size_t actual_hna_dest_count = er.hna_dest_count();

    if (expected_count == actual_hna_dest_count)
	return true;

    debug_msg("expected %u, got %u\n",
	XORP_UINT_CAST(expected_count),
	XORP_UINT_CAST(actual_hna_dest_count));

    return false;
}

bool
Nodes::originate_hna(const IPv4Net& dest)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    bool is_originated = false;
    try {
	is_originated = er.originate_hna_route_out(dest);
    } catch (...) {}

    return is_originated;
}

bool
Nodes::withdraw_hna(const IPv4Net& dest)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    ExternalRoutes& er = olsr->external_routes();

    bool is_withdrawn = false;
    try {
	er.withdraw_hna_route_out(dest);
	is_withdrawn = true;
    } catch (...) {}

    return is_withdrawn;
}

bool
Nodes::verify_tc_origin_count(const size_t expected_count)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    TopologyManager& tm = olsr->topology_manager();

    size_t actual_tc_node_count = tm.tc_node_count();

    if (expected_count == actual_tc_node_count)
	return true;

    debug_msg("expected %u, got %u\n",
	XORP_UINT_CAST(expected_count),
	XORP_UINT_CAST(actual_tc_node_count));

    return false;
}

// verify advertised neighbor set from given origin
// has ANSN of 'expected_ansn' and matches contents of addrs.
bool
Nodes::verify_tc_ans(const IPv4& origin,
		     const uint16_t expected_ansn,
		     const vector<IPv4>& expected_addrs)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    TopologyManager& tm = olsr->topology_manager();

    uint16_t actual_ansn;
    vector<IPv4> actual_addrs = tm.get_tc_neighbor_set(origin, actual_ansn);

    if (expected_ansn == actual_ansn) {
	if (expected_addrs == actual_addrs) {
	    return true;
	} else {
	    debug_msg("address list mismatch\n");
	}
    } else {
	debug_msg("ansn mismatch: expected %u, got %u\n",
		  XORP_UINT_CAST(expected_ansn),
		  XORP_UINT_CAST(actual_ansn));
    }

    return false;
}

bool
Nodes::verify_tc_origin_seen(const IPv4& origin, const bool expected_seen)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    TopologyManager& tm = olsr->topology_manager();

    // Try and look up the origin's ANS. If we can't find the
    // origin at all, then we know we either never saw it,
    // or it has completely timed out and ANSN was valid.
    bool actual_seen = false;
    try {
	uint16_t ansn;
	vector<IPv4> addrs = tm.get_tc_neighbor_set(origin, ansn);
	debug_msg("ansn %u with %sempty ans.\n",
		  ansn,
		  addrs.empty() ? "" : "non");
	actual_seen = true;
	UNUSED(ansn);
	UNUSED(addrs);
    } catch (BadTopologyEntry bte) {
	debug_msg("BadTopologyEntry caught\n");
    }

    if (expected_seen == actual_seen)
	return true;

    debug_msg("expected %s, actual %s\n",
	      bool_c_str(expected_seen),
	      bool_c_str(actual_seen));

    return false;
}

bool
Nodes::verify_tc_distance(const IPv4& origin,
			  const IPv4& neighbor_addr,
			  const uint16_t expected_distance)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    TopologyManager& tm = olsr->topology_manager();

    try {
	uint16_t actual_distance = tm.get_tc_distance(origin, neighbor_addr);

	if (expected_distance == actual_distance)
	    return true;

        debug_msg("expected %u, actual %u\n",
		  XORP_UINT_CAST(expected_distance),
		  XORP_UINT_CAST(actual_distance));
    } catch (...) {}

    return false;
}

bool
Nodes::verify_tc_destination(const IPv4& dest_addr,
			     const size_t expected_count)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    Olsr* olsr = nat.olsr();
    TopologyManager& tm = olsr->topology_manager();

    try {
	size_t actual_count = tm.get_tc_lasthop_count_by_dest(dest_addr);

	if (expected_count == actual_count)
	    return true;
    } catch (...) {}

    return false;
}

bool
Nodes::verify_routing_table_size(const size_t rtsize)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    DebugIO* io = nat.io();

    size_t actual_rtsize = io->routing_table_size();
    if (rtsize == actual_rtsize)
	return true;

    debug_msg("expected %u, got %u\n",
	XORP_UINT_CAST(rtsize),
	XORP_UINT_CAST(actual_rtsize));

    return false;
}

bool
Nodes::verify_routing_entry(IPv4Net net,
			    IPv4 nexthop,
			    uint32_t metric)
{
    XLOG_ASSERT(node_is_selected());

    NodeTuple& nat = _selected_node;
    DebugIO* io = nat.io();

    bool result = io->routing_table_verify(net, nexthop, metric);

    return result;
}

NodeTuple&
Nodes::get_node_tuple_by_addr(const IPv4& addr)
    throw(NoSuchAddress)
{
    map<IPv4, NodeTuple>::iterator ii = _nodes.find(addr);
    if (ii == _nodes.end()) {
	xorp_throw(NoSuchAddress,
		   c_format("address %s not found in simulation",
			    cstring(addr)));
    }

    NodeTuple& nat = (*ii).second;
    return nat;
}

bool
Links::add_link(const IPv4& left_addr, const IPv4& right_addr)
    throw(NoSuchAddress)
{

    // Check if mapping already exists for address pair.
    multimap<pair<IPv4, IPv4>, LinkTuple>::iterator ii =
	_links.find(make_pair(left_addr, right_addr));
    if (ii != _links.end()) {
	xorp_throw(NoSuchAddress,
		   c_format("Link between %s and %s already exists",
			    cstring(left_addr),
			    cstring(right_addr)));
	return false;
    }

    NodeTuple& left = _parent->nodes().get_node_tuple_by_addr(left_addr);
    NodeTuple& right = _parent->nodes().get_node_tuple_by_addr(right_addr);

    EmulateSubnet* es = new EmulateSubnet(_parent->info(), _eventloop);

    // TODO: Make the address/instance representation less wasteful...
    // and promote LinkTuple to a class like it should be.
    es->bind_interface(left.instancename(), left.ifname(), left.vifname(),
		       left_addr, MY_OLSR_PORT, *left.io());
    es->bind_interface(right.instancename(), right.ifname(), right.vifname(),
		       right_addr, MY_OLSR_PORT, *right.io());

    debug_msg("add_link %p %s %s\n",
	      es,
	      cstring(left_addr),
	      cstring(right_addr));

    _links.insert(make_pair(make_pair(left_addr, right_addr),
		  LinkTuple(es, left, right)));

    return true;
}

void
Links::purge_link_tuple(LinkTuple& lt,
			const IPv4& left_addr,
			const IPv4& right_addr)
{
    EmulateSubnet* es = lt._es;

    NodeTuple& left = lt._left;
    es->unbind_interface(left.instancename(), left.ifname(), left.vifname(),
		         left_addr, MY_OLSR_PORT, *left.io());

    NodeTuple& right = lt._right;
    es->unbind_interface(right.instancename(), right.ifname(), right.vifname(),
		         right_addr, MY_OLSR_PORT, *right.io());

    delete es;
}

size_t
Links::remove_all_links()
{
    size_t nlinks = 0;

    multimap<pair<IPv4, IPv4>, LinkTuple>::iterator ii;
    while (! _links.empty()) {
	ii =_links.begin();
	// A tuple of tuples ((l,r),lt)
	IPv4 left_addr = (*ii).first.first;
	IPv4 right_addr = (*ii).first.second;
	purge_link_tuple((*ii).second, left_addr, right_addr);
	_links.erase(ii);
	nlinks++;
    }

    return nlinks;
}

bool
Links::remove_link(const IPv4& left_addr, const IPv4& right_addr)
    throw(NoSuchAddress)
{
    // Check if mapping exists for address pair.
    multimap<pair<IPv4, IPv4>, LinkTuple>::iterator ii =
	_links.find(make_pair(left_addr, right_addr));
    if (ii == _links.end()) {
	return false;	    // nothing to delete.
    }

    purge_link_tuple((*ii).second, left_addr, right_addr);
    _links.erase(ii);

    return true;
}

// remove all links which contain link_addr.
bool
Links::remove_all_links_for_addr(const IPv4& link_addr)
    throw(NoSuchAddress)
{
    size_t found_count = 0;

    multimap<pair<IPv4, IPv4>, LinkTuple>::iterator ii;
    for (ii = _links.begin(); ii != _links.end(); ii++) {
	// If link appears in either link tuple for all links
	// in system, zap it. get the arguments the right way round.
	if (link_addr == (*ii).first.first) {
	    ++found_count;
	    purge_link_tuple((*ii).second, link_addr, (*ii).first.second);
	    _links.erase(ii);
	} else if (link_addr == (*ii).first.second) {
	    ++found_count;
	    purge_link_tuple((*ii).second, (*ii).first.first, link_addr);
	    _links.erase(ii);
	}
    }

    if (found_count == 0) {
	xorp_throw(NoSuchAddress,
		   c_format("address %s not found in simulated links" ,
			    cstring(link_addr)));
    }

    return (found_count != 0);
}

bool
Simulator::cmd(Args& args)
    throw(InvalidString)
{
    string word;

    while (args.get_next(word)) {
	if (_info.verbose()) {
	    cout << "[" << word << "]" << endl;
	    cout << args.original_line() << endl;
	}
	if ("#" == word.substr(0,1)) {
	    // CMD: #
	    return true;

	} else if ("echo" == word) {
	    // CMD: echo <text>
	    if (! _info.verbose()) {
		cout << args.original_line() << endl;
	    }
	    return true;

	} else if ("exit" == word) {
	    // CMD: exit
	    return false;

	} else if ("dump_routing_table" == word) {
	    // CMD: dump_routing_table <main-addr>
	    IPv4 main_addr = get_next_ipv4(args, "dump_routing_table");

	    if (! nodes().dump_routing_table(main_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to dump table for node <%s> [%s]",
				    cstring(main_addr),
				    args.original_line().c_str()));
	    }

	} else if ("add_link" == word) {
	    // CMD: add_link <link-addr-l> <link-addr-r>

	    IPv4 left_addr = get_next_ipv4(args, "add_link");
	    IPv4 right_addr = get_next_ipv4(args, "add_link");

	    if (! links().add_link(left_addr, right_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to create link %s %s [%s]",
				    cstring(left_addr),
				    cstring(right_addr),
				    args.original_line().c_str()));
	    }

	} else if ("remove_all_links" == word) {
	    // CMD: remove_all_links

	    size_t nlinks = links().remove_all_links();
	    debug_msg("Removed %u links.\n", XORP_UINT_CAST(nlinks));

	} else if ("remove_link" == word) {
	    // CMD: remove_link <link-addr-l> <link-addr-r>

	    IPv4 left_addr = get_next_ipv4(args, "remove_link");
	    IPv4 right_addr = get_next_ipv4(args, "remove_link");

	    if (! links().remove_link(left_addr, right_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to remove link %s %s [%s]",
				    cstring(left_addr),
				    cstring(right_addr),
				    args.original_line().c_str()));
	    }

	} else if ("create" == word) {
	    // CMD: create <main-address> [link-address-1] ..
	    // all interfaces are bound and configured up by default.

	    vector<IPv4> addrs;

	    IPv4 main_addr = get_next_ipv4(args, "create");
	    addrs.push_back(main_addr);

	    do {
		IPv4 addr;
		try {
		    addrs.push_back(get_next_ipv4(args, "create"));
		} catch (InvalidString is) {
		    break;
		}
	    } while (true);

	    if (! nodes().create_node(addrs)) {
		xorp_throw(InvalidString,
			   c_format("Failed to create node <%s> [%s]",
				    cstring(addrs[0]),
				    args.original_line().c_str()));
	    }

	} else if ("destroy" == word) {
	    // CMD: destroy <main-address>

	    IPv4 main_addr = get_next_ipv4(args, "destroy");

	    if (! nodes().destroy_node(main_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to destroy node <%s> [%s]",
				    cstring(main_addr),
				    args.original_line().c_str()));
	    }

	} else if ("face_up" == word) {
	    // CMD: face_up <link-address>

	    IPv4 link_addr = get_next_ipv4(args, "face_up");

	    if (! nodes().configure_address(link_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to configure address <%s> [%s]",
				    cstring(link_addr),
				    args.original_line().c_str()));
	    }

	} else if ("face_down" == word) {
	    // CMD: face_down <link-address>

	    IPv4 link_addr = get_next_ipv4(args, "face_down");

	    if (! nodes().unconfigure_address(link_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to unconfigure address <%s> [%s]",
				    cstring(link_addr),
				    args.original_line().c_str()));
	    }

	} else if ("set_default_hello_interval" == word) {
	    // CMD: set_default_hello_interval <value>

	    int value = get_next_number(args, "set_default_hello_interval");
	    nodes().set_default_hello_interval(value);

	} else if ("set_default_mid_interval" == word) {
	    // CMD: set_default_mid_interval <value>

	    int value = get_next_number(args, "set_default_mid_interval");
	    nodes().set_default_mid_interval(value);

	} else if ("set_default_mpr_coverage" == word) {
	    // CMD: set_default_mpr_coverage <value>

	    int value = get_next_number(args, "set_default_mpr_coverage");
	    nodes().set_default_mpr_coverage(value);

	} else if ("set_default_refresh_interval" == word) {
	    // CMD: set_default_refresh_interval <value>

	    int value = get_next_number(args, "set_default_refresh_interval");
	    nodes().set_default_refresh_interval(value);

	} else if ("set_default_tc_interval" == word) {
	    // CMD: set_default_tc_interval <value>

	    int value = get_next_number(args, "set_default_tc_interval");
	    nodes().set_default_tc_interval(value);

	} else if ("set_default_willingness" == word) {
	    // CMD: set_default_willingness <value>

	    int value = get_next_number(args, "set_default_willingness");
	    nodes().set_default_willingness(value);

	} else if ("set_default_tc_redundancy" == word) {
	    // CMD: set_default_tc_redundancy <value>

	    int value = get_next_number(args, "set_default_tc_redundancy");
	    nodes().set_default_tc_redundancy(value);

	} else if ("set_hello_interval" == word) {
	    // CMD: set_hello_interval <main-address> <value>

	    IPv4 main_addr = get_next_ipv4(args, "set_hello_interval");
	    int value = get_next_number(args, "set_hello_interval");

	    if (! nodes().set_hello_interval(main_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to set hello interval [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("set_mpr_coverage" == word) {
	    // CMD: set_mpr_coverage <main-address> <value>

	    IPv4 main_addr = get_next_ipv4(args, "set_mpr_coverage");
	    int value = get_next_number(args, "set_mpr_coverage");

	    if (! nodes().set_mpr_coverage(main_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to set coverage <%s> <%d> [%s]",
				    cstring(main_addr),
				    value,
				    args.original_line().c_str()));
	    }

	} else if ("set_mid_interval" == word) {
	    // CMD: set_mid_interval <main-address> <value>

	    IPv4 main_addr = get_next_ipv4(args, "set_mid_interval");
	    int value = get_next_number(args, "set_mid_interval");

	    if (! nodes().set_mid_interval(main_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to set mid interval [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("set_refresh_interval" == word) {
	    // CMD: set_refresh_interval <main-address> <value>

	    IPv4 main_addr = get_next_ipv4(args, "set_refresh_interval");
	    int value = get_next_number(args, "set_refresh_interval");

	    if (! nodes().set_refresh_interval(main_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to set refresh interval [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("set_tc_interval" == word) {
	    // CMD: set_tc_interval <main-address> <value>

	    IPv4 main_addr = get_next_ipv4(args, "set_tc_interval");
	    int value = get_next_number(args, "set_tc_interval");

	    if (! nodes().set_tc_interval(main_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to set TC interval [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("set_willingness" == word) {
	    // CMD: set_willingness <main-address> <value>

	    IPv4 main_addr = get_next_ipv4(args, "set_willingness");
	    int value = get_next_number(args, "set_willingness");

	    if (! nodes().set_willingness(main_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to set willingness <%s> <%d> [%s]",
				    cstring(main_addr),
				    value,
				    args.original_line().c_str()));
	    }

	} else if ("select" == word) {
	    // CMD: select <main-address>

	    IPv4 main_addr = get_next_ipv4(args, "select");

	    if (! nodes().select_node(main_addr)) {
		xorp_throw(InvalidString,
			   c_format("Failed to select node <%s> [%s]",
				    cstring(main_addr),
				    args.original_line().c_str()));
	    }

	} else if ("verify_all_link_state_empty" == word) {
	    // CMD: verify_all_link_state_empty

	    if (! nodes().verify_all_link_state_empty()) {
		xorp_throw(InvalidString,
			   "Failed to verify that link-state "
			   "databases are empty at every node.");
	    }

	} else if ("verify_n1" == word) {
	    // CMD: verify_n1 <n1_addr> <status>

	    IPv4 n1_addr = get_next_ipv4(args, "verify_n1");
	    bool value = get_next_bool(args, "verify_n1");

	    if (! nodes().verify_n1(n1_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify neighbor <%s> [%s]",
				    cstring(n1_addr),
				    args.original_line().c_str()));
	    }

	} else if ("verify_n2" == word) {
	    // CMD: verify_n2 <n2_addr> <status>

	    IPv4 n2_addr = get_next_ipv4(args, "verify_n2");
	    bool value = get_next_bool(args, "verify_n1");

	    if (! nodes().verify_n2(n2_addr, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify two-hop <%s> [%s]",
				    cstring(n2_addr),
				    args.original_line().c_str()));
	    }

	} else if ("verify_is_mpr" == word) {
	    // CMD: verify_is_mpr <status>

	    bool expected_mpr_state = get_next_bool(args, "verify_is_mpr");

	    if (! nodes().verify_is_mpr(expected_mpr_state)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify MPR status [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_mpr_set" == word) {
	    // CMD: verify_mpr_set empty
	    //    | verify_mpr_set <addr> ... <n_addr>

	    bool has_addrs = false;	// true if second form of command.

	    vector<IPv4> mpr_addrs;
	    do {
		try {
		    IPv4 addr = get_next_ipv4(args, "verify_mpr_set");
		    mpr_addrs.push_back(addr);
		    has_addrs = true;
		} catch (InvalidString is) {
		    // If we already saw addresses, OK to stop parsing.
		    if (has_addrs)
			break;
		    // Re-throw exception if no string present, or if
		    // first word is not "empty".
		    args.push_back();
		    string emptyword = get_next_word(args, "verify_mpr_set");
		    if ("empty" == emptyword)
			break;
		    else
			throw;
		}
	    } while (true);

	    // has_addrs is false if and only if "empty" was specified.
	    if (has_addrs && mpr_addrs.empty()) {
		    xorp_throw(InvalidString,
			       c_format("No valid addresses [%s]",
					args.original_line().c_str()));
	    }

	    if (! nodes().verify_mpr_set(mpr_addrs)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify MPR set [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_mpr_selector_set" == word) {
	    // CMD: verify_mpr_selector_set empty
	    //    | verify_mpr_selector_set <addr> ... <n_addr>

	    bool has_addrs = false;	// true if second form of command.

	    vector<IPv4> mprs_addrs;
	    do {
		try {
		    IPv4 addr = get_next_ipv4(args, "verify_mpr_selector_set");
		    mprs_addrs.push_back(addr);
		    has_addrs = true;
		} catch (InvalidString is) {
		    // If we already saw addresses, OK to stop parsing.
		    if (has_addrs)
			break;
		    // Re-throw exception if no string present, or if
		    // first word is not "empty".
		    args.push_back();
		    string emptyword = get_next_word(args,
						     "verify_mpr_selector_set");
		    if ("empty" == emptyword)
			break;
		    else
			throw;
		}
	    } while (true);

	    // has_addrs is false if and only if "empty" was specified.
	    if (has_addrs && mprs_addrs.empty()) {
		    xorp_throw(InvalidString,
			       c_format("No valid addresses [%s]",
					args.original_line().c_str()));
	    }

	    if (! nodes().verify_mpr_selector_set(mprs_addrs)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify MPR selector set [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_routing_table_size" == word) {
	    // CMD: verify_routing_table_size <count>

	    int expected = get_next_number(args, "verify_routing_table_size");

	    if (! nodes().verify_routing_table_size(expected)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify table size <%d> [%s]",
				    expected,
				    args.original_line().c_str()));
	    }

	} else if ("verify_routing_entry" == word) {
	    // CMD: verify_routing_entry <dest> <nexthop> <metric>

	    IPv4Net dest = get_next_ipv4_net(args, "verify_routing_entry");
	    IPv4 nexthop = get_next_ipv4(args, "verify_routing_entry");
	    int metric = get_next_number(args, "verify_routing_entry");

	    if (! nodes().verify_routing_entry(dest, nexthop, metric)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify table entry [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_mid_node_count" == word) {
	    // CMD: verify_mid_node_count <count>

	    int expected = get_next_number(args, "verify_mid_node_count");

	    if (! nodes().verify_mid_node_count(expected)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify MID count <%d> [%s]",
				    expected,
				    args.original_line().c_str()));
	    }

	} else if ("verify_mid_node_addrs" == word) {
	    // CMD: verify_mid_node_addrs <origin> empty
	    //    | verify_mid_node_addrs <origin> <addr1> ... <addrN>

	    IPv4 mid_origin = get_next_ipv4(args, "verify_mid_node_addrs");

	    bool has_addrs = false;	// true if second form of command.
	    vector<IPv4> mid_addrs;
	    do {
		try {
		    IPv4 addr = get_next_ipv4(args, "verify_mid_node_addrs");
		    mid_addrs.push_back(addr);
		    has_addrs = true;
		} catch (InvalidString is) {
		    // If we already saw addresses, OK to stop parsing.
		    if (has_addrs)
			break;
		    // Re-throw exception if no string present, or if
		    // first word is not "empty".
		    args.push_back();
		    string emptyword = get_next_word(args,
						     "verify_mid_node_addrs");
		    if ("empty" == emptyword)
			break;
		    else
			throw;
		}
	    } while (true);

	    // has_addrs is false if and only if "empty" was specified.
	    if (has_addrs && mid_addrs.empty()) {
		    xorp_throw(InvalidString,
			       c_format("No valid addresses [%s]",
					args.original_line().c_str()));
	    }

	    if (! nodes().verify_mid_node_addrs(mid_origin, mid_addrs)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify MID node [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_mid_distance" == word) {
	    // CMD: verify_mid_distance <origin> <addr> <distance>

	    IPv4 origin = get_next_ipv4(args, "verify_mid_distance");
	    IPv4 iface_addr = get_next_ipv4(args, "verify_mid_distance");
	    int distance = get_next_number(args, "verify_mid_distance");

	    if (! nodes().verify_mid_address_distance(origin, iface_addr,
						      distance)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify MID distance [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_tc_origin_count" == word) {
	    // CMD: verify_tc_origin_count <num>

	    int count = get_next_number(args, "verify_tc_origin_count");

	    if (! nodes().verify_tc_origin_count(count)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify TC origin count [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_tc_ans" == word) {
	    // CMD: verify_tc_ans <origin> <ansn> empty
	    //    | verify_tc_ans <origin> <ansn> <addr1> ... <addrN>

	    IPv4 origin = get_next_ipv4(args, "verify_tc_ans");
	    uint16_t ansn = get_next_number(args, "verify_tc_ans");

	    vector<IPv4> addrs;
	    bool has_addrs = false;	// true if second form of command.
	    do {
		try {
		    IPv4 addr = get_next_ipv4(args, "verify_tc_ans");
		    addrs.push_back(addr);
		    has_addrs = true;
		} catch (InvalidString is) {
		    // If we already saw addresses, OK to stop parsing.
		    if (has_addrs)
			break;
		    // Re-throw exception if no string present, or if
		    // first word is not "empty".
		    args.push_back();
		    string emptyword = get_next_word(args,
						     "verify_tc_ans");
		    if ("empty" == emptyword)
			break;
		    else
			throw;
		}
	    } while (true);

	    if (! nodes().verify_tc_ans(origin, ansn, addrs)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify TC advertised set [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_tc_origin_seen" == word) {
	    // CMD: verify_tc_origin_seen <origin> <bool>

	    IPv4 origin = get_next_ipv4(args, "verify_tc_origin_seen");
	    bool value = get_next_bool(args, "verify_tc_origin_seen");

	    if (! nodes().verify_tc_origin_seen(origin, value)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify TC origin seen [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_tc_distance" == word) {
	    // CMD: verify_tc_distance <origin> <addr> <distance>

	    IPv4 origin = get_next_ipv4(args, "verify_tc_distance");
	    IPv4 addr = get_next_ipv4(args, "verify_tc_distance");
	    uint16_t distance = get_next_number(args, "verify_tc_distance");

	    if (! nodes().verify_tc_distance(origin, addr, distance)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify TC distance [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_tc_destination" == word) {
	    // CMD: verify_tc_destination <addr> <count>

	    IPv4 addr = get_next_ipv4(args, "verify_tc_destination");
	    size_t count = get_next_number(args, "verify_tc_destination");

	    if (! nodes().verify_tc_destination(addr, count)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify TC destination [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_hna_entry" == word) {
	    // CMD: verify_hna_entry <dest> <origin>

	    IPv4Net dest = get_next_ipv4_net(args, "verify_hna_entry");
	    IPv4 origin = get_next_ipv4(args, "verify_hna_entry");

	    if (! nodes().verify_hna_entry(dest, origin)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify HNA entry [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_hna_distance" == word) {
	    // CMD: verify_hna_distance <dest> <origin> <distance>

	    IPv4Net dest = get_next_ipv4_net(args, "verify_hna_distance");
	    IPv4 origin = get_next_ipv4(args, "verify_hna_distance");
	    uint16_t distance = get_next_number(args, "verify_hna_distance");

	    if (! nodes().verify_hna_distance(dest, origin, distance)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify HNA distance [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_hna_entry_count" == word) {
	    // CMD: verify_hna_entry_count <num>

	    int count = get_next_number(args, "verify_hna_entry_count");

	    if (! nodes().verify_hna_entry_count(count)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify HNA entry count [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_hna_origin_count" == word) {
	    // CMD: verify_hna_origin_count <num>

	    int count = get_next_number(args, "verify_hna_origin_count");

	    if (! nodes().verify_hna_origin_count(count)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify HNA origin count [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("verify_hna_dest_count" == word) {
	    // CMD: verify_hna_dest_count <num>

	    int count = get_next_number(args, "verify_hna_dest_count");

	    if (! nodes().verify_hna_dest_count(count)) {
		xorp_throw(InvalidString,
			   c_format("Failed to verify HNA prefix count [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("originate_hna" == word) {
	    // CMD: originate_hna <dest>

	    IPv4Net dest = get_next_ipv4_net(args, "originate_hna");

	    if (! nodes().originate_hna(dest)) {
		xorp_throw(InvalidString,
			   c_format("Failed to originate HNA [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("withdraw_hna" == word) {
	    // CMD: withdraw_hna <dest>

	    IPv4Net dest = get_next_ipv4_net(args, "withdraw_hna");

	    if (! nodes().withdraw_hna(dest)) {
		xorp_throw(InvalidString,
			   c_format("Failed to withdraw HNA [%s]",
				    args.original_line().c_str()));
	    }

	} else if ("wait" == word) {
	    // CMD: wait <secs>

	    int secs = get_next_number(args, "wait");
	    wait(secs);

	} else {
	    xorp_throw(InvalidString, c_format("Unknown command <%s>. [%s]",
					   word.c_str(),
					   args.original_line().c_str()))
	}
    }

    return true;
}

int
go(bool verbose, int verbose_level)
{
    Simulator simulator(verbose, verbose_level);

    try {
	for(;;) {
	    string line;
	    if (!getline(cin, line))
		return 0;
	    Args args(line);
	    if (!simulator.cmd(args))
		return -1;
	}
    } catch(...) {
	xorp_print_standard_exceptions();
	return -1;
    }
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);
    t.complete_args_parsing();

    if (0 != t.exit())
	return t.exit();

    int retval = go(t.get_verbose(), t.get_verbose_level());

    xlog_stop();
    xlog_exit();

    return retval;
}
