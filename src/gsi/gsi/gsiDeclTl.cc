
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "tlLog.h"
#include "tlTimer.h"
#include "tlProgress.h"
#include "tlExpression.h"
#include "tlGlobPattern.h"

// ----------------------------------------------------------------
//  Logger binding

namespace gsi 
{

/**
 *  @brief A pseudo class that wraps the logger functionality
 */
class Logger
{
public:
  static void out (tl::Channel *channel, const char *msg)
  {
    *channel << msg;
  }

  static void info (const char *msg)
  {
    out (&tl::info, msg);
  }

  static void log (const char *msg)
  {
    out (&tl::log, msg);
  }

  static void warn (const char *msg)
  {
    out (&tl::warn, msg);
  }

  static void error (const char *msg)
  {
    out (&tl::error, msg);
  }

  static int verbosity () 
  {
    return tl::verbosity ();
  }

  static void set_verbosity (int v)
  {
    tl::verbosity (v);
  }
};

}

namespace tl {
  template <> struct type_traits<gsi::Logger> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };
}

namespace gsi
{

Class<Logger> decl_Logger ("tl", "Logger",
  gsi::method ("info", &Logger::info, 
    "@brief Writes the given string to the info channel\n"
    "@args msg\n"
    "\n"
    "The info channel is printed as neutral messages unconditionally.\n"
  ) +
  gsi::method ("error", &Logger::error, 
    "@brief Writes the given string to the error channel\n"
    "@args msg\n"
    "\n"
    "The error channel is formatted as an error (i.e. red in the logger window) and output unconditionally.\n"
  ) +
  gsi::method ("warn", &Logger::warn, 
    "@brief Writes the given string to the warning channel\n"
    "@args msg\n"
    "\n"
    "The warning channel is formatted as a warning (i.e. blue in the logger window) and output unconditionally.\n"
  ) +
  gsi::method ("log", &Logger::log, 
    "@brief Writes the given string to the log channel\n"
    "@args msg\n"
    "\n"
    "Log messages are printed as neutral messages and are output only if the verbosity is above 0.\n"
  ) +
  gsi::method ("verbosity", &Logger::verbosity, 
    "@brief Returns the verbosity level\n"
    "\n"
    "The verbosity level is defined by the application (see -d command line option for example). "
    "Level 0 is silent, levels 10, 20, 30 etc. denote levels with increasing verbosity. "
    "11, 21, 31 .. are sublevels which also enable timing logs in addition to messages."
  ) +
  gsi::method ("verbosity=", &Logger::set_verbosity, 
    "@brief Sets the verbosity level for the application\n"
    "@args v\n"
    "\n"
    "See \\verbosity for a definition of the verbosity levels. Please note that this method "
    "changes the verbosity level for the whole application.\n"
  ),
  "@brief A logger\n"
  "\n"
  "The logger outputs messages to the log channels. If the log viewer is open, the "
  "log messages will be shown in the logger view. Otherwise they will be printed to the terminal "
  "on Linux for example.\n"
  "\n"
  "A code example:\n"
  "\n"
  "@code\n"
  "RBA::Logger::error(\"An error message\")\n"
  "RBA::Logger::warn(\"A warning\")\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}

// ----------------------------------------------------------------
//  Timer binding

namespace gsi
{

static std::string timer_to_s (const tl::Timer *timer)
{
  return tl::sprintf ("%.12gs (sys), %.12gs (user), %.12gs (wall)", timer->sec_sys (), timer->sec_user (), timer->sec_wall ());
}

Class<tl::Timer> decl_Timer ("tl", "Timer",
  gsi::method ("user", &tl::Timer::sec_user, 
    "@brief Returns the elapsed CPU time in user mode from start to stop in seconds\n"
  ) +
  gsi::method ("sys", &tl::Timer::sec_sys, 
    "@brief Returns the elapsed CPU time in kernel mode from start to stop in seconds\n"
  ) +
  gsi::method ("wall", &tl::Timer::sec_wall,
    "@brief Returns the elapsed real time from start to stop in seconds\n"
    "This method has been introduced in version 0.26."
  ) +
  gsi::method_ext ("to_s", &timer_to_s,
    "@brief Produces a string with the currently elapsed times\n"
  ) +
  gsi::method ("start", &tl::Timer::start, 
    "@brief Starts the timer\n"
  ) +
  gsi::method ("stop", &tl::Timer::stop, 
    "@brief Stops the timer\n"
  ),
  "@brief A timer (stop watch)\n"
  "\n"
  "The timer provides a way to measure CPU time. It provides two basic methods: start and stop. "
  "After it has been started and stopped again, the time can be retrieved using the user and sys "
  "attributes, i.e.:\n"
  "\n"
  "@code\n"
  "t = RBA::Timer::new\n"
  "t.start\n"
  "# ... do something\n"
  "t.stop\n"
  "puts \"it took #{t.sys} seconds (kernel), #{t.user} seconds (user) on the CPU\"\n"
  "@/code\n"
  "\n"
  "The time is reported in seconds.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}

// ----------------------------------------------------------------
//  Progress reporter objects

namespace tl {

  template <> struct type_traits<tl::Progress> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };

  template <> struct type_traits<tl::RelativeProgress> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };

  template <> struct type_traits<tl::AbsoluteProgress> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };

}

namespace gsi 
{
  
Class<tl::Progress> decl_Progress ("tl", "Progress",
  gsi::method ("desc=", &tl::Progress::set_desc, 
    "@brief Sets the description text of the progress object\n"
    "@args desc\n"
  ) +
  gsi::method ("desc", &tl::Progress::desc, 
    "@brief Gets the description text of the progress object\n"
  ) +
  gsi::method ("title=", &tl::Progress::set_desc, 
    "@brief Sets the title text of the progress object\n"
    "@args title\n"
    "\n"
    "Initially the title is equal to the description.\n"
  ),
  "@brief A progress reporter\n"
  "\n"
  "This is the base class for all progress reporter objects. Progress reporter objects are used "
  "to report the progress of some operation and to allow aborting an operation. "
  "Progress reporter objects must be triggered periodically, i.e. a value must be set. "
  "On the display side, a progress bar usually is used to represent the progress of an operation.\n"
  "\n"
  "Actual implementations of the progress reporter class are \\RelativeProgress and \\AbsoluteProgress.\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

static tl::RelativeProgress *rel_progress_2 (const std::string &desc, size_t max)
{
  return new tl::RelativeProgress (desc, max);
}

static tl::RelativeProgress *rel_progress_3 (const std::string &desc, size_t max, size_t yield_interval)
{
  return new tl::RelativeProgress (desc, max, yield_interval);
}

static void rel_progress_set_1 (tl::RelativeProgress *progress, size_t value)
{
  progress->set (value);
}

static void rel_progress_set_2 (tl::RelativeProgress *progress, size_t value, bool force_yield)
{
  progress->set (value, force_yield);
}

Class<tl::RelativeProgress> decl_RelativeProgress (decl_Progress, "tl", "RelativeProgress",
  gsi::constructor ("new", &rel_progress_2,
    "@brief Creates a relative progress reporter with the given description and maximum value\n"
    "@args desc, max_value\n"
    "\n"
    "The reported progress will be 0 to 100% for values between 0 and the maximum value.\n"
    "The values are always integers. Double values cannot be used property.\n"
  ) + 
  gsi::constructor ("new", &rel_progress_3,
    "@brief Creates a relative progress reporter with the given description and maximum value\n"
    "@args desc, max_value, yield_interval\n"
    "\n"
    "The reported progress will be 0 to 100% for values between 0 and the maximum value.\n"
    "The values are always integers. Double values cannot be used property.\n"
    "\n"
    "The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, "
    "the event loop will be executed every tenth call of \\inc or \\set.\n"
  ) + 
  gsi::method ("format=", &tl::RelativeProgress::set_format, 
    "@brief sets the output format (sprintf notation) for the progress text\n"
    "@args format\n"
  ) +
  gsi::method ("inc", &tl::RelativeProgress::operator++, 
    "@brief Increments the progress value\n"
  ) +
  gsi::method_ext ("value=", &rel_progress_set_1,
    "@brief Sets the progress value\n"
    "@args value\n"
  ) + 
  gsi::method_ext ("set", &rel_progress_set_2,
    "@brief Sets the progress value\n"
    "@args value, force_yield\n"
    "\n"
    "This method is equivalent to \\value=, but it allows forcing the event loop to be triggered.\n"
    "If \"force_yield\" is true, the event loop will be triggered always, irregardless of the yield interval specified in the constructor.\n"
  ),
  "@brief A progress reporter counting progress in relative units\n"
  "\n"
  "A relative progress reporter counts from 0 to some maximum value representing 0 to 100 percent completion of a task. "
  "The progress can be configured to have a description text, a title and a format.\n"
  "The \"inc\" method increments the value, the \"set\" or \"value=\" methods set the value to a specific value.\n"
  "\n"
  "While one of these three methods is called, they will run the event loop in regular intervals. That makes the application "
  "respond to mouse clicks, specifically the Cancel button on the progress bar. If that button is clicked, an exception will be "
  "raised by these methods.\n"
  "\n"
  "The progress object must be destroyed explicitly in order to remove the progress status bar.\n"
  "\n"
  "A code example:\n"
  "\n"
  "@code\n"
  "p = RBA::RelativeProgress::new(\"test\", 10000000)\n"
  "begin\n"
  "  10000000.times { p.inc }\n"
  "ensure\n"
  "  p.destroy\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

static tl::AbsoluteProgress *abs_progress_1 (const std::string &desc)
{
  return new tl::AbsoluteProgress (desc);
}

static tl::AbsoluteProgress *abs_progress_2 (const std::string &desc, size_t yield_interval)
{
  return new tl::AbsoluteProgress (desc, yield_interval);
}

static void abs_progress_set_1 (tl::AbsoluteProgress *progress, size_t value)
{
  progress->set (value);
}

static void abs_progress_set_2 (tl::AbsoluteProgress *progress, size_t value, bool force_yield)
{
  progress->set (value, force_yield);
}

Class<tl::AbsoluteProgress> decl_AbsoluteProgress (decl_Progress, "tl", "AbsoluteProgress",
  gsi::constructor ("new", &abs_progress_1,
    "@brief Creates an absolute progress reporter with the given description\n"
    "@args desc, max_value\n"
  ) + 
  gsi::constructor ("new", &abs_progress_2,
    "@brief Creates an absolute progress reporter with the given description\n"
    "@args desc, max_value, yield_interval\n"
    "\n"
    "The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, "
    "the event loop will be executed every tenth call of \\inc or \\set.\n"
  ) + 
  gsi::method ("format=", &tl::AbsoluteProgress::set_format, 
    "@brief sets the output format (sprintf notation) for the progress text\n"
    "@args format\n"
  ) +
  gsi::method ("unit=", &tl::AbsoluteProgress::set_unit, 
    "@brief Sets the unit\n"
    "@args unit\n"
    "\n"
    "Specifies the count value corresponding to 1 percent on the "
    "progress bar. By default, the current value divided by the unit "
    "is used to create the formatted value from the output string. "
    "Another attribute is provided (\\format_unit=) to specify "
    "a separate unit for that purpose.\n"
  ) + 
  gsi::method ("format_unit=", &tl::AbsoluteProgress::set_format_unit, 
    "@brief Sets the format unit\n"
    "@args unit\n"
    "\n"
    "This is the unit used for formatted output.\n"
    "The current count is divided by the format unit to render\n"
    "the value passed to the format string.\n"
  ) +
  gsi::method ("inc", &tl::AbsoluteProgress::operator++, 
    "@brief Increments the progress value\n"
  ) +
  gsi::method_ext ("value=", &abs_progress_set_1,
    "@brief Sets the progress value\n"
    "@args value\n"
  ) + 
  gsi::method_ext ("set", &abs_progress_set_2,
    "@brief Sets the progress value\n"
    "@args value, force_yield\n"
    "\n"
    "This method is equivalent to \\value=, but it allows forcing the event loop to be triggered.\n"
    "If \"force_yield\" is true, the event loop will be triggered always, irregardless of the yield interval specified in the constructor.\n"
  ),
  "@brief A progress reporter counting progress in absolute units\n"
  "\n"
  "An absolute progress reporter counts from 0 upwards without a known limit. "
  "A unit value is used to convert the value to a bar value. One unit corresponds to 1% on the bar.\n"
  "For formatted output, a format string can be specified as well as a unit value by which the "
  "current value is divided before it is formatted.\n"
  "\n"
  "The progress can be configured to have a description text, a title and a format.\n"
  "The \"inc\" method increments the value, the \"set\" or \"value=\" methods set the value to a specific value.\n"
  "\n"
  "While one of these three methods is called, they will run the event loop in regular intervals. That makes the application "
  "respond to mouse clicks, specifically the Cancel button on the progress bar. If that button is clicked, an exception will be "
  "raised by these methods.\n"
  "\n"
  "The progress object must be destroyed explicitly in order to remove the progress status bar.\n"
  "\n"
  "The following sample code creates a progress bar which displays the current count as \"Megabytes\".\n"
  "For the progress bar, one percent corresponds to 16 kByte:\n"
  "\n"
  "@code\n"
  "p = RBA::AbsoluteProgress::new(\"test\")\n"
  "p.format = \"%.2f MBytes\"\n"
  "p.unit = 1024*16\n"
  "p.format_unit = 1024*1024\n"
  "begin\n"
  "  10000000.times { p.inc }\n"
  "ensure\n"
  "  p.destroy\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.23.\n"
);

}

// ----------------------------------------------------------------
//  Expressions

namespace
{

/**
 *  @brief A convenience wrapper for the expression parser
 */
class ExpressionWrapper
  : public tl::Object
{
public:
  ExpressionWrapper ()
    : mp_eval (new tl::Eval ())
  {
  }

  void parse (const std::string &e)
  {
    mp_expr.reset (0);

    std::auto_ptr<tl::Expression> ex (new tl::Expression ());
    mp_eval->parse (*ex, e);
    mp_expr.reset (ex.release ());
  }

  void set_var (const std::string &name, const tl::Variant &value)
  {
    mp_eval->set_var (name, value);
  }

  static void set_global_var (const std::string &name, const tl::Variant &value)
  {
    tl::Eval::set_global_var (name, value);
  }

  tl::Variant eval ()
  {
    if (mp_expr.get ()) {
      return mp_expr->execute ();
    } else {
      return tl::Variant ();
    }
  }

private:
  std::auto_ptr<tl::Expression> mp_expr;
  std::auto_ptr<tl::Eval> mp_eval;
};

tl::Variant eval_expr (const std::string &e)
{
  ExpressionWrapper expr;
  expr.parse (e);
  return expr.eval ();
}

ExpressionWrapper *new_expr1 (const std::string &e)
{
  std::auto_ptr<ExpressionWrapper> expr (new ExpressionWrapper ());
  expr->parse (e);
  return expr.release ();
}

ExpressionWrapper *new_expr2 (const std::string &e, const std::map<std::string, tl::Variant> &variables)
{
  std::auto_ptr<ExpressionWrapper> expr (new ExpressionWrapper ());
  for (std::map<std::string, tl::Variant>::const_iterator v = variables.begin (); v != variables.end (); ++v) {
    expr->set_var (v->first, v->second);
  }
  expr->parse (e);
  return expr.release ();
}

}

namespace tl {

  template <> struct type_traits<ExpressionWrapper> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::true_tag has_default_constructor;
  };

}

namespace gsi
{

Class<ExpressionWrapper> decl_ExpressionWrapper ("tl", "Expression",
  gsi::constructor ("new", &new_expr1, gsi::arg ("expr"),
    "@brief Creates an expression evaluator\n"
  ) +
  gsi::constructor ("new", &new_expr2, gsi::arg ("expr"), gsi::arg ("variables"),
    "@brief Creates an expression evaluator\n"
    "This version of the constructor takes a hash of variables available to the expressions."
  ) +
  gsi::method ("text=", &ExpressionWrapper::parse, gsi::arg ("expr"),
    "@brief Sets the given text as the expression."
  ) +
  gsi::method ("var", &ExpressionWrapper::set_var, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Defines a variable with the given name and value\n"
  ) +
  gsi::method ("global_var", &ExpressionWrapper::set_global_var, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Defines a global variable with the given name and value\n"
  ) +
  gsi::method ("eval", &ExpressionWrapper::eval,
    "@brief Evaluates the current expression and returns the result\n"
  ) +
  gsi::method ("eval", &eval_expr, gsi::arg ("expr"),
    "@brief A convience function to evaluate the given expression and directly return the result\n"
    "This is a static method that does not require instantiation of the expression object first."
  ),
  "@brief Evaluation of Expressions\n"
  "\n"
  "This class allows evaluation of expressions. Expressions are used in many places throughout KLayout and "
  "provide computation features for various applications. Having a script language, there is no real use for expressions "
  "inside a script client. This class is provided mainly for testing purposes.\n"
  "\n"
  "An expression is 'compiled' into an Expression object and can be evaluated multiple times.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
);


static tl::GlobPattern *new_glob_pattern (const std::string &s)
{
  return new tl::GlobPattern (s);
}

namespace {

template <class Iter>
struct to_var_iterator
  : public Iter
{
  typedef typename Iter::value_type original_value_type;
  typedef typename tl::Variant *pointer;
  typedef typename tl::Variant &reference;
  typedef typename Iter::difference_type difference_type;

  to_var_iterator (const Iter &iter)
    : Iter (iter)
  { }

  pointer operator-> ()
  {
    m_var = tl::Variant (Iter::operator* ());
    return &m_var;
  }

  reference operator* ()
  {
    m_var = tl::Variant (Iter::operator* ());
    return m_var;
  }

private:
  tl::Variant m_var;
};

}

static tl::Variant match (const tl::GlobPattern *pattern, const std::string &s)
{
  std::vector<std::string> brackets;
  if (pattern->match (s, brackets)) {
    return tl::Variant (to_var_iterator<std::vector<std::string>::const_iterator> (brackets.begin ()), to_var_iterator<std::vector<std::string>::const_iterator> (brackets.end ()));
  } else {
    return tl::Variant ();
  }
}

Class<tl::GlobPattern> decl_GlobPattern ("tl", "GlobPattern",
  gsi::constructor ("new", &new_glob_pattern, gsi::arg ("pattern"),
    "@brief Creates a new glob pattern match object\n"
  ) +
  gsi::method ("case_sensitive=", &tl::GlobPattern::set_case_sensitive, gsi::arg ("case_sensitive"),
    "@brief Sets a value indicating whether the glob pattern match is case sensitive."
  ) +
  gsi::method ("case_sensitive", &tl::GlobPattern::case_sensitive,
    "@brief Gets a value indicating whether the glob pattern match is case sensitive."
  ) +
  gsi::method ("head_match=", &tl::GlobPattern::set_header_match, gsi::arg ("head_match"),
    "@brief Sets a value indicating whether trailing characters are allowed.\n"
    "If this predicate is false, the glob pattern needs to match the full subject string. "
    "If true, the match function will ignore trailing characters and return true if the "
    "front part of the subject string matches."
  ) +
  gsi::method ("head_match", &tl::GlobPattern::header_match,
    "@brief Gets a value indicating whether trailing characters are allowed.\n"
  ) +
  gsi::method_ext ("match", &match, gsi::arg ("subject"),
    "@brief Matches the subject string against the pattern.\n"
    "Returns nil if the subject string does not match the pattern. Otherwise returns a list "
    "with the substrings captured in round brackets."
  ),
  "@brief A glob pattern matcher\n"
  "This class is provided to make KLayout's glob pattern matching available to scripts too. "
  "The intention is to provide an implementation which is compatible with KLayout's pattern "
  "syntax.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}
