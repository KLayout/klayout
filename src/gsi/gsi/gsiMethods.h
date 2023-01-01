
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#ifndef _HDR_gsiMethods
#define _HDR_gsiMethods

#include "tlString.h"
#include "tlUtils.h"
#include "tlAssert.h"
#include "gsiSerialisation.h"

namespace gsi
{

class SignalHandler;

struct NoArgumentsAllowedException
  : public tl::Exception
{
  NoArgumentsAllowedException ()
    : tl::Exception (tl::to_string (tr ("Method does not allow arguments")))
  { }
};

struct NeedsArgumentsException
  : public tl::Exception
{
  NeedsArgumentsException (unsigned int got, unsigned int want)
    : tl::Exception (tl::sprintf (tl::to_string (tr ("Method requires %d arguments, got %d")), want, got))
  { }
};

struct IncompatibleReturnTypeException
  : public tl::Exception
{
  IncompatibleReturnTypeException (const ArgType &got, const ArgType &want)
    : tl::Exception (tl::to_string (tr ("Incompatible return types: got '")) + got.to_string () + tl::to_string (tr ("', want '")) + want.to_string () + "'")
  { }
};

/**
 *  @brief Basic declaration of a method
 *
 *  A class declaration collects objects of this kind to represent methods and their specific
 *  implementation.
 *  This class is implemented in various specific ways that bind the abstract call to a specific
 *  C++ method.
 */
class GSI_PUBLIC MethodBase 
{
public:
  typedef std::vector<ArgType>::const_iterator argument_iterator;

  /**
   *  @brief Declares a method as a special one
   */
  enum special_method_type
  {
    None = 0,
    DefaultCtor,
    Keep,
    Release,
    Destroy,
    Create,
    IsConst, 
    Destroyed, 
    Assign, 
    Dup
  };

  /**
   *  @brief Declares a synonym for the method
   */
  struct MethodSynonym
  {
    MethodSynonym ()
      : name (), deprecated (false), is_predicate (false), is_setter (false), is_getter (false)
    { }

    std::string name;
    bool deprecated : 1;
    bool is_predicate : 1;
    bool is_setter : 1;
    bool is_getter : 1;
  };

  typedef std::vector<MethodSynonym>::const_iterator synonym_iterator;

  /**
   *  @brief Creates a method with the given name string, documentation and const and static flag
   *
   *  The name string encodes some additional information, specifically:
   *    "*..."      The method is protected
   *    "x|y"       Aliases (synonyms)
   *    "x|#y"      y is deprecated
   *    "x="        x is a setter
   *    ":x"        x is a getter
   *    "x?"        x is a predicate
   *  Backslashes can be used to escape the special characters, like "*" and "|".
   */
  MethodBase (const std::string &name, const std::string &doc, bool c, bool s);

  /**
   *  @brief Creates a method with the given name and documentation string,
   *
   *  The method will not be static nor const. See the full constructor for a description of
   *  the name string.
   */
  MethodBase (const std::string &name, const std::string &doc);

  /**
   *  @brief Destructor
   */
  virtual ~MethodBase ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Iterates the synonyms (begin)
   */
  synonym_iterator begin_synonyms () const
  {
    return m_method_synonyms.begin ();
  }

  /**
   *  @brief Iterates the synonyms (end)
   */
  synonym_iterator end_synonyms () const
  {
    return m_method_synonyms.end ();
  }

  /**
   *  @brief Returns the combined name 
   *
   *  This name reflects the synonyms in the original way.
   */
  std::string combined_name () const;

  /**
   *  @brief Gets the primary name 
   *
   *  The primary name is the name of the first synonym.
   */
  const std::string &primary_name () const;

  /**
   *  @brief Gets a list of the names for diagnostic purposes
   *
   *  The list is a |-combined list of names.
   */
  std::string names () const;

  /**
   *  @brief Gets a string describing this method with the signature
   */
  std::string to_string () const;

  /**
   *  @brief Returns a value indicating whether the method is protected
   */
  bool is_protected () const
  {
    return m_protected;
  }

  /**
   *  @brief Returns the documentation string
   */
  const std::string &doc () const
  {
    return m_doc;
  }

  /**
   *  @brief Sets the documentation text
   */
  void set_doc (const std::string &d)
  {
    m_doc = d;
  }

  /**
   *  @brief Returns the nth (index) argument
   */
  const ArgType &arg (size_t index) const
  {
    tl_assert (m_arg_types.size () > index);
    return m_arg_types [index];
  }

  /**
   *  @brief Returns the nth (index) argument (non_const version)
   */
  ArgType &arg (size_t index)
  {
    tl_assert (m_arg_types.size () > index);
    return m_arg_types [index];
  }

  /**
   *  @brief Iterates the arguments (begin)
   */
  argument_iterator begin_arguments () const
  {
    return m_arg_types.begin ();
  }

  /**
   *  @brief Iterates the arguments (end)
   */
  argument_iterator end_arguments () const
  {
    return m_arg_types.end ();
  }

  /**
   *  @brief Gets the return type
   */
  const ArgType &ret_type () const
  {
    return m_ret_type;
  }

  /**
   *  @brief Gets the return type (non-const version)
   */
  ArgType &ret_type ()
  {
    return m_ret_type;
  }

  /**
   *  @brief Gets a value indicating whether this method is a const method
   */
  bool is_const () const
  {
    return m_const;
  }

  /**
   *  @brief Sets a value indicating whether the method is a const method
   */
  void set_const (bool c)
  {
    m_const = c;
  }

  /**
   *  @brief Gets a value indicating whether the method is a static method
   */
  bool is_static () const
  {
    return m_static;
  }

  /**
   *  @brief Gets a value indicator whether the method is a constructor
   *
   *  Static methods returning a new object are constructors.
   */
  bool is_constructor () const
  {
    return (is_static () && ret_type ().pass_obj () && ret_type ().is_ptr ());
  }

  /**
   *  @brief Returns a value indicating whether the method is compatible with the given number of arguments
   */
  bool compatible_with_num_args (unsigned int num) const;

  /**
   *  @brief Throws an exception if the method requires arguments
   */
  void check_no_args () const;

  /**
   *  @brief Throws an exception if the method is not satisfied with num arguments
   */
  void check_num_args (unsigned int num) const;

  /**
   *  @brief Throws an exception if the method delivers a return value of the given type
   */
  void check_return_type (const ArgType &a) const;

  /**
   *  @brief Gets the size of the argument list buffer in bytes
   */
  unsigned int argsize () const
  {
    return m_argsize;
  }

  /**
   *  @brief Gets the size of the return value buffer in bytes
   */
  unsigned int retsize () const
  {
    return m_ret_type.size ();
  }

  /**
   *  @brief Clears the arguments and return type
   */
  void clear ()
  {
    m_arg_types.clear ();
    m_ret_type = ArgType ();
  }

  /**
   *  @brief Adds an argument to the argument list (of type X)
   */
  template <class X>
  void add_arg ()
  {
    ArgType a;
    a.template init<X, arg_make_reference> ();
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief Adds an argument to the argument list (of type X)
   */
  template <class X, class Transfer>
  void add_arg ()
  {
    ArgType a;
    a.template init<X, Transfer> ();
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief Adds an argument to the argument list (of type X plus additional specs)
   */
  template <class X>
  void add_arg (const ArgSpecBase &spec) 
  {
    ArgType a;
    a.template init<X, arg_make_reference> (spec);
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief Adds an argument to the argument list (of type X plus additional specs)
   */
  template <class X, class Transfer>
  void add_arg (const ArgSpecBase &spec)
  {
    ArgType a;
    a.template init<X, Transfer> (spec);
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief This version will take the ownership of the ArgSpecBase object
   */
  template <class X>
  void add_arg (ArgSpecBase *spec) 
  {
    ArgType a;
    a.template init<X, arg_make_reference> (spec);
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief This version will take the ownership of the ArgSpecBase object
   */
  template <class X, class Transfer>
  void add_arg (ArgSpecBase *spec)
  {
    ArgType a;
    a.template init<X, Transfer> (spec);
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief Adds an argument to the argument list
   */
  void add_arg (const ArgType &a) 
  {
    m_arg_types.push_back (a);
    m_argsize += a.size ();
  }

  /**
   *  @brief Sets the return type to "X"
   */
  template <class X>
  void set_return () 
  {
    m_ret_type.template init<X, arg_default_return_value_preference> ();
  }

  /**
   *  @brief Sets the return type to "X"
   */
  template <class X, class Transfer>
  void set_return ()
  {
    m_ret_type.template init<X, Transfer> ();
  }

  /**
   *  @brief Sets the return type to "new object of type X"
   */
  template <class X> 
  void set_return_new () 
  {
    m_ret_type.template init<X, arg_pass_ownership> ();
  }

  /**
   *  @brief Sets the return type
   */
  void set_return (const ArgType &r) 
  {
    m_ret_type = r;
  }

  /**
   *  @brief Clones this object
   */
  virtual MethodBase *clone () const 
  {
    return new MethodBase (*this);
  }

  /**
   *  @brief Initializes the method (can be overridden to define the method later
   */
  virtual void initialize () 
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Calls this method on the given object with the given arguments
   *
   *  The implementation will execute the method and fill the ret buffer with the
   *  return values.
   */
  virtual void call (void * /*obj*/, SerialArgs & /*args*/, SerialArgs & /*ret*/) const 
  {
    tl_assert (false);
  }

  /**
   *  @brief Returns a value indicating whether the method is a special method 
   *
   *  Special methods are declared implicitly and serve special purposes, i.e.
   *  the default constructor, the assignment operator etc.
   */
  virtual special_method_type smt () const
  {
    return None;
  }

  /**
   *  @brief Returns a value indicating whether the method is a callback (called by the script client)
   */
  virtual bool is_callback () const 
  {
    return false;
  }

  /**
   *  @brief Connects the callback method with an object and callback structure
   */
  virtual void set_callback (void * /*v*/, const Callback & /*cb*/) const 
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Returns a value indicating whether this method is a signal
   *
   *  Events are methods that are called from the C++ side and execute code on
   *  the script client side. See gsiSignals.h for details.
   */
  virtual bool is_signal () const
  {
    return false;
  }

  /**
   *  @brief Installs a signal handler for a signal
   *
   *  If the method is a signal, this function is supposed to install a signal
   *  handler on the given object. The implementation is specific to the kind of
   *  signal (i.e. Qt signal or tl::Event).
   */
  virtual void add_handler (void * /*obj*/, SignalHandler * /*handler*/) const
  {
    //  .. nothing yet ..
  }

#if defined(TRACE_METHOD_CALLS)

public:
  /**
   *  @brief Gets a flag indicating whether the method was called
   */
  inline bool was_called () const
  {
    return m_called;
  }

protected:
  /**
   *  @brief Marks the method as "was called"
   *  This feature is intended for coverage tests only.
   */
  inline void mark_called () const
  {
    m_called = true;
  }

  /**
   *  @brief Resets the "was called" flag.
   *  This feature is intended for coverage tests only.
   */
  inline void reset_called () const
  {
    m_called = false;
  }

private:
  mutable bool m_called;

#else
  inline void mark_called () const { }
  inline void reset_called () const { }
  inline bool was_called () const { return true; }
#endif

private:
  std::string m_name, m_doc;
  std::vector<ArgType> m_arg_types;
  ArgType m_ret_type;
  bool m_const : 1;
  bool m_static : 1;
  bool m_protected : 1;
  unsigned int m_argsize;
  std::vector<MethodSynonym> m_method_synonyms;

  void parse_name (const std::string &);
};

/**
 *  @brief Provide a hook class for the special built-in methods such as "dup", "assign" etc.
 */
class SpecialMethod
  : public MethodBase
{
public:
  /**
   *  @brief Constructor
   */
  SpecialMethod (const std::string &name, const std::string &doc, bool c, bool s, special_method_type smt)
    : MethodBase (name, doc, c, s), m_smt (smt)
  { 
    // .. nothing yet ..
  }

  /**
   *  @brief Gets the special method type
   */
  virtual special_method_type smt () const
  {
    return m_smt;
  }

  virtual MethodBase *clone () const
  {
    return new SpecialMethod (*this);
  }

private:
  special_method_type m_smt;
};

/**
 *  @brief A collection of methods
 *
 *  The basic purpose of this object is to provide the + operator that allows concatenation
 *  method declarations in the class declaration.
 */
class GSI_PUBLIC Methods
{
public:
  typedef std::vector<MethodBase *>::const_iterator iterator;

  Methods ()
    : m_methods ()
  {
    // .. nothing yet ..
  }

  explicit Methods (MethodBase *m)
    : m_methods ()
  {
    m_methods.push_back (m);
  }

  Methods (const Methods &d)
  {
    operator= (d);
  }

  Methods &operator= (const Methods &d)
  {
    if (this != &d) {
      clear ();
      m_methods.reserve (d.m_methods.size ());
      for (std::vector<MethodBase *>::const_iterator m = d.m_methods.begin (); m != d.m_methods.end (); ++m) {
        m_methods.push_back ((*m)->clone ());
      }
    }
    return *this;
  }

  ~Methods ()
  {
    clear ();
  }

  void initialize ()
  {
    for (std::vector<MethodBase *>::iterator m = m_methods.begin (); m != m_methods.end (); ++m) {
      (*m)->initialize ();
    }
  }

  void clear ()
  {
    for (std::vector<MethodBase *>::iterator m = m_methods.begin (); m != m_methods.end (); ++m) {
      delete *m;
    }
    m_methods.clear ();
  }

  //  HINT: this is not the usual + semantics but this is more effective
  Methods &operator+ (const Methods &m)
  {
    return operator+= (m);
  }

  //  HINT: this is not the usual + semantics but this is more effective
  Methods &operator+ (MethodBase *m)
  {
    return operator+= (m);
  }

  Methods &operator+= (const Methods &m)
  {
    for (std::vector<MethodBase *>::const_iterator mm = m.m_methods.begin (); mm != m.m_methods.end (); ++mm) 
    {
      add_method ((*mm)->clone ());
    }
    return *this;
  }

  Methods &operator+= (MethodBase *m)
  {
    add_method (m);
    return *this;
  }

  iterator begin () const
  {
    return m_methods.begin ();
  }

  iterator end () const
  {
    return m_methods.end ();
  }

  void add_method (MethodBase *method)
  {
    m_methods.push_back (method);
  }

  size_t size () const
  {
    return m_methods.size ();
  }

  void swap (Methods &other) 
  {
    m_methods.swap (other.m_methods);
  }

public:
  std::vector<MethodBase *> m_methods;
};

inline Methods operator+ (const Methods &a, const Methods &b)
{
  return Methods (a) + b;
}

template <class X>
class MethodSpecificBase 
  : public MethodBase
{
public:
  MethodSpecificBase (const std::string &name, const std::string &doc, bool c, bool s, Callback X::*cb)
    : MethodBase (name, doc, c, s),
      m_cb (cb)
  { 
    // .. nothing yet ..
  }

  virtual void set_callback (void *v, const Callback &cb) const
  {
    typedef typename non_const_x<X>::nc_x nc_x;
    nc_x *x = (nc_x *)v;
    x->*m_cb = cb;
  }

  virtual bool is_callback () const
  {
    return m_cb != 0;
  }

  Callback X::*callback () const
  {
    return m_cb;
  }

private:
  Callback X::*m_cb;
};

class StaticMethodBase 
  : public MethodBase
{
public:
  StaticMethodBase (const std::string &name, const std::string &doc, bool is_const = false)
    : MethodBase (name, doc, is_const, true)
  { 
    // .. nothing yet ..
  }
};

/**
 *  @brief Standard argument key
 *  With that key, an unnamed argument without a default value will be generated.
 */
inline ArgSpec<void> arg () 
{
  return ArgSpec<void> ();
}

/**
 *  @brief Named argument key
 *  With that key, a named argument without a default value will be generated.
 */
inline ArgSpec<void> arg (const std::string &name) 
{
  return ArgSpec<void> (name);
}

/**
 *  @brief Optional, named argument key
 *  With that key, a named argument with a default value will be generated.
 */
template <class T>
inline ArgSpec<T> arg (const std::string &name, T t) 
{
  return ArgSpec<T> (name, t);
}

/**
 *  @brief Optional, named argument key and a documentation string for the initial value
 *  With that key, a named argument with a default value will be generated.
 */
template <class T>
inline ArgSpec<T> arg (const std::string &name, T t, const std::string &t_doc) 
{
  return ArgSpec<T> (name, t, t_doc);
}

/**
 *  @brief A helper class to create a constant (a static method with "const" attribute, not taking any arguments)
 *  This specific construct is required for Python to make constants not having upper-case names.
 */
template <class R>
class ConstantGetter
  : public StaticMethodBase
{
public:
  ConstantGetter (const std::string &name, R (*m) (), const std::string &doc)
    : StaticMethodBase (name, doc, true), m_m (m)
  { 
  }

  void initialize ()
  {
    this->clear ();
    //  Note: a constant must not return a reference to an existing object, hence "set_return_new":
    this->template set_return_new<R> ();
  }

  virtual MethodBase *clone () const 
  {
    return new ConstantGetter (*this);
  }

  virtual void call (void *, SerialArgs &, SerialArgs &ret) const 
  {
    mark_called ();
    ret.write<R> ((*m_m) ());
  }

private:
  R (*m_m) ();
};

template <class R>
Methods
constant (const std::string &name, R (*m) (), const std::string &doc = std::string ())
{
  return Methods (new ConstantGetter <R> (name, m, doc));
}

/**
 *  @brief A helper class to create a constant (a static method with "const" attribute, not taking any arguments)
 *  This version creates a constant getter from a real constant value.
 */
template <class R>
class ConstantValueGetter
  : public StaticMethodBase
{
public:
  ConstantValueGetter (const std::string &name, const R &v, const std::string &doc)
    : StaticMethodBase (name, doc, true), m_v (v)
  {
  }

  void initialize ()
  {
    this->clear ();
    //  Note: a constant must not return a reference to an existing object, hence "set_return_new":
    this->template set_return_new<R> ();
  }

  virtual MethodBase *clone () const
  {
    return new ConstantValueGetter (*this);
  }

  virtual void call (void *, SerialArgs &, SerialArgs &ret) const
  {
    mark_called ();
    ret.write<R> (m_v);
  }

private:
  R m_v;
};

template <class R>
Methods
constant (const std::string &name, const R &v, const std::string &doc = std::string ())
{
  return Methods (new ConstantValueGetter <R> (name, v, doc));
}

// 0 argument

#define _COUNT        0
#define _NAME(x)      x##0
#define _TMPLARG
#define _TMPLARGSPECS
#define _FUNCARGLIST  
#define _ADDARGS      
#define _GETARGVARS   
#define _ARGVARLIST   
#define _ARGSPECARGS   
#define _ARGSPEC
#define _ARGSPECS
#define _ARGSPECINIT
#define _ARGSPECMEM

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 1 argument

#define _COUNT        1
#define _NAME(x)      x##1
#define _TMPLARG      class A1
#define _TMPLARGSPECS class S1
#define _FUNCARGLIST  A1
#define _ADDARGS      this->template add_arg<A1> (m_s1);
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init ();
#define _ARGVARLIST   a1
#define _ARGSPECARGS  s1
#define _ARGSPEC      const ArgSpec<A1> &s1
#define _ARGSPECS     const ArgSpec<S1> &s1
#define _ARGSPECINIT  m_s1 = s1;
#define _ARGSPECMEM   ArgSpec<A1> m_s1;

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 2 arguments

#define _COUNT        2
#define _NAME(x)      x##2
#define _TMPLARG      class A1, class A2
#define _TMPLARGSPECS class S1, class S2
#define _FUNCARGLIST  A1, A2
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init ();
#define _ARGVARLIST   a1, a2
#define _ARGSPECARGS  s1, s2
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 3 arguments

#define _COUNT        3
#define _NAME(x)      x##3
#define _TMPLARG      class A1, class A2, class A3
#define _TMPLARGSPECS class S1, class S2, class S3
#define _FUNCARGLIST  A1, A2, A3
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init ();
#define _ARGVARLIST   a1, a2, a3
#define _ARGSPECARGS  s1, s2, s3
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 4 arguments

#define _COUNT        4
#define _NAME(x)      x##4
#define _TMPLARG      class A1, class A2, class A3, class A4
#define _TMPLARGSPECS class S1, class S2, class S3, class S4
#define _FUNCARGLIST  A1, A2, A3, A4
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init ();
#define _ARGVARLIST   a1, a2, a3, a4
#define _ARGSPECARGS  s1, s2, s3, s4
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 5 arguments

#define _COUNT        5
#define _NAME(x)      x##5
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5
#define _FUNCARGLIST  A1, A2, A3, A4, A5
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5
#define _ARGSPECARGS  s1, s2, s3, s4, s5
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 6 arguments

#define _COUNT        6
#define _NAME(x)      x##6
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 7 arguments

#define _COUNT        7
#define _NAME(x)      x##7
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 8 arguments

#define _COUNT        8
#define _NAME(x)      x##8
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 9 arguments

#define _COUNT        9
#define _NAME(x)      x##9
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 10 arguments

#define _COUNT        10
#define _NAME(x)      x##10
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 11 arguments

#define _COUNT        11
#define _NAME(x)      x##11
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10, class S11
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); \
                      this->template add_arg<A11> (m_s11); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init (); \
                      A11 a11 = args ? args.template read<A11> (heap, &m_s11) : m_s11.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10, const ArgSpec<A11> &s11
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10, const ArgSpec<S11> &s11
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; m_s11 = s11; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; \
                      ArgSpec<A11> m_s11; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 12 arguments

#define _COUNT        12
#define _NAME(x)      x##12
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10, class S11, class S12
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); \
                      this->template add_arg<A11> (m_s11); \
                      this->template add_arg<A12> (m_s12); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init (); \
                      A11 a11 = args ? args.template read<A11> (heap, &m_s11) : m_s11.init (); \
                      A12 a12 = args ? args.template read<A12> (heap, &m_s12) : m_s12.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10, const ArgSpec<A11> &s11, const ArgSpec<A12> &s12
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10, const ArgSpec<S11> &s11, const ArgSpec<S12> &s12
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; m_s11 = s11; m_s12 = s12; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; \
                      ArgSpec<A11> m_s11; \
                      ArgSpec<A12> m_s12; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 13 arguments

#define _COUNT        13
#define _NAME(x)      x##13
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10, class S11, class S12, class S13
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); \
                      this->template add_arg<A11> (m_s11); \
                      this->template add_arg<A12> (m_s12); \
                      this->template add_arg<A13> (m_s13); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init (); \
                      A11 a11 = args ? args.template read<A11> (heap, &m_s11) : m_s11.init (); \
                      A12 a12 = args ? args.template read<A12> (heap, &m_s12) : m_s12.init (); \
                      A13 a13 = args ? args.template read<A13> (heap, &m_s13) : m_s13.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10, const ArgSpec<A11> &s11, const ArgSpec<A12> &s12, const ArgSpec<A13> &s13
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10, const ArgSpec<S11> &s11, const ArgSpec<S12> &s12, const ArgSpec<S13> &s13
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; m_s11 = s11; m_s12 = s12; m_s13 = s13; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; \
                      ArgSpec<A11> m_s11; \
                      ArgSpec<A12> m_s12; \
                      ArgSpec<A13> m_s13; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 14 arguments

#define _COUNT        14
#define _NAME(x)      x##14
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10, class S11, class S12, class S13, class S14
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); \
                      this->template add_arg<A11> (m_s11); \
                      this->template add_arg<A12> (m_s12); \
                      this->template add_arg<A13> (m_s13); \
                      this->template add_arg<A14> (m_s14); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init (); \
                      A11 a11 = args ? args.template read<A11> (heap, &m_s11) : m_s11.init (); \
                      A12 a12 = args ? args.template read<A12> (heap, &m_s12) : m_s12.init (); \
                      A13 a13 = args ? args.template read<A13> (heap, &m_s13) : m_s13.init (); \
                      A14 a14 = args ? args.template read<A14> (heap, &m_s14) : m_s14.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10, const ArgSpec<A11> &s11, const ArgSpec<A12> &s12, const ArgSpec<A13> &s13, const ArgSpec<A14> &s14
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10, const ArgSpec<S11> &s11, const ArgSpec<S12> &s12, const ArgSpec<S13> &s13, const ArgSpec<S14> &s14
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; m_s11 = s11; m_s12 = s12; m_s13 = s13; m_s14 = s14; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; \
                      ArgSpec<A11> m_s11; \
                      ArgSpec<A12> m_s12; \
                      ArgSpec<A13> m_s13; \
                      ArgSpec<A14> m_s14; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 15 arguments

#define _COUNT        15
#define _NAME(x)      x##15
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10, class S11, class S12, class S13, class S14, class S15
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); \
                      this->template add_arg<A11> (m_s11); \
                      this->template add_arg<A12> (m_s12); \
                      this->template add_arg<A13> (m_s13); \
                      this->template add_arg<A14> (m_s14); \
                      this->template add_arg<A15> (m_s15); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init (); \
                      A11 a11 = args ? args.template read<A11> (heap, &m_s11) : m_s11.init (); \
                      A12 a12 = args ? args.template read<A12> (heap, &m_s12) : m_s12.init (); \
                      A13 a13 = args ? args.template read<A13> (heap, &m_s13) : m_s13.init (); \
                      A14 a14 = args ? args.template read<A14> (heap, &m_s14) : m_s14.init (); \
                      A15 a15 = args ? args.template read<A15> (heap, &m_s15) : m_s15.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10, const ArgSpec<A11> &s11, const ArgSpec<A12> &s12, const ArgSpec<A13> &s13, const ArgSpec<A14> &s14, const ArgSpec<A15> &s15
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10, const ArgSpec<S11> &s11, const ArgSpec<S12> &s12, const ArgSpec<S13> &s13, const ArgSpec<S14> &s14, const ArgSpec<S15> &s15
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; m_s11 = s11; m_s12 = s12; m_s13 = s13; m_s14 = s14; m_s15 = s15; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; \
                      ArgSpec<A11> m_s11; \
                      ArgSpec<A12> m_s12; \
                      ArgSpec<A13> m_s13; \
                      ArgSpec<A14> m_s14; \
                      ArgSpec<A15> m_s15; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

// 16 arguments

#define _COUNT        16
#define _NAME(x)      x##16
#define _TMPLARG      class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15, class A16
#define _TMPLARGSPECS class S1, class S2, class S3, class S4, class S5, class S6, class S7, class S8, class S9, class S10, class S11, class S12, class S13, class S14, class S15, class S16
#define _FUNCARGLIST  A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16
#define _ADDARGS      this->template add_arg<A1> (m_s1); \
                      this->template add_arg<A2> (m_s2); \
                      this->template add_arg<A3> (m_s3); \
                      this->template add_arg<A4> (m_s4); \
                      this->template add_arg<A5> (m_s5); \
                      this->template add_arg<A6> (m_s6); \
                      this->template add_arg<A7> (m_s7); \
                      this->template add_arg<A8> (m_s8); \
                      this->template add_arg<A9> (m_s9); \
                      this->template add_arg<A10> (m_s10); \
                      this->template add_arg<A11> (m_s11); \
                      this->template add_arg<A12> (m_s12); \
                      this->template add_arg<A13> (m_s13); \
                      this->template add_arg<A14> (m_s14); \
                      this->template add_arg<A15> (m_s15); \
                      this->template add_arg<A16> (m_s16); 
#define _GETARGVARS   tl::Heap heap;\
                      A1 a1 = args ? args.template read<A1> (heap, &m_s1) : m_s1.init (); \
                      A2 a2 = args ? args.template read<A2> (heap, &m_s2) : m_s2.init (); \
                      A3 a3 = args ? args.template read<A3> (heap, &m_s3) : m_s3.init (); \
                      A4 a4 = args ? args.template read<A4> (heap, &m_s4) : m_s4.init (); \
                      A5 a5 = args ? args.template read<A5> (heap, &m_s5) : m_s5.init (); \
                      A6 a6 = args ? args.template read<A6> (heap, &m_s6) : m_s6.init (); \
                      A7 a7 = args ? args.template read<A7> (heap, &m_s7) : m_s7.init (); \
                      A8 a8 = args ? args.template read<A8> (heap, &m_s8) : m_s8.init (); \
                      A9 a9 = args ? args.template read<A9> (heap, &m_s9) : m_s9.init (); \
                      A10 a10 = args ? args.template read<A10> (heap, &m_s10) : m_s10.init (); \
                      A11 a11 = args ? args.template read<A11> (heap, &m_s11) : m_s11.init (); \
                      A12 a12 = args ? args.template read<A12> (heap, &m_s12) : m_s12.init (); \
                      A13 a13 = args ? args.template read<A13> (heap, &m_s13) : m_s13.init (); \
                      A14 a14 = args ? args.template read<A14> (heap, &m_s14) : m_s14.init (); \
                      A15 a15 = args ? args.template read<A15> (heap, &m_s15) : m_s15.init (); \
                      A16 a16 = args ? args.template read<A16> (heap, &m_s16) : m_s16.init ();
#define _ARGVARLIST   a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16
#define _ARGSPECARGS  s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16
#define _ARGSPEC      const ArgSpec<A1> &s1, const ArgSpec<A2> &s2, const ArgSpec<A3> &s3, const ArgSpec<A4> &s4, const ArgSpec<A5> &s5, const ArgSpec<A6> &s6, const ArgSpec<A7> &s7, const ArgSpec<A8> &s8, const ArgSpec<A9> &s9, const ArgSpec<A10> &s10, const ArgSpec<A11> &s11, const ArgSpec<A12> &s12, const ArgSpec<A13> &s13, const ArgSpec<A14> &s14, const ArgSpec<A15> &s15, const ArgSpec<A16> &s16
#define _ARGSPECS     const ArgSpec<S1> &s1, const ArgSpec<S2> &s2, const ArgSpec<S3> &s3, const ArgSpec<S4> &s4, const ArgSpec<S5> &s5, const ArgSpec<S6> &s6, const ArgSpec<S7> &s7, const ArgSpec<S8> &s8, const ArgSpec<S9> &s9, const ArgSpec<S10> &s10, const ArgSpec<S11> &s11, const ArgSpec<S12> &s12, const ArgSpec<S13> &s13, const ArgSpec<S14> &s14, const ArgSpec<S15> &s15, const ArgSpec<S16> &s16
#define _ARGSPECINIT  m_s1 = s1; m_s2 = s2; m_s3 = s3; m_s4 = s4; m_s5 = s5; m_s6 = s6; m_s7 = s7; m_s8 = s8; m_s9 = s9; m_s10 = s10; m_s11 = s11; m_s12 = s12; m_s13 = s13; m_s14 = s14; m_s15 = s15; m_s16 = s16; 
#define _ARGSPECMEM   ArgSpec<A1> m_s1; \
                      ArgSpec<A2> m_s2; \
                      ArgSpec<A3> m_s3; \
                      ArgSpec<A4> m_s4; \
                      ArgSpec<A5> m_s5; \
                      ArgSpec<A6> m_s6; \
                      ArgSpec<A7> m_s7; \
                      ArgSpec<A8> m_s8; \
                      ArgSpec<A9> m_s9; \
                      ArgSpec<A10> m_s10; \
                      ArgSpec<A11> m_s11; \
                      ArgSpec<A12> m_s12; \
                      ArgSpec<A13> m_s13; \
                      ArgSpec<A14> m_s14; \
                      ArgSpec<A15> m_s15; \
                      ArgSpec<A16> m_s16; 

#include "gsiMethodsVar.h"

#undef _ARGSPECARGS   
#undef _ARGSPEC
#undef _ARGSPECS
#undef _ARGSPECINIT
#undef _ARGSPECMEM
#undef _ARGVARLIST   
#undef _GETARGVARS   
#undef _ADDARGS
#undef _FUNCARGLIST
#undef _TMPLARG
#undef _TMPLARGSPECS
#undef _NAME
#undef _COUNT

}

#endif


