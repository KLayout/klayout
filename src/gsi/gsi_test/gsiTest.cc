
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "gsiTest.h"
#include "gsiTestForceLink.h"
#include "gsiDecl.h"
#include "gsiEnums.h"
#include "gsiSignals.h"

#include <iostream>

namespace gsi_test
{

/**
 *  @brief For the forceLink implementation
 */
int _force_link_f ()
{
  return 0;
}

// ----------------------------------------------------------------
//  Implementation of A

static std::unique_ptr<A> a_inst;
static int a_count = 0;

void A::br () 
{
  std::cout << "YOUR CHANCE TO SET A BREAKPOINT HERE" << std::endl;
}

A::A () {
  ++a_count;
  e = Enum (0);
  n = 17; 
  f = false;
}

A::A (int nn) {
  ++a_count;
  e = Enum (0);
  n = nn; 
  f = false;
}

A::A (int n1, int n2) {
  ++a_count;
  e = Enum (0);
  n = n1 + n2;
  f = false;
}

A::A (int n1, int n2, double n3) {
  ++a_count;
  e = Enum (0);
  n = (n1 + n2) * n3;
  f = false;
}

A::A (const A &a)
  : gsi::ObjectBase (a)
{
  ++a_count;
  operator= (a);
}

A &A::operator= (const A &a)
{
  if (this != &a) {
    e = a.e;
    m_d = a.m_d;
    n = a.n;
    f = a.f;
  }
  return *this;
}

int A::instance_count ()
{
  return a_count;
}

std::string A::to_s () const
{ 
  return tl::sprintf("A: %d", n); 
}

A::~A ()
{
  //  This allows destruction from outside the auto_ptr
  if (a_inst.get () == this) {
    a_inst.release ();
  }
  --a_count;
}

const char *A::a_static () 
{ 
  return "static_a"; 
}

tl::Variant A::new_a_by_variant ()
{
  return tl::Variant (A ());
}

#if defined(HAVE_QT)

std::vector<int>
A::qba_cref_to_ia (const QByteArray &ba)
{
  const char *cp = ba.constData ();
  size_t n = ba.size ();
  std::vector<int> ia;
  for (size_t i = 0; i < n; ++i) {
    ia.push_back (int (*cp++));
  }
  return ia;
}

QByteArray
A::ia_cref_to_qba (const std::vector<int> &ia)
{
  QByteArray ba;
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    ba.push_back (char (*i));
  }
  return ba;
}

QByteArray &
A::ia_cref_to_qba_ref (const std::vector<int> &ia)
{
  static QByteArray ba;
  ba.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    ba.push_back (char (*i));
  }
  return ba;
}

#if QT_VERSION >= 0x60000

std::vector<int>
A::qbav_cref_to_ia (const QByteArrayView &ba)
{
  const char *cp = ba.constData ();
  size_t n = ba.size ();
  std::vector<int> ia;
  for (size_t i = 0; i < n; ++i) {
    ia.push_back (int (*cp++));
  }
  return ia;
}

QByteArrayView
A::ia_cref_to_qbav (const std::vector<int> &ia)
{
  static QByteArray ba;
  ba.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    ba.push_back (char (*i));
  }
  return ba;
}

QByteArrayView &
A::ia_cref_to_qbav_ref (const std::vector<int> &ia)
{
  static QByteArray ba;
  ba.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    ba.push_back (char (*i));
  }
  static QByteArrayView bav;
  bav = ba;
  return bav;
}

#endif

std::vector<int>
A::qs_cref_to_ia (const QString &qs)
{
  const QChar *cp = qs.constData ();
  size_t n = qs.size ();
  std::vector<int> ia;
  for (size_t i = 0; i < n; ++i) {
    ia.push_back ((*cp++).unicode ());
  }
  return ia;
}

QString
A::ia_cref_to_qs (const std::vector<int> &ia)
{
  QString s;
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    s.push_back (QChar (*i));
  }
  return s;
}

QString &
A::ia_cref_to_qs_ref (const std::vector<int> &ia)
{
  static QString s;
  s.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    s.push_back (QChar (*i));
  }
  return s;
}

#if QT_VERSION >= 0x50000

std::vector<int>
A::ql1s_cref_to_ia (const QLatin1String &ql1s)
{
  std::vector<int> ia;
  const char *cp = ql1s.data ();
  size_t n = ql1s.size ();
  for (size_t i = 0; i < n; ++i) {
    ia.push_back ((unsigned char) *cp++);
  }
  return ia;
}

QLatin1String
A::ia_cref_to_ql1s (const std::vector<int> &ia)
{
  static std::string str;
  str.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    str += char (*i);
  }
  return QLatin1String (str.c_str (), str.size ());
}

QLatin1String &
A::ia_cref_to_ql1s_ref (const std::vector<int> &ia)
{
  static std::string str;
  str.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    str += char (*i);
  }
  static QLatin1String s (0);
  s = QLatin1String (str.c_str (), str.size ());
  return s;
}

#endif

#if QT_VERSION >= 0x60000

std::vector<int>
A::qsv_cref_to_ia (const QStringView &qs)
{
  const QChar *cp = qs.constData ();
  size_t n = qs.size ();
  std::vector<int> ia;
  for (size_t i = 0; i < n; ++i) {
    ia.push_back ((*cp++).unicode ());
  }
  return ia;
}

QStringView
A::ia_cref_to_qsv (const std::vector<int> &ia)
{
  static QString s;
  s.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    s.push_back (char (*i));
  }
  return s;
}

QStringView &
A::ia_cref_to_qsv_ref (const std::vector<int> &ia)
{
  static QString s;
  s.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    s.push_back (char (*i));
  }
  static QStringView sv;
  sv = s;
  return sv;
}

#endif

#endif

std::vector<int>
A::ba_cref_to_ia (const std::vector<char> &ba)
{
  std::vector<int> ia;
  for (std::vector<char>::const_iterator i = ba.begin (); i != ba.end (); ++i) {
    ia.push_back (int (*i));
  }
  return ia;
}

std::vector<char>
A::ia_cref_to_ba (const std::vector<int> &ia)
{
  std::vector<char> ba;
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    ba.push_back (char (*i));
  }
  return ba;
}

std::vector<char> &
A::ia_cref_to_ba_ref (const std::vector<int> &ia)
{
  static std::vector<char> ba;
  ba.clear ();
  for (std::vector<int>::const_iterator i = ia.begin (); i != ia.end (); ++i) {
    ba.push_back (char (*i));
  }
  return ba;
}


static A *a_ctor (int i)
{
  return new A (i);
}

static A *a_ctor2 (int i, int j)
{
  return new A (i, j);
}

static A *a_ctor3 (int i, int j, double f)
{
  return new A (i, j, f);
}

void A::a20 (A *ptr)
{ 
  if (a_inst.get () != ptr) {
    a_inst.reset (ptr);
  }
}

A *A::a20_get () 
{ 
  return a_inst.get (); 
}

static int s_sp = 0;

int A::sp_i_get ()
{
  return s_sp;
}

void A::sp_i_set (int v)
{
  s_sp = v + 1;
}

// ----------------------------------------------------------------
//  Implementation of B

B *B::b_inst = 0;
static int b_count = 0;

B::B () 
{ 
  ++b_count;
  m_av.push_back (A (100));
  m_av.push_back (A (121));
  m_av.push_back (A (144));
  m_avc_nc.push_back (new A_NC (-3100));
  m_avc_nc.push_back (new A_NC (-3121));
  m_av_nc.push_back (new A_NC (7100));
  m_av_nc.push_back (new A_NC (7121));
  m_av_nc.push_back (new A_NC (7144));
  m_av_nc.push_back (new A_NC (7169));
}

B::~B ()
{
  while (! m_av_nc.empty ()) {
    delete m_av_nc.back ();
    m_av_nc.pop_back ();
  }
  while (! m_avc_nc.empty ()) {
    delete const_cast<A_NC *> (m_avc_nc.back ());
    m_avc_nc.pop_back ();
  }
  if (b_inst == this) {
    b_inst = 0;
  }
  --b_count;
}

B::B (const B &d)
{
  operator=(d);
  ++b_count;
}

int B::instance_count ()
{
  return b_count;
}

B &B::operator=(const B &d)
{
  if (&d == this) {
    return *this;
  }

  m = d.m;
  m_a = d.m_a;
  m_bv = d.m_bv;
  m_av = d.m_av;
  while (! m_av_nc.empty ()) {
    delete m_av_nc.back ();
    m_av_nc.pop_back ();
  }
  for (std::vector <A_NC *>::const_iterator i = d.m_av_nc.begin (); i != d.m_av_nc.end (); ++i) {
    m_av_nc.push_back (new A_NC (**i));
  }
  while (! m_avc_nc.empty ()) {
    delete const_cast<A_NC *> (m_avc_nc.back ());
    m_avc_nc.pop_back ();
  }
  for (std::vector <const A_NC *>::const_iterator i = d.m_avc_nc.begin (); i != d.m_avc_nc.end (); ++i) {
    m_avc_nc.push_back (new A_NC (**i));
  }
  m_var = d.m_var;
  m_vars = d.m_vars;
  m_map2 = d.m_map2;

  return *this;
}

void B::set_inst (B *b) 
{ 
  b_inst = b; 
}

void B::del_inst () 
{ 
  delete b_inst; 
}

B *B::inst () 
{ 
  return b_inst; 
}

bool B::has_inst () 
{ 
  return b_inst != 0; 
}

tl::Variant B::new_b_by_variant ()
{
  return tl::Variant (B ());
}

std::string B::addr () const 
{
  char c[50];
  sprintf (c, "(%p)", (void *)this);
  return c;
}

static int aptr_to_n_ext (const B *b, A *aptr)
{ 
  return b->b3 (aptr);
}

static std::vector <A>::const_iterator b10b_ext (const B *b) 
{ 
  return b->b10b (); 
}

static std::vector <A>::const_iterator b10e_ext (const B *b) 
{ 
  return b->b10e (); 
}

static const A *b10bp_ext (const B *b) 
{ 
  if (b->b10b () == b->b10e ()) {
      return 0;
  } else {
    return b->b10b ().operator-> ();
  }
}

static const A *b10ep_ext (const B *b) 
{ 
  //  The way this code is written there are no assertions from MSVC's
  //  iterator debug mode:
  return b10bp_ext (b) + (b->b10e () - b->b10b ());
}

// ----------------------------------------------------------------
//  Implementation of C

int C::s1 () 
{ 
  return 4451; 
}

std::vector<int>::const_iterator C::s1a () 
{ 
  return m_v.begin (); 
}

std::vector<int>::const_iterator C::s1b () 
{ 
  return m_v.end (); 
}

void C::s2 (double x) 
{ 
  for (int i = 0; i < int (x + 0.5); ++i) {
    m_v.push_back (i);
  }
}

void C::s2clr () 
{ 
  m_v.clear(); 
}

std::string C::s3 (double x) 
{ 
  return tl::sprintf ("%.3f", x); 
}

std::vector<int> C::m_v;

// ----------------------------------------------------------------
//  Implementation of E

std::unique_ptr<E> E::e_inst;
int E::e_count = 0;

E::E() : x(0)
{
  e_count++;
}

E::~E()
{
  --e_count;
}

int E::inst_count()
{
  return e_count;
}

const E &E::icref() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! e_inst.get ()) { 
    e_inst.reset (new E ()); 
  } 
  return *e_inst.get (); 
}

E &E::incref() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! e_inst.get ()) { 
    e_inst.reset (new E ()); 
  }
  return *e_inst.get (); 
}

const E *E::ic() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! e_inst.get ()) { 
    e_inst.reset (new E ()); 
  } 
  return e_inst.get (); 
}

E *E::inc() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! e_inst.get ()) { 
    e_inst.reset (new E ()); 
  }
  return e_inst.get (); 
}

void E::reset_inst ()
{
  e_inst.reset (0);
}

int inst_count();
// ----------------------------------------------------------------
//  Implementation of F

std::unique_ptr<F> F::f_inst;

const F &F::icref() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! f_inst.get ()) { 
    f_inst.reset (new F ()); 
  } 
  return *f_inst.get (); 
}

F &F::incref() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! f_inst.get ()) { 
    f_inst.reset (new F ()); 
  }
  return *f_inst.get (); 
}

const F *F::ic() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! f_inst.get ()) { 
    f_inst.reset (new F ()); 
  } 
  return f_inst.get (); 
}

F *F::inc() 
{ 
  //  late initialisation is required because otherwise no binding happens
  if (! f_inst.get ()) { 
    f_inst.reset (new F ()); 
  }
  return f_inst.get (); 
}

// ----------------------------------------------------------------
//  Implementation of X

std::unique_ptr<X> X::sp_a (new X ("X::a"));
std::unique_ptr<X> X::sp_b (new X ("X::b"));

static X *make_x (const char *x) 
{ 
  return new X(x); 
}

static int s_xinst = 0;
    
X::X () 
{ 
  ++s_xinst;
}

X::X (const char *x) 
  : m_s (x)
{
  ++s_xinst;
}

X::X (const X &x)
  : gsi::ObjectBase ()
{
  *this = x;
  ++s_xinst;
}

X &X::operator= (const X &x)
{
  gsi::ObjectBase::operator= (x);
  if (this != &x) {
    m_s = x.m_s;
  }
  return *this;
}

X::~X ()
{ 
  --s_xinst;
}

int X::instances ()
{
  return s_xinst;
}

void X::init ()
{
  sp_a.reset (new X ("X::a"));
  sp_b.reset (new X ("X::b"));
}

const X *X::x_cptr () 
{
  return sp_a.get ();
}
  
X *X::x_ptr() 
{
  return sp_a.get ();
}
  
std::vector<X> X::vx () 
{
  std::vector<X> r;
  r.push_back(*sp_a);
  r.push_back(*sp_b);
  return r;
}
  
std::vector<const X *> X::vx_cptr () 
{
  std::vector<const X *> r;
  r.push_back(sp_a.get ());
  r.push_back(sp_b.get ());
  return r;
}
  
std::vector<X *> X::vx_ptr () 
{
  std::vector<X *> r;
  r.push_back(sp_a.get ());
  r.push_back(sp_b.get ());
  return r;
}

std::string X::cls_name () const
{ 
  return "X";
}

std::string X::s () const
{ 
  return m_s; 
}

void X::set_s (const std::string &s)
{ 
  m_s = s; 
}

void X::set_si (int v) 
{ 
  m_s = tl::to_string (v + 1);
}

// ----------------------------------------------------------------
//  Implementation of Y

std::unique_ptr<Y> Y::sp_a (new Y ("Y::a"));
std::unique_ptr<Y> Y::sp_b (new Y ("Y::b"));
int Y::s_dyn_count = 0;

static Y *make_y (const char *x)
{ 
  return new Y (x);
}

Y::Y ()
  : mp_c(0)
{ 
  ++s_dyn_count;
}

Y::Y (const char *x)
  : X(x), mp_c(0)
{ 
  ++s_dyn_count;
}

Y::~Y () 
{ 
  --s_dyn_count;
}

void Y::init()
{
  sp_a.reset (new Y ("Y::a"));
  sp_b.reset (new Y ("Y::b"));
}

const X *Y::y_cptr() 
{
  return sp_a.get ();
}
  
X *Y::y_ptr() 
{
  return sp_a.get ();
}
  
int Y::vx_dyn_count()
{
  return s_dyn_count;
}

void Y::vx_dyn_make()
{
  mp_c = new Y();
}

void Y::vx_dyn_destroy()
{
  delete mp_c;
  mp_c = 0;
}

std::vector<X *> Y::vx_dyn() 
{
  std::vector<X *> r;
  r.push_back(mp_c);
  return r;
}
  
std::vector<const X *> Y::vyasx_cptr() 
{
  std::vector<const X *> r;
  r.push_back (sp_a.get ());
  r.push_back (sp_b.get ());
  return r;
}
  
std::vector<X *> Y::vyasx_ptr() 
{
  std::vector<X *> r;
  r.push_back (sp_a.get ());
  r.push_back (sp_b.get ());
  return r;
}
  
std::vector<const Y *> Y::vy_cptr() 
{
  std::vector<const Y *> r;
  r.push_back (sp_a.get ());
  r.push_back (sp_b.get ());
  return r;
}
  
std::vector<Y *> Y::vy0_ptr() 
{
  std::vector<Y *> r;
  r.push_back (0);
  return r;
}

std::vector<Y *> Y::vy_ptr() 
{
  std::vector<Y *> r;
  r.push_back (sp_a.get ());
  r.push_back (sp_b.get ());
  return r;
}

std::string Y::cls_name() const
{ 
  return "Y";
}

int Y::i () const 
{ 
  return (int) m_s.size(); 
}

// ----------------------------------------------------------------
//  Implementation of YY

YY::YY () 
  : Y() 
{ 
  //  .. nothing yet ..
}

YY::YY (const char *x) 
  : Y(x) 
{ 
  //  .. nothing yet ..
}

std::string YY::cls_name() const
{ 
  return "YY";
}

// ----------------------------------------------------------------
//  Implementation of Z

Z::Z ()
  : mp_x (0)
{
  //  .. nothing yet ..
}

Z::~Z () 
{ 
  //  .. nothing yet ..
}

std::string Z::f (X *x) 
{ 
  return x ? tl::to_string (x->s ()) : "(nil)"; 
}

void Z::set_x (X *x)
{
  mp_x = x;
}

X *Z::x ()
{
  return mp_x;
}

void Z::set_x_keep (X *x)
{
  if (mp_x != x) {
    if (mp_x) {
      mp_x->release ();
    }
    mp_x = x;
    if (mp_x) {
      mp_x->keep ();
    }
  }
}

// ----------------------------------------------------------------
//  Implementation of Z_P

std::string Z_P::f (X *x)
{
  return f_cb.issue<Z, std::string, X *> (&Z::f, x);
}

std::string Z_P::f_org (X *x)
{
  return Z::f (x);
}

std::string Z_P::f_with_x (const std::string &s)
{
  X x(s.c_str());
  return f (&x);
}

std::string Z_P::f_with_y (const std::string &s)
{
  Y y(s.c_str());
  return f (&y);
}

std::string Z_P::f_with_yy (const std::string &s)
{
  YY yy(s.c_str());
  return f (&yy);
}

// ----------------------------------------------------------------
//  Implementation of SQ

#if defined(HAVE_QT)

SQ::SQ ()
  : m_tag (0)
{
  //  .. nothing yet ..
}

void SQ::set_tag (int t)
{
  m_tag = t;
}

void SQ::trigger_s0 ()
{
  emit s0 ();
}

void SQ::trigger_s1 (int x)
{
  emit s1 (x);
}

void SQ::trigger_s2 (const QString &s)
{
  emit s2 (s, this);
}

#endif

// ----------------------------------------------------------------
//  Implementation of SE

SE::SE ()
  : m_tag (0)
{
  //  .. nothing yet ..
}

void SE::set_tag (int t)
{
  m_tag = t;
}

void SE::trigger_s0 ()
{
  s0 ();
}

void SE::trigger_s1 (int x)
{
  s1 (x);
}

void SE::trigger_s2 (const std::string &s)
{
  s2 (s, this);
}

// ----------------------------------------------------------------
//  GSI declarations

static gsi::Enum<Enum> decl_enum ("", "Enum",
  gsi::enum_const ("a", Enum_a) +
  gsi::enum_const ("b", Enum_b) +
  gsi::enum_const ("c", Enum_c) 
);

#if defined(HAVE_QT)
static gsi::QFlagsClass<Enum> decl_qflags_enum ("", "Enums");
#endif

static gsi::Class<A> decl_a ("", "A",
  gsi::constructor ("new_a|new", &a_ctor) +
  gsi::constructor ("new", &a_ctor2) +
  gsi::constructor ("new", &a_ctor3) +
  gsi::method ("instance_count", &A::instance_count) +
  gsi::method ("new_a_by_variant", &A::new_a_by_variant) +

#if defined(HAVE_QT)

  gsi::method ("qba_cref_to_ia", &A::qba_cref_to_ia) +
  gsi::method ("qba_ref_to_ia", &A::qba_ref_to_ia) +
  gsi::method ("qba_cptr_to_ia", &A::qba_cptr_to_ia) +
  gsi::method ("qba_ptr_to_ia", &A::qba_ptr_to_ia) +
  gsi::method ("qba_to_ia", &A::qba_to_ia) +
  gsi::method ("ia_cref_to_qba", &A::ia_cref_to_qba) +
  gsi::method ("ia_cref_to_qba_cref", &A::ia_cref_to_qba_cref) +
  gsi::method ("ia_cref_to_qba_ref", &A::ia_cref_to_qba_ref) +
  gsi::method ("ia_cref_to_qba_cptr", &A::ia_cref_to_qba_cptr) +
  gsi::method ("ia_cref_to_qba_ptr", &A::ia_cref_to_qba_ptr) +

  gsi::method ("qs_cref_to_ia", &A::qs_cref_to_ia) +
  gsi::method ("qs_ref_to_ia", &A::qs_ref_to_ia) +
  gsi::method ("qs_cptr_to_ia", &A::qs_cptr_to_ia) +
  gsi::method ("qs_ptr_to_ia", &A::qs_ptr_to_ia) +
  gsi::method ("qs_to_ia", &A::qs_to_ia) +
  gsi::method ("ia_cref_to_qs", &A::ia_cref_to_qs) +
  gsi::method ("ia_cref_to_qs_cref", &A::ia_cref_to_qs_cref) +
  gsi::method ("ia_cref_to_qs_ref", &A::ia_cref_to_qs_ref) +
  gsi::method ("ia_cref_to_qs_cptr", &A::ia_cref_to_qs_cptr) +
  gsi::method ("ia_cref_to_qs_ptr", &A::ia_cref_to_qs_ptr) +

#if QT_VERSION >= 0x50000

  gsi::method ("ql1s_cref_to_ia", &A::ql1s_cref_to_ia) +
  gsi::method ("ql1s_ref_to_ia", &A::ql1s_ref_to_ia) +
  gsi::method ("ql1s_cptr_to_ia", &A::ql1s_cptr_to_ia) +
  gsi::method ("ql1s_ptr_to_ia", &A::ql1s_ptr_to_ia) +
  gsi::method ("ql1s_to_ia", &A::ql1s_to_ia) +
  gsi::method ("ia_cref_to_ql1s", &A::ia_cref_to_ql1s) +
  gsi::method ("ia_cref_to_ql1s_cref", &A::ia_cref_to_ql1s_cref) +
  gsi::method ("ia_cref_to_ql1s_ref", &A::ia_cref_to_ql1s_ref) +
  gsi::method ("ia_cref_to_ql1s_cptr", &A::ia_cref_to_ql1s_cptr) +
  gsi::method ("ia_cref_to_ql1s_ptr", &A::ia_cref_to_ql1s_ptr) +

#endif

#if QT_VERSION >= 0x60000

  gsi::method ("qbav_cref_to_ia", &A::qbav_cref_to_ia) +
  gsi::method ("qbav_ref_to_ia", &A::qbav_ref_to_ia) +
  gsi::method ("qbav_cptr_to_ia", &A::qbav_cptr_to_ia) +
  gsi::method ("qbav_ptr_to_ia", &A::qbav_ptr_to_ia) +
  gsi::method ("qbav_to_ia", &A::qbav_to_ia) +
  gsi::method ("ia_cref_to_qbav", &A::ia_cref_to_qbav) +
  gsi::method ("ia_cref_to_qbav_cref", &A::ia_cref_to_qbav_cref) +
  gsi::method ("ia_cref_to_qbav_ref", &A::ia_cref_to_qbav_ref) +
  gsi::method ("ia_cref_to_qbav_cptr", &A::ia_cref_to_qbav_cptr) +
  gsi::method ("ia_cref_to_qbav_ptr", &A::ia_cref_to_qbav_ptr) +

  gsi::method ("qsv_cref_to_ia", &A::qsv_cref_to_ia) +
  gsi::method ("qsv_ref_to_ia", &A::qsv_ref_to_ia) +
  gsi::method ("qsv_cptr_to_ia", &A::qsv_cptr_to_ia) +
  gsi::method ("qsv_ptr_to_ia", &A::qsv_ptr_to_ia) +
  gsi::method ("qsv_to_ia", &A::qsv_to_ia) +
  gsi::method ("ia_cref_to_qsv", &A::ia_cref_to_qsv) +
  gsi::method ("ia_cref_to_qsv_cref", &A::ia_cref_to_qsv_cref) +
  gsi::method ("ia_cref_to_qsv_ref", &A::ia_cref_to_qsv_ref) +
  gsi::method ("ia_cref_to_qsv_cptr", &A::ia_cref_to_qsv_cptr) +
  gsi::method ("ia_cref_to_qsv_ptr", &A::ia_cref_to_qsv_ptr) +

#endif

#endif

  gsi::method ("ba_cref_to_ia", &A::ba_cref_to_ia) +
  gsi::method ("ba_ref_to_ia", &A::ba_ref_to_ia) +
  gsi::method ("ba_cptr_to_ia", &A::ba_cptr_to_ia) +
  gsi::method ("ba_ptr_to_ia", &A::ba_ptr_to_ia) +
  gsi::method ("ba_to_ia", &A::ba_to_ia) +

  gsi::method ("ia_cref_to_ba", &A::ia_cref_to_ba) +
  gsi::method ("ia_cref_to_ba_ref", &A::ia_cref_to_ba_ref) +
  gsi::method ("ia_cref_to_ba_cref", &A::ia_cref_to_ba_cref) +
  gsi::method ("ia_cref_to_ba_ptr", &A::ia_cref_to_ba_ptr) +
  gsi::method ("ia_cref_to_ba_cptr", &A::ia_cref_to_ba_cptr) +

  gsi::method ("br", &A::br) +
  gsi::method ("get_e", &A::get_e) +
  gsi::method ("get_eptr", &A::get_eptr) +
  gsi::method ("get_ecptr", &A::get_ecptr) +
  gsi::method ("get_eref", &A::get_eref) +
  gsi::method ("get_ecref", &A::get_ecref) +
  gsi::method ("set_e", &A::set_e) +
  gsi::method ("set_eptr", &A::set_eptr) +
  gsi::method ("set_ecptr", &A::set_ecptr) +
  gsi::method ("set_eref", &A::set_eref) +
  gsi::method ("set_ecref", &A::set_ecref) +
  gsi::method ("mod_eptr", &A::mod_eptr) +
  gsi::method ("mod_eref", &A::mod_eref) +
#if defined(HAVE_QT)
  gsi::method ("get_ef", &A::get_ef) +
  gsi::method ("get_efptr", &A::get_efptr) +
  gsi::method ("get_efcptr", &A::get_efcptr) +
  gsi::method ("get_efref", &A::get_efref) +
  gsi::method ("get_efcref", &A::get_efcref) +
  gsi::method ("set_ef", &A::set_ef) +
  gsi::method ("set_efptr", &A::set_efptr) +
  gsi::method ("set_efcptr", &A::set_efcptr) +
  gsi::method ("set_efref", &A::set_efref) +
  gsi::method ("set_efcref", &A::set_efcref) +
  gsi::method ("mod_efptr", &A::mod_efptr) +
  gsi::method ("mod_efref", &A::mod_efref) +
#endif
  gsi::method ("push_ev", &A::push_ev) +
  gsi::method ("ev", &A::ev) +
  gsi::method ("af=", &A::set_af) +
  gsi::method ("af?|af", &A::af0) +
  gsi::method ("af?|af", &A::af1) +
  gsi::method ("aa", &A::a) +
  gsi::method ("aa", &A::a_static) +
  gsi::method ("a1|get_n", &A::a1) +
  gsi::method ("a1c|get_n_const", &A::a1c) +
  gsi::method ("a2", &A::a2) +
  gsi::method ("a3", &A::a3) +
#if defined(HAVE_QT)
  gsi::method ("a3_qba", &A::a3_qba) +
  gsi::method ("a3_qstr", &A::a3_qstr) +
  gsi::method ("a3_qstrref", &A::a3_qstrref) +
#endif
  gsi::method ("a4", &A::a4) +
  gsi::method ("a5|n=", &A::a5) +
  gsi::method ("a10_d", &A::a10_d) +
#if defined(HAVE_QT)
  gsi::method ("a10_d_qba", &A::a10_d_qba) +
  gsi::method ("a10_d_qstr", &A::a10_d_qstr) +
  gsi::method ("a10_d_qstrref", &A::a10_d_qstrref) +
#endif
  gsi::method ("*a10_prot", &A::a10_d) +
  gsi::method ("a10_f", &A::a10_f) +
  gsi::method ("a10_s", &A::a10_s) +
  gsi::method ("a10_us", &A::a10_us) +
  gsi::method ("a10_i", &A::a10_i) +
  gsi::method ("a10_l", &A::a10_l) +
  gsi::method ("a10_ll", &A::a10_ll) +
  gsi::method ("a10_ui", &A::a10_ui) +
  gsi::method ("a10_ul", &A::a10_ul) +
  gsi::method ("a10_ull", &A::a10_ull) +
  gsi::method ("a10_fptr", &A::a10_fptr) +
  gsi::method ("a10_dptr", &A::a10_dptr) +
  gsi::method ("a10_iptr", &A::a10_iptr) +
  gsi::method ("a10_bptr", &A::a10_bptr) +
  gsi::method ("a10_uiptr", &A::a10_uiptr) +
  gsi::method ("a10_ulptr", &A::a10_ulptr) +
  gsi::method ("a10_lptr", &A::a10_lptr) +
  gsi::method ("a10_llptr", &A::a10_llptr) +
  gsi::method ("a10_ullptr", &A::a10_ullptr) +
  gsi::method ("a10_cfptr", &A::a10_cfptr) +
  gsi::method ("a10_cdptr", &A::a10_cdptr) +
  gsi::method ("a10_ciptr", &A::a10_ciptr) +
  gsi::method ("a10_cbptr", &A::a10_cbptr) +
  gsi::method ("a10_cuiptr", &A::a10_cuiptr) +
  gsi::method ("a10_culptr", &A::a10_culptr) +
  gsi::method ("a10_clptr", &A::a10_clptr) +
  gsi::method ("a10_cllptr", &A::a10_cllptr) +
  gsi::method ("a10_cullptr", &A::a10_cullptr) +
  gsi::method ("a10_sptr", &A::a10_sptr) +
  gsi::method ("a10_csptr", &A::a10_csptr) +
  gsi::method ("a10_fref", &A::a10_fref) +
  gsi::method ("a10_dref", &A::a10_dref) +
  gsi::method ("a10_iref", &A::a10_iref) +
  gsi::method ("a10_bref", &A::a10_bref) +
  gsi::method ("a10_uiref", &A::a10_uiref) +
  gsi::method ("a10_ulref", &A::a10_ulref) +
  gsi::method ("a10_lref", &A::a10_lref) +
  gsi::method ("a10_llref", &A::a10_llref) +
  gsi::method ("a10_ullref", &A::a10_ullref) +
  gsi::method ("a10_sref", &A::a10_sref) +
  gsi::method ("a10_cfref", &A::a10_cfref) +
  gsi::method ("a10_cdref", &A::a10_cdref) +
  gsi::method ("a10_ciref", &A::a10_ciref) +
  gsi::method ("a10_cbref", &A::a10_cbref) +
  gsi::method ("a10_cuiref", &A::a10_cuiref) +
  gsi::method ("a10_culref", &A::a10_culref) +
  gsi::method ("a10_clref", &A::a10_clref) +
  gsi::method ("a10_cllref", &A::a10_cllref) +
  gsi::method ("a10_cullref", &A::a10_cullref) +
  gsi::method ("a10_csref", &A::a10_csref) +
  gsi::method ("a11_s", &A::a11_s) +
  gsi::method ("a11_us", &A::a11_us) +
  gsi::method ("a11_i", &A::a11_i) +
  gsi::method ("a11_l", &A::a11_l) +
  gsi::method ("a11_ll", &A::a11_ll) +
  gsi::method ("a11_ui", &A::a11_ui) +
  gsi::method ("a11_ul", &A::a11_ul) +
  gsi::method ("a11_ull", &A::a11_ull) +
  gsi::method ("a_vp1", &A::a_vp1) +
  gsi::method ("a_vp2", &A::a_vp2) +
  gsi::method ("a9a", &A::a9a) +
  gsi::method ("a9b", &A::a9b) +
  gsi::method ("a20", &A::a20) +
  gsi::method ("a20_get", &A::a20_get) +
  gsi::method ("sp_i", &A::sp_i_get) +
  gsi::method ("sp_i=", &A::sp_i_set) +
  gsi::method ("to_s", &A::to_s) +
  gsi::iterator ("a6", &A::a6b, &A::a6e) +
  gsi::iterator ("a7", &A::a7b, &A::a7e) +
  gsi::iterator ("a8", &A::a8b, &A::a8e) +
#if defined(HAVE_QT)
  gsi::method ("ft_qba", &A::ft_qba) +
  gsi::method ("ft_qs", &A::ft_qs) +
#endif
  gsi::method ("ft_str", &A::ft_str) +
  gsi::method ("ft_cv", &A::ft_cv) +
  gsi::method ("ft_cptr", &A::ft_cptr) +
  gsi::method ("ft_var", &A::ft_var)
);

static gsi::Class<A_NC> decl_a_nc (decl_a, "", "A_NC");

static gsi::Class<B> decl_b ("", "B",
#if __cplusplus >= 201703L
  gsi::method ("int_to_optional", &B::int_to_optional) +
  gsi::method ("int_to_optional_a", &B::int_to_optional_a) +
  gsi::method ("optional_to_int", &B::optional_to_int) +
  gsi::method ("optional_cref_to_int", &B::optional_cref_to_int) +
  gsi::method ("optional_ref_to_int", &B::optional_ref_to_int) +
  gsi::method ("optional_cptr_to_int", &B::optional_cptr_to_int) +
  gsi::method ("optional_ptr_to_int", &B::optional_ptr_to_int) +
  gsi::method ("optional_a_to_int", &B::optional_a_to_int) +
  gsi::method ("optional_a_cref_to_int", &B::optional_a_cref_to_int) +
  gsi::method ("optional_a_ref_to_int", &B::optional_a_ref_to_int) +
  gsi::method ("optional_a_cptr_to_int", &B::optional_a_cptr_to_int) +
  gsi::method ("optional_a_ptr_to_int", &B::optional_a_ptr_to_int) +
#endif
  gsi::method ("inst", &B::inst) +
  gsi::method ("has_inst", &B::has_inst) +
  gsi::method ("set_inst", &B::set_inst) +
  gsi::method ("del_inst", &B::del_inst) +
  gsi::method ("instance_count", &B::instance_count) +
  gsi::method ("new_b_by_variant", &B::new_b_by_variant) +
  gsi::method ("addr", &B::addr) +
  gsi::method ("always_5", &B::always_5) +
  gsi::method ("str", &B::str) +
  gsi::method ("set_str", &B::set_str) +
  gsi::method ("str_ccptr", &B::str_ccptr) +
  gsi::method ("set_str_combine", &B::set_str_combine) +
  gsi::method_ext ("b3|aptr_to_n", &aptr_to_n_ext) +
  gsi::method ("b4|aref_to_s", &B::aref_to_s) +
  gsi::method ("make_a", &B::make_a) +
  gsi::method ("set_an", &B::set_an) +
  gsi::method ("an", &B::an) +
  gsi::method ("set_an_cref", &B::set_an_cref) +
  gsi::method ("an_cref", &B::an_cref) +
  //  implemented by extension below:
  // gsi::iterator_ext ("b10", &b10b_ext, &b10e_ext) +
  gsi::iterator ("b10_nc|each_a_be_nc", &B::b10b_nc, &B::b10e_nc) +
  gsi::iterator ("b11|each_a_be_v", &B::b11b, &B::b11e) +
  gsi::iterator ("b12|each_a_be_p", &B::b12b, &B::b12e) +
  gsi::iterator ("b13|each_a_be_cp", &B::b13b, &B::b13e) +
  gsi::method ("amember_or_nil_alt|amember_or_nil", &B::amember_or_nil) +
  gsi::method ("amember_ptr_alt|amember_ptr", &B::amember_ptr) +
  gsi::method ("xxx|amember_cptr", &B::amember_cptr) +
  gsi::method ("yyy|amember_cref", &B::amember_cref) +
  gsi::method ("zzz|amember_ref", &B::amember_ref) +
  gsi::method ("b15|arg_is_not_nil", &B::arg_is_not_nil) +
  gsi::method ("b16a|av", &B::av) +
  gsi::method ("b16b|av_cref", &B::av_cref) +
  gsi::method ("b16c|av_ref", &B::av_ref) +
  gsi::method ("push_a", &B::push_a) +
  gsi::method ("push_a_cref", &B::push_a_cref) +
  gsi::method ("push_a_cptr", &B::push_a_cptr) +
  gsi::method ("push_a_ref", &B::push_a_ref) +
  gsi::method ("push_a_ptr", &B::push_a_ptr) +
  gsi::method ("b17a|av_cref=", &B::set_av_cref) +
  gsi::method ("b17b|av_ref=", &B::set_av_ref) +
  gsi::method ("b17c|av=", &B::set_av) +
  gsi::method ("b17d|av_cptr=", &B::set_av_cptr) +
  gsi::method ("b17e|av_ptr=", &B::set_av_ptr) +
  gsi::iterator ("b18|each_a", &B::b18) +
  gsi::iterator ("b18b|each_a_ref", &B::b18b) +
  gsi::iterator ("b18c|each_a_ptr", &B::b18c) +
  gsi::method ("b20a|var_is_nil", &B::b20a) +
  gsi::method ("b20b|var_is_double", &B::b20b) +
  gsi::method ("b20c|var_is_long", &B::b20c) +
  gsi::method ("b20d|var_is_string", &B::b20d) +
  gsi::method ("b20e|var_is_bool", &B::b20e) +
  gsi::method ("b21a|var_to_string", &B::b21a) +
  gsi::method ("b21b|var_to_double", &B::b21b) +
  gsi::method ("b21c|var_to_long", &B::b21c) +
  gsi::method ("b22a", &B::b22a) +
  gsi::method ("set_vars", &B::set_vars) +
  gsi::method ("b22b", &B::b22b) +
  gsi::method ("b22c", &B::b22c) +
  gsi::method ("b22d", &B::b22d) +
  gsi::method ("var", &B::var) +
  gsi::method ("var_cref", &B::var_cref) +
  gsi::method ("var_cptr", &B::var_cptr) +
  gsi::method ("var_ref", &B::var_ref) +
  gsi::method ("var_ptr", &B::var_ptr) +
  gsi::method ("b23a|vars", &B::b23a) +
  gsi::method ("b23b|vars_cref", &B::b23b) +
  gsi::method ("b23c|vars_ref", &B::b23c) +
  gsi::method ("b23d|vars_as_var", &B::b23d) +
  gsi::method ("b23e|vars_cptr", &B::b23e) +
  gsi::method ("b23e_null|vars_cptr_null", &B::b23e_null) +
  gsi::method ("b23f|vars_ptr", &B::b23f) +
  gsi::method ("b23f_null|vars_ptr_null", &B::b23f_null) +
  gsi::iterator ("b24|var_iter", &B::b24b, &B::b24e) +
  gsi::method ("#b30|bx|#always_17", &B::b30) +
  gsi::method ("#b31|bx|by|#always_xy_sig_i", &B::b31) +
  gsi::method ("bx|#b32|#always_20_5_sig_si", &B::b32) +
  gsi::method ("#b33|bx|always_aref_sig_a", &B::b33) +
  gsi::method ("b34|bx|always_arefi_sig_ai", &B::b34) +
  gsi::method ("insert_map1", &B::insert_map1) +
  gsi::method ("map1", &B::map1) +
  gsi::method ("map1_cref", &B::map1_cref) +
  gsi::method ("map1_ref", &B::map1_ref) +
  gsi::method ("map1_cptr", &B::map1_cptr) +
  gsi::method ("map1_cptr_null", &B::map1_cptr_null) +
  gsi::method ("map1_ptr", &B::map1_ptr) +
  gsi::method ("map1_ptr_null", &B::map1_ptr_null) +
  gsi::method ("map1=|set_map1_cref", &B::set_map1_cref) +
  gsi::method ("set_map1_ref", &B::set_map1_ref) +
  gsi::method ("set_map1_cptr", &B::set_map1_cptr) +
  gsi::method ("set_map1_ptr", &B::set_map1_ptr) +
  gsi::method ("set_map1", &B::set_map1) +
  gsi::method ("insert_map2", &B::insert_map2) +
  gsi::method ("map2", &B::map2) +
  gsi::method ("map2_null", &B::map2_null) +
  gsi::method ("map2=", &B::set_map2) +
  gsi::iterator ("each_b_copy", &B::each_b_copy) +
  gsi::iterator ("each_b_ref", &B::each_b_ref) +
  gsi::iterator ("each_b_ptr", &B::each_b_ptr) +
  gsi::iterator ("each_b_cref", &B::each_b_cref) +
  gsi::iterator ("each_b_cptr", &B::each_b_cptr) +
  gsi::method ("push_b", &B::push_b) +
  gsi::method ("map_iaptr", &B::map_iaptr) +
  gsi::method ("map_iaptr_cref", &B::map_iaptr_cref) +
  gsi::method ("map_iaptr_ref", &B::map_iaptr_ref) +
  gsi::method ("map_iaptr_cptr", &B::map_iaptr_cptr) +
  gsi::method ("map_iaptr_ptr", &B::map_iaptr_ptr) +
  gsi::method ("insert_map_iaptr", &B::insert_map_iaptr) +
  gsi::method ("set_map_iaptr", &B::set_map_iaptr) +
  gsi::method ("set_map_iaptr_cref", &B::set_map_iaptr_cref) +
  gsi::method ("set_map_iaptr_ref", &B::set_map_iaptr_ref) +
  gsi::method ("set_map_iaptr_cptr", &B::set_map_iaptr_cptr) +
  gsi::method ("set_map_iaptr_ptr", &B::set_map_iaptr_ptr) +
  gsi::method ("insert_map_iacptr", &B::insert_map_iacptr) +
  gsi::method ("map_iacptr", &B::map_iacptr) +
  gsi::method ("set_map_iacptr", &B::set_map_iacptr) +
  gsi::method ("insert_map_ia", &B::insert_map_ia) +
  gsi::method ("map_ia", &B::map_ia) +
  gsi::method ("set_map_ia", &B::set_map_ia) +
  gsi::method ("insert_map_iav", &B::insert_map_iav) +
  gsi::method ("push_map_iav", &B::push_map_iav) +
  gsi::method ("map_iav", &B::map_iav) +
  gsi::method ("set_map_iav", &B::set_map_iav) +
  gsi::method ("push_vvs", &B::push_vvs) +
  gsi::method ("vvs", &B::vvs) +
  gsi::method ("vvs_ref", &B::vvs_ref) +
  gsi::method ("vvs_ptr", &B::vvs_ptr) +
  gsi::method ("vvs_cref", &B::vvs_cref) +
  gsi::method ("vvs_cptr", &B::vvs_cptr) +
  gsi::method ("set_vvs", &B::set_vvs) +
  gsi::method ("set_vvs_ref", &B::set_vvs_ref) +
  gsi::method ("set_vvs_cref", &B::set_vvs_cref) +
  gsi::method ("set_vvs_cptr", &B::set_vvs_cptr) +
  gsi::method ("set_vvs_ptr", &B::set_vvs_ptr) +
  gsi::method ("push_ls", &B::push_ls) +
  gsi::method ("ls", &B::ls) +
  gsi::method ("set_ls", &B::set_ls) +
  gsi::method ("push_ss", &B::push_ss) +
  gsi::method ("ss", &B::ss) +
  gsi::method ("set_ss", &B::set_ss)
#if defined(HAVE_QT)
  +
  gsi::method ("push_qls", &B::push_qls) +
  gsi::method ("qls", &B::qls) +
  gsi::method ("set_qls", &B::set_qls) +
  gsi::method ("push_qlv", &B::push_qlv) +
  gsi::method ("qlv", &B::qlv) +
  gsi::method ("set_qlv", &B::set_qlv) +
  gsi::method ("push_qsl", &B::push_qsl) +
  gsi::method ("qsl", &B::qsl) +
  gsi::method ("set_qsl", &B::set_qsl) +
  gsi::method ("push_qvs", &B::push_qvs) +
  gsi::method ("qvs", &B::qvs) +
  gsi::method ("set_qvs", &B::set_qvs) +
  gsi::method ("push_qss", &B::push_qss) +
  gsi::method ("qss", &B::qss) +
  gsi::method ("set_qss", &B::set_qss) +
  gsi::method ("insert_qmap_is", &B::insert_qmap_is) +
  gsi::method ("qmap_is", &B::qmap_is) +
  gsi::method ("set_qmap_is", &B::set_qmap_is) +
  gsi::method ("insert_qhash_is", &B::insert_qhash_is) +
  gsi::method ("qhash_is", &B::qhash_is) +
  gsi::method ("set_qhash_is", &B::set_qhash_is)
#endif
);

//  extending B
static gsi::ClassExt<B> b_ext ( 
  gsi::iterator_ext ("b10|each_a_be", &b10b_ext, &b10e_ext) +
  gsi::iterator_ext ("b10p|each_a_be_pp", &b10bp_ext, &b10ep_ext)
);

CopyDetector *new_cd (int x)
{
  return new CopyDetector (x);
}

static gsi::Class<CopyDetector> decl_copy_detector ("", "CopyDetector",
  gsi::constructor ("new", &new_cd) +
  gsi::method ("x", &CopyDetector::x) +
  gsi::method ("xx", &CopyDetector::xx)
);

static gsi::Class<C_P> decl_c ("", "C",
  gsi::callback ("f", &C_P::f, &C_P::f_cb) +
  gsi::callback ("vfunc", &C_P::vfunc, &C_P::vfunc_cb) +
  gsi::method ("call_vfunc", &C_P::call_vfunc) +
  gsi::method ("pass_cd_direct", &C_P::pass_cd_direct) +
  gsi::method ("pass_cd_cref", &C_P::pass_cd_cref) +
  gsi::method ("pass_cd_cref_as_copy", gsi::return_copy (), &C_P::pass_cd_cref) +
  gsi::method ("pass_cd_cref_as_ref", gsi::return_reference (), &C_P::pass_cd_cref) +
  gsi::method ("pass_cd_cptr", &C_P::pass_cd_cptr) +
  gsi::method ("pass_cd_cptr_as_copy", gsi::return_copy (), &C_P::pass_cd_cptr) +
  gsi::method ("pass_cd_cptr_as_ref", gsi::return_reference (), &C_P::pass_cd_cptr) +
  gsi::method ("pass_cd_ref", &C_P::pass_cd_ref) +
  gsi::method ("pass_cd_ref_as_copy", gsi::return_copy (), &C_P::pass_cd_ref) +
  gsi::method ("pass_cd_ref_as_ref", gsi::return_reference (), &C_P::pass_cd_ref) +
  gsi::method ("pass_cd_ptr", &C_P::pass_cd_ptr) +
  gsi::method ("pass_cd_ptr_as_copy", gsi::return_copy (), &C_P::pass_cd_ptr) +
  gsi::method ("pass_cd_ptr_as_ref", gsi::return_reference (), &C_P::pass_cd_ptr) +
  gsi::method ("g", &C_P::g) +
  gsi::method ("s1", &C::s1) +
  gsi::method ("s2", &C::s2) +
  gsi::method ("s2clr", &C::s2clr) +
  gsi::method ("s3", &C::s3) +
  gsi::iterator ("each", &C::s1a, &C::s1b) 
);

tl::event<E *> &ev1_ext (E *e) { return e->ev1; }

static gsi::Class<E> decl_e ("", "E",
  gsi::event ("e0", &E::ev0) +
  gsi::event_ext ("e1", &ev1_ext) +
  gsi::event ("e2", &E::ev2) +
  // No events with return available currently
  // gsi::event ("e0r", &E::ev0r) +
  gsi::method ("s1", &E::s1) +
  gsi::method ("s2", &E::s2) +
  gsi::method ("s3", &E::s3) +
  // No events with return available currently
  // gsi::method ("s1r", &E::s1r) +
  gsi::method ("ic", &E::ic) +
  gsi::method ("inc", &E::inc) +
  gsi::method ("icref", &E::icref) +
  gsi::method ("incref", &E::incref) +
  gsi::method ("x=", &E::set_x) +
  gsi::method ("x", &E::get_x) +
  gsi::method ("bindme", &E::bindme) +
  gsi::method ("inst_count", &E::inst_count) +
  gsi::method ("reset_inst", &E::reset_inst)
);

static gsi::Class<F> decl_f ("", "F",
  gsi::method ("ic", &F::ic) +
  gsi::method ("inc", &F::inc) +
  gsi::method ("icref", &F::icref) +
  gsi::method ("incref", &F::incref) +
  gsi::method ("x=", &F::set_x) +
  gsi::method ("x", &F::get_x) 
);

static gsi::Class<G> decl_g ("", "G",
  gsi::method ("iv", &G::iv) +
  gsi::method ("sv", &G::sv) +
  gsi::method ("set_iva", &G::set_iv, gsi::arg ()) +
  gsi::method ("set_ivb", &G::set_iv, gsi::arg ("", 1)) +
  gsi::method ("set_sv1a", &G::set_sv1, gsi::arg ()) +
  gsi::method ("set_sv1b", &G::set_sv1, gsi::arg ("name", "value")) +
  gsi::method ("set_sv2a", &G::set_sv2, gsi::arg ()) +
  gsi::method ("set_sv2b", &G::set_sv2, gsi::arg ("", "value")) +
  gsi::method ("set_vva", &G::set_vv) + 
  gsi::method ("set_vvb", &G::set_vv, gsi::arg (), gsi::arg ("", "value")) +
  gsi::method ("set_vvc", &G::set_vv, gsi::arg ("", 1), gsi::arg ("", "value")) 
);

static gsi::Class<X> decl_x ("", "X",
  gsi::constructor ("new", &make_x) + 
  gsi::method ("instances", &X::instances) +
  gsi::method ("x1", &X::x1) +
  gsi::method ("x2", &X::x2) +
  gsi::method ("x_ptr", &X::x_ptr) +
  gsi::method ("x_cptr", &X::x_cptr) +
  gsi::method ("vx", &X::vx) +
  gsi::method ("vx_ptr", &X::vx_ptr) +
  gsi::method ("vx_cptr", &X::vx_cptr) +
  gsi::method ("cls_name", &X::cls_name) +
  gsi::method ("init", &X::init) +
  gsi::method ("s", &X::s) +
  gsi::method ("s=", &X::set_s) +
  gsi::method ("s=", &X::set_si)
);

static gsi::Class<Y> decl_y (decl_x, "", "Y",
  gsi::constructor ("new", &make_y) + 
  gsi::method ("x1", &Y::x1) +
  gsi::method ("y1", &Y::y1) +
  gsi::method ("y_ptr", &Y::y_ptr) +
  gsi::method ("y_cptr", &Y::y_cptr) +
  gsi::method ("vy_ptr", &Y::vy_ptr) +
  gsi::method ("vy0_ptr", &Y::vy0_ptr) +
  gsi::method ("vy_cptr", &Y::vy_cptr) +
  gsi::method ("vyasx_ptr", &Y::vyasx_ptr) +
  gsi::method ("vyasx_cptr", &Y::vyasx_cptr) +
  gsi::method ("init", &Y::init) +
  gsi::method ("i", &Y::i) +
  gsi::method ("vx_dyn_count", &Y::vx_dyn_count) +
  gsi::method ("vx_dyn_make", &Y::vx_dyn_make) +
  gsi::method ("vx_dyn_destroy", &Y::vx_dyn_destroy) +
  gsi::method ("vx_dyn", &Y::vx_dyn)
);

static gsi::SubClass<Y2, X> decl_y2 ("", "Y2",
  gsi::method ("x1", &Y2::x1)
);

static gsi::ChildSubClass<Z_P, Y3, X> decl_y3 ("", "Y3",
  gsi::method ("x1", &Y3::x1)
);

static gsi::ChildClass<Z_P, Y4> decl_y4 ("", "Y4",
  gsi::method ("x1", &Y4::x1)
);

gsi::Class<Z_P> decl_z ("", "Z",
  gsi::method ("f", &Z_P::f_org) +
  gsi::callback ("f", &Z_P::f, &Z_P::f_cb) +
  gsi::method ("f_with_x", &Z_P::f_with_x) +
  gsi::method ("f_with_y", &Z_P::f_with_y) +
  gsi::method ("f_with_yy", &Z_P::f_with_yy) +
  gsi::method ("x", &Z_P::x) +
  gsi::method ("set_x", &Z_P::set_x) +
  gsi::method ("set_x_keep", &Z_P::set_x_keep)
);

#if defined(HAVE_QT)
gsi::Class<SQ> decl_sq ("", "SQ",
  gsi::method ("trigger_s0", &SQ::trigger_s0) +
  gsi::method ("trigger_s1", &SQ::trigger_s1) +
  gsi::method ("trigger_s2", &SQ::trigger_s2) +
  gsi::method ("tag=", &SQ::set_tag) +
  gsi::method ("tag", &SQ::tag) +
  gsi::qt_signal ("s0()", "s0") +
  gsi::qt_signal<int> ("s1(int)", "s1") +
  gsi::qt_signal<const QString &, SQ *> ("s2(const QString &, SQ *)", "s2")
);
#endif

gsi::Class<SE> decl_se ("", "SE",
  gsi::method ("trigger_s0", &SE::trigger_s0) +
  gsi::method ("trigger_s1", &SE::trigger_s1) +
  gsi::method ("trigger_s2", &SE::trigger_s2) +
  gsi::method ("tag=", &SE::set_tag) +
  gsi::method ("tag", &SE::tag) +
  gsi::event ("s0", &SE::s0) +
  gsi::event ("s1", &SE::s1) +
  gsi::event ("s2", &SE::s2)
);

// ------------------------------------------------------------------
//  G and GFactory implementation and GSI declarations

GObject::GObject ()
{
  ++s_g_inst_count;
}

GObject::~GObject ()
{
  --s_g_inst_count;
}

size_t GObject::s_g_inst_count = 0;

GObject_P::GObject_P ()
  : GObject ()
{
  //  .. nothing yet ..
}

int GObject_P::g ()
{
  return g_cb.can_issue () ? g_cb.issue<GObject, int> (&GObject::g) : GObject::g ();
}

GFactory::GFactory ()
{
  //  .. nothing yet ..
}

GFactory::~GFactory ()
{
  //  .. nothing yet ..
}

GFactory_P::GFactory_P ()
{
  //  .. nothing yet ..
}

GObject *GFactory_P::f (int z)
{
  return f_cb.can_issue () ? f_cb.issue<GFactory, GObject *, int> (&GFactory::f, z) : GFactory::f (z);
}

int g_org (GObject_P *go)
{
  return go->GObject::g ();
}

int g_virtual (GObject *go)
{
  return go->g ();
}

static gsi::Class<GObject> decl_gobject_base ("", "GObjectBase",
  gsi::method_ext ("g_virtual", &g_virtual) +
  gsi::Methods()
);

static gsi::Class<GObject_P> decl_gobject (decl_gobject_base, "", "GObject",
  gsi::method_ext ("g_org", &g_org) +
  gsi::callback ("g", &GObject_P::g, &GObject_P::g_cb) +
  gsi::method ("g_inst_count", &GObject::g_inst_count)
);

GObject *f_org (GFactory_P *fo, int z)
{
  return fo->GFactory::f (z);
}

static gsi::Class<GFactory> decl_gfactory_base ("", "GFactoryBase",
  gsi::factory ("create_f", &GFactory::create_f)
);

static gsi::Class<GFactory_P> decl_gfactory (decl_gfactory_base, "", "GFactory",
  gsi::method_ext ("f", &f_org) +
  gsi::factory_callback ("f", &GFactory_P::f, &GFactory_P::f_cb)
);

static gsi::Class<B1> decl_b1 ("", "B1",
  gsi::method ("get1", &B1::get1) +
  gsi::method ("set1", &B1::set1) +
  gsi::constant ("C1", 42)
);

static gsi::Class<B2> decl_b2 ("", "B2",
  gsi::constant ("C2", 17)
);

static gsi::Class<B3> decl_b3 ("", "B3",
  gsi::constant ("C3", -1)
);

gsi::EnumIn<B3, B3::E> enum_in_b3 ("", "E",
  gsi::enum_const ("E3A", B3::E3A) +
  gsi::enum_const ("E3B", B3::E3B) +
  gsi::enum_const ("E3C", B3::E3C)
);

static std::string d4 (BB *, int a, std::string b, double c, B3::E d, tl::Variant e)
{
  return tl::sprintf ("%d,%s,%.12g,%d,%s", a, b, c, int (d), e.to_string ());
}

//  3 base classes and enums
static gsi::Class<BB> decl_bb (decl_b1, "", "BB",
  gsi::method ("d3", &BB::d3) +
  gsi::method_ext ("d4", &d4, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("c"), gsi::arg ("d", B3::E3A, "E3A"), gsi::arg ("e", tl::Variant (), "nil"), "")
);
gsi::ClassExt<BB> b2_in_bb (decl_b2);
gsi::ClassExt<BB> b3_in_bb (decl_b3);

}

