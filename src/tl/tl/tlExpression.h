
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



#ifndef HDR_tlExpression
#define HDR_tlExpression

#include "tlCommon.h"

#include "tlException.h"
#include "tlVariant.h"
#include "tlString.h"

#include <map>
#include <vector>
#include <memory>

namespace tl 
{

class Eval;
class EvalTarget;
class Expression;
class ExpressionNode;
class ExpressionParserContext;

/**
 *  @brief An interface handling the evaluation context
 * 
 *  This object serves to provide extended context for the expressions:
 *
 *  First, this object is supposed to replace angle-bracket expressions
 *  of the kind <something> and <<something>> by a real value.
 *  The handler can be configured through Eval::set_ctx_handler.
 *
 *  Second, this object provides the database unit value for physical unit conversions.
 */
class TL_PUBLIC ContextHandler
{
public:
  /**
   *  @brief Constructor
   */
  ContextHandler()
  {
  }

  /**
   *  @brief Destructor
   */
  virtual ~ContextHandler()
  {
  }

  /**
   *  @brief Evaluates a single-bracket expression
   *
   *  This method receives the content of a single bracket and is supposed to
   *  deliver an evaluated value.
   */
  virtual tl::Variant eval_bracket (const std::string &content) const = 0;

  /**
   *  @brief Evaluates a double-bracket expression
   *
   *  This method receives the content of a double bracket and is supposed to
   *  deliver an evaluated value.
   */
  virtual tl::Variant eval_double_bracket (const std::string &content) const = 0;

  /**
   *  @brief Provide the database unit value
   */
  virtual double dbu () const = 0;
};

/**
 *  @brief An exception thrown by the evaluation
 */
class TL_PUBLIC EvalError 
  : public tl::Exception
{
public:
  EvalError (const std::string &what, const ExpressionParserContext &context);
};

/**
 *  @brief An exception indicating that no such method exists
 */
class TL_PUBLIC NoMethodError 
  : public EvalError
{
public:
  NoMethodError (const std::string &cls_name, const std::string &method, const ExpressionParserContext &context);
};

/**
 *  @brief The expression parser context
 */
class TL_PUBLIC ExpressionParserContext
  : public tl::Extractor
{
public:
  /**
   *  @brief Default constructor
   */
  ExpressionParserContext ();

  /**
   *  @brief Constructor
   *
   *  @param expr The expression to which this context refers to
   *  @param ex The initial location of the parser
   */
  ExpressionParserContext (const Expression *expr, const tl::Extractor &ex);

  /**
   *  @brief Reimplementation of tl::Extractor's error method
   */
  virtual void error (const std::string &message);

  /**
   *  @brief Gets a string indication where we are currently
   */
  std::string where () const;

  /**
   *  @brief Sets the expression parent
   */
  void set_expr (const Expression *expr)
  {
    mp_expr = expr;
  }

private:
  const Expression *mp_expr;
  tl::Extractor m_ex0;
};

/**
 *  @brief A node within an expression tree
 */
class TL_PUBLIC ExpressionNode
{
public:
  /**
   *  @brief Constructor
   */
  ExpressionNode (const ExpressionParserContext &context);

  /**
   *  @brief Constructor with reservation of a certain number of child nodes
   */
  ExpressionNode (const ExpressionParserContext &context, size_t children);

  /**
   *  @brief Copy ctor
   */
  ExpressionNode (const ExpressionNode &other, const tl::Expression *expr);

  /**
   *  @brief Destructor
   */
  virtual ~ExpressionNode ();

  /**
   *  @brief Add a child node
   */
  void add_child (ExpressionNode *node); 

  /**
   *  @brief Execute the node
   */
  virtual void execute (EvalTarget &out) const = 0; 

  /**
   *  @brief Clone the node
   */
  virtual ExpressionNode *clone (const tl::Expression *expr) const = 0;

protected:
  std::vector <ExpressionNode *> m_c;
  ExpressionParserContext m_context;

  /**
   *  @brief Sets the expression parent
   */
  void set_expr (const tl::Expression *expr)
  {
    m_context.set_expr (expr);
  }
};

/**
 *  @brief A class handler for user objects within tl::Variant
 *
 *  In order to enable objects for expressions, the user object in tl::Variant must be provided with 
 *  a class derived from tl::VariantUserClassBase which implements eval_cls to return an EvalClass
 *  implementation which executes the method.
 */
class TL_PUBLIC EvalClass
{
public:
  /**
   *  @brief Constructor 
   *
   *  @param test_function_name The name of the function which will be created and which tests if the variant is of the given type.
   */
  EvalClass () { }

  /**
   *  @brief Destructor
   */
  virtual ~EvalClass () { }
    
  /** 
   *  @brief Execute the method with the given name on the object
   *
   *  @param node The current location in the syntax tree
   *  @param out The return value 
   *  @param object The object on which to execute the method
   *  @param method The name of the method
   *  @param args The arguments of the method
   *
   *  If no method of this kind exists, the implementation may throw a NoMethodError.
   */
  virtual void execute (const ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const = 0;
};

/**
 *  @brief A base class for a function 
 */
class TL_PUBLIC EvalFunction
{
public:
  /**
   *  @brief Constructor
   */
  EvalFunction () { }

  /**
   *  @brief Destructor
   */
  virtual ~EvalFunction () { }

  /**
   *  @brief The actual execution method
   *
   *  @param ex The position inside the current expression
   *  @param args The arguments of the method
   *  @return The return value 
   */
  virtual void execute (const ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args) const = 0;
};

/**
 *  @brief Represents a expression to evaluate
 */
class TL_PUBLIC Expression
{
public:
  /**
   *  @brief Default constructor
   */
  Expression ();

  /**
   *  @brief Copy constructor
   */
  Expression (const Expression &d);

  /**
   *  @brief Assignment
   */
  Expression &operator= (const Expression &d);

  /**
   *  @brief Execution of the expression
   */
  tl::Variant execute () const;

  /**
   *  @brief Execution of the expression (return by reference)
   */
  void execute (EvalTarget &v) const;

  /**
   *  @brief Gets the text of the expression
   */
  const char *text () const
  {
    return mp_text != 0 ? mp_text : m_local_text.c_str ();
  }

  /**
   *  @brief Sets the local text of the expression
   */
  void set_text (const std::string &s)
  {
    m_local_text = s;
  }

  /**
   *  @brief Sets the external text of the expression
   */
  void set_text (const char *s)
  {
    mp_text = s;
  }

private:
  const char *mp_text;
  std::string m_local_text;
  std::unique_ptr<ExpressionNode> m_root;
  Eval *mp_eval;

  friend class Eval;

  /**
   *  @brief Private constructor for Eval 
   */
  Expression (Eval *eval, const std::string &expr);

  /**
   *  @brief Private constructor for Eval 
   */
  Expression (Eval *eval, const char *expr);

  /**
   *  @brief Accessor to the root node
   */
  std::unique_ptr<ExpressionNode> &root () 
  {
    return m_root;
  }
};

/**
 *  @brief Provides the context for the expression parser and evaluation
 */
class TL_PUBLIC Eval
{
public:
  /**
   *  @brief Create a new object for expression evaluation
   *
   *  @param parent The parent evaluation context
   *  @param sloppy True to enable sloppy evaluation for pure parsing
   */
  explicit Eval (Eval *parent = 0, bool sloppy = false);

  /**
   *  @brief Create a new object for expression evaluation
   *
   *  @param global The global evaluation context
   *  @param parent The parent evaluation context
   *  @param sloppy True to enable sloppy evaluation for pure parsing
   */
  explicit Eval (Eval *global, Eval *parent, bool sloppy = false);

  /**
   *  @brief virtual dtor to enable dynamic_cast on derived classes.
   */
  virtual ~Eval ();

  /**
   *  @brief Sets an angle-bracket handler
   *
   *  See \ContextHandler for details.
   *  This method will not take ownership over the object.
   */
  void set_ctx_handler (const ContextHandler *ctx_handler)
  {
    mp_ctx_handler = ctx_handler;
  }

  /**
   *  @brief Gets the context handler
   *
   *  If no handler is set locally, the parent context is looked up for one.
   *  If no context layout is present, 0 is returned.
   */
  const ContextHandler *ctx_handler () const
  {
    if (mp_ctx_handler) {
      return mp_ctx_handler;
    } else if (mp_parent) {
      return mp_parent->ctx_handler ();
    } else {
      return 0;
    }
  }

  /**
   *  @brief Define a global function for use within an expression
   */
  static void define_global_function (const std::string &name, EvalFunction *function)
  {
    m_global.define_function (name, function);
  }

  /**
   *  @brief Define a function for use within an expression
   */
  void define_function (const std::string &name, EvalFunction *function);

  /**
   *  @brief Define a global variable for use within an expression
   */
  static void set_global_var (const std::string &name, const tl::Variant &var)
  {
    m_global.set_var (name, var);
  }

  /**
   *  @brief Define a variable for use within an expression
   */
  void set_var (const std::string &name, const tl::Variant &var);

  /**
   *  @brief Parse an expression from the extractor
   *
   *  @param ex The extractor from which to parse the expression
   *  @param top If true, an expression is parsed at top level (as eval does). If false, the exression is parsed at 'atomic' level (as the string interpolation after the '$' does.
   *  @param expr An expression that can be evaluated (out)
   */
  void parse (Expression &expr, tl::Extractor &ex, bool top = true);

  /**
   *  @brief Convenience method that returns the expression object (caution: poor performance)
   *
   *  @param ex The extractor from which to parse the expression
   *  @param top If true, an expression is parsed at top level (as eval does). If false, the exression is parsed at 'atomic' level (as the string interpolation after the '$' does.
   */
  Expression parse (tl::Extractor &ex, bool top = true)
  {
    Expression expr;
    parse (expr, ex, top);
    return expr;
  }

  /**
   *  @brief Parse an expression from a string
   *
   *  @param s The string from which to parse the expression
   *  @param top If true, an expression is parsed at top level (as eval does). If false, the exression is parsed at 'atomic' level (as the string interpolation after the '$' does.
   *  @return An expression string that can be passed to eval.
   */
  void parse (Expression &expr, const std::string &s, bool top = true);

  /**
   *  @brief Convenience method that returns the expression object (caution: poor performance)
   *
   *  @param s The string from which to parse the expression
   *  @param top If true, an expression is parsed at top level (as eval does). If false, the exression is parsed at 'atomic' level (as the string interpolation after the '$' does.
   */
  Expression parse (const std::string &s, bool top = true)
  {
    Expression expr;
    parse (expr, s, top);
    return expr;
  }

  /**
   *  @brief Parse an expression string from the extractor
   *
   *  @param ex The extractor from which to parse the expression
   *  @param top If true, an expression is parsed at top level (as eval does). If false, the exression is parsed at 'atomic' level (as the string interpolation after the '$' does.
   *  @return An expression string that can be used later to construct an Expression from using the parse method
   */
  static std::string parse_expr (tl::Extractor &ex, bool top = true);

  /**
   *  @brief A convenience method to evaluate an expression (by string) in this context
   */
  tl::Variant eval (const std::string &expr);

  /**
   *  @brief Interpolate the string and return the result
   *
   *  Interpolation will replace all expressions of the form
   *  '$<atomic>' by their string value.
   */
  std::string interpolate (const std::string &str);

  /**
   *  @brief Provide access to the match substrings
   */
  std::vector<std::string> &match_substrings () 
  {
    return m_match_substrings;
  }

  /**
   *  @brief Provide access to the match substrings (const version)
   */
  const std::vector<std::string> &match_substrings () const 
  {
    return m_match_substrings;
  }

private:
  friend class Expression;

  Eval *mp_parent, *mp_global;
  std::map <std::string, tl::Variant> m_local_vars;
  std::map <std::string, EvalFunction *> m_local_functions;
  bool m_sloppy;
  const ContextHandler *mp_ctx_handler;
  std::vector<std::string> m_match_substrings;

  void eval_top (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_assign (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_if (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_boolean (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_conditional (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_shift (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_addsub (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_product (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_bitwise (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_unary (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void eval_atomic (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v, int am);
  void eval_suffix (ExpressionParserContext &context, std::unique_ptr<ExpressionNode> &v);
  void resolve_name (const std::string &name, const EvalFunction *&function, const tl::Variant *&value, tl::Variant *&var);
  void resolve_var_name (const std::string &name, tl::Variant *&value);

  static Eval m_global;
};

}

#endif

