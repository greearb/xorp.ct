// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/policy/common/register_operations.cc,v 1.5 2005/07/13 21:58:40 abittau Exp $"

#include "config.h"
#include "register_operations.hh"
#include "dispatcher.hh"
#include "element.hh"
#include "elem_set.hh"
#include "elem_null.hh"
#include "operator.hh"
#include "element_factory.hh"
#include "policy_utils.hh"
#include <set>
#include <string>

namespace operations {

// Unary operations
Element* 
op_not(const ElemBool& x)
{
    return new ElemBool(!x.val());
}

Element* 
op_head(const ElemStr& x)
{
    const string& str = x.val();

    string::size_type pos = str.find(',', 0);

    // try again
    if (pos == string::npos)
	pos = str.find(' ', 0);

    // return whole thing... [if space and comma not found]

    return new ElemStr(str.substr(0, pos));
}

// operations directly on element value
#define DEFINE_BINOP(name,op) \
template<class Result, class Left, class Right> \
Element* name(const Left& x, const Right& y) \
{ \
    return new Result(x.val() op y.val()); \
}

DEFINE_BINOP(op_and,&&)
DEFINE_BINOP(op_or,||)
DEFINE_BINOP(op_xor,^)

DEFINE_BINOP(op_eq,==)
DEFINE_BINOP(op_ne,!=)

DEFINE_BINOP(op_lt,<)
DEFINE_BINOP(op_gt,>)
DEFINE_BINOP(op_le,<=)
DEFINE_BINOP(op_ge,>=)

DEFINE_BINOP(op_add,+)
DEFINE_BINOP(op_sub,-)
DEFINE_BINOP(op_mul,*)

// Operations for which .val() is not needed. [operation performed on element
// itself].
#define DEFINE_BINOP_NOVAL(name,op) \
template <class Result, class Left, class Right> \
Element* \
name(const Left& x, const Right& y) { \
    return new Result(x op y); \
}

DEFINE_BINOP_NOVAL(op_eq_nv,==)
DEFINE_BINOP_NOVAL(op_ne_nv,!=)

DEFINE_BINOP_NOVAL(op_lt_nv,<)
DEFINE_BINOP_NOVAL(op_gt_nv,>)
DEFINE_BINOP_NOVAL(op_le_nv,<=)
DEFINE_BINOP_NOVAL(op_ge_nv,>=)

// useful for equivalences where we want one class to deal with the operation.
// For example we want sets to know about elements, but not elements to know
// about sets.
// so Element < Set will normally be interpreted as Element::operator< but we do
// not want that... so we switch the parameters and obtain
// Set::operator>(Element)
#define DEFINE_BINOP_SWITCHPARAMS(name,op) \
template <class Result, class Left, class Right> \
Element* \
name(const Left& x, const Right& y) \
{ \
    return new Result(y op x); \
}

DEFINE_BINOP_SWITCHPARAMS(op_eq_sw,==)
DEFINE_BINOP_SWITCHPARAMS(op_ne_sw,!=)

DEFINE_BINOP_SWITCHPARAMS(op_lt_sw,>)
DEFINE_BINOP_SWITCHPARAMS(op_gt_sw,<)

DEFINE_BINOP_SWITCHPARAMS(op_le_sw,>=)
DEFINE_BINOP_SWITCHPARAMS(op_ge_sw,<=)

// 2 template parameters because U can be T or ElemSetAny<T>
template <class T, class U>
Element* 
set_add(const ElemSetAny<T>& s, const U& e)
{
    ElemSetAny<T>* es = new ElemSetAny<T>();

    es->insert(s);
    es->insert(e);

    return es;
}

template <class T>
Element*
set_del(const ElemSetAny<T>& s, const ElemSetAny<T>& r)
{
    ElemSetAny<T>* es = new ElemSetAny<T>();
    es->insert(s);
    es->erase(r);
    return es;
}

template <class T>
Element* 
set_ne_int(const ElemSetAny<T>& l, const ElemSetAny<T>& r)
{
    return new ElemBool(l.nonempty_intersection(r));
}

Element* 
str_add(const ElemStr& left, const ElemStr& right)
{
    return new ElemStr(left.val() + right.val());
}

Element* 
str_mul(const ElemStr& left, const ElemU32& right)
{
    string str = left.val();
    string res = "";
    uint32_t times = right.val();

    for (uint32_t i = 0; i < times; ++i)
	res.append(str);

    return new ElemStr(res);
}

Element* 
ctr(const ElemStr& type, const Element& arg)
{
    ElementFactory ef;

    return ef.create(type.val(), arg.str().c_str());
}

Element*
str_regex(const ElemStr& left, const ElemStr& right)
{
    return new ElemBool(policy_utils::regex(left.val(), right.val()));
}

Element*
str_setregex(const ElemStr& left, const ElemSetStr& right)
{
    string str = left.val();

    // go through all regexps...
    // only 1 needs to match...
    for (ElemSetStr::const_iterator i = right.begin(); i != right.end(); ++i) {
	const ElemStr& re = *i;

	if (policy_utils::regex(str, re.val()))
	    return new ElemBool(true);
    }

    return new ElemBool(false);
}

} // namespace

// XXX: hack to compile on 2.95.x [may not use &operation::op... with templates]
using namespace operations;

// FIXME: no comment =D
// macro ugliness to specify possible operations
RegisterOperations::RegisterOperations()
{
    Dispatcher disp;

    disp.add<ElemBool,&operations::op_not>(OpNot());
    disp.add<ElemStr, &operations::op_head>(OpHead());

#define ADD_BINOP(result,left,right,funct,oper)				\
do {									\
	disp.add<left,right,&funct<result,left,right> >(Op##oper());	\
} while (0)

    // boolean logic
    ADD_BINOP(ElemBool,ElemBool,ElemBool,op_and,And);
    ADD_BINOP(ElemBool,ElemBool,ElemBool,op_or,Or);
    ADD_BINOP(ElemBool,ElemBool,ElemBool,op_xor,Xor);

// EQUAL AND NOT EQUAL
#define ADD_EQOP(arg)							\
do {									\
    ADD_BINOP(ElemBool,arg,arg,op_eq,Eq);				\
    ADD_BINOP(ElemBool,arg,arg,op_ne,Ne);				\
} while (0)

// RELATIONAL OPERATORS
#define ADD_RELOP(arg)							\
do {									\
    ADD_BINOP(ElemBool,arg,arg,op_lt,Lt);				\
    ADD_BINOP(ElemBool,arg,arg,op_gt,Gt);				\
    ADD_BINOP(ElemBool,arg,arg,op_le,Le);				\
    ADD_BINOP(ElemBool,arg,arg,op_ge,Ge);				\
} while (0)
   
// MATH OPERATORS
#define ADD_MATHOP(arg)							\
do {									\
    ADD_BINOP(arg,arg,arg,op_add,Add);					\
    ADD_BINOP(arg,arg,arg,op_sub,Sub);					\
    ADD_BINOP(arg,arg,arg,op_mul,Mul);					\
} while (0)

    // SET ADDITION [used for policy tags] -- insert an element in the set.
    disp.add<ElemSetU32, ElemU32, operations::set_add>(OpAdd());
   
    // SET operations [used for communities in BGP for example].
    disp.add<ElemSetU32, ElemSetU32, operations::set_ne_int>(OpNEInt());
    disp.add<ElemSetU32, ElemSetU32, operations::set_add>(OpAdd());
    disp.add<ElemSetU32, ElemSetU32, operations::set_del>(OpSub());

#define ADD_LSETBINOP(set, arg)						\
do {									\
    ADD_BINOP(ElemBool,set,arg,op_eq_nv,Eq);			        \
    ADD_BINOP(ElemBool,set,arg,op_ne_nv,Ne);   			        \
    ADD_BINOP(ElemBool,set,arg,op_lt_nv,Lt);				\
    ADD_BINOP(ElemBool,set,arg,op_gt_nv,Gt);				\
    ADD_BINOP(ElemBool,set,arg,op_le_nv,Le);				\
    ADD_BINOP(ElemBool,set,arg,op_ge_nv,Ge);				\
} while(0)

    ADD_LSETBINOP(ElemSetU32, ElemSetU32);

#define ADD_SETBINOP(set, arg)                                          \
do {                                                                    \
    ADD_LSETBINOP(set, arg);                                            \
    ADD_BINOP(ElemBool,arg,set,op_eq_sw,Eq);				\
    ADD_BINOP(ElemBool,arg,set,op_ne_sw,Ne);				\
    ADD_BINOP(ElemBool,arg,set,op_lt_sw,Lt);				\
    ADD_BINOP(ElemBool,arg,set,op_gt_sw,Gt);				\
    ADD_BINOP(ElemBool,arg,set,op_le_sw,Le);				\
    ADD_BINOP(ElemBool,arg,set,op_ge_sw,Ge);				\
} while (0)

    // i32
    ADD_EQOP(ElemInt32);
    ADD_RELOP(ElemInt32);
    ADD_MATHOP(ElemInt32);
//    ADD_SETBINOP(ElemInt32);

    // u32
    ADD_EQOP(ElemU32);
    ADD_RELOP(ElemU32);
    ADD_MATHOP(ElemU32);
    ADD_SETBINOP(ElemSetU32, ElemU32);

    // strings
    ADD_EQOP(ElemStr);
//    ADD_SETBINOP(ElemStr);
    disp.add<ElemStr, ElemStr, operations::str_add>(OpAdd());
    disp.add<ElemStr, ElemU32, operations::str_mul>(OpMul());
    disp.add<ElemStr, ElemStr, operations::str_regex>(OpRegex());
    disp.add<ElemStr, ElemSetStr, operations::str_setregex>(OpRegex());

    // IPV4
    ADD_EQOP(ElemIPv4);
    ADD_RELOP(ElemIPv4);
//    ADD_SETBINOP(ElemIPv4);
   
    // IPV4NET
    ADD_EQOP(ElemIPv4Net);
    ADD_RELOP(ElemIPv4Net);
    ADD_SETBINOP(ElemSetIPv4Net, ElemIPv4Net);
   
    // IPV6
    ADD_EQOP(ElemIPv6);
    ADD_RELOP(ElemIPv6);
//    ADD_SETBINOP(ElemIPv6);
   
    // IPV6NET
    ADD_EQOP(ElemIPv6Net);
    ADD_RELOP(ElemIPv6Net);
//    ADD_SETBINOP(ElemIPv6Net);
}
