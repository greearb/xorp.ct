xrl_atom_type = {
# <name used in xrls> : (<c++ type>, <xrlargs accessor method>)
    'bool' :	('bool',		'get_bool'),
    'i32' :	('int32_t',		'get_int32'),
    'u32' :	('uint32_t',		'get_uint32'),
    'ipv4' :	('IPv4',		'get_ipv4'),
    'ipv4net' :	('IPv4Net',		'get_ipv4net'),
    'ipv6' :	('IPv6',		'get_ipv6'),
    'ipv6net' :	('IPv6Net',		'get_ipv6net'),
    'mac' :	('Mac',   		'get_mac'),
    'txt' :	('string',		'get_string'),
    'list' :	('XrlAtomList', 	'get_list'),
    'binary' :	('vector<uint8_t>',	'get_binary'),
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






