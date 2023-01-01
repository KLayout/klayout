
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


//  This header by intention does not have a include guard
//  It is used multiple times inside gsiCallback.h
//
//  It needs these macros to be defined (example for the one argument case)
//   _COUNT        "1"
//   _NAME(x)      "x##1"
//   _TMPLARG      "class A1"
//   _TMPLARGSPECS "class S1"
//   _FUNCARGLIST  "A1"
//   _ADDARGS      "this->template add_arg<A1> (m_s1);"
//   _GETARGVARS   "A1 a1 = args ? args.template read<A1> (heap) : m_s1.init ();"
//   _ARGVARLIST   "a1"
//   _ARGSPECARGS  "s1"
//   _ARGSPEC      "const ArgSpec<A1> &s1"
//   _ARGSPECS     "const ArgSpec<S1> &s1"
//   _ARGSPECINIT  "m_s1 = s1;"
//   _ARGSPECMEM   "ArgSpec<A1> m_s1;"

#undef _COMMA
#if _COUNT != 0
#define _COMMA ,
#else
#define _COMMA
#endif

template <class X _COMMA _TMPLARG>
class _NAME(MethodVoid)
  : public MethodSpecificBase <X>
{
public:
  _NAME(MethodVoid) (const std::string &name, void (X::*m) (_FUNCARGLIST),  const std::string &doc, gsi::Callback X::*cb = 0)
    : MethodSpecificBase <X> (name, doc, false, false, cb), m_m (m) 
  { 
  }

  _NAME(MethodVoid) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(MethodVoid) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    (((X *)cls)->*m_m) (_ARGVARLIST);
  }

private:
  void (X::*m_m) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X _COMMA _TMPLARG>
class _NAME(ConstMethodVoid)
  : public MethodSpecificBase <X>
{
public:
  _NAME(ConstMethodVoid) (const std::string &name, void (X::*m) (_FUNCARGLIST) const, const std::string &doc, gsi::Callback X::*cb = 0)
    : MethodSpecificBase <X> (name, doc, true, false, cb), m_m (m)
  { 
  }

  _NAME(ConstMethodVoid) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ConstMethodVoid) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    (((const X *)cls)->*m_m) (_ARGVARLIST);
  }

private:
  void (X::*m_m) (_FUNCARGLIST) const;
  _ARGSPECMEM
};

template <class X _COMMA _TMPLARG>
class _NAME(ExtMethodVoid)
  : public MethodSpecificBase <X>
{
public:
  _NAME(ExtMethodVoid) (const std::string &name, void (*xm) (X * _COMMA _FUNCARGLIST), const std::string &doc, gsi::Callback X::*cb = 0)
    : MethodSpecificBase <X> (name, doc, is_const_x<X>::value (), false, cb), m_xm (xm)
  { 
  }

  _NAME(ExtMethodVoid) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ExtMethodVoid) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    (*m_xm) ((X *)cls _COMMA _ARGVARLIST);
  }

private:
  void (*m_xm) (X * _COMMA _FUNCARGLIST);
  _ARGSPECMEM
};

#if _COUNT != 0
template <_TMPLARG>
#endif
class _NAME(StaticMethodVoid)
  : public StaticMethodBase
{
public:
  _NAME(StaticMethodVoid) (const std::string &name, void (*m) (_FUNCARGLIST), const std::string &doc)
    : StaticMethodBase (name, doc), m_m (m)
  { 
  }

  _NAME(StaticMethodVoid) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(StaticMethodVoid) (*this);
  }

#if _COUNT != 0
  virtual void call (void *, SerialArgs &args, SerialArgs &) const 
#else
  virtual void call (void *, SerialArgs &, SerialArgs &) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    (*m_m) (_ARGVARLIST);
  }

private:
  void (*m_m) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(Method)
  : public MethodSpecificBase <X>
{
public:
  _NAME(Method) (const std::string &name, R (X::*m) (_FUNCARGLIST), const std::string &doc, gsi::Callback X::*cb = 0)
    : MethodSpecificBase <X> (name, doc, false, false, cb), m_m (m)
  { 
  }

  _NAME(Method) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<R, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(Method) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    ret.write<R> ((((X *)cls)->*m_m) (_ARGVARLIST));
  }

private:
  R (X::*m_m) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ConstMethod)
  : public MethodSpecificBase <X>
{
public:
  _NAME(ConstMethod) (const std::string &name, R (X::*m) (_FUNCARGLIST) const, const std::string &doc, gsi::Callback X::*cb = 0)
    : MethodSpecificBase <X> (name, doc, true, false, cb), m_m (m)
  { 
  }

  _NAME(ConstMethod) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<R, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ConstMethod) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    ret.write<R> ((((const X *)cls)->*m_m) (_ARGVARLIST));
  }

private:
  R (X::*m_m) (_FUNCARGLIST) const;
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ExtMethod)
  : public MethodBase
{
public:
  _NAME(ExtMethod) (const std::string &name, R (*xm) (X * _COMMA _FUNCARGLIST), const std::string &doc)
    : MethodBase (name, doc, is_const_x<X>::value (), false), m_xm (xm)
  { 
  }

  _NAME(ExtMethod) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<R, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ExtMethod) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    ret.write<R> ((*m_xm) ((X *)cls _COMMA _ARGVARLIST));
  }

private:
  R (*m_xm) (X * _COMMA _FUNCARGLIST);
  _ARGSPECMEM
};

template <class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(StaticMethod)
  : public StaticMethodBase
{
public:
  _NAME(StaticMethod) (const std::string &name, R (*m) (_FUNCARGLIST), const std::string &doc)
    : StaticMethodBase (name, doc), m_m (m)
  { 
  }

  _NAME(StaticMethod) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<R, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(StaticMethod) (*this);
  }

#if _COUNT != 0
  virtual void call (void *, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS;
    ret.write<R> ((*m_m) (_ARGVARLIST));
  }

private:
  R (*m_m) (_FUNCARGLIST);
  _ARGSPECMEM
};

//  pointer iterator method descriptors

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(MethodPtrIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(MethodPtrIter) (const std::string &name, R *(X::*b) (_FUNCARGLIST), R *(X::*e) (_FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, false, false, 0), m_b (b), m_e (e)
  { 
  }

  _NAME(MethodPtrIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(MethodPtrIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((X *)cls)->*m_b) (_ARGVARLIST), (((X *)cls)->*m_e) (_ARGVARLIST))));
  }

private:
  R *(X::*m_b) (_FUNCARGLIST);
  R *(X::*m_e) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(MethodPtrConstIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef ConstIterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(MethodPtrConstIter) (const std::string &name, R const *(X::*b) (_FUNCARGLIST), R const *(X::*e) (_FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, false, false, 0), m_b (b), m_e (e)
  { 
  }

  _NAME(MethodPtrConstIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(MethodPtrConstIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((X *)cls)->*m_b) (_ARGVARLIST), (((X *)cls)->*m_e) (_ARGVARLIST))));
  }

private:
  R const *(X::*m_b) (_FUNCARGLIST);
  R const *(X::*m_e) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ConstMethodPtrIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(ConstMethodPtrIter) (const std::string &name, R *(X::*b) (_FUNCARGLIST) const, R *(X::*e) (_FUNCARGLIST) const, const std::string &doc)
    : MethodSpecificBase <X> (name, doc, true, false, 0), m_b (b), m_e (e)
  { 
  }

  _NAME(ConstMethodPtrIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ConstMethodPtrIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((const X *)cls)->*m_b) (_ARGVARLIST), (((const X *)cls)->*m_e) (_ARGVARLIST))));
  }

private:
  R *(X::*m_b) (_FUNCARGLIST) const;
  R *(X::*m_e) (_FUNCARGLIST) const;
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ConstMethodPtrConstIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef ConstIterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(ConstMethodPtrConstIter) (const std::string &name, R const *(X::*b) (_FUNCARGLIST) const, R const *(X::*e) (_FUNCARGLIST) const, const std::string &doc)
    : MethodSpecificBase <X> (name, doc, true, false, 0), m_b (b), m_e (e)
  { 
  }

  _NAME(ConstMethodPtrConstIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ConstMethodPtrConstIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((const X *)cls)->*m_b) (_ARGVARLIST), (((const X *)cls)->*m_e) (_ARGVARLIST))));
  }

private:
  R const *(X::*m_b) (_FUNCARGLIST) const;
  R const *(X::*m_e) (_FUNCARGLIST) const;
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ExtMethodPtrIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(ExtMethodPtrIter) (const std::string &name, R *(*xb) (X * _COMMA _FUNCARGLIST), R *(*xe) (X * _COMMA _FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, is_const_x<X>::value (), false, 0), m_xb (xb), m_xe (xe)
  { 
  }

  _NAME(ExtMethodPtrIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ExtMethodPtrIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_xb) ((X *)cls _COMMA _ARGVARLIST), (*m_xe) ((X *)cls _COMMA _ARGVARLIST))));
  }

private:
  R *(*m_xb) (X * _COMMA _FUNCARGLIST);
  R *(*m_xe) (X * _COMMA _FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ExtMethodPtrConstIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef ConstIterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(ExtMethodPtrConstIter) (const std::string &name, R const *(*xb) (X * _COMMA _FUNCARGLIST), R const *(*xe) (X * _COMMA _FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, is_const_x<X>::value (), false, 0), m_xb (xb), m_xe (xe)
  { 
  }

  _NAME(ExtMethodPtrConstIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ExtMethodPtrConstIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_xb) ((X *)cls _COMMA _ARGVARLIST), (*m_xe) ((X *)cls _COMMA _ARGVARLIST))));
  }

private:
  R const *(*m_xb) (X * _COMMA _FUNCARGLIST);
  R const *(*m_xe) (X * _COMMA _FUNCARGLIST);
  _ARGSPECMEM
};

template <class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(StaticMethodPtrIter) 
  : public StaticMethodBase
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(StaticMethodPtrIter) (const std::string &name, R *(*b) (_FUNCARGLIST), R *(*e) (_FUNCARGLIST), const std::string &doc)
    : StaticMethodBase (name, doc), m_b (b), m_e (e)
  { 
  }

  _NAME(StaticMethodPtrIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(StaticMethodPtrIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void * /*cls*/, SerialArgs &args, SerialArgs &ret) const
#else
  virtual void call (void * /*cls*/, SerialArgs &, SerialArgs &ret) const
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_b) (_ARGVARLIST), ((*m_e) (_ARGVARLIST)))));
  }

private:
  R *(*m_b) (_FUNCARGLIST);
  R *(*m_e) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class R _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(StaticMethodPtrConstIter) 
  : public StaticMethodBase
{
public:
  typedef R value_type;
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef ConstIterPtrAdaptor<value_type> iter_adaptor_type;

  _NAME(StaticMethodPtrConstIter) (const std::string &name, R const *(*b) (_FUNCARGLIST), R const *(*e) (_FUNCARGLIST), const std::string &doc)
    : StaticMethodBase (name, doc), m_b (b), m_e (e)
  { 
  }

  _NAME(StaticMethodPtrConstIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(StaticMethodPtrConstIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void * /*cls*/, SerialArgs &args, SerialArgs &ret) const
#else
  virtual void call (void * /*cls*/, SerialArgs &, SerialArgs &ret) const
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_b) (_ARGVARLIST), ((*m_e) (_ARGVARLIST)))));
  }

private:
  R const *(*m_b) (_FUNCARGLIST);
  R const *(*m_e) (_FUNCARGLIST);
  _ARGSPECMEM
};

//  pair iterator method descriptors

template <class X, class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(MethodBiIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterAdaptor<I> iter_adaptor_type;

  _NAME(MethodBiIter) (const std::string &name, I (X::*b) (_FUNCARGLIST), I (X::*e) (_FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, false, false, 0), m_b (b), m_e (e)
  { 
  }

  _NAME(MethodBiIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(MethodBiIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((X *)cls)->*m_b) (_ARGVARLIST), (((X *)cls)->*m_e) (_ARGVARLIST))));
  }

private:
  I (X::*m_b) (_FUNCARGLIST);
  I (X::*m_e) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ConstMethodBiIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterAdaptor<I> iter_adaptor_type;

  _NAME(ConstMethodBiIter) (const std::string &name, I (X::*b) (_FUNCARGLIST) const, I (X::*e) (_FUNCARGLIST) const, const std::string &doc)
    : MethodSpecificBase <X> (name, doc, true, false, 0), m_b (b), m_e (e)
  { 
  }

  _NAME(ConstMethodBiIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ConstMethodBiIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((const X *)cls)->*m_b) (_ARGVARLIST), (((const X *)cls)->*m_e) (_ARGVARLIST))));
  }

private:
  I (X::*m_b) (_FUNCARGLIST) const;
  I (X::*m_e) (_FUNCARGLIST) const;
  _ARGSPECMEM
};

template <class X, class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ExtMethodBiIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterAdaptor<I> iter_adaptor_type;

  _NAME(ExtMethodBiIter) (const std::string &name, I (*xb) (X * _COMMA _FUNCARGLIST), I (*xe) (X * _COMMA _FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, is_const_x<X>::value (), false, 0), m_xb (xb), m_xe (xe)
  { 
  }

  _NAME(ExtMethodBiIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ExtMethodBiIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    I b = (*m_xb) ((X *)cls _COMMA _ARGVARLIST);
    I e = (*m_xe) ((X *)cls _COMMA _ARGVARLIST);
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type (b, e)));
  }

private:
  I (*m_xb) (X * _COMMA _FUNCARGLIST);
  I (*m_xe) (X * _COMMA _FUNCARGLIST);
  _ARGSPECMEM
};

template <class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(StaticMethodBiIter) 
  : public StaticMethodBase
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef IterAdaptor<I> iter_adaptor_type;

  _NAME(StaticMethodBiIter) (const std::string &name, I (*b) (_FUNCARGLIST), I (*e) (_FUNCARGLIST), const std::string &doc)
    : StaticMethodBase (name, doc), m_b (b), m_e (e)
  { 
  }

  _NAME(StaticMethodBiIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(StaticMethodBiIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_b) (_ARGVARLIST), ((*m_e) (_ARGVARLIST)))));
  }

private:
  I (*m_b) (_FUNCARGLIST);
  I (*m_e) (_FUNCARGLIST);
  _ARGSPECMEM
};

//  free iterator method descriptors

template <class X, class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(MethodFreeIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef FreeIterAdaptor<I> iter_adaptor_type;

  _NAME(MethodFreeIter) (const std::string &name, I (X::*i) (_FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, false, false, 0), m_i (i)
  { 
  }

  _NAME(MethodFreeIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(MethodFreeIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((X *)cls)->*m_i) (_ARGVARLIST))));
  }

private:
  I (X::*m_i) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X, class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ConstMethodFreeIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef FreeIterAdaptor<I> iter_adaptor_type;

  _NAME(ConstMethodFreeIter) (const std::string &name, I (X::*i) (_FUNCARGLIST) const, const std::string &doc)
    : MethodSpecificBase <X> (name, doc, true, false, 0), m_i (i)
  { 
  }

  _NAME(ConstMethodFreeIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ConstMethodFreeIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((((const X *)cls)->*m_i) (_ARGVARLIST))));
  }

private:
  I (X::*m_i) (_FUNCARGLIST) const;
  _ARGSPECMEM
};

template <class X, class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(ExtMethodFreeIter) 
  : public MethodSpecificBase <X>
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef FreeIterAdaptor<I> iter_adaptor_type;

  _NAME(ExtMethodFreeIter) (const std::string &name, I (*xi) (X * _COMMA _FUNCARGLIST), const std::string &doc)
    : MethodSpecificBase <X> (name, doc, is_const_x<X>::value (), false, 0), m_xi (xi)
  { 
  }

  _NAME(ExtMethodFreeIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(ExtMethodFreeIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void *cls, SerialArgs &args, SerialArgs &ret) const 
#else
  virtual void call (void *cls, SerialArgs &, SerialArgs &ret) const 
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_xi) ((X *)cls _COMMA _ARGVARLIST))));
  }

private:
  I (*m_xi) (X * _COMMA _FUNCARGLIST);
  _ARGSPECMEM
};

template <class I _COMMA _TMPLARG, class Transfer = gsi::arg_default_return_value_preference>
class _NAME(StaticMethodFreeIter) 
  : public StaticMethodBase
{
public:
  typedef IterAdaptorAbstractBase iter_adaptor_base_type;
  typedef FreeIterAdaptor<I> iter_adaptor_type;

  _NAME(StaticMethodFreeIter) (const std::string &name, I (*i) (_FUNCARGLIST), const std::string &doc)
    : StaticMethodBase (name, doc), m_i (i)
  { 
  }

  _NAME(StaticMethodFreeIter) *add_args (_ARGSPEC) 
  {
    _ARGSPECINIT;
    return this;
  }

  void initialize ()
  {
    this->clear ();
    _ADDARGS
    this->template set_return<iter_adaptor_type, Transfer> ();
  }

  virtual MethodBase *clone () const 
  {
    return new _NAME(StaticMethodFreeIter) (*this);
  }

#if _COUNT != 0
  virtual void call (void * /*cls*/, SerialArgs &args, SerialArgs &ret) const
#else
  virtual void call (void * /*cls*/, SerialArgs &, SerialArgs &ret) const
#endif
  {
    this->mark_called ();
    _GETARGVARS
    ret.write<iter_adaptor_base_type *> (static_cast<iter_adaptor_base_type *> (new iter_adaptor_type ((*m_i) (_ARGVARLIST))));
  }

private:
  I (*m_i) (_FUNCARGLIST);
  _ARGSPECMEM
};

template <class X _COMMA _TMPLARG>
Methods
method (const std::string &name, void (X::*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(MethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc));
}

#if _COUNT != 0
template <class X _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method (const std::string &name, void (X::*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(MethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X _COMMA _TMPLARG>
Methods
method_ext (const std::string &name, void (*xm) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(ExtMethodVoid) <X _COMMA _FUNCARGLIST> (name, xm, doc));
}

#if _COUNT != 0
template <class X _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method_ext (const std::string &name, void (*xm) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethodVoid) <X _COMMA _FUNCARGLIST> (name, xm, doc))->add_args (_ARGSPECARGS));
}
#endif

#if _COUNT != 0
template <_TMPLARG>
#else
inline
#endif
Methods
method (const std::string &name, void (*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
#if _COUNT != 0
  return Methods (new _NAME(StaticMethodVoid) <_FUNCARGLIST> (name, m, doc));
#else
  return Methods (new _NAME(StaticMethodVoid) (name, m, doc));
#endif
}

#if _COUNT != 0
template <_TMPLARG _COMMA _TMPLARGSPECS>
Methods
method (const std::string &name, void (*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethodVoid) <_FUNCARGLIST> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X _COMMA _TMPLARG>
Methods
callback (const std::string &name, void (X::*m) (_FUNCARGLIST), Callback X::*cb, const std::string &doc = std::string ())
{
  return Methods (new _NAME(MethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc, cb));
}

#if _COUNT != 0
template <class X _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
callback (const std::string &name, void (X::*m) (_FUNCARGLIST), Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(MethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}
#endif

template <class X _COMMA _TMPLARG>
Methods
method (const std::string &name, void (X::*m) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc));
}

#if _COUNT != 0
template <class X _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method (const std::string &name, void (X::*m) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X _COMMA _TMPLARG>
Methods
callback (const std::string &name, void (X::*m) (_FUNCARGLIST) const, Callback X::*cb, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc, cb));
}

#if _COUNT != 0
template <class X _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
callback (const std::string &name, void (X::*m) (_FUNCARGLIST) const, Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethodVoid) <X _COMMA _FUNCARGLIST> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
method (const std::string &name, R (X::*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(Method) <X, R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, m, doc));
}

template <class X, class R _COMMA _TMPLARG, class Transfer>
Methods
method (const std::string &name, Transfer, R (X::*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(Method) <X, R _COMMA _FUNCARGLIST, Transfer> (name, m, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method (const std::string &name, R (X::*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(Method) <X, R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, m, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
method (const std::string &name, Transfer, R (X::*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(Method) <X, R _COMMA _FUNCARGLIST, Transfer> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
factory (const std::string &name, R *(X::*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(Method) <X, R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
factory (const std::string &name, R *(X::*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(Method) <X, R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
method_ext (const std::string &name, R (*xm) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(ExtMethod) <X, R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, xm, doc));
}

template <class X, class R _COMMA _TMPLARG, class Transfer>
Methods
method_ext (const std::string &name, Transfer, R (*xm) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(ExtMethod) <X, R _COMMA _FUNCARGLIST, Transfer> (name, xm, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method_ext (const std::string &name, R (*xm) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethod) <X, R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, xm, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
method_ext (const std::string &name, Transfer, R (*xm) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethod) <X, R _COMMA _FUNCARGLIST, Transfer> (name, xm, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
factory_ext (const std::string &name, R *(*xm) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(ExtMethod) <X, R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, xm, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
factory_ext (const std::string &name, R *(*xm) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethod) <X, R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, xm, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X _COMMA _TMPLARG>
Methods
constructor (const std::string &name, X *(*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(StaticMethod) <X * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc));
}

#if _COUNT != 0
template <class X _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
constructor (const std::string &name, X *(*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethod) <X * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class R _COMMA _TMPLARG>
Methods
method (const std::string &name, R (*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(StaticMethod) <R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, m, doc));
}

template <class R _COMMA _TMPLARG, class Transfer>
Methods
method (const std::string &name, Transfer, R (*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(StaticMethod) <R _COMMA _FUNCARGLIST, Transfer> (name, m, doc));
}

#if _COUNT != 0
template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method (const std::string &name, R (*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethod) <R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, m, doc))->add_args (_ARGSPECARGS));
}

template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
method (const std::string &name, Transfer, R (*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethod) <R _COMMA _FUNCARGLIST, Transfer> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class R _COMMA _TMPLARG>
Methods
factory (const std::string &name, R *(*m) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(StaticMethod) <R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc));
}

#if _COUNT != 0
template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
factory (const std::string &name, R *(*m) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethod) <R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
callback (const std::string &name, R (X::*m) (_FUNCARGLIST), Callback X::*cb, const std::string &doc = std::string ())
{
  return Methods (new _NAME(Method) <X, R _COMMA _FUNCARGLIST> (name, m, doc, cb));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
callback (const std::string &name, R (X::*m) (_FUNCARGLIST), Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(Method) <X, R _COMMA _FUNCARGLIST> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
callback (const std::string &name, Transfer, R (X::*m) (_FUNCARGLIST), Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(Method) <X, R _COMMA _FUNCARGLIST> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
factory_callback (const std::string &name, R (X::*m) (_FUNCARGLIST), Callback X::*cb, const std::string &doc = std::string ())
{
  return Methods (new _NAME(Method) <X, R _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc, cb));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
factory_callback (const std::string &name, R (X::*m) (_FUNCARGLIST), Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(Method) <X, R _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
method (const std::string &name, R (X::*m) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, m, doc));
}

template <class X, class R _COMMA _TMPLARG, class Transfer>
Methods
method (const std::string &name, Transfer, R (X::*m) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, Transfer> (name, m, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
method (const std::string &name, R (X::*m) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, arg_default_return_value_preference> (name, m, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
method (const std::string &name, Transfer, R (X::*m) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, Transfer> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
factory (const std::string &name, R *(X::*m) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethod) <X, R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
factory (const std::string &name, R *(X::*m) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethod) <X, R * _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG>
Methods
callback (const std::string &name, R (X::*m) (_FUNCARGLIST) const, Callback X::*cb, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST> (name, m, doc, cb));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
callback (const std::string &name, R (X::*m) (_FUNCARGLIST) const, Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
callback (const std::string &name, Transfer, R (X::*m) (_FUNCARGLIST) const, Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, Transfer> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
factory_callback (const std::string &name, R (X::*m) (_FUNCARGLIST) const, Callback X::*cb, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc, cb));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
factory_callback (const std::string &name, R (X::*m) (_FUNCARGLIST) const, Callback X::*cb _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethod) <X, R _COMMA _FUNCARGLIST, gsi::return_new_object> (name, m, doc, cb))->add_args (_ARGSPECARGS));
}
#endif

//  pointer iterators

template <class R _COMMA _TMPLARG>
Methods
iterator (const std::string &name, R *(*b) (_FUNCARGLIST), R *(*e) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(StaticMethodPtrIter) <R _COMMA _FUNCARGLIST> (name, b, e, doc));
}

#if _COUNT != 0
template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, R *(*b) (_FUNCARGLIST), R *(*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethodPtrIter) <R _COMMA _FUNCARGLIST> (name, b, e, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer, R *(*b) (_FUNCARGLIST), R *(*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethodPtrIter) <R _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc))->add_args (_ARGSPECARGS));
}

template <class R _COMMA _TMPLARG>
Methods
iterator (const std::string &name, R const *(*b) (_FUNCARGLIST), R const *(*e) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(StaticMethodPtrConstIter) <R _COMMA _FUNCARGLIST> (name, b, e, doc));
}

#if _COUNT != 0
template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, R const *(*b) (_FUNCARGLIST), R const *(*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethodPtrConstIter) <R _COMMA _FUNCARGLIST> (name, b, e, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer, R const *(*b) (_FUNCARGLIST), R const *(*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(StaticMethodPtrConstIter) <R _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
iterator (const std::string &name, R *(X::*b) (_FUNCARGLIST), R *(X::*e) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(MethodPtrIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, R *(X::*b) (_FUNCARGLIST), R *(X::*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(MethodPtrIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer, R *(X::*b) (_FUNCARGLIST), R *(X::*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(MethodPtrIter) <X, R _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
iterator (const std::string &name, R const *(X::*b) (_FUNCARGLIST), R const *(X::*e) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(MethodPtrConstIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, R const *(X::*b) (_FUNCARGLIST), R const *(X::*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(MethodPtrConstIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer, R const *(X::*b) (_FUNCARGLIST), R const *(X::*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(MethodPtrConstIter) <X, R _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
iterator_ext (const std::string &name, R *(*xb) (X * _COMMA _FUNCARGLIST), R *(*xe) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(ExtMethodPtrIter) <X, R _COMMA _FUNCARGLIST> (name, xb, xe, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator_ext (const std::string &name, R *(*xb) (X * _COMMA _FUNCARGLIST), R *(*xe) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethodPtrIter) <X, R _COMMA _FUNCARGLIST> (name, xb, xe, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator_ext (const std::string &name, Transfer, R *(*xb) (X * _COMMA _FUNCARGLIST), R *(*xe) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethodPtrIter) <X, R _COMMA _FUNCARGLIST, Transfer> (name, xb, xe, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
iterator_ext (const std::string &name, R const *(*xb) (X * _COMMA _FUNCARGLIST), R const *(*xe) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (new _NAME(ExtMethodPtrConstIter) <X, R _COMMA _FUNCARGLIST> (name, xb, xe, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator_ext (const std::string &name, R const *(*xb) (X * _COMMA _FUNCARGLIST), R const *(*xe) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethodPtrConstIter) <X, R _COMMA _FUNCARGLIST> (name, xb, xe, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator_ext (const std::string &name, Transfer, R const *(*xb) (X * _COMMA _FUNCARGLIST), R const *(*xe) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ExtMethodPtrConstIter) <X, R _COMMA _FUNCARGLIST, Transfer> (name, xb, xe, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
iterator (const std::string &name, R *(X::*b) (_FUNCARGLIST) const, R *(X::*e) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethodPtrIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, R *(X::*b) (_FUNCARGLIST) const, R *(X::*e) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethodPtrIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer, R *(X::*b) (_FUNCARGLIST) const, R *(X::*e) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethodPtrIter) <X, R _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc))->add_args (_ARGSPECARGS));
}

template <class X, class R _COMMA _TMPLARG>
Methods
iterator (const std::string &name, R const *(X::*b) (_FUNCARGLIST) const, R const *(X::*e) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (new _NAME(ConstMethodPtrConstIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc));
}

#if _COUNT != 0
template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, R const *(X::*b) (_FUNCARGLIST) const, R const *(X::*e) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethodPtrConstIter) <X, R _COMMA _FUNCARGLIST> (name, b, e, doc))->add_args (_ARGSPECARGS));
}
#endif

template <class X, class R _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer, R const *(X::*b) (_FUNCARGLIST) const, R const *(X::*e) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods ((new _NAME(ConstMethodPtrConstIter) <X, R _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc))->add_args (_ARGSPECARGS));
}

//  pair iterators

template <class X, class I _COMMA _TMPLARG, class Transfer>
_NAME(MethodBiIter) <X, I _COMMA _FUNCARGLIST, Transfer> *
_iterator (const std::string &name, I (X::*b) (_FUNCARGLIST), I (X::*e) (_FUNCARGLIST), Transfer, const std::string &doc)
{
  return new _NAME(MethodBiIter) <X, I _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc);
}

template <class X, class I _COMMA _TMPLARG, class Transfer>
_NAME(ExtMethodBiIter) <X, I _COMMA _FUNCARGLIST, Transfer> *
_iterator_ext (const std::string &name, I (*xb) (X * _COMMA _FUNCARGLIST), I (*xe) (X * _COMMA _FUNCARGLIST), Transfer, const std::string &doc)
{
  return new _NAME(ExtMethodBiIter) <X, I _COMMA _FUNCARGLIST, Transfer> (name, xb, xe, doc);
}

template <class I _COMMA _TMPLARG, class Transfer>
_NAME(StaticMethodBiIter) <I _COMMA _FUNCARGLIST, Transfer> *
_iterator (const std::string &name, I (*b) (_FUNCARGLIST), I (*e) (_FUNCARGLIST), Transfer, const std::string &doc)
{
  return new _NAME(StaticMethodBiIter) <I _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc);
}

template <class I _COMMA _TMPLARG>
Methods
iterator (const std::string &name, I (*b) (_FUNCARGLIST), I (*e) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, I (*b) (_FUNCARGLIST), I (*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer transfer, I (*b) (_FUNCARGLIST), I (*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, transfer, doc)->add_args (_ARGSPECARGS));
}

template <class X, class I _COMMA _TMPLARG>
Methods
iterator (const std::string &name, I (X::*b) (_FUNCARGLIST), I (X::*e) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, I (X::*b) (_FUNCARGLIST), I (X::*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer transfer, I (X::*b) (_FUNCARGLIST), I (X::*e) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, transfer, doc)->add_args (_ARGSPECARGS));
}

template <class X, class I _COMMA _TMPLARG>
Methods
iterator_ext (const std::string &name, I (*xb) (X * _COMMA _FUNCARGLIST), I (*xe) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (_iterator_ext (name, xb, xe, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator_ext (const std::string &name, I (*xb) (X * _COMMA _FUNCARGLIST), I (*xe) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator_ext (name, xb, xe, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator_ext (const std::string &name, Transfer transfer, I (*xb) (X * _COMMA _FUNCARGLIST), I (*xe) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator_ext (name, xb, xe, transfer, doc)->add_args (_ARGSPECARGS));
}

template <class X, class I _COMMA _TMPLARG, class Transfer>
_NAME(ConstMethodBiIter) <X, I _COMMA _FUNCARGLIST, Transfer> *
_iterator (const std::string &name, I (X::*b) (_FUNCARGLIST) const, I (X::*e) (_FUNCARGLIST) const, Transfer, const std::string &doc)
{
  return new _NAME(ConstMethodBiIter) <X, I _COMMA _FUNCARGLIST, Transfer> (name, b, e, doc);
}

template <class X, class I _COMMA _TMPLARG>
Methods
iterator (const std::string &name, I (X::*b) (_FUNCARGLIST) const, I (X::*e) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, I (X::*b) (_FUNCARGLIST) const, I (X::*e) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer transfer, I (X::*b) (_FUNCARGLIST) const, I (X::*e) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, b, e, transfer, doc)->add_args (_ARGSPECARGS));
}

//  free iterators

template <class X, class I _COMMA _TMPLARG, class Transfer>
_NAME(MethodFreeIter) <X, I _COMMA _FUNCARGLIST, Transfer> *
_iterator (const std::string &name, I (X::*i) (_FUNCARGLIST), Transfer, const std::string &doc)
{
  return new _NAME(MethodFreeIter) <X, I _COMMA _FUNCARGLIST, Transfer> (name, i, doc);
}

template <class X, class I _COMMA _TMPLARG, class Transfer>
_NAME(ExtMethodFreeIter) <X, I _COMMA _FUNCARGLIST, Transfer> *
_iterator_ext (const std::string &name, I (*xi) (X * _COMMA _FUNCARGLIST), Transfer, const std::string &doc)
{
  return new _NAME(ExtMethodFreeIter) <X, I _COMMA _FUNCARGLIST, Transfer> (name, xi, doc);
}

template <class I _COMMA _TMPLARG, class Transfer>
_NAME(StaticMethodFreeIter) <I _COMMA _FUNCARGLIST, Transfer> *
_iterator (const std::string &name, I (*i) (_FUNCARGLIST), Transfer, const std::string &doc)
{
  return new _NAME(StaticMethodFreeIter) <I _COMMA _FUNCARGLIST, Transfer> (name, i, doc);
}

template <class I _COMMA _TMPLARG>
Methods
iterator (const std::string &name, I (*i) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, I (*i) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer transfer, I (*i) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, transfer, doc)->add_args (_ARGSPECARGS));
}

template <class X, class I _COMMA _TMPLARG>
Methods
iterator (const std::string &name, I (X::*i) (_FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, I (X::*i) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer transfer, I (X::*i) (_FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, transfer, doc)->add_args (_ARGSPECARGS));
}

template <class X, class I _COMMA _TMPLARG>
Methods
iterator_ext (const std::string &name, I (*xi) (X * _COMMA _FUNCARGLIST), const std::string &doc = std::string ())
{
  return Methods (_iterator_ext (name, xi, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator_ext (const std::string &name, I (*xi) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator_ext (name, xi, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator_ext (const std::string &name, Transfer transfer, I (*xi) (X * _COMMA _FUNCARGLIST) _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator_ext (name, xi, transfer, doc)->add_args (_ARGSPECARGS));
}

template <class X, class I _COMMA _TMPLARG, class Transfer>
_NAME(ConstMethodFreeIter) <X, I _COMMA _FUNCARGLIST, Transfer> *
_iterator (const std::string &name, I (X::*i) (_FUNCARGLIST) const, Transfer, const std::string &doc)
{
  return new _NAME(ConstMethodFreeIter) <X, I _COMMA _FUNCARGLIST, Transfer> (name, i, doc);
}

template <class X, class I _COMMA _TMPLARG>
Methods
iterator (const std::string &name, I (X::*i) (_FUNCARGLIST) const, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, arg_default_return_value_preference (), doc));
}

#if _COUNT != 0
template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS>
Methods
iterator (const std::string &name, I (X::*i) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, arg_default_return_value_preference (), doc)->add_args (_ARGSPECARGS));
}
#endif

template <class X, class I _COMMA _TMPLARG _COMMA _TMPLARGSPECS, class Transfer>
Methods
iterator (const std::string &name, Transfer transfer, I (X::*i) (_FUNCARGLIST) const _COMMA _ARGSPECS, const std::string &doc = std::string ())
{
  return Methods (_iterator (name, i, transfer, doc)->add_args (_ARGSPECARGS));
}

#undef _COMMA

