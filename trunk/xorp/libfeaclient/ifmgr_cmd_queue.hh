// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __IFMGR_CMD_QUEUE_HH__
#define __IFMGR_CMD_QUEUE_HH__

#include <list>
#include "libxorp/ref_ptr.hh"

class IfMgrCommandBase;

class IfMgrCommandSinkBase {
public:
    typedef ref_ptr<IfMgrCommandBase> Cmd;
public:
    virtual void push(const Cmd& cmd) = 0;
    virtual ~IfMgrCommandSinkBase();
};

class IfMgrCommandTee : public IfMgrCommandSinkBase {
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

public:
    IfMgrCommandTee(IfMgrCommandSinkBase& o1, IfMgrCommandSinkBase& o2);
    void push(const Cmd& cmd);
protected:
    IfMgrCommandSinkBase& _o1;
    IfMgrCommandSinkBase& _o2;
};

/**
 * @short Base class for Command Queue classes.
 */
class IfMgrCommandQueueBase : public IfMgrCommandSinkBase {
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;
public:
    /**
     * Add an item to the queue.
     */
    virtual void push(const Cmd& cmd) = 0;

    /**
     * @return true if queue has no items, false otherwise.
     */
    virtual bool empty() const = 0;

    /**
     * Accessor for front item from queue.
     *
     * @return reference to front item if queue is not empty, junk otherwise.
     */
    virtual Cmd& front() = 0;

    /**
     * Accessor for front item from queue.
     *
     * @return reference to front item if queue is not empty, junk otherwise.
     */
    virtual const Cmd& front() const = 0;

    /**
     * Pop the front item from queue.
     */
    virtual void pop_front() = 0;
};

/**
 * @short FIFO Queue for command objects.
 */
class IfMgrCommandFifoQueue : public IfMgrCommandQueueBase {
public:
    typedef IfMgrCommandQueueBase::Cmd Cmd;

public:
    void 	push(const Cmd& cmd);
    bool	empty() const;
    Cmd&	front();
    const Cmd&	front() const;
    void	pop_front();

protected:
    list<Cmd> _fifo;
};

/**
 * @short Interface Command Clustering Queue.
 *
 * This Queue attempts to cluster commands based on their interface
 * name.  Only command objects derived from IfMgrIfCommandBase may be
 * placed in the queue, command objects not falling in this category will
 * give rise to assertion failures.
 */
class IfMgrCommandIfClusteringQueue : public IfMgrCommandQueueBase {
public:
    typedef IfMgrCommandQueueBase::Cmd Cmd;
    typedef list<Cmd> CmdList;

public:
    IfMgrCommandIfClusteringQueue();

    void 	push(const Cmd& cmd);
    bool	empty() const;
    Cmd&	front();
    const Cmd&	front() const;
    void	pop_front();

protected:
    void	change_active_interface();

protected:
    string		_current_ifname;	// ifname of commands current
    						// commands queue
    						// last command output
    CmdList		_future_cmds;		// commands with ifname not
						// matching _current_ifname
    CmdList		_current_cmds;
};


/**
 * @short Class to convert an IfMgrIfTree object into a sequence of commands.
 */
class IfMgrIfTreeToCommands {
public:
    /**
     * Constructor
     */
    inline IfMgrIfTreeToCommands(const IfMgrIfTree& tree)
	: _tree(tree)
    {}

    /**
     * Convert the entire contents of IfMgrIfTree object to a sequence of
     * configuration commands.
     *
     * @param sink output target for commands that would generate tree.
     */
    void convert(IfMgrCommandSinkBase& sink) const;

protected:
    const IfMgrIfTree& _tree;
};

/**
 * @short Class to convert an IfMgrIfAtom object into a sequence of commands.
 */
class IfMgrIfAtomToCommands {
public:
    /**
     * Constructor
     */
    inline IfMgrIfAtomToCommands(const IfMgrIfAtom& interface)
	: _i(interface)
    {}

    /**
     * Convert the entire contents of IfMgrIfTree object to a sequence of
     * configuration commands.
     *
     * @param sink output target for commands that would generate
     * interface state.
     */
    void convert(IfMgrCommandSinkBase& sink) const;

protected:
    const IfMgrIfAtom& _i;
};

/**
 * @short Class to convert an IfMgrVifAtom object into a sequence of commands.
 */
class IfMgrVifAtomToCommands {
public:
    /**
     * Constructor
     */
    inline IfMgrVifAtomToCommands(const IfMgrVifAtom& vif)
	: _v(vif)
    {}

    /**
     * Convert the entire contents of IfMgrIfTree object to a sequence of
     * configuration commands.
     *
     * @param sink output target for commands that would generate
     * virtual interface state.
     */
    void convert(IfMgrCommandSinkBase& sink) const;

protected:
    const IfMgrVifAtom& _v;
};

/**
 * @short Class to convert an IfMgrIPv4Atom object into a sequence of commands.
 */
class IfMgrIPv4AtomToCommands {
public:
    /**
     * Constructor
     */
    inline IfMgrIPv4AtomToCommands(const IfMgrIPv4Atom& a)
	: _a(a)
    {}

    /**
     * Convert the entire contents of IfMgrIfTree object to a sequence of
     * configuration commands.
     *
     * @param sink output target for commands that would generate
     * interface state.
     */
    void convert(IfMgrCommandSinkBase& sink) const;

protected:
    const IfMgrIPv4Atom& _a;
};

/**
 * @short Class to convert an IfMgrIPv6Atom object into a sequence of commands.
 */
class IfMgrIPv6AtomToCommands {
public:
    /**
     * Constructor
     */
    inline IfMgrIPv6AtomToCommands(const IfMgrIPv6Atom& a)
	: _a(a)
    {}

    /**
     * Convert the entire contents of IfMgrIfTree object to a sequence of
     * configuration commands.
     *
     * @param sink output target for commands that would generate
     * interface state.
     */
    void convert(IfMgrCommandSinkBase& sink) const;

protected:
    const IfMgrIPv6Atom& _a;
};

#endif // __IFMGR_CMD_QUEUE_HH__
