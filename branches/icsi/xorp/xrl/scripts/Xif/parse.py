# Python imports
import string, re, sys
from xiftypes import *
from util import quit

# XIF specific imports

g_source_files = {}
g_lines = []
g_files = []
g_line  = 0

def push_file(file, line):
    global g_files, g_lines, g_line, g_source_files
    g_files.append(file)
    g_lines.append(int(line) - 1)
    g_line = int(line)
    g_source_files[file] = 1

def pop_file():
    global g_files, g_lines
    g_files = g_files[0:-1]
    g_lines = g_lines[0:-1]

def incr_line():
    global g_line
    g_line = g_line + 1

def get_input_line():
    global g_line
    return int(g_line)

def get_input_file():
    return g_files[-1]

line_file_flag = re.compile("#\s+(\d+)\s+\"(.*)\"\s+(\d*)")

# Parse C-preprocessor generated # line, return 1 if recognized, 0 otherwise
def parse_cpp_hash(line):
    # Format is '# LINENUM FILENAME [FLAG]' where FLAG is either:
    #
    # `1' This indicates the start of a new file.
    #
    # `2' This indicates returning to a file (after having included another
    # file).
    #
    # `3' This indicates that the following text comes from a system header
    # file, so certain warnings should be suppressed.
    #
    # `4' This indicates that the following text should be treated as C.

    p = line_file_flag.match(line)
    if p:
        groups = p.groups()
        line, file, flag = groups
        if flag == '':
            flag = "1"
    else:
        return 0

    if flag == "1":
        push_file(file, line)
    elif flag == "2":
        pop_file()
        pop_file()
        push_file(file, line)
    else:
        print "Invalid pre-processor #line flag (%d)\n", flag
        sys.exit(1)
    return 1

def validate_name(file, line, type, value, extra):
    pattern = "[A-Za-z][A-z0-9%s]?" % extra
    if re.match(pattern, value) == None:
        errmsg = "%s name \"%s\" has chars other than %s" \
             % (type, value, pattern) 
        quit(file, line, errmsg)

def parse_args(file, lineno, str):
    if str == "":
        return []

    toks = string.split(str, "&")

    # arg format is <name>[<type>]
    #                     ^      ^

    xrl_args = []
    for t in toks:

        lb = string.find(t, ":")
        if lb == -1:
            quit(file, lineno, "Missing \":\" in xrlatom \"%s\"" % t)
            
        name = t[:lb]
        type = t[lb + 1:]

        validate_name(file, lineno, "Atom", name, '_-')

        if xrl_atom_type.has_key(type) == 0:
            quit(file, lineno, "Atom type \"%s\" not amongst those known %s"
                 % (type, xrl_atom_type.keys()))

        xrl_args.append(XrlArg(name, type))
    return xrl_args

""" Fill in any missing separators in Xrl to make parsing trivial"""
def fix_up_line(line):
    # chomp whitespace
    table = string.maketrans("","")
    line =  string.translate(line, table, string.whitespace)

    # Fill in missing component separators.
    if line.find("->") < 0:
        line += "->"
    if line.find("?") < 0:
        line = line.replace("->", "?->")
    return line

""" Split line into tokens separated by separators """
def componentize(line, separators):
    r = [line]
    for s in separators:
        tokens = string.split(r[-1], s, 1)
        r[-1] = tokens[0]
        r.append(tokens[1])
    return r;

target_if = re.compile("([\w-]+)/(\d+\.\d+)")
target_if_sep = re.compile("\s*,\s*")

def parse_target_interfaces(file, lineno, line):
    ifs = []
    while (line != ""):
        m = target_if.match(line)
        if m == None:
            print "Bad interface in file %s at line %d\n\"%s\"\n" % \
                  (file, lineno, line)
            sys.exit(1)
        ifs.append((m.group(1), m.group(2)))
        line = line[m.end():]
        m = target_if_sep.match(line)
        if m:
            line = line[m.end():]
    return ifs

method_outline = re.compile("([^\?]+)\?(.*)->(.*)")

def parse_method(file, lineno, line):
    line = fix_up_line(line)

    m = method_outline.match(line)
    if m == None:
        print "Bad method declaration from %s line %d:\n\"%s\"\n" %\
              (file, lineno, line)
        sys.exit(1)

    method, args, rargs = m.groups()

    if method == "":
        quit(file, lineno, "Missing a method name")
    validate_name(file, lineno, "Method", method, "/_-")

    method_args = parse_args(file, lineno, args)
    return_args = parse_args(file, lineno, rargs);

    for i in method_args:
        for j in return_args:
            if i.name() == j.name():
                quit(file, lineno, "\"%s\" appears as an input and output " \
                     "parameter name." % i.name())

    return XrlMethod(method, method_args, return_args)

""" Parses input file into lists of interfaces and targets """
class XifParser:
    def __init__(self, file):
        self._interfaces = []
        self._targets = []

        if_decl = re.compile("interface\s+([\w\-_]+)/(\d+\.\d+)", re.IGNORECASE)
        target_decl = re.compile("target\s+(\w+)\s+implements\s+(.*)", \
                                 re.IGNORECASE)
        grouping_begin = re.compile("{")
        grouping_end = re.compile("}")
        not_grouping_end = re.compile("[^}]+")

        c_comment_begin       = re.compile("/\*[^*]")
        kdoc_comment_begin    = re.compile("\s*/\*\*")
        oneline_c_comment     = re.compile("/\*.*\*/")
        oneline_kdoc_comment  = re.compile("/\*\*.*\*/")
        comment_end           = re.compile("\*/")

        line_buffer = ""
        grouping = 0

        in_c_comment = 0
        in_kdoc_comment = 0
        current_kdoc_comment = ""

        for line in file.readlines():
            # Update state and continue if line is C-preprocessor output
            if parse_cpp_hash(line):
                continue

            # Increment line number keep error messages in sync
            incr_line()

            # Cache oneline kdoc comments and strip oneline comments from input.
            if (in_kdoc_comment == 0) & (in_c_comment == 0):
                # kdoc bit
                while len(line):
                    mo = oneline_kdoc_comment.search(line)
                    if mo == None:
                        break
                    current_kdoc_comment = line[mo.start():mo.end()]
                    tmp_line = line
                    line = tmp_line[0:mo.start()] + tmp_line[mo.end():]
                # c comment bit
                while len(line):
                    mo = oneline_c_comment.search(line)
                    if mo == None:
                        break
                    tmp_line = line
                    line = tmp_line[0:mo.start()] + tmp_line[mo.end():]

            # Check for END and BEGINNING of kdoc comments
            if in_kdoc_comment:
                mo = comment_end.search(line)
                if mo != None:
                    current_kdoc_comment += line[0:mo.end()]
                    line = line[mo.end():]
                    in_kdoc_comment = 0
                else:
                    current_kdoc_comment += line
                    continue
            elif in_c_comment == 0:
                mo = kdoc_comment_begin.search(line)
                if mo != None:
                    current_kdoc_comment = line[mo.start():]
                    line = line[:mo.start()]
                    in_kdoc_comment = 1

            # Check for END and BEGINNING of C comments
            if in_c_comment:
                mo = comment_end.search(line)
                if mo != None:
                    line = line[mo.end():]
                    in_c_comment = 0
                else:
                    continue
            elif in_kdoc_comment == 0:
                mo = c_comment_begin.search(line)
                if mo != None:
                    line = line[:mo.start()]
                    in_c_comment = 1

            line = line.lower()
            line_buffer += line.strip()
            if len(line_buffer) == 0:
                continue

            # Handle continuation lines
            if line_buffer[-1] == "\\":
                line_buffer = line_buffer[0 : -1]
                continue

            # Parse line
            while line_buffer != "":
                # This strip() is paranoia
                line_buffer = line_buffer.strip()
                if grouping == 0:
                    m = if_decl.match(line_buffer)
                    if m:
                        self._interfaces.append(XrlInterface(m.group(1), m.group(2), get_input_file()))
                        line_buffer = line_buffer[m.end():].strip()
                        continue

                    m = target_decl.match(line_buffer)
                    if m:
                        target_name = m.group(1)
                        tgt = XrlTarget(target_name, "0.0", get_input_file())

                        target_interfaces = parse_target_interfaces( \
                            get_input_file(), get_input_line(), m.group(2))

                        for tif in target_interfaces:
                            tif_name = tif[0]
                            found = 0
                            for i in self._interfaces:
                                if tif_name == i.name():
                                    found = 1
                            if found == 0:
                                print "Interface %s/%s not defined" % tif, \
                                      "in file %s line %d\n" % \
                                      (get_input_file(), get_input_line())
                                print "Valid interfaces are:"
                                for i in self._interfaces:
                                    print "\t%s/%s"% (i.name(), i.version())
                                sys.exit(1)
                            else:
                                tgt.add_interface(tif)

                        self._targets.append(tgt)
                        line_buffer = line_buffer[m.end():].strip()
                        continue

                    m = grouping_begin.match(line_buffer)
                    if m:
                        grouping = 1
                        line_buffer = line_buffer[m.end():].strip()
                        continue

                    print "Unrecognized command in file %s at line %d:\n\"%s\"\n" \
                          % (get_input_file(), get_input_line(), line_buffer)
                    sys.exit(1)
                else:
                    m = not_grouping_end.match(line_buffer)
                    if m:
                        # a method declaration
                        method = parse_method(get_input_file(), \
                                              get_input_line(), \
                                              line_buffer[m.start():m.end()])
                        method.set_annotation(current_kdoc_comment)
                        current_kdoc_comment = ""
                        self._interfaces[-1].add_method(method)
                        line_buffer = line_buffer[m.end():].strip()
                        continue

                    m = grouping_end.match(line_buffer)
                    if m:
                        line_buffer = line_buffer[m.end():].strip()
                        grouping = 0
                        continue
            continue

    def interfaces(self):
        return self._interfaces

    def targets(self):
        return self._targets
