""" C++ generic callback template generator

Notes:

This program generates types and methods for creating and invoking
callbacks in C++ with arbitrary bound arguments and late arguments
passed in.

At present the generated code assumes that returning void is permitted
by the compiler.  Some additional hacking will be needed to support
the specializations necessary when this is not the case, but on our
main build system this is not an issue.

The notion of having a script to generate templates was borrowed from
David Mazieres callback.h file (in the async lib distributed with
sfs).  The code generated bears strong similarities to David's work, but
was largely written independently of it.

"""

import getopt, sys, string

def aname(type):
    """Return arg name derived from type"""
    return type.lower()

def decl_arg(t):
    return "%s %s" % (t, aname(t))

def decl_args(types):
    return map(decl_arg, types)

def mem_arg(t):
    return "_%s" % aname(t)

def mem_args(types):
    return map(mem_arg, types)

def mem_decl(t):
    return "%s %s" % (t, mem_arg(t))

def mem_decls(types):
    return map(mem_decl, types)

def cons_arg(t):
    return "%s(%s)" % (mem_arg(t), aname(t))

def cons_args(types):
    return map(cons_arg, types)

def call_args(types):
    return map(aname, types)

def class_arg(t):
    return "class %s" % t

def class_args(types):
    return map(class_arg, types)

def first_arg(args):
    return args[0]

def first_args(tuple_list):
    return map(first_arg, tuple_list)

def second_arg(args):
    return args[1]

def second_args(tuple_list):
    return map(second_arg, tuple_list)

def flatten_pair(p):
    return p[0] + " " + p[1]

def flatten_pair_list(tuple_list):
    return map(flatten_pair, tuple_list)

def csv(l, comma = ", "):
    """
    Transform list of strings into a comma separated values string
    """
    s = ''
    n = len(l)
    if (n >= 1):
        s = "%s" % l[0]
    for i in range(1,n):
        s += "%s%s" % (comma, l[i])
    return s;

def joining_csv(l, comma = ", "):
    """
    Transform list of strings into a comma separated values string suitable
    for appending to an existing comma separated value string.
    """
    s = ''
    n = len(l)
    for i in range(0,n):
        s += "%s%s" % (comma, l[i])
    return s;

def starting_csv(l, comma = ", "):
    """
    Starts a comma separated list.  Last element is follow by
    comma on the assumption that the list will continue after
    l.
    """
    s = ''
    n = len(l)
    for i in range(0,n):
        s += "%s%s" % (l[i], comma)
    return s;

def output_header(args, dbg):
    from time import time, localtime, strftime
    print \
"""/*
 * Copyright (c) 2001-2003 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * This file is PROGRAMMATICALLY GENERATED.
 *"""
    print " * This instance was generated with:\n *     ",
    for a in args:
        print "%s" % a,
    print "\n */"

    print """
/**
 * @libdoc Callbacks
 *
 * @sect Callback Overview
 *
 * XORP is an asynchronous programming environment and as a result there
 * are many places where callbacks are useful.  Callbacks are typically
 * invoked to signify the completion or advancement of an asynchronous
 * operation.
 *
 * XORP provides a generic and flexible callback interface that utilizes
 * overloaded templatized functions for for generating callbacks
 * in conjunction with many small templatized classes. Whilst this makes
 * the syntax a little ugly, it provides a great deal of flexibility.
 *
 * XorpCallbacks are callback objects are created by the callback()
 * function which returns a reference pointer to a newly created callback
 * object.  The callback is invoked by calling dispatch(), eg.
 *
<pre>

#include <iostream>

#include "config.h"
#include "libxorp/callback.hh"

static void hello_world() {
    cout << "Hello World" << endl;
}

int main() {
    // Typedef callback() return type for readability.  SimpleCallback
    // declares a XorpCallback taking 0 arguments and of return type void.
    typedef XorpCallback0<void>::RefPtr SimpleCallback;

    // Create XorpCallback object using callback()
    SimpleCallback cb = callback(hello_world);

    // Invoke callback, results in call to hello_world.
    cb->dispatch();
    return 0;
}

</pre>
 *
 * The callback() method is overloaded and can also be used to create
 * callbacks to member functions, eg.
 *
<pre>

#include <iostream>

#include "config.h"
#include "libxorp/callback.hh"

class Foo {
public:
    void hello_world() {
	cout << "Foo::Hello World" << endl;
    }
};

int main() {
    typedef XorpCallback0<void>::RefPtr SimpleCallback;

    Foo f;

    // Create a callback to a member function
    SimpleCallback cb = callback(&f, &Foo::hello_world);

    // Invoke f.hello_world
    cb->dispatch();

    return 0;
}

</pre>
 *
 * In addition, to being able to invoke member functions, callbacks can
 * also store arguments to functions. eg.
 *
<pre>

#include <iostream>

#include "config.h"
#include "libxorp/callback.hh"

static int sum(int x, int y) {
    cout << "sum(x = " << x << ", y = " << y << ")" << endl;
    return x + y;
}

int main() {
    // Callback to function returning "int"
    typedef XorpCallback0<int>::RefPtr NoArgCallback;

    NoArgCallback cb1 = callback(sum, 1, 2);
    cout << "cb1->dispatch() returns " << cb1->dispatch() << endl; // "3"
    cout << endl;

    // Callback to function returning int and taking an integer argument
    typedef XorpCallback1<int,int>::RefPtr OneIntArgCallback;

    OneIntArgCallback cb2 = callback(sum, 5);
    cout << "cb2->dispatch(10) returns " << cb2->dispatch(10) << endl; // 15
    cout << endl;

    cout << "cb2->dispatch(20) returns " << cb2->dispatch(20) << endl; // 25
    cout << endl;

    // Callback to function returning int and taking  2 integer arguments
    typedef XorpCallback2<int,int,int>::RefPtr TwoIntArgCallback;

    TwoIntArgCallback cb3 = callback(sum);
    cout << "cb3->dispatch() returns " << cb3->dispatch(50, -50) << endl; // 0

    return 0;
}

</pre>
 *
 * Bound arguments, as with member functions, are implemented by the
 * overloading of the callback() method.  At dispatch time, the bound
 * arguments are last arguments past to the wrappered function.  If you
 * compile and run the program you will see:
 *
<pre>
sum(x = 10, y = 5)
cb2->dispatch(10) returns 15
</pre>
 *
 * and:
 *
<pre>
sum(x = 20, y = 5)
cb2->dispatch(20) returns 25
</pre>
 *
 * for the one bound argument cases.
 *
 * @sect Declaring Callback Types
 *
 * There are a host of XorpCallbackN types.  The N denotes the number
 * of arguments that will be passed at dispatch time by the callback
 * invoker.  The template parameters to XorpCallbackN types are the
 * return value followed by the types of arguments that will be passed
 * at dispatch time.  Thus type:
 *
 * <pre>
XorpCallback1<double, int>::RefPtr
 * </pre>
 *
 * corresponds to callback object returning a double when invoked and
 * requiring an integer argument to passed at dispatch time.
 *
 * When arguments are bound to a callback they are not specified
 * in the templatized argument list. So the above declaration is good
 * for a function taking an integer argument followed by upto the
 * maximum number of bound arguments.
 *
 * Note: In this header file, support is provided for upto %d bound
 * arguments and %d dispatch arguments.
 *
 * @sect Ref Pointer Helpers
 *
 * Callback objects may be set to NULL, since they use reference pointers
 * to store the objects.  Callbacks may be unset using the ref_ptr::release()
 * method:
 *
<pre>
    cb.release();
</pre>
 * and to tested using the ref_ptr::is_empty() method:
<pre>
if (! cb.is_empty()) {
    cb->dispatch();
}
</pre>
 *
 * In many instances, the RefPtr associated with a callback on an object
 * will be stored by the object itself.  For instance, a class may own a
 * timer object and the associated timer expiry callback which is
 * a member function of the containing class.  Because the containing class
 * owns the callback object corresponding the timer callback, there is
 * never an opportunity for the callback to be dispatched on a deleted object
 * or with invalid data.
 */
"""

    print """
#ifndef __XORP_CALLBACK_HH__
#define __XORP_CALLBACK_HH__

#include "minitraits.hh"
#include "ref_ptr.hh"
#include "safe_callback_obj.hh"
"""
    if (dbg):
        print \
"""
#if defined(__GNUC__) && (__GNUC__ < 3)
#define callback(x...) dbg_callback(__FILE__,__LINE__,x)
#else
#define callback(...) dbg_callback(__FILE__,__LINE__,__VA_ARGS__)
#endif
"""

def output_trailer():
    print "#endif /* __XORP_CALLBACK_HH__ */"

def output_kdoc_base_class(nl):
    print "/**"
    print " * @short Base class for callbacks with %d dispatch time args." % nl
    print " */"

def output_kdoc_class(target, nl, nb):
    if (target != ''):
        target = target.strip() + ' '
    print "/**"
    print " * @short Callback object for %swith %d dispatch time" % (target, nl)
    print " * arguments and %d bound (stored) arguments." % nb
    print " */"

def output_kdoc_factory_function(target, nl, nb):
    if (target != ''):
        target = target.strip() + ' '
    print "/**"
    print " * Factory function that creates a callback object targetted at a"
    print " * %swith %d dispatch time arguments and %d bound arguments." % (target, nl, nb)
    print " */"

def output_base(l_types, dbg):
    n = len(l_types)

    print "///////////////////////////////////////////////////////////////////////////////"
    print "//"
    print "// Code relating to callbacks with %d late args" % n
    print "//"
    print
    output_kdoc_base_class(n)
    print "template<class R%s>" % joining_csv(class_args(l_types))
    print "struct XorpCallback%d {" % n
    print "    typedef ref_ptr<XorpCallback%d> RefPtr;\n" % n
    if (dbg):
        print "    XorpCallback%d(const char* file, int line)" % n
        print "\t: _file(file), _line(line) {}"
    print "    virtual ~XorpCallback%d() {}" % n
    print "    virtual R dispatch(%s)" % csv(l_types) + " = 0;"

    if (dbg):
        print "    const char* file() const\t\t{ return _file; }"
        print "    int line() const\t\t\t{ return _line; }"
        print "private:"
        print "    const char* _file;"
        print "    int         _line;"
    print "};\n"

def output_rest(l_types, b_types, dbg):
    nl = len(l_types)
    nb = len(b_types)

    base_class = "XorpCallback%d<R%s>" % (nl, joining_csv(l_types))
    if (dbg):
        debug_args = (("const char*", "file"),
                                 ("int", "line"))
    else:
        debug_args = (())

    output_kdoc_class("functions", nl, nb)

    o = "template <class R%s>\n" % \
        joining_csv(class_args(l_types) + class_args(b_types))
    o += "struct XorpFunctionCallback%dB%d : public %s {\n" \
          % (nl, nb, base_class)
    o += "    typedef R (*F)(%s);\n" % csv(l_types + b_types)
    o += "    XorpFunctionCallback%dB%d(" % (nl, nb)
    o += starting_csv(flatten_pair_list(debug_args))
    o += "F f%s)\n\t: %s(%s),\n\t  _f(f)%s\n    {}\n" \
          % (joining_csv(decl_args(b_types)),
             base_class,
             csv(second_args(debug_args)),
             joining_csv(cons_args(b_types)))
    o += "    R dispatch(%s) { return (*_f)(%s); }\n" \
         % (csv(decl_args(l_types)),
            csv(call_args(l_types) + mem_args(b_types)))
    o += "protected:\n    F   _f;\n"
    for ba in mem_decls(b_types):
        o += "    %s;\n" % ba
    o += "};\n"
    print o
    print ''

    output_kdoc_factory_function("function", nl, nb)
    o  = "template <class R%s>\n" % joining_csv(class_args(l_types + b_types))
    o += "typename XorpCallback%d<R%s>::RefPtr\n" % (nl, joining_csv(l_types))
    if (dbg):
        o += "dbg_"
    o += "callback("
    o += starting_csv(flatten_pair_list(debug_args))
    o += "R (*f)(%s)%s) {\n" % (csv(l_types + b_types), joining_csv(decl_args(b_types)))
    o += "    return XorpCallback%d<R%s>::RefPtr(new XorpFunctionCallback%dB%d<R%s>(%sf%s));\n" \
          % (nl, joining_csv(l_types), nl, nb, joining_csv(l_types + b_types),
             starting_csv(second_args(debug_args)),
             joining_csv(call_args(b_types)))
    o += "}"
    print o
    print

    for CONST,const in [('',''), ('Const', ' const')]:
        output_kdoc_class("%s member methods" % const, nl, nb)
        o = ""
        o += "template <class R, class O%s>\n" % (joining_csv(class_args(l_types + b_types)))
        o += "struct Xorp%sMemberCallback%dB%d : public %s {\n" \
              % (CONST, nl, nb, base_class)
        o += "    typedef R (O::*M)(%s) %s;\n" % (csv(l_types + b_types), const)
        o += "    Xorp%sMemberCallback%dB%d(" % (CONST, nl, nb)
        o += starting_csv(flatten_pair_list(debug_args))
        o += "O* o, M m%s)\n\t :" % joining_csv(decl_args(b_types))
        o += " %s(%s),\n\t  " % (base_class, csv(second_args(debug_args)))
        o += "_o(o), _m(m)%s {}\n" % joining_csv(cons_args(b_types))
        o += "    R dispatch(%s) { return ((*_o).*_m)(%s); }\n" \
             % (csv(decl_args(l_types)), csv(call_args(l_types) + mem_args(b_types)))
        o += "protected:\n"
        o += "    O*	_o;	// Callback's target object\n"
        o += "    M	_m;	// Callback's target method\n"
        for ba in mem_decls(b_types):
            o += "    %s;	// Bound argument\n" % ba
        o += "};\n"
        print o

        output_kdoc_class("%s safe member methods" % const, nl, nb)
        o = ""
        o += "template <class R, class O%s>\n" % (joining_csv(class_args(l_types + b_types)))
        o += "struct Xorp%sSafeMemberCallback%dB%d\n" % (CONST, nl, nb)
        o += "    : public Xorp%sMemberCallback%dB%d<R, O%s>,\n" % (CONST, nl, nb, (joining_csv(l_types + b_types)))
        o += "      public SafeCallbackBase {\n"
        o += "    typedef typename Xorp%sMemberCallback%dB%d<R, O%s>::M M;\n" % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += "    Xorp%sSafeMemberCallback%dB%d(" % (CONST, nl, nb)
        o += starting_csv(flatten_pair_list(debug_args))
        o += "O* o, M m%s)\n\t : " % joining_csv(decl_args(b_types))
        o += "Xorp%sMemberCallback%dB%d<R, O%s>(" % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += starting_csv(second_args(debug_args))
        o += "o, m"
        o += joining_csv(call_args(b_types))
        o += "),\n\t   SafeCallbackBase(o) {}\n"
        o += "    CallbackSafeObject* base()		{ return _o; }\n"
        o += "    void invalidate()			{ _o = 0; }\n"
        o += "    R dispatch(%s) {\n" % (csv(decl_args(l_types)))
        o += "\tif (base())\n"
        o += "\t    return Xorp%sMemberCallback%dB%d<R, O%s>::dispatch(%s);\n" % (CONST, nl, nb, joining_csv(l_types + b_types), csv(call_args(l_types)))
        o += "    }\n"
        o += "};\n"
        print o

        o = ""
        o += "template <class R, class O%s, bool B=true>\n" \
             % (joining_csv(class_args(l_types) + class_args(b_types)))
        o += "struct Xorp%sMemberCallbackFactory%dB%d\n" % (CONST, nl, nb)
        o += "{\n"
        o += "    inline static Xorp%sMemberCallback%dB%d<R, O%s>*\n" \
             % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += "    make(%s" % starting_csv(flatten_pair_list(debug_args))
        o += "O* o, R (O::*p)(%s)%s%s)\n" \
             % (csv(l_types + b_types), const, joining_csv(decl_args(b_types)))
        o += "    {\n"
        o += "\treturn new Xorp%sSafeMemberCallback%dB%d<R, O%s>(" \
             % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += starting_csv(second_args(debug_args))
        o += "o, p%s);\n" % joining_csv(call_args(b_types))
        o += "    }\n"
        o += "};\n"
        o += "\n"
        o += "template <class R, class O%s>\n" \
             % (joining_csv(class_args(l_types) + class_args(b_types)))
        o += "struct Xorp%sMemberCallbackFactory%dB%d<R, O%s, false>\n" \
             % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += "{\n"
        o += "    inline static Xorp%sMemberCallback%dB%d<R, O%s>*\n" \
             % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += "    make(%s" % starting_csv(flatten_pair_list(debug_args))
        o += "O* o, R (O::*p)(%s)%s%s)\n" \
             % (csv(l_types + b_types), const, joining_csv(decl_args(b_types)))
        o += "    {\n"
        o += "\treturn new Xorp%sMemberCallback%dB%d<R, O%s>(" \
             % (CONST, nl, nb, joining_csv(l_types + b_types))
        o += starting_csv(second_args(debug_args))
        o += "o, p%s);\n" % joining_csv(call_args(b_types))
        o += "    };\n"
        o += "};\n"
        print o

        for p,q in [('*', ''), ('&', '&')]:
            output_kdoc_factory_function("%s member function" % const, nl, nb)

            o  = "template <class R, class O%s> typename " \
                      % joining_csv(class_args(l_types) + class_args(b_types))
            o += "XorpCallback%s<R%s>::RefPtr\n"  %  (nl, joining_csv(l_types))
            if (dbg):
                o += "dbg_"
            o += "callback("
            o += starting_csv(flatten_pair_list(debug_args))
            o += "O%s o, R (O::*p)(%s)%s%s)\n" \
                 % (p, csv(l_types + b_types), const, joining_csv(decl_args(b_types)))
            o += "{\n"
            o += "    return Xorp%sMemberCallbackFactory%dB%d<" % (CONST, nl, nb)
            o += "R, O%s, BaseAndDerived<CallbackSafeObject, O>::True>::make(" % joining_csv(l_types + b_types)
            o += starting_csv(second_args(debug_args))
            o += "%so, p%s);\n" % (q, joining_csv(call_args(b_types)))
            o += "}\n"
            print o
        print ''

def cb_gen(max_bound, max_late, dbg):
    l_types = []
    for l in range(0, max_late):
        if (l):
            l_types.append("A%d" % l);
        output_base(l_types, dbg)

        b_types = []
        for b in range (0, max_bound):
            if (b):
                b_types.append("BA%d" % b)
            output_rest(l_types, b_types, dbg)

us = \
"""Usage: %s [options]
   -h                  Display usage information
   -n		       No debug information (no file and line info)
   -b <B>              Set maximum number of bound argument
   -l <L>              Set maximum number of late arguments"""

def main():
    def usage():
        print us % sys.argv[0]

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hnb:l:",\
                                   ["help",  "nodebug", "bound-args=", "late-args="])
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    nb = 4
    nl = 4
    dbg = 1

    for o, a in opts:
        if (o in ("-h", "--help")):
            usage()
            sys.exit()
        if (o in ("-b", "--bound-args")):
            nb = int(a)
        if (o in ("-l", "--late-args")):
            nl = int(a)
        if (o in ("-n", "--nodebug")):
            dbg = 0

    output_header(sys.argv[:], dbg)

    cb_gen(nb + 1, nl + 1, dbg)

    output_trailer()

if __name__ == '__main__':
    main()
