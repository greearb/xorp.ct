/*
 * This is the dog-slowest lookup you can imagine.  It does linear
 * search on an unsorted list of routes for the best match.
 */

#ifndef __LOOKUP_BRUTUS_HH__
#define __LOOKUP_BRUTUS_HH__

#include <map>
#include <vector>

namespace BruteLookup {

    template <typename A, typename P>
    struct RouteEntry {
	RouteEntry(const IPNet<A>& n, P p) : net(n), port(p) {}
	RouteEntry() : net(IPNet<A>(A::ZERO(), 0)), port(0) {}
	IPNet<A>	net;
	P		port;
    };

    template <typename A, typename P>
    struct EngineData {
    public:
	typedef vector<RouteEntry<A,P> > Container;

    public:
	Container entries;

    public:
	/**
	 * @return sizeof data structure in bytes.
	 */
	size_t bytes() const {
	    size_t s = sizeof(*this);
	    s += sizeof(typename Container::value_type) * entries.size();
	    return s;
	}
    };


    template <typename A, typename P>
    class Compiler {
    public:
	typedef A 			AddrType;
	typedef P 			PortType;

	static const P NO_PORT  = ~0;
	static const P MAX_PORT = NO_PORT - 1;

    public:
	Compiler(const char* /* settings */) {}
	static const char* name() { return "brute"; }
	inline bool add_route(const IPNet<A>& n, P portno);
	inline bool remove_route(const IPNet<A>& n);
	inline bool compile(EngineData<A,P>& ed);

    private:
	map<IPNet<A>,P> 	_m;
    };


    template <typename A, typename P>
    class Engine {
    public:
	typedef A 	AddrType;
	typedef P 	PortType;

	static const P NO_PORT  = Compiler<A,P>::NO_PORT;
	static const P MAX_PORT = Compiler<A,P>::MAX_PORT;

    public:
	inline Engine() {}

	inline void set_engine_data(const EngineData<A,P>* ned)	{ _ed = ned; }
	inline const EngineData<A,P>* engine_data()		{ return _ed; }
	inline P lookup(const A& addr);

    private:
	const EngineData<A,P>* _ed;
    };


    // ------------------------------------------------------------------------
    // Compiler Implementation

    template <typename A, typename P>
    bool
    Compiler<A,P>::add_route(const IPNet<A>& net, P port)
    {
	if (port > MAX_PORT)
	    return false;

	_m[net] = port;
	return true;
    }

    template <typename A, typename P>
    bool
    Compiler<A,P>::remove_route(const IPNet<A>& net)
    {
	typename map<IPNet<A>,P>::iterator i = _m.find(net);
	if (i != _m.end()) {
	    _m.erase(i);
	    return true;
	}
	return false;
    }

    template <typename A, typename P>
    bool
    Compiler<A,P>::compile(EngineData<A,P>& ed)
    {
	ed.entries.resize(0);

	typename map<IPNet<A>,P>::const_iterator i = _m.begin();
	while (i != _m.end()) {
	    ed.entries.push_back(RouteEntry<A,P>(i->first, i->second));
	    ++i;
	}
	return true;
    }


    // ------------------------------------------------------------------------
    // Engine implementation

    template <typename A, typename P>
    P
    Engine<A,P>::lookup(const A& addr)
    {
	P 		best_port  = NO_PORT;
	int32_t 	best_score = 0;
	int32_t 	score;

	typename EngineData<A,P>::Container::const_iterator i;
	for (i = _ed->entries.begin(); i != _ed->entries.end(); ++i) {
	    if (i->net.contains(addr)) {
		score = 1 + i->net.prefix_len();
		if (score > best_score) {
		    best_score = score;
		    best_port  = i->port;
		}
	    }
	}
	return best_port;
    }

};

#endif /* __LOOKUP_BRUTUS_HH__ */
