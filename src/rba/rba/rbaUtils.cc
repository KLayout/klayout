
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


#if defined(HAVE_RUBY)

#include "rba.h"
#include "rbaUtils.h"
#include "rbaInternal.h"
#include "tlInclude.h"

#if HAVE_RUBY_VERSION_CODE >= 20200
#  include <ruby/debug.h>
#endif

static VALUE ruby_top_self = Qnil;

VALUE rb_get_top_self ()
{
  return ruby_top_self;
}

void rb_init_top_self ()
{
  //  hopefully this one always remains there:
  ruby_top_self = rb_eval_string ("self");
  rb_gc_register_address (&ruby_top_self);
}

void rb_release_top_self ()
{
  rb_gc_unregister_address (&ruby_top_self);
  ruby_top_self = Qnil;
}

namespace rba
{

tl::BacktraceElement
rba_split_bt_information (const char *m, size_t l)
{
  for (size_t i = 0; i + 1 < l; ++i) {

    if (m[i] == ':' && isdigit (m[i + 1])) {

      size_t j = i + 1;
      int line = 0;
      while (j < l && isdigit (m[j])) {
        line = (line * 10) + (int)(m[j] - '0');
        ++j;
      }

      std::string file;
      if (line > 0) {
        file = std::string (m, i);
      }
      if (j == l) {
        return tl::BacktraceElement (file, line);
      } else if (m[j] == ':') {
        return tl::BacktraceElement (file, line, std::string (m, j + 1, l - (j + 1)));
      }

    }

  }

  return tl::BacktraceElement (std::string (), 0, std::string (m, 0, l));
}

void
rba_get_backtrace_from_array (VALUE backtrace, std::vector<tl::BacktraceElement> &bt, unsigned int skip)
{
  if (TYPE (backtrace) == T_ARRAY) {

    unsigned int len = RARRAY_LEN(backtrace);
    VALUE *el = RARRAY_PTR(backtrace);
    bt.reserve (bt.size () + (len >= skip ? len - skip : 0));
    while (len-- > 0) {
      if (skip > 0) {
        ++el;
        --skip;
      } else {
        VALUE str = StringValue (*el++);
        bt.push_back (rba_split_bt_information (RSTRING_PTR(str), RSTRING_LEN(str)));
      }
    }

    //  Remove top entries with file "-e" - they originate from internal frames
    while (! bt.empty () && bt.back ().file == "-e") {
      bt.pop_back ();
    }

  }
}

void
block_exceptions (bool f)
{
  if (RubyInterpreter::instance ()) {
    RubyInterpreter::instance ()->block_exceptions (f);
  }
}

bool
exceptions_blocked ()
{
  return RubyInterpreter::instance () ? RubyInterpreter::instance ()->exceptions_blocked () : false;
}

void
rba_check_error (int state)
{
  VALUE lasterr = rb_errinfo ();

  //  Ruby employs this pseudo-exception to indicate a "break" or "return" of a loop.
  //  As this is an opaque condition, we continue Ruby execution later through a "RubyContinueException".
#if HAVE_RUBY_VERSION_CODE < 10900
  if (lasterr == Qnil) {
    throw RubyContinueException (state);
  }
#elif HAVE_RUBY_VERSION_CODE < 20300
  if (TYPE (lasterr) == T_NODE) {
    throw RubyContinueException (state);
  }
#else
  if (TYPE (lasterr) == T_IMEMO) {
    throw RubyContinueException (state);
  }
#endif

  if (CLASS_OF (lasterr) == rb_eSystemExit) {
    int status = NUM2INT (rb_funcall (lasterr, rb_intern ("status"), 0));
    throw tl::ExitException (status);
  }

  VALUE klass = rb_class_path (CLASS_OF (lasterr));
  std::string eclass = std::string (RSTRING_PTR(klass), RSTRING_LEN(klass));
  VALUE message = rba_safe_obj_as_string(lasterr);
  std::string emsg = std::string (RSTRING_PTR(message), RSTRING_LEN(message));

  std::vector <tl::BacktraceElement> bt;
  rba_get_backtrace_from_array (rb_funcall (lasterr, rb_intern ("backtrace"), 0), bt, 0);

  if (RubyInterpreter::instance ()) {
    const std::string &ds = RubyInterpreter::instance ()->debugger_scope ();
    bt.erase (bt.begin (), bt.begin () + RubyStackTraceProvider::scope_index (bt, ds));
  }

  //  parse the backtrace to get the line number
  tl::BacktraceElement info;
  if (CLASS_OF (lasterr) == rb_eSyntaxError) {
    //  for syntax errors we try to parse the message
    info = rba_split_bt_information (emsg.c_str (), emsg.size ());
    if (info.line == 0 && ! bt.empty ()) {
      info = bt.front ();
    }
  } else if (! bt.empty ()) {
    //  use the backtrace
    info = bt.front ();
  } else {
    //  use the error message
    info = rba_split_bt_information (emsg.c_str (), emsg.size ());
  }

  if (info.line > 0) {
    throw RubyError (lasterr, emsg.c_str (), info.file.c_str (), info.line, eclass.c_str (), bt);
  } else {
    throw RubyError (lasterr, emsg.c_str (), eclass.c_str (), bt);
  }
}

/**
 *  @brief needed because StringValue is a macro:
 */
VALUE
rba_string_value_f (VALUE obj)
{
  return StringValue (obj);
}

/**
 *  @brief string value retrieval with check
 */
VALUE
rba_safe_string_value (VALUE obj)
{
  return rba_safe_func (rba_string_value_f, obj);
}

/**
 *  @brief object to string with check
 */
VALUE
rba_safe_obj_as_string (VALUE obj)
{
  if (TYPE(obj) == T_STRING) {
    return obj;
  } else {
    return rba_safe_func (rb_obj_as_string, obj);
  }
}

/**
 *  @brief object to string with check
 */
VALUE
rba_safe_inspect (VALUE obj)
{
  return rba_safe_func (rb_inspect, obj);
}

/**
 *  @brief needed because NUM2INT may be a macro:
 */
int
rba_num2int_f (VALUE obj)
{
  return NUM2INT (obj);
}

/**
 *  @brief A safe NUM2INT implementation
 */
int
rba_safe_num2int (VALUE obj)
{
  return rba_safe_func (rba_num2int_f, obj);
}

/**
 *  @brief needed because NUM2UINT may be a macro:
 */
unsigned int
rba_num2uint_f (VALUE obj)
{
  return NUM2UINT (obj);
}

/**
 *  @brief A safe NUM2UINT implementation
 */
unsigned int
rba_safe_num2uint (VALUE obj)
{
  return rba_safe_func (rba_num2uint_f, obj);
}

/**
 *  @brief needed because NUM2LONG may be a macro:
 */
long
rba_num2long_f (VALUE obj)
{
  return NUM2LONG (obj);
}

/**
 *  @brief A safe NUM2LONG implementation
 */
long
rba_safe_num2long (VALUE obj)
{
  return rba_safe_func (rba_num2long_f, obj);
}

/**
 *  @brief needed because NUM2ULONG may be a macro:
 */
unsigned long
rba_num2ulong_f (VALUE obj)
{
  return NUM2ULONG (obj);
}

/**
 *  @brief A safe NUM2ULONG implementation
 */
unsigned long
rba_safe_num2ulong (VALUE obj)
{
  return rba_safe_func (rba_num2ulong_f, obj);
}

/**
 *  @brief needed because NUM2LL may be a macro:
 */
long long
rba_num2ll_f (VALUE obj)
{
  return NUM2LL (obj);
}

/**
 *  @brief A safe NUM2LL implementation
 */
long long
rba_safe_num2ll (VALUE obj)
{
  return rba_safe_func (rba_num2ll_f, obj);
}

/**
 *  @brief needed because NUM2ULL may be a macro:
 */
unsigned long long
rba_num2ull_f (VALUE obj)
{
  return NUM2ULL (obj);
}

/**
 *  @brief A safe NUM2ULL implementation
 */
unsigned long long
rba_safe_num2ull (VALUE obj)
{
  return rba_safe_func (rba_num2ull_f, obj);
}

/**
 *  @brief needed because NUM2DBL may be a macro:
 */
double
rba_num2dbl_f (VALUE obj)
{
  return NUM2DBL (obj);
}

/**
 *  @brief A safe NUM2DBL implementation
 */
double
rba_safe_num2dbl (VALUE obj)
{
  return rba_safe_func (rba_num2dbl_f, obj);
}

/**
 *  @brief Gets the class as a string
 */
std::string
rba_class_name (VALUE self)
{
  if (TYPE (self) != T_CLASS) {
    self = rb_class_of (self);
  }

  VALUE str = rb_obj_as_string (self);
  return std::string (RSTRING_PTR (str), RSTRING_LEN (str));
}

struct rb_class_new_instance_param
{
  int argc;
  VALUE *argv;
  VALUE klass;
};

static VALUE
rb_class_new_instance_wrap (VALUE args)
{
  //  HINT: this ugly cast is required since there is only one form of rb_protect
  const rb_class_new_instance_param *p = (const rb_class_new_instance_param *) args;
  return rb_class_new_instance (p->argc, p->argv, p->klass);
}

VALUE
rba_class_new_instance_checked (int argc, VALUE *argv, VALUE klass)
{
  VALUE ret = Qnil;

  rb_class_new_instance_param p;
  p.argc = argc;
  p.argv = argv;
  p.klass = klass;

  int error = 0;
  //  HINT: the ugly (VALUE) cast is required since there is only one form of rb_protect
  rb_protect_init (); // see above

  RUBY_BEGIN_EXEC
    ret = rb_protect (&rb_class_new_instance_wrap, (VALUE) &p, &error);
  RUBY_END_EXEC

  if (error) {
    rba_check_error (error);
  }
  return ret;
}

//  A protect wrapper for rb_funcall2

struct rb_funcall2_params
{
  VALUE obj;
  ID id;
  int argc;
  VALUE *args;
};

static VALUE
rb_funcall2_wrap (VALUE args)
{
  //  HINT: this ugly cast is required since there is only one form of rb_protect
  const rb_funcall2_params *p = (const rb_funcall2_params *) args;
  return rb_funcall2 (p->obj, p->id, p->argc, p->args);
}

VALUE rba_funcall2_checked (VALUE obj, ID id, int argc, VALUE *args)
{
#if HAVE_RUBY_VERSION_CODE>=10900
  //  Hint: calling of methods on terminated objects cannot really be avoided in all cases -
  //  for example when the destructor triggers some callback as is the case in Qt events
  //  (i.e. childEvent is triggered when a child is removed and may happen on a parent which
  //  is terminated but not destroyed yet). To avoid problems in that case we simply ignore the
  //  call.
  if (TYPE (obj) == T_ZOMBIE) {
    return Qnil;
  }
#endif

  VALUE ret = Qnil;

  rb_funcall2_params p;
  p.obj  = obj;
  p.id   = id;
  p.argc = argc;
  p.args = args;

  int error = 0;
  //  HINT: the ugly (VALUE) cast is required since there is only one form of rb_protect
  rb_protect_init (); // see above

  if (! ruby_native_thread_p ()) {
    throw tl::Exception (tl::to_string (tr ("Can't execute Ruby callbacks from non-Ruby threads")));
  }

  RUBY_BEGIN_EXEC
    ret = rb_protect (&rb_funcall2_wrap, (VALUE) &p, &error);
  RUBY_END_EXEC

  if (error) {
    rba_check_error (error);
  }
  return ret;
}

struct rb_f_eval_params
{
  int argc;
  VALUE *argv;
  VALUE self;
};

static VALUE
rb_f_eval_wrap (VALUE args)
{
  //  HINT: this ugly cast is required since there is only one form of rb_protect
  const rb_f_eval_params *p = (const rb_f_eval_params *) args;
  return rb_funcall2 (p->self, rb_intern ("eval"), p->argc, p->argv);
}

VALUE
rba_f_eval_checked (int argc, VALUE *argv, VALUE self)
{
  VALUE ret = Qnil;

  rb_f_eval_params p;
  p.argc = argc;
  p.argv = argv;
  p.self = self;

  int error = 0;
  //  HINT: the ugly (VALUE) cast is required since there is only one form of rb_protect
  rb_protect_init (); // see above

  RUBY_BEGIN_EXEC
    ret = rb_protect (&rb_f_eval_wrap, (VALUE) &p, &error);
  RUBY_END_EXEC

  if (error) {
    rba_check_error (error);
  }
  return ret;
}

void
rba_yield_checked (VALUE value)
{
  int error = 0;
  rb_protect_init (); // see above

  RUBY_BEGIN_EXEC
    rb_protect (&rb_yield, value, &error);
  RUBY_END_EXEC

  if (error) {
    rba_check_error (error);
  }
}

#if HAVE_RUBY_VERSION_CODE >= 20200
static VALUE debug_inspector_get_binding (const rb_debug_inspector_t *dbg_context, void *data)
{
  long context = long (reinterpret_cast<size_t> (data));
  return rb_debug_inspector_frame_binding_get(dbg_context, context);
}
#endif

VALUE rba_eval_string_in_context (const char *expr, const char *file, int line, int context)
{
  rb_set_errinfo (Qnil);

  if (file) {
    ruby_script (file);
  } else {
    const char *e = "<immediate>";
    ruby_script (e);
  }

  int argc;
  VALUE args[4];
  args[0] = rb_str_new (expr, long (strlen (expr)));
  //  use the current binding if there is one. This allows using "eval" in the context of a current trace callback
  //  when eval is called from the trace handler.
  if (context < 0) {
    args[1] = rb_const_get (rb_cObject, rb_intern ("TOPLEVEL_BINDING")); // taken from ruby.c
  }
#if HAVE_RUBY_VERSION_CODE >= 20200
  else if (context > 0) {
    args[1] = rb_debug_inspector_open (debug_inspector_get_binding, reinterpret_cast<void *> (long (context)));
  }
#endif
  else {
    //  TODO: throw an exception if context > 0 - for Ruby 1.x
    args[1] = rb_binding_new ();
  }
  if (file) {
    args[2] = rb_str_new (file, long (strlen (file)));
    args[3] = INT2NUM(line);
    argc = 4;
  } else {
    argc = 2;
  }

  return rba_f_eval_checked (argc, args, rb_get_top_self ());
}

}

#endif
