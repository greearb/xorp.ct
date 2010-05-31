#ifndef __LOOKUP_LINEAR_HH__
#define __LOOKUP_LINEAR_HH__

#include <algorithm>
#include <map>
#include <vector>

namespace LinearLookup {

    template <typename A, typename P>
    struct Entry {
	Entry(const IPNet<A>& n, P p) : net(n), port(p) {}
	Entry() : net(IPNet<A>(A::ZERO(), 0)), port(0) {}
	IPNet<A>	net;
	P		port;
    };

    template <typename A, typename P>
    struct EngineData {
	typedef vector<Entry<A,P> > Container;
	Container entries;

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
	Compiler(const char* /* settings */) 		{}
	static const char* name()			{ return "linear"; }
	inline bool add_route(const IPNet<A>& n, P portno);
	inline bool remove_route(const IPNet<A>& n);
	inline bool compile(EngineData<A,P>& ed);

    private:
	map<IPNet<A>,P>	_m;	// Used to build table
    };


    template <typename A, typename P>
    class Engine {
    public:
	typedef A AddrType;
	typedef P PortType;

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
	if (i != _m.end) {
	    _m.erase(i);
	    return true;
	}
	return false;
    }

    template <typename A, typename P>
    struct less_table_entry {
	bool operator() (const Entry<A,P>& a,
			 const Entry<A,P>& b)
	{
	    const IPNet<A>& na = a.net;
	    const IPNet<A>& nb = b.net;
	    if (na.prefix_len() > nb.prefix_len())
		return true;
	    if (na.prefix_len() < nb.prefix_len())
		return false;
	    return na.masked_addr() < nb.masked_addr();
	}
    };

    template <typename A, typename P>
    bool
    Compiler<A,P>::compile(EngineData<A,P>& ed)
    {
	typename map<IPNet<A>,P>::const_iterator i = _m.begin();
	ed.entries.resize(0);

	while (i != _m.end()) {
	    ed.entries.push_back(Entry<A,P>(i->first, i->second));
	    ++i;
	}
	stable_sort(ed.entries.begin(),
		    ed.entries.end(),
		    less_table_entry<A,P>());

	return true;
    }


    // ------------------------------------------------------------------------
    // Engine implementation

    template <typename A, typename P>
    P
    Engine<A,P>::lookup(const A& addr)
    {
	typename EngineData<A,P>::Container::const_iterator i;
	for (i = _ed->entries.begin(); i != _ed->entries.end(); ++i) {
	    if (i->net.contains(addr)) {
		return i->port;
	    }
	}
	return NO_PORT;
    }

};

#endif /* __LOOKUP_LINEAR_HH__ */
