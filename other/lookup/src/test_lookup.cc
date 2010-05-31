using namespace std;

#include "config.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <list>

#include <sys/types.h>
#include <sys/time.h>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "lookup_brutus.hh"
#include "lookup_linear.hh"
#include "lookup_prefix_table.hh"
#include "lookup_xorp_trie.hh"
#include "lookup_kary.hh"
#include "lookup_kary2.hh"
#include "lookup_kary_compressed.hh"

// Max port - limits assignment of nets to port numbers.  Must be a
// power of two.  Number is equivalent to number of ingress/egress
// points within system.
static const uint32_t MAX_PORT = 32;

// Number of ticks per second according to measurement system.
static uint64_t n_tps;

#ifdef HAVE_PENTIUM_COUNTERS

inline static void
rdtsc(uint64_t& x)
{
    __asm __volatile(".byte 0x0f, 0x31" : "=A" (x));
}

#else // HAVE_PENTIUM_COUNTERS

inline static void
rdtsc(uint64_t& x)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    x = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

#endif // HAVE_PENTIUM_COUNTERS

static void
callibrate_tps()
{
    // This is very rough!
    uint64_t t0, t1;
    rdtsc(t0);
    sleep(1);
    rdtsc(t1);
    n_tps = t1 - t0;
}

static const uint64_t&
tps()
{
    return n_tps;
}


/**
 * Utility structure to hold Network,Port pairs.
 */
template <typename A>
struct RouteEntry
{
    RouteEntry(const IPNet<A>& n, uint32_t p) : net(n), port(p) {}
    IPNet<A> 	net;
    uint32_t 	port;
};

/**
 * Structure for touching drawing data into cache, flushing away
 * lookup engine state.
 */
struct CacheKiller
{
    CacheKiller(size_t cache_size = 128 * 1024)
	: _p(0), _sz(cache_size)
    {
	if (cache_size) {
	    _p = new char [cache_size];
	}
    }

    ~CacheKiller()
    {
	delete _p;
    }

    void kill()
    {
	if (_sz) {
	    for (uint32_t i = 0; i < _sz; i += 8) {
		_p[i] ^= _p[_sz - i - 1];
	    }
	}
    }

protected:
    char*  _p;
    size_t _sz;
};


template <typename A>
struct TestEngineCompilerBase
{
    virtual ~TestEngineCompilerBase() {};

    /**
     * Add routes to route table compiler.
     */
    virtual void add_routes(const vector<RouteEntry<A> >& routes) = 0;

    /**
     * Compile routes added with @ref add_route() into lookup data
     * structure and pass structure to lookup engine for validation
     * and benchmarking.
     */
    virtual void compile_and_push() = 0;

    /**
     * Validate Engine looking up supplied routes.
     *
     * @param n_routes_validate number of routes of those supplied to
     * use in validating.  0 indicates all the roues supplied should
     * be used.
     */
    virtual void validate_engine(const vector<RouteEntry<A> >& routes,
				 uint32_t n_routes_validate = 0) = 0;

    /**
     * Benchmark Engine looking up supplied routes.
     *
     * @param n_routes_benchmark number of routes of those supplied to
     * use in benchmarking.  0 indicates all the roues supplied should
     * be used.
     */
   virtual void benchmark(const vector<RouteEntry<A> >& routes,
			   uint32_t n_routes_benchmark = 0) = 0;

};

/**
 * Test Framework for Lookup Compiler and Engines.
 */
template <typename C, typename E, typename D>
struct TestEngineCompiler
    : public TestEngineCompilerBase<typename C::AddrType>
{
    typedef typename C::AddrType A;
    typedef typename C::PortType P;
    typedef C Compiler;
    typedef E Engine;
    typedef D EngineData;

    TestEngineCompiler(const char* config) : _c(config)
    {
    }

    void add_routes(const vector<RouteEntry<A> >& r)
    {
	uint64_t t0, t1;

	rdtsc(t0);
	for (size_t i = 0; i < r.size(); i++) {
	    _c.add_route(r[i].net, r[i].port);
	}
	rdtsc(t1);
    }

    void compile_and_push()
    {
	uint64_t t0, t1;

	cerr << "Compiling...";
	rdtsc(t0);
	if (_c.compile(_d) == false) {
	    cerr << "failed." << endl;
	    exit(-1);
	}
	rdtsc(t1);
	cerr << "...done." << endl;

	cout << "# compiled in    " << t1 - t0 << " ticks" << endl;
	cout << "# engine data size " << _d.bytes() << " bytes" << endl;
	_e.set_engine_data(&_d);
    }

    void validate_engine(const vector<RouteEntry<A> >& r, uint32_t n = 0)
    {
	if (n == 0 || n > r.size()) {
	    n = r.size();
	}

	PrefixTableLookup::Compiler<A,P>	vc(0);
	PrefixTableLookup::Engine<A,P>		ve;
	PrefixTableLookup::EngineData<A,P>	ved;

	for (uint32_t i = 0; i < n; i++) {
	    vc.add_route(r[i].net, r[i].port);
	}
	vc.compile(ved);
	ve.set_engine_data(&ved);

	cerr << "Validating " << n << " routes" << endl;

	for (size_t i = 0; i < n; i++) {
	    A addr(r[i].net.masked_addr());
	    P pv = ve.lookup(addr) & (MAX_PORT - 1);
	    P pe = _e.lookup(addr) & (MAX_PORT - 1);
	    if (pv != pe) {
		cout << "! Lookup mismatch resolving " << addr.str() << " ";
		cout << " expected " << uint32_t(pv) << " != "
		     << " returned " << uint32_t(pe) << endl;
	    }
	    if (i && (i % 1000) == 0) cerr << ".";
	    if (i && (i % 10000) == 0) {
		int w = cerr.width(6);
		cerr << i;
		cerr.width(w);
	    }
	}

	cerr << "...done." << endl;
    }

    void benchmark(const vector<RouteEntry<A> >& r, uint32_t n = 0)
    {
	if (n == 0 || n > r.size()) {
	    n = r.size();
	}

	cout.setf(ios::fixed, ios::floatfield);
	cout.precision(2);

	uint64_t min_ticks = ~0;
	uint64_t max_ticks = 0;
	uint64_t sum_ticks = 0;

	uint64_t t0, t1, delta;
	double avg;

	//
	// WARM CACHE SINGLE MEASUREMENTS
	//
	for (size_t i = 0; i < n; i++) {
	    A addr(random());
	    rdtsc(t0);
	    _e.lookup(addr);
	    rdtsc(t1);

	    delta = t1 - t0;
	    if (delta < min_ticks) {
		min_ticks = delta;
	    }
	    if (delta > max_ticks) {
		max_ticks = delta;
	    }
	    sum_ticks += delta;
	}

	avg = double(sum_ticks) / double(n);
	cout << endl;
	cout << "=== WARM CACHE SINGLE MEASUREMENTS ===" << endl;
	cout << "Lookups   = " << n << endl;
	cout << "Min / Avg / Max ticks = "
	     << min_ticks	<< " "
	     << avg		<< " "
	     << max_ticks	<< " "
	     << endl;
	cout << "Effective LPS = " << setw(15)
	     << (tps() / avg) << endl;

	//
	// COLD CACHE SINGLE MEASUREMENTS
	//
	CacheKiller ck;
	for (size_t i = 0; i < n; i++) {
	    ck.kill();
	    A addr(random());
	    rdtsc(t0);
	    _e.lookup(addr);
	    rdtsc(t1);

	    delta = t1 - t0;
	    if (delta < min_ticks) {
		min_ticks = delta;
	    }
	    if (delta > max_ticks) {
		max_ticks = delta;
	    }
	    sum_ticks += delta;
	}

	avg = double(sum_ticks) / double(n);

	cout << endl;
	cout << "=== COLD CACHE SINGLE MEASUREMENTS ===" << endl;
	cout << "Lookups   = " << n	    << endl;
	cout << "Min / Avg / Max ticks = "
	     << min_ticks 	<< " "
	     << avg		<< " "
	     << max_ticks 	<< " "
	     << endl;
	cout << "Effective LPS = " << setw(15)
	     << (tps() / avg) << endl;

	//
	// WARM CACHE GROUP MEASUREMENTS
	//
	static const size_t N = 1; // Number of times routes tested.

	rdtsc(t0);
	for (size_t k = 0; k < N; k++) {
	    for (size_t i = 0; i < n; i++) {
		A addr(random());
		_e.lookup(addr);
	    }
	}
	rdtsc(t1);
	sum_ticks = t1 - t0;

	// Compute loop time without lookup
	rdtsc(t0);
	for (size_t k = 0; k < N; k++) {
	    for (size_t i = 0; i < n; i++) {
		A(random());
	    }
	}
	rdtsc(t1);
	sum_ticks -= t1 - t0;
	avg = double(sum_ticks) / double(n * N);

	cout << endl;
	cout << "=== WARM CACHE GROUP MEASUREMENTS ===" << endl;
	cout << "Avg ticks = " << avg << endl;
	cout << "Effective LPS = " << setw(15)
	     << (tps() / avg) << endl;

	//
	// COLD CACHE GROUP MEASUREMENTS
	//
	rdtsc(t0);
	for (size_t k = 0; k < N; k++) {
	    for (size_t i = 0; i < n; i++) {
		ck.kill();
		A addr(random());
		_e.lookup(addr);
	    }
	}
	rdtsc(t1);
	sum_ticks = t1 - t0;

	// Compute loop time without lookup
	rdtsc(t0);
	for (size_t k = 0; k < N; k++) {
	    for (size_t i = 0; i < n; i++) {
		ck.kill();
		A(random());
	    }
	}
	rdtsc(t1);
	uint64_t sum_ticks2 = t1 - t0;

	cout << endl;
	cout << "=== COLD CACHE GROUP MEASUREMENTS ===" << endl;
	if (sum_ticks2 >= sum_ticks) {
	    // System activity caused non-lookup loop to take longer than
	    // lookup lookup loop.
	    cout << "Run without lookups took more time than run with lookups";
	    cout << endl;
	}
	sum_ticks -= sum_ticks2;
	avg = double(sum_ticks) / double(n * N);

	cout << "Avg ticks = " << avg << endl;
	cout << "Effective LPS = " << setw(15)
	     << (tps() / avg) << endl;
    }

protected:
    Compiler 	_c;
    Engine 	_e;
    EngineData 	_d;
};


// ----------------------------------------------------------------------------
// Base for factory classes for producing TestEngineCompiler instances.
template <typename A>
struct TestEngineCompilerFactoryBase
{
public:
    TestEngineCompilerFactoryBase(const string& n) : _name(n)
    {
    }
    virtual ~TestEngineCompilerFactoryBase() {}

    inline const string& name() const { return _name; }

    virtual TestEngineCompilerBase<A>*
    make_test_instance(const char* config) = 0;

private:
    string _name;
};


// ----------------------------------------------------------------------------
// Factory class template for producing TestEngineCompiler instances.
template <typename C, typename E, typename D>
struct TestEngineCompilerFactory
    : public TestEngineCompilerFactoryBase<typename C::AddrType>
{
public:
    typedef typename C::AddrType	A;

public:
    TestEngineCompilerFactory()
	: TestEngineCompilerFactoryBase<A>(C::name())
    {
    }

    TestEngineCompilerBase<A>* make_test_instance(const char* config)
    {
	return new TestEngineCompiler<C,E,D>(config);
    }
};


// ----------------------------------------------------------------------------
// Miscellany

static void
usage()
{
    cerr << "Usage: test_lookup -f <datafile> -t <lookup_type> "
	    "[-c <lookup_config>]" << endl;
    cerr << "Use -l to list available lookup types." << endl;
    exit(1);
}


// ----------------------------------------------------------------------------
// The hallowed land
int
main(int argc, char * const argv[])
{
    vector<TestEngineCompilerFactoryBase<IPv4>*> factories;
    factories.push_back(
	new TestEngineCompilerFactory<
		BruteLookup::Compiler<IPv4, uint8_t>,
		BruteLookup::Engine<IPv4, uint8_t>,
		BruteLookup::EngineData<IPv4, uint8_t>
		>()
	);
    factories.push_back(
	new TestEngineCompilerFactory<
		LinearLookup::Compiler<IPv4, uint8_t>,
		LinearLookup::Engine<IPv4, uint8_t>,
		LinearLookup::EngineData<IPv4, uint8_t>
		>()
	);
    factories.push_back(
	new TestEngineCompilerFactory<
		PrefixTableLookup::Compiler<IPv4, uint8_t>,
		PrefixTableLookup::Engine<IPv4, uint8_t>,
		PrefixTableLookup::EngineData<IPv4, uint8_t>
		>()
	);
    factories.push_back(
	new TestEngineCompilerFactory<
		XorpTrieLookup::Compiler<IPv4, uint8_t>,
		XorpTrieLookup::Engine<IPv4, uint8_t>,
		XorpTrieLookup::EngineData<IPv4, uint8_t>
		>()
	);
    factories.push_back(
	new TestEngineCompilerFactory<
		kAryLookup::Compiler<IPv4, uint16_t>,
		kAryLookup::Engine<IPv4, uint16_t>,
		kAryLookup::EngineData<IPv4, uint16_t>
		>()
	);
    factories.push_back(
	new TestEngineCompilerFactory<
		kAry2Lookup::Compiler<IPv4, uint16_t>,
		kAry2Lookup::Engine<IPv4, uint16_t>,
		kAry2Lookup::EngineData<IPv4, uint16_t>
		>()
	);
    factories.push_back(
	new TestEngineCompilerFactory<
		kAryCompressedLookup::Compiler<IPv4, uint16_t>,
		kAryCompressedLookup::Engine<IPv4, uint16_t>,
		kAryCompressedLookup::EngineData<IPv4, uint16_t>
		>()
	);

    //
    // List of <Factory, config args> that we fill in parsing command line.
    //
    list<pair<TestEngineCompilerFactoryBase<IPv4>*,const char*> > suts;

    const char* datafile = "/dev/stdin";
    int ch;

    while ((ch = getopt(argc, argv, "f:lt:c:")) != -1) {
	switch (ch) {
	case 'f':
	    datafile = optarg;
	    break;
	case 'l':
	    cerr << "Available lookup algorithms are:" << endl;
	    for (size_t i = 0; i < factories.size(); ++i) {
		cerr << "\t" << factories[i]->name() << endl;
	    }
	    usage();
	    break;
	case 't':
	    for (size_t i = 0; i < factories.size(); ++i) {
		if (factories[i]->name() == optarg) {
		    suts.push_back(make_pair(factories[i],
					     (const char*)0));
		    break;
		}
	    }
	    break;
	case 'c':
	    if (suts.empty()) {
		cerr << "Configuration must follow a named test (-t <test>)"
		     << endl;
		usage();
		break;
	    }
	    suts.back().second = optarg;
	    break;
	default:
	    usage();
	}
    }

    //
    // Route estimate of ticks per second.
    //
    callibrate_tps();
    cout << "# Ticks per second " << tps() << endl;

    //
    // Read in routes from stdin
    //
    vector<RouteEntry<IPv4> > routes;

    ifstream fin(datafile);
    if (!fin) {
	cerr << "Could not open datafile " << datafile << endl;
	exit(-1);
    }

    string l;
    uint8_t port = 0;
    while (fin >> l) {
	try {
	    IPNet<IPv4> net(l.c_str());
	    port = (port + 1) % MAX_PORT;
	    routes.push_back(RouteEntry<IPv4>(net, port));
	} catch (...) {
	    cout << "Whoops!" << endl;
	}
    }
    fin.close();

    while (suts.empty() == false) {
	TestEngineCompilerBase<IPv4>* tec =
	    suts.front().first->make_test_instance(suts.front().second);

	tec->add_routes(routes);
	tec->compile_and_push();
	tec->validate_engine(routes);
	tec->benchmark(routes);

	delete tec;
	suts.pop_front();
    }

    for (size_t i = 0; i < factories.size(); ++i) {
	delete factories[i];
	factories[i] = 0;
    }

    return 0;
}
