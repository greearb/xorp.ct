/*
 * Copyright (c) 2001-2003 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * This file is PROGRAMMATICALLY GENERATED.
 * 
 * This instance was generated with:
 *      /home/puma/u0/hodson/src/xorp/libxorp/callback-gen.py -b 6 -l 13 
 */

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


#ifndef __XORP_CALLBACK_HH__
#define __XORP_CALLBACK_HH__

#include "ref_ptr.hh"


#if defined(__GNUC__) && (__GNUC__ < 3)
#  define callback(x...) dbg_callback(__FILE__,__LINE__,x)
#else
#  define callback(...) dbg_callback(__FILE__,__LINE__,__VA_ARGS__)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 0 late args
//

/**
 * @short Base class for callbacks with 0 dispatch time args.
 */
template<class R>
struct XorpCallback0 {
    typedef ref_ptr<XorpCallback0> RefPtr;

    XorpCallback0(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback0() {}
    virtual R dispatch() = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R>
struct XorpFunctionCallback0B0 : public XorpCallback0<R> {
    typedef R (*F)();
    XorpFunctionCallback0B0(const char* file, int line, F f)
	: XorpCallback0<R>(file, line),
	  _f(f) 
    {}
    R dispatch() { return (*_f)(); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 0 bound arguments.
 */
template <class R>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)()) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B0<R>(file, line, f));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O>
struct XorpMemberCallback0B0 : public XorpCallback0<R> {
    typedef R (O::*M)() ;
    XorpMemberCallback0B0(const char* file, int line, O *o, M m)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m) {}
    R dispatch() { return ((*_o).*_m)(); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)())
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B0<R, O>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)())
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B0<R, O>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O>
struct XorpConstMemberCallback0B0 : public XorpCallback0<R> {
    typedef R (O::*M)()  const;
    XorpConstMemberCallback0B0(const char* file, int line, O *o, M m)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m) {}
    R dispatch() { return ((*_o).*_m)(); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)() const)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B0<R, O>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)() const)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B0<R, O>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class BA1>
struct XorpFunctionCallback0B1 : public XorpCallback0<R> {
    typedef R (*F)(BA1);
    XorpFunctionCallback0B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback0<R>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch() { return (*_f)(_ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 1 bound arguments.
 */
template <class R, class BA1>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)(BA1), BA1 ba1) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B1<R, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class BA1>
struct XorpMemberCallback0B1 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1) ;
    XorpMemberCallback0B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch() { return ((*_o).*_m)(_ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class BA1> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1), BA1 ba1)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B1<R, O, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class BA1> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1), BA1 ba1)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B1<R, O, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class BA1>
struct XorpConstMemberCallback0B1 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1)  const;
    XorpConstMemberCallback0B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch() { return ((*_o).*_m)(_ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class BA1> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1) const, BA1 ba1)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B1<R, O, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class BA1> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1) const, BA1 ba1)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B1<R, O, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class BA1, class BA2>
struct XorpFunctionCallback0B2 : public XorpCallback0<R> {
    typedef R (*F)(BA1, BA2);
    XorpFunctionCallback0B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback0<R>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch() { return (*_f)(_ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 2 bound arguments.
 */
template <class R, class BA1, class BA2>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)(BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B2<R, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2>
struct XorpMemberCallback0B2 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2) ;
    XorpMemberCallback0B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class BA1, class BA2> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B2<R, O, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class BA1, class BA2> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B2<R, O, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2>
struct XorpConstMemberCallback0B2 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2)  const;
    XorpConstMemberCallback0B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class BA1, class BA2> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B2<R, O, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class BA1, class BA2> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B2<R, O, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class BA1, class BA2, class BA3>
struct XorpFunctionCallback0B3 : public XorpCallback0<R> {
    typedef R (*F)(BA1, BA2, BA3);
    XorpFunctionCallback0B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback0<R>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch() { return (*_f)(_ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 3 bound arguments.
 */
template <class R, class BA1, class BA2, class BA3>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)(BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B3<R, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3>
struct XorpMemberCallback0B3 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3) ;
    XorpMemberCallback0B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B3<R, O, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B3<R, O, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback0B3 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3)  const;
    XorpConstMemberCallback0B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B3<R, O, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B3<R, O, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback0B4 : public XorpCallback0<R> {
    typedef R (*F)(BA1, BA2, BA3, BA4);
    XorpFunctionCallback0B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback0<R>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch() { return (*_f)(_ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 4 bound arguments.
 */
template <class R, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)(BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B4<R, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback0B4 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3, BA4) ;
    XorpMemberCallback0B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B4<R, O, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B4<R, O, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback0B4 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback0B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B4<R, O, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B4<R, O, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback0B5 : public XorpCallback0<R> {
    typedef R (*F)(BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback0B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback0<R>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch() { return (*_f)(_ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 5 bound arguments.
 */
template <class R, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)(BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B5<R, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback0B5 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback0B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B5<R, O, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B5<R, O, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback0B5 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback0B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B5<R, O, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B5<R, O, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 0 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback0B6 : public XorpCallback0<R> {
    typedef R (*F)(BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback0B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback0<R>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch() { return (*_f)(_ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 0 dispatch time arguments and 6 bound arguments.
 */
template <class R, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, R (*f)(BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback0<R>::RefPtr(new XorpFunctionCallback0B6<R, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 0 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback0B6 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback0B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B6<R, O, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 0 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback0<R>::RefPtr(new XorpMemberCallback0B6<R, O, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 0 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback0B6 : public XorpCallback0<R> {
    typedef R (O::*M)(BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback0B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback0<R>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch() { return ((*_o).*_m)(_ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B6<R, O, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 0 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback0<R>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback0<R>::RefPtr(new XorpConstMemberCallback0B6<R, O, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 1 late args
//

/**
 * @short Base class for callbacks with 1 dispatch time args.
 */
template<class R, class A1>
struct XorpCallback1 {
    typedef ref_ptr<XorpCallback1> RefPtr;

    XorpCallback1(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback1() {}
    virtual R dispatch(A1) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1>
struct XorpFunctionCallback1B0 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1);
    XorpFunctionCallback1B0(const char* file, int line, F f)
	: XorpCallback1<R, A1>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1)) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B0<R, A1>(file, line, f));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1>
struct XorpMemberCallback1B0 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1) ;
    XorpMemberCallback1B0(const char* file, int line, O *o, M m)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1))
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B0<R, O, A1>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1))
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B0<R, O, A1>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1>
struct XorpConstMemberCallback1B0 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1)  const;
    XorpConstMemberCallback1B0(const char* file, int line, O *o, M m)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1) const)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B0<R, O, A1>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1) const)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B0<R, O, A1>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class BA1>
struct XorpFunctionCallback1B1 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1, BA1);
    XorpFunctionCallback1B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback1<R, A1>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class BA1>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, BA1), BA1 ba1) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B1<R, A1, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1>
struct XorpMemberCallback1B1 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1) ;
    XorpMemberCallback1B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class BA1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1), BA1 ba1)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B1<R, O, A1, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class BA1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1), BA1 ba1)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B1<R, O, A1, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1>
struct XorpConstMemberCallback1B1 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1)  const;
    XorpConstMemberCallback1B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class BA1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1) const, BA1 ba1)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B1<R, O, A1, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class BA1> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1) const, BA1 ba1)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B1<R, O, A1, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class BA1, class BA2>
struct XorpFunctionCallback1B2 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1, BA1, BA2);
    XorpFunctionCallback1B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback1<R, A1>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class BA1, class BA2>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B2<R, A1, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2>
struct XorpMemberCallback1B2 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2) ;
    XorpMemberCallback1B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B2<R, O, A1, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B2<R, O, A1, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2>
struct XorpConstMemberCallback1B2 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2)  const;
    XorpConstMemberCallback1B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B2<R, O, A1, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B2<R, O, A1, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3>
struct XorpFunctionCallback1B3 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1, BA1, BA2, BA3);
    XorpFunctionCallback1B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback1<R, A1>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B3<R, A1, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3>
struct XorpMemberCallback1B3 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3) ;
    XorpMemberCallback1B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B3<R, O, A1, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B3<R, O, A1, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback1B3 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3)  const;
    XorpConstMemberCallback1B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B3<R, O, A1, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B3<R, O, A1, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback1B4 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1, BA1, BA2, BA3, BA4);
    XorpFunctionCallback1B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback1<R, A1>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B4<R, A1, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback1B4 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback1B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B4<R, O, A1, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B4<R, O, A1, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback1B4 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback1B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B4<R, O, A1, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B4<R, O, A1, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback1B5 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback1B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback1<R, A1>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B5<R, A1, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback1B5 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback1B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B5<R, O, A1, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B5<R, O, A1, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback1B5 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback1B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B5<R, O, A1, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B5<R, O, A1, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 1 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback1B6 : public XorpCallback1<R, A1> {
    typedef R (*F)(A1, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback1B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback1<R, A1>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1) { return (*_f)(a1, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 1 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback1<R, A1>::RefPtr(new XorpFunctionCallback1B6<R, A1, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 1 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback1B6 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback1B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B6<R, O, A1, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 1 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpMemberCallback1B6<R, O, A1, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 1 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback1B6 : public XorpCallback1<R, A1> {
    typedef R (O::*M)(A1, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback1B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback1<R, A1>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1) { return ((*_o).*_m)(a1, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B6<R, O, A1, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 1 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback1<R, A1>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback1<R, A1>::RefPtr(new XorpConstMemberCallback1B6<R, O, A1, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 2 late args
//

/**
 * @short Base class for callbacks with 2 dispatch time args.
 */
template<class R, class A1, class A2>
struct XorpCallback2 {
    typedef ref_ptr<XorpCallback2> RefPtr;

    XorpCallback2(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback2() {}
    virtual R dispatch(A1, A2) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2>
struct XorpFunctionCallback2B0 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2);
    XorpFunctionCallback2B0(const char* file, int line, F f)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2)) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B0<R, A1, A2>(file, line, f));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2>
struct XorpMemberCallback2B0 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2) ;
    XorpMemberCallback2B0(const char* file, int line, O *o, M m)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2))
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B0<R, O, A1, A2>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2))
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B0<R, O, A1, A2>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2>
struct XorpConstMemberCallback2B0 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2)  const;
    XorpConstMemberCallback2B0(const char* file, int line, O *o, M m)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2) const)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B0<R, O, A1, A2>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2) const)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B0<R, O, A1, A2>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class BA1>
struct XorpFunctionCallback2B1 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2, BA1);
    XorpFunctionCallback2B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class BA1>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, BA1), BA1 ba1) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B1<R, A1, A2, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1>
struct XorpMemberCallback2B1 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1) ;
    XorpMemberCallback2B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1), BA1 ba1)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B1<R, O, A1, A2, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1), BA1 ba1)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B1<R, O, A1, A2, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1>
struct XorpConstMemberCallback2B1 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1)  const;
    XorpConstMemberCallback2B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1) const, BA1 ba1)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B1<R, O, A1, A2, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1) const, BA1 ba1)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B1<R, O, A1, A2, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2>
struct XorpFunctionCallback2B2 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2, BA1, BA2);
    XorpFunctionCallback2B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B2<R, A1, A2, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2>
struct XorpMemberCallback2B2 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2) ;
    XorpMemberCallback2B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B2<R, O, A1, A2, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B2<R, O, A1, A2, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2>
struct XorpConstMemberCallback2B2 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2)  const;
    XorpConstMemberCallback2B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B2<R, O, A1, A2, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B2<R, O, A1, A2, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3>
struct XorpFunctionCallback2B3 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2, BA1, BA2, BA3);
    XorpFunctionCallback2B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B3<R, A1, A2, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3>
struct XorpMemberCallback2B3 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3) ;
    XorpMemberCallback2B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B3<R, O, A1, A2, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B3<R, O, A1, A2, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback2B3 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3)  const;
    XorpConstMemberCallback2B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B3<R, O, A1, A2, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B3<R, O, A1, A2, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback2B4 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2, BA1, BA2, BA3, BA4);
    XorpFunctionCallback2B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B4<R, A1, A2, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback2B4 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback2B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B4<R, O, A1, A2, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B4<R, O, A1, A2, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback2B4 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback2B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B4<R, O, A1, A2, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B4<R, O, A1, A2, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback2B5 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback2B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B5<R, A1, A2, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback2B5 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback2B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B5<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B5<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback2B5 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback2B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B5<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B5<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 2 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback2B6 : public XorpCallback2<R, A1, A2> {
    typedef R (*F)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback2B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback2<R, A1, A2>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2) { return (*_f)(a1, a2, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 2 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpFunctionCallback2B6<R, A1, A2, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 2 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback2B6 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback2B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B6<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 2 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpMemberCallback2B6<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 2 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback2B6 : public XorpCallback2<R, A1, A2> {
    typedef R (O::*M)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback2B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback2<R, A1, A2>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2) { return ((*_o).*_m)(a1, a2, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B6<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 2 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback2<R, A1, A2>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback2<R, A1, A2>::RefPtr(new XorpConstMemberCallback2B6<R, O, A1, A2, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 3 late args
//

/**
 * @short Base class for callbacks with 3 dispatch time args.
 */
template<class R, class A1, class A2, class A3>
struct XorpCallback3 {
    typedef ref_ptr<XorpCallback3> RefPtr;

    XorpCallback3(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback3() {}
    virtual R dispatch(A1, A2, A3) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3>
struct XorpFunctionCallback3B0 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3);
    XorpFunctionCallback3B0(const char* file, int line, F f)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3)) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B0<R, A1, A2, A3>(file, line, f));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3>
struct XorpMemberCallback3B0 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3) ;
    XorpMemberCallback3B0(const char* file, int line, O *o, M m)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3))
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B0<R, O, A1, A2, A3>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3))
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B0<R, O, A1, A2, A3>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3>
struct XorpConstMemberCallback3B0 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3)  const;
    XorpConstMemberCallback3B0(const char* file, int line, O *o, M m)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3) const)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B0<R, O, A1, A2, A3>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3) const)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B0<R, O, A1, A2, A3>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class BA1>
struct XorpFunctionCallback3B1 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3, BA1);
    XorpFunctionCallback3B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class BA1>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, BA1), BA1 ba1) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B1<R, A1, A2, A3, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1>
struct XorpMemberCallback3B1 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1) ;
    XorpMemberCallback3B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1), BA1 ba1)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B1<R, O, A1, A2, A3, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1), BA1 ba1)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B1<R, O, A1, A2, A3, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1>
struct XorpConstMemberCallback3B1 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1)  const;
    XorpConstMemberCallback3B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1) const, BA1 ba1)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B1<R, O, A1, A2, A3, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1) const, BA1 ba1)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B1<R, O, A1, A2, A3, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2>
struct XorpFunctionCallback3B2 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3, BA1, BA2);
    XorpFunctionCallback3B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B2<R, A1, A2, A3, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2>
struct XorpMemberCallback3B2 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2) ;
    XorpMemberCallback3B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B2<R, O, A1, A2, A3, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B2<R, O, A1, A2, A3, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2>
struct XorpConstMemberCallback3B2 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2)  const;
    XorpConstMemberCallback3B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B2<R, O, A1, A2, A3, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B2<R, O, A1, A2, A3, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3>
struct XorpFunctionCallback3B3 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3, BA1, BA2, BA3);
    XorpFunctionCallback3B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B3<R, A1, A2, A3, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3>
struct XorpMemberCallback3B3 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3) ;
    XorpMemberCallback3B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B3<R, O, A1, A2, A3, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B3<R, O, A1, A2, A3, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback3B3 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3)  const;
    XorpConstMemberCallback3B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B3<R, O, A1, A2, A3, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B3<R, O, A1, A2, A3, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback3B4 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3, BA1, BA2, BA3, BA4);
    XorpFunctionCallback3B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B4<R, A1, A2, A3, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback3B4 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback3B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B4<R, O, A1, A2, A3, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B4<R, O, A1, A2, A3, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback3B4 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback3B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B4<R, O, A1, A2, A3, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B4<R, O, A1, A2, A3, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback3B5 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback3B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B5<R, A1, A2, A3, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback3B5 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback3B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B5<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B5<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback3B5 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback3B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B5<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B5<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 3 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback3B6 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (*F)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback3B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback3<R, A1, A2, A3>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return (*_f)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 3 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpFunctionCallback3B6<R, A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 3 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback3B6 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback3B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B6<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 3 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpMemberCallback3B6<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 3 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback3B6 : public XorpCallback3<R, A1, A2, A3> {
    typedef R (O::*M)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback3B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback3<R, A1, A2, A3>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3) { return ((*_o).*_m)(a1, a2, a3, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B6<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 3 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback3<R, A1, A2, A3>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback3<R, A1, A2, A3>::RefPtr(new XorpConstMemberCallback3B6<R, O, A1, A2, A3, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 4 late args
//

/**
 * @short Base class for callbacks with 4 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4>
struct XorpCallback4 {
    typedef ref_ptr<XorpCallback4> RefPtr;

    XorpCallback4(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback4() {}
    virtual R dispatch(A1, A2, A3, A4) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4>
struct XorpFunctionCallback4B0 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4);
    XorpFunctionCallback4B0(const char* file, int line, F f)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4)) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B0<R, A1, A2, A3, A4>(file, line, f));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4>
struct XorpMemberCallback4B0 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4) ;
    XorpMemberCallback4B0(const char* file, int line, O *o, M m)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4))
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B0<R, O, A1, A2, A3, A4>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4))
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B0<R, O, A1, A2, A3, A4>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4>
struct XorpConstMemberCallback4B0 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4)  const;
    XorpConstMemberCallback4B0(const char* file, int line, O *o, M m)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4) const)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B0<R, O, A1, A2, A3, A4>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4) const)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B0<R, O, A1, A2, A3, A4>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1>
struct XorpFunctionCallback4B1 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4, BA1);
    XorpFunctionCallback4B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, BA1), BA1 ba1) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B1<R, A1, A2, A3, A4, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1>
struct XorpMemberCallback4B1 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1) ;
    XorpMemberCallback4B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1), BA1 ba1)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B1<R, O, A1, A2, A3, A4, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1), BA1 ba1)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B1<R, O, A1, A2, A3, A4, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1>
struct XorpConstMemberCallback4B1 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1)  const;
    XorpConstMemberCallback4B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1) const, BA1 ba1)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B1<R, O, A1, A2, A3, A4, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1) const, BA1 ba1)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B1<R, O, A1, A2, A3, A4, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2>
struct XorpFunctionCallback4B2 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4, BA1, BA2);
    XorpFunctionCallback4B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B2<R, A1, A2, A3, A4, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2>
struct XorpMemberCallback4B2 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2) ;
    XorpMemberCallback4B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B2<R, O, A1, A2, A3, A4, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B2<R, O, A1, A2, A3, A4, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2>
struct XorpConstMemberCallback4B2 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2)  const;
    XorpConstMemberCallback4B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B2<R, O, A1, A2, A3, A4, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B2<R, O, A1, A2, A3, A4, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3>
struct XorpFunctionCallback4B3 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4, BA1, BA2, BA3);
    XorpFunctionCallback4B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B3<R, A1, A2, A3, A4, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3>
struct XorpMemberCallback4B3 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3) ;
    XorpMemberCallback4B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B3<R, O, A1, A2, A3, A4, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B3<R, O, A1, A2, A3, A4, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback4B3 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3)  const;
    XorpConstMemberCallback4B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B3<R, O, A1, A2, A3, A4, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B3<R, O, A1, A2, A3, A4, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback4B4 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4, BA1, BA2, BA3, BA4);
    XorpFunctionCallback4B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B4<R, A1, A2, A3, A4, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback4B4 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback4B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B4<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B4<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback4B4 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback4B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B4<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B4<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback4B5 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback4B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B5<R, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback4B5 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback4B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B5<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B5<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback4B5 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback4B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B5<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B5<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 4 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback4B6 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (*F)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback4B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback4<R, A1, A2, A3, A4>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return (*_f)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 4 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpFunctionCallback4B6<R, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 4 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback4B6 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback4B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B6<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 4 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpMemberCallback4B6<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 4 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback4B6 : public XorpCallback4<R, A1, A2, A3, A4> {
    typedef R (O::*M)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback4B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback4<R, A1, A2, A3, A4>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4) { return ((*_o).*_m)(a1, a2, a3, a4, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B6<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 4 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback4<R, A1, A2, A3, A4>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback4<R, A1, A2, A3, A4>::RefPtr(new XorpConstMemberCallback4B6<R, O, A1, A2, A3, A4, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 5 late args
//

/**
 * @short Base class for callbacks with 5 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5>
struct XorpCallback5 {
    typedef ref_ptr<XorpCallback5> RefPtr;

    XorpCallback5(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback5() {}
    virtual R dispatch(A1, A2, A3, A4, A5) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5>
struct XorpFunctionCallback5B0 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5);
    XorpFunctionCallback5B0(const char* file, int line, F f)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5)) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B0<R, A1, A2, A3, A4, A5>(file, line, f));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5>
struct XorpMemberCallback5B0 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5) ;
    XorpMemberCallback5B0(const char* file, int line, O *o, M m)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5))
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B0<R, O, A1, A2, A3, A4, A5>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5))
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B0<R, O, A1, A2, A3, A4, A5>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5>
struct XorpConstMemberCallback5B0 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5)  const;
    XorpConstMemberCallback5B0(const char* file, int line, O *o, M m)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5) const)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B0<R, O, A1, A2, A3, A4, A5>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5) const)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B0<R, O, A1, A2, A3, A4, A5>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1>
struct XorpFunctionCallback5B1 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5, BA1);
    XorpFunctionCallback5B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, BA1), BA1 ba1) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B1<R, A1, A2, A3, A4, A5, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1>
struct XorpMemberCallback5B1 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1) ;
    XorpMemberCallback5B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1), BA1 ba1)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B1<R, O, A1, A2, A3, A4, A5, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1), BA1 ba1)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B1<R, O, A1, A2, A3, A4, A5, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1>
struct XorpConstMemberCallback5B1 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1)  const;
    XorpConstMemberCallback5B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1) const, BA1 ba1)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B1<R, O, A1, A2, A3, A4, A5, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1) const, BA1 ba1)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B1<R, O, A1, A2, A3, A4, A5, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2>
struct XorpFunctionCallback5B2 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5, BA1, BA2);
    XorpFunctionCallback5B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B2<R, A1, A2, A3, A4, A5, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2>
struct XorpMemberCallback5B2 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2) ;
    XorpMemberCallback5B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B2<R, O, A1, A2, A3, A4, A5, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B2<R, O, A1, A2, A3, A4, A5, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2>
struct XorpConstMemberCallback5B2 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2)  const;
    XorpConstMemberCallback5B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B2<R, O, A1, A2, A3, A4, A5, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B2<R, O, A1, A2, A3, A4, A5, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3>
struct XorpFunctionCallback5B3 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5, BA1, BA2, BA3);
    XorpFunctionCallback5B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B3<R, A1, A2, A3, A4, A5, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3>
struct XorpMemberCallback5B3 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3) ;
    XorpMemberCallback5B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B3<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B3<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback5B3 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3)  const;
    XorpConstMemberCallback5B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B3<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B3<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback5B4 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4);
    XorpFunctionCallback5B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B4<R, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback5B4 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback5B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B4<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B4<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback5B4 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback5B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B4<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B4<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback5B5 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback5B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B5<R, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback5B5 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback5B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B5<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B5<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback5B5 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback5B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B5<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B5<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 5 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback5B6 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (*F)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback5B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback5<R, A1, A2, A3, A4, A5>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return (*_f)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 5 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpFunctionCallback5B6<R, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 5 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback5B6 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback5B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B6<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 5 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpMemberCallback5B6<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 5 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback5B6 : public XorpCallback5<R, A1, A2, A3, A4, A5> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback5B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback5<R, A1, A2, A3, A4, A5>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return ((*_o).*_m)(a1, a2, a3, a4, a5, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B6<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 5 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback5<R, A1, A2, A3, A4, A5>::RefPtr(new XorpConstMemberCallback5B6<R, O, A1, A2, A3, A4, A5, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 6 late args
//

/**
 * @short Base class for callbacks with 6 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6>
struct XorpCallback6 {
    typedef ref_ptr<XorpCallback6> RefPtr;

    XorpCallback6(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback6() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6>
struct XorpFunctionCallback6B0 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6);
    XorpFunctionCallback6B0(const char* file, int line, F f)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6)) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B0<R, A1, A2, A3, A4, A5, A6>(file, line, f));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6>
struct XorpMemberCallback6B0 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6) ;
    XorpMemberCallback6B0(const char* file, int line, O *o, M m)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6))
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B0<R, O, A1, A2, A3, A4, A5, A6>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6))
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B0<R, O, A1, A2, A3, A4, A5, A6>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6>
struct XorpConstMemberCallback6B0 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6)  const;
    XorpConstMemberCallback6B0(const char* file, int line, O *o, M m)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6) const)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B0<R, O, A1, A2, A3, A4, A5, A6>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6) const)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B0<R, O, A1, A2, A3, A4, A5, A6>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1>
struct XorpFunctionCallback6B1 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, BA1);
    XorpFunctionCallback6B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, BA1), BA1 ba1) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B1<R, A1, A2, A3, A4, A5, A6, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1>
struct XorpMemberCallback6B1 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1) ;
    XorpMemberCallback6B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1), BA1 ba1)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B1<R, O, A1, A2, A3, A4, A5, A6, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1), BA1 ba1)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B1<R, O, A1, A2, A3, A4, A5, A6, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1>
struct XorpConstMemberCallback6B1 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1)  const;
    XorpConstMemberCallback6B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1) const, BA1 ba1)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B1<R, O, A1, A2, A3, A4, A5, A6, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1) const, BA1 ba1)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B1<R, O, A1, A2, A3, A4, A5, A6, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2>
struct XorpFunctionCallback6B2 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, BA1, BA2);
    XorpFunctionCallback6B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B2<R, A1, A2, A3, A4, A5, A6, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2>
struct XorpMemberCallback6B2 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2) ;
    XorpMemberCallback6B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B2<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B2<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2>
struct XorpConstMemberCallback6B2 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2)  const;
    XorpConstMemberCallback6B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B2<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B2<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3>
struct XorpFunctionCallback6B3 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3);
    XorpFunctionCallback6B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B3<R, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3>
struct XorpMemberCallback6B3 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3) ;
    XorpMemberCallback6B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B3<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B3<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback6B3 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3)  const;
    XorpConstMemberCallback6B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B3<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B3<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback6B4 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4);
    XorpFunctionCallback6B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B4<R, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback6B4 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback6B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B4<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B4<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback6B4 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback6B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B4<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B4<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback6B5 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback6B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B5<R, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback6B5 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback6B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B5<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B5<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback6B5 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback6B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B5<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B5<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 6 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback6B6 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback6B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return (*_f)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 6 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpFunctionCallback6B6<R, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 6 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback6B6 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback6B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B6<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 6 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpMemberCallback6B6<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 6 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback6B6 : public XorpCallback6<R, A1, A2, A3, A4, A5, A6> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback6B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback6<R, A1, A2, A3, A4, A5, A6>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B6<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 6 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback6<R, A1, A2, A3, A4, A5, A6>::RefPtr(new XorpConstMemberCallback6B6<R, O, A1, A2, A3, A4, A5, A6, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 7 late args
//

/**
 * @short Base class for callbacks with 7 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct XorpCallback7 {
    typedef ref_ptr<XorpCallback7> RefPtr;

    XorpCallback7(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback7() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct XorpFunctionCallback7B0 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7);
    XorpFunctionCallback7B0(const char* file, int line, F f)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7)) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B0<R, A1, A2, A3, A4, A5, A6, A7>(file, line, f));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct XorpMemberCallback7B0 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7) ;
    XorpMemberCallback7B0(const char* file, int line, O *o, M m)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7))
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B0<R, O, A1, A2, A3, A4, A5, A6, A7>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7))
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B0<R, O, A1, A2, A3, A4, A5, A6, A7>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct XorpConstMemberCallback7B0 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7)  const;
    XorpConstMemberCallback7B0(const char* file, int line, O *o, M m)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7) const)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B0<R, O, A1, A2, A3, A4, A5, A6, A7>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7) const)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B0<R, O, A1, A2, A3, A4, A5, A6, A7>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1>
struct XorpFunctionCallback7B1 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, BA1);
    XorpFunctionCallback7B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, BA1), BA1 ba1) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B1<R, A1, A2, A3, A4, A5, A6, A7, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1>
struct XorpMemberCallback7B1 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1) ;
    XorpMemberCallback7B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1), BA1 ba1)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B1<R, O, A1, A2, A3, A4, A5, A6, A7, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1), BA1 ba1)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B1<R, O, A1, A2, A3, A4, A5, A6, A7, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1>
struct XorpConstMemberCallback7B1 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1)  const;
    XorpConstMemberCallback7B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1) const, BA1 ba1)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B1<R, O, A1, A2, A3, A4, A5, A6, A7, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1) const, BA1 ba1)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B1<R, O, A1, A2, A3, A4, A5, A6, A7, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2>
struct XorpFunctionCallback7B2 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2);
    XorpFunctionCallback7B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B2<R, A1, A2, A3, A4, A5, A6, A7, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2>
struct XorpMemberCallback7B2 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2) ;
    XorpMemberCallback7B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B2<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B2<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2>
struct XorpConstMemberCallback7B2 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2)  const;
    XorpConstMemberCallback7B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B2<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B2<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3>
struct XorpFunctionCallback7B3 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3);
    XorpFunctionCallback7B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B3<R, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3>
struct XorpMemberCallback7B3 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3) ;
    XorpMemberCallback7B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B3<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B3<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback7B3 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3)  const;
    XorpConstMemberCallback7B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B3<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B3<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback7B4 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4);
    XorpFunctionCallback7B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B4<R, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback7B4 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback7B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B4<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B4<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback7B4 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback7B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B4<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B4<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback7B5 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback7B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B5<R, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback7B5 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback7B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B5<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B5<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback7B5 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback7B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B5<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B5<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 7 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback7B6 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback7B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 7 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpFunctionCallback7B6<R, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 7 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback7B6 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback7B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B6<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 7 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpMemberCallback7B6<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 7 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback7B6 : public XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback7B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B6<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 7 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback7<R, A1, A2, A3, A4, A5, A6, A7>::RefPtr(new XorpConstMemberCallback7B6<R, O, A1, A2, A3, A4, A5, A6, A7, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 8 late args
//

/**
 * @short Base class for callbacks with 8 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct XorpCallback8 {
    typedef ref_ptr<XorpCallback8> RefPtr;

    XorpCallback8(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback8() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7, A8) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct XorpFunctionCallback8B0 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8);
    XorpFunctionCallback8B0(const char* file, int line, F f)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8)) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B0<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line, f));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct XorpMemberCallback8B0 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8) ;
    XorpMemberCallback8B0(const char* file, int line, O *o, M m)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8))
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8))
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct XorpConstMemberCallback8B0 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8)  const;
    XorpConstMemberCallback8B0(const char* file, int line, O *o, M m)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8) const)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8) const)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1>
struct XorpFunctionCallback8B1 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, BA1);
    XorpFunctionCallback8B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, BA1), BA1 ba1) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B1<R, A1, A2, A3, A4, A5, A6, A7, A8, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1>
struct XorpMemberCallback8B1 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1) ;
    XorpMemberCallback8B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1), BA1 ba1)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1), BA1 ba1)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1>
struct XorpConstMemberCallback8B1 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1)  const;
    XorpConstMemberCallback8B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1) const, BA1 ba1)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1) const, BA1 ba1)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2>
struct XorpFunctionCallback8B2 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2);
    XorpFunctionCallback8B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B2<R, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2>
struct XorpMemberCallback8B2 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2) ;
    XorpMemberCallback8B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2>
struct XorpConstMemberCallback8B2 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2)  const;
    XorpConstMemberCallback8B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3>
struct XorpFunctionCallback8B3 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3);
    XorpFunctionCallback8B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B3<R, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3>
struct XorpMemberCallback8B3 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3) ;
    XorpMemberCallback8B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback8B3 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3)  const;
    XorpConstMemberCallback8B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback8B4 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4);
    XorpFunctionCallback8B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B4<R, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback8B4 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback8B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback8B4 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback8B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback8B5 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback8B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B5<R, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback8B5 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback8B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback8B5 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback8B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 8 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback8B6 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback8B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 8 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpFunctionCallback8B6<R, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 8 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback8B6 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback8B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 8 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpMemberCallback8B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 8 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback8B6 : public XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback8B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 8 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback8<R, A1, A2, A3, A4, A5, A6, A7, A8>::RefPtr(new XorpConstMemberCallback8B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 9 late args
//

/**
 * @short Base class for callbacks with 9 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct XorpCallback9 {
    typedef ref_ptr<XorpCallback9> RefPtr;

    XorpCallback9(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback9() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7, A8, A9) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct XorpFunctionCallback9B0 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9);
    XorpFunctionCallback9B0(const char* file, int line, F f)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9)) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B0<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line, f));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct XorpMemberCallback9B0 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9) ;
    XorpMemberCallback9B0(const char* file, int line, O *o, M m)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9))
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9))
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
struct XorpConstMemberCallback9B0 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9)  const;
    XorpConstMemberCallback9B0(const char* file, int line, O *o, M m)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9) const)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9) const)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1>
struct XorpFunctionCallback9B1 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1);
    XorpFunctionCallback9B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1), BA1 ba1) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B1<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1>
struct XorpMemberCallback9B1 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1) ;
    XorpMemberCallback9B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1), BA1 ba1)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1), BA1 ba1)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1>
struct XorpConstMemberCallback9B1 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1)  const;
    XorpConstMemberCallback9B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1) const, BA1 ba1)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1) const, BA1 ba1)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2>
struct XorpFunctionCallback9B2 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2);
    XorpFunctionCallback9B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B2<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2>
struct XorpMemberCallback9B2 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2) ;
    XorpMemberCallback9B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2>
struct XorpConstMemberCallback9B2 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2)  const;
    XorpConstMemberCallback9B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3>
struct XorpFunctionCallback9B3 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3);
    XorpFunctionCallback9B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B3<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3>
struct XorpMemberCallback9B3 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3) ;
    XorpMemberCallback9B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback9B3 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3)  const;
    XorpConstMemberCallback9B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback9B4 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4);
    XorpFunctionCallback9B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B4<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback9B4 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback9B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback9B4 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback9B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback9B5 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback9B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B5<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback9B5 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback9B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback9B5 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback9B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 9 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback9B6 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback9B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 9 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpFunctionCallback9B6<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 9 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback9B6 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback9B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 9 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpMemberCallback9B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 9 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback9B6 : public XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback9B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 9 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::RefPtr(new XorpConstMemberCallback9B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 10 late args
//

/**
 * @short Base class for callbacks with 10 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
struct XorpCallback10 {
    typedef ref_ptr<XorpCallback10> RefPtr;

    XorpCallback10(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback10() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
struct XorpFunctionCallback10B0 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
    XorpFunctionCallback10B0(const char* file, int line, F f)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B0<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line, f));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
struct XorpMemberCallback10B0 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) ;
    XorpMemberCallback10B0(const char* file, int line, O *o, M m)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10))
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10))
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
struct XorpConstMemberCallback10B0 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)  const;
    XorpConstMemberCallback10B0(const char* file, int line, O *o, M m)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) const)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) const)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1>
struct XorpFunctionCallback10B1 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1);
    XorpFunctionCallback10B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1), BA1 ba1) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B1<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1>
struct XorpMemberCallback10B1 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1) ;
    XorpMemberCallback10B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1), BA1 ba1)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1), BA1 ba1)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1>
struct XorpConstMemberCallback10B1 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1)  const;
    XorpConstMemberCallback10B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1) const, BA1 ba1)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1) const, BA1 ba1)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2>
struct XorpFunctionCallback10B2 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2);
    XorpFunctionCallback10B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B2<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2>
struct XorpMemberCallback10B2 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2) ;
    XorpMemberCallback10B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2>
struct XorpConstMemberCallback10B2 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2)  const;
    XorpConstMemberCallback10B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3>
struct XorpFunctionCallback10B3 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3);
    XorpFunctionCallback10B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B3<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3>
struct XorpMemberCallback10B3 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3) ;
    XorpMemberCallback10B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback10B3 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3)  const;
    XorpConstMemberCallback10B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback10B4 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4);
    XorpFunctionCallback10B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B4<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback10B4 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback10B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback10B4 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback10B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback10B5 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback10B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B5<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback10B5 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback10B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback10B5 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback10B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 10 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback10B6 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback10B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 10 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpFunctionCallback10B6<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 10 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback10B6 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback10B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 10 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpMemberCallback10B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 10 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback10B6 : public XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback10B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 10 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback10<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10>::RefPtr(new XorpConstMemberCallback10B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 11 late args
//

/**
 * @short Base class for callbacks with 11 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
struct XorpCallback11 {
    typedef ref_ptr<XorpCallback11> RefPtr;

    XorpCallback11(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback11() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
struct XorpFunctionCallback11B0 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11);
    XorpFunctionCallback11B0(const char* file, int line, F f)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B0<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line, f));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
struct XorpMemberCallback11B0 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) ;
    XorpMemberCallback11B0(const char* file, int line, O *o, M m)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11))
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11))
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>
struct XorpConstMemberCallback11B0 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)  const;
    XorpConstMemberCallback11B0(const char* file, int line, O *o, M m)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) const)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) const)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1>
struct XorpFunctionCallback11B1 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1);
    XorpFunctionCallback11B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1), BA1 ba1) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B1<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1>
struct XorpMemberCallback11B1 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1) ;
    XorpMemberCallback11B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1), BA1 ba1)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1), BA1 ba1)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1>
struct XorpConstMemberCallback11B1 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1)  const;
    XorpConstMemberCallback11B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1) const, BA1 ba1)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1) const, BA1 ba1)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2>
struct XorpFunctionCallback11B2 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2);
    XorpFunctionCallback11B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B2<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2>
struct XorpMemberCallback11B2 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2) ;
    XorpMemberCallback11B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2>
struct XorpConstMemberCallback11B2 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2)  const;
    XorpConstMemberCallback11B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3>
struct XorpFunctionCallback11B3 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3);
    XorpFunctionCallback11B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B3<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3>
struct XorpMemberCallback11B3 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3) ;
    XorpMemberCallback11B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback11B3 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3)  const;
    XorpConstMemberCallback11B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback11B4 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4);
    XorpFunctionCallback11B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B4<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback11B4 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback11B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback11B4 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback11B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback11B5 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback11B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B5<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback11B5 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback11B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback11B5 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback11B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 11 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback11B6 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback11B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 11 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpFunctionCallback11B6<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 11 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback11B6 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback11B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 11 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpMemberCallback11B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 11 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback11B6 : public XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback11B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 11 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback11<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11>::RefPtr(new XorpConstMemberCallback11B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 12 late args
//

/**
 * @short Base class for callbacks with 12 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
struct XorpCallback12 {
    typedef ref_ptr<XorpCallback12> RefPtr;

    XorpCallback12(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback12() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
struct XorpFunctionCallback12B0 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12);
    XorpFunctionCallback12B0(const char* file, int line, F f)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B0<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line, f));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
struct XorpMemberCallback12B0 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) ;
    XorpMemberCallback12B0(const char* file, int line, O *o, M m)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12))
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12))
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12>
struct XorpConstMemberCallback12B0 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)  const;
    XorpConstMemberCallback12B0(const char* file, int line, O *o, M m)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) const)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) const)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1>
struct XorpFunctionCallback12B1 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1);
    XorpFunctionCallback12B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1), BA1 ba1) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B1<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1>
struct XorpMemberCallback12B1 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1) ;
    XorpMemberCallback12B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1), BA1 ba1)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1), BA1 ba1)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1>
struct XorpConstMemberCallback12B1 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1)  const;
    XorpConstMemberCallback12B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1) const, BA1 ba1)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1) const, BA1 ba1)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2>
struct XorpFunctionCallback12B2 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2);
    XorpFunctionCallback12B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B2<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2>
struct XorpMemberCallback12B2 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2) ;
    XorpMemberCallback12B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2>
struct XorpConstMemberCallback12B2 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2)  const;
    XorpConstMemberCallback12B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3>
struct XorpFunctionCallback12B3 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3);
    XorpFunctionCallback12B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B3<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3>
struct XorpMemberCallback12B3 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3) ;
    XorpMemberCallback12B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback12B3 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3)  const;
    XorpConstMemberCallback12B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback12B4 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4);
    XorpFunctionCallback12B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B4<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback12B4 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback12B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback12B4 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback12B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback12B5 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback12B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B5<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback12B5 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback12B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback12B5 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback12B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 12 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback12B6 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback12B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 12 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpFunctionCallback12B6<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 12 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback12B6 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback12B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 12 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpMemberCallback12B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 12 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback12B6 : public XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback12B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 12 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback12<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12>::RefPtr(new XorpConstMemberCallback12B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


///////////////////////////////////////////////////////////////////////////////
//
// Code relating to callbacks with 13 late args
//

/**
 * @short Base class for callbacks with 13 dispatch time args.
 */
template<class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
struct XorpCallback13 {
    typedef ref_ptr<XorpCallback13> RefPtr;

    XorpCallback13(const char* file, int line)
	: _file(file), _line(line) {}
    virtual ~XorpCallback13() {}
    virtual R dispatch(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) = 0;
    const char* file() const		{ return _file; }
    int line() const			{ return _line; }
private:
    const char* _file;
    int         _line;
};

/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
struct XorpFunctionCallback13B0 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13);
    XorpFunctionCallback13B0(const char* file, int line, F f)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13); } 
protected:
    F   _f;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 0 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B0<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line, f));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
struct XorpMemberCallback13B0 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) ;
    XorpMemberCallback13B0(const char* file, int line, O *o, M m)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13))
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13))
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line, &o, p));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 0 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13>
struct XorpConstMemberCallback13B0 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)  const;
    XorpConstMemberCallback13B0(const char* file, int line, O *o, M m)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) const)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line, o, p));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 0 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) const)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B0<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line, &o, p));
}


/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1>
struct XorpFunctionCallback13B1 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1);
    XorpFunctionCallback13B1(const char* file, int line, F f, BA1 ba1)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f), _ba1(ba1) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1); } 
protected:
    F   _f;
    BA1 _ba1;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 1 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1), BA1 ba1) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B1<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1>(file, line, f, ba1));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1>
struct XorpMemberCallback13B1 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1) ;
    XorpMemberCallback13B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1), BA1 ba1)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1), BA1 ba1)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 1 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1>
struct XorpConstMemberCallback13B1 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1)  const;
    XorpConstMemberCallback13B1(const char* file, int line, O *o, M m, BA1 ba1)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1) const, BA1 ba1)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1>(file, line, o, p, ba1));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 1 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1) const, BA1 ba1)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B1<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1>(file, line, &o, p, ba1));
}


/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2>
struct XorpFunctionCallback13B2 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2);
    XorpFunctionCallback13B2(const char* file, int line, F f, BA1 ba1, BA2 ba2)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 2 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2), BA1 ba1, BA2 ba2) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B2<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2>(file, line, f, ba1, ba2));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2>
struct XorpMemberCallback13B2 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2) ;
    XorpMemberCallback13B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2), BA1 ba1, BA2 ba2)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 2 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2>
struct XorpConstMemberCallback13B2 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2)  const;
    XorpConstMemberCallback13B2(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2>(file, line, o, p, ba1, ba2));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 2 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2) const, BA1 ba1, BA2 ba2)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B2<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2>(file, line, &o, p, ba1, ba2));
}


/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3>
struct XorpFunctionCallback13B3 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3);
    XorpFunctionCallback13B3(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 3 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B3<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3>(file, line, f, ba1, ba2, ba3));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3>
struct XorpMemberCallback13B3 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3) ;
    XorpMemberCallback13B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3), BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 3 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3>
struct XorpConstMemberCallback13B3 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3)  const;
    XorpConstMemberCallback13B3(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3>(file, line, o, p, ba1, ba2, ba3));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 3 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3) const, BA1 ba1, BA2 ba2, BA3 ba3)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B3<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3>(file, line, &o, p, ba1, ba2, ba3));
}


/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4>
struct XorpFunctionCallback13B4 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4);
    XorpFunctionCallback13B4(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 4 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B4<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4>(file, line, f, ba1, ba2, ba3, ba4));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4>
struct XorpMemberCallback13B4 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4) ;
    XorpMemberCallback13B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 4 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4>
struct XorpConstMemberCallback13B4 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4)  const;
    XorpConstMemberCallback13B4(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4>(file, line, o, p, ba1, ba2, ba3, ba4));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 4 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B4<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4>(file, line, &o, p, ba1, ba2, ba3, ba4));
}


/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpFunctionCallback13B5 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5);
    XorpFunctionCallback13B5(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4, _ba5); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 5 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B5<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5>(file, line, f, ba1, ba2, ba3, ba4, ba5));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpMemberCallback13B5 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5) ;
    XorpMemberCallback13B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 5 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5>
struct XorpConstMemberCallback13B5 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5)  const;
    XorpConstMemberCallback13B5(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4, _ba5); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5>(file, line, o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 5 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B5<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5));
}


/**
 * @short Callback object for functions with 13 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpFunctionCallback13B6 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (*F)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6);
    XorpFunctionCallback13B6(const char* file, int line, F f, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	: XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line),
	  _f(f), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) 
    {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return (*_f)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); } 
protected:
    F   _f;
    BA1 _ba1;
    BA2 _ba2;
    BA3 _ba3;
    BA4 _ba4;
    BA5 _ba5;
    BA6 _ba6;
};


/**
 * Factory function that creates a callback object targetted at a
 * function with 13 dispatch time arguments and 6 bound arguments.
 */
template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, R (*f)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6) {
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpFunctionCallback13B6<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, f, ba1, ba2, ba3, ba4, ba5, ba6));
}

/**
 * @short Callback object for  member methods with 13 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpMemberCallback13B6 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6) ;
    XorpMemberCallback13B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  member function with 13 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6), BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpMemberCallback13B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * @short Callback object for  const member methods with 13 dispatch time
 * arguments and 6 bound (stored) arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6>
struct XorpConstMemberCallback13B6 : public XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> {
    typedef R (O::*M)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6)  const;
    XorpConstMemberCallback13B6(const char* file, int line, O *o, M m, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
	 : XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>(file, line), 
	  _o(o), _m(m), _ba1(ba1), _ba2(ba2), _ba3(ba3), _ba4(ba4), _ba5(ba5), _ba6(ba6) {}
    R dispatch(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10, A11 a11, A12 a12, A13 a13) { return ((*_o).*_m)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, _ba1, _ba2, _ba3, _ba4, _ba5, _ba6); }
protected:
    O	*_o;	// Callback's target object
    M	 _m;	// Callback's target method
    BA1 _ba1;	// Bound argument
    BA2 _ba2;	// Bound argument
    BA3 _ba3;	// Bound argument
    BA4 _ba4;	// Bound argument
    BA5 _ba5;	// Bound argument
    BA6 _ba6;	// Bound argument
};


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O *o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


/**
 * Factory function that creates a callback object targetted at a
 *  const member function with 13 dispatch time arguments and 6 bound arguments.
 */
template <class R, class O, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class BA1, class BA2, class BA3, class BA4, class BA5, class BA6> typename XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr
dbg_callback(const char* file, int line, O &o, R (O::*p)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6) const, BA1 ba1, BA2 ba2, BA3 ba3, BA4 ba4, BA5 ba5, BA6 ba6)
{
    return XorpCallback13<R, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13>::RefPtr(new XorpConstMemberCallback13B6<R, O, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, BA1, BA2, BA3, BA4, BA5, BA6>(file, line, &o, p, ba1, ba2, ba3, ba4, ba5, ba6));
}


#endif /* __XORP_CALLBACK_HH__ */
