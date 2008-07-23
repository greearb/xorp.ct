// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/source_match_code_generator.hh,v 1.10 2008/01/04 03:17:12 pavlin Exp $

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
    // bool == tag used
    // uint32_t actual tag
    typedef pair<bool, uint32_t> Taginfo;
    typedef vector<Taginfo> Tags;

    /**
     * @short Exception thrown if no protocol was specified in source block.
     */
    class NoProtoSpec : public PolicyException {
    public:
        NoProtoSpec(const char* file, size_t line, const string& init_why = "")
            : PolicyException("NoProtoSpec", file, line, init_why) {} 
    };

    /**
     * @short Exception thrown if protocol was re-defined in source block.
     */
    class ProtoRedefined : public PolicyException {
    public:
        ProtoRedefined(const char* file, size_t line, const string& init_why = "")
            : PolicyException("ProtoRedefined", file, line, init_why) {} 
    };


    /**
     * @param tagstart the first policy tag available.
     * @param varmap the varmap.
     */
    SourceMatchCodeGenerator(uint32_t tagstart, const VarMap& varmap);
    
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

    /**
     * The source match code generator will map source blocks to tags.  If a
     * source block is empty, a tag will not be used.
     *
     * @return information about tags used.
     */
    const Tags& tags() const;

    /**
     * @return The next available policy tag.
     *
     */
    uint32_t next_tag() const;

protected:
    const string& protocol();

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
    
    Tags _tags;
    map<string, set<uint32_t> > _protocol_tags;
    bool _protocol_statement;

    // not impl
    SourceMatchCodeGenerator(const SourceMatchCodeGenerator&);
    SourceMatchCodeGenerator& operator=(const SourceMatchCodeGenerator&);
};

#endif // __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__
