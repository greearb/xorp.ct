
#ifndef __LOOKUP_KARY_COMPRESSED_HH__
#define __LOOKUP_KARY_COMPRESSED_HH__

#include "lookup_kary.hh"

namespace kAryCompressedLookup {

    template <typename P>
    struct CompressedTable {
	typedef uint16_t Key;

	struct Entry {
	    Entry(const Key& k, P p) : key(k), port(p) {}
	    Key	key;
	    P	port;
	    inline bool operator<(const Entry& o) const { return key < o.key; };
	};

	CompressedTable(uint32_t mask_width)
	    : _mw(mask_width), _sz(1 << mask_width) {}
	CompressedTable() {}

	inline P find(uint16_t key) const;
	void push_back(const Entry& e) { _entries.push_back(e); }

	//
	// returns size of uncompressed table, ie effective number of entries.
	//
	inline size_t size() const		{ return _sz; }

	//
	// returns log base 2 of the uncompressed table size.
	//
	inline size_t log2size() const		{ return _mw; }

	//
	// return number of real entries in table.
	//
	inline size_t table_entries() const	{ return _entries.size(); }

	inline size_t bytes() const
	{
	    return sizeof(this) + _entries.size() * sizeof(Entry);
	}

    private:
	size_t 		_mw;
	size_t 		_sz;
	vector<Entry> 	_entries;
    };


    template <typename A, typename P>
    struct EngineData {
    public:
	typedef kAryLookup::kAryTable<P> PrimaryTable;
	typedef CompressedTable<P> SecondaryTable;
	typedef vector<CompressedTable<P> > SecondaryTables;

    public:
	PrimaryTable	primary_table;
	SecondaryTables secondary_tables;

    public:
	EngineData() {}

	void flush()
	{
	    secondary_tables.resize(0);
	}

	size_t bytes() const
	{
	    size_t total = sizeof(*this);
	    total += primary_table.bytes();
	    for (uint32_t i = 0; i < secondary_tables.size(); i++)
		total += secondary_tables[i].bytes();
	    return total;
	}
    private:
	EngineData(const EngineData&);			// not implemented
	EngineData& operator=(const EngineData&);	// not implemented
    };


    template <typename A, typename P>
    class Compiler {
    public:
	typedef A AddrType;
	typedef P PortType;

	static const P NO_PORT  = kAryLookup::Compiler<A,P>::NO_PORT;
	static const P MAX_PORT = kAryLookup::Compiler<A,P>::MAX_PORT;
	static const P TBL_PTR  = kAryLookup::Compiler<A,P>::TBL_PTR;

    public:
	Compiler(const char* settings) : _k(settings)	{}
	static const char* name()	{ return "karycompressed"; }

	inline bool add_route(const IPNet<A>& n, P portno)
	{
	    return _k.add_route(n, portno);
	}

	inline bool remove_route(const IPNet<A>& n)
	{
	    return _k.remove_route(n);
	}

	bool compile(EngineData<A,P>& ed);

    private:
	P compress_table_ptr(P p)
	{
	    // Compressing a table pointer just requires subtracting 1
	    // from it.  The first table is not compressed and stored
	    // in a different strucure.
	    if (p & TBL_PTR) {
		p = p ^ TBL_PTR;
		p = p - 1;
		return p | TBL_PTR;
	    }
	    return p;
	}

    private:
	kAryLookup::Compiler<A,P> _k;
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
	inline const EngineData<A,P>* engine_data()		{ return _ed; }
	inline P lookup(const A& addr);

    private:
	const EngineData<A,P>* _ed;
    };


    template <typename P>
    inline P CompressedTable<P>::find(uint16_t key) const
    {
	typename vector<Entry>::const_iterator it = _entries.begin();
	it = _entries.begin();
	if (key <= it->key)	// 0
	    return it->port;
	it++;
	if (key <= it->key)	// 1
	    return it->port;
	it++;
	if (key <= it->key)	// 2
	    return it->port;
	it++;
	if (key <= it->key)	// 3
	    return it->port;
	it++;
	if (key <= it->key)	// 4
	    return it->port;
	it++;
	if (key <= it->key)	// 5
	    return it->port;
	it++;
	if (key <= it->key)	// 6
	    return it->port;
	it++;
	if (key <= it->key)	// 7
	    return it->port;
	it++;
	if (key <= it->key)	// 8
	    return it->port;
	it++;
	if (key <= it->key)	// 9
	    return it->port;
	it++;
	if (key <= it->key)	// 10
	    return it->port;
	it++;
	if (key <= it->key)	// 11
	    return it->port;
	it++;
	if (key <= it->key)	// 12
	    return it->port;
	it++;
	if (key <= it->key)	// 13
	    return it->port;
	it++;
	if (key <= it->key)	// 14
	    return it->port;
	it++;
	if (key <= it->key)	// 15
	    return it->port;
	it++;

	it = lower_bound(it, _entries.end(), Entry(key, 0));
	return it->port;
    }


    template <typename A, typename P>
    bool
    Compiler<A,P>::compile(EngineData<A,P>& ed)
    {
	typename kAryLookup::EngineData<A,P> raw;
	if (_k.compile(raw) == false)
	    return false;

	ed.flush();
	ed.primary_table = raw.tables[0];

	// Fix pointers in primary table, compressed table format
	// has uncompressed first level table and compressed lower
	// levels.  We need to subtract 1 from table pointer since
	// it now indexes an array not including primary table.
	for (size_t i = 0; i < ed.primary_table.size(); i++) {
	    P p = compress_table_ptr(ed.primary_table.entry(i));
	    ed.primary_table.set_entry(i, p);
	}

	uint32_t one_entries = 0;
	for (size_t i = 1; i < raw.tables.size(); i++) {
	    const kAryLookup::kAryTable<P>& origin = raw.tables[i];
	    // Create new secondary table
	    ed.secondary_tables.push_back(CompressedTable<P>(origin.log2size()));
	    CompressedTable<P>& ct = ed.secondary_tables.back();

	    // Set last to be first entry in table.
	    typename CompressedTable<P>::Entry
		last(0, compress_table_ptr(origin.entry(0)));
	    for (size_t j = 1; j < origin.size(); j++) {
		P p = compress_table_ptr(origin.entry(j));
		if (p != last.port) {
		    ct.push_back(last);
		}
		last = typename CompressedTable<P>::Entry(j,p);
	    }
	    ct.push_back(last);

	    if (ct.table_entries() == 1)
		one_entries++;

	    // The following is all for debugging - we check all the
	    // table entries are the same.
	    {
		bool fail = false;
		for (size_t z = 0; z < origin.size(); z++) {
		    P p1 = ct.find(z);
		    P p2 = compress_table_ptr(origin.entry(z));
		    if (p1 != p2) {
			fail = true;
		    }
		}
		if (fail) {
		    cerr << "-- Table " << i << endl;
		    for (size_t z = 0; z < origin.size(); z++) {
			P p1 = ct.find(z);
			P p2 = compress_table_ptr(origin.entry(z));
			if (p1 != p2) {
			    cerr << "! " << z << " orig " << p2 << " new " << p1 << endl;
			}
			cerr << "  " << z << " orig " << p2 << " new " << p1 << endl;
		    }
		    assert(fail == false);
		}
	    }
	}
	cout << "# Single entry tables " << one_entries << endl;
	return true;
    }


    template <typename A, typename P>
    P
    Engine<A,P>::lookup(const A& addr)
    {
	uint32_t u = A::ADDR_BITLEN;
	uint32_t l;
	uint32_t idx;

	l = _ed->primary_table.log2size();
	u -= l;
	idx = addr.bits(u, l);
	P p = _ed->primary_table.entry(idx);
	if ((p & TBL_PTR) == 0) {
	    return p;
	}

	for (;;)
	{
	    const CompressedTable<P>& t = _ed->secondary_tables[p & NO_PORT];
	    l = t.log2size();
	    u -= l;
	    idx = addr.bits(u, l);
	    p = t.find(idx);
	    if ((p & TBL_PTR) == 0)
		return p;
	}
    }
};

#endif /* __LOOKUP_KARY_COMPRESSED_HH__ */
