// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libproto/test_spt.cc,v 1.16 2007/11/13 09:59:04 atanu Exp $"

#include "libproto_module.h"
#include "libxorp/xorp.h"

#include <fstream>

#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/tokenize.hh"

#include "spt.hh"

#ifndef	DEBUG_LOGGING
#define DEBUG_LOGGING
#endif /* DEBUG_LOGGING */
#ifndef	DEBUG_PRINT_FUNCTION_NAME
#define DEBUG_PRINT_FUNCTION_NAME
#endif /* DEBUG_PRINT_FUNCTION_NAME */

template <>
string
Node<string>::str() const
{
    return _nodename;
}
template <>
string
RouteCmd<string>::str() const
{
    return c() + " node: " + _node + " nexthop: " + _nexthop +
	" prevhop: " + _prevhop +
	" weight: " + c_format("%d", _weight) + 
	" next hop changed: " + bool_c_str(_next_hop_changed) +
	" weight changed: " + bool_c_str(_weight_changed);

}

/**
 * Utility routine to be used by tests to read in graphs from files.
 */
bool
populate_graph(TestInfo& info, Spt<string> *spt, string& fname)
{
    // Lines starting with '#' are considered to be comments.
    // Blank lines are ignored.
    // A standard line is in the form of node weight node.
    // The first node that is encountered is considered to be the
    // origin.

    std::ifstream graph(fname.c_str());
    if (!graph) {
	XLOG_WARNING("Couldn't open %s", fname.c_str());
	return false;
    }

    string line;
    string origin;

    while (graph) {
	getline(graph, line);
	DOUT(info) << '<' << line << '>' << endl;
	// Split this line into words
	vector<string> words;
	tokenize(line, words);
	// If this is an empty line ignore it.
	if (words.empty())
	    continue;
	// Is this a comment
	if (line[0] == '#')
	    continue;
	if (words.size() != 3) {
	    XLOG_WARNING("Bad input %s", line.c_str());
	    continue;
	}
	if (origin.empty())
	    origin = words[0];

	if (!spt->exists_node(words[0]))
	    spt->add_node(words[0]);
	if (!spt->exists_node(words[2]))
	    spt->add_node(words[2]);
	spt->add_edge(words[0], atoi(words[1].c_str()), words[2]);
    }

    spt->set_origin(origin);

    graph.close();

    return true;
}

/**
 * Print the route command.
 */
template <typename A>
class Pr: public unary_function<RouteCmd<A >, void> {
 public:
    Pr(TestInfo& info) : _info(info) {}
    void operator()(const RouteCmd<A>& rcmd) {
	DOUT(_info) << rcmd.str() << endl;
    }
 private:
    TestInfo& _info;
};

/**
 * Verify that the route computation gave the correct result by
 * comparing the expected and received routes.
 */
bool
verify(TestInfo& info,
       list<RouteCmd<string> > received,
       list<RouteCmd<string> > expected)
{
    // remove all the matching routes.
    list<RouteCmd<string> >::iterator ri, ei;
    for(ei = expected.begin(); ei != expected.end();) {
	bool found = false;
	for(ri = received.begin(); ri != received.end(); ri++) {
	    if ((*ei) == (*ri)) {
		found = true;
		received.erase(ri);
		expected.erase(ei++);
		break;
	    }
	}
	if (!found) {
	    DOUT(info) << ei->str() << " Not received" << endl;
	    return false;
	}
    }

    // There should be no expected routes left.
    if (!expected.empty()) {
	DOUT(info) << "Routes not received:\n";
	    for_each(expected.begin(), expected.end(), Pr<string>(info));
	    return false;
    }

    // There should be no extra routes.
    if (!received.empty()) {
	DOUT(info) << "Additional Routes received:\n";
	for_each(received.begin(), received.end(), Pr<string>(info));
	return false;
    }

    return true;
}

/**
 * Test adding and changing edge weights.
 */
bool
test1(TestInfo& info)
{
    Node<string> a("A", info.verbose());

    DOUT(info) << "Nodename: " << a.nodename() << endl;

    Node<string> *b = new Node<string>("B", info.verbose());
    Node<string>::NodeRef bref(b);

    if (!a.add_edge(bref, 10)) {
	DOUT(info) << "Adding an edge failed!!!\n";
	return false;
    }

    // Add the same edge again to force an error.
    if (a.add_edge(bref, 10)) {
	DOUT(info) << "Adding the same edge twice succeeded!!!\n";
	return false;
    }
    
    int weight;
    // Check the edge weight.
    if (!a.get_edge_weight(bref, weight)) {
	DOUT(info) << "Failed to get edge!!!\n";
	return false;
    }

    if (weight != 10) {
	DOUT(info) << "Expected weight of 10 got " << weight << endl;
	return false;
    }

    // Change the weight to 20 and check
    if (!a.update_edge_weight(bref, 20)) {
	DOUT(info) << "Failed to update edge!!!\n";
	return false;
    }
    
    if (!a.get_edge_weight(bref, weight)) {
	DOUT(info) << "Failed to get edge!!!\n";
	return false;
    }
    if (weight != 20) {
	DOUT(info) << "Expected weight of 20 got " << weight << endl;
	return false;
    }

    return true;
}

bool
test2(TestInfo& info)
{
    Spt<string> *spt = new Spt<string>(info.verbose() /* enable tracing */);

    spt->add_node("A");
    spt->add_node("B");
    spt->add_edge("A", 10, "B");
    if (spt->add_edge("A", 10, "B")) {
	DOUT(info) << "Adding the same edge twice succeeded!!!\n";
	return false;
    }
    if (!spt->update_node("A")) {
	DOUT(info) << "Unable to update node A\n";
	return false;
    }

    spt->set_origin("A");

    int weight;
    if (!spt->get_edge_weight("A", weight, "B")) {
	DOUT(info) << "Failed to get weight from A to B\n";
	return false;
    }
    DOUT(info) << "Edge weight from A to B is " << weight << endl;
    if (weight != 10) {
	DOUT(info) << "Edge weight from A to B should be 10 not " << weight
		   << endl;
	return false;
    }

    spt->update_edge_weight("A", 5, "B");
    if (!spt->get_edge_weight("A", weight, "B")) {
	DOUT(info) << "Failed to get weight from A to B\n";
	return false;
    }
    DOUT(info) << "Edge weight from A to B is " << weight << endl;
    if (weight != 5) {
	DOUT(info) << "Edge weight from A to B should be 5 not " << weight
		   << endl;
	return false;
    }

    if (spt->get_edge_weight("B", weight, "A")) {
	DOUT(info) <<
"Got edge weight from B to A should not be possible on unidirectional link\n";
	return false;
    }

    delete spt;

    return true;
}

bool
test3(TestInfo& info)
{
    Spt<string> *spt = new Spt<string>(info.verbose() /* enable tracing */);

    spt->add_node("A");
    for(int i = 0; i < 1000000; i++)
	spt->set_origin("A");

    for(int i = 0; i < 1000000; i++) {
	spt->add_node("B");
	spt->remove_node("B");
    }

    delete spt;

    return true;
}

bool
test4(TestInfo& info)
{
    Spt<string> spt(info.verbose() /* enable tracing */);

    list<RouteCmd<string> > routes;

    // This should fail as the origin has not yet been configured.
    if (spt.compute(routes))
	return false;

    return true;
}

bool
test5(TestInfo& info, string fname)
{
    Spt<string> spt(info.verbose() /* enable tracing */);

    if (!populate_graph(info, &spt, fname))
	return false;

    DOUT(info) << spt.str();

    list<RouteCmd<string> > routes;

    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    return true;
}

/**
 * Manipulate the database and verify the route commands.
 */
bool
test6(TestInfo& info)
{
    Spt<string> spt(info.verbose() /* enable tracing */);

    spt.add_node("s");
    spt.add_node("t");
    spt.add_node("x");
    spt.add_node("y");
    spt.add_node("z");

    spt.set_origin("s");

    spt.add_edge("s", 10, "t");
    spt.add_edge("s", 5, "y");

    spt.add_edge("t", 1, "x");
    spt.add_edge("t", 2, "y");

    spt.add_edge("y", 3, "t");
    spt.add_edge("y", 9, "x");
    spt.add_edge("y", 2, "z");

    spt.add_edge("x", 4, "z");

    spt.add_edge("z", 6, "x");

    DOUT(info) << spt.str();

    list<RouteCmd<string> > routes, expected_routes;

    /* =========================================== */
    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "t", "y", "y", 8));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "x", "y", "t", 9));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "y", "y", "s", 5));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "z", "y", "y", 7));

    if (!verify(info, routes, expected_routes))
	return false;
    /* =========================================== */
    DOUT(info) << "remove edge s y" << endl;
    spt.remove_edge("s", "y");

    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "t", "t", "s", 10, true, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "x", "t", "t", 11, true, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "y", "t", "t", 12, true, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "z", "t", "y", 14, true, true));

    if (!verify(info, routes, expected_routes))
	return false;
    /* =========================================== */
    DOUT(info) << "add edge s 5 y" << endl;
    DOUT(info) << "update edge s 1 t" << endl;
    spt.add_edge("s", 5, "y");
    spt.update_edge_weight("s", 1, "t");

    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "t", "t", "s", 1, false, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "x", "t", "t", 2, false, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "y", "t", "t", 3, false, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "z", "t", "y", 5, false, true));

    if (!verify(info, routes, expected_routes))
	return false;
    /* =========================================== */
    DOUT(info) << "update edge s 10 t" << endl;
    DOUT(info) << "remove node z" << endl;
    spt.update_edge_weight("s", 10, "t");
    spt.remove_node("z");

    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "t", "y", "y", 8, true, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "x", "y", "t", 9, true, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::REPLACE,
				   "y", "y", "s", 5, true, true));
    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::DELETE,
				   "z", "z", "z"));

    if (!verify(info, routes, expected_routes))
	return false;
    /* =========================================== */
    DOUT(info) << "add node z" << endl;
    spt.add_node("z");

    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    if (!verify(info, routes, expected_routes))
	return false;
    /* =========================================== */
    DOUT(info) << "No change" << endl;

    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    if (!verify(info, routes, expected_routes))
	return false;
    /* =========================================== */
    DOUT(info) << "add edge x 4 z" << endl;
    DOUT(info) << "add edge y 2 z" << endl;
    spt.add_edge("x", 4, "z");
    spt.add_edge("y", 2, "z");

    routes.clear();
    expected_routes.clear();
    if (!spt.compute(routes))
	return false;

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "z", "y", "y", 7));

    if (!verify(info, routes, expected_routes))
	return false;

    return true;
}

/**
 * Simple test used to look for memory leaks.
 */
bool
test7(TestInfo& info)
{
    Spt<string> spt(info.verbose() /* enable tracing */);

    spt.add_node("s");
    spt.add_node("t");

    spt.set_origin("s");
    spt.add_edge("s", 10, "t");

    list<RouteCmd<string> > routes;

    if (!spt.compute(routes))
	return false;

    return true;
}

/**
 *  o ---10--> a
 *  |          ^
 *  |          |
 *  3         12
 *  |          |
 *  V          |
 *  b ----3--> c
 *
 *  Shortest path o->a is direct (nexthop: a)
 *  However, nexthop of b is currently returned
 *
 * Provided by: Adam Barr
 * Used to trigger a problem in the nexthop code.
 */
bool
test8(TestInfo& info)
{
    Spt<string> spt(info.verbose() /* enable tracing */);

    spt.add_node("o");
    spt.set_origin("o");

    spt.add_node("a");
    spt.add_node("b");
    spt.add_node("c");

    spt.add_edge("o", 10, "a");
    spt.add_edge("o", 3, "b");
    spt.add_edge("b", 3, "c");
    spt.add_edge("c", 12, "a");

    list<RouteCmd<string> > routes, expected_routes;
     
    if (!spt.compute(routes)) {
	DOUT(info) << "spt compute failed" << endl;
	return false;
    }

    for_each(routes.begin(), routes.end(), Pr<string>(info));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "a", "a", "o", 10));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "b", "b", "o", 3));

    expected_routes.
	push_back(RouteCmd<string>(RouteCmd<string>::ADD, "c", "b", "b", 6));

    if (!verify(info, routes, expected_routes)) {
	DOUT(info) << "verify failed" << endl;
	return false;
    }

    return true;
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

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    string fname =
	t.get_optional_args("-f", "--filename", "test data");
    t.complete_args_parsing();

    if (fname.empty()) {
	char *src = getenv("srcdir");
	fname = "spt_graph1";
	if (src)
	    fname = string(src) + "/" + fname;
    }

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"test1", callback(test1)},
	{"test2", callback(test2)},
	{"test3", callback(test3)},
	{"test4", callback(test4)},
	{"test5", callback(test5, fname)},
	{"test6", callback(test6)},
	{"test7", callback(test7)},
	{"test8", callback(test8)},
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		if (test == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return t.exit();
}
