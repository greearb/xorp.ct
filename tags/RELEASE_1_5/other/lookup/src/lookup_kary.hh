#ifndef __LOOKUP_KARY_HH__
#define __LOOKUP_KARY_HH__

#include <algorithm>
#include <map>
#include <vector>

namespace kAryLookup {

    template <typename P>
    struct kAryTable {
	static const P TBL_PTR  = 1 << (sizeof(P) * 8 - 1);		// msb
	static const P NO_PORT  = P(~TBL_PTR);
	static const P MAX_PORT = NO_PORT - 1;

    private:
	// An entry is either a pointer to a table (if TBL_PTR bit set)
	// or a port no
	vector<P>	_entries;
	uint32_t	_mw;

    public:
	kAryTable(uint32_t 	mask_width = 0,
		  P		defval 	   = NO_PORT)
	    : _entries(1 << mask_width, defval), _mw(mask_width)
	{
	}

	inline P entry(uint32_t idx) const {
	    return _entries[idx];
	}

	void set_entry(uint32_t idx, P p) {
	    _entries[idx] = p;
	}

	inline uint32_t size() const		{ return _entries.size(); }
	inline uint32_t log2size() const	{ return _mw; }
	inline size_t bytes() const
	{
	    return sizeof(this) + size() * sizeof(P);
	}
    };


    template <typename A, typename P>
    struct EngineData {
	typedef vector<kAryTable<P> > Container;
	Container tables;

	/**
	 * Computer size of data structure in bytes.
	 */
	size_t bytes() const
	{
	    size_t total = sizeof(*this);
	    total += tables.size() * sizeof(typename Container::value_type);
	    for (size_t i = 0; i < tables.size(); i++) {
		total += tables[i].bytes();
	    }
	    return total;
	}
    };


    template <typename A, typename P>
    class Compiler {
    public:
	typedef A AddrType;
	typedef P PortType;

	static const P NO_PORT  = kAryTable<P>::NO_PORT;
	static const P MAX_PORT = kAryTable<P>::MAX_PORT;
	static const P TBL_PTR  = kAryTable<P>::TBL_PTR;

	struct TableEntry {
	    TableEntry(const IPNet<A>& n, P p) : net(n), port(p) {}
	    TableEntry() : net(IPNet<A>(A::ZERO(), 0)), port(NO_PORT) {}
	    IPNet<A>	net;
	    P		port;
	};
	typedef vector<TableEntry> BuildTable;

    public:
	inline Compiler(const char* settings);
	static const char* name() { return "kary"; }
	inline bool add_route(const IPNet<A>& n, P portno);
	inline bool remove_route(const IPNet<A>& n);
	inline bool compile(EngineData<A,P>& ed);

    private:
	bool kary_add_route(EngineData<A,P>& 	ed,
			    uint32_t 		curtable,
			    uint32_t 		depth,
			    uint32_t 		addr,
			    uint32_t 		used,
			    uint32_t 		remain,
			    P 			port);

	void fill(kAryTable<P>& tbl, uint32_t low, uint32_t high, P port);
	P insert_table(EngineData<A,P>& ed, uint32_t depth, P port);

    private:
	map<IPNet<A>,P> 	_m;	// Used to hold routes between compiles
	vector<uint32_t>	_bits;	// bits at each depth
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
	inline const EngineData<A,P>* engine_data()			{ return _ed; }
	inline P lookup(const A& addr);

    private:
	const EngineData<A,P>* _ed;
    };

    
    // ------------------------------------------------------------------------
    // Compiler Implementation

    template <typename> struct Defaults {};
    template <>
    struct Defaults<IPv4> {
	static const char* settings()  { return "16/8/8"; }
    };

    template <typename A, typename P>
    Compiler<A,P>::Compiler(const char* settings)
    {
	if (settings == 0) {
	    settings = Defaults<A>::settings();
	}

	uint32_t total_bits = 0;
	uint32_t v = 0;
	const char* p = settings;
	for(;;) {
	    while (isdigit(*p)) {
		v = v * 10 + *p - '0';
		if (total_bits + v > A::ADDR_BITLEN) {
		    v = A::ADDR_BITLEN - total_bits;
		}
		p++;
	    }

	    _bits.push_back(v);
	    total_bits += v;
	    v = 0;

	    if (*p == '\0')
		break;
	    p++;
	}

	assert(total_bits == A::ADDR_BITLEN);
	cout << "Configuration string \"" << settings << "\"" << endl;
	assert(ntohl(A("10.0.0.1").addr()) == 0x0a000001);
    }

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
	bool operator() (const typename Compiler<A,P>::TableEntry& a,
			 const typename Compiler<A,P>::TableEntry& b)
	{
	    const IPNet<A>& na = a.net;
	    const IPNet<A>& nb = b.net;
	    if (na.prefix_len() < nb.prefix_len())
		return true;
	    if (na.prefix_len() > nb.prefix_len())
		return false;
	    return na.masked_addr() < nb.masked_addr();
	}
    };

    template <typename A, typename P>
    bool
    Compiler<A,P>::compile(EngineData<A,P>& ed)
    {
	typename map<IPNet<A>,P>::const_iterator mi = _m.begin();

	// Take routes from map, push them into vector and sort it.
	BuildTable bt;
	while (mi != _m.end()) {
	    bt.push_back(TableEntry(mi->first, mi->second));
	    ++mi;
	}
	stable_sort(bt.begin(), bt.end(), less_table_entry<A,P>());

	// Clear and add first table
	ed.tables.resize(0);
	ed.tables.push_back(kAryTable<P>(_bits[0]));

	// Add routes - these are sorted with increasing prefix order.
	for (size_t i = 0; i < bt.size(); i++) {
	    //	cout << i << " " << bt[i].net.str() << endl;
	    if (kary_add_route(ed, 0, 0,
			       ntohl(bt[i].net.masked_addr().addr()),
			       0,
			       bt[i].net.prefix_len(),
			       bt[i].port) == false) {
		return false;
	    }
	    if (ed.tables.size() > kAryTable<P>::MAX_PORT) {
		cerr << "Too many tables for size of P type." << endl;
		return false;
	    }
	}
	cout << "# kary made tables = " << ed.tables.size() << endl;
	return true;
    }

    template <typename A, typename P>
    void
    Compiler<A,P>::fill(kAryTable<P>& 	tbl,
			uint32_t 	lo,
			uint32_t 	hi,
			P 		p)
    {
	for (uint32_t i = lo; i <= hi; i++) {
	    tbl.set_entry(i, p);
	}
    }

    template <typename A, typename P>
    P
    Compiler<A,P>::insert_table(EngineData<A,P>&	ed,
				uint32_t 		depth,
				P	 		port)
    {
	P ret = ed.tables.size();
	ed.tables.push_back(kAryTable<P>(_bits[depth], port));
	return ret | kAryTable<P>::TBL_PTR;
    }

    inline uint32_t
    make_mask(uint32_t bits) {
	if (bits >= 32)
	    return ~0;
	return (1 << bits) - 1;
    }

    template <typename A, typename P>
    bool
    Compiler<A,P>::kary_add_route(EngineData<A,P>& 	ed,
				  uint32_t 		curtable,
				  uint32_t 		depth,
				  uint32_t 		addr,
				  uint32_t 		used,
				  uint32_t 		remain,
				  P 			port)
    {
	uint32_t m = addr >> (32 - used - _bits[depth]);
	m &= make_mask(_bits[depth]);

	if (remain <= _bits[depth]) {
	    fill(ed.tables[curtable], m,
		 m | make_mask(_bits[depth] - remain), port);
	    return true;
	} else {
	    uint32_t ptr = ed.tables[curtable].entry(m);
	    if (ptr & kAryTable<P>::TBL_PTR) {
		return kary_add_route(ed,
				      ptr ^ kAryTable<P>::TBL_PTR,
				      depth + 1,
				      addr,
				      used + _bits[depth],
				      remain - _bits[depth],
				      port);
	    }

	    // Create new table and fill it with value we're about to replace
	    P new_ptr = insert_table(ed, depth + 1, ptr);
	    ed.tables[curtable].set_entry(m, new_ptr);
	    return kary_add_route(ed,
				  new_ptr ^ kAryTable<P>::TBL_PTR,
				  depth + 1,
				  addr,
				  used + _bits[depth],
				  remain - _bits[depth],
				  port);
	}

	return true;
    }

    
    // ------------------------------------------------------------------------
    // kAryIpLookupEngine implementation

    template <typename A, typename P>
    P
    Engine<A,P>::lookup(const A& addr)
    {
	const kAryTable<P>* t = &(_ed->tables[0]);
	uint32_t u = A::ADDR_BITLEN;
	uint32_t l, idx;
	for(;;) {
	    l = t->log2size();
	    u -= l;
	    idx = addr.bits(u, l);

	    P p = t->entry(idx);

	    if ((p & kAryTable<P>::TBL_PTR) == 0) {
		return p;
	    }
	    t = &(_ed->tables[p & kAryTable<P>::NO_PORT]);
	}
	return NO_PORT;
    }

}; // kAryLookup -- end of namepspace

#endif /* __LOOKUP_KARY_HH__ */
