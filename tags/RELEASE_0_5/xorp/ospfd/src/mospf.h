
/* Definitions used in the MOSPF routing
 * calculations.
 */

/* The multiple possibilities for area
 * initialization. In order of priority for
 * choosing the downstream interface.
 */

enum {
    SourceIntraArea = 1,
    SourceInterArea1 = 2,
    SourceInterArea2 = 3,
    SourceExternal = 4,
    SourceStubExternal,
};

/* The possible values for incoming link type to a vertex,
 * used by the MOSPF tiebreakers invoked by the
 * multicast shortest path calculation.
 * Listed in order of decreasing preference
 */

enum {
    ILVirtual  = 1,	// virtual link
    ILDirect,		// vertex is directly attached to source
    ILNormal,		// transit link advertised in LSAs
    ILSummary,		// advertised by summary-LSA
    ILExternal,		// Advertised in AS-external-LSA
    ILNone,		// Vertex not on the shortest-path tree
};

/* Storage for a multicast cache entry.
 */

class MospfEntry : public AVLitem {
    INrte *srte;
    MCache val;
 public:
    MospfEntry(InAddr src, InAddr group);
    friend class OSPF;
};
