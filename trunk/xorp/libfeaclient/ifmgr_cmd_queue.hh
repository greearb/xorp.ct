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

// $XORP: xorp/libfeaclient/ifmgr_cmd_queue.hh,v 1.3 2003/08/26 19:04:39 hodson Exp $

#ifndef __IFMGR_CMD_QUEUE_HH__
#define __IFMGR_CMD_QUEUE_HH__

#include <list>
#include "libxorp/ref_ptr.hh"

class IfMgrCommandBase;

class IfMgrIfTree;
class IfMgrIfAtom;
class IfMgrVifAtom;
class IfMgrIPv4Atom;
class IfMgrIPv6Atom;

class IfMgrCommandSinkBase {
public:
    typedef ref_ptr<IfMgrCommandBase> Cmd;
public:
    virtual void push(const Cmd& cmd) = 0;
    virtual ~IfMgrCommandSinkBase();
};

/**
 * @short 2-way IfMgr Command Tee class.  Instances push commands
 * pushed into them into two object derived from @ref IfMgrCommandSinkBase.
 */
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
 * @short 2-way IfMgr Command Tee class.  Instances push commands
 * pushed into them into two object derived from @ref IfMgrCommandSinkBase.
 */
class IfMgrNWayCommandTee : public IfMgrCommandSinkBase {
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

public:
    void push(const Cmd& cmd);

    /**
     * Add an additional output for pushed commands.
     * @param output receiver for commands pushed into instance.
     * @return true if output is successfully added, false otherwise.
     */
    bool add_output(IfMgrCommandSinkBase* output);

    /**
     * Remove an output for pushed commands.
     * @param output receiver for commands pushed into instance.
     * @return true if output is successfully remove, false otherwise.
     */
    bool remove_output(IfMgrCommandSinkBase* output);

protected:
    typedef list<IfMgrCommandSinkBase*> OutputList;
    OutputList _outputs;
};

/**
 * @short Class to dispatch Interface Manager Commands.
 *
 * This class buffers exactly one Interface Manager Command (@ref
 * IfMgrCommandBase) and applies it to an Interface Manager
 * Configuration Tree (@ref IfMgrIfTree) when it's execute() method is
 * called.
 */
class IfMgrCommandDispatcher : public IfMgrCommandSinkBase {
public:
    typedef IfMgrCommandSinkBase::Cmd Cmd;

public:
    /**
     * Constructor
     * @param tree configuration tree to apply commands to.
     */
    IfMgrCommandDispatcher(IfMgrIfTree& tree);

    /**
     * Push a command into local storage ready for execution.
     */
    void push(const Cmd& cmd);

    /**
     * Execute command.
     *
     * @return command return status (true indicates success, false failure).
     */
    virtual bool execute();

protected:
    Cmd		 _cmd;
    IfMgrIfTree& _iftree;
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
     *
     * @param ifn the name of the interface the vif belongs to.
     * @param vif the vif to be converted into a sequence of commands.
     */
    inline IfMgrVifAtomToCommands(const string& ifn, const IfMgrVifAtom& vif)
	: _ifn(ifn), _v(vif)
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
    const string&	_ifn;		// Interface name
    const IfMgrVifAtom&	_v;
};

/**
 * @short Class to convert an IfMgrIPv4Atom object into a sequence of commands.
 */
class IfMgrIPv4AtomToCommands {
public:
    /**
     * Constructor
     *
     * @param ifn the name of the interface the vif owning the address
     *            belongs to.
     * @param vifn the name of the vif owning the address.
     * @param a address atom to be converted into a sequence of commands.
     */
    inline IfMgrIPv4AtomToCommands(const string& ifn,
				   const string& vifn,
				   const IfMgrIPv4Atom& a)
	: _ifn(ifn), _vifn(vifn), _a(a)
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
    const string& _ifn;
    const string& _vifn;
    const IfMgrIPv4Atom& _a;
};

/**
 * @short Class to convert an IfMgrIPv6Atom object into a sequence of commands.
 */
class IfMgrIPv6AtomToCommands {
public:
    /**
     * Constructor
     *
     * @param ifn the name of the interface the vif owning the address
     *            belongs to.
     * @param vifn the name of the vif owning the address.
     * @param a address atom to be converted into a sequence of commands.
     */
    inline IfMgrIPv6AtomToCommands(const string& ifn,
				   const string& vifn,
				   const IfMgrIPv6Atom& a)
	: _ifn(ifn), _vifn(vifn), _a(a)
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
    const string&	 _ifn;
    const string&	 _vifn;
    const IfMgrIPv6Atom& _a;
};

#endif // __IFMGR_CMD_QUEUE_HH__
