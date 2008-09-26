// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rtrmgr/template_tree_node.hh,v 1.52 2008/07/23 05:11:44 pavlin Exp $

#ifndef __RTRMGR_TEMPLATE_TREE_NODE_HH__
#define __RTRMGR_TEMPLATE_TREE_NODE_HH__


#include <map>
#include <list>
#include <set>
#include <vector>

#include "libxorp/mac.hh"

#include "config_operators.hh"
#include "rtrmgr_error.hh"
#include "xorp_client.hh"
#include "xrldb.hh"


enum TTNodeType {
    NODE_VOID		= 0,
    NODE_TEXT		= 1,
    NODE_UINT		= 2,
    NODE_INT		= 3,
    NODE_BOOL		= 4,
    NODE_TOGGLE		= 4,
    NODE_IPV4		= 5,
    NODE_IPV4NET	= 6,
    NODE_IPV6		= 7,
    NODE_IPV6NET	= 8,
    NODE_MACADDR	= 9,
    NODE_URL_FILE	= 10,
    NODE_URL_FTP	= 11,
    NODE_URL_HTTP	= 12,
    NODE_URL_TFTP	= 13,
    NODE_ARITH		= 14,
    NODE_UINTRANGE	= 15,
    NODE_IPV4RANGE	= 16,
    NODE_IPV6RANGE	= 17
};

enum TTSortOrder {
    ORDER_UNSORTED,
    ORDER_SORTED_NUMERIC,
    ORDER_SORTED_ALPHABETIC
};

class BaseCommand;
class CommandTree;
class ConfigTreeNode;
class TemplateTree;

class TemplateTreeNode {
public:
    TemplateTreeNode(TemplateTree& template_tree, TemplateTreeNode* parent, 
		     const string& path, const string& varname);
    virtual ~TemplateTreeNode();

    bool expand_template_tree(string& error_msg);
    bool check_template_tree(string& error_msg) const;

    virtual TTNodeType type() const { return NODE_VOID; }
    void add_cmd(const string& cmd) throw (ParseError);
    void add_action(const string& cmd, const list<string>& action_list)
	throw (ParseError);

    map<string, string> create_variable_map(const list<string>& segments) const;

    virtual string str() const;
    virtual string typestr() const { return string("void"); }
    virtual string default_str() const { return string(""); }
    virtual string encoded_typestr() const;
    virtual bool type_match(const string& s, string& error_msg) const;
    BaseCommand* command(const string& cmd_name);
    const BaseCommand* const_command(const string& cmd_name) const;
    set<string> commands() const;
    string varname() const { return _varname; }
    void set_tag() { _is_tag = true; }
    bool is_tag() const { return _is_tag; }
    string subtree_str() const;
    TemplateTreeNode* parent() const { return _parent; }
    const list<TemplateTreeNode*>& children() const { return _children; }
    const string& module_name() const { return _module_name; }
    const string& default_target_name() const { return _default_target_name; }
    void set_subtree_module_name(const string& module_name);
    void set_subtree_default_target_name(const string& default_target_name);
    const string& segname() const { return _segname; }
    string path() const;
    bool is_module_root_node() const;
    bool is_leaf_value() const;

    list<ConfigOperator> allowed_operators() const;

#if 0
    bool check_template_tree(string& error_msg) const;
#endif
    bool check_command_tree(const list<string>& commands, 
			    bool include_intermediate_nodes,
			    bool include_read_only_nodes,
			    bool include_permanent_nodes,
			    bool include_user_hidden_nodes,
			    size_t depth) const;
    bool has_default() const { return _has_default; }
    bool check_variable_name(const vector<string>& parts, size_t part) const;
    string get_module_name_by_variable(const string& varname) const;
    string get_default_target_name_by_variable(const string& varname) const;
    bool expand_variable(const string& varname, string& value,
			 bool ignore_deleted_nodes) const;
    bool expand_expression(const string& expression, string& value) const;
    TemplateTreeNode* find_varname_node(const string& varname);
    const TemplateTreeNode* find_const_varname_node(const string& varname) const;

    bool check_allowed_value(const string& value, string& error_msg) const;
    bool verify_variables(const ConfigTreeNode& ctn, string& error_msg) const;

    const list<string>& mandatory_config_nodes() const { return _mandatory_config_nodes; }
    const string& help() const;
    const string& help_long() const;

    int child_number() const { return _child_number;}

    TTSortOrder order() const { return _order; }
    void set_order(TTSortOrder o) { _order = o; }

    bool is_deprecated() const { return _is_deprecated; }
    void set_deprecated(bool v) { _is_deprecated = v; }
    const string& deprecated_reason() const { return _deprecated_reason; }
    void set_deprecated_reason(const string& v) { _deprecated_reason = v; }
    bool is_user_hidden() const { return _is_user_hidden; }
    void set_user_hidden(bool v) { _is_user_hidden = v; }
    const string& user_hidden_reason() const { return _user_hidden_reason; }
    void set_user_hidden_reason(const string& v) { _user_hidden_reason = v; }
    bool is_mandatory() const { return _is_mandatory; }
    void set_mandatory(bool v) { _is_mandatory = v; }
    bool is_read_only() const { return _is_read_only; }
    const string& read_only_reason() const { return _read_only_reason; }
    bool is_permanent() const { return _is_permanent; }
    const string& permanent_reason() const { return _permanent_reason; }

    /**
     * @return the oldest deprecated ancestor or NULL if no ancestor
     * is deprecated.
     */
    const TemplateTreeNode* find_first_deprecated_ancestor() const;

    /**
     * @return the oldest user-hidden ancestor or NULL if no ancestor
     * is user-hidden.
     */
    const TemplateTreeNode* find_first_user_hidden_ancestor() const;

    void add_allowed_value(const string& value, const string& help);
    void add_allowed_range(int64_t lower_value, int64_t upper_value,
			   const string& help);
    const map<string, string>& allowed_values() const { return _allowed_values; }
    const map<pair<int64_t, int64_t>, string>& allowed_ranges() const {
	return _allowed_ranges;
    }

protected:
    void add_child(TemplateTreeNode* child);

    string strip_quotes(const string& s) const;
    void set_has_default() { _has_default = true; }
    bool name_is_variable() const;

    map<string, BaseCommand *> _cmd_map;
    TemplateTreeNode*	_parent;
    list<TemplateTreeNode*> _children;

private:
    bool split_up_varname(const string& varname,
			  list<string>& var_parts) const;
    TemplateTreeNode* find_parent_varname_node(const list<string>& var_parts);
    TemplateTreeNode* find_child_varname_node(const list<string>& var_parts);

    TemplateTree&	_template_tree;

    string _module_name;
    string _default_target_name;
    string _segname;

    // If this node has a variable name associated with it, _varname is
    // where its stored. Otherwise it is an empty string.
    string _varname;
    map<string, string>	_allowed_values; // Allowed values for this template
    map<pair<int64_t, int64_t>, string> _allowed_ranges; // Allowed int ranges

    // Does the node have a default value?
    bool _has_default;

    // Is the node to be regarded as a tag for it's children rather
    // than a true tree node.
    bool _is_tag;

    // The CLI help information associated with this node.
    string _help;
    string _help_long;

    list<string>	_mandatory_config_nodes;

    int _child_number;

    TTSortOrder _order;

    bool		_verbose;	  // Set to true if output is verbose
    bool		_is_deprecated;   // True if node's usage is deprecated
    string		_deprecated_reason;  // The reason for deprecation
    bool		_is_user_hidden;     // True if hidden from the user
    string		_user_hidden_reason; // The reason for user-hidden
    bool		_is_mandatory;	     // True if this node is mandatory
    bool		_is_read_only;	     // True if a read-only node
    string		_read_only_reason;   // The reason for read-only
    bool		_is_permanent;	     // True if a permanent node
    string		_permanent_reason;   // The reason for permanent
};

class UIntTemplate : public TemplateTreeNode {
public:
    UIntTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);

    string typestr() const { return string("uint"); }
    TTNodeType type() const { return NODE_UINT; }
    unsigned int default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    unsigned int _default;
};

class UIntRangeTemplate : public TemplateTreeNode {
public:
    UIntRangeTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);
    ~UIntRangeTemplate();

    string typestr() const { return string("uintrange"); }
    TTNodeType type() const { return NODE_UINTRANGE; }
    U32Range* default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    U32Range* _default;
};

class IntTemplate : public TemplateTreeNode {
public:
    IntTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		const string& path, const string& varname, 
		const string& initializer) throw (ParseError);
    string typestr() const { return string("int"); }
    TTNodeType type() const { return NODE_INT; }
    int default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    int _default;
};

class ArithTemplate : public TemplateTreeNode {
public:
    ArithTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		  const string& path, const string& varname, 
		  const string& initializer) throw (ParseError);
    string typestr() const { return string("uint"); }
    TTNodeType type() const { return NODE_ARITH; }
    string default_value() const { return _default; }
    string default_str() const { return _default; };
    bool type_match(const string& s, string& error_msg) const;

private:
    string _default;
};

class TextTemplate : public TemplateTreeNode {
public:
    TextTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);

    string typestr() const { return string("text"); }
    TTNodeType type() const { return NODE_TEXT; }
    string default_value() const { return _default; }
    string default_str() const { return _default; }
    bool type_match(const string& s, string& error_msg) const;

private:
    string _default;
};

class BoolTemplate : public TemplateTreeNode {
public:
    BoolTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);

    string typestr() const { return string("bool"); }
    TTNodeType type() const { return NODE_BOOL; }
    bool default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    bool _default;
};

class IPv4Template : public TemplateTreeNode {
public:
    IPv4Template(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname,
		 const string& initializer) throw (ParseError);
    ~IPv4Template();

    string typestr() const { return string("IPv4"); }
    TTNodeType type() const { return NODE_IPV4; }
    IPv4 default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    IPv4* _default;
};

class IPv4NetTemplate : public TemplateTreeNode {
public:
    IPv4NetTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~IPv4NetTemplate();

    string typestr() const { return string("IPv4Net"); }
    TTNodeType type() const { return NODE_IPV4NET; }
    IPv4Net default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    IPv4Net* _default;
};

class IPv4RangeTemplate : public TemplateTreeNode {
public:
    IPv4RangeTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~IPv4RangeTemplate();

    string typestr() const { return string("IPv4Range"); }
    TTNodeType type() const { return NODE_IPV4RANGE; }
    IPv4Range default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    IPv4Range* _default;
};

class IPv6Template : public TemplateTreeNode {
public:
    IPv6Template(TemplateTree& template_tree, TemplateTreeNode* parent,
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);
    ~IPv6Template();

    string typestr() const { return string("IPv6"); }
    TTNodeType type() const { return NODE_IPV6; }
    IPv6 default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    IPv6* _default;
};

class IPv6NetTemplate : public TemplateTreeNode {
public:
    IPv6NetTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~IPv6NetTemplate();

    string typestr() const { return string("IPv6Net"); }
    TTNodeType type() const { return NODE_IPV6NET; }
    IPv6Net default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    IPv6Net* _default;
};

class IPv6RangeTemplate : public TemplateTreeNode {
public:
    IPv6RangeTemplate(TemplateTree& template_tree, TemplateTreeNode* parent,
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);
    ~IPv6RangeTemplate();

    string typestr() const { return string("IPv6Range"); }
    TTNodeType type() const { return NODE_IPV6RANGE; }
    IPv6Range default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    IPv6Range* _default;
};

class MacaddrTemplate : public TemplateTreeNode {
public:
    MacaddrTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~MacaddrTemplate();

    string typestr() const { return string("macaddr"); }
    TTNodeType type() const { return NODE_MACADDR; }
    Mac default_value() const { return *_default; }
    string default_str() const;
    bool type_match(const string& s, string& error_msg) const;

private:
    Mac* _default;
};

class UrlFileTemplate : public TemplateTreeNode {
public:
    UrlFileTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname,
		    const string& initializer) throw (ParseError);

    string typestr() const { return string("URL_FILE"); }
    TTNodeType type() const { return NODE_URL_FILE; }
    string default_value() const { return _default; }
    string default_str() const { return _default; }
    bool type_match(const string& s, string& error_msg) const;

private:
    string _default;
};

class UrlFtpTemplate : public TemplateTreeNode {
public:
    UrlFtpTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		   const string& path, const string& varname,
		   const string& initializer) throw (ParseError);

    string typestr() const { return string("URL_FTP"); }
    TTNodeType type() const { return NODE_URL_FTP; }
    string default_value() const { return _default; }
    string default_str() const { return _default; }
    bool type_match(const string& s, string& error_msg) const;

private:
    string _default;
};

class UrlHttpTemplate : public TemplateTreeNode {
public:
    UrlHttpTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname,
		    const string& initializer) throw (ParseError);

    string typestr() const { return string("URL_HTTP"); }
    TTNodeType type() const { return NODE_URL_HTTP; }
    string default_value() const { return _default; }
    string default_str() const { return _default; }
    bool type_match(const string& s, string& error_msg) const;

private:
    string _default;
};

class UrlTftpTemplate : public TemplateTreeNode {
public:
    UrlTftpTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname,
		    const string& initializer) throw (ParseError);

    string typestr() const { return string("URL_TFTP"); }
    TTNodeType type() const { return NODE_URL_TFTP; }
    string default_value() const { return _default; }
    string default_str() const { return _default; }
    bool type_match(const string& s, string& error_msg) const;

private:
    string _default;
};

#endif // __RTRMGR_TEMPLATE_TREE_NODE_HH__
