xrl_atom_type = {
# <name used in xrls> : (<c++ type>, <xrltom accessor method>)
    'bool' :	('bool',		'boolean'),
    'i32' :	('int32_t',		'int32'),
    'u32' :	('uint32_t',		'uint32'),
    'ipv4' :	('IPv4',		'ipv4'),
    'ipv4net' :	('IPv4Net',		'ipv4net'),
    'ipv6' :	('IPv6',		'ipv6'),
    'ipv6net' :	('IPv6Net',		'ipv6net'),
    'mac' :	('Mac',   		'mac'),
    'txt' :	('string',		'text'),
    'list' :	('XrlAtomList', 	'list'),
    'binary' :	('vector<uint8_t>',	'binary'),
    'i64' :	('int64_t',		'int64'),
    'u64' :	('uint64_t',		'uint64')
}

class XrlArg:
    def __init__(self, name, type, annotation = ""):
        self._name = name
        self._type = type
        self._note = annotation
    def name(self):
        return self._name
    def type(self):
        return self._type
    def cpp_type(self):
        tuple = xrl_atom_type[self._type] 
        return tuple[0]
    def accessor(self):
        tuple = xrl_atom_type[self._type] 
        return tuple[1]
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






