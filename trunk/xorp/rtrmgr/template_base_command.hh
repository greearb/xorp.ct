// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rtrmgr/template_commands.hh,v 1.22 2004/06/10 22:41:54 hodson Exp $

#ifndef __RTRMGR_TEMPLATE_BASE_COMMAND_HH__
#define __RTRMGR_TEMPLATE_BASE_COMMAND_HH__


#include <map>
#include <list>
#include <set>

#include "libxorp/callback.hh"
#include "rtrmgr_error.hh"

class TemplateTree;
class TemplateTreeNode;
class ConfigTreeNode;

class BaseCommand {
public:
    BaseCommand(TemplateTreeNode& template_tree_node, const string& cmd_name);
    virtual ~BaseCommand();

    virtual string str() const;
    TemplateTreeNode& template_tree_node() { return _template_tree_node; }
protected:
    string		_cmd_name;
    TemplateTreeNode&	_template_tree_node;
};

class AllowCommand : public BaseCommand {
public:
    AllowCommand(TemplateTreeNode& template_tree_node, const string& cmd_name);

    virtual void add_action(const list<string>& action) throw (ParseError) = 0;
    virtual bool verify_variable_value(const ConfigTreeNode& ctn,
				       string& errmsg) const = 0;
    virtual string str() const = 0;
};

class AllowOptionsCommand : public AllowCommand {
public:
    AllowOptionsCommand(TemplateTreeNode&	template_tree_node,
			const string&		cmd_name);

    virtual void add_action(const list<string>& action) throw (ParseError);
    virtual bool verify_variable_value(const ConfigTreeNode& 	ctn,
				       string&			errmsg) const;

    virtual string str() const;

private:
    string		_varname;
    list<string>	_allowed_values;
};

class AllowRangeCommand : public AllowCommand {
public:
    AllowRangeCommand(TemplateTreeNode&	template_tree_node,
		      const string&	cmd_name);

    virtual void add_action(const list<string>& action) throw (ParseError);
    virtual bool verify_variable_value(const ConfigTreeNode& 	ctn,
				       string&			errmsg) const;

    virtual string str() const;

private:
    string		_varname;
    int32_t		_lower;
    int32_t		_upper;
};

#endif // __RTRMGR_TEMPLATE_BASE_COMMAND_HH__

