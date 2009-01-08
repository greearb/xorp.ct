"""
This file provides kdoc comment support for XIF interface files.
It helps propagate kdoc comments from interface definitions into C++ files.
"""

import string
import re

def break_into_lines(input_string, linewidth):
    lines = []
    words = input_string.split()
    if len(words) == 0:
        return lines

    cline = words[0]
    for w in words[1:]:
        if len(cline) + len(w) < linewidth:
            cline += " " + w
        else:
            lines.append(cline)
            cline = w
    lines.append(cline)
    return lines


class XifKdocThing:

    def __init__(self, kdoc_comment):
        """
        Parse kdoc comment into main comment, other stuff, and
        list of @param arguments.  The object can then be used to
        generate nicely formatted kdoc comments.
        """

        self._params = {} # key = param name, data = param description
        self._master = ""
        self._other = [] # list of @foo stuff we don't understand

        lines = kdoc_comment.split("\n")
        cleaned_kdoc_comment = "ROOT"
        for line in lines:
            line = line.strip()
            # Strip kdoc comment cruft from line beginnings
            mo = re.search("^\s*/{0,1}\*{1,2}/{0,1}\s*", line)
            if mo:
                line = line[mo.end():]
            # Strip comment ends
            mo = re.search("\s*\*/\s*$", line)
            if mo:
                line = line[:mo.start()]

            cleaned_kdoc_comment += " " + line

        # kdoc keywords all begin with @
        kdoc_sections = cleaned_kdoc_comment.split("@")
        for s in kdoc_sections:
            if s.find("return") == 0:
                print "Ignoring kdoc @return primitive in input file."
                continue
            elif s.find("param") == 0:
                pc = s.split(None, 2)
                if len(pc) != 3:
                    print "@param with missing variable or description: \"%s\"" % s
                    continue
                self._params[pc[1]] = pc[2]
            elif s.find("ROOT") == 0:
                pc = s.split(None, 1)
                if len(pc) > 1:
                    self._master = pc[1]
                else:
                    self._master = ""
            else:
                # Could be anything - prepend @ and chuck into master section
                self._other.append("@" + s)

    def add_kdoc_param(self, name, description):
        """Add a kdoc @param value and description"""
        self._params[name] = description

    def output_kdoc(self, indent_string, paramlist, comment = "", width = 80):
        """
        Output kdoc comments with inserted comment and only for parameters
        named in paramlist.
        """
        s = indent_string + "/**\n"

        indent_string = indent_string.replace("\t", "        ")
        width -= (len(indent_string) + 5)

        if len(comment):
            lines = break_into_lines(comment, width)
            lines.append("")
            for l in lines:
                s += indent_string + " *  " + l + "\n"

        lines = break_into_lines(self._master, width)

        for l in lines:
            s += indent_string + " *  " + l + "\n"

        for p in paramlist:
            if self._params.has_key(p):
                line = "@param " + p + " " + self._params[p]
                lines = break_into_lines(line, width)
                lines.insert(0, "")
                for l in lines:
                    s += indent_string + " *  " + l + "\n"
                
        for o in self._other:
            lines = break_into_lines(o, width)
            lines.insert(0, "")
            for l in lines:
                s += indent_string + " *  " + l + "\n"

        s += indent_string + " */"
        return s

def parse_kdoc_comment(comment):
    return XifKdocThing(comment)

def test():
    comment = """/**
    * hello world
    *
    * @foo bar bar bar
    *
    * @param planet any planet you happen to pick
    * but don't let it be jupiter.   Really, that's ridiculous
    * @batman Mr dean
    * @return pants 
    */
    """
    p = parse_kdoc_comment(comment)
    p.add_kdoc_param("tgt_name", "Xrl Target to handle command")
    s = p.output_kdoc("       ", ["tgt_name", "planet", "plig"], \
                      "This is not an original comment")
    print s
    
if __name__ == '__main__':
    test()
