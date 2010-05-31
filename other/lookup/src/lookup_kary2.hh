// Same as lookup kary table, but tries to use arrays inline in data
// structure rather than stl vectors.

#ifndef __LOOKUP_KARY2_HH__
#define __LOOKUP_KARY2_HH__

#include <algorithm>
#include <map>
#include <vector>

namespace kAry2Lookup {

    struct Allocator {
	static const uint32_t allocator_size = 20000000;

	Allocator() { _p = new uint8_t[allocator_size]; _h = _p; }
	~Allocator() { delete _p; }

	bool can_allocate(size_t bytes) const {
	    return (_h + bytes) < (_p + allocator_size);
	}

	uint8_t* get(size_t bytes) {
	    if (can_allocate(bytes)) {
		uint8_t* r = _h;
		_h += bytes;
		return r;
	    }
	    return 0;
	}
	size_t bytes() { return sizeof(*this) + allocator_size; }

    private:
	uint8_t* _p;	// pool
	uint8_t* _h;	// allocation head
    };


    template <uint32_t M, typename P>
    struct kAry2Table {
	static const P TBL_PTR  = 1 << (sizeof(P) * 8 - 1);		// msb
	static const P NO_PORT  = P(~TBL_PTR);
	static const P MAX_PORT = NO_PORT - 1;

    private:
	uint32_t	_mw;			// Mask width
	uint32_t	_tbl_sz;
	P 		_entries[1 << M];	// Pointer to a table
						// (if TBL_PTR) or a port no.
    public:
	kAry2Table(P	defval = NO_PORT)
	    : _mw(M), _tbl_sz(1 << M)
	{
	    for (uint32_t i = 0 ; i < _tbl_sz; i++) {
		_entries[i] = defval;
	    }
	}

	inline P entry(uint32_t idx) const {
	    // assert(idx < _tbl_sz);
	    return _entries[idx];
	}

	inline void set_entry(uint32_t idx, P p) {
	    // assert(idx < _tbl_sz);
	    _entries[idx] = p;
	}

	inline uint32_t bytes() const	{ return _tblsz * sizeof(P); }
	inline uint32_t size() const	{ return _tbl_sz; }
	inline uint32_t log2size() const	{ return _mw; }
    };


    template <typename A, typename P>
    struct EngineData {
    public:
	typedef vector<kAry2Table<0,P>* > Container;

    public:
	Container tables;
	Allocator* _a;

    public:
	EngineData()
	{
	    _a = new Allocator();
	}

	~EngineData()
	{
	    delete _a;
	}

	/**
	 * Computer size of data structure in bytes.
	 */
	size_t bytes() const
	{
	    size_t total = tables.size() * sizeof(typename Container::value_type);
	    for (size_t i = 0; i < tables.size(); i++) {
		total += tables[i]->size();
	    }
	    return total;
	}

	inline Allocator& allocator()				{ return *_a; }

    private:
	EngineData(const EngineData& o);		// not implemented
	EngineData& operator=(const EngineData& o);	// not implemented
    };


    template <typename A, typename P>
    class Compiler {
    public:
	typedef A AddrType;
	typedef P PortType;

	static const P NO_PORT  = kAry2Table<0,P>::NO_PORT;
	static const P MAX_PORT = kAry2Table<0,P>::MAX_PORT;
	static const P TBL_PTR  = kAry2Table<0,P>::TBL_PTR;

	struct TableEntry {
	    TableEntry(const IPNet<A>& n, P p) : net(n), port(p) {}
	    TableEntry() : net(IPNet<A>(A::ZERO(), 0)), port(NO_PORT) {}
	    IPNet<A>	net;
	    P		port;
	};
	typedef vector<TableEntry> BuildTable;

    public:
	inline Compiler(const char* settings);
	static const char* name()		{ return "kary2"; }
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

	void fill(kAry2Table<0, P>* tbl, uint32_t low, uint32_t high, P port);
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
	static const P TBL_PTR  = Compiler<A,P>::TBL_PTR;

    public:
	inline Engine() {}
	inline void set_engine_data(const EngineData<A,P>* ned)	{ _ed = ned; }
	inline const EngineData<A,P>* engine_data()			{ return _ed; }
	inline P lookup(const A& addr);

    private:
	const EngineData<A,P>* _ed;
    };


    //------------------------------------------------------------------------
    // Utility methods

    template <typename P>
    static kAry2Table<0,P>*
    make_table(Allocator& 	alloc,
	       uint32_t 	log2size,
	       P 		default_value = kAry2Table<0,P>::NO_PORT)
    {
#define MAKE_TABLE_IF_SIZE(n)						\
	case n:								\
	{								\
	    uint8_t* pool = alloc.get(sizeof(kAry2Table<n,P>)); 	\
	    assert(pool != 0);						\
	    kAry2Table<n,P>* t = new (pool) kAry2Table<n,P>(default_value); \
	    return reinterpret_cast<kAry2Table<0,P>*>(t);		\
	}

	switch (log2size) {
	    MAKE_TABLE_IF_SIZE(0x00);
	    MAKE_TABLE_IF_SIZE(0x01);
	    MAKE_TABLE_IF_SIZE(0x02);
	    MAKE_TABLE_IF_SIZE(0x03);
	    MAKE_TABLE_IF_SIZE(0x04);
	    MAKE_TABLE_IF_SIZE(0x05);
	    MAKE_TABLE_IF_SIZE(0x06);
	    MAKE_TABLE_IF_SIZE(0x07);
	    MAKE_TABLE_IF_SIZE(0x08);
	    MAKE_TABLE_IF_SIZE(0x09);
	    MAKE_TABLE_IF_SIZE(0x0a);
	    MAKE_TABLE_IF_SIZE(0x0b);
	    MAKE_TABLE_IF_SIZE(0x0c);
	    MAKE_TABLE_IF_SIZE(0x0d);
	    MAKE_TABLE_IF_SIZE(0x0e);
	    MAKE_TABLE_IF_SIZE(0x0f);
	    MAKE_TABLE_IF_SIZE(0x10);
	    MAKE_TABLE_IF_SIZE(0x11);
	    MAKE_TABLE_IF_SIZE(0x12);
	    MAKE_TABLE_IF_SIZE(0x13);
	    MAKE_TABLE_IF_SIZE(0x14);
	    MAKE_TABLE_IF_SIZE(0x15);
	    MAKE_TABLE_IF_SIZE(0x16);
	    MAKE_TABLE_IF_SIZE(0x17);
	    MAKE_TABLE_IF_SIZE(0x18);
	    MAKE_TABLE_IF_SIZE(0x19);
	    MAKE_TABLE_IF_SIZE(0x1a);
	    MAKE_TABLE_IF_SIZE(0x1b);
	    MAKE_TABLE_IF_SIZE(0x1c);
	}
	return 0;
    }

    inline uint32_t
    make_mask(uint32_t bits) {
	if (bits >= 32)
	    return ~0;
	return (1 << bits) - 1;
    }


    template <typename A, typename P>
    struct k2less_table_entry {
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

    // -----------------------------------------------------------------------
    // kAry2IpLookupCompiler Implementation

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
	stable_sort(bt.begin(), bt.end(), k2less_table_entry<A,P>());

	// Clear and add first table
	ed.tables.resize(0);
	ed.tables.push_back(make_table(ed.allocator(), _bits[0], NO_PORT));

	// Add routes - these are sorted with increasing prefix order.
	for (size_t i = 0; i < bt.size(); i++) {
	    //	cout << i << " " << bt[i].net.str() << endl;
	    if (kary_add_route(ed,
			       0, 0,
			       ntohl(bt[i].net.masked_addr().addr()),
			       0,
			       bt[i].net.prefix_len(),
			       bt[i].port) == false) {
		return false;
	    }
	    if (ed.tables.size() > MAX_PORT) {
		cerr << "Too many tables for size of P type." << endl;
		return false;
	    }
	}
	cout << "# kary made tables = " << ed.tables.size() << endl;
	return true;
    }

    template <typename A, typename P>
    void
    Compiler<A,P>::fill(kAry2Table<0,P>* tbl,
			uint32_t 	lo,
			uint32_t 	hi,
			P 		p)
    {
	for (uint32_t i = lo; i <= hi; i++) {
	    tbl->set_entry(i, p);
	}
    }

    template <typename A, typename P>
    P
    Compiler<A,P>::insert_table(EngineData<A,P>& 	ed,
				uint32_t 		depth,
				P	 		port)
    {
	P ret = ed.tables.size();
	ed.tables.push_back(make_table(ed.allocator(), _bits[depth], port));
	return ret | TBL_PTR;
    }

    template <typename A, typename P>
    bool
    Compiler<A,P>::kary_add_route(EngineData<A,P>& 	ed,
				  uint32_t 		curtable,
				  uint32_t 		depth,
				  uint32_t 		addr,
				  uint32_t 		used,
				  uint32_t 		remain,
				  P 	   		port)
    {
	uint32_t m = addr >> (32 - used - _bits[depth]);
	m &= make_mask(_bits[depth]);

	if (remain <= _bits[depth]) {
	    fill(ed.tables[curtable], m,
		 m | make_mask(_bits[depth] - remain), port);
	    return true;
	} else {
	    uint32_t ptr = ed.tables[curtable]->entry(m);
	    if (ptr & TBL_PTR) {
		return kary_add_route(ed,
				      ptr ^ TBL_PTR,
				      depth + 1,
				      addr,
				      used + _bits[depth],
				      remain - _bits[depth],
				      port);
	    }

	    // Create new table and fill it with value we're about to replace
	    P new_ptr = insert_table(ed, depth + 1, ptr);
	    ed.tables[curtable]->set_entry(m, new_ptr);
	    return kary_add_route(ed,
				  new_ptr ^ TBL_PTR,
				  depth + 1,
				  addr,
				  used + _bits[depth],
				  remain - _bits[depth],
				  port);
	}

	return true;
    }

    
    // ------------------------------------------------------------------------
    // Engine implementation

    template <typename A, typename P>
    P
    Engine<A,P>::lookup(const A& addr)
    {
	const kAry2Table<0,P>* t = _ed->tables[0];
	uint32_t u = A::ADDR_BITLEN;
	uint32_t l, idx;
	for(;;) {
	    // depth 0 + X
	    l = t->log2size();
	    u -= l;
	    //m   = t->size() - 1;
	    //idx = (ntohl(addr.addr()) >> u) & m;
	    idx = addr.bits(u, l);

	    P p = t->entry(idx);
	    if ((p & TBL_PTR) == 0) {
		return p;
	    }


	    // depth 1 + X
	    t = _ed->tables[p & NO_PORT];
	    l = t->log2size();
	    u -= l;
	    idx = addr.bits(u, l);

	    p = t->entry(idx);
	    if ((p & TBL_PTR) == 0) {
		return p;
	    }

	    // depth 2 + X
	    t = _ed->tables[p & NO_PORT];
	    l = t->log2size();
	    u -= l;
	    idx = addr.bits(u, l);

	    p = t->entry(idx);
	    if ((p & TBL_PTR) == 0) {
		return p;
	    }
	    t = _ed->tables[p & NO_PORT];
	}
	return NO_PORT;
    }

};
#endif /* __LOOKUP_KARY2_HH__ */
