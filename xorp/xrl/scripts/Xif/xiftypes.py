#!/usr/bin/python3
xrl_atom_type = {
# <name used in xrls> : (<c++ type>, <xrltom accessor method>, <thrift type>)
    'bool' :	('bool',		'boolean',	'bool'),
    'i32' :	('int32_t',		'int32',	'i32'),
    'u32' :	('uint32_t',		'uint32',	'i32'),
    'ipv4' :	('IPv4',		'ipv4',		'xorp.ipv4'),
    'ipv4net' :	('IPv4Net',		'ipv4net',	'xorp.ipv4net'),
    'ipv6' :	('IPv6',		'ipv6',		'xorp.ipv6'),
    'ipv6net' :	('IPv6Net',		'ipv6net',	'xorp.ipv6net'),
    'mac' :	('Mac',   		'mac',		'binary'),
    'txt' :	('string',		'text',		'string'),
    'list' :	('XrlAtomList', 	'list',		'list'),
    'binary' :	('vector<uint8_t>',	'binary',	'binary'),
    'i64' :	('int64_t',		'int64',	'i64'),
    'u64' :	('uint64_t',		'uint64',	'i64'),
    'fp64' :	('fp64_t',		'fp64',	        'double')
}

class XrlArg:
    def __init__(self, name, type, annotation = ""):
        self._name = name
        self._type = type
        self._note = annotation
        self._member_type = None	# XXX THRIFT-
        self._member = None		# XXX THRIFT+
    def name(self):
        return self._name
    def type(self):
        return self._type
    def cpp_type(self):
        tuple = xrl_atom_type[self._type]
        return tuple[0]
    def set_member_type(self, member_type):
        self._member_type = member_type		# XXX THRIFT-
        # XXX THRIFT+
        self._member = None
        if member_type != None:
            self._member = XrlArg("member", member_type)
    # XXX THRIFT-
    def member_type(self):
        """If type is list, the type of the members embedded within."""
        return self._member_type
    # XXX THRIFT+
    def member(self):
        return self._member
    def is_list(self):
        return self._type == 'list'
    def accessor(self):
        tuple = xrl_atom_type[self._type]
        return tuple[1]
    def thrift_idl_type(self):
        """The type as expressed in the IDL."""
        tuple = xrl_atom_type[self._type]
        return tuple[2]
    def set_annotation(self, annotation):
        self._note = annotation
    def annotation(self):
        return self._note

class XrlMethod:
    def __init__(self, name, args, rargs, annotation = ""):
        self._name = name
        self._args = args
        self._rargs = rargs
        self._note = annotation
    def name(self):
        return self._name
    def set_name(self, name):
        self._name = name
    def args(self):
        return self._args
    def rargs(self):
        return self._rargs
    def set_annotation(self, annotation):
        self._note = annotation
    def annotation(self):
        return self._note

class XrlInterface:
    def __init__(self, name, version, sourcefile):
        self._name = name
        self._version = version
        self._sourcefile = sourcefile
        self._methods = []
    def name(self):
        return self._name
    def version(self):
        return self._version
    def sourcefile(self):
        return self._sourcefile
    def methods(self):
        return self._methods
    def add_method(self, m):
        self._methods.append(m)

# XXX NOTE WELL: 'interfaces' is a list of string tuples,
# NOT a collection of XrlInterface objects.
class XrlTarget:
    def __init__(self, name, version, sourcefile):
        self._name = name
        self._interfaces = []
        self._version = version
        self._sourcefile = sourcefile
    def name(self):
        return self._name
    def sourcefile(self):
        return self._sourcefile
    def version(self):
        return self._version
    def add_interface(self, interface):
        self._interfaces.append(interface)
    def interfaces(self):
        return self._interfaces






