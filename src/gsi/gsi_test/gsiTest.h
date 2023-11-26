
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


#ifndef HDR_gsiTest
#define HDR_gsiTest

#include "gsiCommon.h"

#include "gsiDecl.h"

#include <string>
#include <vector>
#include <memory>
#include <cstdio>

#include "gsiCommon.h"
#include "tlVariant.h"
#include "tlString.h"

#if defined(HAVE_QT)
#  include <QObject>
#endif

namespace gsi_test
{

/**
 *  @brief A test enum
 */
enum Enum
{
  Enum_a = 1,
  Enum_b = 2,
  Enum_c = 11
};

/**
 *  @brief A simple class for testing
 *  The class is managed (derived from gsi::ObjectBase). 
 *  It provides instance counting and tracking of one instance.
 *  It provides copy semantics.
 */
struct A
  : public gsi::ObjectBase
{
  /**
   *  @brief Constructor
   */
  A ();

  /**
   *  @brief Parametrized constructor
   */
  A (int nn);

  /**
   *  @brief Parametrized constructor 2
   */
  A (int n1, int n2);

  /**
   *  @brief Parametrized constructor 3
   */
  A (int n1, int n2, double n3);

  /**
   *  @brief Copy constructor
   */
  A (const A &a);

  /**
   *  @brief Destructor
   */
  ~A ();

  /**
   *  @brief Assignment
   */
  A &operator= (const A &a);

  /**
   *  @brief A static method
   */
  static const char *a_static ();

  /**
   *  @brief Construction through tl::Variant
   */
  static tl::Variant new_a_by_variant ();

#if defined(HAVE_QT)

  /**
   *  @brief Byte sequences: tests access to QByteArray
   */
  static std::vector<int> qba_cref_to_ia (const QByteArray &ba);
  static std::vector<int> qba_ref_to_ia (QByteArray &ba)           { return qba_cref_to_ia (ba); }
  static std::vector<int> qba_cptr_to_ia (const QByteArray *ba)    { return qba_cref_to_ia (*ba); }
  static std::vector<int> qba_ptr_to_ia (QByteArray *ba)           { return qba_cref_to_ia (*ba); }
  static std::vector<int> qba_to_ia (QByteArray ba)                { return qba_cref_to_ia (ba); }

#if QT_VERSION >= 0x60000

  /**
   *  @brief Byte sequences: tests access to QByteArrayView
   */
  static std::vector<int> qbav_cref_to_ia (const QByteArrayView &ba);
  static std::vector<int> qbav_ref_to_ia (QByteArrayView &ba)           { return qbav_cref_to_ia (ba); }
  static std::vector<int> qbav_cptr_to_ia (const QByteArrayView *ba)    { return qbav_cref_to_ia (*ba); }
  static std::vector<int> qbav_ptr_to_ia (QByteArrayView *ba)           { return qbav_cref_to_ia (*ba); }
  static std::vector<int> qbav_to_ia (QByteArrayView ba)                { return qbav_cref_to_ia (ba); }

#endif

  /**
   *  @brief Byte sequences: tests access to QString
   */
  static std::vector<int> qs_cref_to_ia (const QString &qs);
  static std::vector<int> qs_ref_to_ia (QString &qs)                    { return qs_cref_to_ia (qs); }
  static std::vector<int> qs_cptr_to_ia (const QString *qs)             { return qs_cref_to_ia (*qs); }
  static std::vector<int> qs_ptr_to_ia (QString *qs)                    { return qs_cref_to_ia (*qs); }
  static std::vector<int> qs_to_ia (QString qs)                         { return qs_cref_to_ia (qs); }

#if QT_VERSION >= 0x50000

  /**
   *  @brief Byte sequences: tests access to QLatin1String
   */
  static std::vector<int> ql1s_cref_to_ia (const QLatin1String &qs);
  static std::vector<int> ql1s_ref_to_ia (QLatin1String &qs)            { return ql1s_cref_to_ia (qs); }
  static std::vector<int> ql1s_cptr_to_ia (const QLatin1String *qs)     { return ql1s_cref_to_ia (*qs); }
  static std::vector<int> ql1s_ptr_to_ia (QLatin1String *qs)            { return ql1s_cref_to_ia (*qs); }
  static std::vector<int> ql1s_to_ia (QLatin1String qs)                 { return ql1s_cref_to_ia (qs); }

#endif

#if QT_VERSION >= 0x60000

  /**
   *  @brief Byte sequences: tests access to QStringView
   */
  static std::vector<int> qsv_cref_to_ia (const QStringView &qs);
  static std::vector<int> qsv_ref_to_ia (QStringView &qs)               { return qsv_cref_to_ia (qs); }
  static std::vector<int> qsv_cptr_to_ia (const QStringView *qs)        { return qsv_cref_to_ia (*qs); }
  static std::vector<int> qsv_ptr_to_ia (QStringView *qs)               { return qsv_cref_to_ia (*qs); }
  static std::vector<int> qsv_to_ia (QStringView qs)                    { return qsv_cref_to_ia (qs); }

#endif

  /**
   *  @brief Byte sequences: tests return of QByteArray
   */
  static QByteArray ia_cref_to_qba (const std::vector<int> &ia);
  static QByteArray &ia_cref_to_qba_ref (const std::vector<int> &ia);
  static const QByteArray &ia_cref_to_qba_cref (const std::vector<int> &ia)         { return ia_cref_to_qba_ref (ia); }
  static const QByteArray *ia_cref_to_qba_cptr (const std::vector<int> &ia)         { return &ia_cref_to_qba_ref (ia); }
  static QByteArray *ia_cref_to_qba_ptr (const std::vector<int> &ia)                { return &ia_cref_to_qba_ref (ia); }

#if QT_VERSION >= 0x60000

  /**
   *  @brief Byte sequences: tests return of QByteArrayView (uses a static buffer)
   */
  static QByteArrayView ia_cref_to_qbav (const std::vector<int> &ia);
  static QByteArrayView &ia_cref_to_qbav_ref (const std::vector<int> &ia);
  static const QByteArrayView &ia_cref_to_qbav_cref (const std::vector<int> &ia)   { return ia_cref_to_qbav_ref (ia); }
  static const QByteArrayView *ia_cref_to_qbav_cptr (const std::vector<int> &ia)   { return &ia_cref_to_qbav_ref (ia); }
  static QByteArrayView *ia_cref_to_qbav_ptr (const std::vector<int> &ia)          { return &ia_cref_to_qbav_ref (ia); }

#endif

  /**
   *  @brief Byte sequences: tests return of QString
   */
  static QString ia_cref_to_qs (const std::vector<int> &ia);
  static QString &ia_cref_to_qs_ref (const std::vector<int> &ia);
  static const QString &ia_cref_to_qs_cref (const std::vector<int> &ia)           { return ia_cref_to_qs_ref (ia); }
  static const QString *ia_cref_to_qs_cptr (const std::vector<int> &ia)           { return &ia_cref_to_qs_ref (ia); }
  static QString *ia_cref_to_qs_ptr (const std::vector<int> &ia)                  { return &ia_cref_to_qs_ref (ia); }

#if QT_VERSION >= 0x50000

  /**
   *  @brief Byte sequences: tests return of QLatin1String
   */
  static QLatin1String ia_cref_to_ql1s (const std::vector<int> &ia);
  static QLatin1String &ia_cref_to_ql1s_ref (const std::vector<int> &ia);
  static const QLatin1String &ia_cref_to_ql1s_cref (const std::vector<int> &ia)   { return ia_cref_to_ql1s_ref (ia); }
  static const QLatin1String *ia_cref_to_ql1s_cptr (const std::vector<int> &ia)   { return &ia_cref_to_ql1s_ref (ia); }
  static QLatin1String *ia_cref_to_ql1s_ptr (const std::vector<int> &ia)          { return &ia_cref_to_ql1s_ref (ia); }

#endif

#if QT_VERSION >= 0x60000

  /**
   *  @brief Byte sequences: tests return of QStringView (uses a static buffer)
   */
  static QStringView ia_cref_to_qsv (const std::vector<int> &ia);
  static QStringView &ia_cref_to_qsv_ref (const std::vector<int> &ia);
  static const QStringView &ia_cref_to_qsv_cref (const std::vector<int> &ia)     { return ia_cref_to_qsv_ref (ia); }
  static const QStringView *ia_cref_to_qsv_cptr (const std::vector<int> &ia)     { return &ia_cref_to_qsv_ref (ia); }
  static QStringView *ia_cref_to_qsv_ptr (const std::vector<int> &ia)            { return &ia_cref_to_qsv_ref (ia); }

#endif

#endif

  /**
   *  @brief Byte sequences: tests access to std::vector<char> (another byte array)
   */
  static std::vector<int> ba_cref_to_ia (const std::vector<char> &ba);
  static std::vector<int> ba_ref_to_ia (std::vector<char> &ba)           { return ba_cref_to_ia (ba); }
  static std::vector<int> ba_cptr_to_ia (const std::vector<char> *ba)    { return ba_cref_to_ia (*ba); }
  static std::vector<int> ba_ptr_to_ia (std::vector<char> *ba)           { return ba_cref_to_ia (*ba); }
  static std::vector<int> ba_to_ia (std::vector<char> ba)                { return ba_cref_to_ia (ba); }

  /**
   *  @brief Byte sequences: tests return of std::vector<char>
   */
  static std::vector<char> ia_cref_to_ba (const std::vector<int> &ia);
  static std::vector<char> &ia_cref_to_ba_ref (const std::vector<int> &ia);
  static const std::vector<char> &ia_cref_to_ba_cref (const std::vector<int> &ia)   { return ia_cref_to_ba_ref (ia); }
  static std::vector<char> *ia_cref_to_ba_ptr (const std::vector<int> &ia)          { return &ia_cref_to_ba_ref (ia); }
  static const std::vector<char> *ia_cref_to_ba_cptr (const std::vector<int> &ia)   { return &ia_cref_to_ba_ref (ia); }

  /*
   *  @brief A dummy method providing a chance to set a breakpoint in the script
   */
  static void br ();

  const char *a () { return "a"; }

  bool af0 () const { return f; }
  void set_af (bool _f) { f = _f; }
  bool af1 (bool /*dummy*/) { return f; }

  static int instance_count ();
  int a1 () { 
    return n; 
  }
  int a1c () const { 
    return n; 
  }
  void a2 () const { }
  int a3 (const std::string &x) { 
    return int (x.size ());
  }
  int a3_ba (const std::vector<char> &x) {
    return int (x.size ());
  }
#if defined(HAVE_QT)
  int a3_qstr (const QString &x) { 
    return x.size (); 
  }
  int a3_qstrref (const QStringRef &x) { 
    return x.size (); 
  }
  int a3_qba (const QByteArray &x) { 
    return x.size (); 
  }
#endif
  double a4 (const std::vector<double> &d) {
    m_d = d;
    return d.back (); 
  }
  void a5 (int nn) { 
    n = nn; 
  }
  std::vector<double>::iterator a6b () {
    return m_d.begin ();
  }
  std::vector<double>::iterator a6e () {
    return m_d.end ();
  }
  double *a7b () {
    return &*m_d.begin ();
  }
  double *a7e () {
    return a7b () + m_d.size ();
  }
  const double *a8b () const {
    return &*m_d.begin ();
  }
  const double *a8e () const {
    return a8b () + m_d.size ();
  }

  bool a9a (int i) const { return i == 5; }
  int a9b (bool f) const { return f ? 5 : -5; }

  short a11_s (double f) { return short(a11_l(f)); }
  unsigned short a11_us (double f) { return (unsigned short)(a11_ul(f)); }
  int a11_i (double f) { return int(a11_l(f)); }
  unsigned int a11_ui (double f) { return (unsigned int)(a11_ul(f)); }
  long a11_l (double f) { return long(f); }
  unsigned long a11_ul (double f) { return (unsigned long)(f); }
  long long a11_ll (double f) { return (long long)(f); }
  unsigned long long a11_ull (double f) { return (unsigned long long)(f); }

  std::string a10_d (double f) { return tl::to_string (f); }
  std::vector<char> a10_d_ba (double f) { std::string s = tl::to_string (f); return std::vector<char> (s.begin (), s.end ()); }

#if defined(HAVE_QT)
  QByteArray a10_d_qba (double f) { return tl::to_qstring (tl::to_string (f)).toUtf8 (); }
  QString a10_d_qstr (double f) { return tl::to_qstring (tl::to_string (f)); }
  QStringRef a10_d_qstrref (double f) { m_s = tl::to_qstring (tl::to_string (f)); return QStringRef (&m_s); }
#endif

  std::string a10_f (float f) { return tl::to_string(f); }
  std::string a10_s (short l) { return tl::to_string(int (l)); }
  std::string a10_us (unsigned short l) { return tl::to_string(int (l)); }
  std::string a10_i (int l) { return tl::to_string(l); }
  std::string a10_ui (unsigned int l) { return tl::to_string(l); }
  std::string a10_l (long l) { return tl::to_string(l); }
  std::string a10_ul (unsigned long l) { return tl::to_string(l); }
  std::string a10_ll (long long l) { return tl::to_string(l); }
  std::string a10_ull (unsigned long long l) { return tl::to_string(l); }
  std::string a10_fptr (float *f) { if (f) { *f += 5; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_dptr (double *f) { if (f) { *f += 6; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_iptr (int *f) { if (f) { *f += 7; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_bptr (bool *f) { if (f) { *f = true; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_uiptr (unsigned int *f) { if (f) { *f += 10; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_ulptr (unsigned long *f) { if (f) { *f += 11; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_lptr (long *f) { if (f) { *f += 12; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_llptr (long long *f) { if (f) { *f += 13; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_ullptr (unsigned long long *f) { if (f) { *f += 14; return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_cfptr (const float *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_cdptr (const double *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_ciptr (const int *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_cbptr (const bool *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_cuiptr (const unsigned int *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_culptr (const unsigned long *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_clptr (const long *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_cllptr (const long long *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_cullptr (const unsigned long long *f) { if (f) { return tl::to_string(*f); } else { return "nil"; } }
  std::string a10_sptr (std::string *f) { if (f) { *f += "x"; return *f; } else { return "nil"; } }
  std::string a10_csptr (const std::string *f) { if (f) { return *f; } else { return "nil"; } }
  std::string a10_fref (float &f) { f += 10; return tl::to_string(f); }
  std::string a10_dref (double &f) { f += 11; return tl::to_string(f); }
  std::string a10_iref (int &f) { f += 12; return tl::to_string(f); }
  std::string a10_bref (bool &f) { f = true; return tl::to_string(f); }
  std::string a10_uiref (unsigned int &f) { f += 14; return tl::to_string(f); }
  std::string a10_ulref (unsigned long &f) { f += 15; return tl::to_string(f); }
  std::string a10_lref (long &f) { f += 16; return tl::to_string(f); }
  std::string a10_llref (long long &f) { f += 17; return tl::to_string(f); }
  std::string a10_ullref (unsigned long long &f) { f += 18; return tl::to_string(f); }
  std::string a10_sref (std::string &f) { f += "y"; return f; }
  std::string a10_cfref (const float &f) { return tl::to_string(f); }
  std::string a10_cdref (const double &f) { return tl::to_string(f); }
  std::string a10_ciref (const int &f) { return tl::to_string(f); }
  std::string a10_cbref (const bool &f) { return tl::to_string(f); }
  std::string a10_cuiref (const unsigned int &f) { return tl::to_string(f); }
  std::string a10_culref (const unsigned long &f) { return tl::to_string(f); }
  std::string a10_clref (const long &f) { return tl::to_string(f); }
  std::string a10_cllref (const long long &f) { return tl::to_string(f); }
  std::string a10_cullref (const unsigned long long &f) { return tl::to_string(f); }
  std::string a10_csref (const std::string &f) { return f; }

  Enum get_e () const { return e; }
  Enum *get_eptr () { return int (e) == 0 ? 0 : &e; }
  const Enum *get_ecptr () const { return int (e) == 0 ? 0 : &e; }
  Enum &get_eref () { return e; }
  const Enum &get_ecref () const { return e; }
  void set_e (Enum _e) { e = _e; }
  void set_eptr (Enum *_e) { e = _e ? *_e : Enum (0); }
  void set_ecptr (const Enum *_e) { e = _e ? *_e : Enum (0); }
  void set_eref (Enum &_e) { e = _e; }
  void set_ecref (const Enum &_e) { e = _e; }
  void mod_eptr (Enum *_e, Enum ee) { if (_e) *_e = ee; }
  void mod_eref (Enum &_e, Enum ee) { _e = ee; }
  void push_ev (Enum e) { ee.push_back (e); }
  const std::vector<Enum> &ev () const { return ee; }

#if defined(HAVE_QT)
  QFlags<Enum> get_ef () const { return ef; }
  QFlags<Enum> *get_efptr () { return int (ef) == 0 ? 0 : &ef; }
  const QFlags<Enum> *get_efcptr () const { return int (ef) == 0 ? 0 : &ef; }
  QFlags<Enum> &get_efref () { return ef; }
  const QFlags<Enum> &get_efcref () const { return ef; }
  void set_ef (QFlags<Enum> _ef) { ef = _ef; }
  void set_efptr (QFlags<Enum> *_ef) { ef = _ef ? *_ef : Enum (0); }
  void set_efcptr (const QFlags<Enum> *_ef) { ef = _ef ? *_ef : Enum (0); }
  void set_efref (QFlags<Enum> &_ef) { ef = _ef; }
  void set_efcref (const QFlags<Enum> &_ef) { ef = _ef; }
  void mod_efptr (QFlags<Enum> *_ef, Enum ee) { if (_ef) *_ef |= ee; }
  void mod_efref (QFlags<Enum> &_ef, Enum ee) { _ef |= ee; }
#endif

  const char *a_vp1 (void *s) { return (const char *)s; }
  void *a_vp2 () { return (void *)"abc"; }

  static void a20 (A *ptr);
  static A *a20_get ();

  std::string to_s () const;

  //  members
  std::vector<double> m_d;
  int n;
  bool f;
  Enum e;
#if defined(HAVE_QT)
  QFlags<Enum> ef;
#endif
  std::vector<Enum> ee;
#if defined(HAVE_QT)
  QString m_s;
#endif
};


struct A_NC
  : public A
{
  A_NC () : A () { }
  A_NC (int nn) : A (nn) { }
  
private:
  friend struct B;

  A_NC (const A_NC &a) : A (a) { }
  A_NC &operator= (const A_NC &);
};

template<class Iter>
struct ValueIter
{
public:
  typedef typename std::iterator_traits<Iter> it_traits;
  typedef typename it_traits::value_type value_type;
  typedef std::forward_iterator_tag iterator_category;
  typedef value_type reference;
  typedef void pointer;
  typedef void difference_type;
  
  ValueIter (Iter i) : iter(i) { }

  value_type operator*() const
  {
    return *iter;
  }
  
  ValueIter &operator++() 
  {
    ++iter;
    return *this;
  }
  
  bool operator== (const ValueIter &d) const
  {
    return iter == d.iter;
  }
  
private:
  Iter iter;
};

template<class Iter>
struct FreeIter
{
public:
  typedef typename std::iterator_traits<Iter> it_traits;
  typedef typename it_traits::value_type value_type;
  typedef std::forward_iterator_tag iterator_category;
  typedef value_type reference;
  typedef void pointer;
  typedef void difference_type;
  
  FreeIter (Iter b, Iter e) : mb(b), me(e) { }

  value_type operator*() const
  {
    return *mb;
  }
  
  FreeIter &operator++() 
  {
    ++mb;
    return *this;
  }
  
  bool operator== (const FreeIter &d) const
  {
    return mb == d.mb;
  }

  bool at_end () const
  {
    return mb == me;
  }
  
private:
  Iter mb, me;
};

template<class Iter>
struct FreeIterUseRef
{
public:
  typedef typename std::iterator_traits<Iter> it_traits;
  typedef typename it_traits::value_type value_type;
  typedef std::forward_iterator_tag iterator_category;
  typedef typename it_traits::reference reference;
  typedef void pointer;
  typedef void difference_type;
  
  FreeIterUseRef (Iter b, Iter e) : mb(b), me(e) { }

  reference operator*() const
  {
    return *mb;
  }
  
  FreeIterUseRef &operator++() 
  {
    ++mb;
    return *this;
  }
  
  bool operator== (const FreeIterUseRef &d) const
  {
    return mb == d.mb;
  }

  bool at_end () const
  {
    return mb == me;
  }
  
private:
  Iter mb, me;
};

template<class Iter>
struct FreeIterUsePtr
{
public:
  typedef typename std::iterator_traits<Iter> it_traits;
  typedef typename it_traits::pointer value_type;
  typedef std::forward_iterator_tag iterator_category;
  typedef typename it_traits::pointer reference;
  typedef void pointer;
  typedef void difference_type;
  
  FreeIterUsePtr (Iter b, Iter e) : mb(b), me(e) { }

  reference operator*() const
  {
    return &*mb;
  }
  
  FreeIterUsePtr &operator++() 
  {
    ++mb;
    return *this;
  }
  
  bool operator== (const FreeIterUsePtr &d) const
  {
    return mb == d.mb;
  }

  bool at_end () const
  {
    return mb == me;
  }
  
private:
  Iter mb, me;
};

struct B
{
  B (); 
  B (const B &d);
  B &operator= (const B &d);
  ~B ();

  static void set_inst (B *b);
  static void del_inst ();
  static B *inst ();
  static bool has_inst ();

  static int instance_count ();

  /**
   *  @brief Construction through tl::Variant
   */
  static tl::Variant new_b_by_variant ();

#if __cplusplus >= 201703L
  /**
   *  @brief std::optional for simple and complex types
   */

  static std::optional<int> int_to_optional (int value, bool exists)                { return exists ? std::optional<int> (value) : std::optional<int> (); }
  static std::optional<A> int_to_optional_a (int value, bool exists)                { return exists ? std::optional<A> (A (value)) : std::optional<A> (); }

  static int optional_to_int (std::optional<int> optional, int def)                 { return optional ? optional.value () : def; }
  static int optional_cref_to_int (const std::optional<int> &optional, int def)     { return optional_to_int (optional, def); }
  static int optional_ref_to_int (std::optional<int> &optional, int def)            { return optional_to_int (optional, def); }
  static int optional_cptr_to_int (const std::optional<int> *optional, int def)     { return optional_to_int (*optional, def); }
  static int optional_ptr_to_int (std::optional<int> optional, int def)             { return optional_to_int (*optional, def); }

  static int optional_a_to_int (std::optional<A> optional, int def)                 { return optional ? optional.value ().a1 () : def; }
  static int optional_a_cref_to_int (const std::optional<A> &optional, int def)     { return optional_a_to_int (optional, def); }
  static int optional_a_ref_to_int (std::optional<A> &optional, int def)            { return optional_a_to_int (optional, def); }
  static int optional_a_cptr_to_int (const std::optional<A> *optional, int def)     { return optional_a_to_int (*optional, def); }
  static int optional_a_ptr_to_int (std::optional<A> optional, int def)             { return optional_a_to_int (*optional, def); }
#endif

  std::string addr () const;

  int always_5 () const {
    return 5; 
  }

  void set_str (const char *s) { m = s; }
  const std::string &str () const { return m; }
  const char *str_ccptr () const { return m.c_str (); }

  void set_str_combine (const char *p1, const char *p2) 
  { 
    m = p1; 
    m += p2;
  }

  int b3 (A *aptr) const { 
    return aptr->n; 
  }

  std::string aref_to_s (const A &aref) {
    return tl::sprintf ("b4_result: %d", aref.n); 
  }

  A make_a (int n) {
    return A(n);
  }

  void set_an (int n)
  { 
    m_a.n = n;
  }

  int an (A a)
  { 
    return a.n;
  }

  void set_an_cref (const int &n)
  { 
    m_a.n = n;
  }

  const int &an_cref (const A &a)
  { 
    return a.n;
  }

  std::vector <A>::const_iterator b10b () const
  {
    return m_av.begin ();
  }

  std::vector <A>::const_iterator b10e () const
  {
    return m_av.end ();
  }

  std::vector <A>::iterator b10b_nc ()
  {
    return m_av.begin ();
  }

  std::vector <A>::iterator b10e_nc ()
  {
    return m_av.end ();
  }

  ValueIter<std::vector <A>::const_iterator> b11b () const
  {
    return ValueIter<std::vector <A>::const_iterator> (m_av.begin ());
  }

  ValueIter<std::vector <A>::const_iterator> b11e () const
  {
    return ValueIter<std::vector <A>::const_iterator> (m_av.end ());
  }

  std::vector <A_NC *>::const_iterator b12b () const
  {
    return m_av_nc.begin ();
  }

  std::vector <A_NC *>::const_iterator b12e () const
  {
    return m_av_nc.end ();
  }

  std::vector <const A_NC *>::const_iterator b13b () const
  {
    return m_avc_nc.begin ();
  }

  std::vector <const A_NC *>::const_iterator b13e () const
  {
    return m_avc_nc.end ();
  }

  A *amember_or_nil (bool nn) { return nn ? &m_a : 0; }
  A *amember_ptr () { return &m_a; }
  A &amember_ref () { return m_a; }
  const A *amember_cptr () const { return &m_a; }
  const A &amember_cref () const { return m_a; }

  bool arg_is_not_nil (A *a)
  {
    return a != 0;
  }

  std::vector <A> av () const
  {
    return m_av;
  }

  const std::vector <A> &av_cref () const
  {
    return m_av;
  }

  std::vector <A> &av_ref ()
  {
    return m_av;
  }

  void set_av_cref (const std::vector <A> &v)
  {
    m_av = v;
  }

  void set_av_ref (std::vector <A> &v)
  {
    m_av = v;
  }

  void set_av (std::vector <A> v)
  {
    m_av = v;
  }

  void set_av_cptr (const std::vector <A> *v)
  {
    if (v) {
      m_av = *v;
    } else {
      m_av.clear ();
    }
  }

  void set_av_ptr (std::vector <A> *v)
  {
    if (v) {
      m_av = *v;
    } else {
      m_av.clear ();
    }
  }

  void push_a (A a)
  {
    m_av.push_back (a);
  }

  void push_a_cref (const A &a)
  {
    m_av.push_back (a);
  }

  void push_a_ref (A &a)
  {
    m_av.push_back (a);
  }

  void push_a_cptr (const A *a)
  {
    if (a) {
      m_av.push_back (*a);
    }
  }

  void push_a_ptr (A *a)
  {
    if (a) {
      m_av.push_back (*a);
    }
  }

  FreeIter<std::vector <A>::const_iterator> b18 () const
  {
    return FreeIter<std::vector <A>::const_iterator> (m_av.begin (), m_av.end ());
  }

  FreeIterUseRef<std::vector <A>::const_iterator> b18b () const
  {
    return FreeIterUseRef<std::vector <A>::const_iterator> (m_av.begin (), m_av.end ());
  }

  FreeIterUsePtr<std::vector <A>::const_iterator> b18c () const
  {
    return FreeIterUsePtr<std::vector <A>::const_iterator> (m_av.begin (), m_av.end ());
  }

  bool b20a (const tl::Variant &var) const { return var.is_nil (); }
  bool b20b (tl::Variant &var) const { return var.is_double (); }
  bool b20c (tl::Variant var) const { return var.is_long () || var.is_longlong (); }
  bool b20d (const tl::Variant &var) const { return var.is_a_string (); }
  bool b20e (const tl::Variant &var) const { return var.is_bool (); }

  std::string b21a (const tl::Variant &var) const { return var.to_string (); }
  double b21b (const tl::Variant &var) const { return var.to_double (); }
  long b21c (const tl::Variant &var) const { return var.to_long (); }

  long b22a (const std::vector<tl::Variant> &vars) {
    if (vars.empty ()) {
      m_var = tl::Variant ();
    } else {
      m_var = vars.back ();
    }
    m_vars = vars;
    return long (vars.size ());
  }

  long set_vars (const std::vector<tl::Variant> &vars) {
    m_vars = vars;
    return long (vars.size ());
  }

  tl::Variant b22b () const { return tl::Variant (); }
  const tl::Variant &b22c () const { return m_var; }
  tl::Variant &b22d () { return m_var; }

  tl::Variant var () const { return m_vars.back (); }
  const tl::Variant &var_cref () const { return m_vars.back (); }
  const tl::Variant *var_cptr () const { return m_vars.empty () ? 0 : &m_vars.back (); }
  tl::Variant &var_ref () { return m_vars.back (); }
  tl::Variant *var_ptr () { return m_vars.empty () ? 0 : &m_vars.back (); }

   std::vector<tl::Variant> b23a () { return m_vars; }
  const std::vector<tl::Variant> &b23b () const { return m_vars; }
  std::vector<tl::Variant> &b23c () { return m_vars; }
  tl::Variant b23d () { return tl::Variant (m_vars.begin (), m_vars.end ()); }
  const std::vector<tl::Variant> *b23e () const { return &m_vars; }
  const std::vector<tl::Variant> *b23e_null () const { return 0; }
  std::vector<tl::Variant> *b23f () { return &m_vars; }
  std::vector<tl::Variant> *b23f_null () { return 0; }

  std::vector<tl::Variant>::const_iterator b24b () { return m_vars.begin (); }
  std::vector<tl::Variant>::const_iterator b24e () { return m_vars.end (); }

  int b30 () const { return 17; }
  const char *b31 (int) const { return "xz"; }
  const char *b33 (const A & /*a*/) const { return "aref"; }
  const char *b34 (A /*a*/, int /*x*/) const { return "aref+i"; }
  double b32 (const char *, int) const { return 20.5; }

  void insert_map1 (int k, const std::string &v) { m_map1[k] = v; }
  std::map<int, std::string> map1 () const { return m_map1; }
  const std::map<int, std::string> &map1_cref () const { return m_map1; }
  const std::map<int, std::string> *map1_cptr () const { return &m_map1; }
  const std::map<int, std::string> *map1_cptr_null () const { return 0; }
  std::map<int, std::string> &map1_ref () { return m_map1; }
  std::map<int, std::string> *map1_ptr () { return &m_map1; }
  std::map<int, std::string> *map1_ptr_null () { return 0; }
  void set_map1_cref (const std::map<int, std::string> &m) { m_map1 = m; }
  void set_map1_ref (std::map<int, std::string> &m) { m_map1 = m; }
  void set_map1_cptr (const std::map<int, std::string> *m) { if (m) { m_map1 = *m; } }
  void set_map1_ptr (std::map<int, std::string> *m) { if (m) { m_map1 = *m; } }
  void set_map1 (std::map<int, std::string> m) { m_map1 = m; }

  void insert_map2 (const tl::Variant &k, const tl::Variant &v) { m_map2[k] = v; }
  const std::map<tl::Variant, tl::Variant> *map2 () const { return &m_map2; }
  const std::map<tl::Variant, tl::Variant> *map2_null () const { return 0; }
  void set_map2 (const std::map<tl::Variant, tl::Variant> &m) { m_map2 = m; }

  FreeIter<std::vector <B>::const_iterator> each_b_copy () const
  {
    return FreeIter<std::vector <B>::const_iterator> (m_bv.begin (), m_bv.end ());
  }

  FreeIterUseRef<std::vector <B>::const_iterator> each_b_cref () const
  {
    return FreeIterUseRef<std::vector <B>::const_iterator> (m_bv.begin (), m_bv.end ());
  }

  FreeIterUsePtr<std::vector <B>::const_iterator> each_b_cptr () const
  {
    return FreeIterUsePtr<std::vector <B>::const_iterator> (m_bv.begin (), m_bv.end ());
  }

  FreeIterUseRef<std::vector <B>::iterator> each_b_ref () 
  {
    return FreeIterUseRef<std::vector <B>::iterator> (m_bv.begin (), m_bv.end ());
  }

  FreeIterUsePtr<std::vector <B>::iterator> each_b_ptr ()
  {
    return FreeIterUsePtr<std::vector <B>::iterator> (m_bv.begin (), m_bv.end ());
  }

  void push_b (const B &b) { m_bv.push_back (b); }

  std::map<int, A *> map_iaptr () { return m_map_iaptr; }
  const std::map<int, A *> &map_iaptr_cref () { return m_map_iaptr; }
  std::map<int, A *> &map_iaptr_ref () { return m_map_iaptr; }
  const std::map<int, A *> *map_iaptr_cptr () { return &m_map_iaptr; }
  std::map<int, A *> *map_iaptr_ptr () { return &m_map_iaptr; }

  static void insert_map_iaptr (std::map<int, A *> &m, int k, A *v) { m.insert (std::make_pair (k, v)); }
  void set_map_iaptr (std::map<int, A *> m) { m_map_iaptr = m; }
  void set_map_iaptr_cref (const std::map<int, A *> &m) { m_map_iaptr = m; }
  void set_map_iaptr_ref (std::map<int, A *> &m) { m_map_iaptr = m; }

  void set_map_iaptr_cptr (const std::map<int, A *> *m) 
  { 
    if (! m) {
      m_map_iaptr.clear ();
    } else {
      m_map_iaptr = *m;
    }
  }

  void set_map_iaptr_ptr (const std::map<int, A *> *m) 
  { 
    if (! m) {
      m_map_iaptr.clear ();
    } else {
      m_map_iaptr = *m;
    }
  }

  static void insert_map_iacptr (std::map<int, const A *> &m, int k, const A *v) { m.insert (std::make_pair (k, v)); }
  const std::map<int, const A *> &map_iacptr () { return m_map_iacptr; }
  void set_map_iacptr (const std::map<int, const A *> &m) { m_map_iacptr = m; }

  static void insert_map_ia (std::map<int, A> &m, int k, A v) { m.insert (std::make_pair (k, v)); }
  const std::map<int, A> &map_ia () { return m_map_ia; }
  void set_map_ia (const std::map<int, A> &m) { m_map_ia = m; }

  static void insert_map_iav (std::map<int, std::vector<A> > &m, int k, const std::vector<A> &v) { m.insert (std::make_pair (k, v)); }
  static void push_map_iav (std::map<int, std::vector<A> > &m, int k, const A &v) { m[k].push_back (v); }
  const std::map<int, std::vector<A> > &map_iav () { return m_map_iav; }
  void set_map_iav (const std::map<int, std::vector<A> > &m) { m_map_iav = m; }

  static void push_vvs (std::vector<std::vector<std::string> > &m, const std::vector<std::string> &v) { m.push_back (v); }
  std::vector<std::vector<std::string> > vvs () { return m_vvs; }
  std::vector<std::vector<std::string> > &vvs_ref () { return m_vvs; }
  std::vector<std::vector<std::string> > *vvs_ptr () { return &m_vvs; }
  const std::vector<std::vector<std::string> > &vvs_cref () const { return m_vvs; }
  const std::vector<std::vector<std::string> > *vvs_cptr () const { return &m_vvs; }
  void set_vvs (std::vector<std::vector<std::string> > v) { m_vvs = v; }
  void set_vvs_ref (std::vector<std::vector<std::string> > &v) { m_vvs = v; }
  void set_vvs_cref (const std::vector<std::vector<std::string> > &v) { m_vvs = v; }

  void set_vvs_cptr (const std::vector<std::vector<std::string> > *v) 
  { 
    if (! v) {
      m_vvs.clear ();
    } else {
      m_vvs = *v;
    }
  }

  void set_vvs_ptr (std::vector<std::vector<std::string> > *v) 
  { 
    if (! v) {
      m_vvs.clear ();
    } else {
      m_vvs = *v;
    }
  }

  static void push_ls (std::list<std::string> &m, const std::string &v) { m.push_back (v); }
  std::list<std::string> ls () { return m_ls; }
  void set_ls (std::list<std::string> v) { m_ls = v; }

  static void push_ss (std::set<std::string> &m, const std::string &v) { m.insert (v); }
  std::set<std::string> ss () { return m_ss; }
  void set_ss (std::set<std::string> v) { m_ss = v; }

#if defined(HAVE_QT)
  static void push_qls (QList<QString> &m, const QString &v) { m.push_back (v); }
  QList<QString> qls () { return m_qls; }
  void set_qls (QList<QString> v) { m_qls = v; }

  static void push_qsl (QStringList &m, const QString &v) { m.push_back (v); }
  QStringList qsl () { return m_qls; }
  void set_qsl (QStringList v) { m_qls = v; }

  static void push_qlv (QList<QVariant> &m, const QVariant &v) { m.push_back (v); }
  QList<QVariant> qlv () { return m_qlv; }
  void set_qlv (QList<QVariant> v) { m_qlv = v; }

  static void push_qvs (QVector<QString> &m, const QString &v) { m.push_back (v); }
  QVector<QString> qvs () { return m_qvs; }
  void set_qvs (QVector<QString> v) { m_qvs = v; }

  static void push_qss (QSet<QString> &m, const QString &v) { m.insert (v); }
  QSet<QString> qss () { return m_qss; }
  void set_qss (QSet<QString> v) { m_qss = v; }

  static void insert_qmap_is (QMap<int, QString> &m, int k, const QString &v) { m.insert (k, v); }
  QMap<int, QString> qmap_is () { return m_qmap_is; }
  void set_qmap_is (QMap<int, QString> v) { m_qmap_is = v; }

  static void insert_qhash_is (QHash<int, QString> &m, int k, const QString &v) { m.insert (k, v); }
  QHash<int, QString> qhash_is () { return m_qhash_is; }
  void set_qhash_is (QHash<int, QString> v) { m_qhash_is = v; }
#endif

  std::string m;
  A m_a;
  std::vector <B> m_bv;
  std::vector <A> m_av;
  std::vector <A_NC *> m_av_nc;
  std::vector <const A_NC *> m_avc_nc;
  tl::Variant m_var;
  std::vector<tl::Variant> m_vars;
  std::map<int, std::string> m_map1;
  std::map<tl::Variant, tl::Variant> m_map2;
  std::map<int, A *> m_map_iaptr;
  std::map<int, const A *> m_map_iacptr;
  std::map<int, A> m_map_ia;
  std::map<int, std::vector<A> > m_map_iav;
  std::vector<std::vector<std::string> > m_vvs; 
  std::list<std::string> m_ls;
  std::set<std::string> m_ss;
#if defined(HAVE_QT)
  QList<QString> m_qls;
  QList<QVariant> m_qlv;
  QStringList m_qsl;
  QVector<QString> m_qvs;
  QSet<QString> m_qss;
  QMap<int, QString> m_qmap_is;
  QHash<int, QString> m_qhash_is;
#endif

  static B *b_inst;
};

class CopyDetector
{
public:
  CopyDetector (int x)
    : m_x (x), m_xx (x)
  { }

  CopyDetector ()
    : m_x (0), m_xx (0)
  { }

  CopyDetector (const CopyDetector &d)
    : m_x (d.m_x), m_xx (d.m_xx + 1)  //  this detects the copy
  { }

  CopyDetector &operator= (const CopyDetector &d)
  {
    m_x = d.m_x;
    m_xx = d.m_xx + 1;
    return *this;
  }

  int x () const { return m_x; }
  int xx () const { return m_xx; }

private:
  int m_x, m_xx;
};

class C
{
public:
  virtual ~C () { }

  virtual unsigned int f (const std::string & /*s*/) const
  {
    return 1977;
  }

  unsigned int g (const std::string s) const
  {
    return f(s);
  }

  virtual void vfunc (const CopyDetector &)
  {
    //  .. nothing yet ..
  }

  void call_vfunc (const CopyDetector &cd)
  {
    vfunc (cd);
  }

  CopyDetector pass_cd_direct (const CopyDetector &cd) { return cd; }
  const CopyDetector &pass_cd_cref (const CopyDetector &cd) { return cd; }
  const CopyDetector *pass_cd_cptr (const CopyDetector &cd) { return &cd; }
  CopyDetector *pass_cd_ptr (const CopyDetector &cd) { return const_cast<CopyDetector *> (&cd); }
  CopyDetector &pass_cd_ref (const CopyDetector &cd) { return const_cast<CopyDetector &> (cd); }

  static int s1 ();
  static std::vector<int>::const_iterator s1a ();
  static std::vector<int>::const_iterator s1b ();
  static void s2 (double x);
  static void s2clr ();
  static std::string s3 (double x);

  static std::vector<int> m_v;
};

class C_P
  : public C
{
public:
  virtual unsigned int f (const std::string &s) const
  {
    return f_cb.can_issue () ? f_cb.issue<C, unsigned int, const std::string &> (&C::f, s) : C::f (s);
  }

  virtual void vfunc (const CopyDetector &cd)
  {
    return vfunc_cb.can_issue () ? vfunc_cb.issue<C, const CopyDetector &> (&C::vfunc, cd) : C::vfunc (cd);
  }

  gsi::Callback f_cb;
  gsi::Callback vfunc_cb;
};

struct E
  : public gsi::ObjectBase
{
  E ();
  ~E ();

  static void reset_inst ();
  static int inst_count();

  void s1() { ev0(); }
  void s2() { ev1(this); }
  void s3() { ev2(18, "hallo"); }
  // no events with return available currently:
  // int s1r(const std::string &s) { return ev0r(s); }
  void bindme() const { }
  static const E *ic ();
  static E *inc ();
  static const E &icref ();
  static E &incref ();
  void set_x(int i) { x = i; }
  int get_x() const { return x; }

  tl::Event ev0;
  tl::event<E *> ev1;
  tl::event<int, const std::string &> ev2;
  // no events with return available currently:
  // tl::event_with_return<int, const std::string &> ev0r;

private:
  int x;
  static std::unique_ptr<E> e_inst;
  static int e_count;
};

//  Same as "E", but not based on ObjectBase
struct F
{
  F() : x(0) { }
  static const F *ic();
  static F *inc();
  static const F &icref();
  static F &incref();
  void set_x(int i) { x = i; }
  int get_x() const { return x; }

  int x;
  static std::unique_ptr<F> f_inst;
};

struct G
{
  G () : m_iv (0) { }
  int iv() const { return m_iv; }
  std::string sv() const { return m_sv; }

  void set_iv (int v) { m_iv = v; }
  void set_sv1 (const char *s) { m_sv = s; }
  void set_sv2 (const std::string &s) { m_sv = s; }
  void set_vv (int i, const std::string &s) { m_iv = i; m_sv = s; }

  int m_iv;
  std::string m_sv;
};

class X
  : public gsi::ObjectBase
{
public:
  X ();
  X (const char *x);
  X (const X &x);
  X &operator= (const X &x);
  virtual ~X ();

  int x1 () const { return 17; }
  int x2 () const { return 42; }
  static void init ();
  static int instances ();
  static const X *x_cptr ();
  static X *x_ptr ();
  static std::vector<X> vx ();
  static std::vector<const X *> vx_cptr ();
  static std::vector<X *> vx_ptr ();

  virtual std::string cls_name () const;
  std::string s () const;
  void set_s (const std::string &s);
  void set_si (int v);

protected:
  std::string m_s;

private:
  static std::unique_ptr<X> sp_a, sp_b;
};

class Y
  : public X
{
public:
  Y ();
  Y (const char *x);
  ~Y ();

  int x1() const { return 1; }
  int y1() const { return 117; }
  static void init ();
  static const X *y_cptr ();
  static X *y_ptr ();
  int vx_dyn_count ();
  void vx_dyn_make ();
  void vx_dyn_destroy ();
  std::vector<X *> vx_dyn ();
  static std::vector<const X *> vyasx_cptr ();
  static std::vector<X *> vyasx_ptr ();
  static std::vector<const Y *> vy_cptr ();
  static std::vector<Y *> vy0_ptr ();
  static std::vector<Y *> vy_ptr (); 
  virtual std::string cls_name () const;
  int i () const;

private:
  static std::unique_ptr<Y> sp_a, sp_b;
  static int s_dyn_count;
  Y *mp_c;
};

class Y2
  : public X
{
public:
  int x1 () const { return 2; }
};

class Y3
  : public X
{
public:
  int x1 () const { return 3; }
};

class Y4
{
public:
  int x1 () const { return 4; }
};

class YY : public Y
{
public:
  YY ();
  YY (const char *x);
  virtual std::string cls_name() const;
};

class Z
{
public:
  Z ();
  virtual ~Z ();
  virtual std::string f (X *x);

  void set_x (X *x);
  X *x ();
  void set_x_keep (X *x);

private:
  X *mp_x;
};
    
class Z_P
  : public Z
{
public:
  virtual std::string f (X *x);
  std::string f_org (X *x);
  std::string f_with_x (const std::string &s);
  std::string f_with_y (const std::string &s);
  std::string f_with_yy (const std::string &s);

  gsi::Callback f_cb;
};

//  An object that is produced by a factory
class GObject
{
public:
  GObject ();
  virtual ~GObject ();
  virtual int g () { return 0; }
  static size_t g_inst_count ()
  {
    return s_g_inst_count;
  }

private:
  static size_t s_g_inst_count;
};

class GObject_P : public GObject
{
public:
  GObject_P ();
  virtual int g ();

  gsi::Callback g_cb;
};

//  This is the factory for G
class GFactory
{
public:
  GFactory ();
  virtual ~GFactory ();

  virtual GObject *f (int /*z*/) { return 0; }
  static GObject *create_f (GFactory *g_factory, int z)
  {
    return g_factory->f (z);
  }
};

class GFactory_P : public GFactory
{
public:
  GFactory_P ();
  virtual GObject *f (int z);

  gsi::Callback f_cb;
};

#if defined(HAVE_QT)
class SQ
  : public QObject
{
Q_OBJECT
public:
  SQ ();

  void trigger_s0 ();
  void trigger_s1 (int x);
  void trigger_s2 (const QString &s);

  void set_tag (int x);
  int tag () const
  {
    return m_tag;
  }

signals:
  void s0 ();
  void s1 (int x);
  void s2 (const QString &s, SQ *sq);

private:
  int m_tag;
};
#endif

class SE
  : public tl::Object
{
public:
  SE ();

  void trigger_s0 ();
  void trigger_s1 (int x);
  void trigger_s2 (const std::string &s);

  void set_tag (int x);
  int tag () const
  {
    return m_tag;
  }

public:
  tl::event<> s0;
  tl::event<int> s1;
  tl::event<const std::string &, SE *> s2;

private:
  int m_tag;
};

class B1
{
public:
  B1 () : m_value (0) {}

  int get1 () { return m_value; }
  void set1 (int v) { m_value = v; }
private:
  int m_value;
};

class B2
{
public:
  B2 () {}
};

class B3
{
public:
  B3 () {}
  enum E { E3A = 100, E3B = 101, E3C = 102 };
};

class BB
  : public B1, public B2, public B3
{
public:
  int d3 (B3::E a, B3::E b) { return b - a; }
};

}

#endif

