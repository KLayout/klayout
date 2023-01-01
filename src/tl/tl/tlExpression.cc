
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


#include "tlExpression.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlGlobPattern.h"
#include "tlFileUtils.h"
#include "tlEnv.h"

#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#define _USE_MATH_DEFINES // for MSVC
#include <math.h>
#include <string.h>
#include <ctype.h>

//  Suggestions for further functions:
//  - provide date/time function
//  - file functions: (drive), combine, split_path
//  - file tests: exists, locate (in path), readable, writeable 
//  - glob: match, expand (files) - needs glob code

namespace tl
{

static std::locale c_locale ("C");

// ----------------------------------------------------------------------------
//  Implementation of EvalError

EvalError::EvalError (const std::string &what, const ExpressionParserContext &context)
  : tl::Exception (what + tl::to_string (tr (" at ")) + context.where ())
{
  // .. nothing yet ..
}

// ----------------------------------------------------------------------------
//  Implementation of NoMethodError

NoMethodError::NoMethodError (const std::string &cls_name, const std::string &method, const ExpressionParserContext &context)
  : EvalError (tl::sprintf (tl::to_string (tr ("'%s' is not a valid method name for objects of class '%s'")), method, cls_name), context)
{ 
  // .. nothing yet ..
}

// ----------------------------------------------------------------------------
//  Implementation of ExpressionParserContext

ExpressionParserContext::ExpressionParserContext ()
  : mp_expr (0)
{
  //  .. nothing yet ..
}

ExpressionParserContext::ExpressionParserContext (const Expression *expr, const tl::Extractor &ex)
  : tl::Extractor (ex), mp_expr (expr), m_ex0 (ex)
{
  //  .. nothing yet ..
}

void 
ExpressionParserContext::error (const std::string &message)
{
  throw EvalError (message, *this);
}

std::string 
ExpressionParserContext::where () const
{
  if (mp_expr) {

    size_t pos = get () - m_ex0.get ();

    const char *text = mp_expr->text ();
    size_t len = strlen (text);

    if (pos >= len) {
      return tl::to_string (tr ("end of text"));
    } else {

      int line = 1;
      size_t col = 0;
      for (size_t p = 0; p < pos; ++p) {
        if (text [p] == '\n') {
          ++line;
          col = 1;
        } else if (text [p] != '\r') {
          ++col;
        }
      }

      std::ostringstream os;

      if (line == 1) {
        os << tl::to_string (tr ("position")) << " " << pos;
      } else {
        os << tl::to_string (tr ("line")) << " " << line << ", " << tl::to_string (tr ("position")) << " " << col;
      }

      os << " (";
      if (pos > 0) {
        os << "..";
      }
      for (int i = 0; i < 20 && pos < len; ++i) {
        os << text [pos++];
      }
      if (pos < len) {
        os << "..";
      }
      os << ")";

      return os.str ();

    }

  } else {
    return std::string (tl::to_string (tr ("[unspecified location]")));
  }
}

// ----------------------------------------------------------------------------
//  Utilities for evaluation

static double to_double (const ExpressionParserContext &context, const tl::Variant &v)
{
  if (v.can_convert_to_double ()) {
    return v.to_double ();
  } else if (v.is_list ()) {
    return v.get_list ().size ();
  } else {
    throw EvalError (tl::to_string (tr ("Double precision floating point value expected")), context);
  }
}

static double to_double (const ExpressionParserContext &context, const std::vector <tl::Variant> &v)
{
  if (v.size () != 1) {
    throw EvalError (tl::to_string (tr ("Function expects a single numeric argument")), context);
  }

  return to_double (context, v [0]);
}

static long to_long (const ExpressionParserContext &context, const tl::Variant &v)
{
  if (v.can_convert_to_long ()) {
    return v.to_long ();
  } else if (v.is_list ()) {
    return long (v.get_list ().size ());
  } else {
    throw EvalError (tl::to_string (tr ("Integer value expected")), context);
  }
}

static unsigned long to_ulong (const ExpressionParserContext &context, const tl::Variant &v)
{
  if (v.can_convert_to_ulong ()) {
    return v.to_ulong ();
  } else if (v.is_list ()) {
    return (unsigned long) (v.get_list ().size ());
  } else {
    throw EvalError (tl::to_string (tr ("Unsigned integer value expected")), context);
  }
}

static long long to_longlong (const ExpressionParserContext &context, const tl::Variant &v)
{
  if (v.can_convert_to_longlong ()) {
    return v.to_longlong ();
  } else if (v.is_list ()) {
    return long (v.get_list ().size ());
  } else {
    throw EvalError (tl::to_string (tr ("Integer value expected")), context);
  }
}

static unsigned long long to_ulonglong (const ExpressionParserContext &context, const tl::Variant &v)
{
  if (v.can_convert_to_ulonglong ()) {
    return v.to_ulong ();
  } else if (v.is_list ()) {
    return (unsigned long long) (v.get_list ().size ());
  } else {
    throw EvalError (tl::to_string (tr ("Unsigned integer value expected")), context);
  }
}

// ----------------------------------------------------------------------------
//  EvalTarget: a class that encapsulates the target of an evaluation

class TL_PUBLIC EvalTarget
{
public:
  EvalTarget ()
    : mp_lvalue (0), m_rvalue ()
  {
    //  .. nothing yet ..
  }

  void fetch () 
  {
    if (mp_lvalue) {
      m_rvalue = *mp_lvalue;
      mp_lvalue = 0;
    }
  }

  void set (const tl::Variant &v) 
  {
    m_rvalue = v;
    mp_lvalue = 0;
  }

  tl::Variant &get () 
  {
    if (mp_lvalue != 0) {
      return *mp_lvalue;
    } else {
      return m_rvalue;
    }
  }

  tl::Variant *lvalue () 
  {
    return mp_lvalue;
  }

  void set_lvalue (tl::Variant *lvalue) 
  {
    mp_lvalue = lvalue;
    m_rvalue.reset ();
  }

  const tl::Variant *operator-> () const
  {
    if (mp_lvalue != 0) {
      return mp_lvalue;
    } else {
      return &m_rvalue;
    }
  }

  const tl::Variant &operator* () const
  {
    if (mp_lvalue != 0) {
      return *mp_lvalue;
    } else {
      return m_rvalue;
    }
  }

  void swap (tl::Variant &other)
  {
    fetch ();
    m_rvalue.swap (other);
  }

  inline tl::Variant make_result ()
  {
    if (mp_lvalue != 0) {
      //  Make reference from ownership relation
      tl::Object *tl_object = mp_lvalue->to_object ();
      if (tl_object && !mp_lvalue->user_is_ref ()) {
        return tl::Variant (tl_object, mp_lvalue->user_cls (), false);
      } else {
        return *mp_lvalue;
      }
    } else {
      return m_rvalue;
    }
  }

private:
  tl::Variant *mp_lvalue;
  tl::Variant m_rvalue;
};

// ----------------------------------------------------------------------------
//  A class object for the tl::Variant list 

class TL_PUBLIC ListClass
  : public EvalClass
{
public:
  void execute (const ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const
  {
    if (method == "push") {

      if (args.size () != 1) {
        throw EvalError (tl::to_string (tr ("'push' method expects one argument")), context);
      }

      object.push (args [0]);
      out = args [0];

    } else if (method == "size") {
      
      if (args.size () != 0) {
        throw EvalError (tl::to_string (tr ("'size' method does not accept an argument")), context);
      }

      out = object.size ();
    
    } else {
      throw EvalError (tl::to_string (tr ("Unknown method")) + " '" + method + "' for list", context);
    }

  }

  static ListClass instance;
};

ListClass ListClass::instance;

// ----------------------------------------------------------------------------
//  A class object for the tl::Variant array 

class TL_PUBLIC ArrayClass
  : public EvalClass
{
public:
  void execute (const ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const
  {
    if (method == "insert") {

      if (args.size () != 2) {
        throw EvalError (tl::to_string (tr ("'insert' method expects two arguments")), context);
      }

      object.insert (args [0], args [1]);
      out = args [1];

    } else if (method == "size") {
      
      if (args.size () != 0) {
        throw EvalError (tl::to_string (tr ("'size' method does not accept an argument")), context);
      }

      out = object.array_size ();
    
    } else if (method == "keys") {

      if (args.size () != 0) {
        throw EvalError (tl::to_string (tr ("'keys' method does not accept an argument")), context);
      }

      out.set_list (object.array_size ());
      for (tl::Variant::const_array_iterator a = object.begin_array (); a != object.end_array (); ++a) {
        out.push (a->first);
      }

    } else if (method == "values") {

      if (args.size () != 0) {
        throw EvalError (tl::to_string (tr ("'keys' method does not accept an argument")), context);
      }

      out.set_list (object.array_size ());
      for (tl::Variant::const_array_iterator a = object.begin_array (); a != object.end_array (); ++a) {
        out.push (a->second);
      }

    } else {
      throw EvalError (tl::to_string (tr ("Unknown method")) + " '" + method + "' for array", context);
    }

  }

  static ArrayClass instance;
};

ArrayClass ArrayClass::instance;

// ----------------------------------------------------------------------------
//  ExpressionNode implementation

ExpressionNode::ExpressionNode (const ExpressionParserContext &context)
  : m_context (context)
{
  // .. nothing yet ..
}

ExpressionNode::ExpressionNode (const ExpressionParserContext &context, size_t children)
  : m_context (context)
{
  m_c.reserve (children);
}

ExpressionNode::ExpressionNode (const ExpressionNode &other, const tl::Expression *expr)
  : m_context (other.m_context)
{
  m_context.set_expr (expr);
  m_c.reserve (other.m_c.size ());

  for (std::vector <ExpressionNode *>::const_iterator c = other.m_c.begin (); c != other.m_c.end (); ++c) {
    m_c.push_back ((*c)->clone (expr));
  }
}

ExpressionNode::~ExpressionNode ()
{
  for (std::vector <ExpressionNode *>::const_iterator c = m_c.begin (); c != m_c.end (); ++c) {
    delete *c;
  }
  m_c.clear ();
}

void 
ExpressionNode::add_child (ExpressionNode *node)
{
  m_c.push_back (node);
}

// ----------------------------------------------------------------------------
//  ExpressionNode implementations for some binary operators

/**
 *  @brief Assign operator node
 */
class TL_PUBLIC AssignExpressionNode
  : public ExpressionNode
{
public:
  AssignExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  AssignExpressionNode (const AssignExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new AssignExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget a;
    m_c[0]->execute (v);
    m_c[1]->execute (a);

    if (! v.lvalue ()) {
      throw EvalError (tl::to_string (tr ("Assignment needs a lvalue")), m_context);
    }
    a.swap (*v.lvalue ());
  }
};

/**
 *  @brief Less operator node
 */
class TL_PUBLIC LessExpressionNode
  : public ExpressionNode
{
public:
  LessExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  LessExpressionNode (const LessExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new LessExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "<", vv);
      v.swap (o);

    } else {
      v.set (tl::Variant (*v < *b));
    }
  }
};

/**
 *  @brief Less or equal operator node
 */
class TL_PUBLIC LessOrEqualExpressionNode
  : public ExpressionNode
{
public:
  LessOrEqualExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  LessOrEqualExpressionNode (const LessOrEqualExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new LessOrEqualExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "<=", vv);
      v.swap (o);

    } else {
      v.set (tl::Variant (*v < *b || *b == *v));
    }
  }
};

/**
 *  @brief Greater operator node
 */
class TL_PUBLIC GreaterExpressionNode
  : public ExpressionNode
{
public:
  GreaterExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  GreaterExpressionNode (const GreaterExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new GreaterExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), ">", vv);
      v.swap (o);

    } else {
      v.set (tl::Variant (*b < *v));
    }
  }
};

/**
 *  @brief Greater or equal operator node
 */
class TL_PUBLIC GreaterOrEqualExpressionNode
  : public ExpressionNode
{
public:
  GreaterOrEqualExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  GreaterOrEqualExpressionNode (const GreaterOrEqualExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new GreaterOrEqualExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), ">=", vv);
      v.swap (o);

    } else {
      v.set (tl::Variant (*b < *v || *b == *v));
    }
  }
};

/**
 *  @brief Equal operator node
 */
class TL_PUBLIC EqualExpressionNode
  : public ExpressionNode
{
public:
  EqualExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  EqualExpressionNode (const EqualExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new EqualExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "==", vv);
      v.swap (o);

    } else {
      v.set (tl::Variant (*b == *v));
    }
  }
};

/**
 *  @brief Not equal operator node
 */
class TL_PUBLIC NotEqualExpressionNode
  : public ExpressionNode
{
public:
  NotEqualExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  NotEqualExpressionNode (const NotEqualExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new NotEqualExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "!=", vv);
      v.swap (o);

    } else {
      v.set (tl::Variant (!(*b == *v)));
    }
  }
};

/**
 *  @brief Match operator node
 */
class TL_PUBLIC MatchExpressionNode
  : public ExpressionNode
{
public:
  MatchExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b, tl::Eval *eval)
    : ExpressionNode (context, 2), mp_eval (eval)
  {
    add_child (a);
    add_child (b);
  }

  MatchExpressionNode (const MatchExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), mp_eval (other.mp_eval)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new MatchExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "~", vv);
      v.swap (o);

      mp_eval->match_substrings ().clear ();

    } else {

      std::vector <std::string> substrings;
      v.set (tl::GlobPattern (b->to_string ()).match (v->to_string (), substrings));
      mp_eval->match_substrings ().swap (substrings);
      
    }
  }

private:
  tl::Eval *mp_eval;
};

/**
 *  @brief Match substring reference 
 */
class TL_PUBLIC MatchSubstringReferenceNode
  : public ExpressionNode
{
public:
  MatchSubstringReferenceNode (const ExpressionParserContext &context, tl::Eval *eval, int index)
    : ExpressionNode (context, 0), mp_eval (eval), m_index (index)
  {
    //  .. nothing yet ..
  }

  MatchSubstringReferenceNode (const MatchSubstringReferenceNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), mp_eval (other.mp_eval), m_index (other.m_index)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new MatchSubstringReferenceNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    if (int (mp_eval->match_substrings ().size ()) > m_index && m_index >= 0) {
      v.set (mp_eval->match_substrings () [m_index]);
    } else {
      v.set (tl::Variant ());
    }
  }

private:
  tl::Eval *mp_eval;
  int m_index;
};

/**
 *  @brief NoMatch operator node
 */
class TL_PUBLIC NoMatchExpressionNode
  : public ExpressionNode
{
public:
  NoMatchExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  NoMatchExpressionNode (const NoMatchExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new NoMatchExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "!~", vv);
      v.swap (o);

    } else {
      v.set (! tl::GlobPattern (b->to_string ()).match (v->to_string ()));
    }
  }
};

/**
 *  @brief Logical and expression node
 */
class TL_PUBLIC LogAndExpressionNode
  : public ExpressionNode
{
public:
  LogAndExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  LogAndExpressionNode (const LogAndExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new LogAndExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    if (v->is_user ()) {

      //  an object always evaluates to "true"
      m_c[1]->execute (v);

    } else {
      if (v->to_bool ()) {
        m_c[1]->execute (v);
      }
    }
  }
};

/**
 *  @brief Logical or expression node
 */
class TL_PUBLIC LogOrExpressionNode
  : public ExpressionNode
{
public:
  LogOrExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  LogOrExpressionNode (const LogOrExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new LogOrExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    if (v->is_user ()) {

      //  an object always evaluates to "true"

    } else {
      if (! v->to_bool ()) {
        m_c[1]->execute (v);
      }
    }
  }
};

/**
 *  @brief '?:' operator expression node
 */
class TL_PUBLIC IfExpressionNode
  : public ExpressionNode
{
public:
  IfExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b, ExpressionNode *c)
    : ExpressionNode (context, 3)
  {
    add_child (a);
    add_child (b);
    add_child (c);
  }

  IfExpressionNode (const IfExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new IfExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    if (v->to_bool ()) {
      m_c[1]->execute (v);
    } else {
      m_c[2]->execute (v);
    }
  }
};

/**
 *  @brief Shift left expression node
 */
class TL_PUBLIC ShiftLeftExpressionNode
  : public ExpressionNode
{
public:
  ShiftLeftExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  ShiftLeftExpressionNode (const ShiftLeftExpressionNode &other,const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new ShiftLeftExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "<<", vv);
      v.swap (o);

    } else if (v->is_longlong ()) {
      v.set (tl::Variant (v->to_longlong () << to_longlong (m_context, *b)));
    } else if (v->is_ulonglong ()) {
      v.set (tl::Variant (v->to_ulonglong () << to_ulonglong (m_context, *b)));
    } else if (v->is_ulong ()) {
      v.set (tl::Variant (v->to_ulong () << to_ulong (m_context, *b)));
    } else {
      v.set (tl::Variant (to_long (m_context, *v) << to_long (m_context, *b)));
    }
  }
};

/**
 *  @brief Shift right expression node
 */
class TL_PUBLIC ShiftRightExpressionNode
  : public ExpressionNode
{
public:
  ShiftRightExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  ShiftRightExpressionNode (const ShiftRightExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new ShiftRightExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), ">>", vv);
      v.swap (o);

    } else if (v->is_longlong ()) {
      v.set (tl::Variant (v->to_longlong () >> to_longlong (m_context, *b)));
    } else if (v->is_ulonglong ()) {
      v.set (tl::Variant (v->to_ulonglong () >> to_ulonglong (m_context, *b)));
    } else if (v->is_ulong ()) {
      v.set (tl::Variant (v->to_ulong () >> to_ulong (m_context, *b)));
    } else {
      v.set (tl::Variant (to_long (m_context, *v) >> to_long (m_context, *b)));
    }
  }
};

/**
 *  @brief Plus expression node
 */
class TL_PUBLIC PlusExpressionNode
  : public ExpressionNode
{
public:
  PlusExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  PlusExpressionNode (const PlusExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new PlusExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "+", vv);
      v.swap (o);

    } else if (v->is_a_string () || b->is_a_string ()) {
      v.set (tl::Variant (std::string (v->to_string ()) + b->to_string ()));
    } else if (v->is_double () || b->is_double ()) {
      v.set (tl::Variant (to_double (m_context, *v) + to_double (m_context, *b)));
    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      v.set (tl::Variant (to_ulonglong (m_context, *v) + to_ulonglong (m_context, *b)));
    } else if (v->is_longlong () || b->is_longlong ()) {
      v.set (tl::Variant (to_longlong (m_context, *v) + to_longlong (m_context, *b)));
    } else if (v->is_ulong () || b->is_ulong ()) {
      v.set (tl::Variant (to_ulong (m_context, *v) + to_ulong (m_context, *b)));
    } else if (v->is_long () || b->is_long ()) {
      v.set (tl::Variant (to_long (m_context, *v) + to_long (m_context, *b)));
    } else {
      v.set (tl::Variant (to_double (m_context, *v) + to_double (m_context, *b)));
    }
  }
};

/**
 *  @brief Minus expression node
 */
class TL_PUBLIC MinusExpressionNode
  : public ExpressionNode
{
public:
  MinusExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  MinusExpressionNode (const MinusExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new MinusExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "-", vv);
      v.swap (o);

    } else if (v->is_double () || b->is_double ()) {
      v.set (tl::Variant (to_double (m_context, *v) - to_double (m_context, *b)));
    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      v.set (tl::Variant (to_ulonglong (m_context, *v) - to_ulonglong (m_context, *b)));
    } else if (v->is_longlong () || b->is_longlong ()) {
      v.set (tl::Variant (to_longlong (m_context, *v) - to_longlong (m_context, *b)));
    } else if (v->is_ulong () || b->is_ulong ()) {
      v.set (tl::Variant (to_ulong (m_context, *v) - to_ulong (m_context, *b)));
    } else if (v->is_long () || b->is_long ()) {
      v.set (tl::Variant (to_long (m_context, *v) - to_long (m_context, *b)));
    } else {
      v.set (tl::Variant (to_double (m_context, *v) - to_double (m_context, *b)));
    }
  }
};

/**
 *  @brief Star expression node
 */
class TL_PUBLIC StarExpressionNode
  : public ExpressionNode
{
public:
  StarExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  StarExpressionNode (const StarExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new StarExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "*", vv);
      v.swap (o);

    } else if (v->is_a_string ()) {

      long x = to_long (m_context, *b);
      if (x < 0) {
        throw EvalError (tl::to_string (tr ("Numeric argument of '*' operator with string must be positive")), m_context);
      }

      std::string s;
      s.reserve (strlen (v->to_string ()) * size_t (x));
      while (x-- > 0) {
        s += v->to_string ();
      }

      v.set (tl::Variant (s));

    } else if (b->is_a_string ()) {

      long x = to_long (m_context, *v);
      if (x < 0) {
        throw EvalError (tl::to_string (tr ("Numeric argument of '*' operator with string must be positive")), m_context);
      }

      std::string s;
      s.reserve (strlen (b->to_string ()) * size_t (x));
      while (x-- > 0) {
        s += b->to_string ();
      }

      v.set (tl::Variant (s));

    } else if (v->is_double () || b->is_double ()) {
      v.set (tl::Variant (to_double (m_context, *v) * to_double (m_context, *b)));
    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      v.set (tl::Variant (to_ulonglong (m_context, *v) * to_ulonglong (m_context, *b)));
    } else if (v->is_longlong () || b->is_longlong ()) {
      v.set (tl::Variant (to_longlong (m_context, *v) * to_longlong (m_context, *b)));
    } else if (v->is_ulong () || b->is_ulong ()) {
      v.set (tl::Variant (to_ulong (m_context, *v) * to_ulong (m_context, *b)));
    } else if (v->is_long () || b->is_long ()) {
      v.set (tl::Variant (to_long (m_context, *v) * to_long (m_context, *b)));
    } else {
      v.set (tl::Variant (to_double (m_context, *v) * to_double (m_context, *b)));
    }
  }
};

/**
 *  @brief Slash expression node
 */
class TL_PUBLIC SlashExpressionNode
  : public ExpressionNode
{
public:
  SlashExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  SlashExpressionNode (const SlashExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new SlashExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "/", vv);
      v.swap (o);

    } else if (v->is_double () || b->is_double ()) {
      double d = to_double (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Division by zero")), m_context);
      }
      v.set (tl::Variant (to_double (m_context, *v) / d));
    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      unsigned long long d = to_ulonglong (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Division by zero")), m_context);
      }
      v.set (tl::Variant (to_ulonglong (m_context, *v) / d));
    } else if (v->is_longlong () || b->is_longlong ()) {
      long long d = to_longlong (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Division by zero")), m_context);
      }
      v.set (tl::Variant (to_longlong (m_context, *v) / d));
    } else if (v->is_ulong () || b->is_ulong ()) {
      unsigned long d = to_ulong (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Division by zero")), m_context);
      }
      v.set (tl::Variant (to_ulong (m_context, *v) / d));
    } else if (v->is_long () || b->is_long ()) {
      long d = to_long (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Division by zero")), m_context);
      }
      v.set (tl::Variant (to_long (m_context, *v) / d));
    } else {
      double d = to_double (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Division by zero")), m_context);
      }
      v.set (tl::Variant (to_double (m_context, *v) / d));
    }
  }
};

/**
 *  @brief Percent expression node
 */
class TL_PUBLIC PercentExpressionNode
  : public ExpressionNode
{
public:
  PercentExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  PercentExpressionNode (const PercentExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new PercentExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "%", vv);
      v.swap (o);

    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      unsigned long long d = to_ulonglong (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Modulo by zero")), m_context);
      }
      v.set (tl::Variant (to_ulonglong (m_context, *v) % d));
    } else if (v->is_longlong () || b->is_longlong ()) {
      long long d = to_longlong (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Modulo by zero")), m_context);
      }
      v.set (tl::Variant (to_longlong (m_context, *v) % d));
    } else if (v->is_ulong () || b->is_ulong ()) {
      unsigned long d = to_ulong (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Modulo by zero")), m_context);
      }
      v.set (tl::Variant (to_ulong (m_context, *v) % d));
    } else {
      long d = to_long (m_context, *b);
      if (d == 0) {
        throw EvalError (tl::to_string (tr ("Modulo by zero")), m_context);
      }
      v.set (tl::Variant (to_long (m_context, *v) % d));
    }
  }
};

/**
 *  @brief Ampersand expression node
 */
class TL_PUBLIC AmpersandExpressionNode
  : public ExpressionNode
{
public:
  AmpersandExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  AmpersandExpressionNode (const AmpersandExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new AmpersandExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "&", vv);
      v.swap (o);

    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      v.set (tl::Variant (to_ulonglong (m_context, *v) & to_ulonglong (m_context, *b)));
    } else if (v->is_longlong () || b->is_longlong ()) {
      v.set (tl::Variant (to_longlong (m_context, *v) & to_longlong (m_context, *b)));
    } else if (v->is_ulong () || b->is_ulong ()) {
      v.set (tl::Variant (to_ulong (m_context, *v) & to_ulong (m_context, *b)));
    } else {
      v.set (tl::Variant (to_long (m_context, *v) & to_long (m_context, *b)));
    }
  }
};

/**
 *  @brief Pipe expression node
 */
class TL_PUBLIC PipeExpressionNode
  : public ExpressionNode
{
public:
  PipeExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  PipeExpressionNode (const PipeExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new PipeExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "|", vv);
      v.swap (o);

    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      v.set (tl::Variant (to_ulonglong (m_context, *v) | to_ulonglong (m_context, *b)));
    } else if (v->is_longlong () || b->is_longlong ()) {
      v.set (tl::Variant (to_longlong (m_context, *v) | to_longlong (m_context, *b)));
    } else if (v->is_ulong () || b->is_ulong ()) {
      v.set (tl::Variant (to_ulong (m_context, *v) | to_ulong (m_context, *b)));
    } else {
      v.set (tl::Variant (to_long (m_context, *v) | to_long (m_context, *b)));
    }
  }
};

/**
 *  @brief Acute expression node
 */
class TL_PUBLIC AcuteExpressionNode
  : public ExpressionNode
{
public:
  AcuteExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  AcuteExpressionNode (const AcuteExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new AcuteExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget b;
    m_c[0]->execute (v);
    m_c[1]->execute (b);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*b);
      c->execute (m_context, o, v.get (), "^", vv);
      v.swap (o);

    } else if (v->is_ulonglong () || b->is_ulonglong ()) {
      v.set (tl::Variant (to_ulonglong (m_context, *v) ^ to_ulonglong (m_context, *b)));
    } else if (v->is_longlong () || b->is_longlong ()) {
      v.set (tl::Variant (to_longlong (m_context, *v) ^ to_longlong (m_context, *b)));
    } else if (v->is_ulong () || b->is_ulong ()) {
      v.set (tl::Variant (to_ulong (m_context, *v) ^ to_ulong (m_context, *b)));
    } else {
      v.set (tl::Variant (to_long (m_context, *v) ^ to_long (m_context, *b)));
    }
  }
};

/**
 *  @brief Index expression node
 */
class TL_PUBLIC IndexExpressionNode
  : public ExpressionNode
{
public:
  IndexExpressionNode (const ExpressionParserContext &context, ExpressionNode *a, ExpressionNode *b)
    : ExpressionNode (context, 2)
  {
    add_child (a);
    add_child (b);
  }

  IndexExpressionNode (const IndexExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new IndexExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    EvalTarget e;
    m_c[0]->execute (v);
    m_c[1]->execute (e);

    if (v->is_user ()) {

      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      vv.push_back (*e);
      c->execute (m_context, o, v.get (), "[]", vv);
      v.swap (o);

    } else if (v->is_list ()) {

      if (! e->can_convert_to_ulong ()) {
        throw EvalError (tl::to_string (tr ("Invalid index for [] operator")), m_context);
      }
      unsigned long i = e->to_ulong ();
      if (i >= (unsigned long) v->size ()) {
        v.set (tl::Variant ());
      } else if (v.lvalue ()) {
        v.set_lvalue (&(v.lvalue ()->begin () [i]));
      } else {
        v.set (v->begin () [i]);
      }

    } else if (v->is_array ()) {

      if (v.lvalue ()) {
        tl::Variant *x = v.lvalue ()->find (*e);
        if (! x) {
          v.set (tl::Variant ());
        } else {
          v.set_lvalue (x);
        }
      } else {
        const tl::Variant *x = v->find (*e);
        if (! x) {
          v.set (tl::Variant ());
        } else {
          v.set (*x);
        }
      }

    } else {
      throw EvalError (tl::to_string (tr ("[] operator expects a list or an array")), m_context);
    }
  }
};

/**
 *  @brief Unary minus expression node
 */
class TL_PUBLIC UnaryMinusExpressionNode
  : public ExpressionNode
{
public:
  UnaryMinusExpressionNode (const ExpressionParserContext &context, ExpressionNode *a)
    : ExpressionNode (context, 1)
  {
    add_child (a);
  }

  UnaryMinusExpressionNode (const UnaryMinusExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new UnaryMinusExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    if (v->is_user ()) {

      throw EvalError (tl::to_string (tr ("Unary minus not implemented for objects")), m_context);
      /*
      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      c->execute (this, o, v.get (), "-", vv);
      v.swap (o);
      */

    } else if (v->is_long ()) {
      v.set (-v->to_long ());
    } else if (v->is_ulong ()) {
      v.set (-long (v->to_ulong ()));
    } else if (v->is_longlong ()) {
      v.set (-v->to_longlong ());
    } else if (v->is_ulonglong ()) {
      v.set (-(long long)(v->to_ulonglong ()));
    } else {
      v.set (-to_double (m_context, *v));
    }
  }
};

/**
 *  @brief Unary tilde expression node
 */
class TL_PUBLIC UnaryTildeExpressionNode
  : public ExpressionNode
{
public:
  UnaryTildeExpressionNode (const ExpressionParserContext &context, ExpressionNode *a)
    : ExpressionNode (context, 1)
  {
    add_child (a);
  }

  UnaryTildeExpressionNode (const UnaryTildeExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new UnaryTildeExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    if (v->is_user ()) {

      throw EvalError (tl::to_string (tr ("Unary tilde not implemented for objects")), m_context);
      /*
      const EvalClass *c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::to_string (tr ("Not a valid object for a method call (not an object)")), m_context);
      }

      tl::Variant o;
      std::vector <tl::Variant> vv;
      c->execute (this, o, v.get (), "~", vv);
      v.swap (o);
      */

    } else if (v->is_ulong ()) {
      v.set (~v->to_ulong ());
    } else if (v->is_longlong ()) {
      v.set (~v->to_longlong ());
    } else if (v->is_ulonglong ()) {
      v.set (~v->to_ulonglong ());
    } else {
      v.set (~to_long (m_context, *v));
    }
  }
};

/**
 *  @brief Unary not expression node
 */
class TL_PUBLIC UnaryNotExpressionNode
  : public ExpressionNode
{
public:
  UnaryNotExpressionNode (const ExpressionParserContext &context, ExpressionNode *a)
    : ExpressionNode (context, 1)
  {
    add_child (a);
  }

  UnaryNotExpressionNode (const UnaryNotExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new UnaryNotExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    if (v->is_user ()) {
      //  objects act as true
      v.set (false);
    } else {
      v.set (! v->to_bool ());
    }
  }
};

/**
 *  @brief Constant expression node
 */
class TL_PUBLIC ConstantExpressionNode
  : public ExpressionNode
{
public:
  ConstantExpressionNode (const ExpressionParserContext &context, const tl::Variant &value)
    : ExpressionNode (context), m_value (value)
  {
    //  .. nothing yet ..
  }

  ConstantExpressionNode (const ConstantExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), m_value (other.m_value)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new ConstantExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    v.set (m_value);
  }

private:
  tl::Variant m_value;
};

/**
 *  @brief Evaluates a bracket expression in the context
 */
class TL_PUBLIC ContextEvaluationNode
  : public ExpressionNode
{
public:
  ContextEvaluationNode (const ExpressionParserContext &context, const ContextHandler *ctx_handler, ExpressionNode *a, bool double_bracket)
    : ExpressionNode (context, 1), mp_ctx_handler (ctx_handler), m_double_bracket (double_bracket)
  {
    add_child (a);
  }

  ContextEvaluationNode (const ContextEvaluationNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), mp_ctx_handler (other.mp_ctx_handler), m_double_bracket (other.m_double_bracket)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const
  {
    return new ContextEvaluationNode (*this, expr);
  }

  void execute (EvalTarget &v) const
  {
    m_c[0]->execute (v);
    std::string s = v->to_string ();
    if (m_double_bracket) {
      v.set (mp_ctx_handler->eval_double_bracket (s));
    } else {
      v.set (mp_ctx_handler->eval_bracket (s));
    }
  }

private:
  const ContextHandler *mp_ctx_handler;
  bool m_double_bracket;
};

/**
 *  @brief Method call expression node
 */
class TL_PUBLIC MethodExpressionNode
  : public ExpressionNode
{
public:
  MethodExpressionNode (const ExpressionParserContext &context, const std::string &method)
    : ExpressionNode (context), m_method (method)
  {
    //  .. nothing yet ..
  }

  MethodExpressionNode (const MethodExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), m_method (other.m_method)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new MethodExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    m_c[0]->execute (v);

    std::vector <tl::Variant> vv;
    vv.reserve (m_c.size () - 1);
    for (std::vector<ExpressionNode *>::const_iterator c = m_c.begin () + 1; c != m_c.end (); ++c) {
      EvalTarget a;
      (*c)->execute (a);
      vv.push_back (*a);
    }

    const EvalClass *c = 0;
    
    if (v->is_list ()) {
      c = &ListClass::instance;
    } else if (v->is_array ()) {
      c = &ArrayClass::instance;
    } else if (v->is_user ()) {
      c = v->user_cls () ? v->user_cls ()->eval_cls () : 0;
      if (! c) {
        throw EvalError (tl::sprintf (tl::to_string (tr ("Not a valid object for a method call (not an object) - value is %s")), v->to_parsable_string ()), m_context);
      }
    } else {
      throw EvalError (tl::sprintf (tl::to_string (tr ("Not a valid object for a method call (wrong type) - value is %s")), v->to_parsable_string ()), m_context);
    }

    tl::Variant o;
    c->execute (m_context, o, v.get (), m_method, vv);
    v.swap (o);
  }

private:
  std::string m_method;
};

/**
 *  @brief List expression node
 */
class TL_PUBLIC ListExpressionNode
  : public ExpressionNode
{
public:
  ListExpressionNode (const ExpressionParserContext &context)
    : ExpressionNode (context)
  {
    //  .. nothing yet ..
  }

  ListExpressionNode (const ListExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new ListExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    v.set (tl::Variant::empty_list ());
    v.get ().reserve (m_c.size ());

    for (std::vector<ExpressionNode *>::const_iterator c = m_c.begin (); c != m_c.end (); ++c) {
      EvalTarget a;
      (*c)->execute (a);
      v.get ().push (*a);
    }
  }
};

/**
 *  @brief Array expression node
 */
class TL_PUBLIC ArrayExpressionNode
  : public ExpressionNode
{
public:
  ArrayExpressionNode (const ExpressionParserContext &context)
    : ExpressionNode (context)
  {
    //  .. nothing yet ..
  }

  ArrayExpressionNode (const ArrayExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new ArrayExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    v.set (tl::Variant::empty_array ());
    for (std::vector<ExpressionNode *>::const_iterator c = m_c.begin (); c != m_c.end (); c += 2) {
      EvalTarget k, x;
      c[0]->execute (k);
      c[1]->execute (x);
      v.get ().insert (*k, *x);
    }
  }
};

/**
 *  @brief Sequence expression node
 */
class TL_PUBLIC SequenceExpressionNode
  : public ExpressionNode
{
public:
  SequenceExpressionNode (const ExpressionParserContext &context)
    : ExpressionNode (context)
  {
    //  .. nothing yet ..
  }

  SequenceExpressionNode (const SequenceExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new SequenceExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    for (std::vector<ExpressionNode *>::const_iterator c = m_c.begin (); c != m_c.end (); ++c) {
      (*c)->execute (v);
    }
  }
};

/**
 *  @brief Static function expression node
 */
class TL_PUBLIC StaticFunctionExpressionNode
  : public ExpressionNode
{
public:
  StaticFunctionExpressionNode (const ExpressionParserContext &context, const EvalFunction *func)
    : ExpressionNode (context), mp_func (func)
  {
    //  .. nothing yet ..
  }

  StaticFunctionExpressionNode (const StaticFunctionExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), mp_func (other.mp_func)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new StaticFunctionExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    std::vector<tl::Variant> vv;
    vv.reserve (m_c.size ());

    for (std::vector<ExpressionNode *>::const_iterator c = m_c.begin (); c != m_c.end (); ++c) {
      EvalTarget a;
      (*c)->execute (a);
      vv.push_back (*a);
    }

    tl::Variant o;
    mp_func->execute (m_context, o, vv);
    v.swap (o);
  }

private:
  const EvalFunction *mp_func;
};

/**
 *  @brief Variable expression node (as RValue)
 */
class TL_PUBLIC RVariableExpressionNode
  : public ExpressionNode
{
public:
  RVariableExpressionNode (const ExpressionParserContext &context, const tl::Variant *var)
    : ExpressionNode (context), mp_var (var)
  {
    //  .. nothing yet ..
  }

  RVariableExpressionNode (const RVariableExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), mp_var (other.mp_var)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new RVariableExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    v.set (*mp_var);
  }

private:
  const tl::Variant *mp_var;
};

/**
 *  @brief Variable expression node (as LValue)
 */
class TL_PUBLIC LVariableExpressionNode
  : public ExpressionNode
{
public:
  LVariableExpressionNode (const ExpressionParserContext &context, tl::Variant *var)
    : ExpressionNode (context), mp_var (var)
  {
    //  .. nothing yet ..
  }

  LVariableExpressionNode (const LVariableExpressionNode &other, const tl::Expression *expr)
    : ExpressionNode (other, expr), mp_var (other.mp_var)
  {
    //  .. nothing yet ..
  }

  ExpressionNode *clone (const tl::Expression *expr) const 
  {
    return new LVariableExpressionNode (*this, expr);
  }

  void execute (EvalTarget &v) const 
  {
    v.set_lvalue (mp_var);
  }

private:
  tl::Variant *mp_var;
};

// ----------------------------------------------------------------------------
//  Implementation of functions

static void
sin_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = sin (to_double (context, v));
}

static void
sinh_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = sinh (to_double (context, v));
}

static void
cos_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = cos (to_double (context, v));
}

static void
cosh_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = cosh (to_double (context, v));
}

static void
tan_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = tan (to_double (context, v));
}

static void
tanh_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = tanh (to_double (context, v));
}

static void
log_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = ::log (to_double (context, v));
}

static void
log10_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = log10 (to_double (context, v));
}

static void
exp_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = exp (to_double (context, v));
}

static void
floor_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = floor (to_double (context, v));
}

static void
ceil_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = ceil (to_double (context, v));
}

static void
round_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  //  VC++ does not have "round"
  //  out = round (to_double (context, v));
  out = floor (0.5 + to_double (context, v));
}

static void
sqrt_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = sqrt (to_double (context, v));
}

static void
abs_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  if (v.size () != 1) {
    throw EvalError (tl::to_string (tr ("'abs' function expects exactly one argument")), context);
  }

  if (v[0].is_long ()) {
    out = labs (v[0].to_long ());
  } else if (v[0].is_ulong ()) {
    out = v[0].to_ulong ();
  } else if (v[0].is_longlong ()) {
    out = llabs (v[0].to_longlong ());
  } else if (v[0].is_ulonglong ()) {
    out = v[0].to_ulonglong ();
  } else if (v[0].is_double ()) {
    out = fabs (v[0].to_double ());
  } else {
    out = labs (to_long (context, v[0]));
  }
}

static void
acos_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = acos (to_double (context, v));
}

#ifndef _MSC_VER // not available on MS VC++
static void
acosh_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = acosh (to_double (context, v));
}
#endif

static void
asin_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = asin (to_double (context, v));
}

#ifndef _MSC_VER // not available on MS VC++
static void
asinh_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = asinh (to_double (context, v));
}
#endif

static void
atan_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = atan (to_double (context, v));
}

#ifndef _MSC_VER // not available on MS VC++
static void
atanh_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v)
{
  out = atanh (to_double (context, v));
}
#endif

static void
min_f (const ExpressionParserContext &, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  out = tl::Variant ();
  for (std::vector <tl::Variant>::const_iterator v = vv.begin (); v != vv.end (); ++v) {
    if (! v->is_nil () && (out.is_nil () || *v < out)) {
      out = *v;
    }
  }
}

static void
max_f (const ExpressionParserContext &, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  out = tl::Variant ();
  for (std::vector <tl::Variant>::const_iterator v = vv.begin (); v != vv.end (); ++v) {
    if (! v->is_nil () && (out.is_nil () || out < *v)) {
      out = *v;
    }
  }
}

static void
pow_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'pow' function expects exactly two arguments")), context);
  }

  out = pow (to_double (context, vv [0]), to_double (context, vv [1]));
}

static void
atan2_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'atan2' function expects exactly two arguments")), context);
  }

  out = atan2 (to_double (context, vv [0]), to_double (context, vv [1]));
}

static void
to_f_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'to_f' function expects exactly one argument")), context);
  }

  out = vv [0].to_double ();
}

static void
to_s_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'to_s' function expects exactly one argument")), context);
  }

  out = vv [0].to_string ();
}

static void
to_i_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'to_i' function expects exactly one argument")), context);
  }

  out = vv [0].to_long ();
}

static void
to_ui_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'to_ui' function expects exactly one argument")), context);
  }

  out = vv [0].to_ulong ();
}

static void
to_l_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'to_l' function expects exactly one argument")), context);
  }

  out = vv [0].to_longlong ();
}

static void
to_ul_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'to_ul' function expects exactly one argument")), context);
  }

  out = vv [0].to_ulonglong ();
}

static void
is_string_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'is_string' function expects exactly one argument")), context);
  }

  out = vv[0].is_a_string ();
}

static void
is_numeric_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'is_numeric' function expects exactly one argument")), context);
  }

  out = vv [0].can_convert_to_double ();
}

static void
is_array_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'is_array' function expects exactly one argument")), context);
  }

  out = vv [0].is_list ();
}

static void
is_nil_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'is_nil' function expects exactly one argument")), context);
  }

  out = vv [0].is_nil ();
}

static void
gsub_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 3) {
    throw EvalError (tl::to_string (tr ("'gsub' function expects exactly three arguments")), context);
  }

  std::string s (vv [0].to_string ());
  std::string x (vv [1].to_string ());
  std::string y (vv [2].to_string ());

  std::string r;
  r.reserve (s.size ());

  size_t p = 0;
  for (size_t pp = 0; (pp = s.find (x, p)) != std::string::npos; p = pp + x.size ()) {
    r += std::string (s, p, pp - p);
    r += y;
  }

  r += std::string (s, p, std::string::npos);

  out = r;
}

static void
sub_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 3) {
    throw EvalError (tl::to_string (tr ("'sub' function expects exactly three arguments")), context);
  }

  std::string s (vv [0].to_string ());
  std::string x (vv [1].to_string ());
  std::string y (vv [2].to_string ());

  std::string r;

  size_t p = s.find (x);
  if (p != std::string::npos) {

    r.reserve (s.size () + y.size () - x.size ());
    r += std::string (s, 0, p);
    r += y;
    r += std::string (s, p + x.size ());
    out = r;

  } else {
    out = s;
  }

}

static void
find_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'find' function expects exactly two arguments")), context);
  }

  std::string s (vv [0].to_string ());
  std::string x (vv [1].to_string ());

  size_t p = s.find (x);
  if (p != std::string::npos) {
    out = long (p);
  } else {
    out = tl::Variant ();
  }
}

static void
rfind_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'rfind' function expects exactly two arguments")), context);
  }

  std::string s (vv [0].to_string ());
  std::string x (vv [1].to_string ());

  size_t p = s.rfind (x);
  if (p != std::string::npos) {
    out = long (s.size () - (p + x.size ()));
  } else {
    out = tl::Variant ();
  }
}

static void
len_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'len' function expects exactly one argument")), context);
  }

  if (vv [0].is_list ()) {
    out = long (vv [0].end () - vv [0].begin ());
  } else {
    out = long (strlen (vv [0].to_string ()));
  }
}

static void
substr_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 3 && vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'substr' function expects two or three arguments")), context);
  }

  std::string s (vv [0].to_string ());

  long len = -1;
  if (vv.size () > 2) {
    len = std::max (long (0), to_long (context, vv [2]));
  }

  long l = to_long (context, vv [1]);
  if (l < 0) {
    l = long (s.size ()) + l;
    if (l < 0) {
      len += l;
      l = 0;
    }
  } 

  size_t from = size_t (l);

  if (len == 0 || from >= s.size ()) {
    out = tl::Variant ("");
  } else if (len < 0 || from + len >= s.size ()) {
    out = std::string (s, from);
  } else {
    out = std::string (s, from, len);
  }
}

static void
join_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'join' function expects exactly two arguments")), context);
  }

  if (! vv[0].is_list ()) {
    throw EvalError (tl::to_string (tr ("First argument of 'join' function must be a list")), context);
  }

  std::ostringstream r;
  r.imbue (c_locale);

  std::string s (vv [1].to_string ());

  bool first = true;
  for (tl::Variant::const_iterator i = vv [0].begin (); i != vv [0].end (); ++i) {
    if (first) {
      r << s;
      first = false;
    }
    r << i->to_string ();
  }

  out = r.str ();
}

static void
item_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'item' function expects exactly two arguments")), context);
  }

  if (! vv[0].is_list ()) {
    throw EvalError (tl::to_string (tr ("First argument of 'item' function must be a list")), context);
  }

  long index = to_long (context, vv [1]);
  if (index < 0 || index >= long (vv [0].end () - vv [0].begin ())) {
    out = tl::Variant ();
  } else {
    out = *(vv [0].begin () + index);
  }
}

static void
split_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'split' function expects exactly two arguments")), context);
  }

  out = tl::Variant::empty_list ();
  std::string t (vv [0].to_string ()); 
  std::string s (vv [1].to_string ());

  size_t p = 0;
  for (size_t pp = 0; (pp = t.find (s, p)) != std::string::npos; p = pp + s.size ()) {
    out.push (tl::Variant (std::string (t, p, pp - p)));
  }

  out.push (tl::Variant (std::string (t, p)));
}

static void
true_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 0) {
    throw EvalError (tl::to_string (tr ("'true' function must not have arguments")), context);
  }

  out = true;
}

static void
false_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 0) {
    throw EvalError (tl::to_string (tr ("'false' function must not have arguments")), context);
  }

  out = false;
}

static void
nil_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 0) {
    throw EvalError (tl::to_string (tr ("'nil' function must not have arguments")), context);
  }

  out = tl::Variant ();
}

static void
env_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'env' function expects exactly two arguments")), context);
  }

  const char *vn = vv [0].to_string ();
  if (tl::has_env (vn)) {
    out = tl::get_env (vn);
  } else {
    out = tl::Variant ();
  }
}

static void
error_f (const ExpressionParserContext &context, tl::Variant &, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'error' function expects exactly one argument")), context);
  }

  throw tl::Exception (vv [0].to_string ());
}

static void
absolute_file_path_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'absolute_file_path' function expects exactly one argument")), context);
  }

  out = tl::absolute_file_path (vv [0].to_string ());
}

static void
absolute_path_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'absolute_path' function expects exactly one argument")), context);
  }

  out = tl::absolute_path (vv [0].to_string ());
}

static void
path_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'path' function expects exactly one argument")), context);
  }

  out = tl::dirname (vv [0].to_string ());
}

static void
basename_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'basename' function expects exactly one argument")), context);
  }

  out = tl::basename (vv [0].to_string ());
}

static void
extension_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'extension' function expects exactly one argument")), context);
  }

  out = tl::extension (vv [0].to_string ());
}

static void
file_exists_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'file_exists' function expects exactly one argument")), context);
  }

  out = tl::file_exists (vv [0].to_string ());
}

static void
is_dir_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 1) {
    throw EvalError (tl::to_string (tr ("'is_dir' function expects exactly one argument")), context);
  }

  out = tl::is_dir (vv [0].to_string ());
}

static void
combine_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () != 2) {
    throw EvalError (tl::to_string (tr ("'combine' function expects two arguments")), context);
  }

  out = tl::combine_path (vv [0].to_string (), vv [1].to_string ());
}

static void
sprintf_f (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv)
{
  if (vv.size () < 1) {
    throw EvalError (tl::to_string (tr ("'sprintf' function expects at least one argument")), context);
  }

  out = tl::sprintf (vv[0].to_string (), vv, 1);
}

static void
printf_f (const ExpressionParserContext &context, tl::Variant &, const std::vector <tl::Variant> &vv)
{
  if (vv.size () < 1) {
    throw EvalError (tl::to_string (tr ("'printf' function expects at least one argument")), context);
  }

  std::cout << tl::sprintf (vv[0].to_string (), vv, 1);
  std::cout.flush ();
}

// ----------------------------------------------------------------------------
//  Definition of a function wrapper

class EvalStaticFunction 
  : public EvalFunction
{
  typedef void (*function_t) (const ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &v);

public:
  EvalStaticFunction (const std::string &name, function_t func)
    : m_func (func), m_name (name)
  {
    ms_functions.insert (std::make_pair (name, this));
  }

  ~EvalStaticFunction ()
  {
    ms_functions.erase (m_name);
  }

  void execute (const ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args) const 
  {
    m_func (context, out, args);
  }

  static EvalFunction *function_by_name (const std::string &name)
  {
    std::map <std::string, EvalFunction *>::const_iterator f = ms_functions.find (name);
    if (f != ms_functions.end ()) {
      return f->second;
    } else {
      return 0;
    }
  }

private:
  function_t m_func;
  std::string m_name;

  static std::map <std::string, EvalFunction *> ms_functions;
};

std::map <std::string, EvalFunction *> EvalStaticFunction::ms_functions;

// ----------------------------------------------------------------------------
//  Implementation of the function table

static EvalStaticFunction f1 ("sin", &sin_f);
static EvalStaticFunction f2 ("sinh", &sinh_f);
static EvalStaticFunction f3 ("cos", &cos_f);
static EvalStaticFunction f4 ("cosh", &cosh_f);
static EvalStaticFunction f5 ("tan", &tan_f);
static EvalStaticFunction f6 ("tanh", &tanh_f);
static EvalStaticFunction f7 ("log", &log_f);
static EvalStaticFunction f8 ("log10", &log10_f);
static EvalStaticFunction f9 ("exp", &exp_f);
static EvalStaticFunction f10 ("floor", &floor_f);
static EvalStaticFunction f11 ("ceil", &ceil_f);
static EvalStaticFunction f12 ("round", &round_f);
static EvalStaticFunction f13 ("sqrt", &sqrt_f);
static EvalStaticFunction f14 ("max", &max_f);
static EvalStaticFunction f15 ("min", &min_f);
static EvalStaticFunction f16 ("pow", &pow_f);
static EvalStaticFunction f17 ("acos", &acos_f);
#ifndef _MSC_VER // not available on MS VC++
static EvalStaticFunction f18 ("acosh", &acosh_f);
static EvalStaticFunction f19 ("asinh", &asinh_f);
static EvalStaticFunction f20 ("atanh", &atanh_f);
#endif
static EvalStaticFunction f21 ("asin", &asin_f);
static EvalStaticFunction f22 ("atan", &atan_f);
static EvalStaticFunction f23 ("atan2", &atan2_f);
static EvalStaticFunction f24 ("to_f", &to_f_f);
static EvalStaticFunction f25 ("to_s", &to_s_f);
static EvalStaticFunction f26 ("to_i", &to_i_f);
static EvalStaticFunction f27 ("to_ui", &to_ui_f);
static EvalStaticFunction f28 ("to_l", &to_l_f);
static EvalStaticFunction f29 ("to_ul", &to_ul_f);
static EvalStaticFunction f30 ("is_string", &is_string_f);
static EvalStaticFunction f31 ("is_numeric", &is_numeric_f);
static EvalStaticFunction f32 ("is_array", &is_array_f);
static EvalStaticFunction f33 ("is_nil", &is_nil_f);
static EvalStaticFunction f34 ("join", &join_f);
static EvalStaticFunction f35 ("split", &split_f);
static EvalStaticFunction f36 ("item", &item_f);
static EvalStaticFunction f37 ("sub", &sub_f);
static EvalStaticFunction f38 ("gsub", &gsub_f);
static EvalStaticFunction f39 ("find", &find_f);
static EvalStaticFunction f40 ("rfind", &rfind_f);
static EvalStaticFunction f41 ("len", &len_f);
static EvalStaticFunction f42 ("substr", &substr_f);
static EvalStaticFunction f43 ("env", &env_f);
static EvalStaticFunction f44 ("error", &error_f);
static EvalStaticFunction f45 ("sprintf", &sprintf_f);
static EvalStaticFunction f46 ("printf", &printf_f);
static EvalStaticFunction f47 ("false", &false_f);
static EvalStaticFunction f48 ("true", &true_f);
static EvalStaticFunction f49 ("nil", &nil_f);
static EvalStaticFunction f50 ("absolute_file_path", &absolute_file_path_f);
static EvalStaticFunction f51 ("absolute_path", &absolute_path_f);
static EvalStaticFunction f52 ("path", &path_f);
static EvalStaticFunction f53 ("basename", &basename_f);
static EvalStaticFunction f54 ("extension", &extension_f);
static EvalStaticFunction f55 ("file_exists", &file_exists_f);
static EvalStaticFunction f56 ("is_dir", &is_dir_f);
static EvalStaticFunction f57 ("combine", &combine_f);
static EvalStaticFunction f58 ("abs", &abs_f);

// ----------------------------------------------------------------------------
//  Implementation of a constant wrapper

class EvalStaticConstant 
{
public:
  EvalStaticConstant (const std::string &name, const tl::Variant &v)
    : m_var (v), m_name (name)
  {
    ms_constants.insert (std::make_pair (name, v));
  }

  static const tl::Variant *constant_by_name (const std::string &name)
  {
    std::map <std::string, tl::Variant>::const_iterator c = ms_constants.find (name);
    if (c != ms_constants.end ()) {
      return &c->second;
    } else {
      return 0;
    }
  }

private:
  tl::Variant m_var;
  std::string m_name;

  static std::map <std::string, tl::Variant> ms_constants;
};

std::map <std::string, tl::Variant> EvalStaticConstant::ms_constants;

// ----------------------------------------------------------------------------
//  Implementation of the constant table

static EvalStaticConstant c1 ("M_PI", tl::Variant (M_PI));
static EvalStaticConstant c2 ("M_E", tl::Variant (M_E));

// ----------------------------------------------------------------------------
//  Implementation of Expression

Expression::Expression ()
  : mp_text (0), mp_eval (0)
{
  // .. nothing yet ..
}

Expression::Expression (const Expression &d)
  : mp_text (0), mp_eval (0)
{
  operator= (d);
}

Expression::Expression (Eval *eval, const std::string &expr)
  : mp_text (0), m_local_text (expr), mp_eval (eval)
{
  // .. nothing yet ..
}

Expression::Expression (Eval *eval, const char *expr)
  : mp_text (expr), mp_eval (eval)
{
  // .. nothing yet ..
}

Expression &
Expression::operator= (const Expression &d)
{
  if (&d != this) {
    mp_eval = d.mp_eval;
    m_local_text = d.m_local_text;
    mp_text = d.mp_text;
    if (d.m_root.get ()) {
      m_root.reset (d.m_root->clone (this));
    } else {
      m_root.reset (0);
    }
  }
  return *this;
}

tl::Variant 
Expression::execute () const
{
  EvalTarget v;
  execute (v);
  return v.make_result ();
}

void
Expression::execute (EvalTarget &v) const
{
  if (m_root.get ()) {
    m_root->execute (v);
  } 
}

// ----------------------------------------------------------------------------
//  Implementation of Eval

Eval Eval::m_global (0, 0, false);

Eval::Eval (Eval *parent, bool sloppy)
  : mp_parent (parent), mp_global (&Eval::m_global), m_sloppy (sloppy), mp_ctx_handler (0)
{
  // .. nothing yet ..
}

Eval::Eval (Eval *global, Eval *parent, bool sloppy)
  : mp_parent (parent), mp_global (global), m_sloppy (sloppy), mp_ctx_handler (0)
{
  // .. nothing yet ..
}

Eval::~Eval ()
{
  for (std::map <std::string, EvalFunction *>::iterator f = m_local_functions.begin (); f != m_local_functions.end (); ++f) {
    delete f->second;
  }
  m_local_functions.clear ();
}

void 
Eval::set_var (const std::string &name, const tl::Variant &var)
{
  m_local_vars.insert (std::make_pair (name, tl::Variant ())).first->second = var;
}

void 
Eval::define_function (const std::string &name, EvalFunction *function)
{
  EvalFunction *&f = m_local_functions.insert (std::make_pair (name, (EvalFunction *) 0)).first->second;
  if (f != 0) {
    delete f;
  }
  f = function;
}

void
Eval::eval_top (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  do {

    if (ex.test ("#")) {

      //  ignore comments after "#"
      while (*ex && *ex != '\n') {
        ++ex;
      }

    } else {

      std::unique_ptr<ExpressionNode> nn;

      ExpressionParserContext ex1 = ex;

      if (ex.test ("var")) {

        eval_atomic (ex, nn, 2);

        ExpressionParserContext exb = ex;
        if (! exb.test ("=>") && ! exb.test ("==") && ex.test ("=")) {

          std::unique_ptr<ExpressionNode> b;
          eval_assign (ex, b);
          nn.reset (new AssignExpressionNode (ex1, nn.release (), b.release ()));

        }

      } else {
        eval_assign (ex, nn);
      }

      if (! n.get ()) {
        n.reset (nn.release ());
      } else if (dynamic_cast <SequenceExpressionNode *> (n.get ())) {
        n->add_child (nn.release ());
      } else {
        SequenceExpressionNode *m = new SequenceExpressionNode (ex);
        m->add_child (n.release ());
        m->add_child (nn.release ());
        n.reset (m);
      }

      if (!ex.test (";")) {
        return;
      }

    }

  } while (! ex.at_end ());
}

void
Eval::eval_assign (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_if (ex, n);

  ExpressionParserContext ex1 = ex;
  tl::Extractor exb = ex;
  if (! exb.test ("=>") && ! exb.test ("==") && ex.test ("=")) {

    exb = ex;
    std::unique_ptr<ExpressionNode> b;
    eval_assign (ex, b);
    n.reset (new AssignExpressionNode (ex1, n.release (), b.release ()));

  }
}

void
Eval::eval_if (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_boolean (ex, n);

  ExpressionParserContext ex1 = ex;
  if (ex.test ("?")) {

    std::unique_ptr<ExpressionNode> b, c;
    eval_if (ex, b);
    if (! ex.test (":")) {
      throw EvalError (tl::to_string (tr ("Expected ':'")), ex);
    }
    eval_if (ex, c);
    n.reset (new IfExpressionNode (ex1, n.release (), b.release (), c.release ()));

  }
}

void
Eval::eval_boolean (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_conditional (ex, n);

  while (true) {

    ExpressionParserContext ex1 = ex;
    if (ex.test("||")) {

      std::unique_ptr<ExpressionNode> b;
      eval_conditional (ex, b);
      n.reset (new LogOrExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test ("&&")) {

      std::unique_ptr<ExpressionNode> b;
      eval_conditional (ex, b);
      n.reset (new LogAndExpressionNode (ex1, n.release (), b.release ()));

    } else {
      break;
    }

  }
}

void
Eval::eval_conditional (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_shift (ex, n);

  while (true) {

    ExpressionParserContext ex1 = ex;
    if (ex.test("<=")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new LessOrEqualExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("<")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new LessExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test(">=")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new GreaterOrEqualExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test(">")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new GreaterExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("==")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new EqualExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("!=")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new NotEqualExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("~")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new MatchExpressionNode (ex1, n.release (), b.release (), this));

    } else if (ex.test("!~")) {

      std::unique_ptr<ExpressionNode> b;
      eval_shift (ex, b);
      n.reset (new NoMatchExpressionNode (ex1, n.release (), b.release ()));

    } else {
      break;
    }

  }
}

void
Eval::eval_shift (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_addsub (ex, n);

  while (true) {

    ExpressionParserContext ex1 = ex;
    if (ex.test("<<")) {

      std::unique_ptr<ExpressionNode> b;
      eval_addsub (ex, b);
      n.reset (new ShiftLeftExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test(">>")) {

      std::unique_ptr<ExpressionNode> b;
      eval_addsub (ex, b);
      n.reset (new ShiftRightExpressionNode (ex1, n.release (), b.release ()));

    } else {
      break;
    }

  }
}

void
Eval::eval_addsub (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_product (ex, n);

  while (true) {

    ExpressionParserContext ex1 = ex;
    if (ex.test("+")) {

      std::unique_ptr<ExpressionNode> b;
      eval_product (ex, b);
      n.reset (new PlusExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("-")) {

      std::unique_ptr<ExpressionNode> b;
      eval_product (ex, b);
      n.reset (new MinusExpressionNode (ex1, n.release (), b.release ()));

    } else {
      break;
    }

  }
}

void
Eval::eval_product (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_bitwise (ex, n);

  while (true) {

    ExpressionParserContext ex1 = ex;
    if (ex.test("*")) {

      std::unique_ptr<ExpressionNode> b;
      eval_bitwise (ex, b);
      n.reset (new StarExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("/")) {

      std::unique_ptr<ExpressionNode> b;
      eval_bitwise (ex, b);
      n.reset (new SlashExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("%")) {

      std::unique_ptr<ExpressionNode> b;
      eval_bitwise (ex, b);
      n.reset (new PercentExpressionNode (ex1, n.release (), b.release ()));

    } else {
      break;
    }

  }
}

void
Eval::eval_bitwise (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_unary (ex, n);

  while (true) {

    ExpressionParserContext ex1 = ex;
    tl::Extractor exb = ex;
    if (exb.test("||")) {
      break; // not handled here
    } else if (exb.test("&&")) {
      break; // not handled here
    } else if (ex.test("&")) {

      std::unique_ptr<ExpressionNode> b;
      eval_unary (ex, b);
      n.reset (new AmpersandExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("|")) {

      std::unique_ptr<ExpressionNode> b;
      eval_unary (ex, b);
      n.reset (new PipeExpressionNode (ex1, n.release (), b.release ()));

    } else if (ex.test("^")) {

      std::unique_ptr<ExpressionNode> b;
      eval_unary (ex, b);
      n.reset (new AcuteExpressionNode (ex1, n.release (), b.release ()));

    } else {
      break;
    }

  }
}

void
Eval::eval_unary (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  ExpressionParserContext ex1 = ex;
  if (ex.test ("!")) {

    eval_unary (ex, n);
    n.reset (new UnaryNotExpressionNode (ex1, n.release ()));

  } else if (ex.test ("-")) {

    eval_unary (ex, n);
    n.reset (new UnaryMinusExpressionNode (ex1, n.release ()));

  } else if (ex.test ("~")) {

    eval_unary (ex, n);
    n.reset (new UnaryTildeExpressionNode (ex1, n.release ()));
    
  } else {
    eval_suffix (ex, n);
  }
}

static const char *operator_methods[] = 
{
  "==", "[]", "()",
  "&&", "&", "||", "|", ">>", ">=", ">", "<<", "<=", "<",
  "++", "+", "--", "-", "^", "!~", "!=", "!", "~", "%", "*", "/",
  0
};

void
Eval::eval_suffix (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n)
{
  eval_atomic (ex, n, 1);

  while (true) {
    
    ExpressionParserContext ex1 = ex;
    if (ex.test (".")) {

      std::string t;
      //  check for operators
      for (const char **om = operator_methods; *om; ++om) {
        if (ex.test (*om)) {
          t = *om;
          break;
        }
      }
      //  normal method otherwise
      if (t.empty ()) {
        ex.read_word (t, "_");
      }

      tl::Extractor exb = ex;

      if (exb.test ("=>") || exb.test ("==")) { //  no handled here

        MethodExpressionNode *m = new MethodExpressionNode (ex1, t);
        m->add_child (n.release ());
        n.reset (m);

      } else if (ex.test ("=")) {

        t += "=";

        std::unique_ptr<ExpressionNode> a;
        eval_assign (ex, a);

        MethodExpressionNode *m = new MethodExpressionNode (ex1, t);
        m->add_child (n.release ());
        n.reset (m);

        m->add_child (a.release ());

      } else if (ex.test ("(")) {

        MethodExpressionNode *m = new MethodExpressionNode (ex1, t);
        m->add_child (n.release ());
        n.reset (m);

        if (! ex.test (")")) {

          do {

            std::unique_ptr<ExpressionNode> a;
            eval_assign (ex, a);
            m->add_child (a.release ());

            if (ex.test (")")) {
              break;
            } else if (! ex.test (",")) {
              throw EvalError (tl::to_string (tr ("Expected closing bracket ')'")), ex);
            }

          } while (true);

        }

      } else {

        MethodExpressionNode *m = new MethodExpressionNode (ex1, t);
        m->add_child (n.release ());
        n.reset (m);

      }

    } else if (ex.test ("[")) {

      std::unique_ptr<ExpressionNode> a;
      eval_top (ex, a);
      n.reset (new IndexExpressionNode (ex1, n.release (), a.release ()));

      ex.expect ("]");

    } else {
      break;
    }

  }
}

static void 
scan_angle_bracket (tl::Extractor &ex, const char *term, std::string &s)
{
  const char *p0 = ex.get ();

  while (! ex.at_end ()) {

    const char *p = ex.skip ();
    if (ex.test (term)) {
      while (p > p0 && isspace (p[-1])) {
        --p;
      }
      s = std::string (p0, 0, p - p0);
      return;
    }

    if (*ex == '\'' || *ex == '"') {
      std::string n;
      ex.read_quoted (n);
    } else {
      ++ex;
    }

  }

  ex.expect (term);
}

void
Eval::eval_atomic (ExpressionParserContext &ex, std::unique_ptr<ExpressionNode> &n, int am)
{
  double g = 0.0;
  std::string t;

  ExpressionParserContext ex1 = ex;
  if (ex.test ("(")) {

    eval_top (ex, n);
    if (! ex.test (")")) {
      throw EvalError (tl::to_string (tr ("Expected closing bracket ')'")), ex);
    }

  } else if (ex.test ("[")) {

    n.reset (new ListExpressionNode (ex1));

    if (! ex.test ("]")) {

      do {

        std::unique_ptr<ExpressionNode> a;
        eval_top (ex, a);
        n->add_child (a.release ());

        if (ex.test ("]")) {
          break;
        } else if (! ex.test (",")) {
          throw EvalError (tl::to_string (tr ("Expected closing bracket ']'")), ex);
        }

      } while (true);

    }

  } else if (ex.test ("<<")) {

    ExpressionParserContext ex0 = ex;
    if (ex.test ("$") || ex.test ("\"") || ex.test ("\'") || ex.test ("(")) {

      ex = ex0;
      eval_addsub (ex, n);
      ex.expect (">>");

      if (m_sloppy) {
        n.reset (new ConstantExpressionNode (ex1, tl::Variant ()));
      } else if (ctx_handler ()) {
        n.reset (new ContextEvaluationNode (ex1, ctx_handler (), n.release (), true /*double bracket*/));
      } else {
        throw EvalError (tl::to_string (tr ("<<..>> expression not available in this context")), ex1);
      }

    } else {

      std::string s;
      scan_angle_bracket (ex, ">>", s);

      if (m_sloppy) {
        n.reset (new ConstantExpressionNode (ex1, tl::Variant ()));
      } else if (ctx_handler ()) {
        n.reset (new ConstantExpressionNode (ex1, ctx_handler ()->eval_double_bracket (s)));
      } else {
        throw EvalError (tl::to_string (tr ("<<..>> expression not available in this context")), ex1);
      }

    }

  } else if (ex.test ("<")) {

    ExpressionParserContext ex0 = ex;
    if (ex.test ("$") || ex.test ("\"") || ex.test ("\'") || ex.test ("(")) {

      ex = ex0;
      eval_addsub (ex, n);
      ex.expect (">");

      if (m_sloppy) {
        n.reset (new ConstantExpressionNode (ex1, tl::Variant ()));
      } else if (ctx_handler ()) {
        n.reset (new ContextEvaluationNode (ex1, ctx_handler (), n.release (), false /*single bracket*/));
      } else {
        throw EvalError (tl::to_string (tr ("<<..>> expression not available in this context")), ex1);
      }

    } else {

      std::string s;
      scan_angle_bracket (ex, ">", s);

      if (m_sloppy) {
        n.reset (new ConstantExpressionNode (ex1, tl::Variant ()));
      } else if (ctx_handler ()) {
        n.reset (new ConstantExpressionNode (ex1, ctx_handler ()->eval_bracket (s)));
      } else {
        throw EvalError (tl::to_string (tr ("<..> expression not available in this context")), ex1);
      }

    }

  } else if (ex.test ("$")) {

    //  match substring
    int i = 0;
    ex.read (i);
    n.reset (new MatchSubstringReferenceNode (ex1, this, i - 1));

  } else if (ex.test ("{")) {

    n.reset (new ArrayExpressionNode (ex1));

    if (! ex.test ("}")) {

      do {

        ExpressionParserContext ex2 = ex;
        std::unique_ptr<ExpressionNode> k;
        eval_top (ex, k);
        n->add_child (k.release ());

        if (ex.test ("=>")) {
          std::unique_ptr<ExpressionNode> v;
          eval_top (ex, v);
          n->add_child (v.release ());
        } else {
          n->add_child (new ConstantExpressionNode (ex2, tl::Variant ()));
        }

        if (ex.test ("}")) {
          break;
        } else if (! ex.test (",")) {
          throw EvalError (tl::to_string (tr ("Expected closing bracket ']'")), ex);
        }

      } while (true);

    }

  } else if (ex.test ("0x")) {

    long x = 0;
    while (! ex.at_end ()) {
      if (isdigit (*ex) || (tolower (*ex) <= 'f' && tolower (*ex) >= 'a')) {
        if ((x * 16) / 16 != x) {
          throw EvalError (tl::to_string (tr ("Hexadecimal number overflow")), ex1);
        }
        x *= 16;
        if (isdigit (*ex)) {
          x += long (*ex - '0');
        } else {
          x += long (tolower (*ex) - 'a' + 10);
        }
        ++ex;
      } else {
        break;
      }
    }

    n.reset (new ConstantExpressionNode (ex1, tl::Variant (x)));

  } else if (ex.try_read (g)) {

    bool dbu_units = false;

    if (ex.test ("um2") || ex.test("micron2") || ex.test ("mic2")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1.0 / (ctx_handler ()->dbu () * ctx_handler ()->dbu ());
      }
    } else if (ex.test ("nm2")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1e-6 / (ctx_handler ()->dbu () * ctx_handler ()->dbu ());
      }
    } else if (ex.test ("mm2")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1e6 / (ctx_handler ()->dbu () * ctx_handler ()->dbu ());
      }
    } else if (ex.test ("m2")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1e12 / (ctx_handler ()->dbu () * ctx_handler ()->dbu ());
      }
    } else if (ex.test ("bs")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 0.005 / ctx_handler ()->dbu ();
      }
    } else if (ex.test ("nm")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1e-3 / ctx_handler ()->dbu ();
      }
    } else if (ex.test ("um") || ex.test("micron") || ex.test ("mic")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1.0 / ctx_handler ()->dbu ();
      }
    } else if (ex.test ("mm")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1e3 / ctx_handler ()->dbu ();
      }
    } else if (ex.test ("m")) {
      dbu_units = true;
      if (ctx_handler ()) {
        g *= 1e6 / ctx_handler ()->dbu ();
      }
    }

    if (m_sloppy) {

      if (dbu_units && ! ctx_handler ()) {
        n.reset (new ConstantExpressionNode (ex1, tl::Variant ()));
      } else {
        n.reset (new ConstantExpressionNode (ex1, tl::Variant (g)));
      }

    } else {

      if (dbu_units && ! ctx_handler ()) {
        throw EvalError (tl::to_string (tr ("Length or area value with unit requires a layout context")), ex1);
      }

      if (dbu_units) {
        //  round to integers and check whether that is possible
        double gg = g;
        g = floor (0.5 + g);
        if (fabs (g) < 1e12 && fabs (g - gg) > 1e-3) {
          throw EvalError (tl::to_string (tr ("Value is not a multiple of the database unit")), ex1);
        }
      }

      n.reset (new ConstantExpressionNode (ex1, tl::Variant (g)));

    }

  } else if (ex.try_read_quoted (t)) {

    n.reset (new ConstantExpressionNode (ex1, tl::Variant (t)));

  } else if (ex.try_read_word (t, "_")) {

    ExpressionParserContext ex2 = ex;

    //  for a function: collect the parameter or check if it's an assignment
    std::vector <tl::Variant> vv;

    const EvalFunction *function = 0;
    const tl::Variant *value = 0;
    tl::Variant *var = 0;

    if (am == 2) {
      resolve_var_name (t, var);
      if (! var) {
        set_var (t, tl::Variant ());
        resolve_var_name (t, var);
      }
    } else {
      resolve_name (t, function, value, var);
    }

    if (function) {

      n.reset (new StaticFunctionExpressionNode (ex1, function));

      ExpressionParserContext exb = ex;
      if (exb.test ("(")) {

        //  for interpolation we must not eat white spaces.
        ex = exb;

        if (! ex.test (")")) {

          do {

            std::unique_ptr<ExpressionNode> v;
            eval_top (ex, v);
            n->add_child (v.release ());

            if (ex.test (")")) {
              break;
            } else if (! ex.test (",")) {
              throw EvalError (tl::to_string (tr ("Expected closing bracket ')'")), ex);
            }

          } while (true);

        }

      }

    } else if (value) {
      n.reset (new RVariableExpressionNode (ex1, value));
    } else if (var) {
      n.reset (new LVariableExpressionNode (ex1, var));
    } else if (m_sloppy) {
      n.reset (new ConstantExpressionNode (ex1, tl::Variant ()));
    } else {
      throw EvalError (tl::to_string (tr ("Unknown variable or function")) + " '" + t + "'", ex1);
    }

  } else {
    throw EvalError (tl::to_string (tr ("Expected constant, function or bracket expression")), ex1);
  }
}

void 
Eval::resolve_var_name (const std::string &t, tl::Variant *&value) 
{
  value = 0;

  std::map<std::string, tl::Variant>::iterator v;
  v = m_local_vars.find (t);
  if (v != m_local_vars.end ()) {
    value = &v->second;
  }
}

void 
Eval::resolve_name (const std::string &t, const EvalFunction *&function, const tl::Variant *&value, tl::Variant *&var)
{
  function = 0;
  value = 0;
  var = 0;

  std::map <std::string, EvalFunction *>::const_iterator f;
  f = m_local_functions.find (t);
  if (f != m_local_functions.end ()) {
    function = f->second;
  } else if ((function = EvalStaticFunction::function_by_name (t)) == 0) {
    std::map<std::string, tl::Variant>::iterator v;
    v = m_local_vars.find (t);
    if (v != m_local_vars.end ()) {
      var = &v->second;
    } else {
      value = EvalStaticConstant::constant_by_name (t);
    }
  }

  if (! function && ! value && ! var) {
    if (mp_parent) {
      mp_parent->resolve_name (t, function, value, var);
    } else if (mp_global) {
      mp_global->resolve_name (t, function, value, var);
    }
  }
}

tl::Variant 
Eval::eval (const std::string &s)
{
  Expression expr;
  parse (expr, s, true);

  EvalTarget v;
  expr.execute (v);
  return v.make_result ();
}

void
Eval::parse (Expression &expr, const std::string &s, bool top)
{
  expr = Expression (this, s);

  tl::Extractor ex (s.c_str ());
  tl::Extractor ex0 = ex;
  ExpressionParserContext context (&expr, ex);

  if (top) {
    eval_top (context, expr.root ());
  } else {
    eval_atomic (context, expr.root (), 0);
  }

  context.expect_end ();
}

void 
Eval::parse (Expression &expr, tl::Extractor &ex, bool top)
{
  expr = Expression (this, ex.get ());

  tl::Extractor ex0 = ex;
  ExpressionParserContext context (&expr, ex);

  if (top) {
    eval_top (context, expr.root ());
  } else {
    eval_atomic (context, expr.root (), 0);
  }

  expr.set_text (std::string (ex0.get (), ex.get () - ex0.get ())); 

  ex = context;
}

std::string 
Eval::parse_expr (tl::Extractor &ex, bool top)
{
  tl::Eval eval (0, true);
  Expression expr (&eval, ex.get ());

  tl::Extractor ex0 = ex;
  ExpressionParserContext context (&expr, ex);

  std::unique_ptr<ExpressionNode> n;
  if (top) {
    eval.eval_top (context, n);
  } else {
    eval.eval_atomic (context, n, 0);
  }

  ex = context;

  return std::string (ex0.get (), ex.get () - ex0.get ()); 
}

std::string 
Eval::interpolate (const std::string &str)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os.precision(8);

  tl::Extractor ex (str.c_str ());

  while (*ex) {
    if (*ex == '$') {
      ++ex;
      if (*ex == '$') {
        os << '$';
        ++ex;
      } else {

        EvalTarget v;
        try {

          Expression expr;
          parse (expr, ex, false);
          expr.execute (v);

          //  use default precision instead of full precision of to_string ..
          if (v->is_double ()) {
            os << v->to_double();
          } else {
            os << v->to_string ();
          }

        } catch (tl::Exception &ex) {
          os << "[Error: " << ex.msg () << "]";
        }

      }
    } else {
      os << *ex;
      ++ex;
    }
  }

  return os.str ();
}

}

