// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/policy/source_match_code_generator.hh,v 1.14 2008/10/02 21:58:00 bms Exp $

#ifndef __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__
#define __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__






#include "policy/common/policy_exception.hh"

#include "code_generator.hh"

/**
 * @short Code generator for source match filters.
 *
 * This is a specialized version of the import filter CodeGenerator.
 *
 * It skips dest and action blocks in policies. 
 * The action block is replaced with the actual policy tagging.
 */
class SourceMatchCodeGenerator :
    public NONCOPYABLE,
    public CodeGenerator
{
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
    SourceMatchCodeGenerator(uint32_t tagstart, const VarMap& varmap,
			     PolicyMap& pmap, 
			    map<string, set<uint32_t> >& ptags);

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

    void do_term(Term& term);

    /**
     * Adds the the code of the current term being analyzed.
     */
    void addTerm();

    uint32_t			_currtag;
    string			_protocol;
    CodeMap			_codes;
    // FIXME: who deletes these on exception ?
    vector<Code*>		_codes_vect; 
    Tags			_tags;
    map<string, set<uint32_t> >& _protocol_tags;
    bool			_protocol_statement;
    string			_policy;
};

#endif // __POLICY_SOURCE_MATCH_CODE_GENERATOR_HH__
