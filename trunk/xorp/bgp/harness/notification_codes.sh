#!/usr/bin/env bash

#
# $XORP$
#

#
# BGP Notification error codes and subcodes.
#

MSGHEADERERR=1		# Message Header Error
    CONNNOTSYNC=1	# Connection Not Synchronized
    BADMESSLEN=2	# Bad Message Length
    BADMESSTYPE=3	# Bad Message Type

OPENMSGERROR=2		# OPEN Message Error
    UNSUPVERNUM=1	# Unsupported Version Number
    BADASPEER=2		# Bad Peer AS
    BADBGPIDENT=3	# Bad BGP Identifier
    UNSUPOPTPAR=4	# Unsupported Optional Parameter
    AUTHFAIL=5		# Authentication Failure
    UNACCEPTHOLDTIME=6	# Unacceptable Hold Time
    UNSUPCAPABILITY=7	# Unsupported Capability (RFC 3392)

UPDATEMSGERR=3		# Update error
    MALATTRLIST=1       # Malformed Attribute List
    MALASPATH=11	# Malformed AS_PATH
    MISSWATTR=3		# Missing Well-known Attribute

HOLDTIMEEXP=4		# Hold Timer Expired
FSMERROR=5		# Finite State Machine Error
CEASE=6			# Cease
