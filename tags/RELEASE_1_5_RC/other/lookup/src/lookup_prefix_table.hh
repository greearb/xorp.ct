#ifndef __LOOKUP_PREFIX_TABLE_HH__
#define __LOOKUP_PREFIX_TABLE_HH__

#include <algorithm>
#include <map>
#include <vector>

namespace PrefixTableLookup {

    template <typename A>
    struct AddressBits {
	static const uint32_t value;
    };

    template <>
    struct AddressBits<IPv4> {
	static const uint32_t value = 32;
    };

    template <>
    struct AddressBits<IPv6> {
	static const uint32_t value = 128;
    };

    template <typename A, typename P>
    struct Entry {
    public:
	A addr;
	P port;

    public:
	Entry(const A& a, P p) : addr(a), port(p) {}
	Entry() : addr(A::ZERO()), port(NO_PORT) {}
	inline bool operator<(const Entry& o) const {
	    return addr < o.addr;
	}
    };

    template <typename A, typename P>
    struct EngineData {
	typedef vector<Entry<A,P> > Column;

	static const uint32_t n_columns = AddressBits<A>::value + 1;

	inline Column& get_column(uint32_t prefix) {
	    // assert(prefix < n_columns);
	    return columns[prefix];
	}

	inline const Column& get_column(uint32_t prefix) const {
	    // assert(prefix < n_columns);
	    return columns[prefix];
	}

	inline size_t bytes() const {
	    size_t s = sizeof(*this);
	    for (size_t i = 0; i < n_columns; i++) {
		s += columns[i].size() * sizeof(typename Column::value_type);
	    }
	    return s;
	}
	Column columns[n_columns];
    };


    template <typename A, typename P>
    class Compiler {
    public:
	typedef A AddrType;
	typedef P PortType;

	static const P NO_PORT = ~0;
	static const P MAX_PORT = NO_PORT - 1;

    public:
	Compiler(const char* /* settings */)	{}
	static const char* name() 		{ return "prefixtable"; }
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
    // PrefixTableLookupCompiler Implementation

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
    bool
    Compiler<A,P>::compile(EngineData<A,P>& ed)
    {
	for (size_t i = 0; i < EngineData<A,P>::n_columns; ++i) {
	    ed.get_column(i).clear();
	}

	// Transfer routes from map to vector for addresses of given
	// prefix length.
	for (typename map<IPNet<A>,P>::const_iterator i = _m.begin();
	     i != _m.end(); ++i) {
	    typename EngineData<A,P>::Column& pc = ed.get_column(i->first.prefix_len());
	    pc.push_back( Entry<A,P>(i->first.masked_addr(), i->second) );
	}

	for (uint32_t j = 0; j < EngineData<A,P>::n_columns; j++) {
	    typename EngineData<A,P>::Column& pc = ed.get_column(j);
	    sort(pc.begin(), pc.end());
	}

	return true;
    }


    // ------------------------------------------------------------------------
    // PrefixTableLookupEngine implementation

    template <typename A, typename P>
    P
    Engine<A,P>::lookup(const A& addr)
    {
	uint32_t prefix = EngineData<A,P>::n_columns;
	while (prefix != 0) {
	    prefix --;

	    const typename EngineData<A,P>::Column& pc = _ed->get_column(prefix);

	    A masked_addr = addr.mask_by_prefix_len(prefix);
	    Entry<A,P> e(masked_addr, 0);

	    typename EngineData<A,P>::Column::const_iterator i;
	    i = lower_bound(pc.begin(), pc.end(), e);

	    if (i != pc.end()) {
		if (i->addr == masked_addr) {
		    return i->port;
		} else {
		    continue;
		}
	    }
	}
	return NO_PORT;
    }

};

#endif /* __LOOKUP_PREFIX_TABLE_HH__ */
