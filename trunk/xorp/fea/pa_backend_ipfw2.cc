// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/pa_backend_ipfw2.cc,v 1.11 2007/09/15 19:52:39 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_PACKETFILTER_IPFW2
 #if __FreeBSD_version < 500000
  #define IPFW2 1
 #endif
#include <netinet/ip_fw.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"
#include "pa_backend_ipfw2.hh"

/* ------------------------------------------------------------------------- */
/* Constructor and destructor. */

PaIpfw2Backend::PaIpfw2Backend()
    throw(PaInvalidBackendException)
    : _s4(-1)
{
#ifndef HAVE_PACKETFILTER_IPFW2
    throw PaInvalidBackendException();
#else
    // Attempt to probe for IPFW2 with an operation which doesn't
    // change any of the state, and allows us to tell the presence
    // of IPFW2 apart from that of IPFW1.
    uint32_t step;
    if (get_autoinc_step(step) != XORP_OK) {
	XLOG_ERROR("IPFW2 does not appear to be present in the system.");
	throw PaInvalidBackendException();
    }

    // IPFW2 uses a private raw IPv4 socket for communication.
    // The socket options used are very specific to IPFW2, so we bypass
    // the RawSocket4Manager.
    _s4 = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (_s4 == -1) {
	XLOG_ERROR("Could not obtain a raw socket for IPFW2 communication.");
	throw PaInvalidBackendException();
    }

    // Initialize rulesets to a known state. We assume that currently
    // active ACLs for the system are present in IPFW2 ruleset 0. We
    // disable all other sets; they may still be manipulated in their
    // disabled state, but are dormant.
    // Ruleset 0 is always used as the active ruleset.
    // Ruleset 1 is always used for transcribing rules which are about
    // to be pushed to the kernel.
    // Ruleset 31 is reserved for default rules.
    // All other rulesets are used for snapshots of backend state.
    RulesetGroup enable_group, disable_group;
    enable_group.reset();
    enable_group.set(DEFAULT_RULESET);
    disable_group = enable_group;
    disable_group.flip();
    enable_disable_rulesets4(enable_group, disable_group);

    // Set autoinc step to 1, so that XORP rules being pushed require
    // no rule number book-keeping.
    set_autoinc_step(1);
#endif
}

PaIpfw2Backend::~PaIpfw2Backend()
{
#ifdef HAVE_PACKETFILTER_IPFW2
    if (_s4 != -1)
	::close(_s4);
#endif
}

/* ------------------------------------------------------------------------- */
/* General back-end methods. */

const char*
PaIpfw2Backend::get_name() const
{
    return ("ipfw2");
}

const char*
PaIpfw2Backend::get_version() const
{
    return ("0.1");
}

/* ------------------------------------------------------------------------- */
/* IPv4 ACL back-end methods. */

int
PaIpfw2Backend::push_entries4(const PaSnapshot4* snap)
{
#ifndef HAVE_PACKETFILTER_IPFW2
    UNUSED(snap);
    return (XORP_ERROR);
#else
    XLOG_ASSERT(snap != NULL);

    const PaSnapTable4& table = snap->data();
    uint32_t rulebuf[MAX_IPFW2_RULE_WORDS];
    uint32_t size_used;

    // Use TRANSCRIPT_RULESET for transcribing rules from the
    // upper layer to the lower layer. Flush it to begin with.
    flush_ruleset4(TRANSCRIPT_RULESET);

    // Transcribe all the rules contained within the snapshot.
    for (PaSnapTable4::const_iterator i = table.begin();
	 i != table.end(); i++)
    {
	transcribe_rule4(*i, TRANSCRIPT_RULESET, rulebuf, size_used);
	push_rule4(TRANSCRIPT_RULESET, rulebuf, size_used);
    }

    // Swap the default ruleset with the transcript ruleset
    // atomically when complete.
    swap_ruleset4(DEFAULT_RULESET, TRANSCRIPT_RULESET);

    return (XORP_OK);
#endif
}

int
PaIpfw2Backend::delete_all_entries4()
{
#ifndef HAVE_PACKETFILTER_IPFW2
    return (XORP_ERROR);
#else
    flush_ruleset4(DEFAULT_RULESET);
    return (XORP_OK);
#endif
}

const PaBackend::Snapshot4Base*
PaIpfw2Backend::create_snapshot4()
{
#ifndef HAVE_PACKETFILTER_IPFW2
    return (NULL);
#else
    int set;

    // Find first free ruleset number in array and allocate it.
    // Note that 0, 1, and 31 are always marked allocated.
    // If we can't find a free slot, return NULL.
    for (set = TRANSCRIPT_RULESET + 1; set < RESERVED_RULESET; set++) {
	if (_snapshot4db[set] == NULL)
	    break;
    }

    if (set == RESERVED_RULESET)
	return (NULL);

    // Use a private constructor which lets us set the set number.
    PaIpfw2Backend::Snapshot4* snap = new PaIpfw2Backend::Snapshot4(*this, set);

    // Copy the currently active ruleset.
    copy_ruleset4(DEFAULT_RULESET, set);

    // Insert it into the array, thus marking the ruleset as being allocated.
    _snapshot4db[set] = snap;

    return (snap);
#endif
}

int
PaIpfw2Backend::restore_snapshot4(const PaBackend::Snapshot4Base* snap4)
{
#ifndef HAVE_PACKETFILTER_IPFW2
    UNUSED(snap4);
    return (XORP_ERROR);
#else
    // Simply check if the snapshot is an instance of our derived snapshot.
    // If it is not, we received it in error, and it is of no interest to us.
    const PaIpfw2Backend::Snapshot4* dsnap4 =
	dynamic_cast<const PaIpfw2Backend::Snapshot4*>(snap4);
    XLOG_ASSERT(dsnap4 != NULL);

    uint8_t setnum = dsnap4->get_ruleset();
    XLOG_ASSERT(setnum < RESERVED_RULESET);

    // Do an atomic swap of the currently active ruleset with this one.
    swap_ruleset4(DEFAULT_RULESET, setnum);

    // Deallocate slot.
    _snapshot4db[setnum] = NULL;

    return (XORP_OK);
#endif
}

#ifdef HAVE_PACKETFILTER_IPFW2
/* ------------------------------------------------------------------------- */
/* Private utility methods used for manipulating IPFW2 itself */

// Class method to retrieve the auto-increment step size. This only
// exists in IPFW2 and is used to tell it apart from IPFW1, as well as
// telling if IPFW1 is loaded at runtime.
int
PaIpfw2Backend::get_autoinc_step(uint32_t& step)
{
    size_t step_size = sizeof(step);
    int ret = sysctlbyname("net.inet.ip.fw.autoinc_step",
			   &step, &step_size, NULL, 0);
    if (ret == -1)
	return (XORP_ERROR);
    return (XORP_OK);
}

// Set the auto-increment step for when rule numbers are not specified.
int
PaIpfw2Backend::set_autoinc_step(const uint32_t& step)
{
    size_t step_size = sizeof(step);
    int ret = sysctlbyname("net.inet.ip.fw.autoinc_step",
			   NULL, NULL,
			   const_cast<void*>((const void*)&step), step_size);
    if (ret == -1)
	return (XORP_ERROR);
    return (XORP_OK);
}

// Pass an IPFW2 command to the kernel using the correct calling
// convention, and return its result.
int
PaIpfw2Backend::docmd4(int optname, void *optval, socklen_t optlen)
{
    int ret;
    switch (optname) {
    case IP_FW_ADD:
    case IP_FW_GET:
#if 0
    case IP_FW_TABLE_LIST:
    case IP_FW_TABLE_GETSIZE:
#endif
	ret = getsockopt(_s4, IPPROTO_IP, optname, optval, &optlen);
	break;
    default:
	ret = setsockopt(_s4, IPPROTO_IP, optname, optval, optlen);
	break;
    }
    return (ret);
}

// Change the rulesets which are disabled and enabled. 'Real' function.
int
PaIpfw2Backend::enable_disable_rulesets4(RulesetGroup& enable_group,
					 RulesetGroup& disable_group)
{
    uint32_t masks[2];
    masks[0] = disable_group.to_ulong();
    masks[1] = enable_group.to_ulong();
    return (docmd4(IP_FW_DEL, masks, sizeof(masks)));
}

// Enable the specified ruleset. Convenience wrapper.
int
PaIpfw2Backend::enable_ruleset4(int index)
{
    XLOG_ASSERT(index >= DEFAULT_RULESET && index < RESERVED_RULESET);
    RulesetGroup enable_group, disable_group;
    enable_group.set(index);
    return (enable_disable_rulesets4(enable_group, disable_group));
}

// Disable the specified ruleset. Convenience wrapper.
int
PaIpfw2Backend::disable_ruleset4(int index)
{
    XLOG_ASSERT(index >= DEFAULT_RULESET && index < RESERVED_RULESET);
    RulesetGroup enable_group, disable_group;
    disable_group.set(index);
    return (enable_disable_rulesets4(enable_group, disable_group));
}

// Move ruleset in its entirety from source to destination.
int
PaIpfw2Backend::move_ruleset4(int src_index, int dst_index)
{
    XLOG_ASSERT(src_index >= DEFAULT_RULESET && src_index < RESERVED_RULESET);
    XLOG_ASSERT(dst_index >= DEFAULT_RULESET && dst_index < RESERVED_RULESET);
    uint32_t cmds = 0;
    cmds |= (CMD_MOVE_RULESET << 24);
    cmds |= ((dst_index << 16) & 0xFF);
    cmds |= src_index & 0xFFFF;
    return (docmd4(IP_FW_DEL, (void*)(uintptr_t)cmds, sizeof(cmds)));
}

// Swap the contents of the two rulesets.
int
PaIpfw2Backend::swap_ruleset4(int first_index, int second_index)
{
    XLOG_ASSERT(first_index >= DEFAULT_RULESET &&
		 first_index < RESERVED_RULESET);
    XLOG_ASSERT(second_index >= DEFAULT_RULESET &&
		 second_index < RESERVED_RULESET);
    uint32_t cmds = 0;
    cmds |= (CMD_SWAP_RULESETS << 24);
    cmds |= ((first_index << 16) & 0xFF);
    cmds |= second_index & 0xFFFF;
    return (docmd4(IP_FW_DEL, (void*)(uintptr_t)cmds, sizeof(cmds)));
}

// Flush the contents of a given ruleset.
int
PaIpfw2Backend::flush_ruleset4(int index)
{
    XLOG_ASSERT(index >= DEFAULT_RULESET && index < RESERVED_RULESET);
    uint32_t cmds = 0;
    cmds |= (CMD_DEL_RULES_WITH_SET << 24);
    cmds |= index & 0xFFFF;
    return (docmd4(IP_FW_DEL, (void*)(uintptr_t)cmds, sizeof(cmds)));
}

// Micro-assemble an IPFW2 instruction sequence for the specified
// XORP ACL entry.
//
// XXX: Candidates for offsetof() in here, but this is known to
// currently be broken on FreeBSD 5.3 system gcc 3.4.2.
// XXX: The O_IP_xxx_MASK opcodes are currently always specified.
// They are candidates for optimization, i.e. omit micro-assembled
// instructions if addresses evaluate to 0.0.0.0/0.
//
void
PaIpfw2Backend::transcribe_rule4(const PaEntry4& entry,
				 const int ruleset_index,
				 uint32_t rulebuf[MAX_IPFW2_RULE_WORDS],
				 uint32_t& size_used)
{
    XLOG_ASSERT(ruleset_index >= DEFAULT_RULESET &&
		ruleset_index < RESERVED_RULESET);

    // Layout of buffer:
    //   +------- start --------+   <- &rulebuf[0]
    //   |   struct ip_fw       |
    //   +--- action opcode(s)--+   <- actp
    //   |   O_ACCEPT or O_DENY |
    //   +---- match opcodes ---+   <- matchp
    //   |   O_VIA              |
    //   |   O_PROTO            |
    //   |   O_IP_SRC_MASK      |
    //   |   O_IP_DST_MASK      |
    //   |   O_IP_SRCPORT       |
    //   |   O_IP_DSTPORT       |
    //   +----------------------+
    //   |      free space      |
    //   +-------- end ---------+   <- &rulebuf[MAX_IPFW2_RULE_WORDS+1]
    //
    uint32_t	*rulep, *matchp, *actp;

    rulep = &rulebuf[0];
    memset(rulebuf, 0, sizeof(rulebuf));

    struct ip_fw *rule = (struct ip_fw *)rulep;
    rule->rulenum = 0;		// Use auto-increment step for rule numbering.
    rule->set = ruleset_index;
    rule->_pad = 0;		// XXX: Need to work out padding...

    // Advance past the rule structure we instantiate first in vector space.
    // Record index of first instruction in vector space.
    rulep += ((sizeof(struct ip_fw) - sizeof(ipfw_insn)) / sizeof(uint32_t));
    actp = rulep;

    // Add the appropriate action instruction.
    ipfw_insn *actinsp = (ipfw_insn *)rulep;
    if (entry.action() == PaAction(ACTION_PASS)) {
	actinsp->opcode = O_ACCEPT;
    } else if (entry.action() == PaAction(ACTION_DROP)) {
	actinsp->opcode = O_DENY;
    } else {
	actinsp->opcode = O_NOP;
    }
    actinsp->len |= F_INSN_SIZE(ipfw_insn);
    rulep += F_INSN_SIZE(ipfw_insn);

    // Fill out offset of action in 32-bit words.
    // Begin adding match opcodes.
    rule->act_ofs = actp - &rulebuf[0];
    matchp = rulep;

    // Add the interface match opcode.
    // On this platform, ifname == vifname.
    ipfw_insn_if* ifinsp = (ipfw_insn_if *)rulep;
    ifinsp->o.opcode = O_VIA;
    ifinsp->name[0] = '\0';
    strlcpy(ifinsp->name, entry.ifname().c_str(), sizeof(ifinsp->name));
    ifinsp->o.len |= F_INSN_SIZE(ipfw_insn_if);
    rulep += F_INSN_SIZE(ipfw_insn_if);

    // Add the protocol match opcode.
    if (entry.proto() != PaEntry4::IP_PROTO_ANY) {
	ipfw_insn* pinsp = (ipfw_insn *)rulep;
	pinsp->opcode = O_PROTO;
	pinsp->len |= F_INSN_SIZE(ipfw_insn);
	pinsp->arg1 = entry.proto();
	rulep += F_INSN_SIZE(ipfw_insn);
    }

    // Add a match source address and mask instruction.
    ipfw_insn_ip* srcinsp = (ipfw_insn_ip *)rulep;
    srcinsp->o.opcode = O_IP_SRC_MASK;
    srcinsp->o.len |= F_INSN_SIZE(ipfw_insn_ip);
    entry.src().masked_addr().copy_out(srcinsp->addr);
    entry.src().netmask().copy_out(srcinsp->mask);
    rulep += F_INSN_SIZE(ipfw_insn_ip);

    // Add a match destination address and mask instruction.
    ipfw_insn_ip* dstinsp = (ipfw_insn_ip *)rulep;
    dstinsp->o.opcode = O_IP_DST_MASK;
    dstinsp->o.len |= F_INSN_SIZE(ipfw_insn_ip);
    entry.dst().masked_addr().copy_out(dstinsp->addr);
    entry.dst().netmask().copy_out(dstinsp->mask);
    rulep += F_INSN_SIZE(ipfw_insn_ip);

    // Insert port match instructions, if required.
    // We don't use port ranges here, so specify the same
    // port in both members of the ports array.
    if (entry.proto() == IPPROTO_TCP || entry.proto() == IPPROTO_UDP) {
	// Add source port match instruction.
	if (entry.sport() != PaEntry4::PORT_ANY) {
	    ipfw_insn_u16* spinsp = (ipfw_insn_u16 *)rulep;
	    spinsp->o.opcode = O_IP_SRCPORT;
	    spinsp->o.len = F_INSN_SIZE(ipfw_insn_u16);
	    spinsp->ports[0] = spinsp->ports[1] = entry.sport();
	    rulep += F_INSN_SIZE(ipfw_insn_u16);
	}
	// Add destination port match instruction.
	if (entry.dport() != PaEntry4::PORT_ANY) {
	    ipfw_insn_u16* dpinsp = (ipfw_insn_u16 *)rulep;
	    dpinsp->o.opcode = O_IP_DSTPORT;
	    dpinsp->o.len = F_INSN_SIZE(ipfw_insn_u16);
	    dpinsp->ports[0] = dpinsp->ports[1] = entry.sport();
	    rulep += F_INSN_SIZE(ipfw_insn_u16);
	}
    }

    // Compute and store the number of 32-bit words in ip_fw.cmd[].
    // Compute and store the total size, in bytes, of the rule structure.
    rule->cmd_len = rulep - actp;
    size_used = rulep - &rulebuf[0];
}

// Push a previously converted rule into the given ruleset in the kernel.
int
PaIpfw2Backend::push_rule4(const int ruleset_index, uint32_t rulebuf[],
			   const uint32_t size_used)
{
    XLOG_ASSERT(ruleset_index >= DEFAULT_RULESET &&
		ruleset_index < RESERVED_RULESET);
    struct ip_fw *rulep = (struct ip_fw *)&rulebuf[0];
    rulep->set = ruleset_index;
    return (docmd4(IP_FW_ADD, rulep, size_used));
}

static inline struct ip_fw*
next_rule(struct ip_fw* rulep)
{
    return ((struct ip_fw*)((uint8_t*)rulep + RULESIZE(rulep)));
}

// Read the specified ruleset from the kernel in its entirety, and
// store it into the provided empty RulesetDB.
int
PaIpfw2Backend::read_ruleset4(const int ruleset_index, RulesetDB& rulesetdb)
{
    XLOG_ASSERT(rulesetdb.empty());

    vector<uint8_t> chainbuf;
    size_t nbytes;
    size_t nalloc;
    nbytes = nalloc = 1024;

    // Play elastic buffer games to get a snapshot of the entire rule chain.
    chainbuf.reserve(nalloc);
    while (nbytes >= nalloc) {
	nbytes = nalloc = nalloc * 2 + 200;
	chainbuf.reserve(nalloc);
	if (docmd4(IP_FW_GET, &chainbuf[0], nbytes) == -1)
	    return (-1);
    }
    chainbuf.resize(nbytes);

    // Parse buffer contents for all static rules corresponding to the
    // given ruleset. Copy each matching rule into the provided RulesetDB.
    struct ip_fw* rulep = (struct ip_fw*)&chainbuf[0];
    uint8_t* endp = &chainbuf[nbytes];
    while (rulep->rulenum < IPFW_RULENUM_MAX && (uint8_t*)rulep < endp) {
	if (rulep->set == ruleset_index) {
	    rulesetdb.insert(RulesetDB::value_type(rulep->rulenum, RuleBuf()));
	    RuleBuf& rulebuf = rulesetdb[rulep->rulenum];
	    size_t rule_size = rulep->cmd_len * 4 + sizeof(struct ip_fw) -
			       sizeof(ipfw_insn);
	    rulebuf.reserve(rule_size / 4);
	    memcpy(&rulebuf[0], rulep, rule_size);
	}
	rulep = next_rule(rulep);
    }

    return (0);
}

// Renumber the ruleset index of all the rules contained in a RulesetDB.
void
PaIpfw2Backend::renumber_ruleset4(const int ruleset_index, RulesetDB& rulesetdb)
{
    for (RulesetDB::iterator i = rulesetdb.begin(); i != rulesetdb.end(); i++) {
	struct ip_fw* rulep = (struct ip_fw*)&i->second;
	rulep->set = ruleset_index;
    }
}

// Push the contents of a RulesetDB back into the kernel.
int
PaIpfw2Backend::push_rulesetdb4(RulesetDB& rulesetdb)
{
    for (RulesetDB::iterator i = rulesetdb.begin();
	 i != rulesetdb.end(); i++) {
	struct ip_fw* rulep = (struct ip_fw*)&i->second;
	size_t rule_size = rulep->cmd_len * 4 + sizeof(struct ip_fw) -
			   sizeof(ipfw_insn);
	(void)docmd4(IP_FW_ADD, (void*)rulep, rule_size);
    }
    return (0);
}

// Copy one ruleset in its entirety to another.
void
PaIpfw2Backend::copy_ruleset4(int src_index, int dst_index)
{
    XLOG_ASSERT(src_index >= DEFAULT_RULESET && src_index < RESERVED_RULESET);
    XLOG_ASSERT(dst_index >= DEFAULT_RULESET && dst_index < RESERVED_RULESET);
    RulesetDB rulesetdb;
    read_ruleset4(src_index, rulesetdb);
    renumber_ruleset4(dst_index, rulesetdb);
    push_rulesetdb4(rulesetdb);
}

/* ------------------------------------------------------------------------- */
/* Snapshot scoped classes (Memento pattern) */

// Cannot be copied or assigned from base class.
PaIpfw2Backend::Snapshot4::Snapshot4(const PaBackend::Snapshot4Base& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(snap4),
      _parent(NULL),
      _ruleset(RESERVED_RULESET)
{
    throw PaInvalidSnapshotException();
}

// May be copied or assigned from own class.
PaIpfw2Backend::Snapshot4::Snapshot4(const PaIpfw2Backend::Snapshot4& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(snap4),
      _parent(snap4._parent), _ruleset(snap4._ruleset)
{
}

// This constructor is marked private and used internally.
PaIpfw2Backend::Snapshot4::Snapshot4(PaIpfw2Backend& parent, uint8_t ruleset)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base(),
      _parent(&parent), _ruleset(ruleset)
{
}

PaIpfw2Backend::Snapshot4::~Snapshot4()
{
    // Deallocate the snapshot in the parent.
    _parent->get_snapshotdb()[_ruleset] = NULL;
}

/* ------------------------------------------------------------------------- */
#endif // HAVE_PACKETFILTER_IPFW2
