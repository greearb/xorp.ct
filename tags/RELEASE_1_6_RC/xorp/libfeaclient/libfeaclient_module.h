/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

/*
 * $XORP: xorp/libfeaclient/libfeaclient_module.h,v 1.11 2008/10/02 00:30:55 pavlin Exp $
 */

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

#ifndef __LIBFEACLIENT_LIBFEACLIENT_MODULE_H__
#define __LIBFEACLIENT_LIBFEACLIENT_MODULE_H__

#ifndef	XORP_MODULE_NAME
#define XORP_MODULE_NAME	"LIBFEACLIENT"
#endif
#ifndef XORP_MODULE_VERSION
#define XORP_MODULE_VERSION	"0.1"
#endif

#endif /* __LIBFEACLIENT_LIBFEACLIENT_MODULE_H__ */
