#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/notification_codes.sh,v 1.1 2005/12/11 20:33:07 atanu Exp $
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
    UNRECOGWATTR=2	# Unrecognized Well-known Attribute
    MISSWATTR=3		# Missing Well-known Attribute
    ATTRFLAGS=4		# Attribute Flags Error
    ATTRLEN=5		# Attribute Length Error
    INVALORGATTR=6	# Invalid ORIGIN Attribute

    INVALNHATTR=8	# Invalid NEXT_HOP Attribute
    OPTATTR=9		# Optional Attribute Error
    INVALNETFIELD=10	# Invalid Network Field
    MALASPATH=11	# Malformed AS_PATH

HOLDTIMEEXP=4		# Hold Timer Expired
FSMERROR=5		# Finite State Machine Error
CEASE=6			# Cease
