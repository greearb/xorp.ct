/*
 * This file contains models that the IP Lookup Algorithm
 * implementations in this directory adhere to.  There are three data
 * structures in the model:
 *
 * Compiler	stores live routes and produces a "compiled" routing table
 *		on demand.
 *
 * Engine	supports route lookup on a "compiled" routing table.
 *
 * EngineData	represents a "compiled" routing table.
 */

#ifndef __LOOKUP_MODEL_HH__
#define __LOOKUP_MODEL_HH__

#include "libxorp/ipnet.hh"

namespace IpLookup {
    /**
     * Model for EngineData.  EngineData is produced by the @ref
     * compiler and consumed by the @ref Engine.
     */
    template <typename A, typename P>
    struct EngineData {
	/**
	 * Return size of all data associated with instance.
	 */
	size_t bytes() const;
    };

    /**
     * Model of class for building lookup data structures.  Instances
     * are expected to maintain a data structure of live routes and
     * produce EngineData when compile() is called for a lookup @ref
     * Engine to use for route lookups.
     */
    template <typename A, typename P>
    class Compiler {
    public:
	typedef A AddrType;
	typedef P PortType;

	static const P NO_PORT = ~0;
	static const P MAX_PORT = NO_PORT - 1;

    public:
	/**
	 * Constructor.  Should optionally take a string containing
	 * user settings.
	 */
	Compiler(const char* settings = 0);
	void ~Compiler() = 0;

	/**
	 * Static member to return name of compiler.
	 */
	static const char* name();

	/**
	 * Add route.  Adds route to table, and overwrites existing
	 * resolution if it exists.
	 *
	 * @return true on success, false if operation is not supported or
	 * if port is greater than MAX_PORT.
	 */
	bool add_route(const IPNet<A>& net, P port);

	/**
	 * Remove route.
	 *
	 * @return true on success, false if operation is not supported or
	 * no route exists.
	 */
	bool remove_route(const IPNet<A>& net);

	/**
	 * Compile routes into lookup data structure and propagate to
	 * associated Engine.
	 *
	 * @param e engine data to compile routes into.
	 *
	 * @return true on success, false if a problem occurred during build.
	 */
	bool compile(LookupIpEngineData& e);

    };

    /**
     * Model class for querying lookup data structures.
     */
    template <typename A, typename P>
    class Engine {
    public:
	typedef A AddrType;
	typedef P PortType;

    public:
	/**
	 * Set data structure for engine use in lookups.
	 */
	void set_engine_data(const EngineData* e);

	/**
	 * Get data structure engine is using for lookups.
	 */
	const EngineData* engine_data();

	/**
	 * Lookup route for address.
	 *
	 * @return port associated with route on success, NO_PORT on
	 * failure.
	 */
	P lookup(const A& addr) = 0;

    protected:
	const EngineData* _ed;
    };

};

#endif /* __LOOKUP_MODEL_HH__ */
