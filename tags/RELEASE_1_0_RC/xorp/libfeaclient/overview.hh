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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __LIBFEACLIENT_OVERVIEW_HH__
#define __LIBFEACLIENT_OVERVIEW_HH__

#error "This file just contains documentation and should not be included!"

/**

@libdoc libfeaclient

@sect Overview

The classes in libfeaclient exist to maintain and synchronize
interface configuration state between processes.  Their design assumes
that a single process, the FEA, is responsible for maintaining
interface configuration state.  The FEA updates the configuration
state based on information from forwarding hardware and configuration
requests from the router manager.

With libfeaclient the interface configuration information is held
within an @ref IfMgrIfTree instance.  An IfMgrIfTree instance contains
all the information associated with physical interfaces, virtual
interfaces, and addresses associated with virtual interfaces.  Within
the IfMgrIfTree class, physical interfaces are represented by @ref
IfMgrIfAtom instances, virtual interfaces by @ref IfMgrVifAtom
instances, and addresses by instances of @ref IfMgrIPv4Atom and
@ref IfMgrIPv6Atom.

An instance of the IfMgrIfTree is maintained by the FEA and by remote
processes.  In the FEA the IfMgrIfTree instance is contained with an
@ref IfMgrXrlReplicationManager and at the far end within an
@ref IfMgrXrlMirror instance.  Configuration and forwarding of state
occurs through the submission of command objects to the FEA's
IfMgrXrlReplicationManager.  The command is dispatched on the local
@ref IfMgrIfTree instance and if successful forwarded to each remote
@ref IfMgrXrlMirror instance.

<pre>
FEA                                                           Remote Process1

IfMgrCommand ---> IfMgrReplicationManager + - - - - - - - - > IfMgrXrlMirror
                          | ^             |                        | ^
		          V |             |                        V |
                      IfMgrIfTree         |                    IfMgrIfTree
                                          |
                                          |
                                          |                   Remote Process2
                                          |
					  + - - - - - - - - > IfMgrXrlMirror
		                                                   | ^
		                                                   V |
							       IfMgrIfTree
</pre>

The base class for command objects is @ref IfMgrCommandBase.  The
majority of commands are configuration commands for the objects
contained within an @ref IfMgrIfTree.  Configuration commands exist
for each attribute of each object type.  The interface exposed by @ref
IfMgrCommandBase ensures that each command implements an execute()
method and a forward() method.  The execute() method applies the
command to the local @ref IfMgrIfTree and the forward() method
forwards the command object as an Xrl to a remote target.

In addition to configuration commands, there are also "hint" commands
that pass hints to the remote @ref IfMgrXrlMirror.  Example hint
commands are @ref IfMgrHintTreeComplete, to hint that the complete
IfMgrIfTree configuration has been sent, and @ref IfMgrHintUpdatesMade,
to hint that updates have been made.  The @ref IfMgrXrlMirror will pass
hints on to objects implementing the @ref IfMgrXrlMirrorRouterObserver
interface.
 */

#endif // __LIBFEACLIENT_OVERVIEW_HH__
