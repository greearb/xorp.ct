// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

// $XORP$

#ifndef __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__
#define __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__

#include "policy/common/policy_exception.hh"
#include "code_generator.hh"

#include <vector>
#include <string>

/**
 * @short Code generator for source match filters.
 *
 * This is a specialized version of the import filter CodeGenerator.
 *
 * It skips dest and action blocks in policies. 
 * The action block is replaced with the actual policy tagging.
 */
class SourceMatchCodeGenerator : public CodeGenerator {
public:
    /**
     * @short Exception thrown if no protocol was specified in source block.
     */
    class NoProtoSpec : public PolicyException {
    public:
	NoProtoSpec(const string& x) : PolicyException(x) {}
    };

    /**
     * @short Exception thrown if protocol was re-defined in source block.
     */
    class ProtoRedefined : public PolicyException {
    public:
	ProtoRedefined(const string& x) : PolicyException(x) {}
    };


    /**
     * @param tagstart the first policy tag available.
     */
    SourceMatchCodeGenerator(uint32_t tagstart);

    
    const Element* visit_policy(PolicyStatement& policy);

    const Element* visit_term(Term& term);

    const Element* visit_proto(NodeProto& node);

    /**
     * The targets of source match code may be multiple as different protocols
     * may refer to different source terms. Thus many different code fragments
     * may be generated.
     *
     * @return seturn all the code fragments generated.
     */
    vector<Code*>& codes();

private:
    typedef map<string,Code*> CodeMap;
    
    /**
     * Adds the the code of the current term being analyzed.
     *
     * @param pname name of the policy statement
     */
    void addTerm(const string& pname);





    uint32_t _currtag;
    string _protocol;

    CodeMap _codes;

    // FIXME: who deletes these on exception ?
    vector<Code*> _codes_vect; 

    // not impl
    SourceMatchCodeGenerator(const SourceMatchCodeGenerator&);
    SourceMatchCodeGenerator& operator=(const SourceMatchCodeGenerator&);
};

#endif // __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__
