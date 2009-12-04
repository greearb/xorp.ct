# vim:set sts=4 ts=8 sw=4:

# in this namespace
from util import xorp_indent

# -----------------------------------------------------------------------------
# XIF to Thrift output generation functions
# -----------------------------------------------------------------------------

def send_bool(fname):
    lines = []
    lines.append("nout += outp->writeBool(%s);" % fname)
    return lines

def send_i32(fname):
    lines = []
    lines.append("nout += outp->writeI32(%s);" % fname)
    return lines

def send_u32(fname):
    lines = []
    lines.append("nout += outp->writeI32(static_cast<int32_t>(%s));" % fname)
    return lines

def send_i64(fname):
    lines = []
    lines.append("nout += outp->writeI64(%s);" % fname)
    return lines

def send_u64(fname):
    lines = []
    lines.append("nout += outp->writeI64(static_cast<uint64_t>(%s));" % fname)
    return lines

# XXX We need to preserve network endianness. Class IPv4 holds addr
# internally in network byte order, so we need to tell send_ipv4()
# to convert it to host-endian BEFORE passing it to Thrift, which
# will automatically convert it.
# The alternative is to marshal as a byte array, but I'd rather not
# do that because of buffer games.

def send_ipv4(fname):
    lines = []
    lines.append("nout += outp->writeI32(")
    lines.append("    static_cast<int32_t>(ntohl(%s.addr())));" % fname)
    return lines

def send_ipv4net(fname, sname='ipv4net'):
    lines = []
    lines.append("nout += outp->writeStructBegin(\"%s\");" % sname)
    lines.append("nout += outp->writeFieldBegin(\"network\", T_I32, 1);")
    lines.append("nout += outp->writeI32(ntohl(%s.masked_addr().addr()));" % \
	fname)
    lines.append("nout += outp->writeFieldEnd();")
    lines.append("nout += outp->writeFieldBegin(\"prefix\", T_BYTE, 2);")
    lines.append("nout += outp->writeByte(%s.prefix_len());" % fname)
    lines.append("nout += outp->writeFieldEnd();")
    lines.append("nout += outp->writeFieldStop();")
    lines.append("nout += outp->writeStructEnd();")
    return lines

def send_ipv6(fname, sname='ipv6', addrmethod='addr()'):
    lines = []
    lines.append("nout += outp->writeStructBegin(\"%s\");" % sname)
    lines.append("const uint32_t* pa_%s = %s.%s;" % \
	(fname, fname, addrmethod))
    for i in xrange(1,5):
	lines.append(\
	    "nout += outp->writeFieldBegin(\"addr32_%d\", T_I32, %d);" % \
	    (i, i))
	lines.append("nout += outp->writeI32(pa_%s[%d]);" % \
	    (fname, (i-1)))
	lines.append("nout += outp->writeFieldEnd();")
    lines.append("nout += outp->writeFieldStop();")
    lines.append("nout += outp->writeStructEnd();")
    return lines

# struct of structs
def send_ipv6net(fname, sname='ipv6net'):
    lines = []
    lines.append("nout += outp->writeStructBegin(\"%s\");" % sname)
    lines.append("nout += outp->writeFieldBegin(\"network\", T_STRUCT, 1);")
    tmp = send_ipv6(fname, addrmethod="masked_addr().addr()")
    lines += tmp
    lines.append("nout += outp->writeFieldEnd();")
    lines.append("nout += outp->writeFieldBegin(\"prefix\", T_BYTE, 2);")
    lines.append("nout += outp->writeByte(%s.prefix_len());" % fname)
    lines.append("nout += outp->writeFieldEnd();")
    lines.append("nout += outp->writeFieldStop();")
    lines.append("nout += outp->writeStructEnd();")
    return lines

# Use the Mac::addr() accessor to marshal out the MAC address directly
# as binary. We need to cast to string(%s.addr(), %s.addr_bytelen())
# before marshaling as Thrift T_STRING with writeBinary(), so another
# cast is needed.
# FIXME Eliminate string() temporary by overloading TProtocol::writeBinary().
def send_mac(fname):
    lines = []
    lines.append("nout += outp->writeBinary(string(reinterpret_cast<const char*>(%s.addr()), Mac::ADDR_BYTELEN));" % fname)
    return lines

def send_txt(fname):
    lines = []
    lines.append("nout += outp->writeString(%s);" % fname)
    return lines

# XXX: Thrift on-wire protocol uses T_STRING for binary data,
# however we need to call the writeBinary() method on the
# TProtocol instance. We don't use this as the default
# Thrift mapping, as 'binary' is a distinct type in the .thrift
# IDL syntax.
# FIXME Eliminate string() temporary by overloading TProtocol::writeBinary().
def send_binary(fname):
    lines = []
    lines.append("nout += outp->writeBinary(string(reinterpret_cast<const char*>(&%s[0]), %s.size()));" % tuple([fname] * 2))
    return lines

# -----------------------------------------------------------------------------
# XIF to Thrift input generation functions
# -----------------------------------------------------------------------------

def recv_bool(fname):
    lines = []
    lines.append("nin += inp->readBool(%s);" % fname)
    return lines

def recv_i32(fname):
    lines = []
    lines.append("nin += inp->readI32(%s);" % fname)
    return lines

def recv_u32(fname):
    lines = []
    lines.append("int32_t i32_%s;" % fname)
    lines.append("nin += inp->readI32(i32_%s);" % fname)
    lines.append("%s = static_cast<uint32_t>(i32_%s);" % (fname, fname))
    return lines

def recv_i64(fname):
    lines = []
    lines.append("nin += inp->readI64(%s);" % fname)
    return lines

def recv_u64(fname):
    lines = []
    lines.append("int64_t i64_%s;" % fname)
    lines.append("nin += inp->readI64(i64_%s);" % fname)
    lines.append("%s = static_cast<uint64_t>(i64_%s);" % (fname, fname))
    return lines

def recv_ipv4(fname):
    lines = []
    lines.append("int32_t i32_%s;" % fname)
    lines.append("nin += inp->readI32(i32_%s);" % fname)
    lines.append("%s = IPv4(htonl(static_cast<uint32_t>(i32_%s)));" % \
	(fname, fname))
    return lines

# Shim these to support functions to handle embedded struct.

def recv_ipv4net(fname):
    lines = []
    lines.append("nin += xif_read_ipv4net(%s, inp);" % fname)
    return lines

def recv_ipv6(fname):
    lines = []
    lines.append("nin += xif_read_ipv6(%s, inp);" % fname)
    return lines

def recv_ipv6net(fname):
    lines = []
    lines.append("nin += xif_read_ipv6net(%s, inp);" % fname)
    return lines

# This is slightly inefficient as a bounce buffer is involved.
# Thrift binary reads expect to put input into a std::string, so
# rejig accordingly and do a temporary copy into our Mac class.
# Not much to do -- 6 bytes on stack. Nothing to worry about. Better
# than making Mac aware of Thrift.
def recv_mac(fname):
    lines = []
    lines.append("string tmp_%s;" % fname)
    lines.append("nin += inp->readBinary(tmp_%s);" % fname)
    # XXX: We should sanity check the length of the binary data once read.
    lines.append("//assert(tmp_%s.size() == Mac::ADDR_BYTELEN);" % fname)
    lines.append("%s.copy_in(\n" % fname)
    lines.append("    reinterpret_cast<const uint8_t *>(tmp_%s.data()));" % \
	fname)
    return lines

def recv_txt(fname):
    lines = []
    lines.append("nin += inp->readString(%s);" % fname)
    return lines

# XXX This is slightly inefficient as a bounce buffer is involved.
# Thrift readBinary() returns string; we end up bouncing a buffer
# by copying it here, which is likely inefficient.
#
# Future: As the XRL callbacks only see this as a const vector<uint8_t>*,
# we can get away with using an adaptor of some kind on top of
# the string which Thrift expects. Either that or we modify TProtocol...
# Don't worry about this for now -- getting it all up is more important.
def recv_binary(fname):
    lines = []
    lines.append("string tmp_%s;" % fname)
    lines.append("nin += inp->readBinary(tmp_%s);" % fname)
    lines.append("%s.resize(tmp_%s.size());" % (fname, fname))
    lines.append("memcpy(&%s[0], tmp_%s.data(), tmp_%s.size());" % \
	(fname, fname, fname))
    return lines

# -----------------------------------------------------------------------------

xiftothrift = {
    # atom_type: (thrift_wire_type, send_gen_fn, recv_gen_fn)
    'bool':	('T_BOOL',	send_bool,	recv_bool),
    'i32':	('T_I32',	send_i32,	recv_i32),
    'u32':	('T_I32',	send_u32,	recv_u32),
    'ipv4':	('T_I32',	send_ipv4,	recv_ipv4),
    'ipv4net':	('T_STRUCT',	send_ipv4net,	recv_ipv4net),
    'ipv6':	('T_STRUCT',	send_ipv6,	recv_ipv6),
    'ipv6net':	('T_STRUCT',	send_ipv6net,	recv_ipv6net),
    'mac':	('T_STRING',	send_mac,	recv_mac),
    'txt':	('T_STRING',	send_txt,	recv_txt),
    'list':	('T_LIST',	None,		None),	# XXX recursive.
    'binary':	('T_STRING',	send_binary,	recv_binary),
    'i64':	('T_I64',	send_i64,	recv_i64),
    'u64':	('T_I64',	send_u64,	recv_u64),
}

def wire_type(a):
    return xiftothrift[a.type()][0]

# -----------------------------------------------------------------------------

def send_arg(a, fname=None):
    lines = []
    if fname is None:
	fname = a.name()
    if a.is_list():
	lines += send_list(a)
    else:
	lines += (xiftothrift[a.type()][1])(fname)
    return lines

def recv_arg(a, fname=None):
    lines = []
    if fname is None:
	fname = a.name()
    if a.is_list():
	lines += recv_list(a, fname)
    else:
	lines += (xiftothrift[a.type()][2])(fname)
    return lines

# -----------------------------------------------------------------------------

#
# Marshal a Thrift list in as an XrlAtomList.
# TODO: sanity check mt_%s vs m.type()?
#
def recv_list(a, fname):
    lines = []
    m = a.member()

    if a.member().is_list():
	print "Lists-of-lists are currently not implemented, sorry."
	sys.exit(1)

    tab = xorp_indent(1)
    lines.append("uint32_t sz_%s;" % fname)
    lines.append("TType mt_%s;" % fname)
    lines.append("nin += inp->readListBegin(mt_%s, sz_%s);" % \
	tuple([fname] * 2))
    lines.append("for (uint32_t i_%s = 0; i_%s < sz_%s; i_%s++) {" % \
	tuple([fname] * 4))

    # Now read in base type.
    mfname = "tmp_%s" % fname
    lines.append(tab + "%s %s;" % (m.cpp_type(), mfname))

    for l in recv_arg(m, mfname):
    	lines.append(tab + l)

    # now convert to XrlAtom and append to our collection in local scope.
    lines.append(tab + ("%s.append(XrlAtom(tmp_%s));" % \
	tuple([fname] * 2)))
    lines.append("}")
    lines.append("nin += inp->readListEnd();")
    return lines

#
# Marshal an XrlAtomList out as a Thrift list.
#
# Earlier in this branch, we forced XrlAtomLists in XIF to have
# known member types -- this is how the code behaves, and we need
# the hint anyway to map Thrift types back between XIF types.
#
# XrlAtomLists are numbered [1..size()], not [0..size()).
# Thrift collection members are written out individually.
# If no elements are present, just write no members.
# Indent the if() / for() block marshal-out code.
# We'll assume the code has filled out the list w/o introducing
# any heterogenous members. If not, this generated shim will
# throw a WrongType exception in the XrlAtom code.
#
# XXX Special case. This requires the arg object as a parameter.
#
def send_list(a):
    lines = []
    fname = a.name()
    m = a.member()

    if a.member().is_list():
	print "Lists-of-lists are currently not implemented, sorry."
	sys.exit(1)

    tab = xorp_indent(1)
    tab2 = xorp_indent(2)

    lines.append("size_t sz_%s = %s.size();" % (fname, fname))
    lines.append("nout += outp->writeListBegin(%s, sz_%s);" % \
	(wire_type(m), fname))

    lines.append("if (sz_%s > 0) {" % fname)
    lines.append(tab + "for (size_t i_%s = 1; i_%s <= sz_%s; i_%s++) {" % \
	(fname, fname, fname, fname))
    lines.append(tab2 + "const XrlAtom& xa = %s.get(i_%s);" % \
	(fname, fname))
    mfname = "xa_%s" % m.type()	    # member field name with XIF type
    lines.append(tab2 + "const %s& %s = xa.%s();" % \
	(m.cpp_type(), mfname, m.accessor()))

    # now marshal out the member type, indented at this level correctly.
    for l in send_arg(m, mfname):
    	lines.append(tab2 + l)

    lines.append(tab + "}")	# close for
    lines.append("}")		# close if
    lines.append("nout += outp->writeListEnd();")   # close thrift list
    return lines

