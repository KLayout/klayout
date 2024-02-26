
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "gsiObjectHolder.h"
#include "gsiExpression.h"
#include "gsiSignals.h"
#include "gsiInspector.h"
#include "gsiVariantArgs.h"
#include "tlString.h"
#include "tlInternational.h"
#include "tlException.h"
#include "tlAssert.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlExpression.h"
#include "tlFileUtils.h"
#include "tlStream.h"
#include "tlEnv.h"

#include "rba.h"
#include "rbaInspector.h"
#include "rbaConvert.h"
#include "rbaUtils.h"
#include "rbaInternal.h"
#include "rbaMarshal.h"

#include <map>
#include <set>
#include <memory>
#include <iostream>
#include <cctype>
#include <algorithm>

#if !defined(HAVE_RUBY_VERSION_CODE)
#  define HAVE_RUBY_VERSION_CODE 10901
#endif

//  For the installation path
#ifdef _WIN32
#  include <windows.h>
#endif

// -------------------------------------------------------------------
//  This part is available only if Ruby is enabled

#ifdef HAVE_RUBY

#include <ruby.h>
#include <signal.h>

#if HAVE_RUBY_VERSION_CODE >= 20300
#  include <ruby/debug.h>
#endif

namespace rba
{

// -------------------------------------------------------------------
//  RubyStackTraceProvider definition and implementation

RubyStackTraceProvider::RubyStackTraceProvider (const std::string &scope)
  : m_scope (scope)
{ }

std::vector<tl::BacktraceElement>
RubyStackTraceProvider::stack_trace () const
{
  std::vector<tl::BacktraceElement> bt;
  bt.push_back (tl::BacktraceElement (rb_sourcefile (), rb_sourceline ()));
  static ID id_caller = rb_intern ("caller");
  rba_get_backtrace_from_array (rb_funcall (rb_mKernel, id_caller, 0), bt, 0);
  return bt;
}

size_t
RubyStackTraceProvider::scope_index () const
{
  if (! m_scope.empty ()) {
    return RubyStackTraceProvider::scope_index (stack_trace (), m_scope);
  } else {
    return 0;
  }
}

size_t
RubyStackTraceProvider::scope_index (const std::vector<tl::BacktraceElement> &bt, const std::string &scope)
{
  if (! scope.empty ()) {

    static int consider_scope = -1;

    //  disable scoped debugging (e.g. DRC script lines) if $KLAYOUT_RBA_DEBUG_SCOPE is set.
    if (consider_scope < 0) {
      consider_scope = tl::app_flag ("rba-debug-scope") ? 0 : 1;
    }
    if (! consider_scope) {
      return 0;
    }

    for (size_t i = 0; i < bt.size (); ++i) {
      if (bt[i].file == scope) {
        return i;
      }
    }
  }
  return 0;
}

int
RubyStackTraceProvider::stack_depth () const
{
  //  NOTE: this implementation will provide an "internal stack depth".
  //  It's not exactly the same than the length of the stack_trace vector length.
  //  But the purpose is a relative compare, so efficiency is not sacrificed here
  //  for unnecessary consistency.
  int d = 1;
  static ID id_caller = rb_intern ("caller");
  VALUE backtrace = rb_funcall (rb_mKernel, id_caller, 0);
  if (TYPE (backtrace) == T_ARRAY) {
    d += RARRAY_LEN(backtrace);
  }
  return d;
}

//  we could use this for ruby >= 1.9.3
#if 0
static int
RubyStackTraceProvider::count_stack_levels(void *arg, VALUE file, int line, VALUE method)
{
  *(int *)arg += 1;
  return 0;
}

extern "C" int
RubyStackTraceProvider::rb_backtrace_each (int (*iter)(void *arg, VALUE file, int line, VALUE method), void *arg);

virtual int
RubyStackTraceProvider::stack_depth ()
{
  int l = 0;
  rb_backtrace_each(count_stack_levels, &l);
  return l;
}
#endif

// -------------------------------------------------------------------

static inline int
num_args (const gsi::MethodBase *m)
{
  return int (m->end_arguments () - m->begin_arguments ());
}

static VALUE
get_kwarg (const gsi::ArgType &atype, VALUE kwargs)
{
  if (kwargs != Qnil) {
    return rb_hash_lookup2 (kwargs, ID2SYM (rb_intern (atype.spec ()->name ().c_str ())), Qundef);
  } else {
    return Qundef;
  }
}

// -------------------------------------------------------------------
//  The lookup table for the method overload resolution

/**
 *  @brief A single entry in the method table
 *  This class provides an entry for one name. It provides flags
 *  (ctor, static, protected) for the method and a list of implementations
 *  (gsi::MethodBase objects)
 */
class MethodTableEntry
{
public:
  typedef std::vector<const gsi::MethodBase *>::const_iterator method_iterator;

  struct MethodVariantKey
  {
    MethodVariantKey (int argc, VALUE *argv, bool block_given, bool is_ctor, bool is_static, bool is_const)
      : m_block_given (block_given), m_is_ctor (is_ctor), m_is_static (is_static), m_is_const (is_const)
    {
      m_argtypes.reserve (size_t (argc));
      for (int i = 0; i < argc; ++i) {
        m_argtypes.push_back (CLASS_OF (argv[i]));
      }
    }

    bool operator== (const MethodVariantKey &other) const
    {
      return m_argtypes == other.m_argtypes &&
             m_block_given == other.m_block_given &&
             m_is_ctor == other.m_is_ctor &&
             m_is_static == other.m_is_static &&
             m_is_const == other.m_is_const;
    }

    bool operator< (const MethodVariantKey &other) const
    {
      if (m_argtypes != other.m_argtypes) {
        return m_argtypes < other.m_argtypes;
      }
      if (m_block_given != other.m_block_given) {
        return m_block_given < other.m_block_given;
      }
      if (m_is_ctor != other.m_is_ctor) {
        return m_is_ctor < other.m_is_ctor;
      }
      if (m_is_static != other.m_is_static) {
        return m_is_static < other.m_is_static;
      }
      if (m_is_const != other.m_is_const) {
        return m_is_const < other.m_is_const;
      }
      return false;
    }

  private:
    std::vector<size_t> m_argtypes;
    bool m_block_given;
    bool m_is_ctor;
    bool m_is_static;
    bool m_is_const;
  };

  MethodTableEntry (const std::string &name, bool ctor, bool st, bool prot, bool signal)
    : m_name (name), m_is_ctor (ctor), m_is_static (st), m_is_protected (prot), m_is_signal (signal)
  { }

  const std::string &name () const
  {
    return m_name;
  }

  bool is_ctor () const
  {
    return m_is_ctor;
  }

  bool is_signal () const
  {
    return m_is_signal;
  }

  bool is_static () const
  {
    return m_is_static;
  }

  bool is_protected () const
  {
    return m_is_protected;
  }

  void add (const gsi::MethodBase *m)
  {
    m_methods.push_back (m);
  }

  void finish ()
  {
    //  remove duplicate entries in the method list
    std::vector<const gsi::MethodBase *> m = m_methods;
    std::sort(m.begin (), m.end ());
    m_methods.assign (m.begin (), std::unique (m.begin (), m.end ()));
  }

  method_iterator begin () const
  {
    return m_methods.begin ();
  }

  method_iterator end () const
  {
    return m_methods.end ();
  }

  const gsi::MethodBase *get_variant (int argc, VALUE *argv, VALUE kwargs, bool block_given, bool is_ctor, bool is_static, bool is_const) const
  {
    //  caching can't work for arrays or hashes - in this case, give up

    bool nocache = (kwargs != Qnil);

    for (int i = 0; i < argc && ! nocache; ++i) {
      int t = TYPE (argv[i]);
      nocache = (t == T_ARRAY || t == T_HASH);
    }

    if (nocache) {
      return find_variant (argc, argv, kwargs, block_given, is_ctor, is_static, is_const);
    }

    //  try to find the variant in the cache

    MethodVariantKey key (argc, argv, block_given, is_ctor, is_static, is_const);
    std::map<MethodVariantKey, const gsi::MethodBase *>::const_iterator v = m_variants.find (key);
    if (v != m_variants.end ()) {
      return v->second;
    }

    const gsi::MethodBase *meth = find_variant (argc, argv, kwargs, block_given, is_ctor, is_static, is_const);
    m_variants[key] = meth;
    return meth;
  }

private:
  static bool
  compatible_with_args (const gsi::MethodBase *m, int argc, VALUE kwargs)
  {
    int nargs = num_args (m);

    if (argc >= nargs) {
      //  no more arguments to consider
      return argc == nargs && (kwargs == Qnil || RHASH_SIZE (kwargs) == 0);
    }

    if (kwargs != Qnil) {

      int nkwargs = int (RHASH_SIZE (kwargs));
      int kwargs_taken = 0;

      while (argc < nargs) {
        const gsi::ArgType &atype = m->begin_arguments () [argc];
        VALUE rb_arg = rb_hash_lookup2 (kwargs, ID2SYM (rb_intern (atype.spec ()->name ().c_str ())), Qnil);
        if (rb_arg == Qnil) {
          if (! atype.spec ()->has_default ()) {
            return false;
          }
        } else {
          ++kwargs_taken;
        }
        ++argc;
      }

      //  matches if all keyword arguments are taken
      return kwargs_taken == nkwargs;

    } else {

      while (argc < nargs) {
        const gsi::ArgType &atype = m->begin_arguments () [argc];
        if (! atype.spec ()->has_default ()) {
          return false;
        }
        ++argc;
      }

      return true;

    }
  }

  static std::string
  describe_overload (const gsi::MethodBase *m, int argc, VALUE kwargs)
  {
    std::string res = m->to_string ();
    if (compatible_with_args (m, argc, kwargs)) {
      res += " " + tl::to_string (tr ("[match candidate]"));
    }
    return res;
  }

  std::string
  describe_overloads (int argc, VALUE kwargs) const
  {
    std::string res;
    for (auto m = begin (); m != end (); ++m) {
      res += std::string ("  ") + describe_overload (*m, argc, kwargs) + "\n";
    }
    return res;
  }

  const gsi::MethodBase *find_variant (int argc, VALUE *argv, VALUE kwargs, bool block_given, bool is_ctor, bool is_static, bool is_const) const
  {
    //  get number of candidates by argument count
    const gsi::MethodBase *meth = 0;
    unsigned int candidates = 0;
    for (MethodTableEntry::method_iterator m = begin (); m != end (); ++m) {

      if ((*m)->is_signal ()) {

        if (block_given) {

          //  events do not have parameters, but accept a Proc object -> no overloading -> take this one.
          candidates = 1;
          meth = *m;
          break;

        } else if (argc <= 1 && (*m)->is_signal ()) {

          //  calling a signal without an argument will return the SignalHandler object with further
          //  options - with one argument it will reset the signal to this specific handler
          candidates = 1;
          meth = *m;
          break;

        } else {
          throw tl::Exception (tl::to_string (tr ("An event needs a block")));
        }

      } else if ((*m)->is_callback()) {

        //  ignore callbacks

      } else if (compatible_with_args (*m, argc, kwargs)) {

        ++candidates;
        meth = *m;

      }

    }

    //  no method found, but the ctor was requested - implement that method as replacement for the default "initialize"
    if (! meth && argc == 0 && is_ctor && kwargs == Qnil) {
      return 0;
    }

    //  no candidate -> error
    if (! meth) {

      std::set<unsigned int> nargs;
      for (MethodTableEntry::method_iterator m = begin (); m != end (); ++m) {
        if (! (*m)->is_callback ()) {
          nargs.insert (std::distance ((*m)->begin_arguments (), (*m)->end_arguments ()));
        }
      }

      std::string nargs_s;
      for (std::set<unsigned int>::const_iterator na = nargs.begin (); na != nargs.end (); ++na) {
        if (na != nargs.begin ()) {
          nargs_s += "/";
        }
        nargs_s += tl::to_string (*na);
      }

      throw tl::Exception (tl::to_string (tr ("Can't match arguments. Variants are:\n")) + describe_overloads (argc, kwargs));

    }

    //  more than one candidate -> refine by checking the arguments
    if (candidates > 1) {

      meth = 0;
      candidates = 0;
      int score = 0;
      bool const_matching = true;

      for (MethodTableEntry::method_iterator m = begin (); m != end (); ++m) {

        if (! (*m)->is_callback () && ! (*m)->is_signal ()) {

          //  check arguments (count and type)
          bool is_valid = compatible_with_args (*m, argc, kwargs);
          int sc = 0;
          int i = 0;
          for (gsi::MethodBase::argument_iterator a = (*m)->begin_arguments (); is_valid && a != (*m)->end_arguments (); ++a, ++i) {
            VALUE arg = i >= argc ? get_kwarg (*a, kwargs) : argv[i];
            if (arg == Qundef) {
              is_valid = a->spec ()->has_default ();
            } else if (test_arg (*a, arg, false /*strict*/)) {
              ++sc;
            } else if (test_arg (*a, arg, true /*loose*/)) {
              //  non-scoring match
            } else {
              is_valid = false;
            }
          }

          if (is_valid && ! is_static) {

            //  constness matching candidates have precedence
            if ((*m)->is_const () != is_const) {
              if (const_matching && candidates > 0) {
                is_valid = false;
              } else {
                const_matching = false;
              }
            } else if (! const_matching) {
              const_matching = true;
              candidates = 0;
            }

          }

          if (is_valid) {

            //  otherwise take the candidate with the better score or the least number of arguments (faster)
            if (candidates > 0) {
              if (sc > score || (sc == score && num_args (meth) > num_args (*m))) {
                candidates = 1;
                meth = *m;
                score = sc;
              } else if (sc == score && num_args (meth) == num_args (*m)) {
                ++candidates;
                meth = *m;
              }
            } else {
              ++candidates;
              meth = *m;
              score = sc;
            }

          }

        }

      }

    }

    if (! meth) {
      throw tl::Exception (tl::to_string (tr ("No overload with matching arguments. Variants are:\n")) + describe_overloads (argc, kwargs));
    }

    if (candidates > 1) {
      throw tl::Exception (tl::to_string (tr ("Ambiguous overload variants - multiple method declarations match arguments. Variants are:\n")) + describe_overloads (argc, kwargs));
    }

    if (is_const && ! meth->is_const ()) {
      throw tl::Exception (tl::to_string (tr ("Cannot call non-const method on a const reference")));
    }

    return meth;
  }

  std::string m_name;
  bool m_is_ctor : 1;
  bool m_is_static : 1;
  bool m_is_protected : 1;
  bool m_is_signal : 1;
  std::vector<const gsi::MethodBase *> m_methods;
  mutable std::map<MethodVariantKey, const gsi::MethodBase *> m_variants;
};

/**
 *  @brief The method table for a class
 *  The method table will provide the methods associated with a native method, i.e.
 *  a certain name. It only provides the methods, not a overload resolution strategy.
 */
class MethodTable
  : public gsi::PerClassClientSpecificData
{
public:
  /**
   *  @brief Returns the lowest method ID within the space of this table
   *  Method IDs below this one are reserved for base class methods
   */
  size_t bottom_mid () const
  {
    return m_method_offset;
  }

  /**
   *  @brief Returns the topmost + 1 method ID.
   */
  size_t top_mid () const
  {
    return m_method_offset + m_table.size ();
  }

  /**
   *  @brief Adds a method to the table 
   *  The ctor flag indicates that instance method to static call translation needs to
   *  be performed in order to implement "initialize" which basically is a method called
   *  on the already constructed object.
   */
  void add_method_generic (const std::string &name, const gsi::MethodBase *mb, bool ctor) 
  {
    bool st = mb->is_static ();

    std::map<std::pair<bool, std::string>, size_t>::iterator n = m_name_map.find (std::make_pair (st, name));
    if (n == m_name_map.end ()) {

      m_name_map.insert (std::make_pair (std::make_pair(st, name), m_table.size ()));
      m_table.push_back (MethodTableEntry (name, ctor, mb->is_static (), mb->is_protected (), mb->is_signal ()));
      m_table.back ().add (mb);

    } else {

      if (ctor && ! m_table [n->second].is_ctor ()) {
        tl::warn << "Class " << mp_cls_decl->name () << ": method '" << name << " is both a constructor and non-constructor";
      }
      if (m_table [n->second].is_protected () != mb->is_protected ()) {
        tl::warn << "Class " << mp_cls_decl->name () << ": method '" << name << " is both a protected and non-protected";
      }
      if (m_table [n->second].is_signal () != mb->is_signal ()) {
        tl::warn << "Class " << mp_cls_decl->name () << ": method '" << name << " is both a signal and non-signal";
      }
      if (m_table [n->second].is_signal () && mb->is_signal ()) {
        tl::warn << "Class " << mp_cls_decl->name () << ": method '" << name << " is a signal with ambiguous signature";
      }

      m_table [n->second].add (mb);

    }
  }

  /**
   *  @brief Convenience wrapper
   */
  void add_ctor_method (const std::string &name, const gsi::MethodBase *mb) 
  {
    add_method_generic (name, mb, true);
  }

  /**
   *  @brief Convenience wrapper
   */
  void add_method (const std::string &name, const gsi::MethodBase *mb) 
  {
    add_method_generic (name, mb, false);
  }

  /**
   *  @brief Returns true if the method with the given ID has "ctor" semantics (see above)
   */
  bool is_ctor (size_t mid) const
  {
    return m_table [mid - m_method_offset].is_ctor ();
  }

  /**
   *  @brief Returns true if the method with the given ID is a signal
   */
  bool is_signal (size_t mid) const
  {
    return m_table [mid - m_method_offset].is_signal ();
  }

  /**
   *  @brief Returns true if the method with the given ID is static
   */
  bool is_static (size_t mid) const
  {
    return m_table [mid - m_method_offset].is_static ();
  }

  /**
   *  @brief Returns true if the method with the given ID is protected
   */
  bool is_protected (size_t mid) const
  {
    return m_table [mid - m_method_offset].is_protected ();
  }

  /**
   *  @brief Returns the name of the method with the given ID
   */
  const std::string &name (size_t mid) const
  {
    return m_table [mid - m_method_offset].name ();
  }

  /**
   *  @brief Gets the method table entry for the given method
   */
  const MethodTableEntry &entry (size_t mid) const
  {
    return m_table[mid - m_method_offset];
  }

  /**
   *  @brief Finishes construction of the table
   *  This method must be called after the add_method calls have been used
   *  to fill the table. It will remove duplicate entries and clean up memory.
   */
  void finish () 
  {
    for (std::vector<MethodTableEntry>::iterator m = m_table.begin (); m != m_table.end (); ++m) {
      m->finish ();
    }
    m_name_map.clear ();
  }

  /**
   *  @brief Obtain a method table for a given class
   */
  static MethodTable *method_table_by_class (const gsi::ClassBase *cls_decl, bool force_init = false)
  {
    MethodTable *mt = dynamic_cast<MethodTable *>(cls_decl->data (gsi::ClientIndex::Ruby));
    if (! mt || force_init) {
      MethodTable *mtnc = new MethodTable (cls_decl);
      mt = mtnc;
      cls_decl->set_data (gsi::ClientIndex::Ruby, mtnc);
    }
    return mt;
  }

private:
  size_t m_method_offset;
  const gsi::ClassBase *mp_cls_decl;
  std::map<std::pair<bool, std::string>, size_t> m_name_map;
  std::vector<MethodTableEntry> m_table;

  /**
   *  @brief Constructor
   *  This constructor will create a method table for the given class and register
   *  this table under this class. 
   *  It is used by method_table_by_class only, hence it's private.
   */
  MethodTable (const gsi::ClassBase *cls_decl)
    : m_method_offset (0), mp_cls_decl (cls_decl)
  { 
    if (cls_decl->base ()) {
      const MethodTable *base_mt = method_table_by_class (cls_decl->base ());
      tl_assert (base_mt);
      m_method_offset = base_mt->top_mid ();
    }
  }
};

// -------------------------------------------------------------------
//  Ruby interpreter private data

struct RubyInterpreterPrivateData
{
  RubyInterpreterPrivateData ()
  {
    saved_stderr = Qnil;
    saved_stdout = Qnil;
    stdout_klass = Qnil;
    stderr_klass = Qnil;
    current_console = 0;
    current_exec_handler = 0;
    current_exec_level = 0;
    in_trace = false;
    exit_on_next = false;
    block_exceptions = false;
    ignore_next_exception = false;
  }

  VALUE saved_stderr;
  VALUE saved_stdout;
  VALUE stdout_klass;
  VALUE stderr_klass;
  gsi::Console *current_console;
  std::vector<gsi::Console *> consoles;
  gsi::ExecutionHandler *current_exec_handler;
  int current_exec_level;
  bool in_trace;
  bool exit_on_next;
  bool block_exceptions;
  bool ignore_next_exception;
  std::string debugger_scope;
  std::map<const char *, size_t> file_id_map;
  std::vector<gsi::ExecutionHandler *> exec_handlers;
  std::set<std::string> package_paths;
};

// -------------------------------------------------------------------
//  Ruby API 

static void
handle_exception (VALUE exc, bool first_chance)
{
  if (! first_chance) {
    //  Re-raise the exception without blocking in the debugger
    block_exceptions (true);
  }

  rb_exc_raise (exc);
}

static void
handle_exception (const std::string &where, std::exception &ex)
{
  VALUE error_msg = rb_str_new2 ((std::string(ex.what ()) + tl::to_string (tr (" in ")) + where).c_str ());
  VALUE args [1];
  args [0] = error_msg;
  VALUE exc = rb_class_new_instance(1, args, rb_eRuntimeError);
  handle_exception (exc, true);
}

static void
handle_exception (const std::string &where, tl::ExitException &ex)
{
  VALUE error_msg = rb_str_new2 ((ex.msg () + tl::to_string (tr (" in ")) + where).c_str ());
  VALUE args [2];
  args [0] = INT2NUM (ex.status ());
  args [1] = error_msg;
  VALUE exc = rb_class_new_instance (2, args, rb_eSystemExit);
  handle_exception (exc, ex.first_chance ());
}

static void
handle_exception (const std::string & /*where*/, rba::RubyError &ex)
{
  handle_exception (ex.exc (), ex.first_chance ());
}

static void
handle_exception (const std::string &where, tl::Exception &ex)
{
  VALUE error_msg = rb_str_new2 ((ex.msg () + tl::to_string (tr (" in ")) + where).c_str ()); \
  VALUE args [1];
  args [0] = error_msg;
  VALUE exc = rb_class_new_instance(1, args, rb_eRuntimeError);
  handle_exception (exc, ex.first_chance ());
}

static void
handle_exception (const std::string &where)
{
  VALUE error_msg = rb_str_new2 ((tl::to_string (tr ("Unspecific exception in ")) + where).c_str ()); \
  VALUE args [1];
  args [0] = error_msg;
  VALUE exc = rb_class_new_instance(1, args, rb_eRuntimeError);
  handle_exception (exc, true);
}

#define RBA_TRY \
  try {

#define RBA_CATCH(where) \
  } catch (std::exception &ex) { \
    handle_exception ((where), ex); \
  } catch (tl::ExitException &ex) { \
    handle_exception ((where), ex); \
  } catch (rba::RubyError &ex) { \
    handle_exception ((where), ex); \
  } catch (tl::Exception &ex) { \
    handle_exception ((where), ex); \
  } catch (...) { \
    handle_exception ((where)); \
  }

static VALUE
destroy (VALUE self)
{
  //  Destroy the object
  Proxy *p = 0;
  Data_Get_Struct (self, Proxy, p);
  p->destroy ();
  return Qnil;
}

static VALUE
keep (VALUE self)
{
  //  Makes the object kept by another instance
  Proxy *p = 0;
  Data_Get_Struct (self, Proxy, p);
  p->keep ();
  return Qnil;
}

static VALUE
release (VALUE self)
{
  //  Release any other ownership of the object
  Proxy *p = 0;
  Data_Get_Struct (self, Proxy, p);
  p->release ();
  return Qnil;
}

static VALUE
create (VALUE self)
{
  Proxy *p = 0;
  Data_Get_Struct (self, Proxy, p);
  //  this potentially instantiates the object if not done yet
  p->obj ();
  return self;
}

static VALUE
destroyed (VALUE self)
{
  //  return true if the object was destroyed already
  Proxy *p = 0;
  Data_Get_Struct (self, Proxy, p);
  return c2ruby<bool> (p->destroyed ());
}

static VALUE
is_const (VALUE self)
{
  //  return true if the object was destroyed already
  Proxy *p = 0;
  Data_Get_Struct (self, Proxy, p);
  return c2ruby<bool> (p->const_ref ());
}

static VALUE
assign (VALUE self, VALUE src)
{
  //  Compare if the classes are identical
  Proxy *p = 0;

  Data_Get_Struct (src, Proxy, p);
  const gsi::ClassBase *cls_decl_src = p->cls_decl ();
  void *obj_src = p->obj ();

  Data_Get_Struct (self, Proxy, p);
  const gsi::ClassBase *cls_decl_self = p->cls_decl ();
  void *obj_self = p->obj ();

  if (cls_decl_src != cls_decl_self) {
    throw tl::Exception (tl::to_string (tr ("Type is not identical on copy")));
  } 
  if (! cls_decl_self->can_copy ()) {
    throw tl::Exception (tl::to_string (tr ("No assignment provided for class '%s'")), cls_decl_self->name ());
  }
  cls_decl_self->assign (obj_self, obj_src);

  return self;
}

static VALUE
special_method_impl (const gsi::MethodBase *meth, int argc, VALUE *argv, VALUE self, bool ctor)
{
  gsi::MethodBase::special_method_type smt = meth->smt ();

  if (smt == gsi::MethodBase::DefaultCtor) {

    //  Must be called in the ctor context and does nothing since the object is 
    //  automatically default-created
    //  It is mapped to the non-static(!) initialize method.
    tl_assert (ctor);
    return Qnil;

  } else if (smt == gsi::MethodBase::Destroy) {
    tl_assert (!ctor);
    return destroy (self);
  } else if (smt == gsi::MethodBase::Keep) {
    tl_assert (!ctor);
    return keep (self);
  } else if (smt == gsi::MethodBase::Release) {
    tl_assert (!ctor);
    return release (self);
  } else if (smt == gsi::MethodBase::Create) {
    tl_assert (!ctor);
    return create (self);
  } else if (smt == gsi::MethodBase::IsConst) {
    tl_assert (!ctor);
    return is_const (self);
  } else if (smt == gsi::MethodBase::Destroyed) {
    tl_assert (!ctor);
    return destroyed (self);
  } else if (smt == gsi::MethodBase::Assign) {

    //  this is either assign or dup in disguise
    tl_assert (argc == 1);
    return assign (self, argv [0]);

  } else if (smt == gsi::MethodBase::Dup) {

    //  dup is disguised as assign in ctor context
    tl_assert (false); 

  } else {
    return Qnil;
  }
}

static void free_proxy (void *p)
{
  delete ((Proxy *) p);
}

static void mark_proxy (void *p)
{
  ((Proxy *) p)->mark ();
}

static VALUE alloc_proxy (VALUE klass)
{
  tl_assert (TYPE (klass) == T_CLASS);

  const gsi::ClassBase *cls = find_cclass (klass);
  Proxy *proxy = new Proxy (cls);
  VALUE self = Data_Wrap_Struct (klass, &mark_proxy, &free_proxy, proxy);
  proxy->set_self (self);
  return self;
}

/**
 *  @brief Gets the method name from a method id
 */
std::string 
method_name_from_id (int mid, VALUE self)
{
  const gsi::ClassBase *cls_decl;
  Proxy *p = 0;

  if (TYPE (self) == T_CLASS) {
    //  we have a static method
    cls_decl = find_cclass (self);
  } else {
    //  we have an instance method
    Data_Get_Struct (self, Proxy, p);
    cls_decl = p->cls_decl ();
  }

  const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
  tl_assert (mt);

  //  locate the method in the base classes method table if necessary
  while (mid < int (mt->bottom_mid ())) {

    tl_assert (cls_decl->base ());
    cls_decl = cls_decl->base ();
    mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

  }

  return cls_decl->name () + "::" + mt->name (mid);
}

static gsi::ArgType create_void_type ()
{
  gsi::ArgType at;
  at.init<void> ();
  return at;
}

static gsi::ArgType s_void_type = create_void_type ();

static int get_kwargs_keys (VALUE key, VALUE, VALUE arg)
{
  std::set<std::string> *names = reinterpret_cast<std::set<std::string> *> (arg);
  names->insert (ruby2c<std::string> (key));

  return ST_CONTINUE;
}

void
push_args (gsi::SerialArgs &arglist, const gsi::MethodBase *meth, VALUE *argv, int argc, VALUE kwargs, tl::Heap &heap)
{
  int iarg = 0;
  int kwargs_taken = 0;
  int nkwargs = kwargs == Qnil ? 0 : int (RHASH_SIZE (kwargs));

  try {

    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments (); ++a, ++iarg) {

      VALUE arg = iarg >= argc ? get_kwarg (*a, kwargs) : argv[iarg];
      if (arg == Qundef) {
        if (a->spec ()->has_default ()) {
          if (kwargs_taken == nkwargs) {
            //  leave it to the consumer to establish the default values (that is faster)
            break;
          }
          tl::Variant def_value = a->spec ()->default_value ();
          gsi::push_arg (arglist, *a, def_value, &heap);
        } else {
          throw tl::Exception (tl::to_string (tr ("No argument provided (positional or keyword) and no default value available")));
        }
      } else {
        if (iarg >= argc) {
          ++kwargs_taken;
        }
        push_arg (*a, arglist, arg, heap);
      }

    }

    if (kwargs_taken != nkwargs) {

      //  check if there are any left-over keyword parameters with unknown names

      std::set<std::string> invalid_names;
      rb_hash_foreach (kwargs, (int (*)(...)) &get_kwargs_keys, (VALUE) &invalid_names);

      for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments (); ++a) {
        invalid_names.erase (a->spec ()->name ());
      }

      if (invalid_names.size () > 1) {
        std::string names_str = tl::join (invalid_names.begin (), invalid_names.end (), ", ");
        throw tl::Exception (tl::to_string (tr ("Unknown keyword parameters: ")) + names_str);
      } else if (invalid_names.size () == 1) {
        throw tl::Exception (tl::to_string (tr ("Unknown keyword parameter: ")) + *invalid_names.begin ());
      }

    }

  } catch (tl::Exception &ex) {

    //  In case of an error upon write, pop the arguments to clean them up.
    //  Without this, there is a risk to keep dead objects on the stack.
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments () && arglist; ++a) {
      pull_arg (*a, 0, arglist, heap);
    }

    if (iarg < num_args (meth)) {

      const gsi::ArgSpecBase *arg_spec = meth->begin_arguments () [iarg].spec ();

      std::string msg;
      if (arg_spec && ! arg_spec->name ().empty ()) {
        msg = tl::sprintf (tl::to_string (tr ("%s for argument #%d ('%s')")), ex.basic_msg (), iarg + 1, arg_spec->name ());
      } else {
        msg = tl::sprintf (tl::to_string (tr ("%s for argument #%d")), ex.basic_msg (), iarg + 1);
      }

      tl::Exception new_ex (msg);
      new_ex.set_first_chance (ex.first_chance ());
      throw new_ex;

    } else {
      throw;
    }

  } catch (...) {

    //  In case of an error upon write, pop the arguments to clean them up.
    //  Without this, there is a risk to keep dead objects on the stack.
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments () && arglist; ++a) {
      pull_arg (*a, 0, arglist, heap);
    }

    throw;

  }
}

VALUE
method_adaptor (int mid, int argc, VALUE *argv, VALUE self, bool ctor)
{
  VALUE ret = Qnil;

  RBA_TRY

    tl::Heap heap;

    const gsi::ClassBase *cls_decl;
    Proxy *p = 0;

    //  this prevents side effects of callbacks raised from within the called functions -
    //  if these trigger the GC, self is protected from destruction herein.
    GCLocker gc_locker (self);

    if (TYPE (self) == T_CLASS) {
      //  we have a static method
      cls_decl = find_cclass (self);
    } else {
      //  we have an instance method
      Data_Get_Struct (self, Proxy, p);
      cls_decl = p->cls_decl ();
    }

    const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

    //  locate the method in the base classes method table if necessary
    while (mid < int (mt->bottom_mid ())) {

      tl_assert (cls_decl->base ());
      cls_decl = cls_decl->base ();
      mt = MethodTable::method_table_by_class (cls_decl);
      tl_assert (mt);

    }

    //  Check for keyword arguments ..

    VALUE kwargs = Qnil;
    bool check_last = true;
#if HAVE_RUBY_VERSION_CODE>=20700
    check_last = rb_keyword_given_p ();
#endif

    //  This is a heuristics to distinguish methods that are potential candidates for
    //  accepting a keyword argument. Problem is that Ruby confuses function calls with
    //  keyword arguments with arguments that take a single hash argument.
    //  We accept only methods here as candidates that do not have a last argument which
    //  is a map.
    //  For compatibility we do this check also for Ruby >=2.7 which supports rb_keyword_given_p.
    if (check_last) {
      const MethodTableEntry &e = mt->entry (mid);
      for (auto m = e.begin (); m != e.end () && check_last; ++m) {
        auto a = (*m)->end_arguments ();
        if (a != (*m)->begin_arguments () && (--a)->type () == gsi::T_map) {
          check_last = false;
        }
      }
    }

    if (check_last && argc > 0 && RB_TYPE_P (argv[argc - 1], T_HASH)) {
      kwargs = argv[--argc];
    }

    //  Identify the matching variant

    const gsi::MethodBase *meth = mt->entry (mid).get_variant (argc, argv, kwargs, rb_block_given_p (), ctor, p == 0, p != 0 && p->const_ref ());

    if (! meth) {

      //  no method found, but the ctor was requested - implement that method as replacement for the default "initialize"

    } else if (meth->smt () != gsi::MethodBase::None) {

      if (kwargs != Qnil && RHASH_SIZE (kwargs) > 0) {
        throw tl::Exception (tl::to_string (tr ("Keyword arguments not permitted")));
      }

      ret = special_method_impl (meth, argc, argv, self, ctor);

    } else if (meth->is_signal ()) {

      if (kwargs != Qnil && RHASH_SIZE (kwargs) > 0) {
        throw tl::Exception (tl::to_string (tr ("Keyword arguments not permitted on events")));
      }

      if (p) {

        static ID id_set = rb_intern ("set");

        VALUE signal_handler = p->signal_handler (meth);

        if (rb_block_given_p ()) {

          VALUE proc = rb_block_proc ();
          RB_GC_GUARD (proc);
          ret = rba_funcall2_checked (signal_handler, id_set, 1, &proc);

        } else if (argc > 0) {
          ret = rba_funcall2_checked (signal_handler, id_set, argc, argv);
        } else {
          ret = signal_handler;
        }

      }

    } else if (ctor && meth->ret_type ().type () == gsi::T_object && meth->ret_type ().pass_obj ()) {

      tl_assert (p != 0);

      //  This is a non-static constructor ("new" renamed to "initialize"): it does not create a 
      //  new Ruby object but just a new C++ object which replaces the old one.

      gsi::SerialArgs retlist (meth->retsize ());

      {
        gsi::SerialArgs arglist (meth->argsize ());
        push_args (arglist, meth, argv, argc, kwargs, heap);
        meth->call (0, arglist, retlist);
      }

      void *obj = retlist.read<void *> (heap);
      if (obj == 0) {
        p->reset ();
      } else {
        p->set (obj, true, false, true, self);
      }

    } else if (meth->ret_type ().is_iter () && ! rb_block_given_p ()) {

      //  calling an iterator method without block -> deliver an enumerator using "to_enum"

      if (kwargs != Qnil && RHASH_SIZE (kwargs) > 0) {
        throw tl::Exception (tl::to_string (tr ("Keyword arguments not permitted on enumerators")));
      }

      static ID id_to_enum = rb_intern ("to_enum");

      VALUE method_sym = ID2SYM (rb_intern (meth->primary_name ().c_str ()));

      if (argc == 0) {
        ret = rba_funcall2_checked (self, id_to_enum, 1, &method_sym);
      } else {
#if 0
	//  this solution does not work on MSVC2017 for unknown reasons and 
	//  makes the application segfault even without being called
        std::vector<VALUE> new_args;
        new_args.reserve (size_t (argc + 1));
        new_args.push_back (method_sym);
        new_args.insert (new_args.end (), argv, argv + argc);
        ret = rba_funcall2_checked (self, id_to_enum, argc + 1, new_args.begin ().operator-> ());
#else
	VALUE new_args[16];
	tl_assert (argc + 1 <= int (sizeof(new_args) / sizeof(new_args[0])));
	VALUE *a = &new_args[0];
	*a++ = method_sym;
	for (int i = 0; i < argc; ++i) {
	  *a++ = argv[i];
	}
        ret = rba_funcall2_checked (self, id_to_enum, argc + 1, &new_args[0]);
#endif
      }

    } else {

      void *obj = 0;
      if (p) {
        //  Hint: this potentially instantiates the object
        obj = p->obj ();
      }

      gsi::SerialArgs retlist (meth->retsize ());

      {
        gsi::SerialArgs arglist (meth->argsize ());
        push_args (arglist, meth, argv, argc, kwargs, heap);
        meth->call (obj, arglist, retlist);
      }

      if (meth->ret_type ().is_iter ()) {

        ret = Qnil;

        std::unique_ptr<gsi::IterAdaptorAbstractBase> iter ((gsi::IterAdaptorAbstractBase *) retlist.read<void *> (heap));
        if (iter.get ()) {

          try {

            gsi::SerialArgs rr (iter->serial_size ());
            while (! iter->at_end ()) {

              rr.reset ();
              iter->get (rr);

              VALUE value = pull_arg (meth->ret_type (), p, rr, heap);
              rba_yield_checked (value);

              iter->inc ();

            }

          } catch (tl::CancelException &) {
            //  break encountered
          }

        }

      } else if (meth->ret_type () == s_void_type) {

        //  simple, yet magical :)
        return self;

      } else {

        ret = pull_arg (meth->ret_type (), p, retlist, heap);

      }

    }

  RBA_CATCH(method_name_from_id (mid, self))

  return ret;
}

VALUE method_adaptor_n (int mid, int argc, VALUE *argv, VALUE self, bool ctor)
{
  return method_adaptor (mid, argc, argv, self, ctor);
}

template <int N> 
VALUE method_adaptor (int argc, VALUE *argv, VALUE self)
{
  return method_adaptor_n (N, argc, argv, self, false);
}

VALUE (*(method_adaptors [])) (int, VALUE *, VALUE) =
{
  &method_adaptor<0x000>, &method_adaptor<0x001>, &method_adaptor<0x002>, &method_adaptor<0x003>, &method_adaptor<0x004>, &method_adaptor<0x005>, &method_adaptor<0x006>, &method_adaptor<0x007>,
  &method_adaptor<0x008>, &method_adaptor<0x009>, &method_adaptor<0x00a>, &method_adaptor<0x00b>, &method_adaptor<0x00c>, &method_adaptor<0x00d>, &method_adaptor<0x00e>, &method_adaptor<0x00f>,
  &method_adaptor<0x010>, &method_adaptor<0x011>, &method_adaptor<0x012>, &method_adaptor<0x013>, &method_adaptor<0x014>, &method_adaptor<0x015>, &method_adaptor<0x016>, &method_adaptor<0x017>,
  &method_adaptor<0x018>, &method_adaptor<0x019>, &method_adaptor<0x01a>, &method_adaptor<0x01b>, &method_adaptor<0x01c>, &method_adaptor<0x01d>, &method_adaptor<0x01e>, &method_adaptor<0x01f>,
  &method_adaptor<0x020>, &method_adaptor<0x021>, &method_adaptor<0x022>, &method_adaptor<0x023>, &method_adaptor<0x024>, &method_adaptor<0x025>, &method_adaptor<0x026>, &method_adaptor<0x027>,
  &method_adaptor<0x028>, &method_adaptor<0x029>, &method_adaptor<0x02a>, &method_adaptor<0x02b>, &method_adaptor<0x02c>, &method_adaptor<0x02d>, &method_adaptor<0x02e>, &method_adaptor<0x02f>,
  &method_adaptor<0x030>, &method_adaptor<0x031>, &method_adaptor<0x032>, &method_adaptor<0x033>, &method_adaptor<0x034>, &method_adaptor<0x035>, &method_adaptor<0x036>, &method_adaptor<0x037>,
  &method_adaptor<0x038>, &method_adaptor<0x039>, &method_adaptor<0x03a>, &method_adaptor<0x03b>, &method_adaptor<0x03c>, &method_adaptor<0x03d>, &method_adaptor<0x03e>, &method_adaptor<0x03f>,
  &method_adaptor<0x040>, &method_adaptor<0x041>, &method_adaptor<0x042>, &method_adaptor<0x043>, &method_adaptor<0x044>, &method_adaptor<0x045>, &method_adaptor<0x046>, &method_adaptor<0x047>,
  &method_adaptor<0x048>, &method_adaptor<0x049>, &method_adaptor<0x04a>, &method_adaptor<0x04b>, &method_adaptor<0x04c>, &method_adaptor<0x04d>, &method_adaptor<0x04e>, &method_adaptor<0x04f>,
  &method_adaptor<0x050>, &method_adaptor<0x051>, &method_adaptor<0x052>, &method_adaptor<0x053>, &method_adaptor<0x054>, &method_adaptor<0x055>, &method_adaptor<0x056>, &method_adaptor<0x057>,
  &method_adaptor<0x058>, &method_adaptor<0x059>, &method_adaptor<0x05a>, &method_adaptor<0x05b>, &method_adaptor<0x05c>, &method_adaptor<0x05d>, &method_adaptor<0x05e>, &method_adaptor<0x05f>,
  &method_adaptor<0x060>, &method_adaptor<0x061>, &method_adaptor<0x062>, &method_adaptor<0x063>, &method_adaptor<0x064>, &method_adaptor<0x065>, &method_adaptor<0x066>, &method_adaptor<0x067>,
  &method_adaptor<0x068>, &method_adaptor<0x069>, &method_adaptor<0x06a>, &method_adaptor<0x06b>, &method_adaptor<0x06c>, &method_adaptor<0x06d>, &method_adaptor<0x06e>, &method_adaptor<0x06f>,
  &method_adaptor<0x070>, &method_adaptor<0x071>, &method_adaptor<0x072>, &method_adaptor<0x073>, &method_adaptor<0x074>, &method_adaptor<0x075>, &method_adaptor<0x076>, &method_adaptor<0x077>,
  &method_adaptor<0x078>, &method_adaptor<0x079>, &method_adaptor<0x07a>, &method_adaptor<0x07b>, &method_adaptor<0x07c>, &method_adaptor<0x07d>, &method_adaptor<0x07e>, &method_adaptor<0x07f>,
  &method_adaptor<0x080>, &method_adaptor<0x081>, &method_adaptor<0x082>, &method_adaptor<0x083>, &method_adaptor<0x084>, &method_adaptor<0x085>, &method_adaptor<0x086>, &method_adaptor<0x087>,
  &method_adaptor<0x088>, &method_adaptor<0x089>, &method_adaptor<0x08a>, &method_adaptor<0x08b>, &method_adaptor<0x08c>, &method_adaptor<0x08d>, &method_adaptor<0x08e>, &method_adaptor<0x08f>,
  &method_adaptor<0x090>, &method_adaptor<0x091>, &method_adaptor<0x092>, &method_adaptor<0x093>, &method_adaptor<0x094>, &method_adaptor<0x095>, &method_adaptor<0x096>, &method_adaptor<0x097>,
  &method_adaptor<0x098>, &method_adaptor<0x099>, &method_adaptor<0x09a>, &method_adaptor<0x09b>, &method_adaptor<0x09c>, &method_adaptor<0x09d>, &method_adaptor<0x09e>, &method_adaptor<0x09f>,
  &method_adaptor<0x0a0>, &method_adaptor<0x0a1>, &method_adaptor<0x0a2>, &method_adaptor<0x0a3>, &method_adaptor<0x0a4>, &method_adaptor<0x0a5>, &method_adaptor<0x0a6>, &method_adaptor<0x0a7>,
  &method_adaptor<0x0a8>, &method_adaptor<0x0a9>, &method_adaptor<0x0aa>, &method_adaptor<0x0ab>, &method_adaptor<0x0ac>, &method_adaptor<0x0ad>, &method_adaptor<0x0ae>, &method_adaptor<0x0af>,
  &method_adaptor<0x0b0>, &method_adaptor<0x0b1>, &method_adaptor<0x0b2>, &method_adaptor<0x0b3>, &method_adaptor<0x0b4>, &method_adaptor<0x0b5>, &method_adaptor<0x0b6>, &method_adaptor<0x0b7>,
  &method_adaptor<0x0b8>, &method_adaptor<0x0b9>, &method_adaptor<0x0ba>, &method_adaptor<0x0bb>, &method_adaptor<0x0bc>, &method_adaptor<0x0bd>, &method_adaptor<0x0be>, &method_adaptor<0x0bf>,
  &method_adaptor<0x0c0>, &method_adaptor<0x0c1>, &method_adaptor<0x0c2>, &method_adaptor<0x0c3>, &method_adaptor<0x0c4>, &method_adaptor<0x0c5>, &method_adaptor<0x0c6>, &method_adaptor<0x0c7>,
  &method_adaptor<0x0c8>, &method_adaptor<0x0c9>, &method_adaptor<0x0ca>, &method_adaptor<0x0cb>, &method_adaptor<0x0cc>, &method_adaptor<0x0cd>, &method_adaptor<0x0ce>, &method_adaptor<0x0cf>,
  &method_adaptor<0x0d0>, &method_adaptor<0x0d1>, &method_adaptor<0x0d2>, &method_adaptor<0x0d3>, &method_adaptor<0x0d4>, &method_adaptor<0x0d5>, &method_adaptor<0x0d6>, &method_adaptor<0x0d7>,
  &method_adaptor<0x0d8>, &method_adaptor<0x0d9>, &method_adaptor<0x0da>, &method_adaptor<0x0db>, &method_adaptor<0x0dc>, &method_adaptor<0x0dd>, &method_adaptor<0x0de>, &method_adaptor<0x0df>,
  &method_adaptor<0x0e0>, &method_adaptor<0x0e1>, &method_adaptor<0x0e2>, &method_adaptor<0x0e3>, &method_adaptor<0x0e4>, &method_adaptor<0x0e5>, &method_adaptor<0x0e6>, &method_adaptor<0x0e7>,
  &method_adaptor<0x0e8>, &method_adaptor<0x0e9>, &method_adaptor<0x0ea>, &method_adaptor<0x0eb>, &method_adaptor<0x0ec>, &method_adaptor<0x0ed>, &method_adaptor<0x0ee>, &method_adaptor<0x0ef>,
  &method_adaptor<0x0f0>, &method_adaptor<0x0f1>, &method_adaptor<0x0f2>, &method_adaptor<0x0f3>, &method_adaptor<0x0f4>, &method_adaptor<0x0f5>, &method_adaptor<0x0f6>, &method_adaptor<0x0f7>,
  &method_adaptor<0x0f8>, &method_adaptor<0x0f9>, &method_adaptor<0x0fa>, &method_adaptor<0x0fb>, &method_adaptor<0x0fc>, &method_adaptor<0x0fd>, &method_adaptor<0x0fe>, &method_adaptor<0x0ff>,
  &method_adaptor<0x100>, &method_adaptor<0x101>, &method_adaptor<0x102>, &method_adaptor<0x103>, &method_adaptor<0x104>, &method_adaptor<0x105>, &method_adaptor<0x106>, &method_adaptor<0x107>,
  &method_adaptor<0x108>, &method_adaptor<0x109>, &method_adaptor<0x10a>, &method_adaptor<0x10b>, &method_adaptor<0x10c>, &method_adaptor<0x10d>, &method_adaptor<0x10e>, &method_adaptor<0x10f>,
  &method_adaptor<0x110>, &method_adaptor<0x111>, &method_adaptor<0x112>, &method_adaptor<0x113>, &method_adaptor<0x114>, &method_adaptor<0x115>, &method_adaptor<0x116>, &method_adaptor<0x117>,
  &method_adaptor<0x118>, &method_adaptor<0x119>, &method_adaptor<0x11a>, &method_adaptor<0x11b>, &method_adaptor<0x11c>, &method_adaptor<0x11d>, &method_adaptor<0x11e>, &method_adaptor<0x11f>,
  &method_adaptor<0x120>, &method_adaptor<0x121>, &method_adaptor<0x122>, &method_adaptor<0x123>, &method_adaptor<0x124>, &method_adaptor<0x125>, &method_adaptor<0x126>, &method_adaptor<0x127>,
  &method_adaptor<0x128>, &method_adaptor<0x129>, &method_adaptor<0x12a>, &method_adaptor<0x12b>, &method_adaptor<0x12c>, &method_adaptor<0x12d>, &method_adaptor<0x12e>, &method_adaptor<0x12f>,
  &method_adaptor<0x130>, &method_adaptor<0x131>, &method_adaptor<0x132>, &method_adaptor<0x133>, &method_adaptor<0x134>, &method_adaptor<0x135>, &method_adaptor<0x136>, &method_adaptor<0x137>,
  &method_adaptor<0x138>, &method_adaptor<0x139>, &method_adaptor<0x13a>, &method_adaptor<0x13b>, &method_adaptor<0x13c>, &method_adaptor<0x13d>, &method_adaptor<0x13e>, &method_adaptor<0x13f>,
  &method_adaptor<0x140>, &method_adaptor<0x141>, &method_adaptor<0x142>, &method_adaptor<0x143>, &method_adaptor<0x144>, &method_adaptor<0x145>, &method_adaptor<0x146>, &method_adaptor<0x147>,
  &method_adaptor<0x148>, &method_adaptor<0x149>, &method_adaptor<0x14a>, &method_adaptor<0x14b>, &method_adaptor<0x14c>, &method_adaptor<0x14d>, &method_adaptor<0x14e>, &method_adaptor<0x14f>,
  &method_adaptor<0x150>, &method_adaptor<0x151>, &method_adaptor<0x152>, &method_adaptor<0x153>, &method_adaptor<0x154>, &method_adaptor<0x155>, &method_adaptor<0x156>, &method_adaptor<0x157>,
  &method_adaptor<0x158>, &method_adaptor<0x159>, &method_adaptor<0x15a>, &method_adaptor<0x15b>, &method_adaptor<0x15c>, &method_adaptor<0x15d>, &method_adaptor<0x15e>, &method_adaptor<0x15f>,
  &method_adaptor<0x160>, &method_adaptor<0x161>, &method_adaptor<0x162>, &method_adaptor<0x163>, &method_adaptor<0x164>, &method_adaptor<0x165>, &method_adaptor<0x166>, &method_adaptor<0x167>,
  &method_adaptor<0x168>, &method_adaptor<0x169>, &method_adaptor<0x16a>, &method_adaptor<0x16b>, &method_adaptor<0x16c>, &method_adaptor<0x16d>, &method_adaptor<0x16e>, &method_adaptor<0x16f>,
  &method_adaptor<0x170>, &method_adaptor<0x171>, &method_adaptor<0x172>, &method_adaptor<0x173>, &method_adaptor<0x174>, &method_adaptor<0x175>, &method_adaptor<0x176>, &method_adaptor<0x177>,
  &method_adaptor<0x178>, &method_adaptor<0x179>, &method_adaptor<0x17a>, &method_adaptor<0x17b>, &method_adaptor<0x17c>, &method_adaptor<0x17d>, &method_adaptor<0x17e>, &method_adaptor<0x17f>,
  &method_adaptor<0x180>, &method_adaptor<0x181>, &method_adaptor<0x182>, &method_adaptor<0x183>, &method_adaptor<0x184>, &method_adaptor<0x185>, &method_adaptor<0x186>, &method_adaptor<0x187>,
  &method_adaptor<0x188>, &method_adaptor<0x189>, &method_adaptor<0x18a>, &method_adaptor<0x18b>, &method_adaptor<0x18c>, &method_adaptor<0x18d>, &method_adaptor<0x18e>, &method_adaptor<0x18f>,
  &method_adaptor<0x190>, &method_adaptor<0x191>, &method_adaptor<0x192>, &method_adaptor<0x193>, &method_adaptor<0x194>, &method_adaptor<0x195>, &method_adaptor<0x196>, &method_adaptor<0x197>,
  &method_adaptor<0x198>, &method_adaptor<0x199>, &method_adaptor<0x19a>, &method_adaptor<0x19b>, &method_adaptor<0x19c>, &method_adaptor<0x19d>, &method_adaptor<0x19e>, &method_adaptor<0x19f>,
  &method_adaptor<0x1a0>, &method_adaptor<0x1a1>, &method_adaptor<0x1a2>, &method_adaptor<0x1a3>, &method_adaptor<0x1a4>, &method_adaptor<0x1a5>, &method_adaptor<0x1a6>, &method_adaptor<0x1a7>,
  &method_adaptor<0x1a8>, &method_adaptor<0x1a9>, &method_adaptor<0x1aa>, &method_adaptor<0x1ab>, &method_adaptor<0x1ac>, &method_adaptor<0x1ad>, &method_adaptor<0x1ae>, &method_adaptor<0x1af>,
  &method_adaptor<0x1b0>, &method_adaptor<0x1b1>, &method_adaptor<0x1b2>, &method_adaptor<0x1b3>, &method_adaptor<0x1b4>, &method_adaptor<0x1b5>, &method_adaptor<0x1b6>, &method_adaptor<0x1b7>,
  &method_adaptor<0x1b8>, &method_adaptor<0x1b9>, &method_adaptor<0x1ba>, &method_adaptor<0x1bb>, &method_adaptor<0x1bc>, &method_adaptor<0x1bd>, &method_adaptor<0x1be>, &method_adaptor<0x1bf>,
  &method_adaptor<0x1c0>, &method_adaptor<0x1c1>, &method_adaptor<0x1c2>, &method_adaptor<0x1c3>, &method_adaptor<0x1c4>, &method_adaptor<0x1c5>, &method_adaptor<0x1c6>, &method_adaptor<0x1c7>,
  &method_adaptor<0x1c8>, &method_adaptor<0x1c9>, &method_adaptor<0x1ca>, &method_adaptor<0x1cb>, &method_adaptor<0x1cc>, &method_adaptor<0x1cd>, &method_adaptor<0x1ce>, &method_adaptor<0x1cf>,
  &method_adaptor<0x1d0>, &method_adaptor<0x1d1>, &method_adaptor<0x1d2>, &method_adaptor<0x1d3>, &method_adaptor<0x1d4>, &method_adaptor<0x1d5>, &method_adaptor<0x1d6>, &method_adaptor<0x1d7>,
  &method_adaptor<0x1d8>, &method_adaptor<0x1d9>, &method_adaptor<0x1da>, &method_adaptor<0x1db>, &method_adaptor<0x1dc>, &method_adaptor<0x1dd>, &method_adaptor<0x1de>, &method_adaptor<0x1df>,
  &method_adaptor<0x1e0>, &method_adaptor<0x1e1>, &method_adaptor<0x1e2>, &method_adaptor<0x1e3>, &method_adaptor<0x1e4>, &method_adaptor<0x1e5>, &method_adaptor<0x1e6>, &method_adaptor<0x1e7>,
  &method_adaptor<0x1e8>, &method_adaptor<0x1e9>, &method_adaptor<0x1ea>, &method_adaptor<0x1eb>, &method_adaptor<0x1ec>, &method_adaptor<0x1ed>, &method_adaptor<0x1ee>, &method_adaptor<0x1ef>,
  &method_adaptor<0x1f0>, &method_adaptor<0x1f1>, &method_adaptor<0x1f2>, &method_adaptor<0x1f3>, &method_adaptor<0x1f4>, &method_adaptor<0x1f5>, &method_adaptor<0x1f6>, &method_adaptor<0x1f7>,
  &method_adaptor<0x1f8>, &method_adaptor<0x1f9>, &method_adaptor<0x1fa>, &method_adaptor<0x1fb>, &method_adaptor<0x1fc>, &method_adaptor<0x1fd>, &method_adaptor<0x1fe>, &method_adaptor<0x1ff>,
  &method_adaptor<0x200>, &method_adaptor<0x201>, &method_adaptor<0x202>, &method_adaptor<0x203>, &method_adaptor<0x204>, &method_adaptor<0x205>, &method_adaptor<0x206>, &method_adaptor<0x207>,
  &method_adaptor<0x208>, &method_adaptor<0x209>, &method_adaptor<0x20a>, &method_adaptor<0x20b>, &method_adaptor<0x20c>, &method_adaptor<0x20d>, &method_adaptor<0x20e>, &method_adaptor<0x20f>,
  &method_adaptor<0x210>, &method_adaptor<0x211>, &method_adaptor<0x212>, &method_adaptor<0x213>, &method_adaptor<0x214>, &method_adaptor<0x215>, &method_adaptor<0x216>, &method_adaptor<0x217>,
  &method_adaptor<0x218>, &method_adaptor<0x219>, &method_adaptor<0x21a>, &method_adaptor<0x21b>, &method_adaptor<0x21c>, &method_adaptor<0x21d>, &method_adaptor<0x21e>, &method_adaptor<0x21f>,
  &method_adaptor<0x220>, &method_adaptor<0x221>, &method_adaptor<0x222>, &method_adaptor<0x223>, &method_adaptor<0x224>, &method_adaptor<0x225>, &method_adaptor<0x226>, &method_adaptor<0x227>,
  &method_adaptor<0x228>, &method_adaptor<0x229>, &method_adaptor<0x22a>, &method_adaptor<0x22b>, &method_adaptor<0x22c>, &method_adaptor<0x22d>, &method_adaptor<0x22e>, &method_adaptor<0x22f>,
  &method_adaptor<0x230>, &method_adaptor<0x231>, &method_adaptor<0x232>, &method_adaptor<0x233>, &method_adaptor<0x234>, &method_adaptor<0x235>, &method_adaptor<0x236>, &method_adaptor<0x237>,
  &method_adaptor<0x238>, &method_adaptor<0x239>, &method_adaptor<0x23a>, &method_adaptor<0x23b>, &method_adaptor<0x23c>, &method_adaptor<0x23d>, &method_adaptor<0x23e>, &method_adaptor<0x23f>,
  &method_adaptor<0x240>, &method_adaptor<0x241>, &method_adaptor<0x242>, &method_adaptor<0x243>, &method_adaptor<0x244>, &method_adaptor<0x245>, &method_adaptor<0x246>, &method_adaptor<0x247>,
  &method_adaptor<0x248>, &method_adaptor<0x249>, &method_adaptor<0x24a>, &method_adaptor<0x24b>, &method_adaptor<0x24c>, &method_adaptor<0x24d>, &method_adaptor<0x24e>, &method_adaptor<0x24f>,
  &method_adaptor<0x250>, &method_adaptor<0x251>, &method_adaptor<0x252>, &method_adaptor<0x253>, &method_adaptor<0x254>, &method_adaptor<0x255>, &method_adaptor<0x256>, &method_adaptor<0x257>,
  &method_adaptor<0x258>, &method_adaptor<0x259>, &method_adaptor<0x25a>, &method_adaptor<0x25b>, &method_adaptor<0x25c>, &method_adaptor<0x25d>, &method_adaptor<0x25e>, &method_adaptor<0x25f>,
  &method_adaptor<0x260>, &method_adaptor<0x261>, &method_adaptor<0x262>, &method_adaptor<0x263>, &method_adaptor<0x264>, &method_adaptor<0x265>, &method_adaptor<0x266>, &method_adaptor<0x267>,
  &method_adaptor<0x268>, &method_adaptor<0x269>, &method_adaptor<0x26a>, &method_adaptor<0x26b>, &method_adaptor<0x26c>, &method_adaptor<0x26d>, &method_adaptor<0x26e>, &method_adaptor<0x26f>,
  &method_adaptor<0x270>, &method_adaptor<0x271>, &method_adaptor<0x272>, &method_adaptor<0x273>, &method_adaptor<0x274>, &method_adaptor<0x275>, &method_adaptor<0x276>, &method_adaptor<0x277>,
  &method_adaptor<0x278>, &method_adaptor<0x279>, &method_adaptor<0x27a>, &method_adaptor<0x27b>, &method_adaptor<0x27c>, &method_adaptor<0x27d>, &method_adaptor<0x27e>, &method_adaptor<0x27f>,
  &method_adaptor<0x280>, &method_adaptor<0x281>, &method_adaptor<0x282>, &method_adaptor<0x283>, &method_adaptor<0x284>, &method_adaptor<0x285>, &method_adaptor<0x286>, &method_adaptor<0x287>,
  &method_adaptor<0x288>, &method_adaptor<0x289>, &method_adaptor<0x28a>, &method_adaptor<0x28b>, &method_adaptor<0x28c>, &method_adaptor<0x28d>, &method_adaptor<0x28e>, &method_adaptor<0x28f>,
  &method_adaptor<0x290>, &method_adaptor<0x291>, &method_adaptor<0x292>, &method_adaptor<0x293>, &method_adaptor<0x294>, &method_adaptor<0x295>, &method_adaptor<0x296>, &method_adaptor<0x297>,
  &method_adaptor<0x298>, &method_adaptor<0x299>, &method_adaptor<0x29a>, &method_adaptor<0x29b>, &method_adaptor<0x29c>, &method_adaptor<0x29d>, &method_adaptor<0x29e>, &method_adaptor<0x29f>,
  &method_adaptor<0x2a0>, &method_adaptor<0x2a1>, &method_adaptor<0x2a2>, &method_adaptor<0x2a3>, &method_adaptor<0x2a4>, &method_adaptor<0x2a5>, &method_adaptor<0x2a6>, &method_adaptor<0x2a7>,
  &method_adaptor<0x2a8>, &method_adaptor<0x2a9>, &method_adaptor<0x2aa>, &method_adaptor<0x2ab>, &method_adaptor<0x2ac>, &method_adaptor<0x2ad>, &method_adaptor<0x2ae>, &method_adaptor<0x2af>,
  &method_adaptor<0x2b0>, &method_adaptor<0x2b1>, &method_adaptor<0x2b2>, &method_adaptor<0x2b3>, &method_adaptor<0x2b4>, &method_adaptor<0x2b5>, &method_adaptor<0x2b6>, &method_adaptor<0x2b7>,
  &method_adaptor<0x2b8>, &method_adaptor<0x2b9>, &method_adaptor<0x2ba>, &method_adaptor<0x2bb>, &method_adaptor<0x2bc>, &method_adaptor<0x2bd>, &method_adaptor<0x2be>, &method_adaptor<0x2bf>,
  &method_adaptor<0x2c0>, &method_adaptor<0x2c1>, &method_adaptor<0x2c2>, &method_adaptor<0x2c3>, &method_adaptor<0x2c4>, &method_adaptor<0x2c5>, &method_adaptor<0x2c6>, &method_adaptor<0x2c7>,
  &method_adaptor<0x2c8>, &method_adaptor<0x2c9>, &method_adaptor<0x2ca>, &method_adaptor<0x2cb>, &method_adaptor<0x2cc>, &method_adaptor<0x2cd>, &method_adaptor<0x2ce>, &method_adaptor<0x2cf>,
  &method_adaptor<0x2d0>, &method_adaptor<0x2d1>, &method_adaptor<0x2d2>, &method_adaptor<0x2d3>, &method_adaptor<0x2d4>, &method_adaptor<0x2d5>, &method_adaptor<0x2d6>, &method_adaptor<0x2d7>,
  &method_adaptor<0x2d8>, &method_adaptor<0x2d9>, &method_adaptor<0x2da>, &method_adaptor<0x2db>, &method_adaptor<0x2dc>, &method_adaptor<0x2dd>, &method_adaptor<0x2de>, &method_adaptor<0x2df>,
  &method_adaptor<0x2e0>, &method_adaptor<0x2e1>, &method_adaptor<0x2e2>, &method_adaptor<0x2e3>, &method_adaptor<0x2e4>, &method_adaptor<0x2e5>, &method_adaptor<0x2e6>, &method_adaptor<0x2e7>,
  &method_adaptor<0x2e8>, &method_adaptor<0x2e9>, &method_adaptor<0x2ea>, &method_adaptor<0x2eb>, &method_adaptor<0x2ec>, &method_adaptor<0x2ed>, &method_adaptor<0x2ee>, &method_adaptor<0x2ef>,
  &method_adaptor<0x2f0>, &method_adaptor<0x2f1>, &method_adaptor<0x2f2>, &method_adaptor<0x2f3>, &method_adaptor<0x2f4>, &method_adaptor<0x2f5>, &method_adaptor<0x2f6>, &method_adaptor<0x2f7>,
  &method_adaptor<0x2f8>, &method_adaptor<0x2f9>, &method_adaptor<0x2fa>, &method_adaptor<0x2fb>, &method_adaptor<0x2fc>, &method_adaptor<0x2fd>, &method_adaptor<0x2fe>, &method_adaptor<0x2ff>,
  &method_adaptor<0x300>, &method_adaptor<0x301>, &method_adaptor<0x302>, &method_adaptor<0x303>, &method_adaptor<0x304>, &method_adaptor<0x305>, &method_adaptor<0x306>, &method_adaptor<0x307>,
  &method_adaptor<0x308>, &method_adaptor<0x309>, &method_adaptor<0x30a>, &method_adaptor<0x30b>, &method_adaptor<0x30c>, &method_adaptor<0x30d>, &method_adaptor<0x30e>, &method_adaptor<0x30f>,
  &method_adaptor<0x310>, &method_adaptor<0x311>, &method_adaptor<0x312>, &method_adaptor<0x313>, &method_adaptor<0x314>, &method_adaptor<0x315>, &method_adaptor<0x316>, &method_adaptor<0x317>,
  &method_adaptor<0x318>, &method_adaptor<0x319>, &method_adaptor<0x31a>, &method_adaptor<0x31b>, &method_adaptor<0x31c>, &method_adaptor<0x31d>, &method_adaptor<0x31e>, &method_adaptor<0x31f>,
  &method_adaptor<0x320>, &method_adaptor<0x321>, &method_adaptor<0x322>, &method_adaptor<0x323>, &method_adaptor<0x324>, &method_adaptor<0x325>, &method_adaptor<0x326>, &method_adaptor<0x327>,
  &method_adaptor<0x328>, &method_adaptor<0x329>, &method_adaptor<0x32a>, &method_adaptor<0x32b>, &method_adaptor<0x32c>, &method_adaptor<0x32d>, &method_adaptor<0x32e>, &method_adaptor<0x32f>,
  &method_adaptor<0x330>, &method_adaptor<0x331>, &method_adaptor<0x332>, &method_adaptor<0x333>, &method_adaptor<0x334>, &method_adaptor<0x335>, &method_adaptor<0x336>, &method_adaptor<0x337>,
  &method_adaptor<0x338>, &method_adaptor<0x339>, &method_adaptor<0x33a>, &method_adaptor<0x33b>, &method_adaptor<0x33c>, &method_adaptor<0x33d>, &method_adaptor<0x33e>, &method_adaptor<0x33f>,
  &method_adaptor<0x340>, &method_adaptor<0x341>, &method_adaptor<0x342>, &method_adaptor<0x343>, &method_adaptor<0x344>, &method_adaptor<0x345>, &method_adaptor<0x346>, &method_adaptor<0x347>,
  &method_adaptor<0x348>, &method_adaptor<0x349>, &method_adaptor<0x34a>, &method_adaptor<0x34b>, &method_adaptor<0x34c>, &method_adaptor<0x34d>, &method_adaptor<0x34e>, &method_adaptor<0x34f>,
  &method_adaptor<0x350>, &method_adaptor<0x351>, &method_adaptor<0x352>, &method_adaptor<0x353>, &method_adaptor<0x354>, &method_adaptor<0x355>, &method_adaptor<0x356>, &method_adaptor<0x357>,
  &method_adaptor<0x358>, &method_adaptor<0x359>, &method_adaptor<0x35a>, &method_adaptor<0x35b>, &method_adaptor<0x35c>, &method_adaptor<0x35d>, &method_adaptor<0x35e>, &method_adaptor<0x35f>,
  &method_adaptor<0x360>, &method_adaptor<0x361>, &method_adaptor<0x362>, &method_adaptor<0x363>, &method_adaptor<0x364>, &method_adaptor<0x365>, &method_adaptor<0x366>, &method_adaptor<0x367>,
  &method_adaptor<0x368>, &method_adaptor<0x369>, &method_adaptor<0x36a>, &method_adaptor<0x36b>, &method_adaptor<0x36c>, &method_adaptor<0x36d>, &method_adaptor<0x36e>, &method_adaptor<0x36f>,
  &method_adaptor<0x370>, &method_adaptor<0x371>, &method_adaptor<0x372>, &method_adaptor<0x373>, &method_adaptor<0x374>, &method_adaptor<0x375>, &method_adaptor<0x376>, &method_adaptor<0x377>,
  &method_adaptor<0x378>, &method_adaptor<0x379>, &method_adaptor<0x37a>, &method_adaptor<0x37b>, &method_adaptor<0x37c>, &method_adaptor<0x37d>, &method_adaptor<0x37e>, &method_adaptor<0x37f>,
  &method_adaptor<0x380>, &method_adaptor<0x381>, &method_adaptor<0x382>, &method_adaptor<0x383>, &method_adaptor<0x384>, &method_adaptor<0x385>, &method_adaptor<0x386>, &method_adaptor<0x387>,
  &method_adaptor<0x388>, &method_adaptor<0x389>, &method_adaptor<0x38a>, &method_adaptor<0x38b>, &method_adaptor<0x38c>, &method_adaptor<0x38d>, &method_adaptor<0x38e>, &method_adaptor<0x38f>,
  &method_adaptor<0x390>, &method_adaptor<0x391>, &method_adaptor<0x392>, &method_adaptor<0x393>, &method_adaptor<0x394>, &method_adaptor<0x395>, &method_adaptor<0x396>, &method_adaptor<0x397>,
  &method_adaptor<0x398>, &method_adaptor<0x399>, &method_adaptor<0x39a>, &method_adaptor<0x39b>, &method_adaptor<0x39c>, &method_adaptor<0x39d>, &method_adaptor<0x39e>, &method_adaptor<0x39f>,
  &method_adaptor<0x3a0>, &method_adaptor<0x3a1>, &method_adaptor<0x3a2>, &method_adaptor<0x3a3>, &method_adaptor<0x3a4>, &method_adaptor<0x3a5>, &method_adaptor<0x3a6>, &method_adaptor<0x3a7>,
  &method_adaptor<0x3a8>, &method_adaptor<0x3a9>, &method_adaptor<0x3aa>, &method_adaptor<0x3ab>, &method_adaptor<0x3ac>, &method_adaptor<0x3ad>, &method_adaptor<0x3ae>, &method_adaptor<0x3af>,
  &method_adaptor<0x3b0>, &method_adaptor<0x3b1>, &method_adaptor<0x3b2>, &method_adaptor<0x3b3>, &method_adaptor<0x3b4>, &method_adaptor<0x3b5>, &method_adaptor<0x3b6>, &method_adaptor<0x3b7>,
  &method_adaptor<0x3b8>, &method_adaptor<0x3b9>, &method_adaptor<0x3ba>, &method_adaptor<0x3bb>, &method_adaptor<0x3bc>, &method_adaptor<0x3bd>, &method_adaptor<0x3be>, &method_adaptor<0x3bf>,
  &method_adaptor<0x3c0>, &method_adaptor<0x3c1>, &method_adaptor<0x3c2>, &method_adaptor<0x3c3>, &method_adaptor<0x3c4>, &method_adaptor<0x3c5>, &method_adaptor<0x3c6>, &method_adaptor<0x3c7>,
  &method_adaptor<0x3c8>, &method_adaptor<0x3c9>, &method_adaptor<0x3ca>, &method_adaptor<0x3cb>, &method_adaptor<0x3cc>, &method_adaptor<0x3cd>, &method_adaptor<0x3ce>, &method_adaptor<0x3cf>,
  &method_adaptor<0x3d0>, &method_adaptor<0x3d1>, &method_adaptor<0x3d2>, &method_adaptor<0x3d3>, &method_adaptor<0x3d4>, &method_adaptor<0x3d5>, &method_adaptor<0x3d6>, &method_adaptor<0x3d7>,
  &method_adaptor<0x3d8>, &method_adaptor<0x3d9>, &method_adaptor<0x3da>, &method_adaptor<0x3db>, &method_adaptor<0x3dc>, &method_adaptor<0x3dd>, &method_adaptor<0x3de>, &method_adaptor<0x3df>,
  &method_adaptor<0x3e0>, &method_adaptor<0x3e1>, &method_adaptor<0x3e2>, &method_adaptor<0x3e3>, &method_adaptor<0x3e4>, &method_adaptor<0x3e5>, &method_adaptor<0x3e6>, &method_adaptor<0x3e7>,
  &method_adaptor<0x3e8>, &method_adaptor<0x3e9>, &method_adaptor<0x3ea>, &method_adaptor<0x3eb>, &method_adaptor<0x3ec>, &method_adaptor<0x3ed>, &method_adaptor<0x3ee>, &method_adaptor<0x3ef>,
  &method_adaptor<0x3f0>, &method_adaptor<0x3f1>, &method_adaptor<0x3f2>, &method_adaptor<0x3f3>, &method_adaptor<0x3f4>, &method_adaptor<0x3f5>, &method_adaptor<0x3f6>, &method_adaptor<0x3f7>,
  &method_adaptor<0x3f8>, &method_adaptor<0x3f9>, &method_adaptor<0x3fa>, &method_adaptor<0x3fb>, &method_adaptor<0x3fc>, &method_adaptor<0x3fd>, &method_adaptor<0x3fe>, &method_adaptor<0x3ff>,
};

//  zero-arguments method adaptors

template <int N> 
VALUE method_adaptor_ctor (int argc, VALUE *argv, VALUE self)
{
  return method_adaptor_n (N, argc, argv, self, true);
}

VALUE (*(method_adaptors_ctor [])) (int, VALUE *, VALUE) =
{
  &method_adaptor_ctor<0x000>, &method_adaptor_ctor<0x001>, &method_adaptor_ctor<0x002>, &method_adaptor_ctor<0x003>, &method_adaptor_ctor<0x004>, &method_adaptor_ctor<0x005>, &method_adaptor_ctor<0x006>, &method_adaptor_ctor<0x007>,
  &method_adaptor_ctor<0x008>, &method_adaptor_ctor<0x009>, &method_adaptor_ctor<0x00a>, &method_adaptor_ctor<0x00b>, &method_adaptor_ctor<0x00c>, &method_adaptor_ctor<0x00d>, &method_adaptor_ctor<0x00e>, &method_adaptor_ctor<0x00f>,
  &method_adaptor_ctor<0x010>, &method_adaptor_ctor<0x011>, &method_adaptor_ctor<0x012>, &method_adaptor_ctor<0x013>, &method_adaptor_ctor<0x014>, &method_adaptor_ctor<0x015>, &method_adaptor_ctor<0x016>, &method_adaptor_ctor<0x017>,
  &method_adaptor_ctor<0x018>, &method_adaptor_ctor<0x019>, &method_adaptor_ctor<0x01a>, &method_adaptor_ctor<0x01b>, &method_adaptor_ctor<0x01c>, &method_adaptor_ctor<0x01d>, &method_adaptor_ctor<0x01e>, &method_adaptor_ctor<0x01f>,
  &method_adaptor_ctor<0x020>, &method_adaptor_ctor<0x021>, &method_adaptor_ctor<0x022>, &method_adaptor_ctor<0x023>, &method_adaptor_ctor<0x024>, &method_adaptor_ctor<0x025>, &method_adaptor_ctor<0x026>, &method_adaptor_ctor<0x027>,
  &method_adaptor_ctor<0x028>, &method_adaptor_ctor<0x029>, &method_adaptor_ctor<0x02a>, &method_adaptor_ctor<0x02b>, &method_adaptor_ctor<0x02c>, &method_adaptor_ctor<0x02d>, &method_adaptor_ctor<0x02e>, &method_adaptor_ctor<0x02f>,
  &method_adaptor_ctor<0x030>, &method_adaptor_ctor<0x031>, &method_adaptor_ctor<0x032>, &method_adaptor_ctor<0x033>, &method_adaptor_ctor<0x034>, &method_adaptor_ctor<0x035>, &method_adaptor_ctor<0x036>, &method_adaptor_ctor<0x037>,
  &method_adaptor_ctor<0x038>, &method_adaptor_ctor<0x039>, &method_adaptor_ctor<0x03a>, &method_adaptor_ctor<0x03b>, &method_adaptor_ctor<0x03c>, &method_adaptor_ctor<0x03d>, &method_adaptor_ctor<0x03e>, &method_adaptor_ctor<0x03f>,
  &method_adaptor_ctor<0x040>, &method_adaptor_ctor<0x041>, &method_adaptor_ctor<0x042>, &method_adaptor_ctor<0x043>, &method_adaptor_ctor<0x044>, &method_adaptor_ctor<0x045>, &method_adaptor_ctor<0x046>, &method_adaptor_ctor<0x047>,
  &method_adaptor_ctor<0x048>, &method_adaptor_ctor<0x049>, &method_adaptor_ctor<0x04a>, &method_adaptor_ctor<0x04b>, &method_adaptor_ctor<0x04c>, &method_adaptor_ctor<0x04d>, &method_adaptor_ctor<0x04e>, &method_adaptor_ctor<0x04f>,
  &method_adaptor_ctor<0x050>, &method_adaptor_ctor<0x051>, &method_adaptor_ctor<0x052>, &method_adaptor_ctor<0x053>, &method_adaptor_ctor<0x054>, &method_adaptor_ctor<0x055>, &method_adaptor_ctor<0x056>, &method_adaptor_ctor<0x057>,
  &method_adaptor_ctor<0x058>, &method_adaptor_ctor<0x059>, &method_adaptor_ctor<0x05a>, &method_adaptor_ctor<0x05b>, &method_adaptor_ctor<0x05c>, &method_adaptor_ctor<0x05d>, &method_adaptor_ctor<0x05e>, &method_adaptor_ctor<0x05f>,
  &method_adaptor_ctor<0x060>, &method_adaptor_ctor<0x061>, &method_adaptor_ctor<0x062>, &method_adaptor_ctor<0x063>, &method_adaptor_ctor<0x064>, &method_adaptor_ctor<0x065>, &method_adaptor_ctor<0x066>, &method_adaptor_ctor<0x067>,
  &method_adaptor_ctor<0x068>, &method_adaptor_ctor<0x069>, &method_adaptor_ctor<0x06a>, &method_adaptor_ctor<0x06b>, &method_adaptor_ctor<0x06c>, &method_adaptor_ctor<0x06d>, &method_adaptor_ctor<0x06e>, &method_adaptor_ctor<0x06f>,
  &method_adaptor_ctor<0x070>, &method_adaptor_ctor<0x071>, &method_adaptor_ctor<0x072>, &method_adaptor_ctor<0x073>, &method_adaptor_ctor<0x074>, &method_adaptor_ctor<0x075>, &method_adaptor_ctor<0x076>, &method_adaptor_ctor<0x077>,
  &method_adaptor_ctor<0x078>, &method_adaptor_ctor<0x079>, &method_adaptor_ctor<0x07a>, &method_adaptor_ctor<0x07b>, &method_adaptor_ctor<0x07c>, &method_adaptor_ctor<0x07d>, &method_adaptor_ctor<0x07e>, &method_adaptor_ctor<0x07f>,
  &method_adaptor_ctor<0x080>, &method_adaptor_ctor<0x081>, &method_adaptor_ctor<0x082>, &method_adaptor_ctor<0x083>, &method_adaptor_ctor<0x084>, &method_adaptor_ctor<0x085>, &method_adaptor_ctor<0x086>, &method_adaptor_ctor<0x087>,
  &method_adaptor_ctor<0x088>, &method_adaptor_ctor<0x089>, &method_adaptor_ctor<0x08a>, &method_adaptor_ctor<0x08b>, &method_adaptor_ctor<0x08c>, &method_adaptor_ctor<0x08d>, &method_adaptor_ctor<0x08e>, &method_adaptor_ctor<0x08f>,
  &method_adaptor_ctor<0x090>, &method_adaptor_ctor<0x091>, &method_adaptor_ctor<0x092>, &method_adaptor_ctor<0x093>, &method_adaptor_ctor<0x094>, &method_adaptor_ctor<0x095>, &method_adaptor_ctor<0x096>, &method_adaptor_ctor<0x097>,
  &method_adaptor_ctor<0x098>, &method_adaptor_ctor<0x099>, &method_adaptor_ctor<0x09a>, &method_adaptor_ctor<0x09b>, &method_adaptor_ctor<0x09c>, &method_adaptor_ctor<0x09d>, &method_adaptor_ctor<0x09e>, &method_adaptor_ctor<0x09f>,
  &method_adaptor_ctor<0x0a0>, &method_adaptor_ctor<0x0a1>, &method_adaptor_ctor<0x0a2>, &method_adaptor_ctor<0x0a3>, &method_adaptor_ctor<0x0a4>, &method_adaptor_ctor<0x0a5>, &method_adaptor_ctor<0x0a6>, &method_adaptor_ctor<0x0a7>,
  &method_adaptor_ctor<0x0a8>, &method_adaptor_ctor<0x0a9>, &method_adaptor_ctor<0x0aa>, &method_adaptor_ctor<0x0ab>, &method_adaptor_ctor<0x0ac>, &method_adaptor_ctor<0x0ad>, &method_adaptor_ctor<0x0ae>, &method_adaptor_ctor<0x0af>,
  &method_adaptor_ctor<0x0b0>, &method_adaptor_ctor<0x0b1>, &method_adaptor_ctor<0x0b2>, &method_adaptor_ctor<0x0b3>, &method_adaptor_ctor<0x0b4>, &method_adaptor_ctor<0x0b5>, &method_adaptor_ctor<0x0b6>, &method_adaptor_ctor<0x0b7>,
  &method_adaptor_ctor<0x0b8>, &method_adaptor_ctor<0x0b9>, &method_adaptor_ctor<0x0ba>, &method_adaptor_ctor<0x0bb>, &method_adaptor_ctor<0x0bc>, &method_adaptor_ctor<0x0bd>, &method_adaptor_ctor<0x0be>, &method_adaptor_ctor<0x0bf>,
  &method_adaptor_ctor<0x0c0>, &method_adaptor_ctor<0x0c1>, &method_adaptor_ctor<0x0c2>, &method_adaptor_ctor<0x0c3>, &method_adaptor_ctor<0x0c4>, &method_adaptor_ctor<0x0c5>, &method_adaptor_ctor<0x0c6>, &method_adaptor_ctor<0x0c7>,
  &method_adaptor_ctor<0x0c8>, &method_adaptor_ctor<0x0c9>, &method_adaptor_ctor<0x0ca>, &method_adaptor_ctor<0x0cb>, &method_adaptor_ctor<0x0cc>, &method_adaptor_ctor<0x0cd>, &method_adaptor_ctor<0x0ce>, &method_adaptor_ctor<0x0cf>,
  &method_adaptor_ctor<0x0d0>, &method_adaptor_ctor<0x0d1>, &method_adaptor_ctor<0x0d2>, &method_adaptor_ctor<0x0d3>, &method_adaptor_ctor<0x0d4>, &method_adaptor_ctor<0x0d5>, &method_adaptor_ctor<0x0d6>, &method_adaptor_ctor<0x0d7>,
  &method_adaptor_ctor<0x0d8>, &method_adaptor_ctor<0x0d9>, &method_adaptor_ctor<0x0da>, &method_adaptor_ctor<0x0db>, &method_adaptor_ctor<0x0dc>, &method_adaptor_ctor<0x0dd>, &method_adaptor_ctor<0x0de>, &method_adaptor_ctor<0x0df>,
  &method_adaptor_ctor<0x0e0>, &method_adaptor_ctor<0x0e1>, &method_adaptor_ctor<0x0e2>, &method_adaptor_ctor<0x0e3>, &method_adaptor_ctor<0x0e4>, &method_adaptor_ctor<0x0e5>, &method_adaptor_ctor<0x0e6>, &method_adaptor_ctor<0x0e7>,
  &method_adaptor_ctor<0x0e8>, &method_adaptor_ctor<0x0e9>, &method_adaptor_ctor<0x0ea>, &method_adaptor_ctor<0x0eb>, &method_adaptor_ctor<0x0ec>, &method_adaptor_ctor<0x0ed>, &method_adaptor_ctor<0x0ee>, &method_adaptor_ctor<0x0ef>,
  &method_adaptor_ctor<0x0f0>, &method_adaptor_ctor<0x0f1>, &method_adaptor_ctor<0x0f2>, &method_adaptor_ctor<0x0f3>, &method_adaptor_ctor<0x0f4>, &method_adaptor_ctor<0x0f5>, &method_adaptor_ctor<0x0f6>, &method_adaptor_ctor<0x0f7>,
  &method_adaptor_ctor<0x0f8>, &method_adaptor_ctor<0x0f9>, &method_adaptor_ctor<0x0fa>, &method_adaptor_ctor<0x0fb>, &method_adaptor_ctor<0x0fc>, &method_adaptor_ctor<0x0fd>, &method_adaptor_ctor<0x0fe>, &method_adaptor_ctor<0x0ff>,
  &method_adaptor_ctor<0x100>, &method_adaptor_ctor<0x101>, &method_adaptor_ctor<0x102>, &method_adaptor_ctor<0x103>, &method_adaptor_ctor<0x104>, &method_adaptor_ctor<0x105>, &method_adaptor_ctor<0x106>, &method_adaptor_ctor<0x107>,
  &method_adaptor_ctor<0x108>, &method_adaptor_ctor<0x109>, &method_adaptor_ctor<0x10a>, &method_adaptor_ctor<0x10b>, &method_adaptor_ctor<0x10c>, &method_adaptor_ctor<0x10d>, &method_adaptor_ctor<0x10e>, &method_adaptor_ctor<0x10f>,
  &method_adaptor_ctor<0x110>, &method_adaptor_ctor<0x111>, &method_adaptor_ctor<0x112>, &method_adaptor_ctor<0x113>, &method_adaptor_ctor<0x114>, &method_adaptor_ctor<0x115>, &method_adaptor_ctor<0x116>, &method_adaptor_ctor<0x117>,
  &method_adaptor_ctor<0x118>, &method_adaptor_ctor<0x119>, &method_adaptor_ctor<0x11a>, &method_adaptor_ctor<0x11b>, &method_adaptor_ctor<0x11c>, &method_adaptor_ctor<0x11d>, &method_adaptor_ctor<0x11e>, &method_adaptor_ctor<0x11f>,
  &method_adaptor_ctor<0x120>, &method_adaptor_ctor<0x121>, &method_adaptor_ctor<0x122>, &method_adaptor_ctor<0x123>, &method_adaptor_ctor<0x124>, &method_adaptor_ctor<0x125>, &method_adaptor_ctor<0x126>, &method_adaptor_ctor<0x127>,
  &method_adaptor_ctor<0x128>, &method_adaptor_ctor<0x129>, &method_adaptor_ctor<0x12a>, &method_adaptor_ctor<0x12b>, &method_adaptor_ctor<0x12c>, &method_adaptor_ctor<0x12d>, &method_adaptor_ctor<0x12e>, &method_adaptor_ctor<0x12f>,
  &method_adaptor_ctor<0x130>, &method_adaptor_ctor<0x131>, &method_adaptor_ctor<0x132>, &method_adaptor_ctor<0x133>, &method_adaptor_ctor<0x134>, &method_adaptor_ctor<0x135>, &method_adaptor_ctor<0x136>, &method_adaptor_ctor<0x137>,
  &method_adaptor_ctor<0x138>, &method_adaptor_ctor<0x139>, &method_adaptor_ctor<0x13a>, &method_adaptor_ctor<0x13b>, &method_adaptor_ctor<0x13c>, &method_adaptor_ctor<0x13d>, &method_adaptor_ctor<0x13e>, &method_adaptor_ctor<0x13f>,
  &method_adaptor_ctor<0x140>, &method_adaptor_ctor<0x141>, &method_adaptor_ctor<0x142>, &method_adaptor_ctor<0x143>, &method_adaptor_ctor<0x144>, &method_adaptor_ctor<0x145>, &method_adaptor_ctor<0x146>, &method_adaptor_ctor<0x147>,
  &method_adaptor_ctor<0x148>, &method_adaptor_ctor<0x149>, &method_adaptor_ctor<0x14a>, &method_adaptor_ctor<0x14b>, &method_adaptor_ctor<0x14c>, &method_adaptor_ctor<0x14d>, &method_adaptor_ctor<0x14e>, &method_adaptor_ctor<0x14f>,
  &method_adaptor_ctor<0x150>, &method_adaptor_ctor<0x151>, &method_adaptor_ctor<0x152>, &method_adaptor_ctor<0x153>, &method_adaptor_ctor<0x154>, &method_adaptor_ctor<0x155>, &method_adaptor_ctor<0x156>, &method_adaptor_ctor<0x157>,
  &method_adaptor_ctor<0x158>, &method_adaptor_ctor<0x159>, &method_adaptor_ctor<0x15a>, &method_adaptor_ctor<0x15b>, &method_adaptor_ctor<0x15c>, &method_adaptor_ctor<0x15d>, &method_adaptor_ctor<0x15e>, &method_adaptor_ctor<0x15f>,
  &method_adaptor_ctor<0x160>, &method_adaptor_ctor<0x161>, &method_adaptor_ctor<0x162>, &method_adaptor_ctor<0x163>, &method_adaptor_ctor<0x164>, &method_adaptor_ctor<0x165>, &method_adaptor_ctor<0x166>, &method_adaptor_ctor<0x167>,
  &method_adaptor_ctor<0x168>, &method_adaptor_ctor<0x169>, &method_adaptor_ctor<0x16a>, &method_adaptor_ctor<0x16b>, &method_adaptor_ctor<0x16c>, &method_adaptor_ctor<0x16d>, &method_adaptor_ctor<0x16e>, &method_adaptor_ctor<0x16f>,
  &method_adaptor_ctor<0x170>, &method_adaptor_ctor<0x171>, &method_adaptor_ctor<0x172>, &method_adaptor_ctor<0x173>, &method_adaptor_ctor<0x174>, &method_adaptor_ctor<0x175>, &method_adaptor_ctor<0x176>, &method_adaptor_ctor<0x177>,
  &method_adaptor_ctor<0x178>, &method_adaptor_ctor<0x179>, &method_adaptor_ctor<0x17a>, &method_adaptor_ctor<0x17b>, &method_adaptor_ctor<0x17c>, &method_adaptor_ctor<0x17d>, &method_adaptor_ctor<0x17e>, &method_adaptor_ctor<0x17f>,
  &method_adaptor_ctor<0x180>, &method_adaptor_ctor<0x181>, &method_adaptor_ctor<0x182>, &method_adaptor_ctor<0x183>, &method_adaptor_ctor<0x184>, &method_adaptor_ctor<0x185>, &method_adaptor_ctor<0x186>, &method_adaptor_ctor<0x187>,
  &method_adaptor_ctor<0x188>, &method_adaptor_ctor<0x189>, &method_adaptor_ctor<0x18a>, &method_adaptor_ctor<0x18b>, &method_adaptor_ctor<0x18c>, &method_adaptor_ctor<0x18d>, &method_adaptor_ctor<0x18e>, &method_adaptor_ctor<0x18f>,
  &method_adaptor_ctor<0x190>, &method_adaptor_ctor<0x191>, &method_adaptor_ctor<0x192>, &method_adaptor_ctor<0x193>, &method_adaptor_ctor<0x194>, &method_adaptor_ctor<0x195>, &method_adaptor_ctor<0x196>, &method_adaptor_ctor<0x197>,
  &method_adaptor_ctor<0x198>, &method_adaptor_ctor<0x199>, &method_adaptor_ctor<0x19a>, &method_adaptor_ctor<0x19b>, &method_adaptor_ctor<0x19c>, &method_adaptor_ctor<0x19d>, &method_adaptor_ctor<0x19e>, &method_adaptor_ctor<0x19f>,
  &method_adaptor_ctor<0x1a0>, &method_adaptor_ctor<0x1a1>, &method_adaptor_ctor<0x1a2>, &method_adaptor_ctor<0x1a3>, &method_adaptor_ctor<0x1a4>, &method_adaptor_ctor<0x1a5>, &method_adaptor_ctor<0x1a6>, &method_adaptor_ctor<0x1a7>,
  &method_adaptor_ctor<0x1a8>, &method_adaptor_ctor<0x1a9>, &method_adaptor_ctor<0x1aa>, &method_adaptor_ctor<0x1ab>, &method_adaptor_ctor<0x1ac>, &method_adaptor_ctor<0x1ad>, &method_adaptor_ctor<0x1ae>, &method_adaptor_ctor<0x1af>,
  &method_adaptor_ctor<0x1b0>, &method_adaptor_ctor<0x1b1>, &method_adaptor_ctor<0x1b2>, &method_adaptor_ctor<0x1b3>, &method_adaptor_ctor<0x1b4>, &method_adaptor_ctor<0x1b5>, &method_adaptor_ctor<0x1b6>, &method_adaptor_ctor<0x1b7>,
  &method_adaptor_ctor<0x1b8>, &method_adaptor_ctor<0x1b9>, &method_adaptor_ctor<0x1ba>, &method_adaptor_ctor<0x1bb>, &method_adaptor_ctor<0x1bc>, &method_adaptor_ctor<0x1bd>, &method_adaptor_ctor<0x1be>, &method_adaptor_ctor<0x1bf>,
  &method_adaptor_ctor<0x1c0>, &method_adaptor_ctor<0x1c1>, &method_adaptor_ctor<0x1c2>, &method_adaptor_ctor<0x1c3>, &method_adaptor_ctor<0x1c4>, &method_adaptor_ctor<0x1c5>, &method_adaptor_ctor<0x1c6>, &method_adaptor_ctor<0x1c7>,
  &method_adaptor_ctor<0x1c8>, &method_adaptor_ctor<0x1c9>, &method_adaptor_ctor<0x1ca>, &method_adaptor_ctor<0x1cb>, &method_adaptor_ctor<0x1cc>, &method_adaptor_ctor<0x1cd>, &method_adaptor_ctor<0x1ce>, &method_adaptor_ctor<0x1cf>,
  &method_adaptor_ctor<0x1d0>, &method_adaptor_ctor<0x1d1>, &method_adaptor_ctor<0x1d2>, &method_adaptor_ctor<0x1d3>, &method_adaptor_ctor<0x1d4>, &method_adaptor_ctor<0x1d5>, &method_adaptor_ctor<0x1d6>, &method_adaptor_ctor<0x1d7>,
  &method_adaptor_ctor<0x1d8>, &method_adaptor_ctor<0x1d9>, &method_adaptor_ctor<0x1da>, &method_adaptor_ctor<0x1db>, &method_adaptor_ctor<0x1dc>, &method_adaptor_ctor<0x1dd>, &method_adaptor_ctor<0x1de>, &method_adaptor_ctor<0x1df>,
  &method_adaptor_ctor<0x1e0>, &method_adaptor_ctor<0x1e1>, &method_adaptor_ctor<0x1e2>, &method_adaptor_ctor<0x1e3>, &method_adaptor_ctor<0x1e4>, &method_adaptor_ctor<0x1e5>, &method_adaptor_ctor<0x1e6>, &method_adaptor_ctor<0x1e7>,
  &method_adaptor_ctor<0x1e8>, &method_adaptor_ctor<0x1e9>, &method_adaptor_ctor<0x1ea>, &method_adaptor_ctor<0x1eb>, &method_adaptor_ctor<0x1ec>, &method_adaptor_ctor<0x1ed>, &method_adaptor_ctor<0x1ee>, &method_adaptor_ctor<0x1ef>,
  &method_adaptor_ctor<0x1f0>, &method_adaptor_ctor<0x1f1>, &method_adaptor_ctor<0x1f2>, &method_adaptor_ctor<0x1f3>, &method_adaptor_ctor<0x1f4>, &method_adaptor_ctor<0x1f5>, &method_adaptor_ctor<0x1f6>, &method_adaptor_ctor<0x1f7>,
  &method_adaptor_ctor<0x1f8>, &method_adaptor_ctor<0x1f9>, &method_adaptor_ctor<0x1fa>, &method_adaptor_ctor<0x1fb>, &method_adaptor_ctor<0x1fc>, &method_adaptor_ctor<0x1fd>, &method_adaptor_ctor<0x1fe>, &method_adaptor_ctor<0x1ff>,
  &method_adaptor_ctor<0x200>, &method_adaptor_ctor<0x201>, &method_adaptor_ctor<0x202>, &method_adaptor_ctor<0x203>, &method_adaptor_ctor<0x204>, &method_adaptor_ctor<0x205>, &method_adaptor_ctor<0x206>, &method_adaptor_ctor<0x207>,
  &method_adaptor_ctor<0x208>, &method_adaptor_ctor<0x209>, &method_adaptor_ctor<0x20a>, &method_adaptor_ctor<0x20b>, &method_adaptor_ctor<0x20c>, &method_adaptor_ctor<0x20d>, &method_adaptor_ctor<0x20e>, &method_adaptor_ctor<0x20f>,
  &method_adaptor_ctor<0x210>, &method_adaptor_ctor<0x211>, &method_adaptor_ctor<0x212>, &method_adaptor_ctor<0x213>, &method_adaptor_ctor<0x214>, &method_adaptor_ctor<0x215>, &method_adaptor_ctor<0x216>, &method_adaptor_ctor<0x217>,
  &method_adaptor_ctor<0x218>, &method_adaptor_ctor<0x219>, &method_adaptor_ctor<0x21a>, &method_adaptor_ctor<0x21b>, &method_adaptor_ctor<0x21c>, &method_adaptor_ctor<0x21d>, &method_adaptor_ctor<0x21e>, &method_adaptor_ctor<0x21f>,
  &method_adaptor_ctor<0x220>, &method_adaptor_ctor<0x221>, &method_adaptor_ctor<0x222>, &method_adaptor_ctor<0x223>, &method_adaptor_ctor<0x224>, &method_adaptor_ctor<0x225>, &method_adaptor_ctor<0x226>, &method_adaptor_ctor<0x227>,
  &method_adaptor_ctor<0x228>, &method_adaptor_ctor<0x229>, &method_adaptor_ctor<0x22a>, &method_adaptor_ctor<0x22b>, &method_adaptor_ctor<0x22c>, &method_adaptor_ctor<0x22d>, &method_adaptor_ctor<0x22e>, &method_adaptor_ctor<0x22f>,
  &method_adaptor_ctor<0x230>, &method_adaptor_ctor<0x231>, &method_adaptor_ctor<0x232>, &method_adaptor_ctor<0x233>, &method_adaptor_ctor<0x234>, &method_adaptor_ctor<0x235>, &method_adaptor_ctor<0x236>, &method_adaptor_ctor<0x237>,
  &method_adaptor_ctor<0x238>, &method_adaptor_ctor<0x239>, &method_adaptor_ctor<0x23a>, &method_adaptor_ctor<0x23b>, &method_adaptor_ctor<0x23c>, &method_adaptor_ctor<0x23d>, &method_adaptor_ctor<0x23e>, &method_adaptor_ctor<0x23f>,
  &method_adaptor_ctor<0x240>, &method_adaptor_ctor<0x241>, &method_adaptor_ctor<0x242>, &method_adaptor_ctor<0x243>, &method_adaptor_ctor<0x244>, &method_adaptor_ctor<0x245>, &method_adaptor_ctor<0x246>, &method_adaptor_ctor<0x247>,
  &method_adaptor_ctor<0x248>, &method_adaptor_ctor<0x249>, &method_adaptor_ctor<0x24a>, &method_adaptor_ctor<0x24b>, &method_adaptor_ctor<0x24c>, &method_adaptor_ctor<0x24d>, &method_adaptor_ctor<0x24e>, &method_adaptor_ctor<0x24f>,
  &method_adaptor_ctor<0x250>, &method_adaptor_ctor<0x251>, &method_adaptor_ctor<0x252>, &method_adaptor_ctor<0x253>, &method_adaptor_ctor<0x254>, &method_adaptor_ctor<0x255>, &method_adaptor_ctor<0x256>, &method_adaptor_ctor<0x257>,
  &method_adaptor_ctor<0x258>, &method_adaptor_ctor<0x259>, &method_adaptor_ctor<0x25a>, &method_adaptor_ctor<0x25b>, &method_adaptor_ctor<0x25c>, &method_adaptor_ctor<0x25d>, &method_adaptor_ctor<0x25e>, &method_adaptor_ctor<0x25f>,
  &method_adaptor_ctor<0x260>, &method_adaptor_ctor<0x261>, &method_adaptor_ctor<0x262>, &method_adaptor_ctor<0x263>, &method_adaptor_ctor<0x264>, &method_adaptor_ctor<0x265>, &method_adaptor_ctor<0x266>, &method_adaptor_ctor<0x267>,
  &method_adaptor_ctor<0x268>, &method_adaptor_ctor<0x269>, &method_adaptor_ctor<0x26a>, &method_adaptor_ctor<0x26b>, &method_adaptor_ctor<0x26c>, &method_adaptor_ctor<0x26d>, &method_adaptor_ctor<0x26e>, &method_adaptor_ctor<0x26f>,
  &method_adaptor_ctor<0x270>, &method_adaptor_ctor<0x271>, &method_adaptor_ctor<0x272>, &method_adaptor_ctor<0x273>, &method_adaptor_ctor<0x274>, &method_adaptor_ctor<0x275>, &method_adaptor_ctor<0x276>, &method_adaptor_ctor<0x277>,
  &method_adaptor_ctor<0x278>, &method_adaptor_ctor<0x279>, &method_adaptor_ctor<0x27a>, &method_adaptor_ctor<0x27b>, &method_adaptor_ctor<0x27c>, &method_adaptor_ctor<0x27d>, &method_adaptor_ctor<0x27e>, &method_adaptor_ctor<0x27f>,
  &method_adaptor_ctor<0x280>, &method_adaptor_ctor<0x281>, &method_adaptor_ctor<0x282>, &method_adaptor_ctor<0x283>, &method_adaptor_ctor<0x284>, &method_adaptor_ctor<0x285>, &method_adaptor_ctor<0x286>, &method_adaptor_ctor<0x287>,
  &method_adaptor_ctor<0x288>, &method_adaptor_ctor<0x289>, &method_adaptor_ctor<0x28a>, &method_adaptor_ctor<0x28b>, &method_adaptor_ctor<0x28c>, &method_adaptor_ctor<0x28d>, &method_adaptor_ctor<0x28e>, &method_adaptor_ctor<0x28f>,
  &method_adaptor_ctor<0x290>, &method_adaptor_ctor<0x291>, &method_adaptor_ctor<0x292>, &method_adaptor_ctor<0x293>, &method_adaptor_ctor<0x294>, &method_adaptor_ctor<0x295>, &method_adaptor_ctor<0x296>, &method_adaptor_ctor<0x297>,
  &method_adaptor_ctor<0x298>, &method_adaptor_ctor<0x299>, &method_adaptor_ctor<0x29a>, &method_adaptor_ctor<0x29b>, &method_adaptor_ctor<0x29c>, &method_adaptor_ctor<0x29d>, &method_adaptor_ctor<0x29e>, &method_adaptor_ctor<0x29f>,
  &method_adaptor_ctor<0x2a0>, &method_adaptor_ctor<0x2a1>, &method_adaptor_ctor<0x2a2>, &method_adaptor_ctor<0x2a3>, &method_adaptor_ctor<0x2a4>, &method_adaptor_ctor<0x2a5>, &method_adaptor_ctor<0x2a6>, &method_adaptor_ctor<0x2a7>,
  &method_adaptor_ctor<0x2a8>, &method_adaptor_ctor<0x2a9>, &method_adaptor_ctor<0x2aa>, &method_adaptor_ctor<0x2ab>, &method_adaptor_ctor<0x2ac>, &method_adaptor_ctor<0x2ad>, &method_adaptor_ctor<0x2ae>, &method_adaptor_ctor<0x2af>,
  &method_adaptor_ctor<0x2b0>, &method_adaptor_ctor<0x2b1>, &method_adaptor_ctor<0x2b2>, &method_adaptor_ctor<0x2b3>, &method_adaptor_ctor<0x2b4>, &method_adaptor_ctor<0x2b5>, &method_adaptor_ctor<0x2b6>, &method_adaptor_ctor<0x2b7>,
  &method_adaptor_ctor<0x2b8>, &method_adaptor_ctor<0x2b9>, &method_adaptor_ctor<0x2ba>, &method_adaptor_ctor<0x2bb>, &method_adaptor_ctor<0x2bc>, &method_adaptor_ctor<0x2bd>, &method_adaptor_ctor<0x2be>, &method_adaptor_ctor<0x2bf>,
  &method_adaptor_ctor<0x2c0>, &method_adaptor_ctor<0x2c1>, &method_adaptor_ctor<0x2c2>, &method_adaptor_ctor<0x2c3>, &method_adaptor_ctor<0x2c4>, &method_adaptor_ctor<0x2c5>, &method_adaptor_ctor<0x2c6>, &method_adaptor_ctor<0x2c7>,
  &method_adaptor_ctor<0x2c8>, &method_adaptor_ctor<0x2c9>, &method_adaptor_ctor<0x2ca>, &method_adaptor_ctor<0x2cb>, &method_adaptor_ctor<0x2cc>, &method_adaptor_ctor<0x2cd>, &method_adaptor_ctor<0x2ce>, &method_adaptor_ctor<0x2cf>,
  &method_adaptor_ctor<0x2d0>, &method_adaptor_ctor<0x2d1>, &method_adaptor_ctor<0x2d2>, &method_adaptor_ctor<0x2d3>, &method_adaptor_ctor<0x2d4>, &method_adaptor_ctor<0x2d5>, &method_adaptor_ctor<0x2d6>, &method_adaptor_ctor<0x2d7>,
  &method_adaptor_ctor<0x2d8>, &method_adaptor_ctor<0x2d9>, &method_adaptor_ctor<0x2da>, &method_adaptor_ctor<0x2db>, &method_adaptor_ctor<0x2dc>, &method_adaptor_ctor<0x2dd>, &method_adaptor_ctor<0x2de>, &method_adaptor_ctor<0x2df>,
  &method_adaptor_ctor<0x2e0>, &method_adaptor_ctor<0x2e1>, &method_adaptor_ctor<0x2e2>, &method_adaptor_ctor<0x2e3>, &method_adaptor_ctor<0x2e4>, &method_adaptor_ctor<0x2e5>, &method_adaptor_ctor<0x2e6>, &method_adaptor_ctor<0x2e7>,
  &method_adaptor_ctor<0x2e8>, &method_adaptor_ctor<0x2e9>, &method_adaptor_ctor<0x2ea>, &method_adaptor_ctor<0x2eb>, &method_adaptor_ctor<0x2ec>, &method_adaptor_ctor<0x2ed>, &method_adaptor_ctor<0x2ee>, &method_adaptor_ctor<0x2ef>,
  &method_adaptor_ctor<0x2f0>, &method_adaptor_ctor<0x2f1>, &method_adaptor_ctor<0x2f2>, &method_adaptor_ctor<0x2f3>, &method_adaptor_ctor<0x2f4>, &method_adaptor_ctor<0x2f5>, &method_adaptor_ctor<0x2f6>, &method_adaptor_ctor<0x2f7>,
  &method_adaptor_ctor<0x2f8>, &method_adaptor_ctor<0x2f9>, &method_adaptor_ctor<0x2fa>, &method_adaptor_ctor<0x2fb>, &method_adaptor_ctor<0x2fc>, &method_adaptor_ctor<0x2fd>, &method_adaptor_ctor<0x2fe>, &method_adaptor_ctor<0x2ff>,
  &method_adaptor_ctor<0x300>, &method_adaptor_ctor<0x301>, &method_adaptor_ctor<0x302>, &method_adaptor_ctor<0x303>, &method_adaptor_ctor<0x304>, &method_adaptor_ctor<0x305>, &method_adaptor_ctor<0x306>, &method_adaptor_ctor<0x307>,
  &method_adaptor_ctor<0x308>, &method_adaptor_ctor<0x309>, &method_adaptor_ctor<0x30a>, &method_adaptor_ctor<0x30b>, &method_adaptor_ctor<0x30c>, &method_adaptor_ctor<0x30d>, &method_adaptor_ctor<0x30e>, &method_adaptor_ctor<0x30f>,
  &method_adaptor_ctor<0x310>, &method_adaptor_ctor<0x311>, &method_adaptor_ctor<0x312>, &method_adaptor_ctor<0x313>, &method_adaptor_ctor<0x314>, &method_adaptor_ctor<0x315>, &method_adaptor_ctor<0x316>, &method_adaptor_ctor<0x317>,
  &method_adaptor_ctor<0x318>, &method_adaptor_ctor<0x319>, &method_adaptor_ctor<0x31a>, &method_adaptor_ctor<0x31b>, &method_adaptor_ctor<0x31c>, &method_adaptor_ctor<0x31d>, &method_adaptor_ctor<0x31e>, &method_adaptor_ctor<0x31f>,
  &method_adaptor_ctor<0x320>, &method_adaptor_ctor<0x321>, &method_adaptor_ctor<0x322>, &method_adaptor_ctor<0x323>, &method_adaptor_ctor<0x324>, &method_adaptor_ctor<0x325>, &method_adaptor_ctor<0x326>, &method_adaptor_ctor<0x327>,
  &method_adaptor_ctor<0x328>, &method_adaptor_ctor<0x329>, &method_adaptor_ctor<0x32a>, &method_adaptor_ctor<0x32b>, &method_adaptor_ctor<0x32c>, &method_adaptor_ctor<0x32d>, &method_adaptor_ctor<0x32e>, &method_adaptor_ctor<0x32f>,
  &method_adaptor_ctor<0x330>, &method_adaptor_ctor<0x331>, &method_adaptor_ctor<0x332>, &method_adaptor_ctor<0x333>, &method_adaptor_ctor<0x334>, &method_adaptor_ctor<0x335>, &method_adaptor_ctor<0x336>, &method_adaptor_ctor<0x337>,
  &method_adaptor_ctor<0x338>, &method_adaptor_ctor<0x339>, &method_adaptor_ctor<0x33a>, &method_adaptor_ctor<0x33b>, &method_adaptor_ctor<0x33c>, &method_adaptor_ctor<0x33d>, &method_adaptor_ctor<0x33e>, &method_adaptor_ctor<0x33f>,
  &method_adaptor_ctor<0x340>, &method_adaptor_ctor<0x341>, &method_adaptor_ctor<0x342>, &method_adaptor_ctor<0x343>, &method_adaptor_ctor<0x344>, &method_adaptor_ctor<0x345>, &method_adaptor_ctor<0x346>, &method_adaptor_ctor<0x347>,
  &method_adaptor_ctor<0x348>, &method_adaptor_ctor<0x349>, &method_adaptor_ctor<0x34a>, &method_adaptor_ctor<0x34b>, &method_adaptor_ctor<0x34c>, &method_adaptor_ctor<0x34d>, &method_adaptor_ctor<0x34e>, &method_adaptor_ctor<0x34f>,
  &method_adaptor_ctor<0x350>, &method_adaptor_ctor<0x351>, &method_adaptor_ctor<0x352>, &method_adaptor_ctor<0x353>, &method_adaptor_ctor<0x354>, &method_adaptor_ctor<0x355>, &method_adaptor_ctor<0x356>, &method_adaptor_ctor<0x357>,
  &method_adaptor_ctor<0x358>, &method_adaptor_ctor<0x359>, &method_adaptor_ctor<0x35a>, &method_adaptor_ctor<0x35b>, &method_adaptor_ctor<0x35c>, &method_adaptor_ctor<0x35d>, &method_adaptor_ctor<0x35e>, &method_adaptor_ctor<0x35f>,
  &method_adaptor_ctor<0x360>, &method_adaptor_ctor<0x361>, &method_adaptor_ctor<0x362>, &method_adaptor_ctor<0x363>, &method_adaptor_ctor<0x364>, &method_adaptor_ctor<0x365>, &method_adaptor_ctor<0x366>, &method_adaptor_ctor<0x367>,
  &method_adaptor_ctor<0x368>, &method_adaptor_ctor<0x369>, &method_adaptor_ctor<0x36a>, &method_adaptor_ctor<0x36b>, &method_adaptor_ctor<0x36c>, &method_adaptor_ctor<0x36d>, &method_adaptor_ctor<0x36e>, &method_adaptor_ctor<0x36f>,
  &method_adaptor_ctor<0x370>, &method_adaptor_ctor<0x371>, &method_adaptor_ctor<0x372>, &method_adaptor_ctor<0x373>, &method_adaptor_ctor<0x374>, &method_adaptor_ctor<0x375>, &method_adaptor_ctor<0x376>, &method_adaptor_ctor<0x377>,
  &method_adaptor_ctor<0x378>, &method_adaptor_ctor<0x379>, &method_adaptor_ctor<0x37a>, &method_adaptor_ctor<0x37b>, &method_adaptor_ctor<0x37c>, &method_adaptor_ctor<0x37d>, &method_adaptor_ctor<0x37e>, &method_adaptor_ctor<0x37f>,
  &method_adaptor_ctor<0x380>, &method_adaptor_ctor<0x381>, &method_adaptor_ctor<0x382>, &method_adaptor_ctor<0x383>, &method_adaptor_ctor<0x384>, &method_adaptor_ctor<0x385>, &method_adaptor_ctor<0x386>, &method_adaptor_ctor<0x387>,
  &method_adaptor_ctor<0x388>, &method_adaptor_ctor<0x389>, &method_adaptor_ctor<0x38a>, &method_adaptor_ctor<0x38b>, &method_adaptor_ctor<0x38c>, &method_adaptor_ctor<0x38d>, &method_adaptor_ctor<0x38e>, &method_adaptor_ctor<0x38f>,
  &method_adaptor_ctor<0x390>, &method_adaptor_ctor<0x391>, &method_adaptor_ctor<0x392>, &method_adaptor_ctor<0x393>, &method_adaptor_ctor<0x394>, &method_adaptor_ctor<0x395>, &method_adaptor_ctor<0x396>, &method_adaptor_ctor<0x397>,
  &method_adaptor_ctor<0x398>, &method_adaptor_ctor<0x399>, &method_adaptor_ctor<0x39a>, &method_adaptor_ctor<0x39b>, &method_adaptor_ctor<0x39c>, &method_adaptor_ctor<0x39d>, &method_adaptor_ctor<0x39e>, &method_adaptor_ctor<0x39f>,
  &method_adaptor_ctor<0x3a0>, &method_adaptor_ctor<0x3a1>, &method_adaptor_ctor<0x3a2>, &method_adaptor_ctor<0x3a3>, &method_adaptor_ctor<0x3a4>, &method_adaptor_ctor<0x3a5>, &method_adaptor_ctor<0x3a6>, &method_adaptor_ctor<0x3a7>,
  &method_adaptor_ctor<0x3a8>, &method_adaptor_ctor<0x3a9>, &method_adaptor_ctor<0x3aa>, &method_adaptor_ctor<0x3ab>, &method_adaptor_ctor<0x3ac>, &method_adaptor_ctor<0x3ad>, &method_adaptor_ctor<0x3ae>, &method_adaptor_ctor<0x3af>,
  &method_adaptor_ctor<0x3b0>, &method_adaptor_ctor<0x3b1>, &method_adaptor_ctor<0x3b2>, &method_adaptor_ctor<0x3b3>, &method_adaptor_ctor<0x3b4>, &method_adaptor_ctor<0x3b5>, &method_adaptor_ctor<0x3b6>, &method_adaptor_ctor<0x3b7>,
  &method_adaptor_ctor<0x3b8>, &method_adaptor_ctor<0x3b9>, &method_adaptor_ctor<0x3ba>, &method_adaptor_ctor<0x3bb>, &method_adaptor_ctor<0x3bc>, &method_adaptor_ctor<0x3bd>, &method_adaptor_ctor<0x3be>, &method_adaptor_ctor<0x3bf>,
  &method_adaptor_ctor<0x3c0>, &method_adaptor_ctor<0x3c1>, &method_adaptor_ctor<0x3c2>, &method_adaptor_ctor<0x3c3>, &method_adaptor_ctor<0x3c4>, &method_adaptor_ctor<0x3c5>, &method_adaptor_ctor<0x3c6>, &method_adaptor_ctor<0x3c7>,
  &method_adaptor_ctor<0x3c8>, &method_adaptor_ctor<0x3c9>, &method_adaptor_ctor<0x3ca>, &method_adaptor_ctor<0x3cb>, &method_adaptor_ctor<0x3cc>, &method_adaptor_ctor<0x3cd>, &method_adaptor_ctor<0x3ce>, &method_adaptor_ctor<0x3cf>,
  &method_adaptor_ctor<0x3d0>, &method_adaptor_ctor<0x3d1>, &method_adaptor_ctor<0x3d2>, &method_adaptor_ctor<0x3d3>, &method_adaptor_ctor<0x3d4>, &method_adaptor_ctor<0x3d5>, &method_adaptor_ctor<0x3d6>, &method_adaptor_ctor<0x3d7>,
  &method_adaptor_ctor<0x3d8>, &method_adaptor_ctor<0x3d9>, &method_adaptor_ctor<0x3da>, &method_adaptor_ctor<0x3db>, &method_adaptor_ctor<0x3dc>, &method_adaptor_ctor<0x3dd>, &method_adaptor_ctor<0x3de>, &method_adaptor_ctor<0x3df>,
  &method_adaptor_ctor<0x3e0>, &method_adaptor_ctor<0x3e1>, &method_adaptor_ctor<0x3e2>, &method_adaptor_ctor<0x3e3>, &method_adaptor_ctor<0x3e4>, &method_adaptor_ctor<0x3e5>, &method_adaptor_ctor<0x3e6>, &method_adaptor_ctor<0x3e7>,
  &method_adaptor_ctor<0x3e8>, &method_adaptor_ctor<0x3e9>, &method_adaptor_ctor<0x3ea>, &method_adaptor_ctor<0x3eb>, &method_adaptor_ctor<0x3ec>, &method_adaptor_ctor<0x3ed>, &method_adaptor_ctor<0x3ee>, &method_adaptor_ctor<0x3ef>,
  &method_adaptor_ctor<0x3f0>, &method_adaptor_ctor<0x3f1>, &method_adaptor_ctor<0x3f2>, &method_adaptor_ctor<0x3f3>, &method_adaptor_ctor<0x3f4>, &method_adaptor_ctor<0x3f5>, &method_adaptor_ctor<0x3f6>, &method_adaptor_ctor<0x3f7>,
  &method_adaptor_ctor<0x3f8>, &method_adaptor_ctor<0x3f9>, &method_adaptor_ctor<0x3fa>, &method_adaptor_ctor<0x3fb>, &method_adaptor_ctor<0x3fc>, &method_adaptor_ctor<0x3fd>, &method_adaptor_ctor<0x3fe>, &method_adaptor_ctor<0x3ff>,
};

// -------------------------------------------------------------------
//  stderr/stdout replacements

static VALUE
stdout_write (VALUE /*self*/, VALUE a)
{
  if (RubyInterpreter::instance ()->current_console ()) {
    if (TYPE (a) != T_STRING) {
      a = rb_obj_as_string (a);
    }
    RubyInterpreter::instance ()->current_console ()->write_str (StringValuePtr (a), gsi::Console::OS_stdout);
  }
  return Qnil;
}

static VALUE 
stdout_flush (VALUE /*self*/)
{
  if (RubyInterpreter::instance ()->current_console ()) {
    RubyInterpreter::instance ()->current_console ()->flush ();
  }
  return Qnil;
}

static VALUE
stdout_tty (VALUE /*self*/)
{
  if (RubyInterpreter::instance ()->current_console () && RubyInterpreter::instance ()->current_console ()->is_tty ()) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

static VALUE
stdout_winsize (VALUE /*self*/)
{
  if (RubyInterpreter::instance ()->current_console ()) {
    VALUE ary = rb_ary_new ();
    rb_ary_push (ary, INT2NUM (RubyInterpreter::instance ()->current_console ()->rows ()));
    rb_ary_push (ary, INT2NUM (RubyInterpreter::instance ()->current_console ()->columns ()));
    return ary;
  } else {
    return Qnil;
  }
}

static VALUE
stderr_write (VALUE /*self*/, VALUE a)
{
  if (RubyInterpreter::instance ()->current_console ()) {
    if (TYPE (a) != T_STRING) {
      a = rb_obj_as_string (a);
    }
    RubyInterpreter::instance ()->current_console ()->write_str (StringValuePtr (a), gsi::Console::OS_stderr);
  }
  return Qnil;
}

static VALUE 
stderr_flush (VALUE /*self*/)
{
  if (RubyInterpreter::instance ()->current_console ()) {
    RubyInterpreter::instance ()->current_console ()->flush ();
  }
  return Qnil;
}

static VALUE
stderr_tty (VALUE self)
{
  return stdout_tty (self);
}

static VALUE
stderr_winsize (VALUE self)
{
  return stdout_winsize (self);
}

// --------------------------------------------------------------------------
//  RubyInterpreter implementation

static RubyInterpreter *sp_rba_interpreter = 0;

struct RubyConstDescriptor
{
  VALUE klass;
  const gsi::MethodBase *meth;
  std::string name;
};

extern "C" void ruby_prog_init();

static void
rba_add_path (const std::string &path, bool prepend)
{
  VALUE pv = rb_gv_get ("$:");
  if (pv != Qnil && TYPE (pv) == T_ARRAY) {
    if (prepend) {
      rb_ary_unshift (pv, rb_str_new (path.c_str (), long (path.size ())));
    } else {
      rb_ary_push (pv, rb_str_new (path.c_str (), long (path.size ())));
    }
  }
}

static std::string
ruby_name (const std::string &n)
{
  if (n == "*!") {
    //  non-commutative multiplication
    return "*";
  } else {
    return n;
  }
}

namespace
{

class RubyClassGenerator
{
public:
  RubyClassGenerator (VALUE module)
    : m_module (module)
  {
    //  .. nothing yet ..
  }

  //  needs to be called before for each extension before the classes are made
  void register_extension (const gsi::ClassBase *cls)
  {
    if (cls->name ().empty ()) {
      //  got an extension
      tl_assert (cls->parent ());
      m_extensions_for [cls->parent ()->declaration ()].push_back (cls->declaration ());
    }
  }

  VALUE make_class (const gsi::ClassBase *cls, bool as_static, VALUE parent_class = (VALUE) 0, const gsi::ClassBase *parent = 0)
  {
    if (is_registered (cls, as_static)) {
      return ruby_cls (cls, as_static);
    }

    VALUE super = rb_cObject;
    if (cls->base () != 0) {
      super = make_class (cls->base (), as_static);
    }

    VALUE klass;
    if (as_static) {

      if (tl::verbosity () >= 20) {
        tl::log << tl::to_string (tr ("Registering class as Ruby module: ")) << cls->name ();
      }

      std::string mixin_name = cls->name () + "_Mixin";

      if (parent) {
        klass = rb_define_module_under (parent_class, mixin_name.c_str ());
      } else {
        klass = rb_define_module_under (m_module, mixin_name.c_str ());
      }

      //  if the base class is an extension (mixin), we cannot use it as superclass because it's a module
      if (cls->base () != 0) {
        rb_include_module (klass, super);
      }

    } else {

      if (parent) {
        klass = rb_define_class_under (parent_class, cls->name ().c_str (), super);
      } else {
        klass = rb_define_class_under (m_module, cls->name ().c_str (), super);
      }

      rb_define_alloc_func (klass, alloc_proxy);

    }

    register_class (klass, cls, as_static);

    //  mix-in unnamed extensions

    auto exts = m_extensions_for.find (cls);
    if (exts != m_extensions_for.end ()) {
      for (auto ie = exts->second.begin (); ie != exts->second.end (); ++ie) {
        VALUE ext_module = make_class (*ie, true);
        rb_include_module (klass, ext_module);
        rb_extend_object (klass, ext_module);
      }
    }

    //  produce the child classes

    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (! cc->name ().empty ()) {
        if (! is_registered (cc->declaration (), false)) {
          make_class (cc->declaration (), false, klass, cls);
        } else {
          VALUE child_class = ruby_cls (cc->declaration (), false);
          rb_define_const (klass, cc->name ().c_str (), child_class);
        }
      }
    }

    MethodTable *mt = MethodTable::method_table_by_class (cls, true /*force init*/);

    for (auto m = (cls)->begin_methods (); m != (cls)->end_methods (); ++m) {

      if (! (*m)->is_callback ()) {

        if (! (*m)->is_static ()) {

          bool drop_method = false;
          if ((*m)->smt () == gsi::MethodBase::Dup) {
            //  drop dup method -> replaced by Assign in ctor context
            drop_method = true;
          } else if ((*m)->smt () == gsi::MethodBase::Assign) {
            mt->add_ctor_method ("initialize_copy", *m);
          }

          if (! drop_method) {

            for (auto syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
              if (syn->is_predicate) {
                mt->add_method (syn->name, *m);
                mt->add_method (syn->name + "?", *m);
              } else if (syn->is_setter) {
                mt->add_method (syn->name + "=", *m);
              } else {
                mt->add_method (ruby_name (syn->name), *m);
              }
            }

          }

        } else {

          for (auto syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {

            if (isupper (syn->name [0]) && (*m)->begin_arguments () == (*m)->end_arguments ()) {

              //  Static const methods are constants.
              //  Methods without arguments which start with a capital letter are treated as constants
              //  for backward compatibility
              m_constants.push_back (RubyConstDescriptor ());
              m_constants.back ().klass = klass;
              m_constants.back ().meth = *m;
              m_constants.back ().name = (*m)->begin_synonyms ()->name;

            } else if ((*m)->ret_type ().type () == gsi::T_object && (*m)->ret_type ().pass_obj () && syn->name == "new") {

              //  "new" is mapped to "initialize" member function (special translation of
              //  member to static is indicated through the "ctor" attribute.
              mt->add_ctor_method ("initialize", *m);

            } else if (syn->is_predicate) {

              mt->add_method (syn->name, *m);
              mt->add_method (syn->name + "?", *m);

            } else if (syn->is_setter) {

              mt->add_method (syn->name + "=", *m);

            } else {

              mt->add_method (ruby_name (syn->name), *m);

            }
          }

        }

      }

    }

    //  clean up the method table
    mt->finish ();

    //  NOTE: extensions can't carry methods - this is due to the method numbering scheme
    //  which can only handle direct base classes. So only constants are carried forward.
    if (! as_static) {

      //  Hint: we need to do static methods before the non-static ones because
      //  rb_define_module_function creates an private instance method.
      //  If we do the non-static methods afterwards we will make it a public once again.
      //  The order of the names will be "name(non-static), name(static), ..." because
      //  the static flag is the second member of the key (string, bool) pair.
      for (size_t mid = mt->bottom_mid (); mid < mt->top_mid (); ++mid) {

        if (mt->is_static (mid)) {

          tl_assert (mid < size_t (sizeof (method_adaptors) / sizeof (method_adaptors [0])));

          /* Note: Ruby does not support static protected functions, hence we have them (i.e. QThread::usleep).
           *       Do we silently create public ones from them:
          if (mt->is_protected (mid)) {
            tl::warn << "static '" << mt->name (mid) << "' method cannot be protected in class " << c->name ();
          }
          */

          rb_define_module_function (klass, mt->name (mid).c_str (), (ruby_func) method_adaptors[mid], -1);

        }

      }

      for (size_t mid = mt->bottom_mid (); mid < mt->top_mid (); ++mid) {

        if (mt->is_ctor (mid)) {

          tl_assert (mid < size_t (sizeof (method_adaptors_ctor) / sizeof (method_adaptors_ctor [0])));

          if (! mt->is_protected (mid)) {
            rb_define_method (klass, mt->name (mid).c_str (), (ruby_func) method_adaptors_ctor[mid], -1);
          } else {
            //  a protected constructor needs to be provided in both protected and non-protected mode
            rb_define_method (klass, mt->name (mid).c_str (), (ruby_func) method_adaptors_ctor[mid], -1);
            rb_define_protected_method (klass, mt->name (mid).c_str (), (ruby_func) method_adaptors_ctor[mid], -1);
          }

        } else if (! mt->is_static (mid)) {

          tl_assert (mid < size_t (sizeof (method_adaptors) / sizeof (method_adaptors [0])));

          if (! mt->is_protected (mid)) {
            rb_define_method (klass, mt->name (mid).c_str (), (ruby_func) method_adaptors[mid], -1);
          } else {
            rb_define_protected_method (klass, mt->name (mid).c_str (), (ruby_func) method_adaptors[mid], -1);
          }

        }

        if (mt->is_signal (mid)) {

          //  We alias the signal name to an assignment, so the following can be done:
          //    x = object with signal "signal"
          //    x.signal = proc
          //  this will basically map to
          //    x.signal(proc)
          //  which will make proc the only receiver for the signal
          rb_define_alias (klass, (mt->name (mid) + "=").c_str (), mt->name (mid).c_str ());

        }

        if (mt->name (mid) == "to_s") {
    #if HAVE_RUBY_VERSION_CODE>=20000 && defined(GSI_ALIAS_INSPECT)
        //  Ruby 2.x does no longer alias "inspect" to "to_s" automatically, so we have to do this:
          rb_define_alias (klass, "inspect", "to_s");
    #endif
        } else if (mt->name (mid) == "==") {
          rb_define_alias (klass, "eql?", "==");
        }

      }

    }

    return klass;
  }

  void make_constants ()
  {
    for (auto c = m_constants.begin (); c != m_constants.end (); ++c) {

      try {

        gsi::SerialArgs retlist (c->meth->retsize ());
        gsi::SerialArgs arglist (c->meth->argsize ());
        c->meth->call (0, arglist, retlist);
        tl::Heap heap;
        VALUE ret = pull_arg (c->meth->ret_type (), 0, retlist, heap);
        rb_define_const (c->klass, c->name.c_str (), ret);

      } catch (tl::Exception &ex) {
        tl::warn << "Got exception '" << ex.msg () << "' while defining constant " << c->name;
      }

    }
  }

private:
  VALUE m_module;
  std::vector <RubyConstDescriptor> m_constants;
  std::map<const gsi::ClassBase *, std::vector<const gsi::ClassBase *> > m_extensions_for;
  std::set<const gsi::ClassBase *> m_extensions;
};

}

static void
rba_init (RubyInterpreterPrivateData *d)
{
  VALUE module = rb_define_module ("RBA");

  //  initialize the locked object vault as a fast replacement for rb_gc_register_address/rb_gc_unregister_address.
  rba::make_locked_object_vault (module);

  //  save all constants for later (we cannot declare them while we are still producing classes
  //  because of the enum representative classes and enum constants are important)
  std::vector <RubyConstDescriptor> constants;

  std::list<const gsi::ClassBase *> sorted_classes = gsi::ClassBase::classes_in_definition_order ();

  RubyClassGenerator gen (module);

  //  first pass: register the extensions
  for (auto c = sorted_classes.begin (); c != sorted_classes.end (); ++c) {
    if ((*c)->declaration () != *c) {
      gen.register_extension (*c);
    }
  }

  //  second pass: make the classes
  for (auto c = sorted_classes.begin (); c != sorted_classes.end (); ++c) {
    if ((*c)->declaration () == *c) {
      gen.make_class (*c, false);
    }
  }

  //  now make the constants
  gen.make_constants ();

  //  define a signal representative class RBASignal
  SignalHandler::define_class (module, "RBASignal");

  //  define new classes for handling stdout and stderr.
  //  use IO as the basis for that.
  d->stdout_klass = rb_define_class_under (module, "RBAStdout", rb_cIO);
  rb_define_method (d->stdout_klass, "write", (ruby_func) &stdout_write, 1);
  rb_define_method (d->stdout_klass, "flush", (ruby_func) &stdout_flush, 0);
  rb_define_method (d->stdout_klass, "tty?", (ruby_func) &stdout_tty, 0);
  rb_define_method (d->stdout_klass, "winsize", (ruby_func) &stdout_winsize, 0);
  d->stderr_klass = rb_define_class_under (module, "RBAStderr", rb_cIO);
  rb_define_method (d->stderr_klass, "write", (ruby_func) &stderr_write, 1);
  rb_define_method (d->stderr_klass, "flush", (ruby_func) &stderr_flush, 0);
  rb_define_method (d->stderr_klass, "tty?", (ruby_func) &stderr_tty, 0);
  rb_define_method (d->stderr_klass, "winsize", (ruby_func) &stderr_winsize, 0);

  //  register the saved stdout/stderr objects with the garbage collector
  rb_global_variable (& d->saved_stdout);
  rb_global_variable (& d->saved_stderr);

  //  create the handler objects
  VALUE empty_args [] = { INT2NUM (0) };
  d->saved_stderr = rba_class_new_instance_checked (1, empty_args, d->stderr_klass);
  d->saved_stdout = rba_class_new_instance_checked (1, empty_args, d->stdout_klass);
}

RubyInterpreter::RubyInterpreter ()
  : gsi::Interpreter (0, "rba"),
    d (new RubyInterpreterPrivateData ())
{
  tl::SelfTimer timer (tl::verbosity () >= 21, "Initializing Ruby");

  tl_assert (! sp_rba_interpreter);
  sp_rba_interpreter = this;
  rba_init (d);
  rb_init_top_self ();
}

RubyInterpreter::~RubyInterpreter () 
{
  delete d;
  d = 0;

  rb_release_top_self ();

  sp_rba_interpreter = 0;
}

RubyInterpreter *RubyInterpreter::instance ()
{
  return sp_rba_interpreter;
}

std::string
RubyInterpreter::version () const
{
  try {
    return const_cast<RubyInterpreter *> (this)->eval_expr ("RUBY_VERSION.to_s+'-p'+RUBY_PATCHLEVEL.to_s+' ('+RUBY_PLATFORM+')'").to_string ();
  } catch (...) {
    return std::string ("unknown");
  }
}

static int *s_argc = 0;
static char **s_argv = 0;
static int (*s_main_func) (int &, char **) = 0;

static VALUE run_app_func (VALUE)
{
  int res = 0;

  if (s_main_func && s_argv && s_argc && *s_argc > 0) {
    res = (*s_main_func) (*s_argc, s_argv);
  }

  if (res) {
    rb_exit (res);
  }

  return Qnil;
}

int
RubyInterpreter::initialize (int &main_argc, char **main_argv, int (*main_func) (int &, char **))
{
  static char argv1[] = "-e";
  static char argv2[] = "__run_app__";

  int argc = 3;
  char *argvv[3];
  argvv[0] = main_argv[0];
  // Hint: to keep ruby_options from reading stdin, we simulate a "-e" option with an empty script
  argvv[1] = argv1;
  argvv[2] = argv2;
  char **argv = argvv;

#if HAVE_RUBY_VERSION_CODE>=10900
  //  Make sure we call ruby_sysinit on Windows because otherwise the program will crash (this
  //  has been observed on Windows under MSVC 2017 with Ruby 2.5.1 for example)
  //  ruby_sysinit will restore the argc/argv to their originals, so we use copies here.
  int xargc = argc;
  char **xargv = argv;
  ruby_sysinit (&xargc, &xargv);
#endif

  {
    // Hint: this macro must be called from as high as possible on the stack.
    // Ruby does not work properly if any ruby function is called from above the
    // stack. Therefore this method must be called as high as possible in the call
    // stack (i.e. from main).
    // One effect of the fail is that a "require" does not work properly and will
    // abort with a segmentation fault.
#if defined(RUBY_INIT_STACK) // for ruby <=1.8.5.
    RUBY_INIT_STACK;
#endif

    {
      //  Keep ruby_init from changing the SIGINT handler.
      //  The reason for ruby_init doing this is probably to enable the input methods
      //  to receive Ctrl+C. We however do not need this functionality and users complain
      //  about now having Ctrl+C to kill the application.
      void (*org_sigint) (int) = signal (SIGINT, SIG_DFL);
      ruby_init ();
      signal (SIGINT, org_sigint);

#if defined(_WIN32)

      //  On Windows we derive additional path components from a file called ".ruby-paths.txt" 
      //  inside the installation directory. This way, the installer can copy the deployment-time
      //  installation easier.

      try {

        wchar_t buffer[MAX_PATH];
        int len;
        if ((len = GetModuleFileName (NULL, buffer, MAX_PATH)) > 0) {

          std::string inst_dir = tl::absolute_path (tl::to_string (std::wstring (buffer, len)));
          std::string path_file = tl::combine_path (inst_dir, ".ruby-paths.txt");
          if (tl::file_exists (path_file)) {

            tl::log << tl::to_string (tr ("Reading Ruby path from ")) << path_file;

            tl::InputStream path_file_stream (path_file);
            std::string path_file_text = path_file_stream.read_all ();

            tl::Eval eval;
            eval.set_global_var ("inst_path", tl::Variant (inst_dir));
            tl::Expression ex;
            eval.parse (ex, path_file_text.c_str ());
            tl::Variant v = ex.execute ();

            if (v.is_list ()) {
              for (tl::Variant::iterator i = v.begin (); i != v.end (); ++i) {
                rba_add_path (i->to_string (), false);
              }
            }

          }

        }

      } catch (tl::Exception &ex) {
        tl::error << tl::to_string (tr ("Evaluation of Ruby path expression failed")) << ": " << ex.msg ();
      } catch (...) {
        tl::error << tl::to_string (tr ("Evaluation of Ruby path expression failed"));
      }

#endif

      rb_define_global_function("__run_app__", (VALUE (*)(...)) run_app_func, 0);

      s_argc = &main_argc;
      s_argv = main_argv;
      s_main_func = main_func;

      //  Continue with the real code inside the initialized frame indirectly over the
      //  __app_run__ function. This was necessary to make the unit test framework work
      //  with Ruby 2.3.0. Without this scheme, rb_load_protect was giving a segmentation
      //  fault on exception.
     
#if HAVE_RUBY_VERSION_CODE<10900

      //  Remove setters for $0 and $PROGRAM_NAME (still both are linked) because
      //  the setter does strange things with the process and the argv, specifically argv[0] above.
      //  This is no longer the case for 1.9 and 2.x. On ruby 2.5.0 crashes have been observed
      //  with this code, so it got moved into the 1.8.x branch.
      static VALUE argv0 = Qnil;
      argv0 = c2ruby<const char *> (main_argv [0]);
      rb_define_hooked_variable("$0", &argv0, 0, 0);
      rb_define_hooked_variable("$PROGRAM_NAME", &argv0, 0, 0);

      //  1.8.x does not have ruby_run_node
      ruby_options(argc, argv);
      ruby_run();
      int res = 0;

#else
      int res = ruby_run_node (ruby_options (argc, argv));
#endif

      s_argc = 0;
      return res;

    }
  }
}

void 
RubyInterpreter::set_debugger_scope (const std::string &filename)
{
  d->debugger_scope = filename;
}

void 
RubyInterpreter::remove_debugger_scope ()
{
  d->debugger_scope.clear ();
}

const std::string &
RubyInterpreter::debugger_scope () const
{
  return d->debugger_scope;
}

void
RubyInterpreter::ignore_next_exception ()
{
  if (d->current_exec_handler) {
    d->ignore_next_exception = true;
  }
}

void
RubyInterpreter::add_package_location (const std::string &package_path)
{
  std::string path = tl::combine_path (tl::absolute_file_path (package_path), "ruby");
  if (tl::file_exists (path) && d->package_paths.find (path) == d->package_paths.end ()) {
    d->package_paths.insert (path);
    add_path (path);
  }
}

void
RubyInterpreter::remove_package_location (const std::string & /*package_path*/)
{
  //  Currently, we do not really remove the location. Ruby might get screwed up this way.
}

void
RubyInterpreter::add_path (const std::string &path, bool prepend)
{
  rba_add_path (path, prepend);
}

void
RubyInterpreter::require (const std::string &filename_utf8)
{
  std::string fl (rb_cstring_from_utf8 (filename_utf8));

  rb_set_errinfo (Qnil);
  int error = 0;
  rb_protect_init (); // see above

  RUBY_BEGIN_EXEC
    rb_protect ((VALUE (*)(VALUE))rb_require, (VALUE)fl.c_str (), &error);
  RUBY_END_EXEC

  if (error) {
    rba_check_error ();
  }
}

void
RubyInterpreter::load_file (const std::string &filename_utf8)
{
  std::string fl (rb_cstring_from_utf8 (filename_utf8));

  ruby_script (fl.c_str ());

  rb_set_errinfo (Qnil);
  int error = 0;
  int wrap = 0; // TODO: make variable?
  rb_protect_init (); // see above

  RUBY_BEGIN_EXEC
    rb_load_protect (rb_str_new (fl.c_str (), long (fl.size ())), wrap, &error);
  RUBY_END_EXEC

  if (error) {
    rba_check_error ();
  }
}

void
RubyInterpreter::eval_string (const char *expr, const char *file, int line, int context)
{
  d->file_id_map.clear ();
  rba_eval_string_in_context (expr, file, line, context);
}

tl::Variant
RubyInterpreter::eval_expr (const char *expr, const char *file, int line, int context)
{
  d->file_id_map.clear ();
  VALUE res = rba_eval_string_in_context (expr, file, line, context);
  if (res != Qnil) {
    return ruby2c<tl::Variant> (res);
  } else {
    return tl::Variant ();
  }
}

void
RubyInterpreter::eval_string_and_print (const char *expr, const char *file, int line, int context)
{
  d->file_id_map.clear ();
  VALUE res = rba_eval_string_in_context (expr, file, line, context);
  if (current_console () && res != Qnil) {
    VALUE res_s = rba_safe_obj_as_string (res);
    current_console ()->write_str (StringValuePtr (res_s), gsi::Console::OS_stdout);
    current_console ()->write_str ("\n", gsi::Console::OS_stdout);
  }
}

void
RubyInterpreter::define_variable (const std::string &name, const tl::Variant &value)
{
  rb_gv_set (name.c_str (), c2ruby (value));
}

gsi::Inspector *
RubyInterpreter::inspector (int context)
{
  return create_inspector (context);
}

bool
RubyInterpreter::available () const
{
  return true;
}

gsi::Console *RubyInterpreter::current_console ()
{
  return d->current_console;
}

void 
RubyInterpreter::push_console (gsi::Console *console)
{
  if (! d->current_console) {
    std::swap (d->saved_stderr, rb_stderr);
    std::swap (d->saved_stdout, rb_stdout);
  } else {
    d->consoles.push_back (d->current_console);
  }

  d->current_console = console;
}

void 
RubyInterpreter::remove_console (gsi::Console *console)
{
  if (d->current_console == console) {

    if (d->consoles.empty ()) {
      d->current_console = 0;
      std::swap (d->saved_stderr, rb_stderr);
      std::swap (d->saved_stdout, rb_stdout);
    } else {
      d->current_console = d->consoles.back ();
      d->consoles.pop_back ();
    }

  } else {

    for (std::vector<gsi::Console *>::iterator c = d->consoles.begin (); c != d->consoles.end (); ++c) {
      if (*c == console) {
        d->consoles.erase (c);
        break;
      }
    }

  }
}

void
RubyInterpreter::block_exceptions (bool f)
{
  d->block_exceptions = f;
}

bool
RubyInterpreter::exceptions_blocked ()
{
  return d->block_exceptions;
}

static size_t
prepare_trace (RubyInterpreter *interp, const char *fn)
{
  RubyInterpreterPrivateData *d = interp->d;
  d->in_trace = true;

  std::map<const char *, size_t>::const_iterator f = d->file_id_map.find (fn);
  if (f == d->file_id_map.end ()) {
    f = d->file_id_map.insert (std::make_pair (fn, d->current_exec_handler->id_for_path (interp, fn))).first;
  } 

  return f->second;
}

static void
finish_trace (RubyInterpreter *interp)
{
  RubyInterpreterPrivateData *d = interp->d;
  d->in_trace = false;
}

static void
#if HAVE_RUBY_VERSION_CODE<10900
trace_callback (rb_event_t event, NODE *, VALUE /*self*/, ID /*id*/, VALUE /*klass*/)
#elif HAVE_RUBY_VERSION_CODE<20300
trace_callback (rb_event_flag_t event, VALUE /*data*/, VALUE /*self*/, ID /*id*/, VALUE /*klass*/)
#else
//  raw trace func
trace_callback (VALUE /*data*/, rb_trace_arg_t *trace_arg)
#endif
{
#if HAVE_RUBY_VERSION_CODE>=20300
  rb_event_flag_t event = rb_tracearg_event_flag (trace_arg);
#endif

  //  TODO: should not access private data
  RubyInterpreterPrivateData *d = RubyInterpreter::instance ()->d;

  if (d->current_exec_handler && !d->in_trace) {

    if ((event & RUBY_EVENT_LINE) != 0) {

      //  see below for a description of s_block_exceptions
      d->block_exceptions = false;

      RBA_TRY

        if (d->exit_on_next) {
          throw tl::ExitException (0);
        }

        try {

          int line = rb_sourceline ();
          size_t file_id = prepare_trace (RubyInterpreter::instance (), rb_sourcefile ());

          RubyStackTraceProvider st_provider (d->debugger_scope);
          d->current_exec_handler->trace (RubyInterpreter::instance (), file_id, line, &st_provider);

          finish_trace (RubyInterpreter::instance ());

        } catch (...) {
          finish_trace (RubyInterpreter::instance ());
          throw;
        }

      RBA_CATCH("trace callback")

    } else if ((event & RUBY_EVENT_CALL) != 0) {
      d->current_exec_handler->push_call_stack (RubyInterpreter::instance ());
    } else if ((event & RUBY_EVENT_RETURN) != 0) {
      d->current_exec_handler->pop_call_stack (RubyInterpreter::instance ());
    } else if ((event & RUBY_EVENT_RAISE) != 0 && ! d->block_exceptions) {

#if HAVE_RUBY_VERSION_CODE>=20300
      VALUE lasterr = rb_tracearg_raised_exception (trace_arg);
#else
      VALUE lasterr = rb_errinfo ();
#endif

      //  Don't catch syntax errors (it does not make sense to stop in the debugger for them).
      //  Sometimes lasterr is nil - we must suppress these errors as well.
      if (lasterr != Qnil && CLASS_OF (lasterr) != rb_eSyntaxError) {

        //  If the next exception shall be ignored, do so
        if (d->ignore_next_exception) {
          d->ignore_next_exception = false;
        } else {

          try {

            int line = rb_sourceline ();
            size_t file_id = prepare_trace (RubyInterpreter::instance (), rb_sourcefile ());

            std::string eclass = "<unknown>";
            std::string emsg = "<unknown>";

            VALUE klass = rb_class_path (CLASS_OF (lasterr));
            eclass = std::string (RSTRING_PTR(klass), RSTRING_LEN(klass));
            VALUE message = rb_obj_as_string(lasterr);
            emsg = std::string (RSTRING_PTR(message), RSTRING_LEN(message));

            RubyStackTraceProvider st_provider (d->debugger_scope);
            d->current_exec_handler->exception_thrown (RubyInterpreter::instance (), file_id, line, eclass, emsg, &st_provider);

            finish_trace (RubyInterpreter::instance ());

          } catch (tl::ExitException &) {
            d->exit_on_next = true; // delayed handling of tl::ExitException
            finish_trace (RubyInterpreter::instance ());
          } catch (...) {
            //  ignore other exceptions - we can't raise a ruby exception from the trace callback
            //  because this causes a fatal error (exception reentered)
            finish_trace (RubyInterpreter::instance ());
          }

        }

        //  Ruby tends to call this callback twice - once from rb_f_raise and then 
        //  from rb_exc_raise. We use the s_block_exceptions flag to suppress the 
        //  second one
        d->block_exceptions = true;

      }

    }

  }

}

void 
RubyInterpreter::push_exec_handler (gsi::ExecutionHandler *h)
{
  if (d->current_exec_handler) {

    d->exec_handlers.push_back (d->current_exec_handler);

  } else {

#if HAVE_RUBY_VERSION_CODE<10900
    rb_remove_event_hook(trace_callback);
    rb_add_event_hook(trace_callback, RUBY_EVENT_ALL);
#elif HAVE_RUBY_VERSION_CODE<20300
    rb_remove_event_hook(trace_callback);
    rb_add_event_hook(trace_callback, RUBY_EVENT_ALL, Qnil);
#else
    rb_remove_event_hook((rb_event_hook_func_t)trace_callback);
    rb_add_event_hook2((rb_event_hook_func_t)trace_callback, RUBY_EVENT_ALL, Qnil, RUBY_EVENT_HOOK_FLAG_RAW_ARG);
#endif

  }

  d->current_exec_handler = h;
  d->file_id_map.clear ();

  //  if we happen to push the exec handler inside the execution, 
  //  signal start of execution
  if (d->current_exec_level > 0) {
    d->current_exec_handler->start_exec (this);
  }
}

void 
RubyInterpreter::remove_exec_handler (gsi::ExecutionHandler *exec_handler)
{
  if (d->current_exec_handler == exec_handler) {

    //  if we happen to remove the exec handler inside the execution, 
    //  signal end of execution
    if (d->current_exec_level > 0) {
      d->current_exec_handler->end_exec (this);
    }

    if (d->exec_handlers.empty ()) {
      d->current_exec_handler = 0;
#if HAVE_RUBY_VERSION_CODE<20300
      rb_remove_event_hook(trace_callback);
#else
    rb_remove_event_hook((rb_event_hook_func_t)trace_callback);
#endif
    } else {
      d->current_exec_handler = d->exec_handlers.back ();
      d->exec_handlers.pop_back ();
    }

  } else {

    for (std::vector<gsi::ExecutionHandler *>::iterator eh = d->exec_handlers.begin (); eh != d->exec_handlers.end (); ++eh) {
      if (*eh == exec_handler) {
        d->exec_handlers.erase (eh);
        break;
      }
    }

  }

}

void
RubyInterpreter::begin_exec ()
{
  d->exit_on_next = false;
  d->block_exceptions = false;
  if (d->current_exec_level++ == 0) {
    d->file_id_map.clear ();
    if (d->current_exec_handler) {
      d->current_exec_handler->start_exec (this);
    }
  }
}

void
RubyInterpreter::end_exec ()
{
  if (d->current_exec_level > 0 && --d->current_exec_level == 0 && d->current_exec_handler) {
    d->current_exec_handler->end_exec (this);
  }
  if (d->exit_on_next) {
    d->exit_on_next = false;
    throw tl::ExitException (0);
  }
}

}

#else

namespace rba
{

static void fail (const char *file, int line)
{
  throw tl::ScriptError (tl::to_string (tr ("Ruby support not compiled in")).c_str (), file, line, "missing_feature", std::vector<tl::BacktraceElement> ());
}

RubyInterpreter::RubyInterpreter () 
{ 
  // .. nothing ..
}

RubyInterpreter::~RubyInterpreter () 
{
  // .. nothing ..
}

void 
RubyInterpreter::add_path (const std::string &)
{
  // .. nothing ..
}

void
RubyInterpreter::require (const std::string &)
{
  // .. nothing ..
}

void 
RubyInterpreter::set_debugger_scope (const std::string &)
{
  // .. nothing ..
}

void 
RubyInterpreter::remove_debugger_scope ()
{
  // .. nothing ..
}

const std::string &
RubyInterpreter::debugger_scope () const
{
  static const std::string empty;
  return empty;
}

void
RubyInterpreter::ignore_next_exception ()
{
  // .. nothing ..
}

void
RubyInterpreter::load_file (const std::string &)
{
  // .. nothing ..
}

void
RubyInterpreter::eval_string (const char *, const char *file, int line, int)
{
  fail (file, line);
}

void
RubyInterpreter::eval_string_and_print (const char *, const char *file, int line, int)
{
  fail (file, line);
}

void
RubyInterpreter::define_variable (const std::string &, const std::string &)
{
  // .. nothing ..
}

gsi::Inspector *RubyInterpreter::inspector (int)
{
  return 0;
}

bool
RubyInterpreter::available () const
{
  return false;
}

int
RubyInterpreter::initialize (int argc, char **argv, int (*main_func) (int, char **))
{
  return (*main_func) (argc, argv);
}

void 
RubyInterpreter::push_exec_handler (gsi::ExecutionHandler *)
{
  // .. nothing ..
}

void
RubyInterpreter::remove_exec_handler (gsi::ExecutionHandler *)
{
  // .. nothing ..
}

void 
RubyInterpreter::push_console (gsi::Console *)
{
  // .. nothing ..
}

void 
RubyInterpreter::remove_console (gsi::Console *)
{
  // .. nothing ..
}

gsi::Console *
RubyInterpreter::current_console ()
{
  return 0;
}

std::string
RubyInterpreter::version () const
{
  return std::string ();
}

tl::Variant
RubyInterpreter::eval_expr (const char *, const char *file, int line, int)
{
  fail (file, line);
  return tl::Variant ();
}

RubyInterpreter *
RubyInterpreter::instance ()
{
  return 0;
}

}

#endif

