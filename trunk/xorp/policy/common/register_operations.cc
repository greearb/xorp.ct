// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP$"

#include "config.h"
#include "register_operations.hh"
#include "dispatcher.hh"
#include "element.hh"
#include "elem_set.hh"
#include "regex.h"
#include "operator.hh"

#include <set>
#include <string>




namespace operations {

Element* op_not(const ElemBool& x) {
    return new ElemBool(!x.val());
}


// operations directly on element value
#define DEFINE_BINOP(name,op) \
template<class Result, class Left, class Right> \
Element* name(const Left& x, const Right& y) { \
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
template<class Result, class Left, class Right> \
Element* name(const Left& x, const Right& y) { \
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
template<class Result, class Left, class Right> \
Element* name(const Left& x, const Right& y) { \
    return new Result(y op x); \
}

DEFINE_BINOP_SWITCHPARAMS(op_eq_sw,==)
DEFINE_BINOP_SWITCHPARAMS(op_ne_sw,!=)

DEFINE_BINOP_SWITCHPARAMS(op_lt_sw,>)
DEFINE_BINOP_SWITCHPARAMS(op_gt_sw,<)

DEFINE_BINOP_SWITCHPARAMS(op_le_sw,>=)
DEFINE_BINOP_SWITCHPARAMS(op_ge_sw,<=)





template<class T>
Element* set_add(const ElemSet& s, const T& str) {
    ElemSet* es = new ElemSet(s.get_set());

    es->insert(str.str());

    return es;
}


// XXX: for now, the only writable string is aspath.
// in that case, addition means adding an aspath segment, so lets stick a comma
// in between. Ideally aspath should have it's own type.
// The problem is lexer does all type identification, so we would have to define
// ASPATH + STR, STR + ASPATH, ASPATH + ASPATH etc, to get an aspath type. And
// have the ability to assign a string to an aspath.
Element* str_add(const ElemStr& left, const ElemStr& right) {
    return new ElemStr(left.val() + "," + right.val());
}


} // namespace


// XXX: hack to compile on 2.95.x [may not use &operation::op... with templates]
using namespace operations;

// FIXME: no comment =D
// macro ugliness to specify possible operations
RegisterOperations::RegisterOperations() {
    Dispatcher disp;

    disp.add<ElemBool,&operations::op_not>(OpNot());

#define ADD_BINOP(result,left,right,funct,oper) \
disp.add<left,right,&funct<result,left,right> >(Op##oper());

    // boolean logic
    ADD_BINOP(ElemBool,ElemBool,ElemBool,op_and,And)
    ADD_BINOP(ElemBool,ElemBool,ElemBool,op_or,Or)
    ADD_BINOP(ElemBool,ElemBool,ElemBool,op_xor,Xor)

// EQUAL AND NOT EQUAL
#define ADD_EQOP(arg) \
    ADD_BINOP(ElemBool,arg,arg,op_eq,Eq) \
    ADD_BINOP(ElemBool,arg,arg,op_ne,Ne)

// RELATIONAL OPERATORS
#define ADD_RELOP(arg) \
    ADD_BINOP(ElemBool,arg,arg,op_lt,Lt) \
    ADD_BINOP(ElemBool,arg,arg,op_gt,Gt) \
    ADD_BINOP(ElemBool,arg,arg,op_le,Le) \
    ADD_BINOP(ElemBool,arg,arg,op_ge,Ge)
   
// MATH OPERATORS
#define ADD_MATHOP(arg) \
    ADD_BINOP(arg,arg,arg,op_add,Add) \
    ADD_BINOP(arg,arg,arg,op_sub,Sub) \
    ADD_BINOP(arg,arg,arg,op_mul,Mul)


    // SETS
    ADD_BINOP(ElemBool,ElemSet,ElemSet,op_eq_nv,Eq)
    ADD_BINOP(ElemBool,ElemSet,ElemSet,op_ne_nv,Ne)

    ADD_BINOP(ElemBool,ElemSet,ElemSet,op_lt_nv,Lt)
    ADD_BINOP(ElemBool,ElemSet,ElemSet,op_gt_nv,Gt)
    ADD_BINOP(ElemBool,ElemSet,ElemSet,op_le_nv,Le)
    ADD_BINOP(ElemBool,ElemSet,ElemSet,op_ge_nv,Ge)

    // SET ADDITION [used for policy tags] -- insert an element in the set.
    disp.add<ElemSet,ElemU32,operations::set_add>(OpAdd());

#define ADD_SETBINOP(arg) \
    ADD_BINOP(ElemBool,ElemSet,arg,op_eq_nv,Eq) \
    ADD_BINOP(ElemBool,ElemSet,arg,op_ne_nv,Ne) \
    ADD_BINOP(ElemBool,ElemSet,arg,op_lt_nv,Lt) \
    ADD_BINOP(ElemBool,ElemSet,arg,op_gt_nv,Gt) \
    ADD_BINOP(ElemBool,ElemSet,arg,op_le_nv,Le) \
    ADD_BINOP(ElemBool,ElemSet,arg,op_ge_nv,Ge) \
    ADD_BINOP(ElemBool,arg,ElemSet,op_eq_sw,Eq) \
    ADD_BINOP(ElemBool,arg,ElemSet,op_ne_sw,Ne) \
    ADD_BINOP(ElemBool,arg,ElemSet,op_lt_sw,Lt) \
    ADD_BINOP(ElemBool,arg,ElemSet,op_gt_sw,Gt) \
    ADD_BINOP(ElemBool,arg,ElemSet,op_le_sw,Le) \
    ADD_BINOP(ElemBool,arg,ElemSet,op_ge_sw,Ge) \





    // i32
    ADD_EQOP(ElemInt32)
    ADD_RELOP(ElemInt32)
    ADD_MATHOP(ElemInt32)
    ADD_SETBINOP(ElemInt32)

    // u32
    ADD_EQOP(ElemU32)
    ADD_RELOP(ElemU32)
    ADD_MATHOP(ElemU32)
    ADD_SETBINOP(ElemU32)


    // strings
    ADD_EQOP(ElemStr)
    ADD_SETBINOP(ElemStr)
    // string concatenation
    disp.add<ElemStr,ElemStr,operations::str_add>(OpAdd()); 


    // IPV4
    ADD_EQOP(ElemIPv4)
    ADD_RELOP(ElemIPv4)
    ADD_SETBINOP(ElemIPv4)
   
    // IPV4NET
    ADD_EQOP(ElemIPv4Net)
    ADD_RELOP(ElemIPv4Net)
    ADD_SETBINOP(ElemIPv4Net)
   
    // IPV6
    ADD_EQOP(ElemIPv6)
    ADD_RELOP(ElemIPv6)
    ADD_SETBINOP(ElemIPv6)
   
    // IPV6NET
    ADD_EQOP(ElemIPv6Net)
    ADD_RELOP(ElemIPv6Net)
    ADD_SETBINOP(ElemIPv6Net)

}
