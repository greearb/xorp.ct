#ifndef __LOOKUP_XORP_TRIE_HH__
#define __LOOKUP_XORP_TRIE_HH__

#include "libxorp/trie.hh"

namespace XorpTrieLookup {

    template <typename A, typename P>
    struct EngineData {
	Trie<A,P> trie;

	size_t bytes() const
	{
	    // Does not include empty nodes in Trie :-(
	    size_t s = sizeof(*this);
	    s += trie.route_count() * sizeof(typename Trie<A,P>::Node);
	    return s;
	}
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

	static const char* name()		{ return "xorptrie"; }

	inline bool add_route(const IPNet<A>& net, P p) {
	    //	remove_route(net);
	    _t.insert(net, p);
	    return true;
	}

	inline bool remove_route(const IPNet<A>& net) {
	    _t.erase(net);
	    return true;
	}

	inline bool compile(EngineData<A,P>& ed) {
	    ed.trie.delete_all_nodes();
	    typename Trie<A,P>::iterator i = _t.begin();
	    for (i = _t.begin(); i != _t.end(); ++i) {
		if (i.has_payload()) {
		    ed.trie.insert(i.key(), i.payload());
		}
	    }

	    return true;
	}

    protected:
	Trie<A,P> _t;
    };

    template <typename A, typename P>
    class Engine {
    public:
	typedef A AddrType;
	typedef P PortType;

	static const P NO_PORT  = Compiler<A,P>::NO_PORT;
	static const P MAX_PORT = Compiler<A,P>::MAX_PORT;

    public:
	Engine() {}

	void set_engine_data(const EngineData<A,P>* ned)	{ _ed = ned; }
	const EngineData<A,P>* engine_data()			{ return _ed; }

	inline P lookup(const A& addr) {
	    typename Trie<A,P>::iterator i = _ed->trie.find(addr);
	    if (i == _ed->trie.end()) {
		return NO_PORT;
	    }
	    assert(i.cur() != 0);
	    return i.payload();
	}

    protected:
	const EngineData<A,P>* _ed;
    };

}; // XorpTrieLookup -- end of namepsace

#endif /* __LOOKUP_XORP_TRIE_HH__ */
