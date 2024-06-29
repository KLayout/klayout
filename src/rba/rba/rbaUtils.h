
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

#ifndef _HDR_rbaUtils
#define _HDR_rbaUtils

#ifdef HAVE_RUBY

#include "gsiInterpreter.h"
#include "gsiSerialisation.h"

#include <ruby.h>

#include "tlScriptError.h"

/**
 *  @brief Gets the top-level self ("main")
 */
VALUE rb_get_top_self ();

/**
 *  @brief Initializes the top-level self value
 */
void rb_init_top_self ();

/**
 *  @brief Release the top-level self value
 */
void rb_release_top_self ();

//  Add some compatibility definitions for Ruby 1.8
#if HAVE_RUBY_VERSION_CODE < 10900

#  include "node.h"

//  Compatibility macros for Ruby 1.8
#  if !defined(RSTRING_PTR)
#    define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  endif

#  if !defined(RSTRING_LEN)
#    define RSTRING_LEN(s) (RSTRING(s)->len)
#  endif

#  if !defined(RARRAY_PTR)
#    define RARRAY_PTR(s) (RARRAY(s)->ptr)
#  endif

#  if !defined(RARRAY_LEN)
#    define RARRAY_LEN(s) (RARRAY(s)->len)
#  endif

#  if !defined(RCLASS_SUPER)
#    define RCLASS_SUPER(k) (RCLASS (k)->super)
#  endif

//  Ruby 1.8 does not define NUL2ULL
#  if !defined(NUM2ULL)
#    define NUM2ULL(x) rb_num2ull((VALUE)x)
#  endif

//  add rb_errinfo and rb_set_errinfo for Ruby 1.8
inline VALUE rb_errinfo()
{
  return rb_gv_get ("$!");
}

inline void rb_set_errinfo(VALUE v)
{
  rb_gv_set ("$!", v);
}

//  add rb_sourcefile and rb_sourceline for Ruby 1.8
inline const char *rb_sourcefile ()
{
  ruby_set_current_source ();
  return ruby_sourcefile;
}

inline int rb_sourceline ()
{
  ruby_set_current_source ();
  return ruby_sourceline;
}

//  add an emulation of rb_binding_new
inline VALUE rb_binding_new ()
{
  return rb_funcall (rb_get_top_self(), rb_intern ("binding"), 0);
}

#endif

#if HAVE_RUBY_VERSION_CODE < 20000

inline int push_map_key_i (VALUE key, VALUE /*value*/, VALUE arg)
{
  std::vector<VALUE> *v = (std::vector<VALUE> *) arg;
  v->push_back (key);
  return ST_CONTINUE;
}

/**
 *  @brief Emulate rb_hash_clear for Ruby <2.0
 */
inline void rb_hash_clear(VALUE hash)
{
  std::vector<VALUE> keys;
  keys.reserve (RHASH_SIZE (hash));
  rb_hash_foreach (hash, (int (*)(...)) &push_map_key_i, (VALUE) &keys);

  for (std::vector<VALUE>::const_iterator k = keys.begin (); k != keys.end (); ++k) {
    rb_hash_delete (hash, *k);
  }
}

#endif

typedef VALUE (*ruby_func)(ANYARGS);

/**
 *  A method to create a Ruby const char * string from a UTF-8 strings.
 *  This method is used for rb_require, rb_load.
 */
inline std::string rb_cstring_from_utf8 (const std::string &utf8)
{
  // TODO: implement (how?)
  return utf8;
}

/**
 *  @brief Sets up a block for protected evaluation
 *
 *  It was learned (by bitter experience), that in particular rb_protect cannot simply be
 *  called but requires some safety measures, at least in Ruby 1.8.5 as possibly 1.8.x.
 *  What happens in rb_protect is that a RNode object is instantiated in some kind
 *  of compile steps that builds a temporary executable statement. Deep inside this
 *  functionality, the rb_sourcefile () variable is copied into the RNode object, as this
 *  variable is usually updated with the current sourcefile name.
 *  In effect, the string referenced by this variable participates in the GC mark&sweep
 *  steps which leads to unpredictable results, if this variable is not set to a valid
 *  string (ruby) buffer or 0.
 *
 *  As a consequence, this function must be called before rb_protect and likely other xx_protect
 *  derivatives.
 */
inline void rb_protect_init ()
{
#if HAVE_RUBY_VERSION_CODE<10900
  ruby_sourcefile = 0;
  ruby_sourceline = 0;
#endif
}

#define RUBY_BEGIN_EXEC \
  try { \
    if (rba::RubyInterpreter::instance ()) { rba::RubyInterpreter::instance ()->begin_exec (); }

#define RUBY_END_EXEC \
    if (rba::RubyInterpreter::instance ()) { rba::RubyInterpreter::instance()->end_exec (); } \
  } catch (...) { \
    if (rba::RubyInterpreter::instance ()) { rba::RubyInterpreter::instance()->end_exec (); } \
    throw; \
  }

namespace rba
{

// -------------------------------------------------------------------
//  Utilities to generate C++ exceptions from Ruby exceptions

tl::BacktraceElement rba_split_bt_information (const char *m, size_t l);
void rba_get_backtrace_from_array (VALUE backtrace, std::vector<tl::BacktraceElement> &bt, unsigned int skip);
void rba_check_error (int state);
VALUE rba_string_value_f (VALUE obj);
VALUE rba_safe_string_value (VALUE obj);
VALUE rba_safe_obj_as_string (VALUE obj);
VALUE rba_safe_inspect (VALUE obj);
int rba_num2int_f (VALUE obj);
int rba_safe_num2int (VALUE obj);
unsigned int rba_num2uint_f (VALUE obj);
unsigned int rba_safe_num2uint (VALUE obj);
long rba_num2long_f (VALUE obj);
long rba_safe_num2long (VALUE obj);
unsigned long rba_num2ulong_f (VALUE obj);
unsigned long rba_safe_num2ulong (VALUE obj);
long long rba_num2ll_f (VALUE obj);
long long rba_safe_num2ll (VALUE obj);
unsigned long long rba_num2ull_f (VALUE obj);
unsigned long long rba_safe_num2ull (VALUE obj);
double rba_num2dbl_f (VALUE obj);
double rba_safe_num2dbl (VALUE obj);
std::string rba_class_name (VALUE self);
VALUE rba_class_new_instance_checked (int argc, VALUE *argv, VALUE klass);
VALUE rba_funcall2_checked (VALUE obj, ID id, int argc, VALUE *args);
VALUE rba_f_eval_checked (int argc, VALUE *argv, VALUE self);
void rba_yield_checked (VALUE value);
VALUE rba_eval_string_in_context (const char *expr, const char *file, int line, int context);

bool exceptions_blocked ();
void block_exceptions (bool f);

/**
 *  @brief A struct encapsulating the call parameters for a function
 */
template <class R, class A>
struct rba_func_callparam
{
  rba_func_callparam () : r (), a (), f () { }
  R r;
  A a;
  R (*f) (A);
};

/**
 *  @brief A wrapper function
 */
template <class R, class A>
VALUE rba_safe_func_caller (VALUE a)
{
  rba_func_callparam<R, A> *cp = (rba_func_callparam<R, A> *) a;
  cp->r = (*cp->f) (cp->a);
  return Qnil;
}

/**
 *  @brief calls a single argument/single return value function safely
 */
template <class R, class A>
R rba_safe_func (R (*f) (A), A arg)
{
  rba_func_callparam<R, A> cp;
  cp.f = f;
  cp.a = arg;

  rb_set_errinfo (Qnil);
  int error = 0;

  RUBY_BEGIN_EXEC
    //  NOTE: we do not want exceptions to be seen in the debugger here - later they are rethrown after
    //  being annotated. This is when we want to see them.
    bool eb = exceptions_blocked ();
    block_exceptions (true);
    rb_protect (&rba_safe_func_caller<R, A>, (VALUE) &cp, &error);
    block_exceptions (eb);
  RUBY_END_EXEC

  if (error) {
    rba_check_error (error);
  }
  return cp.r;
}

}

#endif

#endif

